/* "mdv_stat.h" mdviewステータス管理のためのヘッダファイル */

#ifndef _MDV_STAT_H_
#define _MDV_STAT_H_

#include "mdv_type.h"

typedef struct {
  /* 引数関連 */
  int arg_version;           /* 引数の書式のバージョン(実際はレビジョン) */

  /* mdv_data関連 */
  double length_unit;        /* 長さの単位 */
  double max_radius;         /* 表示可能最大半径 */
  int center_auto;           /* 重心の自動設定フラグ */
  MDV_3D center_coord;       /* 重心の初期値 */
  MDV_3D period_length;      /* 周期境界長 */
  MDV_3D period_x,           /* 周期境界ベクトル */
         period_y,
         period_z;
  int fold_mode;             /* 周期境界への折り畳みのトグル */
  StringID background_color; /* 背景の色 */
  StringID mark_color;       /* markするときの色 */

  /* mdv_data関連追加 */
  double euler_theta;        /* オイラー角 */
  double euler_psi;
  double euler_phi;

  /* 描画関連 */
  double distance;           /* 視点の相対位置 */
  int outline_mode;          /* 輪郭表示 */

  /* UI関連 */
  int window_size;           /* 表示サイズ */
  int frames;                /* 描画速度 */
  int mark_mode;             /* 粒子のマーク方式 */
  /* TODO: mark情報 */
  /* TODO: layer情報 */
} MDV_Status;

extern MDV_Status *mdv_status;         /* 現在のステータス */
extern MDV_Status *mdv_status_default; /* データファイル時の値 */
extern MDV_Status *mdv_status_tmp;     /* テンポラリの値 */

void MDV_Init(void);

MDV_Status *MDV_StatusAlloc(void);
void MDV_StatusFree(MDV_Status *msp);
void MDV_StatusClear(MDV_Status *msp);
void MDV_StatusCopy(MDV_Status *dst, MDV_Status *src);
void MDV_Status_SetPeriodLength1(MDV_Status *msp, double length);
void MDV_Status_SetPeriodLength3(MDV_Status *msp,
  double x, double y, double z);
void MDV_Status_SetPeriodLength9(MDV_Status *msp,
  double xx, double xy, double xz, double yx, double yy, double yz,
  double zx, double zy, double zz);

#endif /* _MDV_STAT_H_ */
