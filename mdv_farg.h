/* "mdv_farg.h" mdv_farg.cのヘッダファイル */

#ifndef _MDV_FARG_H
#define _MDV_FARG_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

void FARG_Init(const char * const argv[]);
const char *FARG_Read(void);
const char *FARG_Shift(void);
const char *FARG_CurrentFileName(void);
long FARG_CurrentLine(void);

#endif /* _MDV_FARG_H */
