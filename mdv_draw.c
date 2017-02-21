/* "mdv_draw.c" 描画関連 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mdview.h"
#include "mdv.h"
#include "mdv_data.h"
#include "hash.h"
#include "chunk.h"

/* 外部変数群 */
DrawList *drawlist = NULL;
int drawlist_n = 0;

/* 定数に関するマクロ */
#define RADIUSRATIO (1.0)

/* データの表示、コンバート関係 */
void ViewInit(void);
void ViewData(void);
void GetParticleCoordinate(MDV_Size i, double *px, double *py);
const char *GetAtomName(MDV_Size i);
MDV_Size WhichParticle(double x, double y);
void ConvertPovray(FILE *fp_pov, FILE *fp_inc, const char *str_inc);

/* 色関係 */
MDV_Color GetColor(StringID si, int lv);
MDV_Color GetBackGroundColor(void);

/* mark関係 */
void MarkInit(int mode);
int  GetMarkMode(void);
int  SetMarkMode(int mode);
void MarkClear(void);
void MarkSet(MDV_Size x);
MDV_Size MarkRead1st(void);
MDV_Size MarkReadNext(MDV_Size x);
int  MarkRead(MDV_Size x);
void MarkWrite(int val, MDV_Size x);
int  MarkPush(void);
int  MarkPop(void);
int  MarkLoad(void);
int  MarkSave(void);

/*-------------------------------画面表示関係--------------------------------*/

typedef MDV_Array MDV_Draw;
#define MDV_DrawP(a)          ((DrawList *) (a)->p)
#define MDV_DrawAlloc()       MDV_Array_Alloc(sizeof(DrawList))
#define MDV_DrawFree(a)       MDV_Array_Free(a)
#define MDV_DrawGetSize(a)    ((a)->n)
#define MDV_DrawSetSize(a, n) MDV_Array_SetSize((a), (n))

/* for ViewInit() */
static int _view_init = 0;
static MDV_Draw *md_draw = NULL;

static void ViewTerm(void) {
  MDV_DrawFree(md_draw);
  md_draw = NULL;
  drawlist = NULL;
  drawlist_n = 0;
}
void ViewInit(void) {
  if (_view_init) return;

  md_draw = MDV_DrawAlloc();
  drawlist = MDV_DrawP(md_draw);
  drawlist_n = MDV_DrawGetSize(md_draw);

  _view_init = 1;
  AtExit(ViewTerm);
}

/* 周期境界条件用 */
#define MAX_PERIOD_ID (3*3*3)
static MDV_3D period_d[MAX_PERIOD_ID];
static void SetPeriodVectors(void) {
  MDV_3D period_x, period_y, period_z;
  double period_lx, period_ly, period_lz;
  int i;

  period_x = mdv_status->period_x;
  period_y = mdv_status->period_y;
  period_z = mdv_status->period_z;
  period_lx = mdv_status->period_length.x;
  period_ly = mdv_status->period_length.y;
  period_lz = mdv_status->period_length.z;
  for (i = 0; i < MAX_PERIOD_ID; i++) {
    period_d[i].x = 0.0;
    period_d[i].y = 0.0;
    period_d[i].z = 0.0;
    if (i % 3 == 1) {
      period_d[i].x -= period_x.x * period_lx;
      period_d[i].y -= period_x.y * period_lx;
      period_d[i].z -= period_x.z * period_lx;
    } else if (i % 3 == 2) {
      period_d[i].x += period_x.x * period_lx;
      period_d[i].y += period_x.y * period_lx;
      period_d[i].z += period_x.z * period_lx;
    }
    if ((i/3) % 3 == 1) {
      period_d[i].x -= period_y.x * period_ly;
      period_d[i].y -= period_y.y * period_ly;
      period_d[i].z -= period_y.z * period_ly;
    } else if ((i/3) % 3 == 2) {
      period_d[i].x += period_y.x * period_ly;
      period_d[i].y += period_y.y * period_ly;
      period_d[i].z += period_y.z * period_ly;
    }
    if ((i/9) % 3 == 1) {
      period_d[i].x -= period_z.x * period_lz;
      period_d[i].y -= period_z.y * period_lz;
      period_d[i].z -= period_z.z * period_lz;
    } else if ((i/9) % 3 == 2) {
      period_d[i].x += period_z.x * period_lz;
      period_d[i].y += period_z.y * period_lz;
      period_d[i].z += period_z.z * period_lz;
    }
  }
}

static void DrawListClear(void) {
  MDV_DrawSetSize(md_draw, 0);
  drawlist = MDV_DrawP(md_draw);
  drawlist_n = MDV_DrawGetSize(md_draw);
}
static void DrawListAdd(int flag, double a, double b,
                        double c, double d, MDV_Color col, MDV_Size i) {
  DrawList v;

  v.flag = flag;
  v.a = a;
  v.b = b;
  v.c = c;
  v.d = d;
  v.col = col;
  v.i = i;

  drawlist_n = MDV_DrawGetSize(md_draw);
  MDV_DrawSetSize(md_draw, drawlist_n+1);
  drawlist = MDV_DrawP(md_draw);
  drawlist_n = MDV_DrawGetSize(md_draw);
  drawlist[drawlist_n-1] = v;
}
/* 描画命令の生成 */
#define DELTA_Z 0.1 /* 表示不可能領域の開始位置。0より大きいこと */
static void _ViewData(const MDV_3D *coord, const MDV_Size *order,
                                            const LineData *line, MDV_Size n) {
  AtomID atom_list_n;
  const AtomType *atom_list;
  const BondShapeType *bondshape_list;
  const AtomID *atom;
  MDV_Size i, j, k;
  MDV_Color color;
  int col_l;
  const AtomType *atomp;
  double _x, _y, _z, _r, r;
  double coef, z0;
  double a, b, c, d;

  atom_list_n = MDV_Atom_GetAtomListSize(mdv_atom);
  atom_list = MDV_Atom_GetAtomList(mdv_atom);
  bondshape_list = MDV_Atom_GetBondShapeList(mdv_atom);
  atom = MDV_VAtomIDP(md_atom);

  coef = 1.0 / (2.0*RADIUSRATIO*mdv_status->max_radius);
  z0 = mdv_status->distance*mdv_status->max_radius;
  DrawListClear();

  /* 周期境界用ベクトルの設定 */
  SetPeriodVectors();
  RotateData(period_d, MAX_PERIOD_ID);

  /* main loop */
  for (k = 0; k < n; k++) {
    double L, R, _z0, tmp;

    i = order[k];

    /* atom */
    if (InvisibleAtom(i))
      continue; /* 表示しない */

    /* projection */
    atomp = &atom_list[atom[i]];
    r = atomp->radius; 
    _z = coord[i].z;
    _z0 = (mdv_status->distance-DELTA_Z)*mdv_status->max_radius;
    if (_z > _z0 - r)
      continue; /* 表示しない */
    _x = coord[i].x;
    _y = coord[i].y;
    L = z0 - _z;
    R = sqrt(_x*_x + _y*_y + L*L);
    tmp = z0 * L / (L*L-r*r);
    _x *= tmp;
    _y *= tmp;
    _r = r * z0 * R / (L * sqrt(R*R-r*r));

    /* set color */
    if (fabs(_z)*(color_level+1) > color_level*mdv_status->max_radius)
      col_l = color_level;
    else
      col_l = (int) (fabs(_z)*(color_level+1) / mdv_status->max_radius);
    if (_z < 0.0)
      col_l = -col_l;
    if (mdv_status->mark_color >= 0 && MarkRead(i)) {
      color = GetColor(mdv_status->mark_color, col_l);
    } else if (atom_list[atom[i]].color_l >= 0) {
      if (line[i].count == atom_list[atom[i]].count)
        color = GetColor(atomp->color, col_l);
      else if (line[i].count < atom_list[atom[i]].count)
        color = GetColor(atom_list[atom[i]].color_l, col_l);
      else
        color = GetColor(atom_list[atom[i]].color_h, col_l);
    } else {
      color = GetColor(atomp->color, col_l);
    }

    if (_r > 0.0) {
      a = ( _x-_r)*coef;
      b = (-_y-_r)*coef;
      c = d = 2.0*_r*coef;
      DrawListAdd(DRAW_OVALF, a, b, c, d, color, i);
      if (mdv_status->outline_mode)
        DrawListAdd(DRAW_OVALL, a, b, c, d, MDV_COLOR_BLACK, 0);
    }

    /* bonds */
    r = atomp->radius; 

    for (j = 0; j < line[i].n; j++) {
      MDV_Size pair_i, period_id;
      double x1, y1, z1, x2, y2, z2, ratio;

      pair_i = line[i].to[j] % n;
      period_id = line[i].to[j] / n;

      /* if already exist, skip it. */
      if (period_id == 0 && order[n+pair_i] <= order[n+i])
        continue;
      /* if another atom is invisible, skip it, too. */
      if (InvisibleAtom(pair_i))
        continue;

      /* set tip and end */
      x1 = coord[i].x; x2 = coord[pair_i].x + period_d[period_id].x;
      y1 = coord[i].y; y2 = coord[pair_i].y + period_d[period_id].y;
      z1 = coord[i].z; z2 = coord[pair_i].z + period_d[period_id].z;

      /* set color level */
      if (fabs((z1+z2)/2.0)*(color_level+1)
          > color_level*mdv_status->max_radius) {
        col_l = color_level;
      } else {
        col_l = (int) (fabs((z1+z2)/2.0)*(color_level+1)
          / mdv_status->max_radius);
      }
      if (z1+z2 < 0.0) col_l = -col_l;

      if (z1 <= z2) {
        /* cut tip in sphere */ 
        if (r*r > line[i].r2[j])
          continue; /* 原子に隠れて書かれない */
        ratio = r / sqrt(line[i].r2[j]);
        x1 += (x2-x1) * ratio;
        y1 += (y2-y1) * ratio;
        z1 += (z2-z1) * ratio;

        if (z1 > _z0)
          continue; /* 表示しない */

        if (z2 > _z0) {
          /* cut end */
          ratio = (z2 - _z0) / (z2-z1);
          x2 -= (x2-x1) * ratio;
          y2 -= (y2-y1) * ratio;
          z2 = _z0;
        }

        /* projection */
        ratio = mdv_status->distance
          / (mdv_status->distance - z1/mdv_status->max_radius);
        x1 *= ratio;
        y1 *= ratio;
        ratio = mdv_status->distance
          / (mdv_status->distance - z2/mdv_status->max_radius);
        x2 *= ratio;
        y2 *= ratio;
      } else {
        /* projection */
        ratio = mdv_status->distance
          / (mdv_status->distance - z1/mdv_status->max_radius);
        x1 *= ratio;
        y1 *= ratio;
        ratio = mdv_status->distance
          / (mdv_status->distance - z2/mdv_status->max_radius);
        x2 *= ratio;
        y2 *= ratio;

        /* cut tip in sphere */ 
        if (_r*_r > (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)) {
          /* 原子に隠れて書かれない */
          continue;
        } else {
          ratio = _r / sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
          x1 += (x2-x1) * ratio;
          y1 += (y2-y1) * ratio;
        }
      }

      /* set color */
      color = GetColor(bondshape_list[line[i].bsi[j]].color, col_l);

      a =  x1*coef;
      b = -y1*coef;
      c =  x2*coef;
      d = -y2*coef;
      DrawListAdd(DRAW_LINE, a, b, c, d, color, 0);
    }
  }
}
void ViewData(void) {
  MDV_3D *coord, center;
  const MDV_LineData *linelist;
  MDV_Size *order;
  LineData *line;
  MDV_Size n, line_n;

  n = MDV_CoordGetSize(md_coord_org);
  MDV_CoordCopy(md_coord, md_coord_org);
  coord = MDV_CoordP(md_coord);
  line_n = MDV_LineListGetSize(md_linelist);
  linelist = MDV_LineListP(md_linelist);
  MDV_OrderSetSize(md_order, 2*n);
  order = MDV_OrderP(md_order);
  MDV_LineSetSize(md_line, n);
  line = MDV_LineP(md_line);

  GetCenterMass(&center.x, &center.y, &center.z);
  if (mdv_status->fold_mode) Periodic(coord, n, &center);

  if (MDV_CoordCompare(md_coord, md_coord_cache) != 0
      || MDV_LineListCompare(md_linelist, md_linelist_cache) != 0) {
    MDV_CoordCopy(md_coord_cache, md_coord);
    MDV_LineListCopy(md_linelist_cache, md_linelist);
    MakeLines(coord, linelist, line_n, line, n);
  }

  ShiftToCenterMass(coord, n);
  RotateData(coord, n);
  SortData(coord, order, n);
  _ViewData(coord, order, line, n);
}

/* drawlistの単位系で、位置(x, y)に対応する粒子の番号を返す(なければ負) */
MDV_Size WhichParticle(double x, double y) {
  DrawList *p;
  double dx, dy, rx2, ry2;
  MDV_Size ret;
  int i;

  ret = -1;
  for (i = 0; i < drawlist_n; i++) {
    p = drawlist+i;
    if (p->flag == DRAW_OVALF) {
      dx = x - (p->a + p->c/2.0);
      dy = y - (p->b + p->d/2.0);
      rx2 = p->c*p->c/4.0;
      ry2 = p->d*p->d/4.0;
      if (rx2 > 0.0 && ry2 > 0.0 && ry2*dx*dx + rx2*dy*dy <= rx2*ry2)
        ret = p->i;
    }
  }
  return ret;
}

/* drawlistの単位系で、i番目の粒子の座標を返す(なければ何もしない) */
static void _GetParticleCoordinate(const MDV_3D *coord, MDV_Size i,
                                                      double *px, double *py) {
  const AtomType *atom_list;
  const AtomID *atom;
  double z0, r, _x, _y, _z, coef, L, tmp;

  atom_list = MDV_Atom_GetAtomList(mdv_atom);
  atom = MDV_VAtomIDP(md_atom);

  coef = 1.0 / (2.0*RADIUSRATIO*mdv_status->max_radius);
  z0 = mdv_status->distance*mdv_status->max_radius;
  r = atom_list[atom[i]].radius; 
  _z = coord[i].z;
  if (_z > (mdv_status->distance-DELTA_Z)*mdv_status->max_radius - r)
    return; /* 表示できない */
  _x = coord[i].x;
  _y = coord[i].y;
  L = z0 - _z;
  tmp = z0 * L / (L*L - r*r);
  _x *= tmp;
  _y *= tmp;
  *px = _x * coef;
  *py =-_y * coef;
}
void GetParticleCoordinate(MDV_Size i, double *px, double *py) {
  if (i < 0 || i >= MDV_CoordGetSize(md_coord)) {
    *px = 0.0;
    *py = 0.0;
  } else {
    _GetParticleCoordinate(MDV_CoordP(md_coord), i, px, py);
  }
}

/* 原子の名前を返す(あり得ない番号ならヌル文字列) */
const char *GetAtomName(MDV_Size i) {
  const AtomType *atom_list;
  const AtomID *atom;
  MDV_Size n;

  atom_list = MDV_Atom_GetAtomList(mdv_atom);
  atom = MDV_VAtomIDP(md_atom);
  n = MDV_VAtomIDGetSize(md_atom);
  if (i < 0 || i >= n) return "";
  return StringID2Str(atom_list[atom[i]].si);
}

/* 最大nまで表現可能なサイズで(左に0を補って)文字列にする。 */
static char i2str_str[12];
static const char *i2str(int i, int n) {
  int l;

  if (i < 0 || n <= 0) return NULL;
  if (i > n) n = i;
  for (l = 0; n > 0; l++) {
    i2str_str[l] = '0';
    n /= 10;
  }
  i2str_str[l] = '\0';
  for (; i > 0; l--) {
    i2str_str[l-1] = i % 10 + '0';
    i /= 10;
  }

  return (const char *) i2str_str;
}

/* POV-Rayファイルへのコンバート(FILE *がNULLなら処理しない) */
#define COLOR_N_MIN 10
#define _min(x, y) ((x)<=(y)?(x):(y))
static void _ConvertPovray(FILE *fp_pov, FILE *fp_inc, const char *str_inc,
                       const MDV_3D *coord, const LineData *line, MDV_Size n) {
  AtomID atom_list_n;
  const AtomType *atom_list;
  BondShapeID bondshape_list_n;
  const BondShapeType *bondshape_list;
  const AtomID *atom;
  const BondShapeType *bondsp;
  const char *str;
  AtomID ai;
  BondShapeID bsi;
  ColorID col_i, col_n, col_m;
  MDV_Size i;
  int id, j;

  atom_list_n = MDV_Atom_GetAtomListSize(mdv_atom);
  atom_list = MDV_Atom_GetAtomList(mdv_atom);
  bondshape_list_n = MDV_Atom_GetBondShapeListSize(mdv_atom);
  bondshape_list = MDV_Atom_GetBondShapeList(mdv_atom);
  atom = MDV_VAtomIDP(md_atom);
  col_m = col_n = TotalColorID();
  if (col_m < COLOR_N_MIN)
    col_m = COLOR_N_MIN;

  /* make include file */
  if (fp_inc != NULL) {
    fprintf(fp_inc, "/* POV-Ray v3.%d include file (mdview%s.%s) */\n",
      povray_version, MDV_VERSION, MDV_REVISION);
    fprintf(fp_inc, "\n");
    fprintf(fp_inc, "#declare Unit = %s%s // %s\n",
      (fabs(mdv_status->length_unit-1.0) <= 1.0e-16)?
        "1.000000000": "0.529177249",
      povray_version > 0? ";": "",
      (fabs(mdv_status->length_unit-1.0) <= 1.0e-16)?
        "-ang": "-au");
    fprintf(fp_inc, "\n");
    fprintf(fp_inc, "/* color */\n");
    for (col_i = 0; col_i < col_n; col_i++) {
      int col_r, col_g, col_b;

      col_r = col_g = col_b = 255;
      str = StringID2Str(ColorID2ColorStringID(col_i));
      Str2Rgb(str, &col_r, &col_g, &col_b);
      fprintf(fp_inc, "#declare Color%s = color rgb<%.4f %.4f %.4f>%s // %s\n",
        i2str(col_i+1, col_m), col_r/255.0, col_g/255.0, col_b/255.0,
        povray_version > 0? ";": "", str);
    }
    fprintf(fp_inc, "\n");

    fprintf(fp_inc, "light_source {<-200,200,200> color rgb<1,1,1>}\n");
    fprintf(fp_inc, "background {Color%s}\n",
      i2str(ColorStringID2ColorID(mdv_status->background_color)+1, col_m));
    fprintf(fp_inc, "\n");

    fprintf(fp_inc, "/* atom */\n");
    for (ai = 0; ai < atom_list_n; ai++) {
      fprintf(fp_inc, "#declare Atom%s = texture { // %s\n",
        i2str(ai+1, atom_list_n), StringID2Str(atom_list[ai].si));
      fprintf(fp_inc, "    pigment {Color%s}\n",
        i2str(ColorStringID2ColorID(atom_list[ai].color)+1, col_m));
      fprintf(fp_inc, "    finish {specular .6 diffuse .9 ambient .4}\n");
      fprintf(fp_inc, "}%s\n", povray_version > 0? ";": "");
      fprintf(fp_inc, "#declare AtomR%s = %.6e * Unit%s\n",
        i2str(ai+1, atom_list_n),
        atom_list[ai].radius/mdv_status->length_unit,
        povray_version > 0? ";": "");
      fprintf(fp_inc, "\n");
    }

    fprintf(fp_inc, "/* bond */\n");
    for (bsi = 0; bsi < bondshape_list_n; bsi++) {
      bondsp = &bondshape_list[bsi];
      fprintf(fp_inc, "#declare Bond%s = texture { // %s\n",
        i2str(bsi+1, bondshape_list_n), StringID2Str(bondsp->si));
      fprintf(fp_inc, "    pigment {Color%s}\n",
        i2str(ColorStringID2ColorID(bondsp->color)+1, col_m));
      fprintf(fp_inc, "    finish {specular .6 diffuse .9 ambient .4}\n");
      fprintf(fp_inc, "}%s\n", povray_version > 0? ";": "");
      fprintf(fp_inc, "#declare BondR%s = %.6e * Unit%s\n",
        i2str(bsi+1, bondshape_list_n),
        bondsp->radius/mdv_status->length_unit,
        povray_version > 0? ";": "");
      fprintf(fp_inc, "\n");
    }
  }

  /* make POV-Ray file */
  if (fp_pov != NULL) {
    double view_x, view_y;
    double tx, ty, tz;
    double theta, psi, phi;
    StringID color;

    fprintf(fp_pov, "/* POV-Ray v3.%d scene file (mdview%s.%s) */\n",
      povray_version, MDV_VERSION, MDV_REVISION);
    fprintf(fp_pov, "\n");
    fprintf(fp_pov, "#include \"%s\"\n", str_inc);
    fprintf(fp_pov, "\n");
    fprintf(fp_pov, "#declare Unit = %s%s // %s\n",
      (fabs(mdv_status->length_unit-1.0) <= 1.0e-16)?
        "1.000000000": "0.529177249",
      povray_version > 0? ";": "",
      (fabs(mdv_status->length_unit-1.0) <= 1.0e-16)? "-ang": "-au");
    fprintf(fp_pov, "#declare Radius = %.6e * Unit%s\n",
      mdv_status->max_radius/mdv_status->length_unit,
      povray_version > 0? ";": "");
    fprintf(fp_pov, "#declare Distance = %.6e%s\n",
      mdv_status->distance, povray_version > 0? ";": "");
    fprintf(fp_pov, "#declare Zoom = %.4f%s\n",
      GetZoomPercent(), povray_version > 0? ";": "");
    fprintf(fp_pov, "camera {\n");
    fprintf(fp_pov, "    location   z*Distance*Radius\n");
    fprintf(fp_pov, "    direction -z*Distance/2\n");
    fprintf(fp_pov, "    up    y/Zoom\n");
    fprintf(fp_pov, "    right x/Zoom\n");
    get_trans_parameter(&view_x, &view_y);
    fprintf(fp_pov, "    look_at <%-.3f ,%-.3f ,0>*-2*Radius\n",
      view_x, -view_y);
    fprintf(fp_pov, "}\n");
    fprintf(fp_pov, "\n");

    fprintf(fp_pov, "/* coordinate */\n");
    for (i = 0; i < n; i++) {
      fprintf(fp_pov, "#declare R%s = <% .6e,% .6e,% .6e>*Unit%s\n",
        i2str(i+1, n), coord[i].x/mdv_status->length_unit,
        coord[i].y/mdv_status->length_unit,
        coord[i].z/mdv_status->length_unit,
        povray_version > 0? ";": "");
    }
    fprintf(fp_pov, "\n");

    if (mdv_status->period_length.x > 0.0 || mdv_status->period_length.y > 0.0
        || mdv_status->period_length.z > 0.0) {
      SetPeriodVectors();
      for (id = 0; id < MAX_PERIOD_ID; id++) {
        fprintf(fp_pov, "#declare P%s = <% .6e,% .6e,% .6e>*Unit%s\n",
          i2str(id+1, MAX_PERIOD_ID), period_d[id].x/mdv_status->length_unit,
          period_d[id].y/mdv_status->length_unit,
          period_d[id].z/mdv_status->length_unit,
          povray_version > 0? ";": "");
      }
      fprintf(fp_pov, "\n");
    }

    fprintf(fp_pov, "/* mdview object */\n");
    fprintf(fp_pov, "\n");
    fprintf(fp_pov, "#declare MDV_Object = union {\n");

    fprintf(fp_pov, "    /* atom */\n");
    for (i = 0; i < n; i++) {
      if (InvisibleAtom(i))
        fprintf(fp_pov, "// ");
      fprintf(fp_pov, "    sphere {R%s,", i2str(i+1, n));
      fprintf(fp_pov, "AtomR%s ", i2str(atom[i]+1, atom_list_n));
      fprintf(fp_pov, "texture{Atom%s", i2str(atom[i]+1, atom_list_n));

      color = -1;
      if (atom_list[atom[i]].color_l >= 0) {
        if (line[i].count != atom_list[atom[i]].count) {
          if (line[i].count < atom_list[atom[i]].count)
            color = atom_list[atom[i]].color_l;
          else
            color = atom_list[atom[i]].color_h;
        }
      }
      if (mdv_status->mark_color >= 0 && MarkRead(i))
        color = mdv_status->mark_color;
      if (color >= 0)
        fprintf(fp_pov, " pigment {Color%s}",
          i2str(ColorStringID2ColorID(color)+1, col_m));

      fprintf(fp_pov, "} no_shadow}\n");
    }
    fprintf(fp_pov, "\n");

    fprintf(fp_pov, "    /* bond */\n");
    for (i = 0; i < n; i++) {
      for (j = 0; j < line[i].n; j++) {
        MDV_Size pair_i, period_id;

        pair_i = line[i].to[j] % n;
        period_id = line[i].to[j] / n;
        if (period_id == 0 && pair_i <= i) continue;

        if (InvisibleAtom(i) || InvisibleAtom(pair_i))  fprintf(fp_pov, "// ");

        bondsp = &bondshape_list[line[i].bsi[j]];
        fprintf(fp_pov, "    cylinder {R%s", i2str(i+1, n));
        fprintf(fp_pov, ",R%s", i2str(pair_i+1, n));
        if (period_id != 0)
          fprintf(fp_pov, "+P%s", i2str(period_id+1, MAX_PERIOD_ID));
        fprintf(fp_pov, ",BondR%s ", i2str(bondsp->id+1, bondshape_list_n));
        fprintf(fp_pov, "texture{Bond%s} no_shadow}\n",
          i2str(bondsp->id+1, bondshape_list_n));
      }
    }

    fprintf(fp_pov, "}%s\n", povray_version > 0? ";": "");
    fprintf(fp_pov, "\n");

    GetCenterMass(&tx, &ty, &tz);
    GetEulerAngle(&theta, &psi, &phi);

    fprintf(fp_pov, "object {\n");
    fprintf(fp_pov, "    MDV_Object\n");
    fprintf(fp_pov, "    translate <% .6e,% .6e,% .6e>*Unit\n",
      -tx/mdv_status->length_unit, -ty/mdv_status->length_unit,
      -tz/mdv_status->length_unit);
    fprintf(fp_pov, "    rotate -z*% 7.2f // phi\n",   (  phi*360.0/TWO_PI));
    fprintf(fp_pov, "    rotate -x*% 7.2f // theta\n", (theta*360.0/TWO_PI));
    fprintf(fp_pov, "    rotate -z*% 7.2f // psi\n",   (  psi*360.0/TWO_PI));
    fprintf(fp_pov, "}\n");
  }
}
void ConvertPovray(FILE *fp_pov, FILE *fp_inc, const char *str_inc) {
  MDV_3D *coord, center;
  const MDV_LineData *linelist;
  LineData *line;
  MDV_Size n, line_n;

  n = MDV_CoordGetSize(md_coord_org);
  MDV_CoordCopy(md_coord, md_coord_org);
  coord = MDV_CoordP(md_coord);
  line_n = MDV_LineListGetSize(md_linelist);
  linelist = MDV_LineListP(md_linelist);
  MDV_LineSetSize(md_line, n);
  line = MDV_LineP(md_line);

  GetCenterMass(&center.x, &center.y, &center.z);
  if (mdv_status->fold_mode) Periodic(coord, n, &center);

  if (MDV_CoordCompare(md_coord, md_coord_cache) != 0
      || MDV_LineListCompare(md_linelist, md_linelist_cache) != 0) {
    MDV_CoordCopy(md_coord_cache, md_coord);
    MDV_LineListCopy(md_linelist_cache, md_linelist);
    MakeLines(coord, linelist, line_n, line, n);
  }

  _ConvertPovray(fp_pov, fp_inc, str_inc, coord, line, n);
}

/* ---- 色関係(表示システム依存部分) --------------------------------------- */

typedef struct {
  StringID si; /* 色文字列ID */
  int lv;     /* グラデーションレベル(+MAXCOLORLEVEL) */

  MDV_Color color; /* 描画色ID */
} SystemColorType;

/* SystemColorType管理テーブル */
typedef Hash_Type SystemColorTable;
SystemColorTable *SystemColorTable_Alloc(void);
void SystemColorTable_Free(SystemColorTable *sctp);
SystemColorType *SystemColorTable_Search(SystemColorTable *sctp, StringID si,
  int lv);
SystemColorType *SystemColorTable_Insert(SystemColorTable *sctp, StringID si,
  int lv);
int SystemColorTable_Delete(SystemColorTable *sctp, StringID si, int lv);

typedef struct {
  int r, g, b; /* RGB値 */

  MDV_Color color; /* 描画色ID */
} ColorType;

/* ColorType管理テーブル */
typedef Hash_Type ColorTable;
ColorTable *ColorTable_Alloc(void);
void ColorTable_Free(ColorTable *ctp);
ColorType *ColorTable_Search(ColorTable *ctp, int r, int g, int b);
ColorType *ColorTable_Insert(ColorTable *ctp, int r, int g, int b);
int ColorTable_Delete(ColorTable *ctp, int r, int g, int b);

/* 定義 */
static SystemColorTable *systemcolor_table = NULL; /* システム色テーブル */
static ColorTable *color_table = NULL;             /* 描画色テーブル */
static int _init_system_colors = 0;                /* 初期化フラグ */
static MDV_Color color_number = 0;                 /* 描画色ID */

/* 色取得システムの初期化 */
static void TermSystemColors(void) {
  SystemColorTable_Free(systemcolor_table);
  systemcolor_table = NULL;
  ColorTable_Free(color_table);
  color_table = NULL;
  color_number = 0;
}
static void InitSystemColors(void) {
  SystemColorType *scp = NULL;
  ColorType *cp;

  /* テーブルの領域確保 */
  if ((systemcolor_table = SystemColorTable_Alloc()) == NULL
      || (color_table = ColorTable_Alloc()) == NULL)
    HeapError();

  /* 初期保有色の設定 */
  if ((cp = ColorTable_Insert(color_table,   0,   0,   0)) == NULL
      || (scp = SystemColorTable_Insert(systemcolor_table,
      Str2ColorStringID("#000000"), 0)) == NULL)
    HeapError();
  scp->color = cp->color = (MDV_Color) MDV_COLOR_BLACK;
  mdv_mapcolor(cp->color, cp->r, cp->g, cp->b);
  if ((cp = ColorTable_Insert(color_table, 255,   0,   0)) == NULL
      || (scp = SystemColorTable_Insert(systemcolor_table,
      Str2ColorStringID("#FF0000"), 0)) == NULL)
    HeapError();
  scp->color = cp->color = (MDV_Color) MDV_COLOR_RED;
  mdv_mapcolor(cp->color, cp->r, cp->g, cp->b);
  if ((cp = ColorTable_Insert(color_table,   0, 255,   0)) == NULL
      || (scp = SystemColorTable_Insert(systemcolor_table,
      Str2ColorStringID("#00FF00"), 0)) == NULL)
    HeapError();
  scp->color = cp->color = (MDV_Color) MDV_COLOR_GREEN;
  mdv_mapcolor(cp->color, cp->r, cp->g, cp->b);
  if ((cp = ColorTable_Insert(color_table, 255, 255,   0)) == NULL
      || (scp = SystemColorTable_Insert(systemcolor_table,
      Str2ColorStringID("#FFFF00"), 0)) == NULL)
    HeapError();
  scp->color = cp->color = (MDV_Color) MDV_COLOR_YELLOW;
  mdv_mapcolor(cp->color, cp->r, cp->g, cp->b);
  if ((cp = ColorTable_Insert(color_table,   0,   0, 255)) == NULL
      || (scp = SystemColorTable_Insert(systemcolor_table,
      Str2ColorStringID("#0000FF"), 0)) == NULL)
    HeapError();
  scp->color = cp->color = (MDV_Color) MDV_COLOR_BLUE;
  mdv_mapcolor(cp->color, cp->r, cp->g, cp->b);
  if ((cp = ColorTable_Insert(color_table, 255,   0, 255)) == NULL
      || (scp = SystemColorTable_Insert(systemcolor_table,
      Str2ColorStringID("#FF00FF"), 0)) == NULL)
    HeapError();
  scp->color = cp->color = (MDV_Color) MDV_COLOR_MAGENTA;
  mdv_mapcolor(cp->color, cp->r, cp->g, cp->b);
  if ((cp = ColorTable_Insert(color_table,   0, 255, 255)) == NULL
      || (scp = SystemColorTable_Insert(systemcolor_table,
      Str2ColorStringID("#00FFFF"), 0)) == NULL)
    HeapError();
  scp->color = cp->color = (MDV_Color) MDV_COLOR_CYAN;
  mdv_mapcolor(cp->color, cp->r, cp->g, cp->b);
  if ((cp = ColorTable_Insert(color_table, 255, 255, 255)) == NULL
      || (scp = SystemColorTable_Insert(systemcolor_table,
      Str2ColorStringID("#FFFFFF"), 0)) == NULL)
    HeapError();
  scp->color = cp->color = (MDV_Color) MDV_COLOR_WHITE;
  mdv_mapcolor(cp->color, cp->r, cp->g, cp->b);

  /* ユーザ定義色の初期値 */
  color_number = MDV_COLOR_FREE_COL1;

  AtExit(TermSystemColors);
  _init_system_colors = 1;
}

/* システムの色の取得(-color_level <= level <= color_level) */
MDV_Color GetColor(StringID si, int level) {
  SystemColorType *scp;
  int lv;

  if (!_init_system_colors)
    InitSystemColors();

  if (si < 0 || level < -color_level || level > color_level)
    MDV_Fatal("GetColor()");

  lv = level+MAXCOLORLEVEL;
  if ((scp = SystemColorTable_Search(systemcolor_table, si, lv)) == NULL) {
    ColorType *cp;
    int r, g, b;
    const char *str;

    /* SystemColorTypeの新規登録 */
    if ((scp = SystemColorTable_Insert(systemcolor_table, si, lv)) == NULL)
      HeapError();
    if ((str = ColorStringID2Str(si)) == NULL)
      MDV_Fatal("GetColor()");
    if (!Str2Rgb(str, &r, &g, &b))
      {fprintf(stderr, "%s: Illegal color string.\n", str); exit(1);}
    if (level != 0) {
      /* グラデーション */
      if (level < 0) {
        r = ((4*color_level+4+level)*r + 2*color_level+2)/(4*color_level+4);
        g = ((4*color_level+4+level)*g + 2*color_level+2)/(4*color_level+4);
        b = ((4*color_level+4+level)*b + 2*color_level+2)/(4*color_level+4);
      } else {
        r = r + (level*(255-r) + 2*color_level+2)/(4*color_level+4);
        g = g + (level*(255-g) + 2*color_level+2)/(4*color_level+4);
        b = b + (level*(255-b) + 2*color_level+2)/(4*color_level+4);
      }
    }
    if ((cp = ColorTable_Search(color_table, r, g, b)) == NULL) {
      /* ColorTypeの新規登録 */
      if ((cp = ColorTable_Insert(color_table, r, g, b)) == NULL)
        HeapError();
      cp->color = color_number++;
      mdv_mapcolor(cp->color, cp->r, cp->g, cp->b);

      if (debug_mode) {
        fprintf(stderr, "new color %2d = #%02x%02x%02x\n",
          cp->color, cp->r, cp->g, cp->b);
      }
    }
    scp->color = cp->color;
  }

  return scp->color;
}

/* 背景色を返す */
MDV_Color GetBackGroundColor(void) {
  return GetColor(mdv_status->background_color, 0);
}

/* ---- SystemColorType管理テーブル ---------------------------------------- */

/* SystemColorType型の初期化 */
void SystemColorType_Init(SystemColorType *scp, StringID si, int lv) {
  scp->si = si;
  scp->lv = lv;
  scp->color = -1;
}

/* SystemColorType用アロケータ */
#define SYSTEMCOLOR_CHUNK_SIZE (256*sizeof(SystemColorType))
static Chunk_Type *systemcolor_chunk = NULL; /* 暫定版: 終了処理は省略 */

/* 比較関数(si,lvが符号付き整数で、かつ0以上の値であることを前提とする) */
static int SystemColorTable_NodeCompare(const void *va, const void *vb) {
  const SystemColorType *pa, *pb;

  pa = (const SystemColorType *) va;
  pb = (const SystemColorType *) vb;
  if (pa->si != pb->si)
    return (int) (pa->si - pb->si);
  return (int) (pa->lv - pb->lv);
}

/* ハッシュ関数 */
static Hash_Size SystemColorTable_NodeHash(const void *vp) {
  const SystemColorType *scp = (const SystemColorType *) vp;
  unsigned long x; /* 32bit以上の整数 */

  x = (unsigned long) scp->si;
  x ^= (((x&0xFFFFUL)*0x5C9311DDUL) & 0xFFFF0000UL) ^ 0x6F9D564EUL;
  x ^= (((x>>16)*0xF1B83E69UL)>>16) ^ 0xF3F6867EUL  ^ (unsigned long) scp->lv;

  return (Hash_Size) x;
}

/* コピーコンストラクタ */
static void *SystemColorTable_NodeCopy(const void *vp) {
  const SystemColorType *scp = (const SystemColorType *) vp;
  SystemColorType *ret;

  if (vp == NULL || systemcolor_chunk == NULL)
    return NULL;
  if ((ret = (SystemColorType *) Chunk_NodeAlloc(systemcolor_chunk)) == NULL)
    return NULL;
  *ret = *scp;

  return ret;
}

/* デストラクタ */
static void SystemColorTable_NodeFree(void *vp) {
  if (vp == NULL || systemcolor_chunk == NULL)
    return;
  Chunk_NodeFree(systemcolor_chunk, vp);
}

/* インスタンスの作成 */
SystemColorTable *SystemColorTable_Alloc(void) {
  if (systemcolor_chunk == NULL) {
    if ((systemcolor_chunk = Chunk_TypeAlloc(sizeof(SystemColorType),
        SYSTEMCOLOR_CHUNK_SIZE, 0)) == NULL)
      return NULL;
  }
  return Hash_Alloc(0, 0);
}

/* インスタンスの消去 */
void SystemColorTable_Free(SystemColorTable *sctp) {
  Hash_Free(sctp, SystemColorTable_NodeFree);
}

/* 色情報の探索 */
SystemColorType *SystemColorTable_Search(SystemColorTable *sctp,
    StringID si, int lv) {
  SystemColorType key;

  if (si < 0 || lv < 0)
    return NULL;
  SystemColorType_Init(&key, si, lv);
  return (SystemColorType *) Hash_Search(sctp, &key,
    SystemColorTable_NodeCompare, SystemColorTable_NodeHash);
}

/* 色情報の登録 */
SystemColorType *SystemColorTable_Insert(SystemColorTable *sctp,
    StringID si, int lv) {
  SystemColorType systemcolor;

  if (si < 0 || lv < 0)
    return NULL;
  SystemColorType_Init(&systemcolor, si, lv);
  return (SystemColorType *) Hash_Insert(sctp, &systemcolor,
    SystemColorTable_NodeCompare, SystemColorTable_NodeHash,
    SystemColorTable_NodeCopy);
}

/* 色情報の削除 */
int SystemColorTable_Delete(SystemColorTable *sctp,
    StringID si, int lv) {
  SystemColorType key;

  if (si < 0 || lv < 0)
    return 0;
  SystemColorType_Init(&key, si, lv);
  return Hash_Delete(sctp, &key, SystemColorTable_NodeCompare,
    SystemColorTable_NodeHash, SystemColorTable_NodeFree);
}

/* ---- ColorType管理テーブル ---------------------------------------------- */

/* ColorType型の初期化 */
void ColorType_Init(ColorType *cp, int r, int g, int b) {
  cp->r = r;
  cp->g = g;
  cp->b = b;
  cp->color = -1;
}

/* ColorType用アロケータ */
#define COLOR_CHUNK_SIZE (256*sizeof(ColorType))
static Chunk_Type *color_chunk = NULL; /* 暫定版: 終了処理は省略 */

/* 比較関数(r,g,bが符号付き整数で、かつ0以上の値であることを前提とする) */
static int ColorTable_NodeCompare(const void *va, const void *vb) {
  const ColorType *pa, *pb;

  pa = (const ColorType *) va;
  pb = (const ColorType *) vb;
  if (pa->r != pb->r)
    return (int) (pa->r - pb->r);
  if (pa->g != pb->g)
    return (int) (pa->g - pb->g);
  return (int) (pa->b - pb->b);
}

/* ハッシュ関数 */
static Hash_Size ColorTable_NodeHash(const void *vp) {
  const ColorType *cp = (const ColorType *) vp;
  unsigned long x; /* 32bit以上の整数 */

  x = (unsigned long) cp->r;
  x ^= (((x&0xFFFFUL)*0x771667BDUL) & 0xFFFF0000UL) ^ 0x72E5E695UL;
  x ^= (((x>>16)*0x719ADB7BUL)>>16) ^ 0xCF5BE594UL  ^ (unsigned long) cp->g;
  x ^= (((x&0xFFFFUL)*0x6203606FUL) & 0xFFFF0000UL) ^ 0x213F1F37UL;
  x ^= (((x>>16)*0xE924C4FFUL)>>16) ^ 0xBC728877UL  ^ (unsigned long) cp->b;

  return (Hash_Size) x;
}

/* コピーコンストラクタ */
static void *ColorTable_NodeCopy(const void *vp) {
  const ColorType *cp = (const ColorType *) vp;
  ColorType *ret;

  if (vp == NULL || color_chunk == NULL)
    return NULL;
  if ((ret = (ColorType *) Chunk_NodeAlloc(color_chunk)) == NULL)
    return NULL;
  *ret = *cp;

  return ret;
}

/* デストラクタ */
static void ColorTable_NodeFree(void *vp) {
  if (vp == NULL || color_chunk == NULL)
    return;
  Chunk_NodeFree(color_chunk, vp);
}

/* インスタンスの作成 */
ColorTable *ColorTable_Alloc(void) {
  if (color_chunk == NULL) {
    if ((color_chunk = Chunk_TypeAlloc(sizeof(ColorType),
        COLOR_CHUNK_SIZE, 0)) == NULL)
      return NULL;
  }
  return Hash_Alloc(0, 0);
}

/* インスタンスの消去 */
void ColorTable_Free(ColorTable *ctp) {
  Hash_Free(ctp, ColorTable_NodeFree);
}

/* 色情報の探索 */
ColorType *ColorTable_Search(ColorTable *ctp, int r, int g, int b) {
  ColorType key;

  if (r < 0 || g < 0 || b < 0)
    return NULL;
  ColorType_Init(&key, r, g, b);
  return (ColorType *) Hash_Search(ctp, &key, ColorTable_NodeCompare,
    ColorTable_NodeHash);
}

/* 色情報の登録 */
ColorType *ColorTable_Insert(ColorTable *ctp, int r, int g, int b) {
  ColorType color;

  if (r < 0 || g < 0 || b < 0)
    return NULL;
  ColorType_Init(&color, r, g, b);
  return (ColorType *) Hash_Insert(ctp, &color, ColorTable_NodeCompare,
    ColorTable_NodeHash, ColorTable_NodeCopy);
}

/* 色情報の削除 */
int ColorTable_Delete(ColorTable *ctp, int r, int g, int b) {
  ColorType key;

  if (r < 0 || g < 0 || b < 0)
    return 0;
  ColorType_Init(&key, r, g, b);
  return Hash_Delete(ctp, &key, ColorTable_NodeCompare,
    ColorTable_NodeHash, ColorTable_NodeFree);
}

/*================================GLib依存部=================================*/

/*---------------------------------mark関係----------------------------------*/

typedef unsigned int MarkType;                /* mark情報をビット配列で記憶 */

#define _MARK_MAXLEVEL (sizeof(MarkType)*8-1) /* スタックの深さの上限 */
static int  _mark_init = 0;                   /* 初期化フラグ */
static int  _mark_mode = MARK_SINGLE;         /* 利用モード */
static int  _mark_level = 0;                  /* スタックの状態 */

static MDV_Size _mark_sbuf[_MARK_MAXLEVEL]; /* markの状態(MARK_SINGLE用) */
static GArray *_mark_mbuf = NULL;           /* markの状態(MARK_MULTIPLE用) */
#define MarkMBuf(i) (g_array_index(_mark_mbuf, MarkType, i))

/* Markの終了処理 */
static void MarkTerm(void) {
  if (_mark_init) {
    g_array_free(_mark_mbuf, TRUE);
    _mark_mbuf = NULL;
    _mark_init = 0;
  }
}

/* Markの初期化 */
void MarkInit(int mode) {
  MDV_Size i, n;

  if (mode != MARK_SINGLE && mode != MARK_MULTIPLE)
    {fprintf(stderr, "MarkInit(): Illegal argument.\n"); exit(1);}

  if (!_mark_init) {
    _mark_mbuf = g_array_new(FALSE, TRUE, sizeof(MarkType));
    AtExit(MarkTerm);
    _mark_init = 1;
  }

  _mark_mode = mode;
  n = TotalAtoms();
  g_array_set_size(_mark_mbuf, n);

  /* すべてのmarkをクリアする */
  for (i = 0; i < _MARK_MAXLEVEL; i++)
    _mark_sbuf[i] = -1;
  for (i = 0; i < n; i++)
    MarkMBuf(i) = 0;
}

/* モードの読みだし */
int GetMarkMode(void) {
  return _mark_mode;
}

/* モードの変更。返値は成功の真偽 */
int SetMarkMode(int mode) {
  if (!_mark_init || (mode != MARK_SINGLE && mode != MARK_MULTIPLE)) return 0;
  _mark_mode = mode;
  return 1;
}

/* 使用中のmarkをクリアする */
void MarkClear(void) {
  MDV_Size i, n;

  if (!_mark_init) return;
  n = TotalAtoms();
  g_array_set_size(_mark_mbuf, n);

  if (_mark_mode == MARK_SINGLE) {
    _mark_sbuf[_mark_level] = -1;
  } else {
    for (i = 0; i < n; i++)
      MarkMBuf(i) &= ~1;
  }
}

/* x番目をマークする */
void MarkSet(MDV_Size x) {
  if (!_mark_init) return;
  if (x < 0 || x >= TotalAtoms()) return;
  g_array_set_size(_mark_mbuf, TotalAtoms());

  if (_mark_mode == MARK_SINGLE) {
    if (_mark_sbuf[_mark_level] == x)
      _mark_sbuf[_mark_level] = -1;
    else
      _mark_sbuf[_mark_level] = x;
  } else {
    MarkMBuf(x) ^= 1;
  }
}

/* マークされた最初の位置を返す。マーク無しは負となる */
MDV_Size MarkRead1st(void) {
  MDV_Size i, n, ret;

  if (!_mark_init) return -1;
  n = TotalAtoms();
  g_array_set_size(_mark_mbuf, n);

  if (_mark_mode == MARK_SINGLE) {
    ret = _mark_sbuf[_mark_level];
  } else {
    for (i = 0; i < n && !(MarkMBuf(i)&1); i++)
      ;
    ret = (i < n)? i: -1;
  }
  return ret;
}

/* マークされた次の位置を返す。マーク無しは負となる */
MDV_Size MarkReadNext(MDV_Size x) {
  MDV_Size i, n, ret;

  if (!_mark_init) return -1;
  n = TotalAtoms();
  g_array_set_size(_mark_mbuf, n);

  if (_mark_mode == MARK_SINGLE) {
    ret = _mark_sbuf[_mark_level];
  } else if (x < 0) {
    ret = MarkRead1st();
  } else {
    for (i = x+1; i < n && !(MarkMBuf(i)&1); i++)
      ;
    if (i < n)
      ret = i;
    else
      ret = MarkRead1st();
  }
  return ret;
}

/* x番目の粒子のmark状態を返す */
int MarkRead(MDV_Size x) {
  int ret;

  if (!_mark_init) return 0;
  if (x < 0 || x >= TotalAtoms()) return 0;
  g_array_set_size(_mark_mbuf, TotalAtoms());

  if (_mark_mode == MARK_SINGLE)
    ret = (_mark_sbuf[_mark_level] == x);
  else {
    ret = MarkMBuf(x)&1;
  }
  return ret;
}

/* x番目の粒子のmark状態を設定する */
void MarkWrite(int val, MDV_Size x) {
  if (!_mark_init) return;
  if (x < 0 || x >= TotalAtoms()) return;
  g_array_set_size(_mark_mbuf, TotalAtoms());

  if (_mark_mode == MARK_SINGLE) {
    if (_mark_sbuf[_mark_level] == x) {
      if (val == 0) _mark_sbuf[_mark_level] = -1;
    } else {
      if (val != 0) _mark_sbuf[_mark_level] = x;
    }
  } else {
    if (val == 0)
      MarkMBuf(x) &= ~1;
    else
      MarkMBuf(x) |= 1;
  }
}

/* 現状をスタックに移してクリアする。返値は成功の真偽 */
int MarkPush(void) {
  MDV_Size i, n;

  if (!_mark_init) return 0;
  if (_mark_level >= _MARK_MAXLEVEL) return 0;
  n = TotalAtoms();
  g_array_set_size(_mark_mbuf, n);

  _mark_level++;
  _mark_sbuf[_mark_level] = -1;
  for (i = 0; i < n; i++)
    MarkMBuf(i) <<= 1;
  return 1;
}

/* 現状をスタックから戻す。返値は成功の真偽 */
int MarkPop(void) {
  MDV_Size i, n;

  if (!_mark_init) return 0;
  if (_mark_level <= 0) return 0;
  n = TotalAtoms();
  g_array_set_size(_mark_mbuf, n);

  _mark_level--;
  for (i = 0; i < n; i++) {
    MarkMBuf(i) >>= 1;
  }
  return 1;
}

/* 現状をスタックから読み込む。返値は成功の真偽 */
int MarkLoad(void) {
  MDV_Size i, n;
  MarkType val;

  if (!_mark_init) return 0;
  if (_mark_level <= 0) return 0;
  n = TotalAtoms();
  g_array_set_size(_mark_mbuf, n);

  _mark_sbuf[_mark_level] = _mark_sbuf[_mark_level-1];
  for (i = 0; i < n; i++) {
    val = MarkMBuf(i);
    MarkMBuf(i) = (val&~1) | ((val>>1)&1);
  }
  return 1;
}

/* 現状をスタックに書き込む。返値は成功の真偽 */
int MarkSave(void) {
  MDV_Size i, n;
  MarkType val;

  if (!_mark_init) return 0;
  if (_mark_level <= 0) return 0;
  n = TotalAtoms();
  g_array_set_size(_mark_mbuf, n);

  _mark_sbuf[_mark_level-1] = _mark_sbuf[_mark_level];
  for (i = 0; i < n; i++) {
    val = MarkMBuf(i);
    MarkMBuf(i) = (val&~2) | ((val&1)<<1);
  }
  return 1;
}

