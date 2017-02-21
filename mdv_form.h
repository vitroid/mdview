/* "mdv_form.h" mdv_form.c �Τ���Υإå��ե����� */

#ifndef _MDV_FORM_H_
#define _MDV_FORM_H_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "mdv_type.h"

extern int SetFormat(char *form);

#define MOL_SUCCESS    0
#define MOL_ILL_EXPR  -1 /* ������ʬ�Ҽ� */
#define MOL_ILL_FACT  -2 /* ������ʬ�Ұ��� */
#define MOL_ILL_CFACT -3 /* �����ʷ������� */
#define MOL_TOO_LONG  -4 /* ���Ϥ�Ĺ�᤮�� */
#define MOL_ILL_LEFT  -5 /* �����ʺ��ͥ��� */
#define MOL_ILL_LEFT1 -6 /* �����ʥͥ���'(' */
#define MOL_ILL_LEFT2 -7 /* �����ʥͥ���'{' */
#define MOL_ILL_LEFT3 -8 /* �����ʥͥ���'[' */
#define MOL_ILL_RIGHT -9 /* �����ʱ��ͥ��� */
extern int ReadMolExpression(const char *src, char *dest, int max);

/* �ѿ��ꥹ�� */
#define MAXPARAMETER 10
typedef struct {
  int n;                /* �ѿ��ο� */
  char c[MAXPARAMETER]; /* �б�����ʸ�� */
  int v[MAXPARAMETER];  /* �б������� */
} ParameterList;

extern int SetParameterValue(int nvalue, int *value, ParameterList *pparam); 
extern int GetParameters(const char *mol, ParameterList *pparam);
extern int Substitute(const char *src, char *dst, int max,
                                                  const ParameterList *pparam);
extern MDV_Size TotalParticles(const char *mol, AtomID (*pf)(const char *));
extern MDV_Size SetAtomTable(const char *mol, AtomID *atom,
                                                   AtomID (*pf)(const char *));

#endif /* _MDV_FORM_H */
