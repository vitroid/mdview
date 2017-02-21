/* "chunk.c" 固定長単位局所ヒープ Ver. 0.60  Programed by BABA Akinori */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "chunk.h"

/* ---- 各種設定 ----------------------------------------------------------- */

typedef unsigned int Chunk_USize; /* Chunk_Sizeの符号なし版 */

#define CHUNK_ASSERT_ENABLE /* 整合性検査を有効にする */
#define CHUNK_CHECK_ERROR 1 /* 0:無視 1:stderrに出力してexit(1) 2:abort() */

/* ヒープ関連マクロ */
#define _Chunk_Malloc(n) malloc(n)
#define _Chunk_Free(p)   free(p)

/* ---- 内部用宣言 --------------------------------------------------------- */

/* ブロック型 */
typedef struct _Chunk_Block {
  Chunk_Type          *chp;    /* Chunk_Typeへのポインタ */
  struct _Chunk_Block *next;   /* 次のブロックへのポインタ */
  struct _Chunk_Block *prev;   /* 前のブロックへのポインタ */
  Chunk_Size           node_n; /* 登録しているノードの数 */
} Chunk_Block;

#define CHUNK_BLOCK_HEADERSIZE (1+(sizeof(Chunk_Block)-1)/sizeof(Chunk_Size))
#define Chunk_block_node_i(bp,i,size) \
  (((Chunk_Size *)bp)[CHUNK_BLOCK_HEADERSIZE+(i)*(1+(size))])
#define Chunk_block_node_p(bp,i,size) \
  (((Chunk_Size *)bp)+CHUNK_BLOCK_HEADERSIZE+(i)*(1+(size))+1)
#define Chunk_block_np_i(np)   ((np)[-1])

static Chunk_Block *Chunk_BlockAlloc(Chunk_Type *chp);
static void Chunk_BlockFree(Chunk_Block *bp);
static Chunk_Size Chunk_Block_Node_Alloc(Chunk_Type *chp, Chunk_Block *bp);
#define Chunk_BlockLimit(n) ((n)-(n)/5) /* 追加可能なノード数(現在8割) */

/* 乱数 */
static void Chunk_SRand(Chunk_Size seed);

/* エラー出力 */
static void Chunk_Error(const char *format, ...) {
#if CHUNK_CHECK_ERROR == 1
  va_list ap;

  fprintf(stderr, "Chunk: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
#elif CHUNK_CHECK_ERROR == 2
  abort();
#endif
}

#undef NDEBUG
#ifndef CHUNK_ASSERT_ENABLE
#define NDEBUG
#endif

/* ---- ランダムヒープ型 --------------------------------------------------- */

/* 識別用文字列 */
static const char *header_str = "Chunk_Type";

static int _Chunk_init = 0; /* ランダムヒープ型の初期化フラグ */

/* ヒープの確保 */
Chunk_Type *Chunk_TypeAlloc(Chunk_Size node_size,
    Chunk_Size block_size, Chunk_Size max_size) {
  Chunk_Type *chp;
  Chunk_Size node_max, block_max;

  if (!_Chunk_init) {
    Chunk_SRand(1);
    _Chunk_init = 1;
  }

  if (node_size <= 0 || block_size <= 0 || max_size < 0)
    return NULL;

  node_size = (node_size-1)/sizeof(Chunk_Size) + 1;
  block_size /= sizeof(Chunk_Size);
  if (max_size == 0)
    max_size = (~((Chunk_USize) 0))>>1;
  max_size /= sizeof(Chunk_Size);
  node_max = (block_size - CHUNK_BLOCK_HEADERSIZE)/(1+node_size);
  block_size = CHUNK_BLOCK_HEADERSIZE + node_max * (1+node_size);
  block_max = max_size/block_size;

  if (node_max <= 1 || block_max <= 0)
    return NULL;

  if ((chp = (Chunk_Type *) _Chunk_Malloc(sizeof(Chunk_Type))) == NULL)
    return NULL;
  chp->header_str = header_str;
  chp->node_size = node_size;
  chp->node_max = node_max;
  chp->block_size = block_size;
  chp->block_n = 1;
  chp->block_max = block_max;
  if ((chp->block_p = (void *) Chunk_BlockAlloc(chp)) == NULL) {
    _Chunk_Free(chp);
    return NULL;
  }
  chp->block_p_full = chp->block_p;

  return chp;
}

/* ヒープの解放 */
void Chunk_TypeFree(Chunk_Type *chp) {
  Chunk_Block *bp, *next;

  if (chp == NULL)
    return;

  assert(chp->header_str == header_str);

  /* ブロックの開放 */
  bp = (Chunk_Block *) chp->block_p;
  while ((next = bp->next) != bp) {
    assert(bp != NULL && bp->prev != NULL && bp->next != NULL
      && bp->prev->next == bp && bp->next->prev == bp);
    bp->prev->next = bp->next;
    bp->next->prev = bp->prev;
    Chunk_BlockFree(bp);
    bp = next;
  }
  assert(bp != NULL && bp->prev != NULL && bp->next != NULL
    && bp->prev->next == bp && bp->next->prev == bp);
  Chunk_BlockFree(bp);

  /* ヒープの開放 */
  chp->node_size = 0;
  chp->node_max = 0;
  chp->block_size = 0;
  chp->block_n = 0;
  chp->block_max = 0;
  chp->block_p = NULL;
  chp->block_p_full = NULL;
  _Chunk_Free(chp);
}

/* データ領域の確保 */
void *Chunk_NodeAlloc(Chunk_Type *chp) {
  Chunk_Block *bp, *obp;
  Chunk_Size *np;
  Chunk_Size i;

  if (chp == NULL)
    return NULL;

  assert(chp->header_str == header_str);

  bp = obp = (Chunk_Block *) chp->block_p;
  if (bp->node_n >= Chunk_BlockLimit(chp->node_max)) {
    /* 新しいノードを追加可能リストの先頭に追加 */
    if (chp->block_n >= chp->block_max || (bp = Chunk_BlockAlloc(chp)) == NULL)
      return NULL;
    assert(obp->chp == chp && obp->prev != NULL && obp->next != NULL
      && obp->prev->next == obp && obp->next->prev == obp);
    bp->prev = obp->prev;
    bp->next = obp;
    obp->prev->next = bp;
    obp->prev = bp;
    chp->block_p = (void *) bp;
    chp->block_n++;
  }

  /* bpに登録 */
  i = Chunk_Block_Node_Alloc(chp, bp);
  assert(i >= 0);
  np = Chunk_block_node_p(bp, i, chp->node_size);
  Chunk_block_node_i(bp, i, chp->node_size) = np - (Chunk_Size *) bp;
  bp->node_n++;

  if (bp->node_n >= Chunk_BlockLimit(chp->node_max)) {
    /* 追加不能リストの最後に移動 */
    assert(bp->chp == chp && bp->prev != NULL && bp->next != NULL
      && bp->prev->next == bp && bp->next->prev == bp);
    chp->block_p = (void *) (bp->next);
  }

  return (void *) np;
}

/* データ領域の解放 */
void Chunk_NodeFree(Chunk_Type *chp, void *p) {
  Chunk_Block *bp;
  Chunk_Size *np;
  Chunk_Size i;

  if (chp == NULL || p == NULL)
    return;

  assert(chp->header_str == header_str);

  /* ブロックへのポインタを求める */
  np = (Chunk_Size *) p;
  i = Chunk_block_np_i(np);
#ifndef CHUNK_ASSERT_ENABLE
  bp = (Chunk_Block *) (np - i);
#else
  if (i < 0 || i >= chp->block_size
      || (bp = (Chunk_Block *) (np - i))->chp != chp) {
    Chunk_Error("Chunk_NodeFree: Junk pointer detected.");
    return;
  }
#endif /* CHUNK_ASSERT_ENABLE */

  /* 削除 */
  assert(bp->node_n > 0);
  if (bp->node_n >= Chunk_BlockLimit(chp->node_max)) {
    /* 追加可能リストの最後に移動 */
    assert(bp->chp == chp && bp->prev != NULL && bp->next != NULL
      && bp->prev->next == bp && bp->next->prev == bp);
    if (bp == (Chunk_Block *) chp->block_p_full) {
      chp->block_p_full = (void *) (bp->next);
    } else {
      Chunk_Block *obp = (Chunk_Block *) chp->block_p_full;

      assert(obp->chp == chp && obp->prev != NULL && obp->next != NULL
        && obp->prev->next == obp && obp->next->prev == obp);
      bp->prev->next = bp->next;
      bp->next->prev = bp->prev;
      bp->prev = obp->prev;
      bp->next = obp;
      obp->prev->next = bp;
      obp->prev = bp;
    }
  }
  Chunk_block_np_i(np) = -1;
  bp->node_n--;
}

/* ---- 整数乱数 ----------------------------------------------------------- */

/* M系列乱数 */
extern int mrnd_i;
extern unsigned long mrnd_buf[];
extern void mrnd_init(unsigned long seed);
extern void mrnd_update();
#define mrnd() (((mrnd_i>=521)?mrnd_update():(void)0),mrnd_buf[mrnd_i++])

/* 整数乱数の初期化 */
#define CHUNK_SIZEMASK (~((Chunk_USize) 0)>>1)
static void Chunk_SRand(Chunk_Size seed) {
  int i;

  mrnd_init(seed);
  for (i = 0; i < 521; i++)
    mrnd_buf[i] &= CHUNK_SIZEMASK;
}

/* 0以上n未満の整数乱数 */
#define Chunk_Rand(n) (((Chunk_Size)mrnd())%(n))

/* ---- ブロック型 --------------------------------------------------------- */

/* ブロックの確保 */
static Chunk_Block *Chunk_BlockAlloc(Chunk_Type *chp) {
  Chunk_Block *bp;
  Chunk_Size i;

  if ((bp = (Chunk_Block *)
      _Chunk_Malloc(sizeof(Chunk_Size)*chp->block_size)) == NULL)
    return NULL;
  bp->chp = chp;
  bp->next = bp;
  bp->prev = bp;
  bp->node_n = 0;
  for (i = 0; i < chp->node_max; i++)
    Chunk_block_node_i(bp, i, chp->node_size) = -1; /* 負は無効な値 */

  return bp;
}

/* ブロックの開放 */
static void Chunk_BlockFree(Chunk_Block *bp) {
  bp->chp = NULL;
  bp->next = NULL;
  bp->prev = NULL;
  bp->node_n = 0;
  _Chunk_Free(bp);
}

/* 最大公約数 */
static Chunk_Size Chunk_GCD(Chunk_Size x, Chunk_Size y) {
  Chunk_Size t;

  while (y != 0) {
    t = x % y;
    x = y;
    y = t;
  }
  return x;
}

/* ブロックbp内のデータ領域の確保(返り値はノード番号) */
#define CHUNK_NTRY 10
static Chunk_Size Chunk_Block_Node_Alloc(Chunk_Type *chp, Chunk_Block *bp) {
  Chunk_Size i, n = 0, dn;

  for (i = 0; i < CHUNK_NTRY; i++) {
    n = Chunk_Rand(chp->node_max);
    if (Chunk_block_node_i(bp, n, chp->node_size) < 0)
      break;
  }
  if (i >= CHUNK_NTRY) {
    do {
      dn = Chunk_Rand(chp->node_max);
    } while (Chunk_GCD(chp->node_max, dn) != 1);
    if (dn <= 0)
      dn = 1;

    for (i = 1; i < chp->node_max; i++) {
      n += dn;
      n %= chp->node_max;
      if (Chunk_block_node_i(bp, n, chp->node_size) < 0)
        break;
    }
    if (i >= chp->node_max)
      return -1;
  }

  return n;
}

/* ---- M系列乱数 ---------------------------------------------------------- */

int mrnd_i = 0;
unsigned long mrnd_buf[521] = {0};

void mrnd_update() {
  int i;

  for (i = 0; i < 32; i++)
    mrnd_buf[i] ^= mrnd_buf[i + 489];
  for (i = 32; i < 521; i++)
    mrnd_buf[i] ^= mrnd_buf[i - 32];
  mrnd_i = 0;
}

#define MRND_MASK32 0xFFFFFFFFUL
void mrnd_init(unsigned long seed) {
  int i, j;
  unsigned long u;

  u = 0;
  for (i = 0; i <= 16; i++) {
    for (j = 0; j < 32; j++) {
      seed = seed * 1566083941UL + 1;
      u = (u >> 1) | (seed & (1UL << 31));
    }
    mrnd_buf[i] = u;
  }
  mrnd_buf[16] = (MRND_MASK32&(mrnd_buf[16]<<23))
    ^ (mrnd_buf[0]>>9) ^ mrnd_buf[15];
  for (i = 17; i <= 520; i++) {
    mrnd_buf[i] = (MRND_MASK32&(mrnd_buf[i-17]<<23))
      ^ (mrnd_buf[i-16]>>9) ^ mrnd_buf[i-1];
  }
  mrnd_update();
}

