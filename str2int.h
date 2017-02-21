/* 文字列 -> 文字列ID 変換テーブル型 ヘッダ */

#ifndef STR2INT_H
#define STR2INT_H

typedef int Str2Int_Size;

typedef struct {
  Str2Int_Size  buf_n;      /* 文字列バッファのアクセス位置 */
  Str2Int_Size  buf_max;    /* 文字列バッファのサイズ */
  char         *buf;        /* 文字列登録用バッファ (realloc()方式) */
  Str2Int_Size  hash_max;   /* ハッシュのサイズ */
  Str2Int_Size  hash_prime; /* hash_max以下の最大の素数 */
  Str2Int_Size *hash_id;    /* ハッシュ値 -> 文字列ID */
  Str2Int_Size  id_n;       /* 文字列IDの発行数(兼、次の値) */
  Str2Int_Size  id_max;     /* 文字列IDバッファのサイズ */
  Str2Int_Size *id_bufid;   /* 文字列ID -> 文字列位置 (realloc()方式) */
} Str2Int_Type;

extern Str2Int_Type *Str2Int_Alloc();
extern void          Str2Int_Free(Str2Int_Type *sip);
extern Str2Int_Size  Str2Int_Str2ID(const char *str, Str2Int_Type *sip);
extern Str2Int_Size  Str2Int_SearchID(const char *str, Str2Int_Type *sip);
extern const char   *Str2Int_ID2Str(Str2Int_Size id, Str2Int_Type *sip);

#endif /* STR2INT_H */

