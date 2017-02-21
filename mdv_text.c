/* "mdv_text.c" mdv_file.*�Τ���Υƥ����Ƚ��� */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> /* iscntrl() */
#include "mdv_util.h" /* MDV_Array* */
#include "mdv_text.h"

/* �����ѥ�����ꥢ */
static MDV_Array *_text_str = NULL;
static MDV_Array *_text_comment = NULL;

/* IsPlain() */
static int _is_plain[256];
#define IsPlain(c) (_is_plain[c])

/* �ƥ����Ƚ����Τ���ν���� */
#define IsSpecial(c) (iscntrl(c) || strchr("\"()?*<>|&;`!$~",(c)) != NULL)
static void Text_Init(void) {
  int c;

  if (_text_str != NULL)
    return;

  for (c = 0; c < 256; c++) {
    _is_plain[c] = !(IsSpecial(c) || c == ' ' || c == '\t' || c == '\n'
      || c == '\\' || c == '\'' || c == '#' || c == '\0'
      || c == '\x1a' || c == '\r');
  }
  _text_str = MDV_Array_Alloc(1); /* ������: �������ʤ� */
  _text_comment = MDV_Array_Alloc(1);
}

/* �ե����뤫��Υȡ�������ɤ߹��� */
/*
 * (���ͤϥȡ������ؤ����󡢼��Ԥʤ�NULL��*plines�Ͻ��������Կ���
 * *plast�ϺǸ��ʸ��(�ɤ�ʤ��ä���EOF�ʤɤˤʤ�)��*pcomment��
 * �����åפ���������ʸ����(NULL��Ϳ������֤��ʤ�)
 */
const char *ReadToken(FILE *fp, int *plines, int *plast,
    const char **pcomment) {
  int lines = 0;
  int last = EOF;
  int ret = 0;
  int c;

  if (fp == NULL)
    goto term;

  /* ����� */
  Text_Init();
  MDV_Array_SetSize(_text_str, 0);
  MDV_Array_SetSize(_text_comment, 0);

  /* �ե�������ɤ� */
  while (1) {
    switch (c = getc(fp)) {
    case '\n':
      lines++;
      /* continue; */
    case ' ':
    case '\t':
      continue;
    case '\r':
      if ((c = getc(fp)) != '\n') {
        ungetc(c, fp);
        last = '\r';
        goto term;
      }
      lines++;
      continue;
    case '#':
      /* ������ */
      while ((c = getc(fp)) != '\n' && c != EOF && c != '\0' && c != '\x1a') {
        if (c == '\r') {
          if ((c = getc(fp)) == '\n') {
            break;
          } else {
            ungetc(c, fp);
            c = '\r';
          }
        }
        MDV_Array_AppendChar(_text_comment, c);
      }
      if (c == '\0') {
        last = c;
        goto term;
      } else if (c == '\n') {
        MDV_Array_AppendChar(_text_comment, '\n');
        lines++;
      } else if (c == '\x1a') {
        ungetc('\x1a', fp);
        c = EOF;
      } else if (c != EOF) {
        MDV_Array_AppendChar(_text_comment, c);
      }
      continue;
    default:
      ;
    }
    break;
  }
  if (c == '\x1a') {
    ungetc('\x1a', fp);
    c = EOF;
  }
  last = c;
  if (c == EOF)
    goto term;

  /* �ȡ������ȯ������¸ */
  while (1) {
    switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case EOF:
    case '\x1a':
    case '\r':
      break;
    case '\\':
      /* �Хå�����å��� */
      if ((c = getc(fp)) == '\x1a') {
        ungetc('\x1a', fp);
        last = '\\';
        goto term;
      } else if (c == EOF) {
        last = '\\';
        goto term;
      } else if (c == '\0') {
        last = c;
        goto term;
      } else if (c == '\r') {
        if ((c = getc(fp)) != '\n') {
          ungetc(c, fp);
          c = '\r';
        }
      }
      MDV_Array_AppendChar(_text_str, c);
      last = c;
      c = getc(fp);
      continue;
    case '\'':
      /* ���󥰥륯������ */
      while ((c = getc(fp)) != '\'' && c != EOF && c != '\0' && c != '\x1a') {
        if (c == '\r') {
          if ((c = getc(fp)) != '\n') {
            ungetc(c, fp);
            c = '\r';
          }
        }
        MDV_Array_AppendChar(_text_str, c);
      }
      if (c == '\0') {
        last = c;
        goto term;
      } else if (c != '\'') {
        if (c == '\x1a')
          ungetc('\x1a', fp);
        last = '\'';
        goto term;
      }
      last = c;
      c = getc(fp);
      continue;
    case '#':
      /* ������ */
      ungetc(c, fp);
      ret = 1;
      goto term; /* ��������������ޤǤ�ȡ�����Ȥ����֤� */
    default:
      if (IsPlain(c)) {
        /* �̾��ʸ�� */
        do {
          MDV_Array_AppendChar(_text_str, c);
          last = c;
        } while ((c = getc(fp)) != EOF && IsPlain(c));
        continue;
      } else {
        /* �ü��ʸ�� */
        last = c;
        goto term;
      }
    }
    break;
  }
  if (c == '\x1a') {
    ungetc('\x1a', fp);
    c = EOF;
  }
  if (c == '\r') {
    if ((c = getc(fp)) != '\n') {
      ungetc(c, fp);
      last = '\r';
      goto term;
    }
  }
  if (c == '\n')
    lines++;
  last = c;
  ret = 1;

term:
  MDV_Array_AppendChar(_text_str, '\0');
  MDV_Array_AppendChar(_text_comment, '\0');
  *plast = last;
  *plines = lines;
  if (pcomment != NULL)
    *pcomment = (const char *) _text_comment->p;
  return (ret)? (const char *) _text_str->p: NULL;
}

/* ʸ���󤫤�Υȡ�������ɤ߹��� */
/*
 * (���ͤϥȡ������ؤ����󡢼��Ԥʤ�NULL��
 * *endp�Ͻ�����λ���Υݥ��󥿤ΰ��֡�*plines�Ͻ��������Կ���
 * *plast�ϺǸ��ʸ��(�ɤ�ʤ��ä���EOF�ʤɤˤʤ�)��*pcomment��
 * �����åפ���������ʸ����(NULL��Ϳ������֤��ʤ�)
 */
const char *SReadToken(const char *str, const char **endp, int *plines,
    int *plast, const char **pcomment) {
  int lines = 0;
  int last = EOF;
  int ret = 0;
  int c;

  if (str == NULL)
    goto term;

  /* ����� */
  Text_Init();
  MDV_Array_SetSize(_text_str, 0);
  MDV_Array_SetSize(_text_comment, 0);

  /* ʸ������ɤ� */
  while (1) {
    switch (c = *str++) {
    case '\n':
      lines++;
      /* continue; */
    case ' ':
    case '\t':
      continue;
    case '\r':
      if ((c = *str++) != '\n') {
        str--;
        last = '\r';
        goto term;
      }
      lines++;
      continue;
    case '#':
      /* ������ */
      while ((c = *str++) != '\n' && c != EOF && c != '\0' && c != '\x1a') {
        if (c == '\r') {
          if ((c = *str++) == '\n') {
            break;
          } else {
            str--;
            c = '\r';
          }
        }
        MDV_Array_AppendChar(_text_comment, c);
      }
      if (c == '\0') {
        last = c;
        goto term;
      } else if (c == '\n') {
        MDV_Array_AppendChar(_text_comment, '\n');
        lines++;
      } else if (c == '\x1a') {
        str--;
        c = EOF;
      } else if (c != EOF) {
        MDV_Array_AppendChar(_text_comment, c);
      }
      continue;
    default:
      ;
    }
    break;
  }
  if (c == '\x1a') {
    str--;
    c = EOF;
  }
  last = c;
  if (c == EOF)
    goto term;

  /* �ȡ������ȯ������¸ */
  while (1) {
    switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case EOF:
    case '\x1a':
    case '\r':
      break;
    case '\\':
      /* �Хå�����å��� */
      if ((c = *str++) == '\x1a') {
        str--;
        last = '\\';
        goto term;
      } else if (c == EOF) {
        last = '\\';
        goto term;
      } else if (c == '\0') {
        last = c;
        goto term;
      } else if (c == '\r') {
        if ((c = *str++) != '\n') {
          str--;
          c = '\r';
        }
      }
      MDV_Array_AppendChar(_text_str, c);
      last = c;
      c = *str++;
      continue;
    case '\'':
      /* ���󥰥륯������ */
      while ((c = *str++) != '\'' && c != EOF && c != '\0' && c != '\x1a') {
        if (c == '\r') {
          if ((c = *str++) != '\n') {
            str--;
            c = '\r';
          }
        }
        MDV_Array_AppendChar(_text_str, c);
      }
      if (c == '\0') {
        last = c;
        goto term;
      } else if (c != '\'') {
        if (c == '\x1a')
          str--;
        last = '\'';
        goto term;
      }
      last = c;
      c = *str++;
      continue;
    case '#':
      /* ������ */
      str--;
      ret = 1;
      goto term; /* ��������������ޤǤ�ȡ�����Ȥ����֤� */
    default:
      if (IsPlain(c)) {
        /* �̾��ʸ�� */
        do {
          MDV_Array_AppendChar(_text_str, c);
          last = c;
        } while ((c = *str++) != EOF && IsPlain(c));
        continue;
      } else {
        /* �ü��ʸ�� */
        last = c;
        goto term;
      }
    }
    break;
  }
  if (c == '\x1a') {
    str--;
    c = EOF;
  }
  if (c == '\r') {
    if ((c = *str++) != '\n') {
      str--;
      last = '\r';
      goto term;
    }
  }
  if (c == '\n')
    lines++;
  last = c;
  ret = 1;

term:
  MDV_Array_AppendChar(_text_str, '\0');
  MDV_Array_AppendChar(_text_comment, '\0');
  *plast = last;
  *plines = lines;
  if (pcomment != NULL)
    *pcomment = (char *) _text_comment->p;
  if (endp != NULL)
    *endp = str;
  return (ret)? (const char *) _text_str->p: NULL;
}

/* �ե����뤫��ιԤ��ɤ߹��� */
/* (�֤��ͤ��̾�ιԤ��������Ԥ�0�����Ԥ���) */
int ReadLine(FILE *fp, const char **pstr) {
  int ret = 0;
  int c;

  MDV_Array_SetSize(_text_str, 0);
  while (1) {
    switch (c = getc(fp)) {
    case '\0':
    case '\n':
    case EOF:
    case '\x1a':
    case '#':
      break;
    case ' ':
    case '\t':
      /* ����ʸ�� */
      MDV_Array_AppendChar(_text_str, c);
      continue;
    case '\\':
      /* �Хå�����å��� */
      MDV_Array_AppendChar(_text_str, '\\');
      if ((c = getc(fp)) == '\x1a') {
        ungetc('\x1a', fp);
        ret = -1;
        goto term;
      } else if (c == EOF || c == '\0') {
        ret = -1;
        goto term;
      } else if (c == '\r') {
        MDV_Array_AppendChar(_text_str, '\r');
        if ((c = getc(fp)) == '\n')
          MDV_Array_AppendChar(_text_str, '\n');
        else
          ungetc(c, fp);
      } else {
        MDV_Array_AppendChar(_text_str, c);
      }
      ret = 1;
      continue;
    case '\'':
      /* ���󥰥륯������ */
      MDV_Array_AppendChar(_text_str, '\'');
      while ((c = getc(fp)) != '\'' && c != EOF && c != '\x1a' && c != '\0')
        MDV_Array_AppendChar(_text_str, c);
      if (c != '\'') {
        if (c == '\x1a')
          ungetc('\x1a', fp);
        ret = -1;
        goto term;
      }
      MDV_Array_AppendChar(_text_str, '\'');
      ret = 1;
      continue;
    default:
      /* �̾��ʸ�� */
      MDV_Array_AppendChar(_text_str, c);
      ret = 1;
      continue;
    }
    break;
  }

  if (c == '\0') {
    ret = -1;
    goto term;
  } else if (c == '\x1a') {
    ungetc('\x1a', fp);
  } else if (c == EOF) {
    ;
  } else if (c == '\n') {
    MDV_Array_AppendChar(_text_str, '\n');
  } else if (c == '#') {
    MDV_Array_AppendChar(_text_str, '#');
    while ((c = getc(fp)) != '\n' && c != EOF && c != '\x1a' && c != '\0')
      MDV_Array_AppendChar(_text_str, c);
    if (c == '\0') {
      ret = -1;
      goto term;
    } else if (c == '\n') {
      MDV_Array_AppendChar(_text_str, '\n');
    } else if (c == '\x1a') {
      ungetc('\x1a', fp);
    }
    ret = 1;
  } else {
    MDV_Fatal("ReadLine()");
  }

term:
  MDV_Array_AppendChar(_text_str, '\0');
  if (pstr != NULL)
    *pstr = (const char *) _text_str->p;
  return ret;
}

