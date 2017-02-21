/* "mdv_data.h" mdv_data.c のためのヘッダファイル */

#ifndef _MDV_DATA_H_
#define _MDV_DATA_H_

#include "mdv.h"

/* 定数に関するマクロ */
#ifndef PI
 #define PI (3.14159265358979323)
#endif
#define TWO_PI (2.0*PI)

/* 結合型 */
#define BOND_MAX 8
typedef struct {
  MDV_Size n;
  MDV_Size to[BOND_MAX];
  BondShapeID bsi[BOND_MAX];
  double r2[BOND_MAX];
  int count;
} LineData;

/* 表示順序型 */
typedef MDV_Array MDV_Order;
#define MDV_OrderP(order)          ((MDV_Size *) (order)->p)
#define MDV_OrderAlloc()           MDV_Array_Alloc(sizeof(MDV_Size))
#define MDV_OrderFree(order)       MDV_Array_Free(order)
#define MDV_OrderGetSize(order)    ((order)->n)
#define MDV_OrderSetSize(order, n) MDV_Array_SetSize((order), (n))

/* 結合リスト型 */
typedef MDV_Array MDV_Line;
#define MDV_LineP(line)          ((LineData *) (line)->p)
#define MDV_LineAlloc()          MDV_Array_Alloc(sizeof(LineData))
#define MDV_LineFree(line)       MDV_Array_Free(line)
#define MDV_LineGetSize(line)    ((line)->n)
#define MDV_LineSetSize(line, n) MDV_Array_SetSize((line), (n))

/* 瞬間構造型 */
extern MDV_Coord *md_coord;
extern MDV_Coord *md_coord_org;
extern MDV_Coord *md_coord_cache;
extern MDV_LineList *md_linelist;
extern MDV_LineList *md_linelist_cache;
extern int md_init;
extern MDV_Order *md_order;
extern MDV_Line *md_line;

/* データ処理、変換 */
extern void MakeLines(const MDV_3D *coord, const MDV_LineData *linelist,
  MDV_Size line_n, LineData *line, MDV_Size n);
extern void SortData(const MDV_3D *coord, MDV_Size *order, MDV_Size n);
extern long BondNumber(long i, const LineData *line);

/* 座標変換関係 */
extern void GetCenterMass(double *px, double *py, double *pz);
extern void ShiftToCenterMass(MDV_3D *coord, MDV_Size n);
extern void Periodic(MDV_3D *coord, MDV_Size n, const MDV_3D *center);
extern void RotateData(MDV_3D *coord, MDV_Size n);

#endif /* _MDV_DATA_H_ */
