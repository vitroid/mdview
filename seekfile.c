/* "seekfile.c" シーク位置記憶型ファイル v1.10 */

#include <stdio.h>     /* ftell(), fseek() */
#include <stdlib.h>
#include <sys/types.h> /* ftello(), fseeko() */
#include <sys/stat.h>  /* stat() */
#include "seekfile.h"

/* ==== private宣言 ======================================================== */

#if HAVE_FTELLO
# define Seek_Ftell(p)       ftello(p)
# define Seek_Fseek(p, o, w) fseeko(p, o, w)
#else
# define Seek_Ftell(p)       ftell(p)
# define Seek_Fseek(p, o, w) fseek(p, o, w)
#endif

#define SEEK_FILE_MAX 16
#define SEEK_STEP_MAX 0 /* 0を指定すると無制限となる */

/* SeekCacheType型 */
static SeekCacheType *SeekCacheAlloc(const SeekFileDataFunc *);
static void SeekCacheFree(SeekCacheType *cp);
static int  SeekCacheSave(SeekCacheType *cp, const void *val, SeekSize step);
static int  SeekCacheLoad(const SeekCacheType *cp, void *val, SeekSize step);
static void SeekCacheClear(SeekCacheType *cp);

/* SeekPosType型 */
static void SeekInitPos(SeekFILE *sfp);
static void SeekTermPos(SeekFILE *sfp);
static off_t SeekSetPos(SeekFILE *sfp, SeekSize step, off_t val);
static off_t SeekGetPos(SeekFILE *sfp, SeekSize step);

/* ==== public定義 ========================================================= */

static int _seek_file_init = 0;
static SeekFILE _seek_file[SEEK_FILE_MAX];

static int SeekGetFileID(const SeekFILE *sfp);
#define _SeekFileIsActive(i) (_seek_file[i].path != NULL)

/* SeekFILEシステムの初期化 */
static void SeekFileInit(void) {
  int i;

  if (_seek_file_init)
    return;
  for (i = 0; i < SEEK_FILE_MAX; i++)
    _seek_file[i].path = NULL;
  _seek_file_init = 1;
}

/* SeekFILE型を開く。dataは扱うデータのメンバ関数。失敗はNULLを返す */
SeekFILE *SeekFileOpen(const char *path, const SeekFileDataFunc *vf,
    int (*vf_Read)(struct _SeekFILE *, void *, off_t), void *dp) {
  SeekFILE *sfp;
  struct stat st;
  int i;

  if (!_seek_file_init)
    SeekFileInit();
  for (i = 0; i < SEEK_FILE_MAX && _SeekFileIsActive(i); i++)
    ;
  if (i >= SEEK_FILE_MAX)
    return NULL;
  sfp = _seek_file+i;

  if (path == NULL || vf == NULL || vf_Read == NULL || dp == NULL)
    {fprintf(stderr, "SeekFileOpen(): Illegal Argument(s).\n"); return NULL;}
  sfp->vf = vf;
  sfp->vf_Read = vf_Read;
  sfp->dp = dp;
  if (stat(path, &st) != 0)
    {fprintf(stderr, "\"%s\": File not found.(1)\n", path); return NULL;}
  if ((sfp->fp = fopen(path, "r")) == NULL)
    {fprintf(stderr, "\"%s\": Can't open the file.(4)\n", path); return NULL;}
  sfp->size = st.st_size;
  sfp->time = st.st_mtime;
  SeekInitPos(sfp);
  if (SeekSetPos(sfp, 0, Seek_Ftell(sfp->fp)) < 0)
    {fprintf(stderr, "Read error.(2)\n"); return NULL;}
  if ((sfp->cache = SeekCacheAlloc(sfp->vf)) == NULL)
    {fprintf(stderr, "SeekFileOpen(): System error.\n"); return NULL;}

  sfp->path = path;

  return sfp;
}

/* ファイルを(もし更新されていたら)読み直す。(返り値は成功の真偽) */
int SeekFileReload(SeekFILE *sfp) {
  struct stat st;

  if (stat(sfp->path, &st) != 0)
    {fprintf(stderr, "\"%s\": Read error.(3)\n", sfp->path); return 0;}

  if (st.st_mtime == sfp->time || st.st_size == sfp->size)
    return 1; /* 読み直し不要 */
  if (st.st_size < sfp->size)
    {fprintf(stderr, "\"%s\": Illegal modification.\n", sfp->path); return 0;}

  /* 読み直す */
  fclose(sfp->fp);
  if ((sfp->fp = fopen(sfp->path, "r")) == NULL)
    {fprintf(stderr, "\"%s\": Can't reopen.\n", sfp->path); return 0;}
  sfp->size = st.st_size;
  sfp->time = st.st_mtime;

  /* シーク */
  if (!SeekPositionMove(sfp, sfp->pos.i))
    return 0;
  if (sfp->pos.max >= 0 && sfp->pos.i >= sfp->pos.max)
    SeekCacheClear(sfp->cache); /* とりあえず完全にクリアする */
  sfp->pos.max = -1;

  return 1;
}

/* ファイルを閉じる */
void SeekFileClose(SeekFILE *sfp) {
  if (!_seek_file_init)
    SeekFileInit();
  if (SeekGetFileID(sfp) < 0)
    return;

  SeekCacheFree(sfp->cache);
  sfp->cache = NULL;
  SeekTermPos(sfp);
  fclose(sfp->fp);
  sfp->fp = NULL;

  sfp->path = NULL;
}

/* MSDOSのEOFに対応したfeof */
static int _feof(FILE *fp) {
  int c;

  if (feof(fp))
    return 1;
  c = getc(fp);
  ungetc(c, fp);
  return (c == '\x1a');
}

/* ファイルsfpからstep番目のデータを読む */
int SeekFileRead(SeekFILE *sfp, void *val, SeekSize step) {
  if (SeekGetFileID(sfp) < 0)
    {fprintf(stderr, "SeekFileRead(): Illegal argument.\n"); return 0;}

  if (step < 0)
    return SEEK_READ_UNDERFLOW;

#if SEEK_STEP_MAX > 0
  if (step >= SEEK_STEP_MAX) {
    int ret;

    ret = SeekFileRead(sfp, val, SEEK_STEP_MAX-1); /* 再帰呼び出し */
    return (ret == SEEK_READ_SUCCESS)? SEEK_READ_OVERFLOW: ret;
  }
#endif

  /* キャッシュの利用 */
  if (SeekCacheLoad(sfp->cache, (void *) val, step))
    return SEEK_READ_SUCCESS;

  if (step != sfp->pos.i) {
    if (step < sfp->pos.n) {
      if (!SeekPositionMove(sfp, step))
        return SEEK_READ_FAILURE;
    } else if (sfp->pos.i < sfp->pos.n) {
      if (!SeekPositionMove(sfp, sfp->pos.n))
        return SEEK_READ_FAILURE;
    }
  }

  /* データ読み込み */
  while (step >= sfp->pos.i) {
    off_t len;

    if ((len = SeekGetPos(sfp, step+1)) >= 0)
      len -= SeekGetPos(sfp, step);
    else
      len = -1;
    if (!(sfp->vf_Read(sfp, (step == sfp->pos.i)? val: NULL, len))) {
      if (_feof(sfp->fp)) {
        sfp->pos.max = sfp->pos.i-1;
        return SEEK_READ_OVERFLOW;
      } else
        return SEEK_READ_FAILURE;
    }
    sfp->pos.i++;

#if SEEK_STEP_MAX > 0
    if (sfp->pos.i >= SEEK_STEP_MAX)
      sfp->pos.max = SEEK_STEP_MAX-1;
    else if (sfp->pos.i > sfp->pos.n && !SeekPositionAdd(sfp))
      return SEEK_READ_FAILURE;
#else
    if (sfp->pos.i > sfp->pos.n && !SeekPositionAdd(sfp))
      return SEEK_READ_FAILURE;
#endif

  }

  /* キャッシングする */
  SeekCacheSave(sfp->cache, (const void *) val, step);

  return SEEK_READ_SUCCESS;
}

/* 最大のステップ数を返す(不明の時は負を返す) */
SeekSize SeekMaxStep(const SeekFILE *sfp) {
  return (SeekGetFileID(sfp) < 0)? -1L: sfp->pos.max;
}

/* SeekFILEポインタからIDを得る(異常なら負を返す) */
static int SeekGetFileID(const SeekFILE *sfp) {
  int i;

  if (!_seek_file_init) return -1;
  for (i = 0; i < SEEK_FILE_MAX && sfp != _seek_file+i; i++)
    ;
  return (i >= SEEK_FILE_MAX)? -1: i;
}

/* sfpの現在位置を登録する */
int SeekPositionAdd(SeekFILE *sfp) {
#if SEEK_STEP_MAX > 0
  if (SeekGetFileID(sfp) < 0
      || sfp->pos.i < 0 || sfp->pos.i >= SEEK_STEP_MAX
      || sfp->pos.i != sfp->pos.n+1
      || SeekSetPos(sfp, sfp->pos.n+1, Seek_Ftell(sfp->fp)) < 0)
    return 0;
#else
  if (SeekGetFileID(sfp) < 0
      || sfp->pos.i < 0
      || sfp->pos.i != sfp->pos.n+1
      || SeekSetPos(sfp, sfp->pos.n+1, Seek_Ftell(sfp->fp)) < 0)
    return 0;
#endif

  sfp->pos.n++;

  return 1;
}

/* step番目に登録した位置にシークする。(0 <= step <= pos.n) */
int SeekPositionMove(SeekFILE *sfp, SeekSize step) {
  off_t pos;

  if (SeekGetFileID(sfp) < 0
      || step > sfp->pos.n || step < 0
      || (pos = SeekGetPos(sfp, step)) < 0
      || Seek_Fseek(sfp->fp, pos, SEEK_SET) != 0)
    return 0;
  sfp->pos.i = step;

  return 1;
}

/* ---- SeekPosType型 ------------------------------------------------------ */

#define SEEK_POS_RAW 1000
#define SEEK_POS_COL_MIN 16

/* SeekPosTypeの初期化 */
static void SeekInitPos(SeekFILE *sfp) {
  SeekSize i;

  sfp->pos.i = 0;
  sfp->pos.n = 0;
  sfp->pos.max = -1;

#if SEEK_STEP_MAX > 0
  sfp->pos.buf_max = ((SEEK_STEP_MAX-1)/SEEK_POS_RAW+1);
#else
  sfp->pos.buf_max = SEEK_POS_COL_MIN;
#endif

  if ((sfp->pos.buf = (off_t **)
      malloc(sizeof(off_t *) * sfp->pos.buf_max)) == NULL) {
    fprintf(stderr, "Allocation failed.\n");
    exit(1);
  }
  for (i = 0; i < sfp->pos.buf_max; i++)
    sfp->pos.buf[i] = NULL;
}

/* SeekPosTypeの終了処理 */
static void SeekTermPos(SeekFILE *sfp) {
  SeekSize i;

  for (i = 0; i < sfp->pos.buf_max; i++) {
    free(sfp->pos.buf[i]);
    sfp->pos.buf[i] = NULL;
  }
  free(sfp->pos.buf);
  sfp->pos.buf_max = 0;
  sfp->pos.i = 0;
  sfp->pos.n = 0;
  sfp->pos.max = -1;
}

/* step番目の位置をvalに設定する(返り値はval。失敗は負を返す) */
static off_t SeekSetPos(SeekFILE *sfp, SeekSize step, off_t val) {
  SeekSize col, i;
  off_t *p;

  col = step / SEEK_POS_RAW;

#if SEEK_STEP_MAX <= 0
  if (col >= sfp->pos.buf_max) {
    SeekSize buf_max_org = sfp->pos.buf_max;
    off_t **tmp;

    do {
      sfp->pos.buf_max <<= 1;
      if ((size_t) (sizeof(off_t *) * sfp->pos.buf_max) <= 0)
        {fprintf(stderr, "Allocation failed.\n"); exit(1);}
    } while (col >= sfp->pos.buf_max);
    if ((tmp = (off_t **)
        realloc(sfp->pos.buf, sizeof(off_t *) * sfp->pos.buf_max)) == NULL)
      {fprintf(stderr, "Allocation failed.\n"); exit(1);}
    sfp->pos.buf = tmp;
    for (i = buf_max_org; i < sfp->pos.buf_max; i++)
      sfp->pos.buf[i] = NULL;
  }
#endif

  if ((p = sfp->pos.buf[col]) == NULL) {
    if ((p = (off_t *) malloc(sizeof(off_t) * SEEK_POS_RAW)) == NULL)
      {fprintf(stderr, "Allocation failed.\n"); exit(1);}
    sfp->pos.buf[col] = p;
    for (i = 0; i <  SEEK_POS_RAW; i++)
      p[i] = -1; /* 不正な値 */
  }
  return (p[step % SEEK_POS_RAW] = val);
}

/* step番目の位置を求める(失敗は負の値) */
static off_t SeekGetPos(SeekFILE *sfp, SeekSize step) {
  off_t *p;

  if (step < 0 || step >= sfp->pos.buf_max*SEEK_POS_RAW
      || (p = sfp->pos.buf[step / SEEK_POS_RAW]) == NULL)
    return -1;
  return p[step % SEEK_POS_RAW];
}

/* ---- SeekCacheType型 ---------------------------------------------------- */

#define CASHE_NOTHING (-1)
#define CASHE_MAX     16

static int _cache_init = 0;
static SeekCacheType _cache[CASHE_MAX];

#define _SeekCacheIsActive(i) (_cache[i].p != NULL)

/* SeekCacheTypeシステムの初期化 */
static void SeekCacheInit(void) {
  int i;

  if (_cache_init) return;
  for (i = 0; i < CASHE_MAX; i++)
    _cache[i].p = NULL;
  _cache_init = 1;
}

/* SeekCacheTypeの確保(失敗はNULLを返す) */
static SeekCacheType *SeekCacheAlloc(const SeekFileDataFunc *vf) {
  SeekCacheType *cp;
  int i;

  if (!_cache_init)
    SeekCacheInit();
  if (vf == NULL || vf->Alloc == NULL || vf->Free == NULL || vf->Copy == NULL)
    {fprintf(stderr, "SeekCacheAlloc(): Illegal argument.\n"); return NULL;}

  for (i = 0; i < CASHE_MAX && _SeekCacheIsActive(i); i++)
    ;
  if (i >= CASHE_MAX)
    return NULL;
  cp = _cache+i;

  cp->vf = vf;
  if ((cp->p = cp->vf->Alloc()) == NULL)
    return NULL;
  cp->current = CASHE_NOTHING;

  return cp;
}

/* SeekCacheTypeの無効化 */
static void SeekCacheClear(SeekCacheType *cp) {
  if (!_cache_init)
    return;
  cp->current = CASHE_NOTHING;
}

/* SeekCacheTypeの解放 */
static void SeekCacheFree(SeekCacheType *cp) {
  int i;

  if (!_cache_init)
    return;
  for (i = 0; i < CASHE_MAX && cp != _cache+i; i++)
    ;
  if (i >= CASHE_MAX)
    return;
  cp->vf->Free(cp->p);
  cp->p = NULL;
  cp->current = CASHE_NOTHING;
}

/* SeekCacheTypeの書き込み(返り値は成功の真偽) */
static int SeekCacheSave(SeekCacheType *cp, const void *val, SeekSize step) {
  SeekSize i;

  if (!_cache_init || step < 0)
    return 0;
  for (i = 0; i < CASHE_MAX && cp != _cache+i; i++)
    ;
  if (i >= CASHE_MAX)
    return 0;

  if (step != cp->current) {
    cp->vf->Copy(cp->p, val);
    cp->current = step;
  }

  return 1;
}

/* SeekCacheTypeの読み込み(返り値は成功の真偽) */
static int SeekCacheLoad(const SeekCacheType *cp, void *val, SeekSize step) {
  SeekSize i;

  if (!_cache_init)
    return 0;
  for (i = 0; i < CASHE_MAX && cp != _cache+i; i++)
    ;
  if (i >= CASHE_MAX)
    return 0;
  if (cp->current == CASHE_NOTHING || step != cp->current)
    return 0;
  cp->vf->Copy(val, cp->p);

  return 1;
}

