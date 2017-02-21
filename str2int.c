/* ʸ���� -> ʸ����ID �Ѵ��ơ��֥뷿 v0.70 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "str2int.h"

typedef unsigned int Str2Int_USize;

#define STR2INT_HASHMAX_MIN 256
#define STR2INT_BUFMAX_MIN 4096
#define STR2INT_NIL (-1)

/* �ҡ��פγ��ݡ����� */
#define _Str2Int_Malloc(n)    malloc(n)
#define _Str2Int_Realloc(p,n) realloc((p),(n))
#define _Str2Int_Free(n)      free(n)

/* �����ƥ२�顼 */
static void Str2Int_Fatal(const char *str) {
  fprintf(stderr, "Str2Int: %s\n", str);
  exit(1);
}

/* n�ʲ��Ǻ�����ǿ����֤� */
static Str2Int_Size Str2Int_Prime(Str2Int_Size n) {
  Str2Int_Size p = 0;
  Str2Int_Size i;

  if (n < 2) {
    Str2Int_Fatal("Str2Int_Prime(): system error.");
  } else if (n == 2) {
    p = 2;
  } else {
    for (p = (n % 2)? n: n-1; ; p -= 2) {
      for (i = 3; i*i <= p && p % i != 0; i += 2)
        ;
      if (i*i > p)
        break;
    }
  }

  return p;
}

/* ���󥹥��󥹤�����(���Ԥϰ۾ｪλ) */
Str2Int_Type *Str2Int_Alloc() {
  Str2Int_Type *sip;
  Str2Int_Size i;

  if ((sip = (Str2Int_Type *) _Str2Int_Malloc(sizeof(Str2Int_Type))) == NULL)
    Str2Int_Fatal("Allocation failed.");

  sip->buf_n = 0;
  sip->buf_max = STR2INT_BUFMAX_MIN;
  sip->hash_max = STR2INT_HASHMAX_MIN;
  sip->hash_prime = Str2Int_Prime(sip->hash_max);
  sip->id_n = 0;
  sip->id_max = STR2INT_HASHMAX_MIN;

  if ((sip->buf = (char *)
        _Str2Int_Malloc(sip->buf_max)) == NULL
      || (sip->hash_id = (Str2Int_Size *)
        _Str2Int_Malloc(sizeof(Str2Int_Size) * sip->hash_max)) == NULL
      || (sip->id_bufid = (Str2Int_Size *)
        _Str2Int_Malloc(sizeof(Str2Int_Size) * sip->id_max)) == NULL )
    Str2Int_Fatal("Allocation failed.");

  for (i = 0; i < sip->hash_max; i++)
    sip->hash_id[i] = STR2INT_NIL;
  for (i = 0; i < sip->id_max; i++)
    sip->id_bufid[i] = STR2INT_NIL;

  return sip;
}

/* ���󥹥��󥹤ξõ� */
void Str2Int_Free(Str2Int_Type *sip) {
  if (sip == NULL)
    return;
  _Str2Int_Free(sip->buf);
  _Str2Int_Free(sip->id_bufid);
  _Str2Int_Free(sip->hash_id);
  _Str2Int_Free(sip);
}

/* ---- ʸ����Хåե� ----------------------------------------------------- */

static Str2Int_Size Str2Int_Str2BufID(const char *str, Str2Int_Type *sip) {
  Str2Int_Size i, bufid;

  while ((i = sip->buf_n + strlen(str) + 1) > sip->buf_max) {
    /* �ΰ���­�����ľ�� */
    char *p;

    sip->buf_max = sip->buf_max*2; /* �ܤΥ����� */
    if ((p = (char *) _Str2Int_Realloc(sip->buf, sip->buf_max)) == NULL)
      Str2Int_Fatal("Allocation failed.");
    sip->buf = p;
  }
  bufid = sip->buf_n;
  strcpy(sip->buf + sip->buf_n, str);
  sip->buf_n = i;

  return bufid;
}

/* ---- �ϥå��� ----------------------------------------------------------- */

/* ʸ����str�Υϥå����ͤ��֤� */
static Str2Int_Size Str2Int_Str2Hash(const char *str, Str2Int_Type *sip) {
  Str2Int_USize i, h, h1, h2, prime;
  Str2Int_Size id;
  const char *p;

  prime = sip->hash_prime;

  /* �켡�ϥå���(�軻���Ѥ���ʸ����Υϥå���) */
  h = 0;
  p = str;
  while (*p)
    h = h*31 + *p++;
  h1 = h % prime;
  if ((id = sip->hash_id[h1]) == STR2INT_NIL
      || strcmp(str, sip->buf + sip->id_bufid[id]) == 0)
    return h1;

  /* �󼡥ϥå���(16bit�軻���Ѥ���32bit�����Υϥå���) */
  h ^= (((h & 0xFFFFUL)*0x58A68FF7UL) & 0xFFFF0000UL) ^ 0xF3FCEB5AUL;
  h ^= (((h >> 16)*0x8C67AD8FUL) >> 16) ^ 0xB6B67D3FUL;
  h ^= (((h & 0xFFFFUL)*0xEA6CAF69UL) & 0xFFFF0000UL) ^ 0xBF55FB92UL;
  h2 = h % prime;
  if (h2 <= 0) h2 = 1;
  for (i = (h1+h2) % prime; i != h1; i = (i+h2) % prime) {
    if ((id = sip->hash_id[i]) == STR2INT_NIL
        || strcmp(str, sip->buf + sip->id_bufid[id]) == 0)
      break;
  }
  if (i == h1) {
    /* ��Ͽ��꤬�ʤ� */
    Str2Int_Fatal("Str2Int_Str2Hash(): system error.");
  }

  return i;
}

/* ---- ʸ����ID ----------------------------------------------------------- */

/* ʸ����str��ID���֤�(̤�Τ�ʸ����ξ�硢����֤�) */
Str2Int_Size Str2Int_SearchID(const char *str, Str2Int_Type *sip) {
  Str2Int_Size h;

  if (str == NULL || sip == NULL)
    return STR2INT_NIL;

  /* ���� */
  h = Str2Int_Str2Hash(str, sip);

  return sip->hash_id[h];
}

/* ʸ����str��ID���֤�(̤�Τ�ʸ����ξ�硢������Ͽ����) */
Str2Int_Size Str2Int_Str2ID(const char *str, Str2Int_Type *sip) {
  Str2Int_Size h, id;

  if (str == NULL || sip == NULL)
    return STR2INT_NIL;

  /* ���� */
  h = Str2Int_Str2Hash(str, sip);

  if (sip->hash_id[h] == STR2INT_NIL) {
    if ((id = sip->id_n) >= sip->id_max) {
      /* ID���ΰ���­�����ľ�� */
      Str2Int_Size *p;
      Str2Int_Size i;

      if (sip->id_max*2 < 0)
        Str2Int_Fatal("Str2Int_Str2ID(): id_max overflow.");
      sip->id_max = sip->id_max*2;
      if ((p = (Str2Int_Size *) _Str2Int_Realloc(sip->id_bufid,
          sizeof(Str2Int_Size) * sip->id_max)) == NULL)
        Str2Int_Fatal("Allocation failed.");
      sip->id_bufid = p;
      for (i = sip->id_n; i < sip->id_max; i++)
        sip->id_bufid[i] = STR2INT_NIL;
    }

    if (sip->id_n >= sip->hash_prime-(sip->hash_prime/10)) { /* ����Ψ9�� */
      /* �ϥå�����ΰ���­�����ľ���� */
      Str2Int_Size *p;
      Str2Int_Size i;

      if ((sip->hash_max/sizeof(Str2Int_Size))*2 < 0)
        Str2Int_Fatal("Str2Int_Str2ID(): hash_max overflow.");
      sip->hash_max = sip->hash_max*2;
      sip->hash_prime = Str2Int_Prime(sip->hash_max);
      if ((p = (Str2Int_Size *) _Str2Int_Realloc(sip->hash_id,
          sizeof(Str2Int_Size) * sip->hash_max)) == NULL)
        Str2Int_Fatal("Allocation failed.");
      sip->hash_id = p;
      for (i = 0; i < sip->hash_max; i++)
        sip->hash_id[i] = STR2INT_NIL;

      /* ����Ͽ */
      for (i = 0; i < sip->id_n; i++) {
        h = Str2Int_Str2Hash(sip->buf + sip->id_bufid[i], sip);
        if (sip->hash_id[h] != STR2INT_NIL)
          Str2Int_Fatal("Str2Int_Str2ID(): system error.");
        sip->hash_id[h] = i;
      }
      h = Str2Int_Str2Hash(str, sip);
    }

    /* ������Ͽ */
    sip->hash_id[h] = id;
    sip->id_bufid[id] = Str2Int_Str2BufID(str, sip);
    sip->id_n++;
  }

  return sip->hash_id[h];
}

/* id���б�����ʸ����ؤΥݥ��󥿤��֤�(���Ԥ�NULL) */
const char *Str2Int_ID2Str(Str2Int_Size id, Str2Int_Type *sip) {
  Str2Int_Size bufid;

  if (sip == NULL || id < 0 || id >= sip->id_n)
    return NULL;
  if ((bufid = sip->id_bufid[id]) < 0)
    return NULL;

  return sip->buf + bufid;
}

