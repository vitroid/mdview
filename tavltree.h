/* "tavltree.h" �Ҥ��դ�AVL�ڷ� �إå�  Programed by Akinori Baba */

#ifndef _TAVLTREE_H
#define _TAVLTREE_H

typedef struct _TAVL_Node {
  struct _TAVL_Node *left,  /* ��¦(�ͤξ�����¦)�ؤλ� */
                    *right; /* ��¦(�ͤ��礭��¦)�ؤλ� */
  char flags;               /* �Ҥ�Ǥ��뤫�Υե饰 */
  char nest;                /* �ͥ��Ȥο��� */
  const void *p;            /* �ǡ����ؤΥݥ��� */
} TAVL_Node;

typedef TAVL_Node TAVL_Tree; /* �Ҥ��դ�AVL�ڷ�(�Ρ��ɷ��Ȱ���) */

/* �ڤγ��� */
TAVL_Tree *TAVL_Alloc(void);

/* �ڤβ��� */
void TAVL_Free(TAVL_Tree *tp, void (*ffree)(void *));

/* �ǡ��������� */
const void *TAVL_Insert(TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *), void *(*fcpy)(const void *));

/* �ǡ����κ�� */
int TAVL_Delete(TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *), void (*ffree)(void *));

/* �ǡ����θ��� */
const void *TAVL_Search(const TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *));

/* �Ρ��ɤμ��� */
const TAVL_Node *TAVL_SearchNode(const TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *));
const TAVL_Node *TAVL_FirstNode(const TAVL_Tree *tp);
const TAVL_Node *TAVL_LastNode(const TAVL_Tree *tp);
const TAVL_Node *TAVL_PrevNode(const TAVL_Tree *tp, const TAVL_Node *np);
const TAVL_Node *TAVL_NextNode(const TAVL_Tree *tp, const TAVL_Node *np);

#endif
