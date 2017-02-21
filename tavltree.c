/* "tavltree.c" ひも付きAVL木型 Ver. 0.61  Programed by Akinori Baba */

#include<stdio.h>
#include<stdlib.h>
#include"tavltree.h"

/* ---- 設定 --------------------------------------------------------------- */

#define TAVL_ASSERT_ENABLE /* 整合性判定機能を有効にする */
#define TAVL_CHUNK_ENABLE  /* 木ごとにメモリ管理を独立に行う */

/* ---- 宣言 --------------------------------------------------------------- */

/* TAVL_Node型 */
#define TAVL_LFLAG 0x01 /* 左が子へのポインタならON */
#define TAVL_RFLAG 0x02 /* 右が子へのポインタならON */
#define TAVL_MAXNEST 48 /* ポインタ型のビット数*1.5で十分 */
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

/* ヒープ関連 */
#ifdef TAVL_CHUNK_ENABLE
/* Chunk_Typeの利用 */
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
/* 通常のmalloc()/free()の利用 */
#define _TAVL_InitMalloc(tp)  1
#define _TAVL_TermMalloc(tp)  ((void)0)
#define _TAVL_NodeAlloc(tp)   malloc(sizeof(TAVL_Node))
#define _TAVL_NodeFree(tp,np) free(np)
#endif /* TAVL_CHUNK_ENABLE */

/* エラーチェック */
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

/* ---- public関数定義 ----------------------------------------------------- */

/* 木の確保 */
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

/* 木の解放(ffree()はデータの解放関数。NULLなら解放しない) */
void TAVL_Free(TAVL_Tree *tp, void (*ffree)(void *)) {
  TAVL_Node *np;

  if (tp == NULL)
    return;
  TAVL_FuncBegin("TAVL_Free");

  /* ノードの解放 */
#ifdef TAVL_CHUNK_ENABLE
  if (ffree != NULL) {
#endif
  np = tp;
  for (;;) {
    /* 葉を探す */
    for (;;) {
      if (np->flags & TAVL_LFLAG)
        np = np->left;
      else if (np->flags & TAVL_RFLAG)
        np = np->right;
      else
        break;
      TAVL_Assert(TAVL_IsNormalNest(np));
    }
    /* それでも根なら完了 */
    if (np == tp)
      break;

    /* 葉ノードを削除して１つ親に戻る */
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

  /* 根の解放 */
  _TAVL_TermMalloc(tp);
  tp->left = NULL;
  tp->right = NULL;
  tp->flags = 0;
  tp->p = NULL;
  TAVL_NodeFree(NULL, tp);

  TAVL_FuncEnd();
}

/* 最初のノードの取得(失敗はNULL) */
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

/* 最後のノードの取得(失敗はNULL) */
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

/* 直前のノードの取得 */
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

/* 直後のノードの取得 */
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

/* 挿入(fcmp()は比較関数。fcpy()はコピー関数で、NULLなら何もしない) */
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
  tmp = 1; /* 最初の子は右側 */
  level = 0;
  for (;;) {
    if (tmp < 0) {
      if (np->flags & TAVL_LFLAG) {
        TAVL_Assert(level < TAVL_MAXNEST);
        pbuf[level++] = &(np->left);
        np = np->left;
      } else {
        /* 左側に登録 */
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
        /* 右側に登録 */
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
      /* ノードがすでに存在 */
      TAVL_FuncEnd();
      return NULL;
    }
    TAVL_Assert(TAVL_IsNormalNest(np));
    tmp = fcmp(p, np->p);
  }

  /* ネスト数の更新と最適化 */
  while (--level >= 0) {
    if ((tmp = TAVL_SetNest(*pbuf[level])) >= TAVL_OPTDEPTH)
      TAVL_OptLeft(pbuf[level]);
    else if (tmp <= -TAVL_OPTDEPTH)
      TAVL_OptRight(pbuf[level]);
  }

  TAVL_FuncEnd();
  return p;
}

/* 削除(fcmp()は比較関数。ffree()は解放関数で、NULLなら何もしない) */
int TAVL_Delete(TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *), void (*ffree)(void *)) {
  TAVL_Node **pbuf[TAVL_MAXNEST];
  int level;
  TAVL_Node *np, *ptmp;
  int tmp;

  if (tp == NULL || p == NULL || fcmp == NULL)
    return 0;
  TAVL_FuncBegin("TAVL_Delete");

  /* 削除すべきノードの検索 */
  np = tp;
  tmp = 1; /* 最初の子は右側 */
  level = 0;
  for (;;) {
    if (tmp < 0) {
      if (np->flags & TAVL_LFLAG) {
        TAVL_Assert(level < TAVL_MAXNEST);
        pbuf[level++] = &(np->left);
        np = np->left;
      } else {
        /* 存在しない */
        TAVL_FuncEnd();
        return 0;
      }
    } else if (tmp > 0) {
      if (np->flags & TAVL_RFLAG) {
        TAVL_Assert(level < TAVL_MAXNEST);
        pbuf[level++] = &(np->right);
        np = np->right;
      } else {
        /* 存在しない */
        TAVL_FuncEnd();
        return 0;
      }
    } else {
      /* ノードがすでに存在 */
      break;
    }
    TAVL_Assert(TAVL_IsNormalNest(np));
    tmp = fcmp(p, np->p);
  }

  /* データを葉まで移動 */
  p = np->p; /* 削除すべきデータをpに指させておく */
  for (;;) {
    if (np->flags & TAVL_LFLAG) {
      /* 直前のノードと交換 */
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
      /* 直後のノードと交換 */
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
      /* 葉にたどり着いた */
      break;
    }
  }

  /* 削除 */
  TAVL_Assert(p != NULL);
  if (ffree != NULL)
    ffree((void *) p);
  np->p = NULL;
  TAVL_LeafFree(tp, np);
  level--;

  /* ネスト数の更新と最適化 */
  while (--level >= 0) {
    if ((tmp = TAVL_SetNest(*pbuf[level])) >= TAVL_OPTDEPTH)
      TAVL_OptLeft(pbuf[level]);
    else if (tmp <= -TAVL_OPTDEPTH)
      TAVL_OptRight(pbuf[level]);
  }

  TAVL_FuncEnd();
  return 1;
}

/* (private版)ノードの検索(返値はノードへのポインタ。失敗はNULL) */
static const TAVL_Node *_TAVL_Search(const TAVL_Tree *tp, const void *p,
    int (*fcmp)(const void *, const void *)) {
  const TAVL_Node *np;
  int cmp;

  np = tp;
  cmp = 1; /* 最初の子は右側 */
  for (;;) {
    TAVL_Assert(np != NULL);
    if (cmp < 0) {
      if (np->flags & TAVL_LFLAG) {
        np = np->left;
      } else {
        /* 存在しない */
        return NULL;
      }
    } else if (cmp > 0) {
      if (np->flags & TAVL_RFLAG) {
        np = np->right;
      } else {
        /* 存在しない */
        return NULL;
      }
    } else {
      /* ノードがすでに存在 */
      break;
    }
    cmp = fcmp(p, np->p);
  }

  return np;
}

/* データの検索(fcmp()はデータの比較。返値はデータへのポインタ。失敗はNULL) */
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

/* ノードの検索(fcmp()はデータの比較。返値はデータへのポインタ。失敗はNULL) */
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

/* ---- private関数定義 ---------------------------------------------------- */

/* ノードの確保 */
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

/* ノードの解放 */
static void TAVL_NodeFree(TAVL_Tree *tp, TAVL_Node *np) {
  TAVL_FuncBegin("TAVL_NodeFree");

  if (np->p != NULL || np->left != NULL || np->right != NULL || np->flags)
    TAVL_Error("Memory leakage found.");
  TAVL_Assert(np != NULL);
  _TAVL_NodeFree(tp, np);

  TAVL_FuncEnd();
}

/* 葉ノードの解放(木の整合性を維持する。親へのポインタを返す。) */
static TAVL_Node *TAVL_LeafFree(TAVL_Tree *tp, TAVL_Node *np) {
  TAVL_Node *ptmp;

  TAVL_FuncBegin("TAVL_LeafFree");

  /* ひとつ親に戻る */
  ptmp = np;
  if (ptmp->right->left == ptmp) {
    /* 親の左側が葉 */
    np = ptmp->right;
    np->left = ptmp->left;
    np->flags &= ~(TAVL_LFLAG);
  } else if (ptmp->left->right == ptmp) {
    /* 親の右側が葉 */
    np = ptmp->left;
    np->right = ptmp->right;
    np->flags &= ~(TAVL_RFLAG);
  } else {
    TAVL_Error("Illegal node found.");
  }

  /* 葉ノードを削除 */
  ptmp->left = NULL;
  ptmp->right = NULL;
  TAVL_NodeFree(tp, ptmp);

  TAVL_FuncEnd();
  return np;
}

/* (private版)直前のノードの取得(失敗はNULL, 最初のノードの前なら根を指す) */
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

/* (private版)直後のノードの取得(失敗はNULL, 最後のノードの後なら根を指す) */
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

/* ノードのネスト数の設定(返値はネストの差) */
static int TAVL_SetNest(TAVL_Node *np) {
  int nl, nr;

  TAVL_Assert(np != NULL && np->left != NULL && np->right != NULL);
  nl = (np->flags & TAVL_LFLAG)? np->left->nest: 0;
  nr = (np->flags & TAVL_RFLAG)? np->right->nest: 0;
  np->nest = (nl > nr)? nl+1: nr+1;

  return (nl-nr);
}

/* ノードのネスト数の整合性検査 */
static int TAVL_IsNormalNest(const TAVL_Node *np) {
  int nl, nr;

  TAVL_Assert(np != NULL && np->left != NULL && np->right != NULL);
  nl = (np->flags & TAVL_LFLAG)? np->left->nest: 0;
  nr = (np->flags & TAVL_RFLAG)? np->right->nest: 0;

  return (np->nest == ((nl > nr)? nl+1: nr+1));
}

/* (private版)ネスト最適化(左側が重い場合) */
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
    /* Ａの左側が重い場合 */
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
    /* Ａの右側が重い場合 */
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

/* (private版)ネスト最適化(右側が重い場合) */
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
    /* Ｃの右側が重い場合 */
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
    /* Ｃの左側が重い場合 */
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

/* ---- エラー処理 --------------------------------------------------------- */

static char *func_name = NULL; /* ユーザーが呼んだ関数名 */
static int func_nest = 0;      /* 関数のネストの深さ */

/* 関数の開始と終了の宣言 */
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

/* エラー出力関数の定義 */
static void TAVL_Error(char *str) {
  if (func_name != NULL)
    fprintf(stderr, "%s(): %s\n", func_name, str);
  else
    fprintf(stderr, "TAVL system: %s\n", str);
  exit(1);
}

