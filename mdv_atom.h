/* "mdv_atom.h" 原子情報に関する処理 ヘッダファイル */

#ifndef _MDV_ATOM_H_
#define _MDV_ATOM_H_

#include <stdio.h>
#include "mdv_type.h"
#include "hash.h"
#include "tavltree.h"

#define MAXBONDTYPE 256  /* 結合の種類の上限 */

typedef struct {
  StringID si;      /* 文字列ID */

  double mass;      /* 質量 */
  double radius;    /* 半径 */
  StringID color;   /* 色文字列ID */

  AtomID id;        /* 原子ID */
  int flag;         /* 使用/未使用フラグ */
  int count;        /* カウント数 */
  StringID color_l, /* 不足した時の色文字列ID */
           color_h; /* 過剰の時の色文字列ID */
} AtomType;

typedef struct {
  StringID si;     /* 文字列ID */

  double radius;   /* 結合の円筒半径 */
  StringID color;  /* 色文字列ID */

  BondShapeID id;  /* 結合形状ID */
} BondShapeType;

typedef struct {
  StringID si;      /* 原子iの文字列ID */
  StringID sj;      /* 原子jの文字列ID (sj >= si) */
  int lv;           /* 結合レベル */

  double length2;   /* 結合長の２乗 */
  StringID sij;     /* 結合形状の文字列ID */

  BondID id;        /* 結合ID */
  BondShapeID bsi;  /* 結合形状ID */
  int count_i;      /* i側のカウント数 */
  int count_j;      /* j側のカウント数 */
} BondType;

typedef struct {
  StringID si;      /* 原子の文字列ID */
  StringID sij;     /* 結合形状の文字列ID */

  int count;        /* カウント数 */
  StringID color_l, /* 不足した時の色文字列ID */
           color_h; /* 過剰の時の色文字列ID */
} CountType;

typedef struct {
  AtomID ai; /* 原子i */
  AtomID aj; /* 原子j */

  BondID bl; /* 最小結合レベルの結合ID */
  BondID br; /* 最大結合レベルの結合ID */
} AtomPairType;

typedef BondID AtomPairID;

typedef Hash_Type AtomTable;
typedef TAVL_Tree BondTable;
typedef Hash_Type BondShapeTable;
typedef TAVL_Tree CountTable;
typedef Hash_Type AtomPairTable;

typedef struct {
  AtomTable *atom_table;           /* 原子情報(動的) */
  BondShapeTable *bondshape_table; /* 結合形状情報(動的) */
  BondTable *bond_table;           /* 結合情報(動的) */
  CountTable *count_table;         /* 結合数情報(動的) */
  int changed;                     /* 変更フラグ */

  AtomID atom_list_n;              /* 原子IDの総数 */
  AtomType *atom_list;             /* 原子リスト */
  BondShapeID bondshape_list_n;    /* 結合形状IDの総数 */
  BondShapeType *bondshape_list;   /* 結合形状リスト */
  BondID bond_list_n;              /* 結合IDの総数 */
  BondType *bond_list;             /* 結合リスト */
  CountID count_list_n;            /* 結合数IDの総数 */
  CountType *count_list;           /* 結合数リスト */
  AtomPairTable *atompair_table;   /* 原子対情報 */
  AtomPairID atompair_list_n;      /* 原子対IDの総数 */
  AtomPairType *atompair_list;     /* 原子対リスト */
} MDV_Atom;

extern MDV_Atom *mdv_atom; /* 原子関連情報インスタンス(暫定版) */

MDV_Atom *MDV_Atom_Alloc(void);
void MDV_Atom_Free(MDV_Atom *map);

StringID GetBondShapeStringID(StringID si, StringID sj, int lv);
const AtomType *MDV_Atom_SearchAtom(const MDV_Atom *map, StringID si);
const AtomType *MDV_Atom_EnterAtom(MDV_Atom *map, StringID si, double mass,
  double radius, StringID color);
int MDV_Atom_EraseAtom(MDV_Atom *map, StringID si);
const BondType *MDV_Atom_EnterBond(MDV_Atom *map, StringID si, StringID sj,
  int lv, double length2, StringID sij);
int MDV_Atom_EraseBond(MDV_Atom *map, StringID si, StringID sj, int lv);
const BondShapeType *MDV_Atom_SearchBondShape(const MDV_Atom *map,
  StringID sij);
const BondShapeType *MDV_Atom_EnterBondShape(MDV_Atom *map, StringID sij,
  double radius, StringID color);
const CountType *MDV_Atom_SearchCount(const MDV_Atom *map, StringID si,
  StringID sij);
int MDV_Atom_EnterCount(MDV_Atom *map, StringID si, int count,
  StringID color_l, StringID color_h);
int MDV_Atom_EraseCount(MDV_Atom *map, StringID si);
int MDV_Atom_EnterCountN(MDV_Atom *map, StringID si, StringID sij, int count);

void MDV_Atom_SetStaticInfo(MDV_Atom *map);
void MDV_Atom_DebugOutput(MDV_Atom *map, FILE *fp);

AtomID MDV_Atom_GetAtomListSize(MDV_Atom *map);
const AtomType *MDV_Atom_GetAtomList(MDV_Atom *map);
BondID MDV_Atom_GetBondListSize(MDV_Atom *map);
const BondType *MDV_Atom_GetBondList(MDV_Atom *map);
BondShapeID MDV_Atom_GetBondShapeListSize(MDV_Atom *map);
const BondShapeType *MDV_Atom_GetBondShapeList(MDV_Atom *map);
CountID MDV_Count_GetCountListSize(MDV_Atom *map);
const CountType *MDV_Count_GetCountList(MDV_Atom *map);
AtomPairID MDV_Atom_GetAtomPairListSize(MDV_Atom *map);
const AtomPairType *MDV_Atom_GetAtomPairList(MDV_Atom *map);

#endif /* _MDV_ATOM_H_ */

