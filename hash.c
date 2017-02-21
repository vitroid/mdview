/* ハッシュテーブル型 v0.60 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "chunk.h"
#include "hash.h"

/* ---- 各種設定 ----------------------------------------------------------- */

typedef unsigned int Hash_USize; /* Hash_Sizeの符号なし版 */

#define HASH_ASSERT_ENABLE /* 整合性検査を有効にする */

/* ヒープの確保、開放 */
#define _Hash_Malloc(n) malloc(n)
#define _Hash_Free(n)   free(n)

/* ---- 内部用宣言 --------------------------------------------------------- */

#undef NDEBUG
#ifndef HASH_ASSERT_ENABLE
#define NDEBUG
#endif /* HASH_ASSERT_ENABLE */

#define HASH_MINSIZE 256
#define HASH_CHUNK_MINSIZE 256

/* ノード型。同じhash値の線形リストで、終端はNULL */
typedef struct _Hash_Node {
  struct _Hash_Node *next; /* 次のノードへのポインタ */
  void *p;                 /* 登録されたデータへのポインタ */
} Hash_Node;

/* 追加可能なノード数(現在100%) */
#define Hash_Limit(n) (n)

static int Hash_Check(Hash_Type *hp);

/* ---- ハッシュテーブル型 ------------------------------------------------- */

/* インスタンスの生成(失敗はNULLを返す) */
Hash_Type *Hash_Alloc(Hash_Size min_size, Hash_Size max_size) {
  Hash_Type *hp;
  Hash_Size i;

  if (min_size < 0 || max_size < 0)
    return NULL;

  if ((hp = (Hash_Type *) _Hash_Malloc(sizeof(Hash_Type))) == NULL)
    goto heap_error;
  hp->n = 0;
  hp->chunk = NULL;
  hp->table = NULL;

  min_size /= sizeof(void *);
  if (min_size <= HASH_MINSIZE)
    hp->size_n = HASH_MINSIZE;
  else
    hp->size_n = min_size;
  if (max_size == 0)
    max_size = (~((Hash_USize) 0))>>1;
  max_size /= sizeof(void *);
  if (max_size/2 <= HASH_MINSIZE)
    hp->size_max = HASH_MINSIZE;
  else
    hp->size_max = max_size/2;

  if ((hp->chunk = Chunk_TypeAlloc(sizeof(Hash_Node), HASH_CHUNK_MINSIZE,
      hp->size_max*sizeof(void *))) == NULL)
    goto heap_error;

  if ((hp->table = (void **) _Hash_Malloc(sizeof(void *) * hp->size_n)) == NULL)
    goto heap_error;
  for (i = 0; i < hp->size_n; i++)
    hp->table[i] = NULL;

  return hp;

heap_error:
  if (hp != NULL) {
    if (hp->table != NULL)
      _Hash_Free(hp->table);
    if (hp->chunk != NULL)
      Chunk_TypeFree(hp->chunk);
    _Hash_Free(hp);
  }
  return NULL;
}

/* インスタンスの消去 */
void Hash_Free(Hash_Type *hp, void (*ffree)(void *)) {
  if (hp == NULL)
    return;

  assert(Hash_Check(hp));
  if (ffree != NULL) {
    Hash_Size i;
    Hash_Node *np;

    for (i = 0; i < hp->size_n; i++) {
      for (np = (Hash_Node *) hp->table[i]; np != NULL; np = np->next)
        ffree(np->p);
      hp->table[i] = NULL;
    }
  }

  hp->size_max = 0;
  hp->size_n = 0;
  hp->n = 0;
  _Hash_Free(hp->table);
  hp->table = NULL;
  Chunk_TypeFree(hp->chunk);
  hp->chunk = NULL;
  _Hash_Free(hp);
}

/* ハッシュテーブルの探索(失敗はNULL) */
void *Hash_Search(const Hash_Type *hp, const void *key,
    int (*fcmp)(const void *, const void *),
    Hash_Size (*fhash)(const void *)) {
  Hash_Size h;
  Hash_Node *np;

  if (hp == NULL || key == NULL || fcmp == NULL || fhash == NULL)
    return NULL;

  h = ((Hash_USize) fhash(key)) % ((Hash_USize) hp->size_n);
  for (np = (Hash_Node *) hp->table[h];
      np != NULL && fcmp(key, np->p) != 0; np = np->next)
    ;

  return (np == NULL)? NULL: np->p;
}

/* ハッシュテーブルへの登録(失敗はNULL) */
void *Hash_Insert(Hash_Type *hp, void *p,
    int (*fcmp)(const void *, const void *),
    Hash_Size (*fhash)(const void *),
    void *(*fcpy)(const void *)) {
  Hash_Size h;
  Hash_Node *np;

  if (hp == NULL || p == NULL || fcmp == NULL || fhash == NULL)
    return NULL;

  if (hp->n >= Hash_Limit(hp->size_n)) {
    Hash_Size size_n, i;
    void **table;

    /* ハッシュの領域不足。取り直す */
    assert(Hash_Check(hp));
    if (hp->size_n*2 > hp->size_max)
      return NULL; /* これ以上取れない */
    size_n = hp->size_n;
    table = hp->table;
    hp->size_n *= 2;
    if ((hp->table = (void **)
        _Hash_Malloc(sizeof(void *) * hp->size_n)) == NULL)
      return NULL;
    for (i = 0; i < hp->size_n; i++)
      hp->table[i] = NULL;

    /* 再登録 */
    for (i = 0; i < size_n; i++) {
      Hash_Node *next;

      for (np = (Hash_Node *) table[i]; np != NULL; np = next) {
        next = np->next;
        h = ((Hash_USize) fhash(np->p)) % ((Hash_USize) hp->size_n);
        np->next = (Hash_Node *) hp->table[h];
        hp->table[h] = (void *) np;
      }
      table[i] = NULL;
    }
    _Hash_Free(table);
    assert(Hash_Check(hp));
  }

  /* 検索 */
  h = ((Hash_USize) fhash(p)) % ((Hash_USize) hp->size_n);
  for (np = (Hash_Node *) hp->table[h];
      np != NULL && fcmp(p, np->p) != 0; np = np->next)
    ;
  if (np != NULL)
    return NULL; /* すでに存在する */

  /* 登録 */
  if ((np = (Hash_Node *) Chunk_NodeAlloc(hp->chunk)) == NULL)
    return NULL; /* 登録不能 */
  np->next = (Hash_Node *) hp->table[h];
  hp->table[h] = (void *) np;
  if (fcpy != NULL)
    p = fcpy(p);
  np->p = p;
  hp->n++;

  return p;
}

/* ハッシュテーブルからの削除(返値は成功の真偽) */
int Hash_Delete(Hash_Type *hp, const void *key,
    int (*fcmp)(const void *, const void *),
    Hash_Size (*fhash)(const void *),
    void (*ffree)(void *)) {
  Hash_Size h;
  Hash_Node *np;

  if (hp == NULL || key == NULL || fcmp == NULL || fhash == NULL)
    return 0;

  /* 検索 */
  h = ((Hash_USize) fhash(key)) % ((Hash_USize) hp->size_n);
  np = (Hash_Node *) hp->table[h];
  if (np == NULL)
    return 0; /* 見つからない */

  if (fcmp(key, np->p) == 0) {
    /* 最初のノードの削除 */
    hp->table[h] = (void *) (np->next);
  } else {
    Hash_Node *prev;

    do {
      prev = np;
      np = np->next;
    } while (np != NULL && fcmp(key, np->p) != 0);
    if (np == NULL)
      return 0; /* 見つからない */
    prev->next = np->next;
  }

  /* 削除 */
  if (ffree != NULL)
    ffree(np->p);
  Chunk_NodeFree(hp->chunk, np);
  hp->n--;

  return 1;
}

/* 全てのデータの削除 */
void Hash_DeleteAll(Hash_Type *hp, void (*ffree)(void *)) {
  Hash_Size i;
  Hash_Node *np, *next;

  if (hp == NULL)
    return;

  assert(Hash_Check(hp));

  for (i = 0; i < hp->size_n; i++) {
    np = (Hash_Node *) hp->table[i];
    while (np != NULL) {
      next = np->next;
      if (ffree != NULL)
        ffree(np->p);
      Chunk_NodeFree(hp->chunk, np);
      np = next;
    }
    hp->table[i] = NULL;
  }
  hp->n = 0;
}

/* データの総数を返す */
Hash_Size Hash_Total(const Hash_Type *hp) {
  return (hp == NULL)? 0: hp->n;
}

/* buf[]に登録されたデータを全て並べる */
void Hash_ReadAll(const Hash_Type *hp, void **buf) {
  Hash_Size i, k;
  Hash_Node *np;

  if (hp == NULL || buf == NULL)
    return;
  k = 0;
  for (i = 0; i < hp->size_n; i++) {
    for (np = (Hash_Node *) hp->table[i]; np != NULL; np = np->next)
      buf[k++] = np->p;
  }
  assert(k == hp->n);
}

/* 整合性判定 */
static int Hash_Check(Hash_Type *hp) {
  Hash_Size i, n;
  Hash_Node *np;

  if (hp == NULL || hp->size_max <= 0 || hp->size_n <= 0
      || hp->size_max < hp->size_n || hp->table == NULL || hp->chunk == NULL)
    return 0;

  n = 0;
  for (i = 0; i < hp->size_n; i++) {
    for (np = (Hash_Node *) hp->table[i]; np != NULL; np = np->next)
      n++;
  }

  return 1;
}

