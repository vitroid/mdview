/* ハッシュテーブル型 ヘッダ */

#ifndef _HASH_H
#define _HASH_H

#include "chunk.h"

typedef int Hash_Size;

typedef struct {
  Hash_Size size_max; /* ハッシュサイズの上限(sizeof(void *)単位) */
  Hash_Size size_n;   /* ハッシュの実サイズ(sizeof(void *)単位) */
  Hash_Size n;        /* 登録した数 */
  void **table;       /* ハッシュテーブル */
  Chunk_Type *chunk;  /* 内部のメモリ管理用 */
} Hash_Type;

extern Hash_Type *Hash_Alloc(Hash_Size min_size, Hash_Size max_size);
extern void Hash_Free(Hash_Type *hp, void (*ffree)(void *));
extern void *Hash_Insert(Hash_Type *hp, void *p,
    int (*fcmp)(const void *, const void *), Hash_Size (*fhash)(const void *),
    void *(*fcpy)(const void *));
extern void *Hash_Search(const Hash_Type *hp, const void *key,
    int (*fcmp)(const void *, const void *), Hash_Size (*fhash)(const void *));
extern int Hash_Delete(Hash_Type *hp, const void *key,
    int (*fcmp)(const void *, const void *), Hash_Size (*fhash)(const void *),
    void (*ffree)(void *));
extern void Hash_DeleteAll(Hash_Type *hp, void (*ffree)(void *));
extern Hash_Size Hash_Total(const Hash_Type *hp);
extern void Hash_ReadAll(const Hash_Type *hp, void **buf);

#endif /* _HASH_H */

