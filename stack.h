/* 可変長スタック ヘッダ  */

#ifndef _STACK_H_
#define _STACK_H_

typedef int Stack_Size; /* 汎用符号付き整数 */

typedef struct {
  Stack_Size next_id; /* 次のノードへのポインタ */
  Stack_Size level;   /* サイズ(2の累乗) */
  void *p;            /* 確保したポインタ */
} Stack_Node;

#define STACK_MINLEVEL 8  /* 確保する最小サイズ(2の累乗の乗数) */
#define STACK_MAXLEVEL 32 /*         最大                      */

typedef struct {
  const char *header_str; /* 識別用ヘッダ文字列 */
  Stack_Size active_n;    /* 使用中ノード数 */
  Stack_Size spool_n;     /* 貯蔵ノード数 */
  Stack_Size depth_max;   /* (現在までの)最大階層数 */
  Stack_Size active_id;   /* 使用中リストの先頭ノードID */
  Stack_Size spool_id[STACK_MAXLEVEL-STACK_MINLEVEL+1];
    /* サイズ(2の累乗の乗数) -> 貯蔵リストの先頭ノードID */
  Stack_Size node_max;    /* node[]のサイズ */
  Stack_Node *node;       /* ノードID -> ノード(可変長配列) */
} Stack_Type;

Stack_Type *Stack_TypeAlloc(void);
void Stack_TypeFree(Stack_Type *stp);
void *Stack_NodeAlloc(Stack_Type *stp, Stack_Size n);
void Stack_NodeFree(Stack_Type *stp, void *p);

#endif /* _STACK_H_ */
