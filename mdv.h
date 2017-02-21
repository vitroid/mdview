/* "mdv.h" mdview汎用データ管理のためのヘッダファイル */

#ifndef _MDV_H_
#define _MDV_H_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "mdv_type.h"

/* 引数関連 */
#define MAXCOLORLEVEL 9
extern int debug_mode;           /* デバッグ表示のトグル */
extern int formatted_mode;       /* -format関連 */
extern int color_level;          /* 奥行きを表現する色の段階数 */
extern int terminate_blank_line; /* 拡張テキストで空行で終端する */
extern int povray_version;       /* POV-Rayのバージョン(実際はレビジョン) */

/* ---- mdviewステータス --------------------------------------------------- */

#include "mdv_stat.h"

/* ---- 原子関連情報 ------------------------------------------------------- */

#include "mdv_atom.h"

/* ---- MDV_Array型 -------------------------------------------------------- */

#include "mdv_util.h"

/* ---- 粒子情報 ----------------------------------------------------------- */

/* MDV_VAtomID型 */
typedef MDV_Array MDV_VAtomID;
#define MDV_VAtomIDP(atom)          ((AtomID *) (atom)->p)
#define MDV_VAtomIDAlloc()          MDV_Array_Alloc(sizeof(AtomID))
#define MDV_VAtomIDFree(atom)       MDV_Array_Free(atom)
#define MDV_VAtomIDGetSize(atom)    ((atom)->n)
#define MDV_VAtomIDSetSize(atom, n) MDV_Array_SetSize((atom), (n))

extern MDV_VAtomID *md_atom;  /* 粒子→原子番号 */
extern MDV_Size max_particle; /* 粒子数の上限 */
MDV_Size TotalAtoms(void);

/* ---- 色情報 ------------------------------------------------------------- */

StringID SearchColorStringID(const char *str);
StringID Str2ColorStringID(const char *str);
const char *ColorStringID2Str(StringID si);
ColorID ColorStringID2ColorID(StringID si);
StringID ColorID2ColorStringID(ColorID ci);
ColorID TotalColorID(void);

/* ---- 3D空間情報 --------------------------------------------------------- */

/* MDV_Coord型 */
typedef MDV_Array MDV_Coord;
#define MDV_CoordP(coord)          ((MDV_3D *) (coord)->p)
#define MDV_CoordAlloc()           MDV_Array_Alloc(sizeof(MDV_3D))
#define MDV_CoordFree(coord)       MDV_Array_Free(coord)
#define MDV_CoordGetSize(coord)    ((coord)->n)
#define MDV_CoordSetSize(coord, n) MDV_Array_SetSize((coord), (n))
void MDV_CoordCopy(MDV_Coord *dst, const MDV_Coord *src);
int MDV_CoordCompare(const MDV_Coord *c1, const MDV_Coord *c2);

/* ---- 結合情報 ----------------------------------------------------------- */

typedef struct {
  BondShapeID bsi; /* 結合形状ID */
  MDV_Size i;      /* 粒子i */
  MDV_Size j;      /* 粒子j */
} MDV_LineData;

/* MDV_LineList型 */
typedef MDV_Array MDV_LineList;
#define MDV_LineListP(line)          ((MDV_LineData *) (line)->p)
#define MDV_LineListAlloc()          MDV_Array_Alloc(sizeof(MDV_LineData))
#define MDV_LineListFree(line)       MDV_Array_Free(line)
#define MDV_LineListGetSize(line)    ((line)->n)
#define MDV_LineListSetSize(line, n) MDV_Array_SetSize((line), (n))
void MDV_LineListCopy(MDV_LineList *dst, const MDV_LineList *src);
int MDV_LineListCompare(const MDV_LineList *c1, const MDV_LineList *c2);

/* mdv_arg.c */
StringID SearchBondShapeStringID(const char *str);
StringID Str2BondShapeStringID(const char *str);

/* ---- 引数解釈関係 ------------------------------------------------------- */

#define MAXARGLEN 1024 /* 引数の文字数の上限 */

/* 引数リスト */
typedef enum {
  ARG_CANCEL = -3,
  ARG_ILLEGAL = -2,
  ARG_UNKNOWN = -1,
  ARG_DEBUG = 0,
  /* 通常の引数 */
  ARG_ATOMIC_UNIT,
  ARG_ANGSTROM,
  ARG_ARG_VERSION,
  ARG_ATOM,         _ARG_ATOM,
  ARG_BACKGROUND,
  ARG_BLANK_LINE,   _ARG_BLANK_LINE,
  ARG_BOND,         _ARG_BOND,
  ARG_BOND_SHAPE,
  ARG_CENTER,       _ARG_CENTER,
  ARG_CLEAR_ATOM,   _ARG_CLEAR_ATOM,
  ARG_COLOR_LEVEL,
  ARG_CONVERT_MOL3,
  ARG_COUNT,        _ARG_COUNT,
  ARG_COUNT_N,
  ARG_DISTANCE,
  ARG_EULER_ANGLE,
  ARG_FOLD,         _ARG_FOLD,
  ARG_FORMAT,       _ARG_FORMAT,
  ARG_FRAMES,
  ARG_HELP,
  ARG_IDLE_CALL,    _ARG_IDLE_CALL,
  ARG_LAYER,        _ARG_LAYER,
  ARG_LAYER_VISIBLE,
  ARG_LAYER_KILL,
  ARG_LENGTH,       _ARG_LENGTH,
  ARG_LOOP,         _ARG_LOOP,
  ARG_MARK,
  ARG_MARK_MULTI,   _ARG_MARK_MULTI,
  ARG_MAX_PARTICLE,
  ARG_POVRAY,
  ARG_RANGE,
  ARG_VERSION,
  ARG_WINDOW_SIZE
} ArgumentID;

/* mdv_arg.c */
ArgumentID GetArgumentID(const char *str);
int GetArgumentSize(ArgumentID arg, const MDV_Status *msp);
char **SetOptArgv(int argc, const char *(*f_getarg)(void));
int MDV_StatusUpdate(MDV_Status *msp, ArgumentID arg, const char *str_arg,
  char * optargv[]);

#endif /* _MDV_H_ */
