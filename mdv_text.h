/* "mdv_text.h" mdv_file.*�Τ���Υƥ����Ƚ��� �إå� */

#ifndef _MDV_TEXT_H
#define _MDV_TEXT_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* �ե����뤫��ιԤ��ɤ߹��� */
extern int ReadLine(FILE *fp, const char **pstr);

/* �ե����뤫��Υȡ�������ɤ߹��� */
extern const char *ReadToken(FILE *fp, int *plines, int *plast,
  const char **pcomment);

/* ʸ���󤫤�Υȡ�������ɤ߹��� */
extern const char *SReadToken(const char *str, const char **endp,
  int *plines, int *plast, const char **pcomment);

#endif /* _MDV_TEXT_H */
