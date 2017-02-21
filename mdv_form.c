/* "mdv_form.c" �ǡ����ե�����Υե����ޥå�����ط� */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>  /* isupper(), islower(), isdigit(), isalnum() */
#include <string.h> /* memcpy() */
#include "mdv_form.h"

int ReadMolExpression(const char *src, char *dest, int max);
int SetParameterValue(int nvalue, int *value, ParameterList *pparam);
int GetParameters(const char *mol, ParameterList *pparam);
int Substitute(const char *src, char *dst, int max,
                                                  const ParameterList *pparam);
MDV_Size TotalParticles(const char *mol, AtomID (*pf)(const char *));
MDV_Size SetAtomTable(const char *mol, AtomID *atom,
                                                   AtomID (*pf)(const char *));

/*----------------------------�ѥ�᡼�������Ϣ-----------------------------*/

/* �ꥹ�Ȥν���� */
static void InitParameters(ParameterList *pparam) {
  int i;

  pparam->n = 0;
  for (i = 0; i < MAXPARAMETER; i++) {
    pparam->c[i] = '\0';
    pparam->v[i] = 0;
  }
}

/* �ѿ��μ���(���Ԥ�����֤�) */
int  GetParameters(const char *mol, ParameterList *pparam) {
  int i;

  /* ����� */
  InitParameters(pparam);

  while (*mol != '\0') {
    /* �ѿ�̾��õ�� */
    if (islower(*mol)) {
      /* �ѿ�ȯ�� */
      for (i = 0; i < pparam->n && *mol != pparam->c[i]; i++)
        ;
      if (i >= pparam->n) {
        /* �������ѿ�����Ͽ */
        if (pparam->n >= MAXPARAMETER)
          return -1;
        pparam->c[i] = *mol;
        pparam->n++;
      }
    }
    /* ���Υȡ�����ޤǥ����å� */
    while (isalnum(*mol))
      mol++;
    while (*mol != '\0' && !isalnum(*mol))
      mol++;
  }

  return pparam->n;
}

/* �ѿ����ͤ�����(�����Ͽ������Ԥϵ����֤�) */
int SetParameterValue(int nvalue, int *value, ParameterList *pparam) {
  int i;

  for (i = 0; i < nvalue; i++) {
    /* �Ȥꤢ�������л���Τ߲�ǽ */
    if (i >= pparam->n)
      return 0;
    pparam->v[i] = value[i];
  }

  return 1;
}

/* ����val��maxʸ���ʲ���ʸ������Ѵ��������Ͻ��ϥХ��ȿ������Ԥ�����֤�) */
static int _i2str(int val, char *str, int max) {
  char c;
  int i, n;

  if (val <= 0) return -1;
  /* �ս�˵��� */
  for (i = 0; i < max-1 && val > 0; i++) {
    str[i] = val%10 + '0';
    val /= 10;
  }
  if (i >= max-1) return -1;
  n = i;
  /* ȿž */
  for (i = 0; i < n/2; i++) {
    c = str[i];
    str[i] = str[n-1-i];
    str[n-1-i] = c;
  }
  str[n] = '\0';

  return n;
}

#define _MAXSTACKLEVEL 20
/* �ѿ������������δ�ά��(�����Ͻ��ϥХ��ȿ������Ԥ�����֤�) */
int Substitute(const char *src, char *dst, int max,
                                                 const ParameterList *pparam) {
  int stack[_MAXSTACKLEVEL];
  int stack_n = 0;
  int size[_MAXSTACKLEVEL];
  int n; /* ���Ϥ����Х��ȿ� */
  int i;

  n = 0;
  while (*src != '\0') {
    if (*src == ' ') {
      /* ���ڡ��� */
      if (stack_n <= 0) return -1;
      if (size[stack_n-1] > 0) {
        /* ʬ�Ҽ�(������)�˸¤ꥹ�ڡ�����ä��� */
        if (n >= max-1) return -1;
        dst[n++] = *src;
        size[stack_n-1]++;
      }
      src++;
    } else if (isupper(*src)) {
      /* ���� */
      if (stack_n >= _MAXSTACKLEVEL) return -1;
      for (i = 0; isalnum(*src); i++) {
        if (n+i >= max-1) return -1;
        dst[n+i] = *src++;
      }
      stack[stack_n] = 1;
      size[stack_n] = i;
      stack_n++;
      n += i;
    } else if (islower(*src)) {
      /* �ѿ� */
      if (stack_n >= _MAXSTACKLEVEL) return -1;
      for (i = 0; i < pparam->n && *src != pparam->c[i]; i++)
        ;
      if (i >= pparam->n) return -1;
      stack[stack_n] = pparam->v[i];
      size[stack_n] = 0;
      stack_n++;
      src++;
    } else if (isdigit(*src)) {
      /* ���� */
      if (stack_n >= _MAXSTACKLEVEL) return -1;
      stack[stack_n] = atoi(src);
      size[stack_n] = 0;
      stack_n++;
      while (isalnum(*src))
        src++;
    } else if (*src == '&') {
      /* Ϣ�� */
      if (stack_n < 2) return -1;
      if (stack[stack_n-1] <= 0) {
        /* Ϣ���褬������ */
        ;
      } else if (stack[stack_n-2] <= 0) {
        /* Ϣ�븵�������� */
        if (size[stack_n-2] > 0) return -1;
        size[stack_n-2] = size[stack_n-1];
      } else {
        /* �̾��Ϣ�� */
        if (n >= max-1) return -1;
        dst[n++] = *src;
        size[stack_n-2] += size[stack_n-1] + 1;
      }
      src++;
      stack[stack_n-2] += stack[stack_n-1];
      stack_n--;
    } else if (*src == '^') {
      /* �����֤� */
      if (stack_n < 2 || stack[stack_n-1] < 0) return -1;
      if (stack[stack_n-2] <= 0 || stack[stack_n-1] <= 0) {
        /* ���󤯤��֤� */
        n -= size[stack_n-2]+size[stack_n-1];
        stack[stack_n-2] = 0;
        size[stack_n-2] = 0;
      } else if (stack[stack_n-1] == 1) {
        /* ���󤯤��֤� */
        ;
      } else {
        /* �̾�Τ����֤� */
        if ((i = _i2str(stack[stack_n-1], dst+n, max-n)) < 0) return -1;
        n += i;
        if (n >= max-1) return -1;
        dst[n++] = *src;
        stack[stack_n-2] *= stack[stack_n-1];
        size[stack_n-2] += i + 1;
      }
      stack_n--;
      src++;
    } else if (*src == '+') {
      /* �� */
      if (stack_n < 2) return -1;
      stack[stack_n-2] += stack[stack_n-1];
      n -= size[stack_n-1];
      stack_n--;
      src++;
    } else if (*src == '-') {
      /* �� */
      if (stack_n < 2) return -1;
      stack[stack_n-2] -= stack[stack_n-1];
      n -= size[stack_n-1];
      stack_n--;
      src++;
    } else if (*src == '*') {
      /* �� */
      if (stack_n < 2) return -1;
      stack[stack_n-2] *= stack[stack_n-1];
      n -= size[stack_n-1];
      stack_n--;
      src++;
    } else if (*src == '/') {
      /* �� */
      if (stack_n < 2 || stack[stack_n-2] == 0) return -1;
      stack[stack_n-2] /= stack[stack_n-1];
      n -= size[stack_n-1];
      stack_n--;
      src++;
    } else {
      /* �۾���� */
      return -1;
    }
  }
  dst[n] = '\0';

  return (stack_n == 1)? n: -1;
}

/* ��γ�ҿ��μ���(���Ԥ�����֤�) */
MDV_Size TotalParticles(const char *mol, AtomID (*pf)(const char *)) {
  return SetAtomTable(mol, NULL, pf);
}

/* γ�Ң������б�ɽ������(��������γ�ҿ������Ԥ�����֤�) */
MDV_Size SetAtomTable(const char *mol, AtomID *atom,
                                                  AtomID (*pf)(const char *)) {
  MDV_Size stack[_MAXSTACKLEVEL];
  int stack_n = 0;
  MDV_Size n; /* ���Ϥ���γ�ҿ� */
  MDV_Size i, j;

  n = 0;
  while (*mol != '\0') {
    if (*mol == ' ') {
      /* ���ڡ��� */
      mol++;
    } else if (isupper(*mol)) {
      /* ���� */
      if (stack_n >= _MAXSTACKLEVEL)
        return -1;
      if (atom != NULL) {
        if ((atom[n++] = pf(mol)) < 0)
          return -1;
      }
      stack[stack_n++] = 1;
      while (isalnum(*mol))
        mol++;
    } else if (isdigit(*mol)) {
      /* ���� */
      if (stack_n >= _MAXSTACKLEVEL)
        return -1;
      stack[stack_n++] = atoi(mol);
      while (isalnum(*mol))
        mol++;
    } else if (*mol == '&') {
      /* Ϣ�� */
      if (stack_n < 2)
        return -1;
      stack[stack_n-2] += stack[stack_n-1];
      stack_n--;
      mol++;
    } else if (*mol == '^') {
      /* �����֤� */
      if (stack_n < 2 || stack[stack_n-1] <= 0)
        return -1;
      if (atom != NULL) {
        for (i = 1; i < stack[stack_n-1]; i++) {
          for (j = 0; j < stack[stack_n-2]; j++)
            atom[n+j] = atom[n+j - stack[stack_n-2]];
          n += stack[stack_n-2];
        }
      }
      stack[stack_n-2] *= stack[stack_n-1];
      stack_n--;
      mol++;
    } else {
      /* �۾���� */
      return -1;
    }
  }

  return (stack_n == 1)? stack[0]: -1;
}

/*-----------------------������ˡ�����ֵ�ˡ�ؤ��Ѵ��ط�----------------------*/
/*                                                                           */
/*---------------------------------������ˡ----------------------------------*/
/* (������)                                                                  */
/*                                                                           */
/* �ѿ��Ͼ�ʸ����ʸ����ɽ����                                                */
/* �����0-9��������Ϣ³��ɽ�����롣��ο��ϰ۾�Ȥߤʤ���                   */
/* ��§�黻�Τߥ��ݡ��Ȥ��롣                                                */
/* ���å���[]{}()�Σ�������Ȥ߹�碌�ƻ��ѤǤ��롣                          */
/* �ȡ�����δ֤˥��ڡ���������뤳�Ȥ��Ǥ��롣                              */
/* ������ѿ����¤����礫���֤˥��ڡ����Τ�¸�ߤ����硢�軻����*����ά   */
/* ����Ƥ���Ȥߤʤ���                                                      */
/*---------------------------------------------------------------------------*/
/* (ʬ�Ҽ�)                                                                  */
/*                                                                           */
/* ���Ҥ���ʸ���ǻϤ�ơ���ʸ��(��)��Ǥ�դο��ղä�����Τ�ɽ����(��: Atom)  */
/* ���Ҥα��ˡ������֤������̣���뷸�������ղä��뤳�Ȥ��Ǥ��롣(��: H2)    */
/* ����(��)�򥫥å��Ǥ����äơ�����ñ�̤Ƿ����֤��������Ǥ��롣(��:(CH3)2) */
/* ���å���[]{}()�Σ�������Ȥ߹�碌�ƻ��ѤǤ��롣                          */
/* �ȡ�����δ֤˥��ڡ���������뤳�Ȥ��Ǥ��롣                              */
/* ���ڡ����ϡ��ȡ������ʬΥ�����̣���Ĥ�������ʳ��Ǥ�̵�뤵��롣      */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------���ֵ�ˡ----------------------------------*/
/* (������)                                                                  */
/*                                                                           */
/* �ѿ��Ͼ�ʸ����ʸ����ɽ����                                                */
/* �����0-9��������Ϣ³��ɽ�����롣��ο��ϰ۾�Ȥߤʤ���                   */
/* ���ڡ����ϥ����å���*�Ͼ軻��+���»���ɽ����                              */
/*---------------------------------------------------------------------------*/
/* (ʬ�Ҽ�)                                                                  */
/*                                                                           */
/* ���Ҥ���ʸ���ǻϤ�ơ���ʸ��(��)��Ǥ�դο��ղä�����Τ�ɽ����(��: Atom)  */
/* ���ڡ����ϥ����å���^�Ϥ����֤���&�Ϸ���ɽ����                          */
/*---------------------------------------------------------------------------*/

/* ʸ����Ƚ�̴ؿ� */
int is_end(char c) {return (c=='\0' || c=='\n');}
int is_left(char c) {return (c=='(' || c=='{' || c=='[');}
int is_right(char c) {return (c==')' || c=='}' || c==']');}
int is_pair(char ci, char co) {
  return ( (ci == '(' && co == ')')
        || (ci == '{' && co == '}')
        || (ci == '[' && co == ']') );
}
/* ʸ��c���б�����ͥ��Ȥ��ʤ����Υ��顼�ͤ����� */
int IllegalNest(char c) {
  int ret;

  switch (c) {
  case '(': ret = MOL_ILL_LEFT1; break;
  case '{': ret = MOL_ILL_LEFT2; break;
  case '[': ret = MOL_ILL_LEFT3; break;
  default:  ret = MOL_ILL_LEFT;
  }
  return ret;
}

int IsMolExpr(const char *str);
int IsCoefFact(const char *str);
int MolExpr (void);
int MolTerm (void);
int MolFact (void);
int CoefExpr(void);
int CoefTerm(void);
int CoefFact(void);
int Particle(void);
int Parameter(void);
int Number  (void);
void Space(void);

static const char *mol_si = NULL;
static char *mol_so = NULL;
static int mol_ni = 0;
static int mol_no = 0;
static int mol_max = 0;
static int mol_ret = MOL_SUCCESS;

/* ʬ�Ҽ����ɤ�(public��) */
int ReadMolExpression(const char *src, char *dest, int max) {
  mol_si = src;
  mol_so = dest;
  mol_ni = 0;
  mol_no = 0;
  mol_max = max;

  if ((mol_ret = MolExpr()) != MOL_SUCCESS)
    return mol_ret;
  mol_so[mol_no] = '\0';

  return (is_end(mol_si[mol_ni]))? MOL_SUCCESS: MOL_ILL_RIGHT;
}

/* ʬ�Ҽ����ɤ�(private��) */
int MolExpr (void) {
  /* ���Ĥ�ι���ɤ� */
  Space();
  if (!IsMolExpr(mol_si+mol_ni)) return MOL_ILL_EXPR;
  if ((mol_ret = MolTerm()) != MOL_SUCCESS)
    return mol_ret;

  while (IsMolExpr(mol_si+mol_ni)) {
    /* �ʸ�ι���ɤ� */
    Space();
    if ((mol_ret = MolTerm()) != MOL_SUCCESS)
      return mol_ret;

    /* Ϣ�뵭��&���ղä��� */
    if (mol_no > 0 && mol_so[mol_no-1] == ' ')
      mol_so[mol_no-1] = '&';
    else {
      if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
      mol_so[mol_no++] = '&';
    }
  }

  /* �֤�ͤ�� */
  Space();
  if (!is_right(mol_si[mol_ni]) && !is_end(mol_si[mol_ni]))
    return MOL_ILL_EXPR;

  return MOL_SUCCESS;
}

/* ʬ�ҹ���ɤ� */
int MolTerm(void) {
  /* ���Ҥ��ɤ� */
  if ((mol_ret = MolFact()) != MOL_SUCCESS)
    return mol_ret;
  Space();

  if (IsCoefFact(mol_si+mol_ni)) {
    /* ������¸�� */
    if ((mol_ret = CoefFact()) != MOL_SUCCESS)
      return mol_ret;

    /* �����֤�����^���ղä��� */
    if (mol_no > 0 && mol_so[mol_no-1] == ' ')
      mol_so[mol_no-1] = '^';
    else {
      if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
      mol_so[mol_no++] = '^';
    }
  }

  return MOL_SUCCESS;
}

/* ʬ�Ұ��Ҥ��ɤ� */
int MolFact(void) {
  if (isupper(mol_si[mol_ni])) {
    /* ���ҤǤ����� */
    if ((mol_ret = Particle()) != MOL_SUCCESS)
      return mol_ret;
    /* ���ڡ������ղä��� */
    if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
    mol_so[mol_no++] = ' ';
  } else {
    /* (ʬ�Ҽ�)�Ǥ����� */
    int c;

    if (!is_left(c = mol_si[mol_ni++])) return MOL_ILL_FACT;
    if ((mol_ret = MolExpr()) != MOL_SUCCESS)
      return mol_ret;
    if (!is_pair(c, mol_si[mol_ni++])) return IllegalNest(c);
  }

  return MOL_SUCCESS;
}

/* ���������ɤ� */
int CoefExpr(void) {
  Space();
  if ((mol_ret = CoefTerm()) != MOL_SUCCESS)
    return mol_ret;
  for (;;) {
    Space();
    if (mol_si[mol_ni] == '+') {
      /* �� */
      mol_ni++;
      if ((mol_ret = CoefTerm()) != MOL_SUCCESS)
        return mol_ret;
      /* �û�����+���ղä��� */
      if (mol_no > 0 && mol_so[mol_no-1] == ' ')
        mol_so[mol_no-1] = '+';
      else {
        if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
        mol_so[mol_no++] = '+';
      }
    } else if (mol_si[mol_ni] == '-') {
      /* �� */
      mol_ni++;
      if ((mol_ret = CoefTerm()) != MOL_SUCCESS)
        return mol_ret;
      /* ��������-���ղä��� */
      if (mol_no > 0 && mol_so[mol_no-1] == ' ')
        mol_so[mol_no-1] = '-';
      else {
        if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
        mol_so[mol_no++] = '-';
      }
    } else
      break;
  }

  return MOL_SUCCESS;
}

/* ��������ɤ� */
int CoefTerm(void) {
  if ((mol_ret = CoefFact()) != MOL_SUCCESS)
    return mol_ret;
  for (;;) {
    Space();
    if (mol_si[mol_ni] == '/') {
      mol_ni++;
      if ((mol_ret = CoefFact()) != MOL_SUCCESS)
        return mol_ret;
      /* ��������/���ղä��� */
      if (mol_no > 0 && mol_so[mol_no-1] == ' ')
        mol_so[mol_no-1] = '/';
      else {
        if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
        mol_so[mol_no++] = '/';
      }
    } else if (mol_si[mol_ni] == '*') {
      mol_ni++;
      if ((mol_ret = CoefFact()) != MOL_SUCCESS)
        return mol_ret;
      /* �軻����*���ղä��� */
      if (mol_no > 0 && mol_so[mol_no-1] == ' ')
        mol_so[mol_no-1] = '*';
      else {
        if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
        mol_so[mol_no++] = '*';
      }
    } else if (IsCoefFact(mol_si+mol_ni)) {
      if ((mol_ret = CoefFact()) != MOL_SUCCESS)
        return mol_ret;
      /* �軻����*���ղä��� */
      if (mol_no > 0 && mol_so[mol_no-1] == ' ')
        mol_so[mol_no-1] = '*';
      else {
        if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
        mol_so[mol_no++] = '*';
      }
    } else
      break;
  }

  return MOL_SUCCESS;
}

/* �������Ҥ��ɤ� */
int CoefFact(void) {
  if (isdigit(mol_si[mol_ni])) {
    /* ��� */
    if ((mol_ret = Number()) != MOL_SUCCESS)
      return mol_ret;
    /* ���ڡ������ղä��� */
    if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
    mol_so[mol_no++] = ' ';
  } else if (islower(mol_si[mol_ni])) {
    /* �ѿ� */
    if ((mol_ret = Parameter()) != MOL_SUCCESS)
      return mol_ret;
    /* ���ڡ������ղä��� */
    if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
    mol_so[mol_no++] = ' ';
  } else {
    /* (������) */
    int c;

    if (!is_left(c = mol_si[mol_ni++])) return MOL_ILL_CFACT;
    if ((mol_ret = CoefExpr()) != MOL_SUCCESS)
      return mol_ret;
    if (!is_pair(c, mol_si[mol_ni++])) return IllegalNest(c);
  }

  return MOL_SUCCESS;
}

/* ���Ҥ��ɤ� */
int Particle(void) {
  int c;

  if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
  mol_so[mol_no++] = mol_si[mol_ni++];
  while (mol_no < mol_max && !is_end(c = mol_si[mol_ni]) && islower(c)) {
    mol_ni++;
    mol_so[mol_no++] = c;
  }
  if (mol_no >= mol_max) return MOL_TOO_LONG;

  return MOL_SUCCESS;
}

/* �ѿ����ɤ� */
int Parameter(void) {
  if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
  mol_so[mol_no++] = mol_si[mol_ni++];

  return MOL_SUCCESS;
}

/* �������ɤ� */
int Number(void) {
  int c;

  while (mol_no < mol_max && !is_end(c = mol_si[mol_ni]) && isdigit(c)) {
    mol_ni++;
    mol_so[mol_no++] = c;
  }
  if (mol_no >= mol_max) return MOL_TOO_LONG;

  return MOL_SUCCESS;
}

/* ���ڡ������ɤ� */
void Space(void) {
  int c;

  while ((c = mol_si[mol_ni]) == ' ' || c == '\t')
    mol_ni++;
}

/* ʬ�Ҽ��Ǥ���ʤ鿿���֤� */
int IsMolExpr(const char *str) {
  int i, nest, c;

  nest = 0;
  for (i = 0; !is_end(c = str[i]) && nest >= 0; i++) {
    if (isupper(c)) return 1;
    if (is_left(c)) nest++;
    if (is_right(c)) nest--;
  }

  return 0;
}

/* �������ҤǤ���п����֤� */
int IsCoefFact(const char *str) {
  if (is_left(*str))
    return (!IsMolExpr(str+1));
  return (isdigit(*str) || islower(*str));
}
