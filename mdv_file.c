/* "mdv_file.c" データファイルに関する処理 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>   /* memcpy(), strlen() */
#include <math.h>
#include <time.h>     /* ctime() */
#include <ctype.h>    /* isspace() */
#include "mdv.h"      /* mdv_atom, MDV_Coord, MDV_LineList, formatted_mode,
                         mdv_status_default, mdv_status_tmp, md_atom */
#include "mdv_util.h" /* MDV_Work_Alloc, MDV_Work_Free */
#include "mdv_text.h" /* ReadToken */
#include "mdv_file.h"
#include "machine.h"  /* Strlcat() */

/* ---- private宣言 -------------------------------------------------------- */

/* 可変長文字列 */
typedef MDV_Array MDV_String;
#define MDV_StringP(sp)          ((char *) (sp)->p)
#define MDV_String_Free(p)       MDV_Array_Free(p)
#define MDV_String_GetSize(p)    ((p)->n-1)
#define MDV_String_SetSize(p, n) MDV_Array_SetSize((p), (n)+1)
static MDV_String *MDV_String_Alloc(void);
static void MDV_String_StrCpy(MDV_String *p, const char *str);
static void MDV_String_StrCat(MDV_String *p, const char *str);

/* MDV_IO型 */
typedef struct {
  MDV_String *comment;   /* コメント */
  MDV_Array *argument;   /* 引数列 */
  MDV_Array *atom;       /* 原子リスト */
  MDV_Array *coordinate; /* 原子の座標 */
  MDV_Array *bond;       /* 結合リスト */
} MDV_IO;

static void *MDV_IO_Alloc(void);
static void MDV_IO_Free(void *p);
static void MDV_IO_Copy(void *v1, const void *v2);

/* エンディアンネス関連 */
#define BINARY_HEADERSIZE 4
static MDV_Size ReadHeader(char *header);
static MDV_Size xchg_int(MDV_Size x);
static void xchg_dbl_array(double *x, MDV_Size n);

/* ==== public定義 ========================================================= */

#define MDV_FILE_UNKNOWN 0
#define MDV_FILE_TEXT   -1
#define MDV_FILE_TEXT2  -2
#define MDV_FILE_BINARY  1
#define MDV_FILE_BINARY2 2 /* different endianness */

static int MDV_GetFileType(MDV_FILE *mfp);
static int MDV_ReadData(SeekFILE *sfp, void *val, off_t len);

static SeekFileDataFunc mdv_io_f = {
  MDV_IO_Alloc,
  MDV_IO_Free,
  MDV_IO_Copy
};

/* off_t型を10進数文字列に変換する */
#define OFF_T_STRMAX 20 /* (1<<63)-1を表せる文字数+'\0' */
static char off_t2str_p[OFF_T_STRMAX];
static const char *off_t2str(off_t val) {
  int i;

  if (val < 0)
    MDV_Fatal("off_t2str()");

  off_t2str_p[OFF_T_STRMAX-1] = '\0';
  off_t2str_p[OFF_T_STRMAX-2] = '0';
  for (i = OFF_T_STRMAX-2; val > 0 && i >= 0; i--) {
    off_t2str_p[i] = '0' + val % 10;
    val /= 10;
  }
  if (val > 0)
    MDV_Fatal("off_t2str()");

  return &off_t2str_p[i+1];
}

/* ファイルのオープン(失敗はNULL) */
MDV_FILE *MDV_FileOpen(const char *path, MDV_Size n) {
  MDV_FILE *mfp;

  if ((mfp = (MDV_FILE *) malloc(sizeof(MDV_FILE))) == NULL)
    return NULL;
  if ((mfp->sfp = SeekFileOpen(path, &mdv_io_f, MDV_ReadData, mfp)) == NULL)
    return NULL;

  /* file type */
  mfp->data_n = n;
  if (formatted_mode) {
    if ((mfp->mode = MDV_GetFileType(mfp)) == MDV_FILE_UNKNOWN)
      {fprintf(stderr, "\"%s\": Illegal format.\n", path); return NULL;}
    rewind(mfp->sfp->fp);
  } else {
    mfp->mode = MDV_FILE_TEXT2;
  }

  if (debug_mode) {
    /* debug output */
    fprintf(stderr, "MDV_FileOpen(): done.\n");
    fprintf(stderr, "path = \"%s\"\n", mfp->sfp->path);
    fprintf(stderr, "size = %s\n", off_t2str(mfp->sfp->size));
    fprintf(stderr, "date = %s", ctime(&mfp->sfp->time));
    fprintf(stderr, "mode = ");
    switch (mfp->mode) {
    case MDV_FILE_BINARY:
      fprintf(stderr, "FORTRAN binary\n");
      break;
    case MDV_FILE_BINARY2:
      fprintf(stderr, "FORTRAN binary (different endianness)\n");
      break;
    case MDV_FILE_TEXT:
      fprintf(stderr, "formatted text\n");
      break;
    case MDV_FILE_TEXT2:
      fprintf(stderr, "extended text\n");
      break;
    default:
      fprintf(stderr, "unknown (code = %d)\n", (int) mfp->mode);
    }
  }

  return mfp;
}

/* ファイルの再読み込み */
int MDV_FileReload(MDV_FILE *mfp) {
  return SeekFileReload(mfp->sfp);
}

/* ファイルのクローズ */
void MDV_FileClose(MDV_FILE *mfp) {
  if (mfp == NULL)
    return;
  SeekFileClose(mfp->sfp);
  mfp->sfp = NULL;
  mfp->mode = MDV_FILE_UNKNOWN;
  mfp->data_n = 0;
  if (debug_mode)
    fprintf(stderr, "MDV_FileClose(): done.\n");
}

/* ファイルの種類の判別 */
#ifndef STRSIZE
#define STRSIZE 256
#endif
static int MDV_GetFileType(MDV_FILE *mfp) {
  FILE *fp = mfp->sfp->fp;
  int n = mfp->data_n;
  char header[BINARY_HEADERSIZE];
  char str[STRSIZE];
  int i;

  if (mfp == NULL)
    MDV_Fatal("MDV_GetFileType()");

  rewind(fp);
  if (fread(&header, BINARY_HEADERSIZE, 1, fp) < 1)
    return MDV_FILE_UNKNOWN;
  if (ReadHeader(header) == sizeof(double)*3*n) {
    fseek(fp, sizeof(double)*3*n, SEEK_CUR);
    if (fread(&header, BINARY_HEADERSIZE, 1, fp) < 1)
      return MDV_FILE_UNKNOWN;
    if (ReadHeader(header) == sizeof(double)*3*n)
      return MDV_FILE_BINARY;
  } else if (xchg_int(ReadHeader(header)) == sizeof(double)*3*n) {
    fseek(fp, sizeof(double)*3*n, SEEK_CUR);
    if (fread(&header, BINARY_HEADERSIZE, 1, fp) < 1)
      return MDV_FILE_UNKNOWN;
    if (xchg_int(ReadHeader(header)) == sizeof(double)*3*n)
      return MDV_FILE_BINARY2;
  } else {
    rewind(fp);
    if (fgets(str, STRSIZE, fp) == NULL)
      return MDV_FILE_UNKNOWN;
    for (i = 0; i < STRSIZE && str[i] != '\0'; i++) {
      if (str[i]&0x80) /* TODO: 厳密ではない。改良すべき */
        return MDV_FILE_UNKNOWN;
    }
    return MDV_FILE_TEXT;
  }

  return MDV_FILE_UNKNOWN;
}

/* ファイルのコメント */
#define MAXCOMMENT 65536
static char comment_str[MAXCOMMENT] = "";

/* コメントを返す(TODO: 現在、nstepに関係なく最新の値を返す) */
char *GetInformation(MDV_Size i) {
  return comment_str;
}

static FILE *_readtoken_fp = NULL;
static const char *_readtoken_str = NULL;
static MDV_String *_readtoken_comment = NULL;

/* _ReadToken()の初期化 */
static void _ReadToken_Init(FILE *fp, MDV_String *comment) {
  if (_readtoken_fp != NULL || fp == NULL)
    MDV_Fatal("_ReadToken_Init()");
  comment_str[0] = '\0';
  _readtoken_fp = fp;
  _readtoken_comment = comment;
}

/* _ReadToken()の終了処理 */
static void _ReadToken_Term(void) {
  if (_readtoken_fp == NULL)
    MDV_Fatal("_ReadToken_Term()");
  if (_readtoken_comment != NULL)
    Strlcpy(comment_str, MDV_StringP(_readtoken_comment), MAXCOMMENT);
  _readtoken_fp = NULL;
  _readtoken_comment = NULL;
}

/* _SReadToken()の初期化 */
static void _SReadToken_Init(const char *str, MDV_String *comment) {
  if (_readtoken_str != NULL || str == NULL)
    MDV_Fatal("_SReadToken_Init()");
  comment_str[0] = '\0';
  _readtoken_str = str;
  _readtoken_comment = comment;
}

/* _SReadToken()の終了処理 */
static void _SReadToken_Term(void) {
  if (_readtoken_str == NULL)
    MDV_Fatal("_SReadToken_Term()");
  if (_readtoken_comment != NULL)
    Strlcpy(comment_str, MDV_StringP(_readtoken_comment), MAXCOMMENT);
  _readtoken_str = NULL;
  _readtoken_comment = NULL;
}

/* コメントを含む引数の読み込み */
static const char *_ReadToken(void) {
  int lines, last;
  const char *str;
  const char *comment;

  if ((str = ReadToken(_readtoken_fp, &lines, &last, &comment)) == NULL)
    return NULL;
  if (comment != NULL && _readtoken_comment != NULL)
    MDV_String_StrCat(_readtoken_comment, comment);

  return str;
}

/* コメントを含む引数の読み込み */
static const char *_SReadToken(void) {
  int lines, last;
  const char *str;
  const char *comment;
  const char *endp;

  if ((str = SReadToken(_readtoken_str, &endp, &lines, &last, &comment))
      == NULL) {
    _readtoken_str = endp;
    return NULL;
  }
  if (comment != NULL && _readtoken_comment != NULL)
    MDV_String_StrCat(_readtoken_comment, comment);
  _readtoken_str = endp;

  return str;
}

/* FORTRAN風のdoubleの表現をC風に直す。 */
static const char *StrF2C(const char *str) {
  static MDV_String *strf2c_p = NULL; /* 暫定版: 解放しない */
  char c, *p;
  int i;

  if (strf2c_p == NULL)
    strf2c_p = MDV_String_Alloc();
  MDV_String_StrCpy(strf2c_p, str);
  p = MDV_StringP(strf2c_p);

  for (i = 0; (c = p[i]) != '\0'; i++) {
    if (c == 'd')
      p[i] = 'e';
    else if (c == 'D')
      p[i] = 'E';
  }

  return p;
}

/* 可変長文字配列に文字列strを追加する */
static void MDV_Array_AppendString(MDV_Array *p, const char *str) {
  MDV_Size n, last;

  if (p->size != 1)
    MDV_Fatal("MDV_Array_AppendString");
  n = strlen(str);
  last = p->n;
  MDV_Array_SetSize(p, last+n+1);
  Strlcpy(((char *) p->p)+last, str, n+1);
}

/* ReadData_*のための可変長文字列 */
static MDV_String *_readdata_str = NULL;

/* テキストから1stepを読む(返り値は成功の真偽) */
static int ReadData_Text(FILE *fp, MDV_IO *iop, off_t len) {
  const char *(*f_token)(void);
  const char *str;
  char *end;
  AtomID *atom;
  MDV_Size i, j, n;
  double t;
  int ret = 0;

  if (terminate_blank_line) {
    /* 文字配列に読み込む */
    if (_readdata_str == NULL)
      _readdata_str = MDV_String_Alloc(); /* 暫定版: 解放しない */
    if (len > 0) {
      if ((MDV_Size) len != len || (size_t) len != len)
        MDV_Fatal("ReadData_Text2(): Too large data block found.");
      MDV_Array_SetSize(_readdata_str, (MDV_Size) len);
      if (fread(_readdata_str->p, (size_t) len, 1, fp) < 1)
        return 0;
    } else {
      int val;

      MDV_String_StrCpy(_readdata_str, "");
      if ((val = ReadLine(fp, &str)) <= 0)
        return 0;
      MDV_String_StrCat(_readdata_str, str);
      while ((val = ReadLine(fp, &str)) > 0)
        MDV_String_StrCat(_readdata_str, str);
      MDV_String_StrCat(_readdata_str, str);
    }
    if (iop == NULL)
      return 1;

    /* _SReadToken()の初期化 */
    MDV_String_StrCpy(iop->comment, "");
    _SReadToken_Init((const char *) _readdata_str->p, iop->comment);
    f_token = _SReadToken;
  } else {
    /* _ReadToken()の初期化 */
    if (iop != NULL) {
      MDV_String_StrCpy(iop->comment, "");
      _ReadToken_Init(fp, iop->comment);
    } else {
      _ReadToken_Init(fp, NULL);
    }
    f_token = _ReadToken;
  }

  /* 原子の対応表とサイズ */
  atom = MDV_VAtomIDP(md_atom);
  n = MDV_VAtomIDGetSize(md_atom);
  if (n <= 0)
    MDV_Fatal("MDV_Read1Step()");
  if (iop != NULL) {
    MDV_Array_SetSize(iop->atom, 0);
    MDV_Array_SetSize(iop->coordinate, n*3);
  }

  /* 粒子の座標 */
  for (i = 0; i < n; i++) {
    for (j = 0; j < 3; j++) {
      if ((str = f_token()) == NULL)
        goto term;
      t = strtod(StrF2C(str), &end);
      if (*end != '\0')
        goto term;
      if (iop != NULL)
        ((double *) iop->coordinate->p)[3*i+j] = t;
    }
  }
  ret = 1;

term:
  if (terminate_blank_line)
    _SReadToken_Term();
  else
    _ReadToken_Term();
  return ret;
}

/* 拡張テキストから1stepを読む(返り値は成功の真偽) */
static int ReadData_Text2(FILE *fp, MDV_IO *iop, off_t len) {
  const char *(*f_token)(void);
  const char *str;
  char *end;
  ArgumentID arg;
  char **optargv;
  int arg_n;
  MDV_Size i, j, n, line_n;
  double t;
  int ret = 0;

  if (terminate_blank_line) {
    /* 文字配列に読み込む */
    if (_readdata_str == NULL)
      _readdata_str = MDV_String_Alloc(); /* 暫定版: 解放しない */
    if (len > 0) {
      if ((MDV_Size) len != len || (size_t) len != len)
        MDV_Fatal("ReadData_Text2(): Too large data block found.");
      MDV_Array_SetSize(_readdata_str, (MDV_Size) len);
      if (fread(_readdata_str->p, (size_t) len, 1, fp) < 1)
        return 0;
    } else {
      int val;

      MDV_String_StrCpy(_readdata_str, "");
      if ((val = ReadLine(fp, &str)) <= 0)
        return 0;
      MDV_String_StrCat(_readdata_str, str);
      while ((val = ReadLine(fp, &str)) > 0)
        MDV_String_StrCat(_readdata_str, str);
      MDV_String_StrCat(_readdata_str, str);
    }
    if (iop == NULL)
      return 1;

    /* _SReadToken()の初期化 */
    MDV_String_StrCpy(iop->comment, "");
    _SReadToken_Init((const char *) _readdata_str->p, iop->comment);
    f_token = _SReadToken;
  } else {
    /* _ReadToken()の初期化 */
    if (iop != NULL) {
      MDV_String_StrCpy(iop->comment, "");
      _ReadToken_Init(fp, iop->comment);
    } else {
      _ReadToken_Init(fp, NULL);
    }
    f_token = _ReadToken;
  }

  /* 引数 */
  if (iop != NULL)
    MDV_Array_SetSize(iop->argument, 0);
  while ((str = f_token()) != NULL && str[0] == '-') {
    if (iop != NULL)
      MDV_Array_AppendString(iop->argument, str);
    arg = GetArgumentID(str);
    if ((arg_n = GetArgumentSize(arg, mdv_status_default)) < 0)
      goto term;
    if ((optargv = SetOptArgv(arg_n, f_token)) == NULL)
      goto term;
    if (iop != NULL) {
      for (i = 0; i < arg_n; i++)
        MDV_Array_AppendString(iop->argument, optargv[i]);
    }
  }
  if (str == NULL)
    goto term;

  /* 粒子数 */
  n = strtol(str, &end, 10);
  if (n <= 0 || *end != '\0')
    goto term;
  if (max_particle != 0 && n > max_particle) {
    fprintf(stderr, "%ld (particles) > %ld (-max option)\n",
      (long) n, (long) max_particle);
    exit(1);
  }

  /* 結合リストの大きさ */
  if ((str = f_token()) == NULL) {
    if (n == 0)
      ret = 1;
    goto term;
  }
  line_n = strtol(str, &end, 10);
  if (n <= 0 || *end != '\0') {
    line_n = 0;
  } else {
    if ((str = f_token()) == NULL)
      goto term;
  }

  /* 原子リストと粒子の座標 */
  if (iop != NULL) {
    MDV_Array_SetSize(iop->atom, 0);
    MDV_Array_SetSize(iop->coordinate, n*3);
  }
  for (i = 0; i < n; i++) {
    if (i != 0) {
      if ((str = f_token()) == NULL)
        goto term;
    }
    if (iop != NULL)
      MDV_Array_AppendString(iop->atom, str);

    for (j = 0; j < 3; j++) {
      if ((str = f_token()) == NULL)
        goto term;
      t = strtod(StrF2C(str), &end);
      if (*end != '\0')
        goto term;
      if (iop != NULL)
        ((double *) iop->coordinate->p)[3*i+j] = t;
    }
  }

  /* 結合リスト */
  if (iop != NULL)
    MDV_Array_SetSize(iop->bond, 0);
  if (line_n > 0) {
    for (i = 0; i < line_n*3; i++) {
      if ((str = f_token()) == NULL)
        goto term;
      if (iop != NULL)
        MDV_Array_AppendString(iop->bond, str);
    }
  }
  ret = 1;

term:
  if (terminate_blank_line)
    _SReadToken_Term();
  else
    _ReadToken_Term();
  return ret;
}

/* FORTRANバイナリから1stepを読む(返り値は成功の真偽) */
static int ReadData_Binary(FILE *fp, MDV_IO *iop, int xchg) {
  AtomID *atom;
  char header[BINARY_HEADERSIZE];
  double *work;
  MDV_Size i, n;
  int ret = 0;

  /* _ReadToken()の初期化 */
  if (iop != NULL) {
    MDV_String_StrCpy(iop->comment, "");
    _ReadToken_Init(fp, iop->comment);
  } else {
    _ReadToken_Init(fp, NULL);
  }

  /* 原子の対応表とサイズ */
  atom = MDV_VAtomIDP(md_atom);
  n = MDV_VAtomIDGetSize(md_atom);
  if (n <= 0)
    MDV_Fatal("MDV_Read1Step()");
  if (iop != NULL) {
    MDV_Array_SetSize(iop->atom, 0);
    MDV_Array_SetSize(iop->coordinate, n*3);
  }

  /* 粒子の座標 */
  if (fread(&header, BINARY_HEADERSIZE, 1, fp) < 1)
    goto term;
  work = MDV_Work_Alloc(sizeof(double)*3*n);
  if (fread(work, sizeof(double)*3*n, 1, fp) < 1) {
    MDV_Work_Free(work);
    goto term;
  }
  if (iop != NULL) {
    if (xchg)
      xchg_dbl_array(work, 3*n);
    for (i = 0; i < n; i++) {
      ((double *) iop->coordinate->p)[i*3] = work[i];
      ((double *) iop->coordinate->p)[i*3+1] = work[i+n];
      ((double *) iop->coordinate->p)[i*3+2] = work[i+n*2];
    }
  }
  MDV_Work_Free(work);
  if (fread(&header, BINARY_HEADERSIZE, 1, fp) < 1)
    goto term;
  ret = 1;

term:
  _ReadToken_Term();
  return ret;
}

/*
 * 1step分のデータを読む。
 * valはデータへのポインタ。NULLなら結果の記録はしない。
 * lenはステップのバイト数。負の値なら不明。
 * 返り値は成功の真偽。
 */
static int MDV_ReadData(SeekFILE *sfp, void *val, off_t len) {
  MDV_FILE *mfp;
  FILE *fp = NULL;
  MDV_IO *iop = (MDV_IO *) val;
  int ret = 0;

  mfp = (MDV_FILE *) sfp->dp;
  if (mfp->sfp != sfp || (fp = mfp->sfp->fp) == NULL)
    MDV_Fatal("MDV_ReadData()");

  if (mfp->mode == MDV_FILE_TEXT)
    ret = ReadData_Text(fp, iop, len);
  else if (mfp->mode == MDV_FILE_TEXT2)
    ret = ReadData_Text2(fp, iop, len);
  else if (mfp->mode == MDV_FILE_BINARY)
    ret = ReadData_Binary(fp, iop, 0);
  else if (mfp->mode == MDV_FILE_BINARY2)
    ret = ReadData_Binary(fp, iop, 1);
  else
    MDV_Fatal("MDV_ReadData()");

  return ret;
}

/* ファイルからstep番目のステップの内容を読み、coord, linelistを設定する */
#define MAXOPTARGS 5
int _MDV_FileRead(MDV_FILE *mfp, MDV_Coord *coord, MDV_LineList *linelist,
    MDV_Size step) {
  static MDV_IO *iop = NULL; /* 暫定版: 開放しない */
  MDV_3D *coordp;
  MDV_Size n, i;
  int ret;

  if (debug_mode)
    fprintf(stderr, "MDV_FileRead(): step = %ld\n", (long) step);
  if (mfp == NULL)
    MDV_Fatal("_MDV_FileRead()");

  /* 初期化 */
  if (iop == NULL) {
    if ((iop = MDV_IO_Alloc()) == NULL)
      HeapError();
  }

  /* ファイルを読む */
  if ((ret = SeekFileRead(mfp->sfp, iop, step)) != SEEK_READ_SUCCESS)
    MDV_Info("Seek failed.");
    return ret;

  /* 引数 */
  if (iop->argument->n > 0) {
    const char *p = (const char *) iop->argument->p;

    MDV_StatusCopy(mdv_status_tmp, mdv_status_default);
    while (p < (const char *) iop->argument->p + iop->argument->n) {
      char *optargv[MAXOPTARGS];
      const char *str_arg;
      ArgumentID arg;
      int arg_n;

      str_arg = p;
      arg = GetArgumentID(str_arg);
      arg_n = GetArgumentSize(arg, mdv_status_tmp);
      for (i = 0; i < arg_n; i++) {
        p += strlen(p)+1;
        optargv[i] = (char *) p;
      }
      if (!MDV_StatusUpdate(mdv_status_tmp, arg, str_arg, optargv))
        return SEEK_READ_FAILURE;
      p += strlen(p)+1;
    }
    MDV_StatusCopy(mdv_status_default, mdv_status_tmp);
  }

  /* atom[]の設定 */
  if (iop->atom->n > 0) {
    const char *p = (const char *) iop->atom->p;

    for (n = 0; p < (const char *) iop->atom->p + iop->atom->n; n++) {
      AtomID *atom;
      const AtomType *ap;
      StringID si;

      if ((si = SearchStringID(p)) < 0
          || (ap = MDV_Atom_SearchAtom(mdv_atom, si)) == NULL)
        return SEEK_READ_FAILURE;
      MDV_VAtomIDSetSize(md_atom, n+1);
      atom = MDV_VAtomIDP(md_atom);
      atom[n] = ap->id;
      p += strlen(p)+1;
    }
  } else {
    n = MDV_VAtomIDGetSize(md_atom);
  }

  /* coord[]の設定 */
  MDV_CoordSetSize(coord, n);
  coordp = MDV_CoordP(coord);
  for (i = 0; i < n; i++) {
    coordp[i].x = ((double *) iop->coordinate->p)[i*3]
      * mdv_status_default->length_unit;
    coordp[i].y = ((double *) iop->coordinate->p)[i*3+1]
      * mdv_status_default->length_unit;
    coordp[i].z = ((double *) iop->coordinate->p)[i*3+2]
      * mdv_status_default->length_unit;
  }

  /* linelist[]の設定 */
  if (iop->bond->n <= 0) {
    MDV_LineListSetSize(linelist, 0);
  } else {
    MDV_Size line_n;
    const char *p = (const char *) iop->bond->p;

    for (line_n = 0; p < (const char *) iop->bond->p + iop->bond->n;
        line_n++) {
      MDV_LineData *linelistp;
      const BondShapeType *bsp;
      StringID sij;
      char *end;

      if ((sij = SearchBondShapeStringID(p)) < 0
          || (bsp = MDV_Atom_SearchBondShape(mdv_atom, sij)) == NULL)
        return SEEK_READ_FAILURE;
      MDV_LineListSetSize(linelist, line_n+1);
      linelistp = MDV_LineListP(linelist);
      linelistp[line_n].bsi = bsp->id;
      p += strlen(p)+1;
      linelistp[line_n].i = strtol(p, &end, 10)-1;
      if (*end != '\0')
        return SEEK_READ_FAILURE;
      p += strlen(p)+1;
      linelistp[line_n].j = strtol(p, &end, 10)-1;
      if (*end != '\0')
        return SEEK_READ_FAILURE;
      p += strlen(p)+1;
    }
  }

  return SEEK_READ_SUCCESS;
}

/* 最大のステップ数を返す(失敗は負の値) */
MDV_Step MDV_MaxStep(const MDV_FILE *mfp) {
  if (mfp == NULL)
    MDV_Fatal("MDV_MaxStep()");
  return (MDV_Step) SeekMaxStep(mfp->sfp);
}

/* step番目にシークする(返り値は成功の真偽) */
int MDV_SeekTo(MDV_FILE *mfp, MDV_Step step) {
  if (mfp == NULL)
    MDV_Fatal("MDV_MaxStep()");
  return SeekPositionMove(mfp->sfp, step);
}

/* ==== private定義 ======================================================== */

/* ---- エンディアンネス関連 ----------------------------------------------- */

/* big endianかどうかの検査 */
static int IsBigEndian(void) {
  int x = 0;
  int i;

  for (i = 0; i < sizeof(int); i++) {
    x <<= 8;
    x += i % 2;
  }
  return (((char *) (&x))[0] == 0);
}

/* ヘッダからサイズを読む(endian保持) */
static MDV_Size ReadHeader(char *header) {
  MDV_Size n = 0;
  int i;

  if (IsBigEndian()) {
    for (i = 0; i < BINARY_HEADERSIZE; i++) {
      n <<= 8;
      n += (unsigned char) header[i];
    }
  } else {
    for (i = BINARY_HEADERSIZE-1; i >= 0; i--) {
      n <<= 8;
      n += (unsigned char) header[i];
    }
  }

  return n;
}

/* BINARY_HEADERSIZE Byte整数のバイトスワップ */
static MDV_Size xchg_int(MDV_Size x) {
  MDV_Size n = 0;
  int i;

  for (i = 0; i < BINARY_HEADERSIZE; i++) {
    n <<= 8;
    n += (x & 0xff);
    x >>= 8;
  }

  return n;
}

/* double配列のバイトスワップ */
static void xchg_dbl_array(double *x, MDV_Size n) {
  char *s, c;
  MDV_Size i;

  s = (char *) x;
  for (i = 0; i < n; i++) {
    c = s[0]; s[0] = s[7]; s[7] = c;
    c = s[1]; s[1] = s[6]; s[6] = c;
    c = s[2]; s[2] = s[5]; s[5] = c;
    c = s[3]; s[3] = s[4]; s[4] = c;
    s += 8;
  }
}

/* ---- MDV_IO型 ----------------------------------------------------------- */

/* MDV_IO型の確保 */
static void *MDV_IO_Alloc(void) {
  MDV_IO *p;

  if ((p = (MDV_IO *) malloc(sizeof(MDV_IO))) == NULL)
    return NULL;
  p->comment = MDV_String_Alloc();
  p->argument = MDV_Array_Alloc(1);
  p->atom = MDV_Array_Alloc(1);
  p->coordinate = MDV_Array_Alloc(sizeof(double));
  p->bond = MDV_Array_Alloc(1);

  return (void *) p;
}

/* MDV_IO型の開放 */
static void MDV_IO_Free(void *p) {
  if (p == NULL)
    return;
  MDV_String_Free(((MDV_IO *) p)->comment);
  MDV_Array_Free(((MDV_IO *) p)->argument);
  MDV_Array_Free(((MDV_IO *) p)->atom);
  MDV_Array_Free(((MDV_IO *) p)->coordinate);
  MDV_Array_Free(((MDV_IO *) p)->bond);
  free(p);
  return;
}

/* MDV_IO型のコピー */
static void MDV_IO_Copy(void *v1, const void *v2) {
  MDV_IO *p1, *p2;

  p1 = (MDV_IO *) v1;
  p2 = (MDV_IO *) v2;
  if (p1 == NULL || p2 == NULL)
    MDV_Fatal("MDV_IO_Copy()");

  MDV_String_StrCpy(p1->comment, MDV_StringP(p2->comment));
  MDV_Array_SetSize(p1->argument, p2->argument->n);
  memcpy(p1->argument->p, p2->argument->p, p2->argument->n*p2->argument->size);
  MDV_Array_SetSize(p1->atom, p2->atom->n);
  memcpy(p1->atom->p, p2->atom->p, p2->atom->n*p2->atom->size);
  MDV_Array_SetSize(p1->coordinate, p2->coordinate->n);
  memcpy(p1->coordinate->p, p2->coordinate->p,
    p2->coordinate->n*p2->coordinate->size);
  MDV_Array_SetSize(p1->bond, p2->bond->n);
  memcpy(p1->bond->p, p2->bond->p, p2->bond->n*p2->bond->size);
}

/* ---- 可変長文字列 ------------------------------------------------------- */

/*
 * MDV_String型の文字配列は必ず'\0'で終了する。
 * また、返すサイズは'\0'を含まない。
 */

/* MDV_String型の確保 */
static MDV_String *MDV_String_Alloc(void) {
  MDV_String *p;
  
  p = MDV_Array_Alloc(1);
  MDV_Array_SetSize(p, 1);
  ((char *) p->p)[0] = '\0';

  return p;
}

/* 文字列を書き込む */
static void MDV_String_StrCpy(MDV_String *p, const char *str) {
  MDV_Size n;

  n = strlen(str);
  MDV_Array_SetSize(p, n+1);
  Strlcpy((char *) p->p, str, p->n);
}

/* 文字列を追加書き込みする */
static void MDV_String_StrCat(MDV_String *p, const char *str) {
  MDV_Size n, last;

  n = strlen(str);
  last = p->n-1;
  MDV_Array_SetSize(p, last+n+1);
  Strlcpy(((char *) p->p)+last, str, n+1);
}

