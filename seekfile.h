/* "seekfile.h" seekfile.c �Τ���Υإå��ե����� */

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

/* SeekFileData��(���Ф�����ʤ����ᡢ�ºݤˤ�¸�ߤ��ʤ�) */
typedef struct {
  void *(*Alloc)(void);
  void (*Free)(void *);
  void (*Copy)(void *, const void *);
} SeekFileDataFunc;

/* ���ѥ���å��� */
typedef struct {
  SeekSize current;           /* ���߰��� */
  void *p;                    /* �ǡ����ؤΥݥ��� */
  const SeekFileDataFunc *vf; /* �ǡ����Υ��дؿ� */
} SeekCacheType;

/* ���������ֵ��������� */
typedef struct {
  off_t **buf;      /* ���������������� */
  SeekSize buf_max; /* ����������������Υ����� */
  SeekSize i,       /* ���ߤ�step�ֹ� */
           n,       /* ��Ͽstep�� */
           max;     /* ����step�� */
} SeekPosType;

/* ���������ֵ������ե����� */
typedef struct _SeekFILE {
  const char *path; /* ̤���Ѥʤ�NULL������ */
  off_t size;
  time_t time;
  FILE *fp;
  SeekCacheType *cache;
  SeekPosType pos;
  const SeekFileDataFunc *vf;
  int (*vf_Read)(struct _SeekFILE *, void *, off_t); /* �ǡ����ɤ߼���Ѵؿ� */
  void *dp; /* �������ؤΥݥ��� */
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
