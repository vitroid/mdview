/* stack.c: ����Ĺ�����å� v0.30 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "stack.h"

/* ---- ����ե�����졼����� --------------------------------------------- */

#define STACK_TEST_ARGUMENT

#define _Stack_Malloc(n)    malloc(n)
#define _Stack_Realloc(p,n) realloc((p),(n))
#define _Stack_Free(p)      free(p)

/* ---- Stack_Type�� ------------------------------------------------------- */

const char *_stack_str = "Stack_Type";

#define STACK_MINNODE 32
#define STACK_NIL     (-1)

/* Stack_Type���γ���(���Ԥ�NULL���֤�) */
Stack_Type *Stack_TypeAlloc(void) {
  Stack_Type *stp;
  int i;

  if ((stp = (Stack_Type *) _Stack_Malloc(sizeof(Stack_Type))) == NULL)
    return NULL;
  stp->header_str = _stack_str;
  stp->active_n = 0;
  stp->spool_n = 0;
  stp->depth_max = 0;
  stp->active_id = STACK_NIL;
  for (i = 0; i < STACK_MAXLEVEL-STACK_MINLEVEL+1; i++)
    stp->spool_id[i] = STACK_NIL;
  stp->node_max = STACK_MINNODE;
  if ((stp->node = (Stack_Node *)
      _Stack_Malloc(sizeof(Stack_Node) * stp->node_max)) == NULL) {
    _Stack_Free(stp);
    return NULL;
  }

  return stp;
}

/* Stack_Type���γ��� */
void Stack_TypeFree(Stack_Type *stp) {
  if (stp == NULL)
    return;
#ifdef STACK_TEST_ARGUMENT
  assert(stp->header_str == _stack_str);
  stp->header_str = NULL;
#endif

  _Stack_Free(stp->node);
  _Stack_Free(stp);
}

/* ���������б������٥���֤� */
static int _Stack_Level(Stack_Size n) {
  int ret = 1;

  n--;
  if (n < 0) n = 0;
  if (n >= 1L<<16) {
    ret += 16;
    n >>= 16;
  }
  if (n >= 1L<<8) {
    ret += 8;
    n >>= 8;
  }
  if (n >= 1L<<4) {
    ret += 4;
    n >>= 4;
  }
  if (n >= 1L<<2) {
    ret += 2;
    n >>= 2;
  }
  if (n >= 1L<<1)
    ret++;
  if (ret < STACK_MINLEVEL)
    ret = STACK_MINLEVEL;
  return ret;
}

/* �Ρ��ɤγ���(���Ԥ�NULL���֤�) */
void *Stack_NodeAlloc(Stack_Type *stp, Stack_Size n) {
  int level;
  int i, id;

  if (stp == NULL || n < 0)
    return NULL;
#ifdef STACK_TEST_ARGUMENT
  assert(stp->header_str == _stack_str);
#endif

  /* depth_max�ι��� */
  if (stp->active_n+1 > stp->depth_max)
    stp->depth_max = stp->active_n+1;

  /* �Ρ��ɤγ��� */
  level = _Stack_Level(n);
  if ((id = stp->spool_id[level-STACK_MINLEVEL]) != STACK_NIL) {
    /* ��Ŭ�ʥ������ΥΡ��� */
    stp->spool_id[level-STACK_MINLEVEL] = stp->node[id].next_id;
    stp->spool_n--;
  } else if (stp->spool_n + stp->active_n < 2*stp->depth_max) {
    /* �������� */
    id = stp->spool_n + stp->active_n;
    if (id >= stp->node_max) {
      Stack_Node *p;

      /* �Ρ����ΰ�γ��� */
      if ((p = _Stack_Realloc(stp->node, stp->node_max*2)) == NULL)
        return NULL;
      stp->node_max *= 2;
      stp->node = p;
    }
    if ((stp->node[id].p = _Stack_Malloc(1L<<level)) == NULL)
      return NULL;
    stp->node[id].level = level;
  } else {
    for (i = level-STACK_MINLEVEL+1; i < STACK_MAXLEVEL-STACK_MINLEVEL+1
        && stp->spool_id[i] == STACK_NIL; i++)
      ;
    if (i < STACK_MAXLEVEL-STACK_MINLEVEL+1) {
      /* �����Υ������ΥΡ��� */
      id = stp->spool_id[i];
      stp->spool_id[i] = stp->node[id].next_id;
      stp->spool_n--;
    } else {
      void *p;

      /* �����ͤ��Ŭ�ʥ������˼��ľ�� */
      for (i = STACK_MAXLEVEL-STACK_MINLEVEL; i >= 0
          && stp->spool_id[i] == STACK_NIL; i--)
        ;
      assert(i >= 0);
      id = stp->spool_id[i];
      if ((p = _Stack_Malloc(1L<<level)) == NULL)
        return NULL;
      _Stack_Free(stp->node[id].p);
      stp->node[id].p = p;
      stp->node[id].level = level;
      stp->spool_id[i] = stp->node[id].next_id;
      stp->spool_n--;
    }
  }
  stp->node[id].next_id = stp->active_id;
  stp->active_id = id;
  stp->active_n++;

  return stp->node[id].p;
}

/* �Ρ��ɤγ��� */
void Stack_NodeFree(Stack_Type *stp, void *p) {
  int i, id;

  if (stp == NULL)
    return;
#ifdef STACK_TEST_ARGUMENT
  assert(stp->header_str == _stack_str);
#endif
  if (p == NULL)
    return;

  assert(stp->active_n > 0);
  id = stp->active_id;
  stp->active_id = stp->node[id].next_id;
  stp->active_n--;
#ifdef STACK_TEST_ARGUMENT
  assert(stp->node[id].p == p);
#endif

  i = stp->node[id].level - STACK_MINLEVEL;
  stp->node[id].next_id = stp->spool_id[i];
  stp->spool_id[i] = id;
  stp->spool_n++;
}

