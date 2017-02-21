/* "mdv_type.h" mdview汎用の型に関する定義 */

#ifndef _MDV_TYPE_H_
#define _MDV_TYPE_H_

typedef long MDV_Size;    /* 粒子数 */
typedef long MDV_Step;    /* ステップ数 */
typedef int  MDV_Color;   /* 描画色 */
typedef int  AtomID;      /* 原子ID */
typedef int  BondShapeID; /* 結合形状ID */
typedef int  BondID;      /* 結合ID */
typedef int  CountID;     /* 結合数ID */
typedef int  ColorID;     /* 色ID */
typedef int  StringID;    /* 文字列ID */

/* 3次元座標型 */
typedef struct {
  double x, y, z;
} MDV_3D;

#endif /* _MDV_TYPE_H_ */
