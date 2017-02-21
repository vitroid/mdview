/* "mdv_farg.c" 引数ファイルのインクルード機能付き引数処理 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mdv_util.h" /* AtExit() */
#include "mdv_text.h" /* ReadToken() */
#include "mdv_farg.h"

#define FARG_MINSTR   256        /* 可変長文字列の最小サイズ */
#define FARG_MAXFILES 10         /* ファイルの最大ネスト数 */

static const char * const *_farg_argv = NULL; /* 引数へのリファレンス */
static int _farg_i = 0;                       /* 引数の現在の参照位置 */
static int _farg_is_end = 0;                  /* 終了判定用フラグ */
static char *_farg_buf = NULL;                /* 引数格納用可変長配列 */
static size_t _farg_buf_n = 0;                /* 同上のサイズ */
static char *_farg_bak = NULL;                /* 引数のバックアップ */

static FILE *_farg_fp[FARG_MAXFILES];    /* ファイルポインタ格納用 */
static char *_farg_fp_s[FARG_MAXFILES];  /* ファイル名 */
static size_t _farg_fp_l[FARG_MAXFILES]; /* ファイルの処理行数 */
static int _farg_fp_i = -1;              /* 現在のネストレベル */

/* システムの終了 */
static void FARG_Term(void) {
  int i;

  _farg_argv = NULL;
  _farg_i = 0;
  _farg_is_end = 0;
  free(_farg_buf); _farg_buf = NULL;
  free(_farg_bak); _farg_bak = NULL;
  _farg_buf_n = 0;
  for (i = 0; i < FARG_MAXFILES; i++) {
    if (_farg_fp[i] != NULL) {fclose(_farg_fp[i]); _farg_fp[i] = NULL;}
    free(_farg_fp_s[i]); _farg_fp_s[i] = NULL;
    _farg_fp_l[i] = 0;
  }
  _farg_fp_i = -1;
}

/* 初期化 */
void FARG_Init(const char * const argv[]) {
  int i;

  /* システムの初期化 */
  if (_farg_argv == NULL) {
    _farg_buf_n = FARG_MINSTR;
    if ((_farg_buf = (char *) malloc(_farg_buf_n)) == NULL) HeapError();
    _farg_buf[0] = '\0';
    for (i = 0; i < FARG_MAXFILES; i++) {
      _farg_fp[i] = NULL;
      _farg_fp_s[i] = NULL;
      _farg_fp_l[i] = 0;
    }
    AtExit(FARG_Term);
  }

  /* 引数のリファレンスの設定 */
  if (argv == NULL)
    {fprintf(stderr, "FARG_Init(): Illegal argument.\n"); exit(1);}
  _farg_argv = argv;
  _farg_i = 0;
  _farg_is_end = 0;

  /* 引数のバックアップ領域の(再)初期化 */
  free(_farg_bak); _farg_bak = NULL;

  /* 引数ファイルバッファの(再)初期化 */
  for (i = 0; i < FARG_MAXFILES; i++) {
    if (_farg_fp[i] != NULL) {fclose(_farg_fp[i]); _farg_fp[i] = NULL;}
    if (_farg_fp_s[i] != NULL) {free(_farg_fp_s[i]); _farg_fp_s[i] = NULL;}
    _farg_fp_l[i] = 0;
  }
  _farg_fp_i = -1;
}

/* 文字列を可変長配列に保存する */
static void FARG_Save(const char *str) {
  size_t n;

  if (str == NULL) {
    strcpy(_farg_buf, "");
    return;
  }

  if ((n = strlen(str)+1) > _farg_buf_n) {
    /* リサイズ */
    while (n > _farg_buf_n) _farg_buf_n *= 2;
    if ((_farg_buf = (char *) realloc(_farg_buf, _farg_buf_n)) == NULL)
      HeapError();
  }
  strcpy(_farg_buf, str);
}

/* 現在アクセス中のファイル名を得る(リファレンス) */
const char *FARG_CurrentFileName(void) {
  return (_farg_fp_i < 0)? NULL: (const char *) _farg_fp_s[_farg_fp_i];
}

/* 現在アクセス中のファイルの行番号を得る */
long FARG_CurrentLine(void) {
  return (_farg_fp_i < 0)? 0: (long) _farg_fp_l[_farg_fp_i];
}

/* (private版) 引数を1つ読み進める(返値は進んだ先へのリファレンス) */
#define FARG_FILEOPT "-f"
static const char *_FARG_Shift(void) {
  int lines, last;
  const char *str;

  for (;;) {
    /* 引数を1つ読む */
    if (_farg_fp_i >= 0) {
      /* ファイル引数を読む */
      str = ReadToken(_farg_fp[_farg_fp_i], &lines, &last, NULL);
      _farg_fp_l[_farg_fp_i] += lines;
      if (str == NULL) {
        if (last == EOF) {
          /* ファイルを閉じて上の階層へ */
          fclose(_farg_fp[_farg_fp_i]);
          _farg_fp[_farg_fp_i] = NULL;
          free(_farg_fp_s[_farg_fp_i]);
          _farg_fp_s[_farg_fp_i] = NULL;
          _farg_fp_l[_farg_fp_i] = 0;
          _farg_fp_i--;
          continue;
        } else
          goto error;
      }
    } else {
      /* 実引数を読む */
      if (_farg_argv[_farg_i] == NULL) {
        /* 読み尽くした */
        _farg_is_end = 1;
        return NULL;
      }
      str = _farg_argv[_farg_i++];
    }

    /* ファイルのインクルード判定 */
    if (strcmp(str, FARG_FILEOPT) == 0) {
      if (_farg_fp_i >= 0) {
        /* ファイル引数を読む */
        str = ReadToken(_farg_fp[_farg_fp_i], &lines, &last, NULL);
        _farg_fp_l[_farg_fp_i] += lines;
        if (str == NULL)
          goto error;
      } else {
        /* 実引数を読む */
        if (_farg_argv[_farg_i] == NULL)
          {last = EOF; goto error;}
        str = _farg_argv[_farg_i++];
      }
      /* ファイルを開いて下の階層へ */
      if (++_farg_fp_i >= FARG_MAXFILES)
        {fprintf(stderr, "Too many nested include files.\n"); exit(1);}
      if ((_farg_fp[_farg_fp_i] = fopen(str, "rb")) == NULL)
        {fprintf(stderr, "%s: Can't open.(7)\n", str); exit(1);}
      if ((_farg_fp_s[_farg_fp_i] = (char *) malloc(strlen(str)+1)) == NULL)
        HeapError();
      strcpy(_farg_fp_s[_farg_fp_i], str);
      _farg_fp_l[_farg_fp_i] = 0;
      continue;
    }

    break;
  }
  return str;

error:
  /* エラー終了 */
  if (_farg_fp_i >= 0) {
    fprintf(stderr, "%s, line %ld: ",
                      _farg_fp_s[_farg_fp_i], (long) _farg_fp_l[_farg_fp_i]+1);
  }
  switch (last) {
  case '\\': /* バックスラッシュのあといきなりEOFの場合 */
    fprintf(stderr, "Illegal \\.\n");
    break;
  case '\'':
    fprintf(stderr, "Unmatched \'.\n");
    break;
  case EOF: /* FARG_FILEOPTの直後がEOFの場合 */
    fprintf(stderr,"No file name is found.\n");
    break;
  default:
    fprintf(stderr, "Illegal character ");
    if (iscntrl(last))
      fprintf(stderr, "^%c.\n", last+'@');
    else
      fprintf(stderr, "%c.\n", last);
  }
  exit(1);
}

/* 引数のバックアップの初期化(返値は成功の真偽) */
static int FARG_BackupInit(void) {
  const char *str;
  size_t l;

  if ((str = _FARG_Shift()) == NULL) return 0;
  FARG_Save(str);
  l = strlen(_farg_buf)+1;
  if ((_farg_bak = (char *) malloc((l > FARG_MINSTR)? l: FARG_MINSTR)) == NULL)
    HeapError();
  strcpy(_farg_bak, _farg_buf);

  return 1;
}

/* 引数を1つ読む(返値は引数へのリファレンス。失敗はNULL) */
const char *FARG_Read(void) {
  if (_farg_bak == NULL) {
    if (!FARG_BackupInit()) return NULL;
  }
  return (!_farg_is_end)? _farg_buf: NULL;
}

/* 引数を1つ読み進める。(返値は進む前の引数へのリファレンス。失敗はNULL) */
const char *FARG_Shift(void) {
  if (_farg_bak == NULL) {
    if (!FARG_BackupInit()) return NULL;
  }
  if (_farg_is_end) return NULL;

  /* バックアップ */
  if (strlen(_farg_buf)+1 >= FARG_MINSTR) {
    if ((_farg_bak = (char *) realloc(_farg_bak, strlen(_farg_buf)+1)) == NULL)
      HeapError();
  }
  strcpy(_farg_bak, _farg_buf);

  FARG_Save(_FARG_Shift());

  return _farg_bak;
}
