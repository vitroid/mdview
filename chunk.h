/* "chunk.h" ����Ĺñ�̶ɽ�ҡ��� �إå�  Programed by BABA Akinori */

#ifndef _CHUNK_H
#define _CHUNK_H

typedef int Chunk_Size; /* ��������դ��������󥢥饤����ñ�� */

typedef struct {
  const char *header_str; /* �����ѥإå�ʸ���� */
  Chunk_Size node_size;   /* �Ρ��ɤΥ�����(���饤����ñ��) */
  Chunk_Size node_max;    /* �֥�å�������Ρ��ɿ� */
  Chunk_Size block_size;  /* �֥�å��Υ�����(���饤����ñ��) */
  Chunk_Size block_n;     /* �֥�å��� */
  Chunk_Size block_max;   /* �֥�å����ξ�� */
  void *block_p;          /* �ɲò�ǽ�ʥ֥�å��ؤΥݥ��� */
  void *block_p_full;     /* �ɲ���ǽ�ʥ֥�å��ؤΥݥ��� */
} Chunk_Type;

Chunk_Type *Chunk_TypeAlloc(Chunk_Size node_size,
  Chunk_Size block_size, Chunk_Size max_size);
void Chunk_TypeFree(Chunk_Type *chp);
void *Chunk_NodeAlloc(Chunk_Type *chp);
void Chunk_NodeFree(Chunk_Type *chp, void *p);
void Chunk_NodeFreeAll(Chunk_Type *chp);

#endif /* _CHUNK_H */

