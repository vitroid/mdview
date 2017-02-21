/* "mdv_text.h" mdv_file.*のためのテキスト処理 ヘッダ */

#ifndef _MDV_TEXT_H
#define _MDV_TEXT_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* ファイルからの行の読み込み */
extern int ReadLine(FILE *fp, const char **pstr);

/* ファイルからのトークンの読み込み */
extern const char *ReadToken(FILE *fp, int *plines, int *plast,
  const char **pcomment);

/* 文字列からのトークンの読み込み */
extern const char *SReadToken(const char *str, const char **endp,
  int *plines, int *plast, const char **pcomment);

#endif /* _MDV_TEXT_H */
