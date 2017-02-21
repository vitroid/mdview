/* "mdv_util.h" 各種ユーティリティ ヘッダ */

#ifndef _MDV_UTIL_H
#define _MDV_UTIL_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* size_t */
#if STDC_HEADERS
# include <sys/types.h>
# include <stdlib.h>
#else
# error the ANSI headers are required.
#endif

#include "mdv_type.h"

/* 拡張atexit() */
extern int  AtExit(void (*func)(void));

/* 文字列 <-> 文字列ID */
extern StringID SearchStringID(const char *str);
extern StringID Str2StringID(const char *str);
extern const char *StringID2Str(StringID sid);

/* 色文字列->RGB対応表 */
extern int Str2Rgb(const char *name, int *pr, int *pg, int *pb);

/* ワークエリア */
extern void *MDV_Work_Alloc(MDV_Size n);
extern void MDV_Work_Free(void *p);

/* 可変長配列 */
typedef struct {
  const char *header_str; /* ヘッダ文字列 */
  void *p;       /* データ領域 */
  MDV_Size n;    /* 現在の配列の要素数 */
  MDV_Size size; /* 1要素のバイト数 */
  MDV_Size max;  /* データ領域のバイト数 */
} MDV_Array;

extern MDV_Array *MDV_Array_Alloc(MDV_Size size);
extern void MDV_Array_Free(MDV_Array *p);
extern void MDV_Array_SetSize(MDV_Array *p, MDV_Size n);
extern void _MDV_Array_Resize(MDV_Array *a, char c);
#define MDV_Array_AppendChar(a, c) ((a)->n<(a)->max?\
  ((char *) (a)->p)[(a)->n++]=(c):_MDV_Array_Resize((a), (c)))

/* path関連 */
extern int IsRelativePath(const char *path);
extern const char *Path2Dir(const char *path);
extern const char *Path2File(const char *path);

/* エラー終了処理 */
extern void MDV_Fatal(const char *str);
extern void MDV_Info(const char *str);
extern void HeapError(void);

#endif /* _MDV_UTIL_H */
