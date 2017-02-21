/* "mdv_form.h" mdv_form.c のためのヘッダファイル */

#ifndef _MDV_FORM_H_
#define _MDV_FORM_H_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "mdv_type.h"

extern int SetFormat(char *form);

#define MOL_SUCCESS    0
#define MOL_ILL_EXPR  -1 /* 不正な分子式 */
#define MOL_ILL_FACT  -2 /* 不正な分子因子 */
#define MOL_ILL_CFACT -3 /* 不正な係数因子 */
#define MOL_TOO_LONG  -4 /* 出力が長過ぎる */
#define MOL_ILL_LEFT  -5 /* 不正な左ネスト */
#define MOL_ILL_LEFT1 -6 /* 不正なネスト'(' */
#define MOL_ILL_LEFT2 -7 /* 不正なネスト'{' */
#define MOL_ILL_LEFT3 -8 /* 不正なネスト'[' */
#define MOL_ILL_RIGHT -9 /* 不正な右ネスト */
extern int ReadMolExpression(const char *src, char *dest, int max);

/* 変数リスト */
#define MAXPARAMETER 10
typedef struct {
  int n;                /* 変数の数 */
  char c[MAXPARAMETER]; /* 対応する文字 */
  int v[MAXPARAMETER];  /* 対応する値 */
} ParameterList;

extern int SetParameterValue(int nvalue, int *value, ParameterList *pparam); 
extern int GetParameters(const char *mol, ParameterList *pparam);
extern int Substitute(const char *src, char *dst, int max,
                                                  const ParameterList *pparam);
extern MDV_Size TotalParticles(const char *mol, AtomID (*pf)(const char *));
extern MDV_Size SetAtomTable(const char *mol, AtomID *atom,
                                                   AtomID (*pf)(const char *));

#endif /* _MDV_FORM_H */
