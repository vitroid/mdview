/* "mdv_atom.h" ���Ҿ���˴ؤ������ �إå��ե����� */

#ifndef _MDV_ATOM_H_
#define _MDV_ATOM_H_

#include <stdio.h>
#include "mdv_type.h"
#include "hash.h"
#include "tavltree.h"

#define MAXBONDTYPE 256  /* ���μ���ξ�� */

typedef struct {
  StringID si;      /* ʸ����ID */

  double mass;      /* ���� */
  double radius;    /* Ⱦ�� */
  StringID color;   /* ��ʸ����ID */

  AtomID id;        /* ����ID */
  int flag;         /* ����/̤���ѥե饰 */
  int count;        /* ������ȿ� */
  StringID color_l, /* ��­�������ο�ʸ����ID */
           color_h; /* ���λ��ο�ʸ����ID */
} AtomType;

typedef struct {
  StringID si;     /* ʸ����ID */

  double radius;   /* ���α���Ⱦ�� */
  StringID color;  /* ��ʸ����ID */

  BondShapeID id;  /* ������ID */
} BondShapeType;

typedef struct {
  StringID si;      /* ����i��ʸ����ID */
  StringID sj;      /* ����j��ʸ����ID (sj >= si) */
  int lv;           /* ����٥� */

  double length2;   /* ���Ĺ�Σ��� */
  StringID sij;     /* ��������ʸ����ID */

  BondID id;        /* ���ID */
  BondShapeID bsi;  /* ������ID */
  int count_i;      /* i¦�Υ�����ȿ� */
  int count_j;      /* j¦�Υ�����ȿ� */
} BondType;

typedef struct {
  StringID si;      /* ���Ҥ�ʸ����ID */
  StringID sij;     /* ��������ʸ����ID */

  int count;        /* ������ȿ� */
  StringID color_l, /* ��­�������ο�ʸ����ID */
           color_h; /* ���λ��ο�ʸ����ID */
} CountType;

typedef struct {
  AtomID ai; /* ����i */
  AtomID aj; /* ����j */

  BondID bl; /* �Ǿ�����٥�η��ID */
  BondID br; /* �������٥�η��ID */
} AtomPairType;

typedef BondID AtomPairID;

typedef Hash_Type AtomTable;
typedef TAVL_Tree BondTable;
typedef Hash_Type BondShapeTable;
typedef TAVL_Tree CountTable;
typedef Hash_Type AtomPairTable;

typedef struct {
  AtomTable *atom_table;           /* ���Ҿ���(ưŪ) */
  BondShapeTable *bondshape_table; /* ����������(ưŪ) */
  BondTable *bond_table;           /* ������(ưŪ) */
  CountTable *count_table;         /* ��������(ưŪ) */
  int changed;                     /* �ѹ��ե饰 */

  AtomID atom_list_n;              /* ����ID����� */
  AtomType *atom_list;             /* ���ҥꥹ�� */
  BondShapeID bondshape_list_n;    /* ������ID����� */
  BondShapeType *bondshape_list;   /* �������ꥹ�� */
  BondID bond_list_n;              /* ���ID����� */
  BondType *bond_list;             /* ���ꥹ�� */
  CountID count_list_n;            /* ����ID����� */
  CountType *count_list;           /* �����ꥹ�� */
  AtomPairTable *atompair_table;   /* �����о��� */
  AtomPairID atompair_list_n;      /* ������ID����� */
  AtomPairType *atompair_list;     /* �����Хꥹ�� */
} MDV_Atom;

extern MDV_Atom *mdv_atom; /* ���Ҵ�Ϣ���󥤥󥹥���(������) */

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

