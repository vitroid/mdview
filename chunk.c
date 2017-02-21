/* "chunk.c" ����Ĺñ�̶ɽ�ҡ��� Ver. 0.60  Programed by BABA Akinori */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "chunk.h"

/* ---- �Ƽ����� ----------------------------------------------------------- */

typedef unsigned int Chunk_USize; /* Chunk_Size�����ʤ��� */

#define CHUNK_ASSERT_ENABLE /* ������������ͭ���ˤ��� */
#define CHUNK_CHECK_ERROR 1 /* 0:̵�� 1:stderr�˽��Ϥ���exit(1) 2:abort() */

/* �ҡ��״�Ϣ�ޥ��� */
#define _Chunk_Malloc(n) malloc(n)
#define _Chunk_Free(p)   free(p)

/* ---- ��������� --------------------------------------------------------- */

/* �֥�å��� */
typedef struct _Chunk_Block {
  Chunk_Type          *chp;    /* Chunk_Type�ؤΥݥ��� */
  struct _Chunk_Block *next;   /* ���Υ֥�å��ؤΥݥ��� */
  struct _Chunk_Block *prev;   /* ���Υ֥�å��ؤΥݥ��� */
  Chunk_Size           node_n; /* ��Ͽ���Ƥ���Ρ��ɤο� */
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
#define Chunk_BlockLimit(n) ((n)-(n)/5) /* �ɲò�ǽ�ʥΡ��ɿ�(����8��) */

/* ��� */
static void Chunk_SRand(Chunk_Size seed);

/* ���顼���� */
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

/* ---- ������ҡ��׷� --------------------------------------------------- */

/* ������ʸ���� */
static const char *header_str = "Chunk_Type";

static int _Chunk_init = 0; /* ������ҡ��׷��ν�����ե饰 */

/* �ҡ��פγ��� */
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

/* �ҡ��פβ��� */
void Chunk_TypeFree(Chunk_Type *chp) {
  Chunk_Block *bp, *next;

  if (chp == NULL)
    return;

  assert(chp->header_str == header_str);

  /* �֥�å��γ��� */
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

  /* �ҡ��פγ��� */
  chp->node_size = 0;
  chp->node_max = 0;
  chp->block_size = 0;
  chp->block_n = 0;
  chp->block_max = 0;
  chp->block_p = NULL;
  chp->block_p_full = NULL;
  _Chunk_Free(chp);
}

/* �ǡ����ΰ�γ��� */
void *Chunk_NodeAlloc(Chunk_Type *chp) {
  Chunk_Block *bp, *obp;
  Chunk_Size *np;
  Chunk_Size i;

  if (chp == NULL)
    return NULL;

  assert(chp->header_str == header_str);

  bp = obp = (Chunk_Block *) chp->block_p;
  if (bp->node_n >= Chunk_BlockLimit(chp->node_max)) {
    /* �������Ρ��ɤ��ɲò�ǽ�ꥹ�Ȥ���Ƭ���ɲ� */
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

  /* bp����Ͽ */
  i = Chunk_Block_Node_Alloc(chp, bp);
  assert(i >= 0);
  np = Chunk_block_node_p(bp, i, chp->node_size);
  Chunk_block_node_i(bp, i, chp->node_size) = np - (Chunk_Size *) bp;
  bp->node_n++;

  if (bp->node_n >= Chunk_BlockLimit(chp->node_max)) {
    /* �ɲ���ǽ�ꥹ�ȤκǸ�˰�ư */
    assert(bp->chp == chp && bp->prev != NULL && bp->next != NULL
      && bp->prev->next == bp && bp->next->prev == bp);
    chp->block_p = (void *) (bp->next);
  }

  return (void *) np;
}

/* �ǡ����ΰ�β��� */
void Chunk_NodeFree(Chunk_Type *chp, void *p) {
  Chunk_Block *bp;
  Chunk_Size *np;
  Chunk_Size i;

  if (chp == NULL || p == NULL)
    return;

  assert(chp->header_str == header_str);

  /* �֥�å��ؤΥݥ��󥿤���� */
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

  /* ��� */
  assert(bp->node_n > 0);
  if (bp->node_n >= Chunk_BlockLimit(chp->node_max)) {
    /* �ɲò�ǽ�ꥹ�ȤκǸ�˰�ư */
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

/* ---- ������� ----------------------------------------------------------- */

/* M������� */
extern int mrnd_i;
extern unsigned long mrnd_buf[];
extern void mrnd_init(unsigned long seed);
extern void mrnd_update();
#define mrnd() (((mrnd_i>=521)?mrnd_update():(void)0),mrnd_buf[mrnd_i++])

/* ��������ν���� */
#define CHUNK_SIZEMASK (~((Chunk_USize) 0)>>1)
static void Chunk_SRand(Chunk_Size seed) {
  int i;

  mrnd_init(seed);
  for (i = 0; i < 521; i++)
    mrnd_buf[i] &= CHUNK_SIZEMASK;
}

/* 0�ʾ�n̤����������� */
#define Chunk_Rand(n) (((Chunk_Size)mrnd())%(n))

/* ---- �֥�å��� --------------------------------------------------------- */

/* �֥�å��γ��� */
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
    Chunk_block_node_i(bp, i, chp->node_size) = -1; /* ���̵������ */

  return bp;
}

/* �֥�å��γ��� */
static void Chunk_BlockFree(Chunk_Block *bp) {
  bp->chp = NULL;
  bp->next = NULL;
  bp->prev = NULL;
  bp->node_n = 0;
  _Chunk_Free(bp);
}

/* �������� */
static Chunk_Size Chunk_GCD(Chunk_Size x, Chunk_Size y) {
  Chunk_Size t;

  while (y != 0) {
    t = x % y;
    x = y;
    y = t;
  }
  return x;
}

/* �֥�å�bp��Υǡ����ΰ�γ���(�֤��ͤϥΡ����ֹ�) */
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

/* ---- M������� ---------------------------------------------------------- */

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

