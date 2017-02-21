/* "mdv_util.h" �Ƽ�桼�ƥ���ƥ� �إå� */

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

/* ��ĥatexit() */
extern int  AtExit(void (*func)(void));

/* ʸ���� <-> ʸ����ID */
extern StringID SearchStringID(const char *str);
extern StringID Str2StringID(const char *str);
extern const char *StringID2Str(StringID sid);

/* ��ʸ����->RGB�б�ɽ */
extern int Str2Rgb(const char *name, int *pr, int *pg, int *pb);

/* ������ꥢ */
extern void *MDV_Work_Alloc(MDV_Size n);
extern void MDV_Work_Free(void *p);

/* ����Ĺ���� */
typedef struct {
  const char *header_str; /* �إå�ʸ���� */
  void *p;       /* �ǡ����ΰ� */
  MDV_Size n;    /* ���ߤ���������ǿ� */
  MDV_Size size; /* 1���ǤΥХ��ȿ� */
  MDV_Size max;  /* �ǡ����ΰ�ΥХ��ȿ� */
} MDV_Array;

extern MDV_Array *MDV_Array_Alloc(MDV_Size size);
extern void MDV_Array_Free(MDV_Array *p);
extern void MDV_Array_SetSize(MDV_Array *p, MDV_Size n);
extern void _MDV_Array_Resize(MDV_Array *a, char c);
#define MDV_Array_AppendChar(a, c) ((a)->n<(a)->max?\
  ((char *) (a)->p)[(a)->n++]=(c):_MDV_Array_Resize((a), (c)))

/* path��Ϣ */
extern int IsRelativePath(const char *path);
extern const char *Path2Dir(const char *path);
extern const char *Path2File(const char *path);

/* ���顼��λ���� */
extern void MDV_Fatal(const char *str);
extern void HeapError(void);

#endif /* _MDV_UTIL_H */
