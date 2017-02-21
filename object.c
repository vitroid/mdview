/* object.c: 瞬間構造の描画 */

#include "mdview.h"
#include "mdv_stat.h"

/* ユーザインタフェース側の設定 */
static int ui_center_flag = 0;
static MDV_3D ui_center_coord = {0.0, 0.0, 0.0};

/* 表示関連の設定の読み込み */
void LoadViewParameter(void) {
  MDV_StatusCopy(mdv_status, mdv_status_default);
  distance = mdv_status->distance;
  outline_mode = mdv_status->outline_mode;
  window_size = mdv_status->window_size;
  frames = mdv_status->frames;
  mark_mode = mdv_status->mark_mode;
}

/* 瞬間構造の描画 */
void UpdateViewData(void) {
  /* mdv_statusの更新 */
  MDV_StatusCopy(mdv_status, mdv_status_default);
  if (ui_center_flag)
    mdv_status->center_coord = ui_center_coord;
  mdv_status->distance = distance;
  mdv_status->outline_mode = outline_mode;
  mdv_status->window_size = window_size;
  mdv_status->frames = frames;
  mdv_status->mark_mode = mark_mode;

  /* 描画 */
  ViewData();
}

/* 重心の設定 */
void set_center_of_mass(double x, double y, double z) {
  ui_center_flag = 1;
  ui_center_coord.x = x;
  ui_center_coord.y = y;
  ui_center_coord.z = z;
}

/* 重心の設定解除 */
void unset_center_of_mass(void) {
  ui_center_flag = 0;
}

