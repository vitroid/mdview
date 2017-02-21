/* "tavltree.h" ひも付きAVL木型 ヘッダ  Programed by Akinori Baba */

#ifndef _TAVLTREE_H
#define _TAVLTREE_H

typedef struct _TAVL_Node {
  struct _TAVL_Node *left,  /* 左側(値の小さい側)への枝 */
                    *right; /* 右側(値の大きい側)への枝 */
  char flags;               /* ひもであるかのフラグ */
  char nest;                /* ネストの深さ */
  const void *p;            /* データへのポインタ */
} TAVL_Node;

typedef TAVL_Node TAVL_Tree; /* ひも付きAVL木型(ノード型と一致) */

/* 木の確保 */
TAVL_Tree *TAVL_Alloc(void);

/* 木の解放 */
void TAVL_Free(TAVL_Tree *tp, void (*ffree)(void *));

/* データの挿入 */
const void *TAVL_Insert(TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *), void *(*fcpy)(const void *));

/* データの削除 */
int TAVL_Delete(TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *), void (*ffree)(void *));

/* データの検索 */
const void *TAVL_Search(const TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *));

/* ノードの取得 */
const TAVL_Node *TAVL_SearchNode(const TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *));
const TAVL_Node *TAVL_FirstNode(const TAVL_Tree *tp);
const TAVL_Node *TAVL_LastNode(const TAVL_Tree *tp);
const TAVL_Node *TAVL_PrevNode(const TAVL_Tree *tp, const TAVL_Node *np);
const TAVL_Node *TAVL_NextNode(const TAVL_Tree *tp, const TAVL_Node *np);

#endif
