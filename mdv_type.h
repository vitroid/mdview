/* "mdv_type.h" mdview���Ѥη��˴ؤ������ */

#ifndef _MDV_TYPE_H_
#define _MDV_TYPE_H_

typedef long MDV_Size;    /* γ�ҿ� */
typedef long MDV_Step;    /* ���ƥå׿� */
typedef int  MDV_Color;   /* ���迧 */
typedef int  AtomID;      /* ����ID */
typedef int  BondShapeID; /* ������ID */
typedef int  BondID;      /* ���ID */
typedef int  CountID;     /* ����ID */
typedef int  ColorID;     /* ��ID */
typedef int  StringID;    /* ʸ����ID */

/* 3������ɸ�� */
typedef struct {
  double x, y, z;
} MDV_3D;

#endif /* _MDV_TYPE_H_ */
