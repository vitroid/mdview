/* "mdv_file.h" mdv_file.c のためのヘッダファイル */

#ifndef _MDV_FILE_H_
#define _MDV_FILE_H_

#include "mdv_type.h"
#include "mdv.h"
#include "seekfile.h"

/* MDV_FILE型 */
typedef struct {
  SeekFILE *sfp;
  MDV_Size data_n;
  int mode;
} MDV_FILE;

#define MDV_READ_SUCCESS   SEEK_READ_SUCCESS  
#define MDV_READ_UNDERFLOW SEEK_READ_UNDERFLOW
#define MDV_READ_OVERFLOW  SEEK_READ_OVERFLOW 
#define MDV_READ_FAILURE   SEEK_READ_FAILURE  

MDV_FILE *MDV_FileOpen(const char *path, MDV_Size n);
int  MDV_FileReload(MDV_FILE *mfp);
void MDV_FileClose(MDV_FILE *mfp);
int _MDV_FileRead(MDV_FILE *mfp, MDV_Coord *coord, MDV_LineList *linelist,
  MDV_Step step);
int  MDV_SeekTo(MDV_FILE *fp, MDV_Step step);

char *GetInformation(MDV_Step i);

MDV_Step MDV_MaxStep(const MDV_FILE *mfp);
int  MDV_FileRead(MDV_FILE *mfp, MDV_Step step);

#endif /* _MDV_FILE_H_ */
