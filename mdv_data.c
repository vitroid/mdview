/* mdv_data.c */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mdview.h"
#include "mdv.h"
#include "mdv_file.h"
#include "mdv_data.h"
#include "mdv_util.h" /* MDV_Work_Alloc(), MDV_Work_Free() */

/* 外部変数群 (for MD data) */
int md_init = 0;
MDV_Coord *md_coord = NULL;
MDV_Coord *md_coord_org = NULL;
MDV_Coord *md_coord_cache = NULL;
MDV_LineList *md_linelist = NULL;
MDV_LineList *md_linelist_cache = NULL;
MDV_Order *md_order = NULL;
MDV_Line  *md_line = NULL;

/* データ処理、変換 */
void MDInit(void);
int  MDV_FileRead(MDV_FILE *mfp, MDV_Step step);
void MakeLines(const MDV_3D *coord, const MDV_LineData *linelist,
  MDV_Size line_n, LineData *line, MDV_Size n);
void SortData(const MDV_3D *coord, MDV_Size *order, MDV_Size n);
int  GetCoordinates(MDV_Size num, double *px, double *py, double *pz);

/* 座標変換関係 */
void SetCenterMass(void);
void GetCenterMass(double *px, double *py, double *pz);
void ShiftToCenterMass(MDV_3D *coord, MDV_Size n);
void Periodic(MDV_3D *coord, MDV_Size n, const MDV_3D *center);
void RotateData(MDV_3D *coord, MDV_Size n);
void SetEulerAngle(double theta, double psi, double phi);
void GetEulerAngle(double *theta, double *psi, double *phi);
void RotateMatrixX(double phi);
void RotateMatrixY(double phi);
void RotateMatrixZ(double phi);

/* MDV_Coord関係 */
void MDV_CoordCopy(MDV_Coord *dst, const MDV_Coord *src);
int  MDV_CoordCompare(const MDV_Coord *c1 , const MDV_Coord *c2);

/* MDV_3D関係 */
#define inner_product(a, b) ((a).x*(b).x + (a).y*(b).y + (a).z*(b).z)

/* ---- データの処理、変換 ------------------------------------------------- */

#define MAXPOS  65536      /* 結合計算用の領域分割数の上限 */
#define MAXCELL (MAXPOS/9) /* 一次元の分割数の上限 */

static double md_max_length2 = -1.0;

/* MD dataの領域開放 */
static void MDTerm(void) {
  if (!md_init) return;
  MDV_CoordFree(md_coord); md_coord = NULL;
  MDV_CoordFree(md_coord_org); md_coord_org = NULL;
  MDV_CoordFree(md_coord_cache); md_coord_cache = NULL;
  MDV_LineListFree(md_linelist); md_linelist = NULL;
  MDV_LineListFree(md_linelist_cache); md_linelist_cache = NULL;
  MDV_OrderFree(md_order); md_order = NULL;
  MDV_LineFree(md_line); md_line = NULL;
}

/* MD data用配列の確保 */
void MDInit(void) {
  AtomID atom_list_n;
  BondID bond_list_n;
  const BondType *bond_list;
  MDV_Size n;
  BondID bi;

  if (md_init) return;

  n = MDV_VAtomIDGetSize(md_atom); /* 0でも問題ない */
  atom_list_n = MDV_Atom_GetAtomListSize(mdv_atom);
  bond_list_n = MDV_Atom_GetBondListSize(mdv_atom);
  bond_list = MDV_Atom_GetBondList(mdv_atom);

  md_coord = MDV_CoordAlloc();
  md_coord_org = MDV_CoordAlloc();
  md_coord_cache = MDV_CoordAlloc();
  md_linelist = MDV_LineListAlloc();
  md_linelist_cache = MDV_LineListAlloc();
  md_order = MDV_OrderAlloc();
  md_line = MDV_LineAlloc();
  MDV_CoordSetSize(md_coord, n);
  MDV_CoordSetSize(md_coord_org, n);
  MDV_CoordSetSize(md_coord_cache, n);
  MDV_LineListSetSize(md_linelist, 0);
  MDV_LineListSetSize(md_linelist_cache, 0);
  MDV_OrderSetSize(md_order, 2*n);
  MDV_LineSetSize(md_line, n);

  MarkInit(mdv_status->mark_mode);
  MarkClear();

  /* 角度の初期化 */
  SetEulerAngle(mdv_status->euler_theta*TWO_PI/360.0,
                mdv_status->euler_psi  *TWO_PI/360.0,
                mdv_status->euler_phi  *TWO_PI/360.0);

  /* 最大結合長の設定 */
  md_max_length2 = -1.0;
  for (bi = 0; bi < bond_list_n; bi++) {
    if (bond_list[bi].length2 > md_max_length2)
      md_max_length2 = bond_list[bi].length2;
  }

  md_init = 1;
  AtExit(MDTerm);
}

int MDV_FileRead(MDV_FILE *mfp, MDV_Step step) {
  return _MDV_FileRead(mfp, md_coord_org, md_linelist, step);
}

/* ---- MakeLines() -------------------------------------------------------- */

/* 結合数の計算 */
static void MakeCount(LineData *line, MDV_Size n) {
  const AtomID *atom;
  const AtomType *atom_list;
  const BondShapeType *bondshape_list;
  const CountType *cp;
  StringID si, sij;
  MDV_Size i, j;

  atom = MDV_VAtomIDP(md_atom);
  atom_list = MDV_Atom_GetAtomList(mdv_atom);
  bondshape_list = MDV_Atom_GetBondShapeList(mdv_atom);

  for (i = 0; i < n; i++) {
    line[i].count = 0;
    for (j = 0; j < line[i].n; j++) {
      si = atom_list[atom[i]].si;
      sij = bondshape_list[line[i].bsi[j]].si;
      if ((cp = MDV_Atom_SearchCount(mdv_atom, si, sij)) == NULL)
        continue;
      line[i].count += cp->count;
    }
  }
}

/* 周期境界のa軸方向のセル幅(の比率)を求める */
static double cell_width(const MDV_3D *a, const MDV_3D *b, const MDV_3D *c) {
  MDV_3D v, h;
  double coef;

  /* 水平成分 */
  h = *a;
  coef = sqrt(h.x*h.x + h.y*h.y + h.z*h.z);
  if (coef <= 0.0)
    MDV_Fatal("cell_width(): Illegal argument(s) detected.");
  h.x /= coef;
  h.y /= coef;
  h.z /= coef;

  /* 垂直成分の外積方向 */
  v.x = b->y*c->z - b->z*c->y;
  v.y = b->z*c->x - b->x*c->z;
  v.z = b->x*c->y - b->y*c->x;
  coef = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
  if (coef <= 0.0)
    MDV_Fatal("cell_width(): Illegal argument(s) detected.");
  v.x /= coef;
  v.y /= coef;
  v.z /= coef;

  return inner_product(h, v);
}

/* 逆行列の転置を求める */
static void trans_inverse(MDV_3D *rx, MDV_3D *ry, MDV_3D *rz,
                        const MDV_3D *px, const MDV_3D *py, const MDV_3D *pz) {
  double det;

  det = (px->x*(py->y*pz->z - py->z*pz->y)
        +px->y*(py->z*pz->x - py->x*pz->z)
        +px->z*(py->x*pz->y - py->y*pz->x));
  if (det <= 0.0)
    MDV_Fatal("trans_inverse: Illegal argument(s) detected.");
  rx->x = (py->y*pz->z - py->z*pz->y) / det;
  rx->y = (py->z*pz->x - py->x*pz->z) / det;
  rx->z = (py->x*pz->y - py->y*pz->x) / det;
  ry->x = (pz->y*px->z - pz->z*px->y) / det;
  ry->y = (pz->z*px->x - pz->x*px->z) / det;
  ry->z = (pz->x*px->y - pz->y*px->x) / det;
  rz->x = (px->y*py->z - px->z*py->y) / det;
  rz->y = (px->z*py->x - px->x*py->z) / det;
  rz->z = (px->x*py->y - px->y*py->x) / det;
}

/* 結合計算用ワークエリア */
#define NCELLTABLE 2       /* cell_table[]のハッシュ領域の比率 */
#define MAXNEIGHBOR 14
static int pos_diff[MAXNEIGHBOR];        /* 隣接セル位置差分の設定 */
#define MAX_PERIOD_ID (3*3*3)
static int id_pair[MAX_PERIOD_ID];       /* セル位置のペアIDの設定 */
static int cell_diff[MAX_PERIOD_ID];     /* 周期境界セル位置差分 */
static MDV_3D coord_diff[MAX_PERIOD_ID]; /* 座標差分の設定 */

/* 結合の生成 */
#define my_fmod(x, y) ((x)-floor((x)/(y))*(y)) /* yが正の時正しい */
void _MakeLines(const MDV_3D *coord, LineData *line, MDV_Size n) {
  MDV_3D *coord_fold;   /* 折り畳み後の座標 */
  MDV_Size *atom_i,     /* 原子順登録位置 -> 座標格納位置   */
           *atom_n,     /* 原子           -> 原子順登録終端 */
           *cell_i,     /* セル順登録位置 -> 座標格納位置   */
           *cell_table; /* 原子,セル位置  -> cell_i[]位置   */
  int *pos,             /* 座標格納位置   -> セル位置       */
      *cell_id;         /* 周期境界用、セル周期ID */
  const AtomID *atom;
  AtomID atom_list_n;
  const BondType *bond_list;
  AtomPairID atompair_list_n;
  const AtomPairType *atompair_list;
  unsigned long hash_div; /* 32bit以上の整数 */
  MDV_3D ex, ey, ez, rx, ry, rz, tx, ty, tz;
  MDV_3D period_x, period_y, period_z;
  double period_lx, period_ly, period_lz, period_hx, period_hy, period_hz;
  double xmin, ymin, zmin, xmax, ymax, zmax, xlen, ylen, zlen;
  double vx, vy, vz, cx, cy, cz;
  int ncell_x, ncell_y, ncell_z, maxcell_x, maxcell_xy, maxcell_xyz,
    nx, ny, nz, k;
  int pos_i, pos_j, id;
  AtomID ai, aj;
  AtomPairID api;
  MDV_Size i, j, ni, nj;

  if (n <= 0)
    return;

  if (md_max_length2 <= 0.0)
    return; /* 結合は起きない */

  /* 原子情報の取得 */
  atom = MDV_VAtomIDP(md_atom);
  atom_list_n = MDV_Atom_GetAtomListSize(mdv_atom);
  bond_list = MDV_Atom_GetBondList(mdv_atom);
  atompair_list_n = MDV_Atom_GetAtomPairListSize(mdv_atom);
  atompair_list = MDV_Atom_GetAtomPairList(mdv_atom);

  /* ワークエリアの確保 */
  coord_fold = (MDV_3D *) MDV_Work_Alloc(n * sizeof(MDV_3D));
  atom_i = (MDV_Size *) MDV_Work_Alloc(n * sizeof(MDV_Size));
  atom_n = (MDV_Size *) MDV_Work_Alloc(atom_list_n * sizeof(MDV_Size));
  cell_i = (MDV_Size *) MDV_Work_Alloc(n * sizeof(MDV_Size));
  cell_table = (MDV_Size *) MDV_Work_Alloc(NCELLTABLE*n * sizeof(MDV_Size));
  pos = (int *) MDV_Work_Alloc(n * sizeof(int));
  cell_id = (int *) MDV_Work_Alloc(MAXPOS * sizeof(int));

  /* セル幅 */
  period_lx = mdv_status->period_length.x;
  period_ly = mdv_status->period_length.y;
  period_lz = mdv_status->period_length.z;

  /* セル単位ベクトル */
  ex = mdv_status->period_x;
  ey = mdv_status->period_y;
  ez = mdv_status->period_z;

  /* セル単位ベクトルの逆行列(の転置) */
  trans_inverse(&rx, &ry, &rz, &ex, &ey, &ez);

  /* セル全長の設定 */
  xmin = xmax = inner_product(coord[0], rx);
  ymin = ymax = inner_product(coord[0], ry);
  zmin = zmax = inner_product(coord[0], rz);
  for (i = 1; i < n; i++) {
    vx = inner_product(coord[i], rx);
    vy = inner_product(coord[i], ry);
    vz = inner_product(coord[i], rz);
    if (vx < xmin) xmin = vx;
    if (vx > xmax) xmax = vx;
    if (vy < ymin) ymin = vy;
    if (vy > ymax) ymax = vy;
    if (vz < zmin) zmin = vz;
    if (vz > zmax) zmax = vz;
  }
  if (period_lx > 0.0) {
    xmin = 0.0;
    xmax = period_lx;
  }
  if (period_ly > 0.0) {
    ymin = 0.0;
    ymax = period_ly;
  }
  if (period_lz > 0.0) {
    zmin = 0.0;
    zmax = period_lz;
  }

  /* セル分割数の設定 */
  cx = cell_width(&ex, &ey, &ez);
  cy = cell_width(&ey, &ez, &ex);
  cz = cell_width(&ez, &ex, &ey);
  if (xmax-xmin > (MAXCELL-2)*sqrt(md_max_length2)/cx)
    ncell_x = MAXCELL-2;
  else
    ncell_x = (int) ((xmax-xmin) / (sqrt(md_max_length2)/cx));
  if (ymax-ymin > (MAXCELL-2)*sqrt(md_max_length2)/cy)
    ncell_y = MAXCELL-2;
  else
    ncell_y = (int) ((ymax-ymin) / (sqrt(md_max_length2)/cy));
  if (zmax-zmin > (MAXCELL-2)*sqrt(md_max_length2)/cz)
    ncell_z = MAXCELL-2;
  else
    ncell_z = (int) ((zmax-zmin) / (sqrt(md_max_length2)/cz));
  if (ncell_x <= 0) ncell_x = 1;
  if (ncell_y <= 0) ncell_y = 1;
  if (ncell_z <= 0) ncell_z = 1;
  if (debug_mode)
    fprintf(stderr, "(%2d,%2d,%2d)\n", ncell_x, ncell_y, ncell_z);

  /* セル分割数の調節 */
  ncell_x += 2;
  ncell_y += 2;
  ncell_z += 2;
  if ((double) MAXPOS < ((double) ncell_x)*ncell_y*ncell_z) {
    double coef;

    coef = pow(MAXPOS/(((double) ncell_x)*ncell_y*ncell_z), 1.0/3.0);
    ncell_x = (int) (ncell_x * coef + 1.0);
    ncell_y = (int) (ncell_y * coef + 1.0);
    ncell_z = (int) (ncell_z * coef + 1.0);
  }
  while ((double) MAXPOS < ((double) ncell_x)*ncell_y*ncell_z) {
    if (debug_mode)
      fprintf(stderr, "(%2d,%2d,%2d)\n", ncell_x-2, ncell_y-2, ncell_z-2);
    vx = sqrt(md_max_length2)/cx * (ncell_x-3);
    vy = sqrt(md_max_length2)/cy * (ncell_x-3);
    vz = sqrt(md_max_length2)/cz * (ncell_x-3);
    if (vx*(ymax-ymin) > vy*(xmax-xmin)) {
      if (vx*(zmax-zmin) > vz*(xmax-xmin))
        ncell_x--;
      else
        ncell_z--;
    } else {
      if (vy*(zmax-zmin) > vz*(ymax-ymin))
        ncell_y--;
      else
        ncell_z--;
    }
  }
  ncell_x -= 2;
  ncell_y -= 2;
  ncell_z -= 2;
  xlen = (xmax-xmin) / (double) ncell_x;
  ylen = (ymax-ymin) / (double) ncell_y;
  zlen = (zmax-zmin) / (double) ncell_z;
  if (debug_mode)
    fprintf(stderr, "cell size = (%2d,%2d,%2d)\n", ncell_x, ncell_y, ncell_z);

  /* IDのペアIDの設定 */
  for (i = 0; i < MAX_PERIOD_ID; i++) {
    k = 0;
    if (i    %3 == 1) k += 2;
    if (i    %3 == 2) k += 1;
    if ((i/3)%3 == 1) k += 6;
    if ((i/3)%3 == 2) k += 3;
    if ((i/9)%3 == 1) k += 18;
    if ((i/9)%3 == 2) k += 9;
    id_pair[i] = k;
  }

  /* 隣接セル位置差分の設定 */
  maxcell_x = ncell_x+2;
  maxcell_xy = (ncell_y+2)*maxcell_x;
  maxcell_xyz = (ncell_z+2)*maxcell_xy;
  pos_diff[0] = 0;
  pos_diff[1] = 1;
  pos_diff[2] =-1+maxcell_x;
  pos_diff[3] =   maxcell_x;
  pos_diff[4] = 1+maxcell_x;
  pos_diff[5] =-1-maxcell_x+maxcell_xy;
  pos_diff[6] =  -maxcell_x+maxcell_xy;
  pos_diff[7] = 1-maxcell_x+maxcell_xy;
  pos_diff[8] =-1          +maxcell_xy;
  pos_diff[9] =             maxcell_xy;
  pos_diff[10]= 1          +maxcell_xy;
  pos_diff[11]=-1+maxcell_x+maxcell_xy;
  pos_diff[12]=   maxcell_x+maxcell_xy;
  pos_diff[13]= 1+maxcell_x+maxcell_xy;

  /* 周期境界セル位置差分の設定 */
  for (i = 0; i < MAX_PERIOD_ID; i++) {
    k = 0;
    if (i    %3 == 1) k -= ncell_x;
    if (i    %3 == 2) k += ncell_x;
    if ((i/3)%3 == 1) k -= ncell_y*maxcell_x;
    if ((i/3)%3 == 2) k += ncell_y*maxcell_x;
    if ((i/9)%3 == 1) k -= ncell_z*maxcell_xy;
    if ((i/9)%3 == 2) k += ncell_z*maxcell_xy;
    cell_diff[i] = k;
  }

  /* 座標差分の設定 */
  period_x = ex;
  period_x.x *= xmax-xmin;
  period_x.y *= xmax-xmin;
  period_x.z *= xmax-xmin;
  period_y = ey;
  period_y.x *= ymax-ymin;
  period_y.y *= ymax-ymin;
  period_y.z *= ymax-ymin;
  period_z = ez;
  period_z.x *= zmax-zmin;
  period_z.y *= zmax-zmin;
  period_z.z *= zmax-zmin;
  for (i = 0; i < MAX_PERIOD_ID; i++) {
    coord_diff[i].x = 0.0;
    coord_diff[i].y = 0.0;
    coord_diff[i].z = 0.0;
    if (i % 3 == 1) {
      coord_diff[i].x += period_x.x;
      coord_diff[i].y += period_x.y;
      coord_diff[i].z += period_x.z;
    } else if (i % 3 == 2) {
      coord_diff[i].x -= period_x.x;
      coord_diff[i].y -= period_x.y;
      coord_diff[i].z -= period_x.z;
    }
    if ((i/3) % 3 == 1) {
      coord_diff[i].x += period_y.x;
      coord_diff[i].y += period_y.y;
      coord_diff[i].z += period_y.z;
    } else if ((i/3) % 3 == 2) {
      coord_diff[i].x -= period_y.x;
      coord_diff[i].y -= period_y.y;
      coord_diff[i].z -= period_y.z;
    }
    if ((i/9) % 3 == 1) {
      coord_diff[i].x += period_z.x;
      coord_diff[i].y += period_z.y;
      coord_diff[i].z += period_z.z;
    } else if ((i/9) % 3 == 2) {
      coord_diff[i].x -= period_z.x;
      coord_diff[i].y -= period_z.y;
      coord_diff[i].z -= period_z.z;
    }
  }

  /* 座標をセル空間(0以上セル幅未満)に折り畳む */
  for (i = 0; i < n; i++) {
    vx = inner_product(coord[i], rx);
    vy = inner_product(coord[i], ry);
    vz = inner_product(coord[i], rz);
    if (period_lx > 0.0)
      coord_fold[i].x = my_fmod(vx, period_lx);
    else
      coord_fold[i].x = vx - xmin;
    if (period_ly > 0.0)
      coord_fold[i].y = my_fmod(vy, period_ly);
    else
      coord_fold[i].y = vy - ymin;
    if (period_lz > 0.0)
      coord_fold[i].z = my_fmod(vz, period_lz);
    else
      coord_fold[i].z = vz - zmin;
  }

  /* 所属セル番号表pos[]の設定 */
  for (i = 0; i < n; i++) {
    k = (coord_fold[i].x >= ncell_x*xlen)? ncell_x-1: coord_fold[i].x / xlen;
    if (k >= ncell_x) k = ncell_x-1;
    if (k < 0) k = 0;
    pos[i] = k+1;
    k = (coord_fold[i].y >= ncell_y*ylen)? ncell_y-1: coord_fold[i].y / ylen;
    if (k >= ncell_y) k = ncell_y-1;
    if (k < 0) k = 0;
    pos[i] += (k+1) * maxcell_x;
    k = (coord_fold[i].z >= ncell_z*zlen)? ncell_z-1: coord_fold[i].z / zlen;
    if (k >= ncell_z) k = ncell_z-1;
    if (k < 0) k = 0;
    pos[i] += (k+1) * maxcell_xy;
  }

  /* セルから実空間への変換行列 */
  tx.x = ex.x;
  tx.y = ey.x;
  tx.z = ez.x;
  ty.x = ex.y;
  ty.y = ey.y;
  ty.z = ez.y;
  tz.x = ex.z;
  tz.y = ey.z;
  tz.z = ez.z;

  /* 折り畳んだ座標を実験室系(原点を保存)に戻す */
  for (i = 0; i < n; i++) {
    vx = inner_product(coord_fold[i], tx);
    vy = inner_product(coord_fold[i], ty);
    vz = inner_product(coord_fold[i], tz);
    coord_fold[i].x = vx;
    coord_fold[i].y = vy;
    coord_fold[i].z = vz;
  }

  /* 周期境界セルIDの設定 */
  for (nz = 0; nz < ncell_z+2; nz++) {
    for (ny = 0; ny < ncell_y+2; ny++) {
      for (nx = 0; nx < ncell_x+2; nx++) {
        pos_i = nx + ny*maxcell_x + nz*maxcell_xy;
        id = 0;
        if (period_lx > 0.0) {
          if (nx <= 0)         id += 2;
          if (nx >= ncell_x+1) id += 1;
        }
        if (period_ly > 0.0) {
          if (ny <= 0)         id += 6;
          if (ny >= ncell_y+1) id += 3;
        }
        if (period_lz > 0.0) {
          if (nz <= 0)         id += 18;
          if (nz >= ncell_z+1) id += 9;
        }
        cell_id[pos_i] = id;
      }
    }
  }

  /* atom_n[]の設定 */
  for (ai = 0; ai < atom_list_n; ai++)
    atom_n[ai] = 0;
  for (i = 0; i < n; i++)
    atom_n[atom[i]]++;
  for (ai = 1; ai < atom_list_n; ai++)
    atom_n[ai] += atom_n[ai-1]; /* 密度->分布の変換 */
  if (atom_n[atom_list_n-1] != n)
    MDV_Fatal("MakeLines(): Illegal atom[] format.");

  /* atom_i[]の設定(分布数えソート) */
  for (i = n-1; i >= 0; i--)
    atom_i[--atom_n[atom[i]]] = i;
  for (ai = 0; ai < atom_list_n-1; ai++)
    atom_n[ai] = atom_n[ai+1]; /* 値の再生 */
  atom_n[ai] = n;

  /* cell_i[]の設定 */
  for (i = 0; i < n; i++)
    cell_i[i] = atom_i[i];
  for (ai = 0; ai < atom_list_n; ai++) {
    MDV_Size gap, swapped = 1, t;

    /* comb sort */
    gap = (ai == 0)? atom_n[ai]: atom_n[ai]-atom_n[ai-1];
    while (gap > 1 || swapped) {
      swapped = 0;
      gap = (10*gap + 3) / 13;
      for (ni = (ai == 0)? 0: atom_n[ai-1]; ni < atom_n[ai]-gap; ni++) {
        if (pos[cell_i[ni]] > pos[cell_i[ni+gap]]) {
          t = cell_i[ni];
          cell_i[ni] = cell_i[ni+gap];
          cell_i[ni+gap] = t;
          swapped = 1;
        }
      }
    }
  }

  /* cell_table[]の設定 */
  hash_div = 0xFFFFFFFFUL / (((unsigned long) NCELLTABLE)*n) + 1;
  for (ni = 0; ni < NCELLTABLE*n; ni++)
    cell_table[ni] = -1;
  ni = 0;
  while (ni < n) {
    unsigned long h; /* 32bit以上の整数 */

    i = cell_i[ni];
    ai = atom[i];
    pos_i = pos[i];

    /* (ai, pos_i) -> niを登録 */
    h = (((unsigned long) ai)*1566083941UL
      ^ ((unsigned long) pos_i)*1812433253UL) & 0xFFFFFFFFUL;
    h /= hash_div;
    while (cell_table[h] >= 0)
      h = (h+1) % (NCELLTABLE*n);
    cell_table[h] = ni;

    while (++ni < n && pos[(i = cell_i[ni])] == pos_i && atom[i] == ai)
      ;
  }

  /* 結合の計算 */
  period_hx = period_lx/2.0;
  period_hy = period_ly/2.0;
  period_hz = period_lz/2.0;
  for (api = 0; api < atompair_list_n; api++) {
    double maxr2;

    ai = atompair_list[api].ai;
    aj = atompair_list[api].aj;
    maxr2 = bond_list[atompair_list[api].br].length2;

    nj = (aj == 0)? 0: atom_n[aj-1];
    for (; nj < atom_n[aj]; nj++) {
      j = cell_i[nj];
      pos_j = pos[j];

      for (k = 0; k < MAXNEIGHBOR; k++) {
        pos_i = pos_j + pos_diff[k];
        if (pos_i == pos_j && ai < aj) continue;
        id = cell_id[pos_i];
        pos_i += cell_diff[id];

        if (ai == aj && k == 0 && pos_i == pos_j) {
          ni = nj+1;
        } else {
          unsigned long h; /* 32bit以上の整数 */

          /* (ai, pos_i)を探索 */
          h = (((unsigned long) ai)*1566083941UL
            ^ ((unsigned long) pos_i)*1812433253UL) & 0xFFFFFFFFUL;
          h /= hash_div;
          while ((ni = cell_table[h]) >= 0
              && (pos[(i = cell_i[ni])] != pos_i || atom[i] != ai))
            h = (h+1) % (NCELLTABLE*n);
          if (ni < 0)
            continue; /* 見つからない。スキップする */
        }

        for (; ni < n
            && pos[(i = cell_i[ni])] == pos_i && atom[i] == ai; ni++) {
          double dx, dy, dz, r2;

          if (line[i].n >= BOND_MAX || line[j].n >= BOND_MAX)
            continue;

          /* セル相対位置 */
          dx = coord_fold[i].x + coord_diff[id].x - coord_fold[j].x;
          dy = coord_fold[i].y + coord_diff[id].y - coord_fold[j].y;
          dz = coord_fold[i].z + coord_diff[id].z - coord_fold[j].z;

          /* 結合の判定と生成 */
          if ((r2 = dx*dx+dy*dy+dz*dz) <= maxr2) {
            MDV_3D tmp;
            int id_i, id_j;
            BondID bl, br, bm;

            /* 自己相対位置 */
            dx = coord[i].x - coord[j].x - dx;
            dy = coord[i].y - coord[j].y - dy;
            dz = coord[i].z - coord[j].z - dz;
            tmp.x = dx;
            tmp.y = dy;
            tmp.z = dz;
            vx = inner_product(tmp, rx);
            vy = inner_product(tmp, ry);
            vz = inner_product(tmp, rz);
            id_j = 0;
            if (period_lx > 0.0) {
              if      (vx > period_hx) {vx -= period_lx; id_j += 1;}
              else if (vx <-period_hx) {vx += period_lx; id_j += 2;}
            }
            if (period_ly > 0.0) {
              if      (vy > period_hy) {vy -= period_ly; id_j += 3;}
              else if (vy <-period_hy) {vy += period_ly; id_j += 6;}
            }
            if (period_lz > 0.0) {
              if      (vz > period_hz) {vz -= period_lz; id_j += 9;}
              else if (vz <-period_hz) {vz += period_lz; id_j +=18;}
            }
            id_i = id_pair[id_j];

            /* 結合レベルの判定 */
            bl = atompair_list[api].bl;
            br = atompair_list[api].br;
            while (bl < br) {
              bm = (bl+br)/2;
              if (bond_list[bm].length2 < r2)
                bl = bm+1;
              else
                br = bm;
            }

            /* 結合の登録 */
            line[i].to[line[i].n] = j + id_i*n;
            line[i].r2[line[i].n] = r2;
            line[i].bsi[line[i].n] = bond_list[bl].bsi;
            line[i].n++;
            line[j].to[line[j].n] = i + id_j*n;
            line[j].r2[line[j].n] = r2;
            line[j].bsi[line[j].n] = bond_list[bl].bsi;
            line[j].n++;
          }
        }
      }
    }
  }

  /* ワークエリアの開放 */
  MDV_Work_Free(cell_id);
  MDV_Work_Free(pos);
  MDV_Work_Free(cell_table);
  MDV_Work_Free(cell_i);
  MDV_Work_Free(atom_n);
  MDV_Work_Free(atom_i);
  MDV_Work_Free(coord_fold);

  /* 結合数の計算 */
  MakeCount(line, n);
}

/* LineList型からLineData型への変換 */
static void LineList2LineData(const MDV_3D *coord,
    const MDV_LineData *linelist, MDV_Size line_n, LineData *line,
    MDV_Size n) {
  MDV_3D ex, ey, ez, rx, ry, rz, tx, ty, tz;
  double period_lx, period_ly, period_lz, period_hx, period_hy, period_hz;
  MDV_Size line_i;
  int i, k;

  if (line_n <= 0)
    return;

  period_lx = mdv_status->period_length.x;
  period_ly = mdv_status->period_length.y;
  period_lz = mdv_status->period_length.z;
  period_hx = period_lx/2.0;
  period_hy = period_ly/2.0;
  period_hz = period_lz/2.0;
  ex = mdv_status->period_x;
  ey = mdv_status->period_y;
  ez = mdv_status->period_z;
  trans_inverse(&rx, &ry, &rz, &ex, &ey, &ez);
  tx.x = ex.x;
  tx.y = ey.x;
  tx.z = ez.x;
  ty.x = ex.y;
  ty.y = ey.y;
  ty.z = ez.y;
  tz.x = ex.z;
  tz.y = ey.z;
  tz.z = ez.z;
  for (i = 0; i < MAX_PERIOD_ID; i++) {
    k = 0;
    if (i    %3 == 1) k += 2;
    if (i    %3 == 2) k += 1;
    if ((i/3)%3 == 1) k += 6;
    if ((i/3)%3 == 2) k += 3;
    if ((i/9)%3 == 1) k += 18;
    if ((i/9)%3 == 2) k += 9;
    id_pair[i] = k;
  }

  for (line_i = 0; line_i < line_n; line_i++) {
    MDV_3D tmp;
    double vx, vy, vz, dx, dy, dz, r2;
    MDV_Size i, j;
    int id_i, id_j;

    i = linelist[line_i].i;
    j = linelist[line_i].j;
    if (line[i].n >= BOND_MAX || line[j].n >= BOND_MAX)
      continue;

    /* セル相対位置 */
    tmp.x = coord[i].x - coord[j].x;
    tmp.y = coord[i].y - coord[j].y;
    tmp.z = coord[i].z - coord[j].z;
    vx = inner_product(tmp, rx) + period_hx;
    vy = inner_product(tmp, ry) + period_hy;
    vz = inner_product(tmp, rz) + period_hz;
    if (period_lx > 0.0) vx = my_fmod(vx, period_lx);
    if (period_ly > 0.0) vy = my_fmod(vy, period_ly);
    if (period_lz > 0.0) vz = my_fmod(vz, period_lz);
    tmp.x = vx - period_hx;
    tmp.y = vy - period_hy;
    tmp.z = vz - period_hz;
    dx = inner_product(tmp, tx);
    dy = inner_product(tmp, ty);
    dz = inner_product(tmp, tz);
    r2 = dx*dx + dy*dy + dz*dz;

    /* 自己相対位置 */
    tmp.x = coord[i].x - coord[j].x - dx;
    tmp.y = coord[i].y - coord[j].y - dy;
    tmp.z = coord[i].z - coord[j].z - dz;
    vx = inner_product(tmp, rx);
    vy = inner_product(tmp, ry);
    vz = inner_product(tmp, rz);
    id_j = 0;
    if (period_lx > 0.0) {
      if      (vx > period_hx) {vx -= period_lx; id_j += 1;}
      else if (vx <-period_hx) {vx += period_lx; id_j += 2;}
    }
    if (period_ly > 0.0) {
      if      (vy > period_hy) {vy -= period_ly; id_j += 3;}
      else if (vy <-period_hy) {vy += period_ly; id_j += 6;}
    }
    if (period_lz > 0.0) {
      if      (vz > period_hz) {vz -= period_lz; id_j += 9;}
      else if (vz <-period_hz) {vz += period_lz; id_j +=18;}
    }
    id_i = id_pair[id_j];

    /* 結合の登録 */
    line[i].to[line[i].n] = j + id_i*n;
    line[i].r2[line[i].n] = r2;
    line[i].bsi[line[i].n] = linelist[line_i].bsi;
    line[i].n++;
    line[j].to[line[j].n] = i + id_j*n;
    line[j].r2[line[j].n] = r2;
    line[j].bsi[line[j].n] = linelist[line_i].bsi;
    line[j].n++;
  }
}

/* 結合の生成(独立に計算可能な集合ごとに_MakeLines()を実行) */
void MakeLines(const MDV_3D *coord, const MDV_LineData *linelist,
    MDV_Size line_n, LineData *line, MDV_Size n) {
  AtomID atom_list_n;
  const AtomType *atom_list;
  AtomPairID atompair_list_n;
  const AtomPairType *atompair_list;
  MDV_Size *atompair_list_i, *set;
  MDV_Size set_n, set_i;
  AtomID ai, aj, ak;
  AtomPairID api;
  int i;

  /* 原子情報の取得 */
  atom_list_n = MDV_Atom_GetAtomListSize(mdv_atom);
  atom_list = MDV_Atom_GetAtomList(mdv_atom);
  atompair_list_n = MDV_Atom_GetAtomPairListSize(mdv_atom);
  atompair_list = MDV_Atom_GetAtomPairList(mdv_atom);

  /* ワークエリアの確保 */
  atompair_list_i = MDV_Work_Alloc(atom_list_n * sizeof(MDV_Size));
  set = MDV_Work_Alloc(atom_list_n * sizeof(MDV_Size));

  /* atompair_list_i[]の設定 */
  api = 0;
  for (ai = 0; ai < atom_list_n; ai++) {
    for (; api < atompair_list_n && atompair_list[api].ai <= ai; api++)
      ;
    atompair_list_i[ai] = api;
  }

  /* 独立に計算可能な集合を求める */
  for (ai = 0; ai < atom_list_n; ai++)
    set[ai] = -1; /* -1は未定 */
  set_n = 0;
  for (ai = 0; ai < atom_list_n; ai++) {
    if (set[ai] >= 0)
      continue;

    /* stackの初期化 */
    set[ai] = atom_list_n; /* atom_list_nは終端 */
    set_i = ai;

    /* 新規集合の探索 */
    while (set_i < atom_list_n) {
      /* stackから一つ読む */
      aj = set_i;
      set_i = set[aj];

      /* 集合に登録 */
      set[aj] = set_n;

      /* 隣接要素の探索 */
      for (api = (aj == 0)? 0: atompair_list_i[aj-1];
          api < atompair_list_i[aj]; api++) {
        ak = atompair_list[api].aj;
        if (set[ak] < 0) {
          set[ak] = set_i;
          set_i = ak;
        }
      }
    }
    set_n++;
  }

  /* デバッグ出力 */
  if (debug_mode) {
    fprintf(stderr, "--- set information ---\n");
    for (ai = 0; ai < atom_list_n; ai++)
      fprintf(stderr, "%s: %d\n",
        StringID2Str(atom_list[ai].si), (int) set[ai]);
  }

  /* 結合情報のクリア */
  for (i = 0; i < n; i++) {
    line[i].n = 0;
    line[i].count = 0;
  }

  /* line[]の設定 */
  LineList2LineData(coord, linelist, line_n, line, n);

  /* TODO: 集合単位の結合演算を利用できるようにする */

  _MakeLines(coord, line, n);

  /* ワークエリアの開放 */
  MDV_Work_Free(set);
  MDV_Work_Free(atompair_list_i);
}

/* ---- SortData() --------------------------------------------------------- */

static double _Median(double x, double y, double z) {
  if (x >= y) {
    if (y >= z) return y;
    return (z >= x) ? x : z;
  }
  if (y <= z) return y;
  return (z <= x) ? x : z;
}
#define THRESHOLD   11
#define QSORT_STACK 32
static void _SortData(MDV_Size n, const double *a, MDV_Size *index) {
  MDV_Size i, j, left, right, p;
  MDV_Size leftstack[QSORT_STACK], rightstack[QSORT_STACK];
  MDV_Size t;
  double x;

  for (i = 0; i < n; i++)
    index[i] = i;

  left = 0; right = n-1; p = 0;
  for (;;) {
    /*  小さいブロックを無視  */
    if (right-left < THRESHOLD) {
      if (p == 0) break;
      p--;
      left = leftstack[p];
      right = rightstack[p];
    }

    x = _Median(a[index[left]], a[index[(left+right)>>1]], a[index[right]]);

    /*  クイックソート  */
    i = left; j = right;
    for (;;) {
      while (a[index[i]] < x) i++;
      while (x < a[index[j]]) j--;
      if (i >= j) break;
      t = index[i]; index[i] = index[j]; index[j] = t;
      i++; j--;
    }
    if (i-left > right-j) {
      if (right-j > THRESHOLD) {
        leftstack[p] = left;
        rightstack[p] = i-1;
        p++;
        left = j+1;
      } else
        right = i-1;
    } else {
      if (i-left > THRESHOLD) {
        leftstack[p] = j+1;
        rightstack[p] = right;
        p++;
        right = i-1;
      } else
        left = j+1;
    }
  }

  /*  残した部分の挿入ソート  */
  for (i = 1; i < n; i++) {
    t = index[i];
    for (j = i-1; j >= 0 && a[index[j]] > a[t]; j--)
      index[j+1] = index[j];
    index[j+1] = t;
  }
}
void SortData(const MDV_3D *coord, MDV_Size *order, MDV_Size n) {
  MDV_Size i;
  double *work;

  if (n <= 0)
    return;

  work = (double *) MDV_Work_Alloc(sizeof(double)*n);
  for (i = 0; i < n; i++)
    work[i] = coord[i].z;

  _SortData(n, work, order);
  MDV_Work_Free(work);

  /* make inverse table */
  for (i = 0; i < n; i++)
    order[n+order[i]] = i;
}

/* ---- その他 ------------------------------------------------------------- */

/* ファイルに書かれていた粒子の座標を返す */
int GetCoordinates(MDV_Size num, double *px, double *py, double *pz) {
  const MDV_3D *_coord_org;

  if (num < 0 || num >= MDV_CoordGetSize(md_coord_org)) return 0;
  _coord_org = MDV_CoordP(md_coord_org);
  *px = _coord_org[num].x / mdv_status->length_unit;
  *py = _coord_org[num].y / mdv_status->length_unit;
  *pz = _coord_org[num].z / mdv_status->length_unit;
  return 1;
}

/* ==== 座標変換関係 ======================================================= */

static void _SetCenterMass(const MDV_3D *coord, MDV_Size n) {
  const AtomID *atom;
  const AtomType *atom_list;
  MDV_3D *work;
  double cx, cy, cz, dx, dy, dz;
  double total_mass;
  double t, r2;
  MDV_Size i;

  if (n <= 0) {
    mdv_status_default->center_coord.x = 0.0;
    mdv_status_default->center_coord.y = 0.0;
    mdv_status_default->center_coord.z = 0.0;
    return;
  }

  atom = MDV_VAtomIDP(md_atom);
  atom_list = MDV_Atom_GetAtomList(mdv_atom);

  /* center of mass */
  cx = cy = cz = 0.0;
  total_mass = 0.0;

  for (i = 0; i < n; i++) {
    double mass;

    mass = atom_list[atom[i]].mass;
    total_mass += mass;
    cx += mass * coord[i].x;
    cy += mass * coord[i].y;
    cz += mass * coord[i].z;
  }
  if (total_mass == 0.0)
    MDV_Fatal("total mass = 0.");
  cx /= total_mass;
  cy /= total_mass;
  cz /= total_mass;

  if (mdv_status_default->center_auto) {
    mdv_status_default->center_coord.x = cx;
    mdv_status_default->center_coord.y = cy;
    mdv_status_default->center_coord.z = cz;
  }
  if (debug_mode) {
    fprintf(stderr, "center = (%f,%f,%f)\n",
      mdv_status_default->center_coord.x,
      mdv_status_default->center_coord.y,
      mdv_status_default->center_coord.z);
  }

  /* max radius (for the window size) */
  work = (MDV_3D *) MDV_Work_Alloc(sizeof(MDV_3D)*n);
  for (i = 0; i < n; i++)
    work[i] = coord[i];
  if (mdv_status_default->fold_mode
      && (mdv_status_default->period_length.x > 0.0
      || mdv_status_default->period_length.y > 0.0
      || mdv_status_default->period_length.z > 0.0)) {
    Periodic(work, n, &(mdv_status_default->center_coord));
    cx = mdv_status_default->center_coord.x;
    cy = mdv_status_default->center_coord.y;
    cz = mdv_status_default->center_coord.z;
  }
  r2 = 0;
  for (i = 0; i < n; i++) {
    double radius;

    radius = atom_list[atom[i]].radius;
    dx = work[i].x - cx;
    dy = work[i].y - cy;
    dz = work[i].z - cz;
    t = dx*dx + dy*dy + dz*dz + radius*radius;
    if (t > r2) r2 = t;
  }
  if (mdv_status_default->max_radius <= 0.0)
    mdv_status_default->max_radius = sqrt(r2);
  if (debug_mode)
    fprintf(stderr, "max_radius = %f\n", mdv_status_default->max_radius);
  MDV_Work_Free(work);
}
void SetCenterMass(void) {
  _SetCenterMass(MDV_CoordP(md_coord_org), MDV_CoordGetSize(md_coord_org));
}

void GetCenterMass(double *px, double *py, double *pz) {
  *px = mdv_status->center_coord.x;
  *py = mdv_status->center_coord.y;
  *pz = mdv_status->center_coord.z;
}

void ShiftToCenterMass(MDV_3D *coord, MDV_Size n) {
  MDV_Size i;

  for (i = 0; i < n; i++) {
    coord[i].x -= mdv_status->center_coord.x;
    coord[i].y -= mdv_status->center_coord.y;
    coord[i].z -= mdv_status->center_coord.z;
  }
}

/* 周期境界条件の範囲±period_h*に収まるように、centerを中心に折り畳む */
void Periodic(MDV_3D *coord, MDV_Size n, const MDV_3D *center) {
  MDV_3D ex, ey, ez, rx, ry, rz, ci, vi;
  double period_lx, period_ly, period_lz, period_hx, period_hy, period_hz;
  double val;
  MDV_Size i;

  if (n <= 0)
    return;

  period_lx = mdv_status->period_length.x;
  period_ly = mdv_status->period_length.y;
  period_lz = mdv_status->period_length.z;
  period_hx = period_lx/2.0;
  period_hy = period_ly/2.0;
  period_hz = period_lz/2.0;

  ex = mdv_status->period_x;
  ey = mdv_status->period_y;
  ez = mdv_status->period_z;
  trans_inverse(&rx, &ry, &rz, &ex, &ey, &ez);

  for (i = 0; i < n; i++) {
    ci = coord[i];
    ci.x = coord[i].x - center->x;
    ci.y = coord[i].y - center->y;
    ci.z = coord[i].z - center->z;
    vi.x = inner_product(ci, rx);
    vi.y = inner_product(ci, ry);
    vi.z = inner_product(ci, rz);
    if (period_lx > 0.0) {
      val = fmod(fabs(vi.x) + period_hx, period_lx) - period_hx;
      vi.x = (vi.x >= 0)? val: -val;
    }
    if (period_ly > 0.0) {
      val = fmod(fabs(vi.y) + period_hy, period_ly) - period_hy;
      vi.y = (vi.y >= 0)? val: -val;
    }
    if (period_lz > 0.0) {
      val = fmod(fabs(vi.z) + period_hz, period_lz) - period_hz;
      vi.z = (vi.z >= 0)? val: -val;
    }
    ci.x = inner_product(vi, ex);
    ci.y = inner_product(vi, ey);
    ci.z = inner_product(vi, ez);
    coord[i].x = ci.x + center->x;
    coord[i].y = ci.y + center->y;
    coord[i].z = ci.z + center->z;
  }
}

static double _rmatrix[3][3];
void SetEulerAngle(double th, double ps, double ph) {
  double c1, c2, c3, s1, s2, s3;

  c1 = cos(th); s1 = sin(th);
  c2 = cos(ps); s2 = sin(ps);
  c3 = cos(ph); s3 = sin(ph);

  /* z-x-z形式, 実験室系->主軸系変換, α=3, β=1, γ=2 */
  _rmatrix[0][0] =  c2*c3 - c1*s2*s3;
  _rmatrix[0][1] =  c2*s3 + c1*s2*c3;
  _rmatrix[0][2] =  s1*s2;
  _rmatrix[1][0] = -s2*c3 - c1*c2*s3;
  _rmatrix[1][1] = -s2*s3 + c1*c2*c3;
  _rmatrix[1][2] =  s1*c2;
  _rmatrix[2][0] =  s1*s3;
  _rmatrix[2][1] = -s1*c3;
  _rmatrix[2][2] =  c1;
}

#define _acos(x) ((x>1.0)? 0.0: ((x<-1.0)? PI: acos(x)))
void GetEulerAngle(double *pth, double *pps, double *pph) {
  double c1, c2, c3, s1, s2, s3;

  c1 = _rmatrix[2][2];
  s1 = (c1>1.0||c1<-1.0)? 0.0: sqrt(1.0 - c1*c1);
  if (fabs(s1) > 1.0e-5) {
    c2 = _rmatrix[1][2]/s1;
    s2 = _rmatrix[0][2]/s1;
    c3 =-_rmatrix[2][1]/s1;
    s3 = _rmatrix[2][0]/s1;
  } else if (c1 > 0.0) {
    c1 = 1.0;
    s1 = 0.0;
    c2 = _rmatrix[0][0];
    s2 = _rmatrix[0][1];
    c3 = 1.0;
    s3 = 0.0;
  } else {
    c1 =-1.0;
    s1 = 0.0;
    c2 = _rmatrix[0][0];
    s2 =-_rmatrix[0][1];
    c3 = 1.0;
    s3 = 0.0;
  }

  *pth = _acos(c1); if (s1 < 0.0) *pth = TWO_PI - *pth;
  *pps = _acos(c2); if (s2 < 0.0) *pps = TWO_PI - *pps;
  *pph = _acos(c3); if (s3 < 0.0) *pph = TWO_PI - *pph;
}

void RotateMatrixX(double phi) {
  double r10, r11, r12, r20, r21, r22, sp, cp;

  sp = sin(-phi); cp = cos(-phi);
  r10 = _rmatrix[1][0]; r11 = _rmatrix[1][1]; r12 = _rmatrix[1][2];
  r20 = _rmatrix[2][0]; r21 = _rmatrix[2][1]; r22 = _rmatrix[2][2];
  _rmatrix[1][0] = r10*cp - r20*sp;
  _rmatrix[1][1] = r11*cp - r21*sp;
  _rmatrix[1][2] = r12*cp - r22*sp;
  _rmatrix[2][0] = r10*sp + r20*cp;
  _rmatrix[2][1] = r11*sp + r21*cp;
  _rmatrix[2][2] = r12*sp + r22*cp;
}

void RotateMatrixY(double phi) {
  double r00, r01, r02, r20, r21, r22, sp, cp;

  sp = sin(-phi); cp = cos(-phi);
  r00 = _rmatrix[0][0]; r01 = _rmatrix[0][1]; r02 = _rmatrix[0][2];
  r20 = _rmatrix[2][0]; r21 = _rmatrix[2][1]; r22 = _rmatrix[2][2];
  _rmatrix[2][0] = r20*cp - r00*sp;
  _rmatrix[2][1] = r21*cp - r01*sp;
  _rmatrix[2][2] = r22*cp - r02*sp;
  _rmatrix[0][0] = r20*sp + r00*cp;
  _rmatrix[0][1] = r21*sp + r01*cp;
  _rmatrix[0][2] = r22*sp + r02*cp;
}

void RotateMatrixZ(double phi) {
  double r00, r01, r02, r10, r11, r12, sp, cp;

  sp = sin(-phi); cp = cos(-phi);
  r00 = _rmatrix[0][0]; r01 = _rmatrix[0][1]; r02 = _rmatrix[0][2];
  r10 = _rmatrix[1][0]; r11 = _rmatrix[1][1]; r12 = _rmatrix[1][2];
  _rmatrix[0][0] = r00*cp - r10*sp;
  _rmatrix[0][1] = r01*cp - r11*sp;
  _rmatrix[0][2] = r02*cp - r12*sp;
  _rmatrix[1][0] = r00*sp + r10*cp;
  _rmatrix[1][1] = r01*sp + r11*cp;
  _rmatrix[1][2] = r02*sp + r12*cp;
}

#define _InnerProduct3D(a, b) ((a)[0]*(b).x+(a)[1]*(b).y+(a)[2]*(b).z)
void RotateData(MDV_3D *coord, MDV_Size n) {
  MDV_3D c;
  MDV_Size i;

  for (i = 0; i < n; i++) {
    c = coord[i];
    coord[i].x = _InnerProduct3D(_rmatrix[0], c);
    coord[i].y = _InnerProduct3D(_rmatrix[1], c);
    coord[i].z = _InnerProduct3D(_rmatrix[2], c);
  }
}

/* ==== MDV_Coord ========================================================== */

void MDV_CoordCopy(MDV_Coord *dst, const MDV_Coord *src) {
  const MDV_3D *_src;
  MDV_3D *_dst;
  MDV_Size n;
  guint i;

  if (src == NULL || dst == NULL)
    MDV_Fatal("MDV_CoordCopy()");

  n = MDV_CoordGetSize(src);
  MDV_CoordSetSize(dst, n);
  _src = MDV_CoordP(src);
  _dst = MDV_CoordP(dst);

  for (i = 0; i < n; i++)
    _dst[i] = _src[i];
}

static int MDV_3DEqual(const MDV_3D *v1, const MDV_3D *v2) {
  return (v1->x == v2->x && v1->y == v2->y && v1->z == v2->z);
}
static int MDV_3DCompare(const MDV_3D *v1, const MDV_3D *v2) {
  int ret;

  if (v1->x != v2->x) {
    ret = (v1->x > v2->x)? 1: -1;
  } else if (v1->y != v2->y) {
    ret = (v1->y > v2->y)? 1: -1;
  } else if (v1->z != v2->z) {
    ret = (v1->z > v2->z)? 1: -1;
  } else
    ret = 0;

  return ret;
}

int MDV_CoordCompare(const MDV_Coord *c1, const MDV_Coord *c2) {
  MDV_3D *_c1, *_c2;
  MDV_Size n1, n2;
  guint i;

  if (c1 == NULL || c2 == NULL)
    MDV_Fatal("MDV_CoordCopy()");

  n1 = MDV_CoordGetSize(c1);
  n2 = MDV_CoordGetSize(c2);
  _c1 = MDV_CoordP(c1);
  _c2 = MDV_CoordP(c2);

  for (i = 0; i < n1 && i < n2 && MDV_3DEqual(_c1+i, _c2+i); i++)
    ;
  if (i < n1 && i < n2) {
    return MDV_3DCompare(_c1+i, _c2+i);
  }

  return (n1 > n2)? 1: ((n1 < n2)? -1: 0);
}

/* ==== MDV_LineList ======================================================= */

void MDV_LineListCopy(MDV_LineList *dst, const MDV_LineList *src) {
  const MDV_LineData *_src;
  MDV_LineData *_dst;
  MDV_Size n;
  guint i;

  if (src == NULL || dst == NULL)
    MDV_Fatal("MDV_LineListCopy()");

  n = MDV_LineListGetSize(src);
  MDV_LineListSetSize(dst, n);
  _src = MDV_LineListP(src);
  _dst = MDV_LineListP(dst);

  for (i = 0; i < n; i++)
    _dst[i] = _src[i];
}

static int MDV_LineDataEqual(const MDV_LineData *v1, const MDV_LineData *v2) {
  return (v1->bsi == v2->bsi && v1->i == v2->i && v1->j == v2->j);
}
static int MDV_LineDataCompare(const MDV_LineData *v1,
    const MDV_LineData *v2) {
  int ret;

  if (v1->bsi != v2->bsi) {
    ret = (v1->bsi > v2->bsi)? 1: -1;
  } else if (v1->i != v2->i) {
    ret = (v1->i > v2->i)? 1: -1;
  } else if (v1->j != v2->j) {
    ret = (v1->j > v2->j)? 1: -1;
  } else
    ret = 0;

  return ret;
}

int MDV_LineListCompare(const MDV_LineList *c1, const MDV_LineList *c2) {
  MDV_LineData *_c1, *_c2;
  MDV_Size n1, n2;
  guint i;

  if (c1 == NULL || c2 == NULL)
    MDV_Fatal("MDV_LineListCompare()");

  n1 = MDV_LineListGetSize(c1);
  n2 = MDV_LineListGetSize(c2);
  _c1 = MDV_LineListP(c1);
  _c2 = MDV_LineListP(c2);

  for (i = 0; i < n1 && i < n2 && MDV_LineDataEqual(_c1+i, _c2+i); i++)
    ;
  if (i < n1 && i < n2) {
    return MDV_LineDataCompare(_c1+i, _c2+i);
  }

  return (n1 > n2)? 1: ((n1 < n2)? -1: 0);
}

