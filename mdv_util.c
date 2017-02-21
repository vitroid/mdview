/* "mdv_util.c" �Ƽ�桼�ƥ���ƥ� */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> /* isalpha() */
#include <errno.h>
#include "mdview.h"
#include "mdv_util.h"
#include "hash.h"
#include "chunk.h"
#include "stack.h"
#include "str2int.h"

/* ---- ��ĥatexit()��Ϣ --------------------------------------------------- */

#define MAXATEXIT 256 /* ��Ͽ�ؿ��ο��ξ�� */
static int atexit_n = 0;
static void (*atexit_buf[MAXATEXIT])(void);
static int atexit_active = 0; /* ��λ�����¹���ʤ鿿 */

/* �ºݤ�atexit()�ǸƤФ��ؿ� */
static void TermAtExit(void) {
  atexit_active = 1;
  while (--atexit_n >= 0)
    (atexit_buf[atexit_n])();
}

/* atexit()�����Ѥ򤹤�ؿ�(������0�ǡ����Ԥ���0���֤���) */
int AtExit(void (*func)()) {
  if (atexit_active) {
    /* ���ꤷ���ؿ����AtExit()�򤷤��Ȥ����к� */
    fprintf(stderr,"AtExit(): Illegal function call in TermAtExit().\n");
    return -1;
  }

  if (atexit_n <= 0) {
    /* AtExit()�ν���� */
    atexit(TermAtExit);
  }
  if (atexit_n >= MAXATEXIT) return -1;
  /* ��Ͽ */
  atexit_buf[atexit_n++] = func;

  return 0;
}

/* ---- ʸ���� <-> ʸ����ID ------------------------------------------------ */

static Str2Int_Type *sip = NULL;

/* ʸ����str���б�����StringID���֤���̤�Τ�ʸ����ϼ��ԡ�(���Ԥ������) */
StringID SearchStringID(const char *str) {
  if (sip == NULL)
    sip = Str2Int_Alloc();
  return (StringID) Str2Int_SearchID(str, sip);
}

/* ʸ����str���б�����StringID���֤���̤�Τ�ʸ�������Ͽ��(���Ԥ������) */
StringID Str2StringID(const char *str) {
  if (sip == NULL)
    sip = Str2Int_Alloc();
  return (StringID) Str2Int_Str2ID(str, sip);
}

/* StringID���б�����ʸ����str���֤�(���Ԥ�NULL) */
const char *StringID2Str(StringID sid) {
  if (sip == NULL)
    sip = Str2Int_Alloc();
  return Str2Int_ID2Str((Str2Int_Size) sid, sip);
}

/* ---- Str2Rgb()��Ϣ ------------------------------------------------------ */

#ifndef RGB_FILEPATH
#define RGB_FILEPATH "/usr/X11R6/lib/X11/rgb.txt"
#define RGB_FILEPATH "/opt/X11/share/X11/rgb.txt"
#endif

/* RGB_Type�� */
typedef struct {
  StringID si;
  int r, g, b;
} RGB_Type;

typedef Hash_Type RGB_Table;
RGB_Table *RGB_Table_Alloc(void);
void RGB_Table_Free(RGB_Table *rtp);
RGB_Type *RGB_Table_Search(const RGB_Table *rtp, StringID si);
RGB_Type *RGB_Table_Insert(RGB_Table *rtp, StringID si);

static RGB_Table *rgb_table = NULL; /* ������: �������ʤ� */

/* rgb.txt���ɤ�ǡ�ʸ����-RGB�ͤ��б�ɽ��������� */
static int _init_rgbtable = 0;
void RGBTableInit(void) {
  FILE *fp;

  /* ����� */
  if ((rgb_table = RGB_Table_Alloc()) == NULL)
    HeapError();

  /* rgb.txt�Υ����ץ� */
  if ((RGB_FILEPATH)[0] != '/') {
    char *path;
    int npath;

    npath = strlen(exec_dir)+strlen(RGB_FILEPATH)+1;
    if ((path = (char *) malloc(npath)) == NULL)
      HeapError();
    Strlcpy(path, exec_dir, npath);
    Strlcat(path, RGB_FILEPATH, npath);
    if ((fp = fopen(path, "r")) == NULL)
      fprintf(stderr, "%s: Can't open.(5)\n", path);
    free(path);
  } else {
    if ((fp = fopen(RGB_FILEPATH, "r")) == NULL)
      fprintf(stderr, "%s: Can't open.(6)\n", RGB_FILEPATH);
  }

  /* �ɤ߹��� */
  if (fp != NULL) {
    MDV_Array *line;
    int c;

    line = MDV_Array_Alloc(1);
    while ((c = getc(fp)) != EOF) {
      char *p;
      RGB_Type rgb, *rp;
      int i;

      /* ��Ԥ��ɤ߹��� */
      MDV_Array_SetSize(line, 0);
      do {
        MDV_Array_AppendChar(line, c);
      } while ((c = getc(fp)) != EOF && c != '\n');
      if (c == '\n')
        MDV_Array_AppendChar(line, c);
      MDV_Array_AppendChar(line, '\0');
      p = (char *) line->p;

      /* �����ȹԤΥ����å� */
      if (p[0] == '!')
        continue;

      /* ��ü�ζ���Ȳ���ʸ������ */
      for (i = line->n-1; i >= 0 && (p[i] == ' ' || p[i] == '\t'
          || p[i] == '\n' || p[i] == '\0'); i--) {
        p[i] = '\0';
      }

      /* RGB�ͤ��ɤ߹��� */
      rgb.r = (int) strtol(p, &p, 10);
      if (errno == ERANGE)
        {fprintf(stderr,"Read error.(4)\n"); exit(1);}
      rgb.g = (int) strtol(p, &p, 10);
      if (errno == ERANGE)
        {fprintf(stderr,"Read error.(5)\n"); exit(1);}
      rgb.b = (int) strtol(p, &p, 10);
      if (errno == ERANGE)
        {fprintf(stderr,"Read error.(6)\n"); exit(1);}

      /* ����ʸ���ν��� */
      while (*p == ' ' || *p == '\t')
        p++;

      /* ��ʸ������ɤ߹��� */
      if (p[0] == '\0')
        {fprintf(stderr,"Read error.(7)\n"); exit(1);}
      rgb.si = Str2StringID(p);

      /* ��Ͽ */
      if (!RGB_Table_Search(rgb_table, rgb.si)) {
        if ((rp = RGB_Table_Insert(rgb_table, rgb.si)) == NULL)
          MDV_Fatal("RGBTableInit()");
        *rp = rgb;
      } else {
        fprintf(stderr, "%s: Already exists.\n", p);
      }
    }
    MDV_Array_Free(line);
    fclose(fp);
  }
}

static int _hexnum(int c) {
  if (c >= '0' && c <= '9')
    c = c - '0';
  else if (c >= 'A' && c <= 'F')
    c = c - 'A' + 10;
  else if (c >= 'a' && c <= 'f')
    c = c - 'a' + 10;
  else
    c = -1;

  return c;
}

/* ��ʸ������ɤ�ǡ��б�����RGB�ͤ���롣(���ͤ������ο���) */
int Str2Rgb(const char *name, int *pr, int *pg, int *pb) {
  RGB_Type *ret;
  StringID si;
  int h,l;

  if (!_init_rgbtable) {
    RGBTableInit();
    _init_rgbtable = 1;
  }

  if (name[0] == '#') {
    /* "#RRGGBB"���� */
    if ((h = _hexnum(name[1])) < 0) return 0;
    if ((l = _hexnum(name[2])) < 0) return 0;
    *pr = h*16+l;
    if ((h = _hexnum(name[3])) < 0) return 0;
    if ((l = _hexnum(name[4])) < 0) return 0;
    *pg = h*16+l;
    if ((h = _hexnum(name[5])) < 0) return 0;
    if ((l = _hexnum(name[6])) < 0) return 0;
    *pb = h*16+l;
  } else {
    /* ����̾�� */
    if ((si = SearchStringID(name)) < 0
        || (ret = RGB_Table_Search(rgb_table, si)) == NULL)
      return 0;
    *pr = ret->r;
    *pg = ret->g;
    *pb = ret->b;
  }

  return 1;
}

/* ---- RGB_Table�� -------------------------------------------------------- */

/* RGB_Type���ν���� */
void RGB_Type_Init(RGB_Type *rp, StringID si) {
  rp->si = si;
  rp->r = 0;
  rp->g = 0;
  rp->b = 0;
}

/* RGB_Type�ѥ������� */
#define RGB_CHUNK_SIZE (256*sizeof(RGB_Type))
static Chunk_Type *rgb_chunk = NULL; /* ������: ��λ�����Ͼ�ά */

/* ��Ӵؿ�(StringID������դ������ǡ�����0�ʾ���ͤǤ��뤳�Ȥ�����Ȥ���) */
static int RGB_Table_NodeCompare(const void *va, const void *vb) {
  return (int) (((const RGB_Type *) va)->si - ((const RGB_Type *) vb)->si);
}

/* �ϥå���ؿ� */
static Hash_Size RGB_Table_NodeHash(const void *vp) {
  return (Hash_Size) ((const RGB_Type *) vp)->si;
}

/* ���ԡ����󥹥ȥ饯�� */
static void *RGB_Table_NodeCopy(const void *vp) {
  const RGB_Type *rp = (const RGB_Type *) vp;
  RGB_Type *ret;

  if (vp == NULL || rgb_chunk == NULL)
    return NULL;
  if ((ret = (RGB_Type *) Chunk_NodeAlloc(rgb_chunk)) == NULL)
    return NULL;
  *ret = *rp;

  return ret;
}

/* �ǥ��ȥ饯�� */
static void RGB_Table_NodeFree(void *vp) {
  if (vp == NULL || rgb_chunk == NULL)
    return;
  Chunk_NodeFree(rgb_chunk, vp);
}

/* ���󥹥��󥹤κ��� */
RGB_Table *RGB_Table_Alloc(void) {
  if (rgb_chunk == NULL) {
    if ((rgb_chunk = Chunk_TypeAlloc(sizeof(RGB_Type),
        RGB_CHUNK_SIZE, 0)) == NULL)
      return NULL;
  }
  return Hash_Alloc(0, 0);
}

/* ���󥹥��󥹤ξõ� */
void RGB_Table_Free(RGB_Table *rtp) {
  Hash_Free(rtp, RGB_Table_NodeFree);
}

/* RGB�����õ�� */
RGB_Type *RGB_Table_Search(const RGB_Table *rtp, StringID si) {
  RGB_Type key;

  if (si < 0)
    return NULL;
  RGB_Type_Init(&key, si);
  return (RGB_Type *) Hash_Search(rtp, &key, RGB_Table_NodeCompare,
    RGB_Table_NodeHash);
}

/* RGB�������Ͽ */
RGB_Type *RGB_Table_Insert(RGB_Table *rtp, StringID si) {
  RGB_Type rgb;

  if (si < 0)
    return NULL;
  RGB_Type_Init(&rgb, si);
  return (RGB_Type *) Hash_Insert(rtp, &rgb, RGB_Table_NodeCompare,
    RGB_Table_NodeHash, RGB_Table_NodeCopy);
}

/* ---- ������ꥢ ------------------------------------------------------- */

static Stack_Type *mdv_work = NULL;

static void MDV_WorkTerm(void) {
  Stack_TypeFree(mdv_work);
  mdv_work = NULL;
}

/* ������ꥢ�γ��� */
void *MDV_Work_Alloc(MDV_Size n) {
  void *p;

  if (mdv_work == NULL) {
    if ((mdv_work = Stack_TypeAlloc()) == NULL)
      HeapError();
    AtExit(MDV_WorkTerm);
  }
  if (n < 0)
    MDV_Fatal("MDV_Work_Alloc(): Illegal argument.");
  if ((p = Stack_NodeAlloc(mdv_work, n)) == NULL)
    HeapError();

  return p;
}

/* ������ꥢ�β��� */
void MDV_Work_Free(void *p) {
  if (mdv_work == NULL)
    MDV_Fatal("MDV_Work_Free()");
  Stack_NodeFree(mdv_work, p);
}

/* ---- Path2Dir(),Path2File() --------------------------------------------- */

#ifndef DIR_SEPARATOR
#define DIR_SEPARATOR '/'
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
static char path_buf[MAXPATHLEN];

/* ����path��? */
int IsRelativePath(const char *path) {
  if (path == NULL)
    {fprintf(stderr, "IsRelativePath(): system error.\n"); exit(1);}

  if (DIR_SEPARATOR == '\\') {
    /* MS-DOS�Υɥ饤�ֻ�����б����� */
    if (isalpha(path[0]) && path[1] == ':')
      return 0;
  }
  return (path[0] != DIR_SEPARATOR);
}

/* path����Υǥ��쥯�ȥ�̾���ڤ�Ф�(�֤��ͤ�static char����) */
const char *Path2Dir(const char *path) {
  char *p;

  if (Strlcpy(path_buf, path, MAXPATHLEN) >= MAXPATHLEN)
    {fprintf(stderr, "IsRelativePath(): system error.\n"); exit(1);}
  p = strrchr(path_buf, DIR_SEPARATOR);
  if (p == NULL)
    p = path_buf;
  else
    p++;
  *p = '\0';
  return path_buf;
}

/* path����Υե�����̾���ڤ�Ф�(�֤��ͤ�static char����) */
const char *Path2File(const char *path) {
  char *p;

  if (Strlcpy(path_buf, path, MAXPATHLEN) >= MAXPATHLEN)
    fprintf(stderr, "Path2File(): system error.\n");
  p = strrchr(path_buf, DIR_SEPARATOR);
  if (p == NULL)
    p = path_buf;
  else
    p++;
  return p;
}

/* ---- MDV_Fatal(), HeapError() ------------------------------------------- */

/* ���顼��λ */
void MDV_Fatal(const char *str) {
  fprintf(stderr,"%s: system error.\n", str);
  exit(1);
}

/* �ҡ�����­�ˤ�륨�顼��λ */
void HeapError(void) {
  fprintf(stderr,"Allocation failed.\n");
  exit(1);
}

/* ---- ����Ĺ���� --------------------------------------------------------- */

/* ������ʸ���� */
static const char *mdv_array_str = "MDV_Array";

#define MDV_ALLOC_MIN_SIZE 1024

/* ����Ĺ����γ���(���Ԥϰ۾ｪλ��size�����Ǥ��礭��) */
MDV_Array *MDV_Array_Alloc(MDV_Size size) {
  MDV_Array *p;

  if (size <= 0)
    MDV_Fatal("MDV_Array_Alloc()");
  if ((p = (MDV_Array *) malloc(sizeof(MDV_Array))) == NULL)
    HeapError();
  p->n = 0;
  p->size = size;
  p->max = MDV_ALLOC_MIN_SIZE;
  while (p->max < size) {
    p->max <<= 1;
    if (p->max <= 0)
      MDV_Fatal("MDV_Array_Alloc()");
  }
  if ((p->p = malloc(p->max)) == NULL)
    HeapError();
  p->header_str = mdv_array_str;

  return p;
}

/* ����Ĺ����β��� */
void MDV_Array_Free(MDV_Array *p) {
  if (p == NULL)
    return;
  if (p->header_str != mdv_array_str)
    MDV_Fatal("MDV_Array_Free()");
  free(p->p);
  p->header_str = NULL;
  free(p);

  return;
}

/* ����Ĺ����Υ��������ѹ� */
void MDV_Array_SetSize(MDV_Array *p, MDV_Size n) {
  if (p == NULL || p->header_str != mdv_array_str || n < 0)
    MDV_Fatal("MDV_Array_SetSize()");
  if (n > 0 && p->size > p->max/n) {
    void *tmp;

    do {
      p->max <<= 1;
      if ((size_t) p->max <= 0)
        MDV_Fatal("MDV_Array_SetSize()");
    } while (p->size > p->max/n);
    if ((tmp = realloc(p->p, p->max)) == NULL)
      HeapError();
    p->p = tmp;
  }
  p->n = n;
}

/* ʸ������Ȥߤʤ��ƥꥵ�������� */
void _MDV_Array_Resize(MDV_Array *a, char c) {
  MDV_Size n;

  if (a == NULL || a->size != 1)
    MDV_Fatal("_MDV_Array_Resize");
  n = a->n;
  MDV_Array_SetSize(a, n+1);
  ((char *) a->p)[n] = c;
}

