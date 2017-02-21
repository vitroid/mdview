/* "chunk.h" 固定長単位局所ヒープ ヘッダ  Programed by BABA Akinori */

#ifndef _CHUNK_H
#define _CHUNK_H

typedef int Chunk_Size; /* 汎用符号付き整数、兼アライメント単位 */

typedef struct {
  const char *header_str; /* 識別用ヘッダ文字列 */
  Chunk_Size node_size;   /* ノードのサイズ(アライメント単位) */
  Chunk_Size node_max;    /* ブロック当たりノード数 */
  Chunk_Size block_size;  /* ブロックのサイズ(アライメント単位) */
  Chunk_Size block_n;     /* ブロック数 */
  Chunk_Size block_max;   /* ブロック数の上限 */
  void *block_p;          /* 追加可能なブロックへのポインタ */
  void *block_p_full;     /* 追加不能なブロックへのポインタ */
} Chunk_Type;

Chunk_Type *Chunk_TypeAlloc(Chunk_Size node_size,
  Chunk_Size block_size, Chunk_Size max_size);
void Chunk_TypeFree(Chunk_Type *chp);
void *Chunk_NodeAlloc(Chunk_Type *chp);
void Chunk_NodeFree(Chunk_Type *chp, void *p);
void Chunk_NodeFreeAll(Chunk_Type *chp);

#endif /* _CHUNK_H */

