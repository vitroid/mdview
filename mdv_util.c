/* "mdv_util.c" 各種ユーティリティ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> /* isalpha() */
#include <errno.h>
#include "mdview.h"
#include "mdv_util.h"
#include "hash.h"
#include "chunk.h"
#include "stack.h"
#include "str2int.h"

/* ---- 拡張atexit()関連 --------------------------------------------------- */

#define MAXATEXIT 256 /* 登録関数の数の上限 */
static int atexit_n = 0;
static void (*atexit_buf[MAXATEXIT])(void);
static int atexit_active = 0; /* 終了処理実行中なら真 */

/* 実際にatexit()で呼ばれる関数 */
static void TermAtExit(void) {
  atexit_active = 1;
  while (--atexit_n >= 0)
    (atexit_buf[atexit_n])();
}

/* atexit()の代用をする関数(成功が0で、失敗は非0を返す。) */
int AtExit(void (*func)()) {
  if (atexit_active) {
    /* 設定した関数内でAtExit()をしたときの対策 */
    fprintf(stderr,"AtExit(): Illegal function call in TermAtExit().\n");
    return -1;
  }

  if (atexit_n <= 0) {
    /* AtExit()の初期化 */
    atexit(TermAtExit);
  }
  if (atexit_n >= MAXATEXIT) return -1;
  /* 登録 */
  atexit_buf[atexit_n++] = func;

  return 0;
}

/* ---- 文字列 <-> 文字列ID ------------------------------------------------ */

static Str2Int_Type *sip = NULL;

/* 文字列strに対応するStringIDを返す。未知の文字列は失敗。(失敗は負の値) */
StringID SearchStringID(const char *str) {
  if (sip == NULL)
    sip = Str2Int_Alloc();
  return (StringID) Str2Int_SearchID(str, sip);
}

/* 文字列strに対応するStringIDを返す。未知の文字列は登録。(失敗は負の値) */
StringID Str2StringID(const char *str) {
  if (sip == NULL)
    sip = Str2Int_Alloc();
  return (StringID) Str2Int_Str2ID(str, sip);
}

/* StringIDに対応する文字列strを返す(失敗はNULL) */
const char *StringID2Str(StringID sid) {
  if (sip == NULL)
    sip = Str2Int_Alloc();
  return Str2Int_ID2Str((Str2Int_Size) sid, sip);
}

/* ---- Str2Rgb()関連 ------------------------------------------------------ */

#ifndef RGB_FILEPATH
#define RGB_FILEPATH "/usr/X11R6/lib/X11/rgb.txt"
#define RGB_FILEPATH "/opt/X11/share/X11/rgb.txt"
#endif

/* RGB_Type型 */
typedef struct {
  StringID si;
  int r, g, b;
} RGB_Type;

typedef Hash_Type RGB_Table;
RGB_Table *RGB_Table_Alloc(void);
void RGB_Table_Free(RGB_Table *rtp);
RGB_Type *RGB_Table_Search(const RGB_Table *rtp, StringID si);
RGB_Type *RGB_Table_Insert(RGB_Table *rtp, StringID si);

static RGB_Table *rgb_table = NULL; /* 暫定版: 解放しない */

/* rgb.txtを読んで、文字列-RGB値の対応表を準備する */
static int _init_rgbtable = 0;
void RGBTableInit(void) {
  FILE *fp;

  /* 初期化 */
  if ((rgb_table = RGB_Table_Alloc()) == NULL)
    HeapError();

  /* rgb.txtのオープン */
  if ((RGB_FILEPATH)[0] != '/') {
    char *path;
    int npath;

    npath = strlen(exec_dir)+strlen(RGB_FILEPATH)+1;
    if ((path = (char *) malloc(npath)) == NULL)
      HeapError();
    Strlcpy(path, exec_dir, npath);
    Strlcat(path, RGB_FILEPATH, npath);
    if ((fp = fopen(path, "r")) == NULL)
      fprintf(stderr, "%s: Can't open.(5)\n", path);
    free(path);
  } else {
    if ((fp = fopen(RGB_FILEPATH, "r")) == NULL)
      fprintf(stderr, "%s: Can't open.(6)\n", RGB_FILEPATH);
  }

  /* 読み込み */
  if (fp != NULL) {
    MDV_Array *line;
    int c;

    line = MDV_Array_Alloc(1);
    while ((c = getc(fp)) != EOF) {
      char *p;
      RGB_Type rgb, *rp;
      int i;

      /* 一行の読み込み */
      MDV_Array_SetSize(line, 0);
      do {
        MDV_Array_AppendChar(line, c);
      } while ((c = getc(fp)) != EOF && c != '\n');
      if (c == '\n')
        MDV_Array_AppendChar(line, c);
      MDV_Array_AppendChar(line, '\0');
      p = (char *) line->p;

      /* コメント行のスキップ */
      if (p[0] == '!')
        continue;

      /* 終端の空白と改行文字を削除 */
      for (i = line->n-1; i >= 0 && (p[i] == ' ' || p[i] == '\t'
          || p[i] == '\n' || p[i] == '\0'); i--) {
        p[i] = '\0';
      }

      /* RGB値の読み込み */
      rgb.r = (int) strtol(p, &p, 10);
      if (errno == ERANGE)
        {fprintf(stderr,"Read error.(4)\n"); exit(1);}
      rgb.g = (int) strtol(p, &p, 10);
      if (errno == ERANGE)
        {fprintf(stderr,"Read error.(5)\n"); exit(1);}
      rgb.b = (int) strtol(p, &p, 10);
      if (errno == ERANGE)
        {fprintf(stderr,"Read error.(6)\n"); exit(1);}

      /* 空白文字の除去 */
      while (*p == ' ' || *p == '\t')
        p++;

      /* 色文字列の読み込み */
      if (p[0] == '\0')
        {fprintf(stderr,"Read error.(7)\n"); exit(1);}
      rgb.si = Str2StringID(p);

      /* 登録 */
      if (!RGB_Table_Search(rgb_table, rgb.si)) {
        if ((rp = RGB_Table_Insert(rgb_table, rgb.si)) == NULL)
          MDV_Fatal("RGBTableInit()");
        *rp = rgb;
      } else {
        fprintf(stderr, "%s: Already exists.\n", p);
      }
    }
    MDV_Array_Free(line);
    fclose(fp);
  }
}

static int _hexnum(int c) {
  if (c >= '0' && c <= '9')
    c = c - '0';
  else if (c >= 'A' && c <= 'F')
    c = c - 'A' + 10;
  else if (c >= 'a' && c <= 'f')
    c = c - 'a' + 10;
  else
    c = -1;

  return c;
}

/* 色文字列を読んで、対応するRGB値を求める。(返値は成功の真偽) */
int Str2Rgb(const char *name, int *pr, int *pg, int *pb) {
  RGB_Type *ret;
  StringID si;
  int h,l;

  if (!_init_rgbtable) {
    RGBTableInit();
    _init_rgbtable = 1;
  }

  if (name[0] == '#') {
    /* "#RRGGBB"形式 */
    if ((h = _hexnum(name[1])) < 0) return 0;
    if ((l = _hexnum(name[2])) < 0) return 0;
    *pr = h*16+l;
    if ((h = _hexnum(name[3])) < 0) return 0;
    if ((l = _hexnum(name[4])) < 0) return 0;
    *pg = h*16+l;
    if ((h = _hexnum(name[5])) < 0) return 0;
    if ((l = _hexnum(name[6])) < 0) return 0;
    *pb = h*16+l;
  } else {
    /* 色の名前 */
    if ((si = SearchStringID(name)) < 0
        || (ret = RGB_Table_Search(rgb_table, si)) == NULL)
      return 0;
    *pr = ret->r;
    *pg = ret->g;
    *pb = ret->b;
  }

  return 1;
}

/* ---- RGB_Table型 -------------------------------------------------------- */

/* RGB_Type型の初期化 */
void RGB_Type_Init(RGB_Type *rp, StringID si) {
  rp->si = si;
  rp->r = 0;
  rp->g = 0;
  rp->b = 0;
}

/* RGB_Type用アロケータ */
#define RGB_CHUNK_SIZE (256*sizeof(RGB_Type))
static Chunk_Type *rgb_chunk = NULL; /* 暫定版: 終了処理は省略 */

/* 比較関数(StringIDが符号付き整数で、かつ0以上の値であることを前提とする) */
static int RGB_Table_NodeCompare(const void *va, const void *vb) {
  return (int) (((const RGB_Type *) va)->si - ((const RGB_Type *) vb)->si);
}

/* ハッシュ関数 */
static Hash_Size RGB_Table_NodeHash(const void *vp) {
  return (Hash_Size) ((const RGB_Type *) vp)->si;
}

/* コピーコンストラクタ */
static void *RGB_Table_NodeCopy(const void *vp) {
  const RGB_Type *rp = (const RGB_Type *) vp;
  RGB_Type *ret;

  if (vp == NULL || rgb_chunk == NULL)
    return NULL;
  if ((ret = (RGB_Type *) Chunk_NodeAlloc(rgb_chunk)) == NULL)
    return NULL;
  *ret = *rp;

  return ret;
}

/* デストラクタ */
static void RGB_Table_NodeFree(void *vp) {
  if (vp == NULL || rgb_chunk == NULL)
    return;
  Chunk_NodeFree(rgb_chunk, vp);
}

/* インスタンスの作成 */
RGB_Table *RGB_Table_Alloc(void) {
  if (rgb_chunk == NULL) {
    if ((rgb_chunk = Chunk_TypeAlloc(sizeof(RGB_Type),
        RGB_CHUNK_SIZE, 0)) == NULL)
      return NULL;
  }
  return Hash_Alloc(0, 0);
}

/* インスタンスの消去 */
void RGB_Table_Free(RGB_Table *rtp) {
  Hash_Free(rtp, RGB_Table_NodeFree);
}

/* RGB情報の探索 */
RGB_Type *RGB_Table_Search(const RGB_Table *rtp, StringID si) {
  RGB_Type key;

  if (si < 0)
    return NULL;
  RGB_Type_Init(&key, si);
  return (RGB_Type *) Hash_Search(rtp, &key, RGB_Table_NodeCompare,
    RGB_Table_NodeHash);
}

/* RGB情報の登録 */
RGB_Type *RGB_Table_Insert(RGB_Table *rtp, StringID si) {
  RGB_Type rgb;

  if (si < 0)
    return NULL;
  RGB_Type_Init(&rgb, si);
  return (RGB_Type *) Hash_Insert(rtp, &rgb, RGB_Table_NodeCompare,
    RGB_Table_NodeHash, RGB_Table_NodeCopy);
}

/* ---- ワークエリア ------------------------------------------------------- */

static Stack_Type *mdv_work = NULL;

static void MDV_WorkTerm(void) {
  Stack_TypeFree(mdv_work);
  mdv_work = NULL;
}

/* ワークエリアの確保 */
void *MDV_Work_Alloc(MDV_Size n) {
  void *p;

  if (mdv_work == NULL) {
    if ((mdv_work = Stack_TypeAlloc()) == NULL)
      HeapError();
    AtExit(MDV_WorkTerm);
  }
  if (n < 0)
    MDV_Fatal("MDV_Work_Alloc(): Illegal argument.");
  if ((p = Stack_NodeAlloc(mdv_work, n)) == NULL)
    HeapError();

  return p;
}

/* ワークエリアの解放 */
void MDV_Work_Free(void *p) {
  if (mdv_work == NULL)
    MDV_Fatal("MDV_Work_Free()");
  Stack_NodeFree(mdv_work, p);
}

/* ---- Path2Dir(),Path2File() --------------------------------------------- */

#ifndef DIR_SEPARATOR
#define DIR_SEPARATOR '/'
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
static char path_buf[MAXPATHLEN];

/* 相対pathか? */
int IsRelativePath(const char *path) {
  if (path == NULL)
    {fprintf(stderr, "IsRelativePath(): system error.\n"); exit(1);}

  if (DIR_SEPARATOR == '\\') {
    /* MS-DOSのドライブ指定に対応する */
    if (isalpha(path[0]) && path[1] == ':')
      return 0;
  }
  return (path[0] != DIR_SEPARATOR);
}

/* pathからのディレクトリ名の切り出し(返り値はstatic char配列) */
const char *Path2Dir(const char *path) {
  char *p;

  if (Strlcpy(path_buf, path, MAXPATHLEN) >= MAXPATHLEN)
    {fprintf(stderr, "IsRelativePath(): system error.\n"); exit(1);}
  p = strrchr(path_buf, DIR_SEPARATOR);
  if (p == NULL)
    p = path_buf;
  else
    p++;
  *p = '\0';
  return path_buf;
}

/* pathからのファイル名の切り出し(返り値はstatic char配列) */
const char *Path2File(const char *path) {
  char *p;

  if (Strlcpy(path_buf, path, MAXPATHLEN) >= MAXPATHLEN)
    fprintf(stderr, "Path2File(): system error.\n");
  p = strrchr(path_buf, DIR_SEPARATOR);
  if (p == NULL)
    p = path_buf;
  else
    p++;
  return p;
}

/* ---- MDV_Fatal(), HeapError() ------------------------------------------- */

/* エラー終了 */
void MDV_Fatal(const char *str) {
  fprintf(stderr,"%s: system error.\n", str);
  exit(1);
}

/* ヒープ不足によるエラー終了 */
void HeapError(void) {
  fprintf(stderr,"Allocation failed.\n");
  exit(1);
}

/* ---- 可変長配列 --------------------------------------------------------- */

/* 識別用文字列 */
static const char *mdv_array_str = "MDV_Array";

#define MDV_ALLOC_MIN_SIZE 1024

/* 可変長配列の確保(失敗は異常終了。sizeは要素の大きさ) */
MDV_Array *MDV_Array_Alloc(MDV_Size size) {
  MDV_Array *p;

  if (size <= 0)
    MDV_Fatal("MDV_Array_Alloc()");
  if ((p = (MDV_Array *) malloc(sizeof(MDV_Array))) == NULL)
    HeapError();
  p->n = 0;
  p->size = size;
  p->max = MDV_ALLOC_MIN_SIZE;
  while (p->max < size) {
    p->max <<= 1;
    if (p->max <= 0)
      MDV_Fatal("MDV_Array_Alloc()");
  }
  if ((p->p = malloc(p->max)) == NULL)
    HeapError();
  p->header_str = mdv_array_str;

  return p;
}

/* 可変長配列の解放 */
void MDV_Array_Free(MDV_Array *p) {
  if (p == NULL)
    return;
  if (p->header_str != mdv_array_str)
    MDV_Fatal("MDV_Array_Free()");
  free(p->p);
  p->header_str = NULL;
  free(p);

  return;
}

/* 可変長配列のサイズの変更 */
void MDV_Array_SetSize(MDV_Array *p, MDV_Size n) {
  if (p == NULL || p->header_str != mdv_array_str || n < 0)
    MDV_Fatal("MDV_Array_SetSize()");
  if (n > 0 && p->size > p->max/n) {
    void *tmp;

    do {
      p->max <<= 1;
      if ((size_t) p->max <= 0)
        MDV_Fatal("MDV_Array_SetSize()");
    } while (p->size > p->max/n);
    if ((tmp = realloc(p->p, p->max)) == NULL)
      HeapError();
    p->p = tmp;
  }
  p->n = n;
}

/* 文字配列とみなしてリサイズする */
void _MDV_Array_Resize(MDV_Array *a, char c) {
  MDV_Size n;

  if (a == NULL || a->size != 1)
    MDV_Fatal("_MDV_Array_Resize");
  n = a->n;
  MDV_Array_SetSize(a, n+1);
  ((char *) a->p)[n] = c;
}

