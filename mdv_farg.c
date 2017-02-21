/* "mdv_farg.c" �����ե�����Υ��󥯥롼�ɵ�ǽ�դ��������� */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mdv_util.h" /* AtExit() */
#include "mdv_text.h" /* ReadToken() */
#include "mdv_farg.h"

#define FARG_MINSTR   256        /* ����Ĺʸ����κǾ������� */
#define FARG_MAXFILES 10         /* �ե�����κ���ͥ��ȿ� */

static const char * const *_farg_argv = NULL; /* �����ؤΥ�ե���� */
static int _farg_i = 0;                       /* �����θ��ߤλ��Ȱ��� */
static int _farg_is_end = 0;                  /* ��λȽ���ѥե饰 */
static char *_farg_buf = NULL;                /* ������Ǽ�Ѳ���Ĺ���� */
static size_t _farg_buf_n = 0;                /* Ʊ��Υ����� */
static char *_farg_bak = NULL;                /* �����ΥХå����å� */

static FILE *_farg_fp[FARG_MAXFILES];    /* �ե�����ݥ��󥿳�Ǽ�� */
static char *_farg_fp_s[FARG_MAXFILES];  /* �ե�����̾ */
static size_t _farg_fp_l[FARG_MAXFILES]; /* �ե�����ν����Կ� */
static int _farg_fp_i = -1;              /* ���ߤΥͥ��ȥ�٥� */

/* �����ƥ�ν�λ */
static void FARG_Term(void) {
  int i;

  _farg_argv = NULL;
  _farg_i = 0;
  _farg_is_end = 0;
  free(_farg_buf); _farg_buf = NULL;
  free(_farg_bak); _farg_bak = NULL;
  _farg_buf_n = 0;
  for (i = 0; i < FARG_MAXFILES; i++) {
    if (_farg_fp[i] != NULL) {fclose(_farg_fp[i]); _farg_fp[i] = NULL;}
    free(_farg_fp_s[i]); _farg_fp_s[i] = NULL;
    _farg_fp_l[i] = 0;
  }
  _farg_fp_i = -1;
}

/* ����� */
void FARG_Init(const char * const argv[]) {
  int i;

  /* �����ƥ�ν���� */
  if (_farg_argv == NULL) {
    _farg_buf_n = FARG_MINSTR;
    if ((_farg_buf = (char *) malloc(_farg_buf_n)) == NULL) HeapError();
    _farg_buf[0] = '\0';
    for (i = 0; i < FARG_MAXFILES; i++) {
      _farg_fp[i] = NULL;
      _farg_fp_s[i] = NULL;
      _farg_fp_l[i] = 0;
    }
    AtExit(FARG_Term);
  }

  /* �����Υ�ե���󥹤����� */
  if (argv == NULL)
    {fprintf(stderr, "FARG_Init(): Illegal argument.\n"); exit(1);}
  _farg_argv = argv;
  _farg_i = 0;
  _farg_is_end = 0;

  /* �����ΥХå����å��ΰ��(��)����� */
  free(_farg_bak); _farg_bak = NULL;

  /* �����ե�����Хåե���(��)����� */
  for (i = 0; i < FARG_MAXFILES; i++) {
    if (_farg_fp[i] != NULL) {fclose(_farg_fp[i]); _farg_fp[i] = NULL;}
    if (_farg_fp_s[i] != NULL) {free(_farg_fp_s[i]); _farg_fp_s[i] = NULL;}
    _farg_fp_l[i] = 0;
  }
  _farg_fp_i = -1;
}

/* ʸ��������Ĺ�������¸���� */
static void FARG_Save(const char *str) {
  size_t n;

  if (str == NULL) {
    strcpy(_farg_buf, "");
    return;
  }

  if ((n = strlen(str)+1) > _farg_buf_n) {
    /* �ꥵ���� */
    while (n > _farg_buf_n) _farg_buf_n *= 2;
    if ((_farg_buf = (char *) realloc(_farg_buf, _farg_buf_n)) == NULL)
      HeapError();
  }
  strcpy(_farg_buf, str);
}

/* ���ߥ���������Υե�����̾������(��ե����) */
const char *FARG_CurrentFileName(void) {
  return (_farg_fp_i < 0)? NULL: (const char *) _farg_fp_s[_farg_fp_i];
}

/* ���ߥ���������Υե�����ι��ֹ������ */
long FARG_CurrentLine(void) {
  return (_farg_fp_i < 0)? 0: (long) _farg_fp_l[_farg_fp_i];
}

/* (private��) ������1���ɤ߿ʤ��(���ͤϿʤ����ؤΥ�ե����) */
#define FARG_FILEOPT "-f"
static const char *_FARG_Shift(void) {
  int lines, last;
  const char *str;

  for (;;) {
    /* ������1���ɤ� */
    if (_farg_fp_i >= 0) {
      /* �ե�����������ɤ� */
      str = ReadToken(_farg_fp[_farg_fp_i], &lines, &last, NULL);
      _farg_fp_l[_farg_fp_i] += lines;
      if (str == NULL) {
        if (last == EOF) {
          /* �ե�������Ĥ��ƾ�γ��ؤ� */
          fclose(_farg_fp[_farg_fp_i]);
          _farg_fp[_farg_fp_i] = NULL;
          free(_farg_fp_s[_farg_fp_i]);
          _farg_fp_s[_farg_fp_i] = NULL;
          _farg_fp_l[_farg_fp_i] = 0;
          _farg_fp_i--;
          continue;
        } else
          goto error;
      }
    } else {
      /* �°������ɤ� */
      if (_farg_argv[_farg_i] == NULL) {
        /* �ɤ߿Ԥ����� */
        _farg_is_end = 1;
        return NULL;
      }
      str = _farg_argv[_farg_i++];
    }

    /* �ե�����Υ��󥯥롼��Ƚ�� */
    if (strcmp(str, FARG_FILEOPT) == 0) {
      if (_farg_fp_i >= 0) {
        /* �ե�����������ɤ� */
        str = ReadToken(_farg_fp[_farg_fp_i], &lines, &last, NULL);
        _farg_fp_l[_farg_fp_i] += lines;
        if (str == NULL)
          goto error;
      } else {
        /* �°������ɤ� */
        if (_farg_argv[_farg_i] == NULL)
          {last = EOF; goto error;}
        str = _farg_argv[_farg_i++];
      }
      /* �ե�����򳫤��Ʋ��γ��ؤ� */
      if (++_farg_fp_i >= FARG_MAXFILES)
        {fprintf(stderr, "Too many nested include files.\n"); exit(1);}
      if ((_farg_fp[_farg_fp_i] = fopen(str, "rb")) == NULL)
        {fprintf(stderr, "%s: Can't open.(7)\n", str); exit(1);}
      if ((_farg_fp_s[_farg_fp_i] = (char *) malloc(strlen(str)+1)) == NULL)
        HeapError();
      strcpy(_farg_fp_s[_farg_fp_i], str);
      _farg_fp_l[_farg_fp_i] = 0;
      continue;
    }

    break;
  }
  return str;

error:
  /* ���顼��λ */
  if (_farg_fp_i >= 0) {
    fprintf(stderr, "%s, line %ld: ",
                      _farg_fp_s[_farg_fp_i], (long) _farg_fp_l[_farg_fp_i]+1);
  }
  switch (last) {
  case '\\': /* �Хå�����å���Τ��Ȥ����ʤ�EOF�ξ�� */
    fprintf(stderr, "Illegal \\.\n");
    break;
  case '\'':
    fprintf(stderr, "Unmatched \'.\n");
    break;
  case EOF: /* FARG_FILEOPT��ľ�夬EOF�ξ�� */
    fprintf(stderr,"No file name is found.\n");
    break;
  default:
    fprintf(stderr, "Illegal character ");
    if (iscntrl(last))
      fprintf(stderr, "^%c.\n", last+'@');
    else
      fprintf(stderr, "%c.\n", last);
  }
  exit(1);
}

/* �����ΥХå����åפν����(���ͤ������ο���) */
static int FARG_BackupInit(void) {
  const char *str;
  size_t l;

  if ((str = _FARG_Shift()) == NULL) return 0;
  FARG_Save(str);
  l = strlen(_farg_buf)+1;
  if ((_farg_bak = (char *) malloc((l > FARG_MINSTR)? l: FARG_MINSTR)) == NULL)
    HeapError();
  strcpy(_farg_bak, _farg_buf);

  return 1;
}

/* ������1���ɤ�(���ͤϰ����ؤΥ�ե���󥹡����Ԥ�NULL) */
const char *FARG_Read(void) {
  if (_farg_bak == NULL) {
    if (!FARG_BackupInit()) return NULL;
  }
  return (!_farg_is_end)? _farg_buf: NULL;
}

/* ������1���ɤ߿ʤ�롣(���ͤϿʤ����ΰ����ؤΥ�ե���󥹡����Ԥ�NULL) */
const char *FARG_Shift(void) {
  if (_farg_bak == NULL) {
    if (!FARG_BackupInit()) return NULL;
  }
  if (_farg_is_end) return NULL;

  /* �Хå����å� */
  if (strlen(_farg_buf)+1 >= FARG_MINSTR) {
    if ((_farg_bak = (char *) realloc(_farg_bak, strlen(_farg_buf)+1)) == NULL)
      HeapError();
  }
  strcpy(_farg_bak, _farg_buf);

  FARG_Save(_FARG_Shift());

  return _farg_bak;
}
