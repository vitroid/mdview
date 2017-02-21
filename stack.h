/* ����Ĺ�����å� �إå�  */

#ifndef _STACK_H_
#define _STACK_H_

typedef int Stack_Size; /* ��������դ����� */

typedef struct {
  Stack_Size next_id; /* ���ΥΡ��ɤؤΥݥ��� */
  Stack_Size level;   /* ������(2���߾�) */
  void *p;            /* ���ݤ����ݥ��� */
} Stack_Node;

#define STACK_MINLEVEL 8  /* ���ݤ���Ǿ�������(2���߾�ξ��) */
#define STACK_MAXLEVEL 32 /*         ����                      */

typedef struct {
  const char *header_str; /* �����ѥإå�ʸ���� */
  Stack_Size active_n;    /* ������Ρ��ɿ� */
  Stack_Size spool_n;     /* ��¢�Ρ��ɿ� */
  Stack_Size depth_max;   /* (���ߤޤǤ�)���糬�ؿ� */
  Stack_Size active_id;   /* ������ꥹ�Ȥ���Ƭ�Ρ���ID */
  Stack_Size spool_id[STACK_MAXLEVEL-STACK_MINLEVEL+1];
    /* ������(2���߾�ξ��) -> ��¢�ꥹ�Ȥ���Ƭ�Ρ���ID */
  Stack_Size node_max;    /* node[]�Υ����� */
  Stack_Node *node;       /* �Ρ���ID -> �Ρ���(����Ĺ����) */
} Stack_Type;

Stack_Type *Stack_TypeAlloc(void);
void Stack_TypeFree(Stack_Type *stp);
void *Stack_NodeAlloc(Stack_Type *stp, Stack_Size n);
void Stack_NodeFree(Stack_Type *stp, void *p);

#endif /* _STACK_H_ */
