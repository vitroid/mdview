/* "seekfile.h" seekfile.c のためのヘッダファイル */

#ifndef _SEEKFILE_H_
#define _SEEKFILE_H_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* FILE */
#if STDC_HEADERS
# include <stdio.h>
#else
# error the ANSI headers are required.
#endif

/* time_t */
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

typedef long SeekSize;

/* SeekFileData型(メンバを持たないため、実際には存在しない) */
typedef struct {
  void *(*Alloc)(void);
  void (*Free)(void *);
  void (*Copy)(void *, const void *);
} SeekFileDataFunc;

/* 汎用キャッシュ */
typedef struct {
  SeekSize current;           /* 現在位置 */
  void *p;                    /* データへのポインタ */
  const SeekFileDataFunc *vf; /* データのメンバ関数 */
} SeekCacheType;

/* シーク位置記憶用配列 */
typedef struct {
  off_t **buf;      /* シーク位置用配列 */
  SeekSize buf_max; /* シーク位置用配列のサイズ */
  SeekSize i,       /* 現在のstep番号 */
           n,       /* 登録step数 */
           max;     /* 最大step数 */
} SeekPosType;

/* シーク位置記憶型ファイル */
typedef struct _SeekFILE {
  const char *path; /* 未使用ならNULLが入る */
  off_t size;
  time_t time;
  FILE *fp;
  SeekCacheType *cache;
  SeekPosType pos;
  const SeekFileDataFunc *vf;
  int (*vf_Read)(struct _SeekFILE *, void *, off_t); /* データ読み取り用関数 */
  void *dp; /* 派生型へのポインタ */
} SeekFILE;

#define SEEK_READ_SUCCESS   0
#define SEEK_READ_UNDERFLOW 1
#define SEEK_READ_OVERFLOW  2
#define SEEK_READ_FAILURE   3

extern SeekFILE *SeekFileOpen(const char *path, const SeekFileDataFunc *vf,
  int (*vf_Read)(struct _SeekFILE *, void *, off_t), void *dp);
extern int SeekFileReload(SeekFILE *sfp);
extern void SeekFileClose(SeekFILE *sfp);
extern int SeekFileRead(SeekFILE *sfp, void *val, SeekSize step);
extern int SeekPositionAdd(SeekFILE *sfp);
extern int SeekPositionMove(SeekFILE *sfp, SeekSize step);
extern SeekSize SeekMaxStep(const SeekFILE *sfp);

#endif /* _SEEKFILE_H_ */
