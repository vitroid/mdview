/* "tavltree.c" �Ҥ��դ�AVL�ڷ� Ver. 0.61  Programed by Akinori Baba */

#include<stdio.h>
#include<stdlib.h>
#include"tavltree.h"

/* ---- ���� --------------------------------------------------------------- */

#define TAVL_ASSERT_ENABLE /* ������Ƚ�굡ǽ��ͭ���ˤ��� */
#define TAVL_CHUNK_ENABLE  /* �ڤ��Ȥ˥����������Ω�˹Ԥ� */

/* ---- ��� --------------------------------------------------------------- */

/* TAVL_Node�� */
#define TAVL_LFLAG 0x01 /* �����ҤؤΥݥ��󥿤ʤ�ON */
#define TAVL_RFLAG 0x02 /* �����ҤؤΥݥ��󥿤ʤ�ON */
#define TAVL_MAXNEST 48 /* �ݥ��󥿷��Υӥåȿ�*1.5�ǽ�ʬ */
#define TAVL_OPTDEPTH 2

static TAVL_Node *TAVL_NodeAlloc(TAVL_Tree *tp);
static void TAVL_NodeFree(TAVL_Tree *tp, TAVL_Node *np);
static TAVL_Node *TAVL_LeafFree(TAVL_Tree *tp, TAVL_Node *np);
static const TAVL_Node *_TAVL_PrevNode(const TAVL_Node *np);
static const TAVL_Node *_TAVL_NextNode(const TAVL_Node *np);
static int TAVL_SetNest(TAVL_Node *np);
static void TAVL_OptLeft(TAVL_Node **npp);
static void TAVL_OptRight(TAVL_Node **npp);
static int TAVL_IsNormalNest(const TAVL_Node *np);

/* �ҡ��״�Ϣ */
#ifdef TAVL_CHUNK_ENABLE
/* Chunk_Type������ */
#include "chunk.h"
#define _TAVL_InitMalloc(tp) \
  (((tp)->p = (void *) Chunk_TypeAlloc(sizeof(TAVL_Node),1024,0)) != NULL)
#define _TAVL_TermMalloc(tp) Chunk_TypeFree((Chunk_Type *)((tp)->p))
#define _TAVL_NodeAlloc(tp) \
  (((tp)==NULL)? malloc(sizeof(TAVL_Node)): \
      Chunk_NodeAlloc((Chunk_Type *)((tp)->p)))
#define _TAVL_NodeFree(tp,np) \
  (((tp)==NULL)? free(np): Chunk_NodeFree((Chunk_Type *)((tp)->p),(np)))
#else
/* �̾��malloc()/free()������ */
#define _TAVL_InitMalloc(tp)  1
#define _TAVL_TermMalloc(tp)  ((void)0)
#define _TAVL_NodeAlloc(tp)   malloc(sizeof(TAVL_Node))
#define _TAVL_NodeFree(tp,np) free(np)
#endif /* TAVL_CHUNK_ENABLE */

/* ���顼�����å� */
static void TAVL_FuncBegin(char *str);
static void TAVL_FuncEnd(void);
static void TAVL_Error(char *str);

static void _TAVL_Assert(int x) {
  if (!x)
    TAVL_Error("Assertion failed.");
}

#ifdef TAVL_ASSERT_ENABLE
#define TAVL_Assert(x) _TAVL_Assert(x)
#else
#define TAVL_Assert(x) ((void) 0)
#endif

/* ---- public�ؿ���� ----------------------------------------------------- */

/* �ڤγ��� */
TAVL_Tree *TAVL_Alloc(void) {
  TAVL_Tree *tp;

  if ((tp = (TAVL_Tree *) TAVL_NodeAlloc(NULL)) == NULL)
    return NULL;
  tp->left = tp;
  tp->right = tp;
  tp->nest = 1;
  if (!_TAVL_InitMalloc(tp)) {
    tp->left = NULL;
    tp->right = NULL;
    tp->p = NULL;
    TAVL_NodeFree(NULL, tp);
    return NULL;
  }

  return tp;
}

/* �ڤβ���(ffree()�ϥǡ����β����ؿ���NULL�ʤ�������ʤ�) */
void TAVL_Free(TAVL_Tree *tp, void (*ffree)(void *)) {
  TAVL_Node *np;

  if (tp == NULL)
    return;
  TAVL_FuncBegin("TAVL_Free");

  /* �Ρ��ɤβ��� */
#ifdef TAVL_CHUNK_ENABLE
  if (ffree != NULL) {
#endif
  np = tp;
  for (;;) {
    /* �դ�õ�� */
    for (;;) {
      if (np->flags & TAVL_LFLAG)
        np = np->left;
      else if (np->flags & TAVL_RFLAG)
        np = np->right;
      else
        break;
      TAVL_Assert(TAVL_IsNormalNest(np));
    }
    /* ����Ǥ⺬�ʤ鴰λ */
    if (np == tp)
      break;

    /* �եΡ��ɤ������ƣ��ĿƤ���� */
    TAVL_Assert(np->p != NULL);
    if (ffree != NULL)
      ffree((void *) np->p);
    np->p = NULL;
    np = TAVL_LeafFree(tp, np);
  }
  if (tp->left != tp || tp->right != tp)
    TAVL_Error("Illegal node found.");
#ifdef TAVL_CHUNK_ENABLE
  }
#endif

  /* ���β��� */
  _TAVL_TermMalloc(tp);
  tp->left = NULL;
  tp->right = NULL;
  tp->flags = 0;
  tp->p = NULL;
  TAVL_NodeFree(NULL, tp);

  TAVL_FuncEnd();
}

/* �ǽ�ΥΡ��ɤμ���(���Ԥ�NULL) */
const TAVL_Node *TAVL_FirstNode(const TAVL_Tree *tp) {
  const TAVL_Node *np;

  if (tp == NULL || !(tp->flags & TAVL_RFLAG))
    return NULL;
  TAVL_FuncBegin("TAVL_FirstNode");

  np = tp->right;
  while (np->flags & TAVL_LFLAG) {
    TAVL_Assert(np != NULL);
    np = np->left;
  }
  if (np->left != tp)
    TAVL_Error("Illegal node found.");

  TAVL_FuncEnd();
  return np;
}

/* �Ǹ�ΥΡ��ɤμ���(���Ԥ�NULL) */
const TAVL_Node *TAVL_LastNode(const TAVL_Tree *tp) {
  const TAVL_Node *np;

  if (tp == NULL || !(tp->flags & TAVL_RFLAG))
    return NULL;
  TAVL_FuncBegin("TAVL_LastNode");

  np = tp->right;
  while (np->flags & TAVL_RFLAG) {
    TAVL_Assert(np != NULL);
    np = np->right;
  }
  if (np->right != tp)
    TAVL_Error("Illegal node found.");

  TAVL_FuncEnd();
  return np;
}

/* ľ���ΥΡ��ɤμ��� */
const TAVL_Node *TAVL_PrevNode(const TAVL_Tree *tp, const TAVL_Node *np) {
  const TAVL_Node *ptmp;

  if (tp == NULL || np == NULL)
    return NULL;
  TAVL_FuncBegin("TAVL_PrevNode");
  if ((ptmp = _TAVL_PrevNode(np)) == NULL)
    TAVL_Error("Illegal node found.");
  TAVL_Assert(ptmp == tp || _TAVL_NextNode(ptmp) == np);

  TAVL_FuncEnd();
  return (ptmp != tp)? ptmp: NULL;
}

/* ľ��ΥΡ��ɤμ��� */
const TAVL_Node *TAVL_NextNode(const TAVL_Tree *tp, const TAVL_Node *np) {
  const TAVL_Node *ptmp;

  if (tp == NULL || np == NULL)
    return NULL;
  TAVL_FuncBegin("TAVL_NextNode");
  if ((ptmp = _TAVL_NextNode(np)) == NULL)
    TAVL_Error("Illegal node found.");
  TAVL_Assert(ptmp == tp || _TAVL_PrevNode(ptmp) == np);

  TAVL_FuncEnd();
  return (ptmp != tp)? ptmp: NULL;
}

/* ����(fcmp()����Ӵؿ���fcpy()�ϥ��ԡ��ؿ��ǡ�NULL�ʤ鲿�⤷�ʤ�) */
const void *TAVL_Insert(TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *), void *(*fcpy)(const void *)) {
  TAVL_Node **pbuf[TAVL_MAXNEST];
  int level;
  TAVL_Node *np, *ptmp;
  int tmp;

  if (tp == NULL || p == NULL || fcmp == NULL)
    return NULL;
  TAVL_FuncBegin("TAVL_Insert");

  np = tp;
  tmp = 1; /* �ǽ�λҤϱ�¦ */
  level = 0;
  for (;;) {
    if (tmp < 0) {
      if (np->flags & TAVL_LFLAG) {
        TAVL_Assert(level < TAVL_MAXNEST);
        pbuf[level++] = &(np->left);
        np = np->left;
      } else {
        /* ��¦����Ͽ */
        if ((ptmp = TAVL_NodeAlloc(tp)) == NULL) {
          TAVL_FuncEnd();
          return NULL;
        }
        if (fcpy != NULL) {
          if ((p = fcpy(p)) == NULL) {
            TAVL_FuncEnd();
            return NULL;
          }
        }
        ptmp->p = p;
        ptmp->left = np->left;
        ptmp->right = np;
        ptmp->nest = 1;
        np->flags |= TAVL_LFLAG;
        np->left = ptmp;
        break;
      }
    } else if (tmp > 0) {
      if (np->flags & TAVL_RFLAG) {
        TAVL_Assert(level < TAVL_MAXNEST);
        pbuf[level++] = &(np->right);
        np = np->right;
      } else {
        /* ��¦����Ͽ */
        if ((ptmp = TAVL_NodeAlloc(tp)) == NULL) {
          TAVL_FuncEnd();
          return NULL;
        }
        if (fcpy != NULL) {
          if ((p = fcpy(p)) == NULL) {
            TAVL_FuncEnd();
            return NULL;
          }
        }
        ptmp->p = p;
        ptmp->right = np->right;
        ptmp->left = np;
        ptmp->nest = 1;
        np->flags |= TAVL_RFLAG;
        np->right = ptmp;
        break;
      }
    } else {
      /* �Ρ��ɤ����Ǥ�¸�� */
      TAVL_FuncEnd();
      return NULL;
    }
    TAVL_Assert(TAVL_IsNormalNest(np));
    tmp = fcmp(p, np->p);
  }

  /* �ͥ��ȿ��ι����Ⱥ�Ŭ�� */
  while (--level >= 0) {
    if ((tmp = TAVL_SetNest(*pbuf[level])) >= TAVL_OPTDEPTH)
      TAVL_OptLeft(pbuf[level]);
    else if (tmp <= -TAVL_OPTDEPTH)
      TAVL_OptRight(pbuf[level]);
  }

  TAVL_FuncEnd();
  return p;
}

/* ���(fcmp()����Ӵؿ���ffree()�ϲ����ؿ��ǡ�NULL�ʤ鲿�⤷�ʤ�) */
int TAVL_Delete(TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *), void (*ffree)(void *)) {
  TAVL_Node **pbuf[TAVL_MAXNEST];
  int level;
  TAVL_Node *np, *ptmp;
  int tmp;

  if (tp == NULL || p == NULL || fcmp == NULL)
    return 0;
  TAVL_FuncBegin("TAVL_Delete");

  /* ������٤��Ρ��ɤθ��� */
  np = tp;
  tmp = 1; /* �ǽ�λҤϱ�¦ */
  level = 0;
  for (;;) {
    if (tmp < 0) {
      if (np->flags & TAVL_LFLAG) {
        TAVL_Assert(level < TAVL_MAXNEST);
        pbuf[level++] = &(np->left);
        np = np->left;
      } else {
        /* ¸�ߤ��ʤ� */
        TAVL_FuncEnd();
        return 0;
      }
    } else if (tmp > 0) {
      if (np->flags & TAVL_RFLAG) {
        TAVL_Assert(level < TAVL_MAXNEST);
        pbuf[level++] = &(np->right);
        np = np->right;
      } else {
        /* ¸�ߤ��ʤ� */
        TAVL_FuncEnd();
        return 0;
      }
    } else {
      /* �Ρ��ɤ����Ǥ�¸�� */
      break;
    }
    TAVL_Assert(TAVL_IsNormalNest(np));
    tmp = fcmp(p, np->p);
  }

  /* �ǡ������դޤǰ�ư */
  p = np->p; /* ������٤��ǡ�����p�˻ؤ����Ƥ��� */
  for (;;) {
    if (np->flags & TAVL_LFLAG) {
      /* ľ���ΥΡ��ɤȸ� */
      ptmp = np;
      TAVL_Assert(level < TAVL_MAXNEST && TAVL_IsNormalNest(np));
      pbuf[level++] = &(np->left);
      np = np->left;
      while (np->flags & TAVL_RFLAG) {
        TAVL_Assert(level < TAVL_MAXNEST && TAVL_IsNormalNest(np));
        pbuf[level++] = &(np->right);
        np = np->right;
      }
      if (np->right != ptmp)
        TAVL_Error("Illegal node found.");
      ptmp->p = np->p;
    } else if (np->flags & TAVL_RFLAG) {
      /* ľ��ΥΡ��ɤȸ� */
      ptmp = np;
      TAVL_Assert(level < TAVL_MAXNEST && TAVL_IsNormalNest(np));
      pbuf[level++] = &(np->right);
      np = np->right;
      while (np->flags & TAVL_RFLAG) {
        TAVL_Assert(level < TAVL_MAXNEST && TAVL_IsNormalNest(np));
        pbuf[level++] = &(np->left);
        np = np->left;
      }
      if (np->left != ptmp)
        TAVL_Error("Illegal node found.");
      ptmp->p = np->p;
    } else {
      /* �դˤ��ɤ��夤�� */
      break;
    }
  }

  /* ��� */
  TAVL_Assert(p != NULL);
  if (ffree != NULL)
    ffree((void *) p);
  np->p = NULL;
  TAVL_LeafFree(tp, np);
  level--;

  /* �ͥ��ȿ��ι����Ⱥ�Ŭ�� */
  while (--level >= 0) {
    if ((tmp = TAVL_SetNest(*pbuf[level])) >= TAVL_OPTDEPTH)
      TAVL_OptLeft(pbuf[level]);
    else if (tmp <= -TAVL_OPTDEPTH)
      TAVL_OptRight(pbuf[level]);
  }

  TAVL_FuncEnd();
  return 1;
}

/* (private��)�Ρ��ɤθ���(���ͤϥΡ��ɤؤΥݥ��󥿡����Ԥ�NULL) */
static const TAVL_Node *_TAVL_Search(const TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *)) {
  const TAVL_Node *np;
  int cmp;

  np = tp;
  cmp = 1; /* �ǽ�λҤϱ�¦ */
  for (;;) {
    TAVL_Assert(np != NULL);
    if (cmp < 0) {
      if (np->flags & TAVL_LFLAG) {
        np = np->left;
      } else {
        /* ¸�ߤ��ʤ� */
        return NULL;
      }
    } else if (cmp > 0) {
      if (np->flags & TAVL_RFLAG) {
        np = np->right;
      } else {
        /* ¸�ߤ��ʤ� */
        return NULL;
      }
    } else {
      /* �Ρ��ɤ����Ǥ�¸�� */
      break;
    }
    cmp = fcmp(p, np->p);
  }

  return np;
}

/* �ǡ����θ���(fcmp()�ϥǡ�������ӡ����ͤϥǡ����ؤΥݥ��󥿡����Ԥ�NULL) */
const void *TAVL_Search(const TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *)) {
  const TAVL_Node *np;

  if (tp == NULL || p == NULL || fcmp == NULL)
    return NULL;
  TAVL_FuncBegin("TAVL_Search");

  np = _TAVL_Search(tp, p, fcmp);

  TAVL_FuncEnd();
  return (np != NULL)? np->p: NULL;
}

/* �Ρ��ɤθ���(fcmp()�ϥǡ�������ӡ����ͤϥǡ����ؤΥݥ��󥿡����Ԥ�NULL) */
const TAVL_Node *TAVL_SearchNode(const TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *)) {
  const TAVL_Node *np;

  if (tp == NULL || p == NULL || fcmp == NULL)
    return NULL;
  TAVL_FuncBegin("TAVL_SearchNode");

  np = _TAVL_Search(tp, p, fcmp);

  TAVL_FuncEnd();
  return np;
}

/* ---- private�ؿ���� ---------------------------------------------------- */

/* �Ρ��ɤγ��� */
static TAVL_Node *TAVL_NodeAlloc(TAVL_Tree *tp) {
  TAVL_Node *np;

  if ((np = (TAVL_Node *) _TAVL_NodeAlloc(tp)) == NULL)
    return NULL;
  np->left = NULL;
  np->right = NULL;
  np->flags = 0;
  np->nest = 0;
  np->p = NULL;

  return np;
}

/* �Ρ��ɤβ��� */
static void TAVL_NodeFree(TAVL_Tree *tp, TAVL_Node *np) {
  TAVL_FuncBegin("TAVL_NodeFree");

  if (np->p != NULL || np->left != NULL || np->right != NULL || np->flags)
    TAVL_Error("Memory leakage found.");
  TAVL_Assert(np != NULL);
  _TAVL_NodeFree(tp, np);

  TAVL_FuncEnd();
}

/* �եΡ��ɤβ���(�ڤ���������ݻ����롣�ƤؤΥݥ��󥿤��֤���) */
static TAVL_Node *TAVL_LeafFree(TAVL_Tree *tp, TAVL_Node *np) {
  TAVL_Node *ptmp;

  TAVL_FuncBegin("TAVL_LeafFree");

  /* �ҤȤĿƤ���� */
  ptmp = np;
  if (ptmp->right->left == ptmp) {
    /* �Ƥκ�¦���� */
    np = ptmp->right;
    np->left = ptmp->left;
    np->flags &= ~(TAVL_LFLAG);
  } else if (ptmp->left->right == ptmp) {
    /* �Ƥα�¦���� */
    np = ptmp->left;
    np->right = ptmp->right;
    np->flags &= ~(TAVL_RFLAG);
  } else {
    TAVL_Error("Illegal node found.");
  }

  /* �եΡ��ɤ��� */
  ptmp->left = NULL;
  ptmp->right = NULL;
  TAVL_NodeFree(tp, ptmp);

  TAVL_FuncEnd();
  return np;
}

/* (private��)ľ���ΥΡ��ɤμ���(���Ԥ�NULL, �ǽ�ΥΡ��ɤ����ʤ麬��ؤ�) */
static const TAVL_Node *_TAVL_PrevNode(const TAVL_Node *np) {
  const TAVL_Node *ptmp;

  if (!(np->flags & TAVL_LFLAG))
    return np->left;
  ptmp = np;
  np = np->left;
  TAVL_Assert(np != NULL);
  while (np->flags & TAVL_RFLAG) {
    np = np->right;
    TAVL_Assert(np != NULL);
  }

  return (np->right == ptmp)? np: NULL;
}

/* (private��)ľ��ΥΡ��ɤμ���(���Ԥ�NULL, �Ǹ�ΥΡ��ɤθ�ʤ麬��ؤ�) */
static const TAVL_Node *_TAVL_NextNode(const TAVL_Node *np) {
  const TAVL_Node *ptmp;

  if (!(np->flags & TAVL_RFLAG))
    return np->right;
  ptmp = np;
  np = np->right;
  TAVL_Assert(np != NULL);
  while (np->flags & TAVL_LFLAG) {
    np = np->left;
    TAVL_Assert(np != NULL);
  }

  return (np->left == ptmp)? np: NULL;
}

/* �Ρ��ɤΥͥ��ȿ�������(���ͤϥͥ��Ȥκ�) */
static int TAVL_SetNest(TAVL_Node *np) {
  int nl, nr;

  TAVL_Assert(np != NULL && np->left != NULL && np->right != NULL);
  nl = (np->flags & TAVL_LFLAG)? np->left->nest: 0;
  nr = (np->flags & TAVL_RFLAG)? np->right->nest: 0;
  np->nest = (nl > nr)? nl+1: nr+1;

  return (nl-nr);
}

/* �Ρ��ɤΥͥ��ȿ������������� */
static int TAVL_IsNormalNest(const TAVL_Node *np) {
  int nl, nr;

  TAVL_Assert(np != NULL && np->left != NULL && np->right != NULL);
  nl = (np->flags & TAVL_LFLAG)? np->left->nest: 0;
  nr = (np->flags & TAVL_RFLAG)? np->right->nest: 0;

  return (np->nest == ((nl > nr)? nl+1: nr+1));
}

/* (private��)�ͥ��Ⱥ�Ŭ��(��¦���Ť����) */
static void TAVL_OptLeft(TAVL_Node **npp) {
  TAVL_Node *a, *b, *c;
  int nl, nr;

  TAVL_Assert(*npp != NULL);
  c = (*npp);
  a = c->left;
  b = a->right;
  TAVL_Assert(a != NULL && b != NULL && c != NULL && a->left != NULL);
  nl = (a->flags & TAVL_LFLAG)? a->left->nest: 0;
  nr = (a->flags & TAVL_RFLAG)? a->right->nest: 0;
  if (nl >= nr) {
    /* ���κ�¦���Ť���� */
    if (a->flags & TAVL_RFLAG) {
      c->left = b;
      c->flags |= TAVL_LFLAG;
    } else {
      c->left = a;
      c->flags &= ~(TAVL_LFLAG);
    }
    TAVL_SetNest(c);
    a->right = c;
    a->flags |= TAVL_RFLAG;
    TAVL_SetNest(a);
    *npp = a;
  } else {
    /* ���α�¦���Ť���� */
    TAVL_Assert(b->left != NULL && b->right != NULL);
    if (b->flags & TAVL_LFLAG) {
      a->right = b->left;
      a->flags |= TAVL_RFLAG;
    } else {
      a->right = b;
      a->flags &= ~(TAVL_RFLAG);
    }
    TAVL_SetNest(a);
    if (b->flags & TAVL_RFLAG) {
      c->left = b->right;
      c->flags |= TAVL_LFLAG;
    } else {
      c->left = b;
      c->flags &= ~(TAVL_LFLAG);
    }
    TAVL_SetNest(c);
    b->left = a;
    b->right = c;
    b->flags = TAVL_LFLAG | TAVL_RFLAG;
    TAVL_SetNest(b);
    *npp = b;
  }
}

/* (private��)�ͥ��Ⱥ�Ŭ��(��¦���Ť����) */
static void TAVL_OptRight(TAVL_Node **npp) {
  TAVL_Node *a, *b, *c;
  int nl, nr;

  TAVL_Assert(*npp != NULL);
  a = (*npp);
  c = a->right;
  b = c->left;
  TAVL_Assert(a != NULL && b != NULL && c != NULL && c->right != NULL);
  nl = (c->flags & TAVL_LFLAG)? c->left->nest: 0;
  nr = (c->flags & TAVL_RFLAG)? c->right->nest: 0;
  if (nl <= nr) {
    /* �äα�¦���Ť���� */
    if (c->flags & TAVL_LFLAG) {
      a->right = b;
      a->flags |= TAVL_RFLAG;
    } else {
      a->right = c;
      a->flags &= ~(TAVL_RFLAG);
    }
    TAVL_SetNest(a);
    c->left = a;
    c->flags |= TAVL_LFLAG;
    TAVL_SetNest(c);
    *npp = c;
  } else {
    /* �äκ�¦���Ť���� */
    TAVL_Assert(b->left != NULL && b->right != NULL);
    if (b->flags & TAVL_LFLAG) {
      a->right = b->left;
      a->flags |= TAVL_RFLAG;
    } else {
      a->right = b;
      a->flags &= ~(TAVL_RFLAG);
    }
    TAVL_SetNest(a);
    if (b->flags & TAVL_RFLAG) {
      c->left = b->right;
      c->flags |= TAVL_LFLAG;
    } else {
      c->left = b;
      c->flags &= ~(TAVL_LFLAG);
    }
    TAVL_SetNest(c);
    b->left = a;
    b->right = c;
    b->flags = TAVL_LFLAG | TAVL_RFLAG;
    TAVL_SetNest(b);
    *npp = b;
  }
}

/* ---- ���顼���� --------------------------------------------------------- */

static char *func_name = NULL; /* �桼�������Ƥ���ؿ�̾ */
static int func_nest = 0;      /* �ؿ��Υͥ��Ȥο��� */

/* �ؿ��γ��ϤȽ�λ����� */
static void TAVL_FuncBegin(char *str) {
  if (func_nest >= TAVL_MAXNEST)
    TAVL_Error("Too many TAVL_FuncBegin()!");
  if (func_nest++ == 0)
    func_name = str;
}
static void TAVL_FuncEnd(void) {
  if (func_nest <= 0)
    TAVL_Error("Too many TAVL_FuncEnd()!");
  if (--func_nest <= 0)
    func_name = NULL;
}

/* ���顼���ϴؿ������ */
static void TAVL_Error(char *str) {
  if (func_name != NULL)
    fprintf(stderr, "%s(): %s\n", func_name, str);
  else
    fprintf(stderr, "TAVL system: %s\n", str);
  exit(1);
}

