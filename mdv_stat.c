/* "mdv_stat.c" mdviewステータス管理 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mdv.h"
#include "mdv_util.h"

/* ---- ステータスアクセス関数群 ------------------------------------------- */

MDV_Status *mdv_status = NULL;
MDV_Status *mdv_status_default = NULL;
MDV_Status *mdv_status_tmp = NULL;

/* 暫定的な初期化ルーチン */
static void MDV_Term(void) {
  MDV_StatusFree(mdv_status); mdv_status = NULL;
  MDV_StatusFree(mdv_status_default); mdv_status_default = NULL;
  MDV_StatusFree(mdv_status_tmp); mdv_status_tmp = NULL;
}
void MDV_Init(void) {
  static int init = 0;
  if (!init) {
    if ((mdv_status = MDV_StatusAlloc()) == NULL) HeapError();
    if ((mdv_status_default = MDV_StatusAlloc()) == NULL) HeapError();
    if ((mdv_status_tmp = MDV_StatusAlloc()) == NULL) HeapError();
    atexit(MDV_Term);
    init = 1;
  }
}

/* ---- MDV_Status型 ------------------------------------------------------- */

/* MDV_Status型の確保 */
MDV_Status *MDV_StatusAlloc(void) {
  MDV_Status *p;

  /* 確保 */
  if ((p = (MDV_Status *) malloc(sizeof(MDV_Status))) == NULL)
    return NULL;

  /* default値 */
  MDV_StatusClear(p);

  return p;
}

/* MDV_Status型の解放 */
void MDV_StatusFree(MDV_Status *msp) {
  free(msp);
}

/* MDV_Status型の初期値設定 */
void MDV_StatusClear(MDV_Status *msp) {
  if (msp == NULL) return;

  msp->arg_version = 0; /* v3.00互換 */
  msp->length_unit = 0.529177249; /* atomic unit (per angstrom) */
  msp->max_radius = 0;
  msp->center_auto = 1;
  msp->center_coord.x = 0.0;
  msp->center_coord.y = 0.0;
  msp->center_coord.z = 0.0;
  msp->period_length.x = 0.0;
  msp->period_length.y = 0.0;
  msp->period_length.z = 0.0;
  msp->period_x.x = 1.0;
  msp->period_x.y = 0.0;
  msp->period_x.z = 0.0;
  msp->period_y.x = 0.0;
  msp->period_y.y = 1.0;
  msp->period_y.z = 0.0;
  msp->period_z.x = 0.0;
  msp->period_z.y = 0.0;
  msp->period_z.z = 1.0;
  msp->fold_mode = 0;
  msp->background_color = -1;
  msp->mark_color = -1;
  msp->euler_theta = 0.0;
  msp->euler_psi = 0.0;
  msp->euler_phi = 0.0;
  msp->distance = 10.0;
  msp->outline_mode = 0;
  msp->window_size = 400;
  msp->frames = 0;
  msp->mark_mode = 0;
}

/* MDV_Status型のコピー */
void MDV_StatusCopy(MDV_Status *dst, MDV_Status *src) {
  if (src == NULL || dst == NULL) return;

  dst->arg_version      = src->arg_version;
  dst->length_unit      = src->length_unit;
  dst->max_radius       = src->max_radius;
  dst->center_auto      = src->center_auto;
  dst->center_coord     = src->center_coord;
  dst->period_length    = src->period_length;
  dst->period_x         = src->period_x;
  dst->period_y         = src->period_y;
  dst->period_z         = src->period_z;
  dst->fold_mode        = src->fold_mode;
  dst->background_color = src->background_color;
  dst->mark_color       = src->mark_color;
  dst->euler_theta      = src->euler_theta;
  dst->euler_psi        = src->euler_psi;
  dst->euler_phi        = src->euler_phi;
  dst->distance         = src->distance;
  dst->outline_mode     = src->outline_mode;
  dst->window_size      = src->window_size;
  dst->frames           = src->frames;
  dst->mark_mode        = src->mark_mode;
}

/* 周期境界の設定(立方体) */
void MDV_Status_SetPeriodLength1(MDV_Status *msp, double length) {
  msp->period_length.x = length;
  msp->period_length.y = length;
  msp->period_length.z = length;
  msp->period_x.x = 1.0;
  msp->period_x.y = 0.0;
  msp->period_x.z = 0.0;
  msp->period_y.x = 0.0;
  msp->period_y.y = 1.0;
  msp->period_y.z = 0.0;
  msp->period_z.x = 0.0;
  msp->period_z.y = 0.0;
  msp->period_z.z = 1.0;
}

/* 周期境界の設定(直方体。非表示方向は0以下を指定) */
void MDV_Status_SetPeriodLength3(MDV_Status *msp,
    double x, double y, double z) {
  msp->period_length.x = x;
  msp->period_length.y = y;
  msp->period_length.z = z;
  msp->period_x.x = 1.0;
  msp->period_x.y = 0.0;
  msp->period_x.z = 0.0;
  msp->period_y.x = 0.0;
  msp->period_y.y = 1.0;
  msp->period_y.z = 0.0;
  msp->period_z.x = 0.0;
  msp->period_z.y = 0.0;
  msp->period_z.z = 1.0;
}

/* 周期境界の設定(変形セル。非表示ベクトルは全ての要素に0を指定) */
void MDV_Status_SetPeriodLength9(MDV_Status *msp,
    double xx, double xy, double xz, double yx, double yy, double yz,
    double zx, double zy, double zz) {
  MDV_3D *px, *py, *pz;
  double cth, sth, cps, sps;
  double det;
  int flag;

  /* 読み込み */
  msp->period_length.x = sqrt(xx*xx + xy*xy + xz*xz);
  if (msp->period_length.x > 0.0) {
    msp->period_x.x = xx / msp->period_length.x;
    msp->period_x.y = xy / msp->period_length.x;
    msp->period_x.z = xz / msp->period_length.x;
  }
  msp->period_length.y = sqrt(yx*yx + yy*yy + yz*yz);
  if (msp->period_length.y > 0.0) {
    msp->period_y.x = yx / msp->period_length.y;
    msp->period_y.y = yy / msp->period_length.y;
    msp->period_y.z = yz / msp->period_length.y;
  }
  msp->period_length.z = sqrt(zx*zx + zy*zy + zz*zz);
  if (msp->period_length.z > 0.0) {
    msp->period_z.x = zx / msp->period_length.z;
    msp->period_z.y = zy / msp->period_length.z;
    msp->period_z.z = zz / msp->period_length.z;
  }

  /* 最適化 */
  flag = 0;
  if (msp->period_length.x > 0.0) flag++;
  if (msp->period_length.y > 0.0) flag++;
  if (msp->period_length.z > 0.0) flag++;
  switch (flag) {
  case 0:
    /* 非表示 */
    msp->period_x.x = 1.0;
    msp->period_x.y = 0.0;
    msp->period_x.z = 0.0;
    msp->period_y.x = 0.0;
    msp->period_y.y = 1.0;
    msp->period_y.z = 0.0;
    msp->period_z.x = 0.0;
    msp->period_z.y = 0.0;
    msp->period_z.z = 1.0;
    break;
  case 1:
    /* １つの軸表示 */
    if (msp->period_length.x > 0.0) {
      pz = &(msp->period_x);
      px = &(msp->period_y);
      py = &(msp->period_z);
    } else if (msp->period_length.y > 0.0) {
      py = &(msp->period_x);
      pz = &(msp->period_y);
      px = &(msp->period_z);
    } else {
      px = &(msp->period_x);
      py = &(msp->period_y);
      pz = &(msp->period_z);
    }
    cth = pz->z;
    sth = sin(acos(cth));
    if (fabs(sth) <= 1.0e-5) {
      /* そのまま利用 */
      px->x = 1.0;
      px->y = 0.0;
      px->z = 0.0;
      py->x = 0.0;
      py->y = 1.0;
      py->z = 0.0;
    } else {
      /* z軸に合わせる */
      cps = pz->y / sth;
      sps = pz->x / sth;
      px->x = cps;
      px->y =-sps;
      px->z = 0.0;
      py->x = cth*sps;
      py->y = cth*cps;
      py->z =-sth;
    }
    break;
  case 2:
    /* ２つの軸表示 */
    if (msp->period_length.x <= 0.0) {
      pz = &(msp->period_x);
      px = &(msp->period_y);
      py = &(msp->period_z);
    } else if (msp->period_length.y <= 0.0) {
      py = &(msp->period_x);
      pz = &(msp->period_y);
      px = &(msp->period_z);
    } else {
      px = &(msp->period_x);
      py = &(msp->period_y);
      pz = &(msp->period_z);
    }
    pz->x = px->y * py->z -px->z * py->y;
    pz->y = px->z * py->x -px->x * py->z;
    pz->z = px->x * py->y -px->y * py->x;
    break;
  case 3:
    /* 全ての軸表示 */
    break;
  }

  /* 行列式のチェック */
  det = msp->period_x.x * (msp->period_y.y * msp->period_z.z
                          -msp->period_y.z * msp->period_z.y)
       +msp->period_x.y * (msp->period_y.z * msp->period_z.x
                          -msp->period_y.x * msp->period_z.z)
       +msp->period_x.z * (msp->period_y.x * msp->period_z.y
                          -msp->period_y.y * msp->period_z.x);
  if (det == 0.0)
    {fprintf(stderr, "Illegal periodic boundary condition.\n"); exit(1);}
}

