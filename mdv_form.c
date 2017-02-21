/* "mdv_form.c" データファイルのフォーマット設定関係 */

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

/*----------------------------パラメータ設定関連-----------------------------*/

/* リストの初期化 */
static void InitParameters(ParameterList *pparam) {
  int i;

  pparam->n = 0;
  for (i = 0; i < MAXPARAMETER; i++) {
    pparam->c[i] = '\0';
    pparam->v[i] = 0;
  }
}

/* 変数の取得(失敗は負を返す) */
int  GetParameters(const char *mol, ParameterList *pparam) {
  int i;

  /* 初期化 */
  InitParameters(pparam);

  while (*mol != '\0') {
    /* 変数名の探索 */
    if (islower(*mol)) {
      /* 変数発見 */
      for (i = 0; i < pparam->n && *mol != pparam->c[i]; i++)
        ;
      if (i >= pparam->n) {
        /* 新しい変数の登録 */
        if (pparam->n >= MAXPARAMETER)
          return -1;
        pparam->c[i] = *mol;
        pparam->n++;
      }
    }
    /* 次のトークンまでスキップ */
    while (isalnum(*mol))
      mol++;
    while (*mol != '\0' && !isalnum(*mol))
      mol++;
  }

  return pparam->n;
}

/* 変数の値の設定(成功は真、失敗は偽を返す) */
int SetParameterValue(int nvalue, int *value, ParameterList *pparam) {
  int i;

  for (i = 0; i < nvalue; i++) {
    /* とりあえず相対指定のみ可能 */
    if (i >= pparam->n)
      return 0;
    pparam->v[i] = value[i];
  }

  return 1;
}

/* 正数valをmax文字以下の文字列に変換。成功は出力バイト数、失敗は負を返す) */
static int _i2str(int val, char *str, int max) {
  char c;
  int i, n;

  if (val <= 0) return -1;
  /* 逆順に記入 */
  for (i = 0; i < max-1 && val > 0; i++) {
    str[i] = val%10 + '0';
    val /= 10;
  }
  if (i >= max-1) return -1;
  n = i;
  /* 反転 */
  for (i = 0; i < n/2; i++) {
    c = str[i];
    str[i] = str[n-1-i];
    str[n-1-i] = c;
  }
  str[n] = '\0';

  return n;
}

#define _MAXSTACKLEVEL 20
/* 変数の代入、式の簡略化(成功は出力バイト数、失敗は負を返す) */
int Substitute(const char *src, char *dst, int max,
                                                 const ParameterList *pparam) {
  int stack[_MAXSTACKLEVEL];
  int stack_n = 0;
  int size[_MAXSTACKLEVEL];
  int n; /* 出力したバイト数 */
  int i;

  n = 0;
  while (*src != '\0') {
    if (*src == ' ') {
      /* スペース */
      if (stack_n <= 0) return -1;
      if (size[stack_n-1] > 0) {
        /* 分子式(非０要素)に限りスペースを加える */
        if (n >= max-1) return -1;
        dst[n++] = *src;
        size[stack_n-1]++;
      }
      src++;
    } else if (isupper(*src)) {
      /* 原子 */
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
      /* 変数 */
      if (stack_n >= _MAXSTACKLEVEL) return -1;
      for (i = 0; i < pparam->n && *src != pparam->c[i]; i++)
        ;
      if (i >= pparam->n) return -1;
      stack[stack_n] = pparam->v[i];
      size[stack_n] = 0;
      stack_n++;
      src++;
    } else if (isdigit(*src)) {
      /* 数字 */
      if (stack_n >= _MAXSTACKLEVEL) return -1;
      stack[stack_n] = atoi(src);
      size[stack_n] = 0;
      stack_n++;
      while (isalnum(*src))
        src++;
    } else if (*src == '&') {
      /* 連結 */
      if (stack_n < 2) return -1;
      if (stack[stack_n-1] <= 0) {
        /* 連結先が０要素 */
        ;
      } else if (stack[stack_n-2] <= 0) {
        /* 連結元が０要素 */
        if (size[stack_n-2] > 0) return -1;
        size[stack_n-2] = size[stack_n-1];
      } else {
        /* 通常の連結 */
        if (n >= max-1) return -1;
        dst[n++] = *src;
        size[stack_n-2] += size[stack_n-1] + 1;
      }
      src++;
      stack[stack_n-2] += stack[stack_n-1];
      stack_n--;
    } else if (*src == '^') {
      /* くり返し */
      if (stack_n < 2 || stack[stack_n-1] < 0) return -1;
      if (stack[stack_n-2] <= 0 || stack[stack_n-1] <= 0) {
        /* ０回くり返し */
        n -= size[stack_n-2]+size[stack_n-1];
        stack[stack_n-2] = 0;
        size[stack_n-2] = 0;
      } else if (stack[stack_n-1] == 1) {
        /* １回くり返し */
        ;
      } else {
        /* 通常のくり返し */
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
      /* 和 */
      if (stack_n < 2) return -1;
      stack[stack_n-2] += stack[stack_n-1];
      n -= size[stack_n-1];
      stack_n--;
      src++;
    } else if (*src == '-') {
      /* 差 */
      if (stack_n < 2) return -1;
      stack[stack_n-2] -= stack[stack_n-1];
      n -= size[stack_n-1];
      stack_n--;
      src++;
    } else if (*src == '*') {
      /* 積 */
      if (stack_n < 2) return -1;
      stack[stack_n-2] *= stack[stack_n-1];
      n -= size[stack_n-1];
      stack_n--;
      src++;
    } else if (*src == '/') {
      /* 商 */
      if (stack_n < 2 || stack[stack_n-2] == 0) return -1;
      stack[stack_n-2] /= stack[stack_n-1];
      n -= size[stack_n-1];
      stack_n--;
      src++;
    } else {
      /* 異常な値 */
      return -1;
    }
  }
  dst[n] = '\0';

  return (stack_n == 1)? n: -1;
}

/* 総粒子数の取得(失敗は負を返す) */
MDV_Size TotalParticles(const char *mol, AtomID (*pf)(const char *)) {
  return SetAtomTable(mol, NULL, pf);
}

/* 粒子→原子対応表の設定(成功は総粒子数、失敗は負を返す) */
MDV_Size SetAtomTable(const char *mol, AtomID *atom,
                                                  AtomID (*pf)(const char *)) {
  MDV_Size stack[_MAXSTACKLEVEL];
  int stack_n = 0;
  MDV_Size n; /* 出力した粒子数 */
  MDV_Size i, j;

  n = 0;
  while (*mol != '\0') {
    if (*mol == ' ') {
      /* スペース */
      mol++;
    } else if (isupper(*mol)) {
      /* 原子 */
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
      /* 数字 */
      if (stack_n >= _MAXSTACKLEVEL)
        return -1;
      stack[stack_n++] = atoi(mol);
      while (isalnum(*mol))
        mol++;
    } else if (*mol == '&') {
      /* 連結 */
      if (stack_n < 2)
        return -1;
      stack[stack_n-2] += stack[stack_n-1];
      stack_n--;
      mol++;
    } else if (*mol == '^') {
      /* くり返し */
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
      /* 異常な値 */
      return -1;
    }
  }

  return (stack_n == 1)? stack[0]: -1;
}

/*-----------------------挿入記法→後置記法への変換関係----------------------*/
/*                                                                           */
/*---------------------------------挿入記法----------------------------------*/
/* (係数式)                                                                  */
/*                                                                           */
/* 変数は小文字一文字で表す。                                                */
/* 定数は0-9の整数の連続で表現する。負の数は異常とみなす。                   */
/* 四則演算のみサポートする。                                                */
/* カッコは[]{}()の３種類を組み合わせて使用できる。                          */
/* トークンの間にスペースを入れることができる。                              */
/* 定数と変数が並んだ場合か、間にスペースのみ存在する場合、乗算記号*が省略   */
/* されているとみなす。                                                      */
/*---------------------------------------------------------------------------*/
/* (分子式)                                                                  */
/*                                                                           */
/* 原子は大文字で始めて、小文字(列)を任意の数付加したもので表す。(例: Atom)  */
/* 原子の右に、繰り返し数を意味する係数式を付加することができる。(例: H2)    */
/* 原子(列)をカッコでくくって、その単位で繰り返し数を指定できる。(例:(CH3)2) */
/* カッコは[]{}()の３種類を組み合わせて使用できる。                          */
/* トークンの間にスペースを入れることができる。                              */
/* スペースは、トークンを分離する意味をもつが、それ以外では無視される。      */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------後置記法----------------------------------*/
/* (係数式)                                                                  */
/*                                                                           */
/* 変数は小文字一文字で表す。                                                */
/* 定数は0-9の整数の連続で表現する。負の数は異常とみなす。                   */
/* スペースはスタック、*は乗算、+は和算を表す。                              */
/*---------------------------------------------------------------------------*/
/* (分子式)                                                                  */
/*                                                                           */
/* 原子は大文字で始めて、小文字(列)を任意の数付加したもので表す。(例: Atom)  */
/* スペースはスタック、^はくり返し、&は結合を表す。                          */
/*---------------------------------------------------------------------------*/

/* 文字の判別関数 */
int is_end(char c) {return (c=='\0' || c=='\n');}
int is_left(char c) {return (c=='(' || c=='{' || c=='[');}
int is_right(char c) {return (c==')' || c=='}' || c==']');}
int is_pair(char ci, char co) {
  return ( (ci == '(' && co == ')')
        || (ci == '{' && co == '}')
        || (ci == '[' && co == ']') );
}
/* 文字cに対応するネストがない場合のエラー値の設定 */
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

/* 分子式を読む(public版) */
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

/* 分子式を読む(private版) */
int MolExpr (void) {
  /* １つめの項を読む */
  Space();
  if (!IsMolExpr(mol_si+mol_ni)) return MOL_ILL_EXPR;
  if ((mol_ret = MolTerm()) != MOL_SUCCESS)
    return mol_ret;

  while (IsMolExpr(mol_si+mol_ni)) {
    /* 以後の項を読む */
    Space();
    if ((mol_ret = MolTerm()) != MOL_SUCCESS)
      return mol_ret;

    /* 連結記号&を付加する */
    if (mol_no > 0 && mol_so[mol_no-1] == ' ')
      mol_so[mol_no-1] = '&';
    else {
      if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
      mol_so[mol_no++] = '&';
    }
  }

  /* 間を詰める */
  Space();
  if (!is_right(mol_si[mol_ni]) && !is_end(mol_si[mol_ni]))
    return MOL_ILL_EXPR;

  return MOL_SUCCESS;
}

/* 分子項を読む */
int MolTerm(void) {
  /* 因子を読む */
  if ((mol_ret = MolFact()) != MOL_SUCCESS)
    return mol_ret;
  Space();

  if (IsCoefFact(mol_si+mol_ni)) {
    /* 係数が存在 */
    if ((mol_ret = CoefFact()) != MOL_SUCCESS)
      return mol_ret;

    /* くり返し記号^を付加する */
    if (mol_no > 0 && mol_so[mol_no-1] == ' ')
      mol_so[mol_no-1] = '^';
    else {
      if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
      mol_so[mol_no++] = '^';
    }
  }

  return MOL_SUCCESS;
}

/* 分子因子を読む */
int MolFact(void) {
  if (isupper(mol_si[mol_ni])) {
    /* 原子である場合 */
    if ((mol_ret = Particle()) != MOL_SUCCESS)
      return mol_ret;
    /* スペースを付加する */
    if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
    mol_so[mol_no++] = ' ';
  } else {
    /* (分子式)である場合 */
    int c;

    if (!is_left(c = mol_si[mol_ni++])) return MOL_ILL_FACT;
    if ((mol_ret = MolExpr()) != MOL_SUCCESS)
      return mol_ret;
    if (!is_pair(c, mol_si[mol_ni++])) return IllegalNest(c);
  }

  return MOL_SUCCESS;
}

/* 係数式を読む */
int CoefExpr(void) {
  Space();
  if ((mol_ret = CoefTerm()) != MOL_SUCCESS)
    return mol_ret;
  for (;;) {
    Space();
    if (mol_si[mol_ni] == '+') {
      /* 和 */
      mol_ni++;
      if ((mol_ret = CoefTerm()) != MOL_SUCCESS)
        return mol_ret;
      /* 加算記号+を付加する */
      if (mol_no > 0 && mol_so[mol_no-1] == ' ')
        mol_so[mol_no-1] = '+';
      else {
        if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
        mol_so[mol_no++] = '+';
      }
    } else if (mol_si[mol_ni] == '-') {
      /* 差 */
      mol_ni++;
      if ((mol_ret = CoefTerm()) != MOL_SUCCESS)
        return mol_ret;
      /* 減算記号-を付加する */
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

/* 係数項を読む */
int CoefTerm(void) {
  if ((mol_ret = CoefFact()) != MOL_SUCCESS)
    return mol_ret;
  for (;;) {
    Space();
    if (mol_si[mol_ni] == '/') {
      mol_ni++;
      if ((mol_ret = CoefFact()) != MOL_SUCCESS)
        return mol_ret;
      /* 除算記号/を付加する */
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
      /* 乗算記号*を付加する */
      if (mol_no > 0 && mol_so[mol_no-1] == ' ')
        mol_so[mol_no-1] = '*';
      else {
        if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
        mol_so[mol_no++] = '*';
      }
    } else if (IsCoefFact(mol_si+mol_ni)) {
      if ((mol_ret = CoefFact()) != MOL_SUCCESS)
        return mol_ret;
      /* 乗算記号*を付加する */
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

/* 係数因子を読む */
int CoefFact(void) {
  if (isdigit(mol_si[mol_ni])) {
    /* 定数 */
    if ((mol_ret = Number()) != MOL_SUCCESS)
      return mol_ret;
    /* スペースを付加する */
    if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
    mol_so[mol_no++] = ' ';
  } else if (islower(mol_si[mol_ni])) {
    /* 変数 */
    if ((mol_ret = Parameter()) != MOL_SUCCESS)
      return mol_ret;
    /* スペースを付加する */
    if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
    mol_so[mol_no++] = ' ';
  } else {
    /* (係数式) */
    int c;

    if (!is_left(c = mol_si[mol_ni++])) return MOL_ILL_CFACT;
    if ((mol_ret = CoefExpr()) != MOL_SUCCESS)
      return mol_ret;
    if (!is_pair(c, mol_si[mol_ni++])) return IllegalNest(c);
  }

  return MOL_SUCCESS;
}

/* 原子を読む */
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

/* 変数を読む */
int Parameter(void) {
  if (mol_no+1 >= mol_max) return MOL_TOO_LONG;
  mol_so[mol_no++] = mol_si[mol_ni++];

  return MOL_SUCCESS;
}

/* 数字を読む */
int Number(void) {
  int c;

  while (mol_no < mol_max && !is_end(c = mol_si[mol_ni]) && isdigit(c)) {
    mol_ni++;
    mol_so[mol_no++] = c;
  }
  if (mol_no >= mol_max) return MOL_TOO_LONG;

  return MOL_SUCCESS;
}

/* スペースを読む */
void Space(void) {
  int c;

  while ((c = mol_si[mol_ni]) == ' ' || c == '\t')
    mol_ni++;
}

/* 分子式であるなら真を返す */
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

/* 係数因子であれば真を返す */
int IsCoefFact(const char *str) {
  if (is_left(*str))
    return (!IsMolExpr(str+1));
  return (isdigit(*str) || islower(*str));
}
