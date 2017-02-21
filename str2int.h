/* ʸ���� -> ʸ����ID �Ѵ��ơ��֥뷿 �إå� */

#ifndef STR2INT_H
#define STR2INT_H

typedef int Str2Int_Size;

typedef struct {
  Str2Int_Size  buf_n;      /* ʸ����Хåե��Υ����������� */
  Str2Int_Size  buf_max;    /* ʸ����Хåե��Υ����� */
  char         *buf;        /* ʸ������Ͽ�ѥХåե� (realloc()����) */
  Str2Int_Size  hash_max;   /* �ϥå���Υ����� */
  Str2Int_Size  hash_prime; /* hash_max�ʲ��κ�����ǿ� */
  Str2Int_Size *hash_id;    /* �ϥå����� -> ʸ����ID */
  Str2Int_Size  id_n;       /* ʸ����ID��ȯ�Կ�(�󡢼�����) */
  Str2Int_Size  id_max;     /* ʸ����ID�Хåե��Υ����� */
  Str2Int_Size *id_bufid;   /* ʸ����ID -> ʸ������� (realloc()����) */
} Str2Int_Type;

extern Str2Int_Type *Str2Int_Alloc();
extern void          Str2Int_Free(Str2Int_Type *sip);
extern Str2Int_Size  Str2Int_Str2ID(const char *str, Str2Int_Type *sip);
extern Str2Int_Size  Str2Int_SearchID(const char *str, Str2Int_Type *sip);
extern const char   *Str2Int_ID2Str(Str2Int_Size id, Str2Int_Type *sip);

#endif /* STR2INT_H */

