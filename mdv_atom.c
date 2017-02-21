/* "mdv_atom.c" ���Ҿ���˴ؤ������ */

#include <stdio.h>
#include <stdlib.h> /* malloc(), free(), qsort() */
#include <math.h>   /* sqrt() */
#include "mdview.h"
#include "mdv.h"    /* ColorStringID2Str() */
#include "mdv_atom.h"
#include "chunk.h"

/* ---- private��� -------------------------------------------------------- */

/* ���ҥơ��֥뷿 */
AtomTable *AtomTable_Alloc(void);
void AtomTable_Free(AtomTable *atp);
AtomType *AtomTable_Search(const AtomTable *atp, StringID si);
AtomType *AtomTable_Insert(AtomTable *atp, StringID si);
int AtomTable_Delete(AtomTable *atp, StringID si);
AtomID AtomTable_Total(const AtomTable *atp);
void AtomTable_List(const AtomTable *atp, AtomType **list);

/* �������ơ��֥뷿 */
BondShapeTable *BondShapeTable_Alloc(void);
void BondShapeTable_Free(BondShapeTable *bstp);
BondShapeType *BondShapeTable_Search(const BondShapeTable *bstp, StringID si);
BondShapeType *BondShapeTable_Insert(BondShapeTable *bstp, StringID si);
int BondShapeTable_Delete(BondShapeTable *bstp, StringID si);
BondShapeID BondShapeTable_Total(const BondShapeTable *bstp);
void BondShapeTable_List(const BondShapeTable *bstp, BondShapeType **list);

/* ���ơ��֥뷿 */
BondTable *BondTable_Alloc(void);
void BondTable_Free(BondTable *btp);
BondType *BondTable_Search(const BondTable *btp, StringID si, StringID sj,
  int lv);
BondType *BondTable_Insert(BondTable *btp, StringID si, StringID sj, int lv);
int BondTable_Delete(BondTable *btp, StringID si, StringID sj, int lv);
BondID BondTable_Total(const BondTable *btp);
void BondTable_List(const BondTable *btp, BondType **list);
BondType *BondTable_SearchNext(const BondTable *btp, StringID si, StringID sj,
  int lv);

/* �����ơ��֥뷿 */
CountTable *CountTable_Alloc(void);
void CountTable_Free(CountTable *ctp);
CountType *CountTable_Search(const CountTable *ctp, StringID si, StringID sij);
CountType *CountTable_Insert(CountTable *ctp, StringID si, StringID sij);
int CountTable_Delete(CountTable *ctp, StringID si, StringID sij);
CountID CountTable_Total(const CountTable *ctp);
void CountTable_List(const CountTable *ctp, CountType **list);
CountType *CountTable_SearchNext(const CountTable *ctp, StringID si,
  StringID sij);

/* �����Хơ��֥뷿 */
static AtomPairTable *AtomPairTable_Alloc(void);
void AtomPairTable_Free(AtomPairTable *aptp);
AtomPairType *AtomPairTable_Search(const AtomPairTable *aptp, AtomID ai,
  AtomID aj);
AtomPairType *AtomPairTable_Insert(AtomPairTable *aptp, AtomID ai, AtomID aj);
int AtomPairTable_Delete(AtomPairTable *aptp, AtomID ai, AtomID aj);
AtomPairID AtomPairTable_Total(const AtomPairTable *aptp);
void AtomPairTable_List(const AtomPairTable *aptp, AtomPairType **list);

/* ==== public��� ========================================================= */

/* MDV_Atom���γ���(���Ԥϰ۾ｪλ) */
MDV_Atom *MDV_Atom_Alloc(void) {
  MDV_Atom *map;

  map = (MDV_Atom *) malloc(sizeof(MDV_Atom));
  if ((map->atom_table = AtomTable_Alloc()) == NULL
      || (map->bondshape_table = BondShapeTable_Alloc()) == NULL
      || (map->bond_table = BondTable_Alloc()) == NULL
      || (map->count_table = CountTable_Alloc()) == NULL
      || (map->atompair_table = AtomPairTable_Alloc()) == NULL)
    HeapError();
  map->changed = 1;
  map->atom_list_n = 0;
  map->atom_list = NULL;
  map->bondshape_list_n = 0;
  map->bondshape_list = NULL;
  map->bond_list_n = 0;
  map->bond_list = NULL;
  map->count_list_n = 0;
  map->count_list = NULL;
  map->atompair_list_n = 0;
  map->atompair_list = NULL;

  return map;
}

/* MDV_Atom���γ��� */
void MDV_Atom_Free(MDV_Atom *map) {
  if (map == NULL)
    return;

  AtomTable_Free(map->atom_table);
  BondShapeTable_Free(map->bondshape_table);
  BondTable_Free(map->bond_table);
  CountTable_Free(map->count_table);
  AtomPairTable_Free(map->atompair_table);
  free(map->atom_list);
  free(map->bondshape_list);
  free(map->bond_list);
  free(map->count_list);
  free(map->atompair_list);
  free(map);
}

/* ---- ưŪ���� ----------------------------------------------------------- */

/* ���̾����������򼨤�ʸ������������� */
StringID GetBondShapeStringID(StringID si, StringID sj, int lv) {
  char *work;
  const char *str_i, *str_j;
  int len, len_i;
  StringID sij;
  int i;

  if (si <= sj) {
    str_i = StringID2Str(si);
    str_j = StringID2Str(sj);
  } else {
    str_i = StringID2Str(sj);
    str_j = StringID2Str(si);
  }
  len = (len_i = strlen(str_i)) + strlen(str_j) + lv+1 + 1;

  work = MDV_Work_Alloc(len);
  Strlcpy(work, str_i, len);
  for (i = len_i; i < len_i+lv+1 && i < len-1; i++)
    work[i] = '-';
  work[i] = '\0';
  Strlcat(work, str_j, len);
  sij = Str2StringID(work);
  MDV_Work_Free(work);

  return sij;
}

/* ���̾���ޥ˥奢���������ɤ������֤� */
#define BONDSHAPE_MANUAL_CHAR ' '
static int IsManualBondShape(StringID sij) {
  return ((StringID2Str(sij))[0] == BONDSHAPE_MANUAL_CHAR);
}

/* ���Ҿ����õ��(�ʤ����NULL���֤�) */
const AtomType *MDV_Atom_SearchAtom(const MDV_Atom *map, StringID si) {
  return AtomTable_Search(map->atom_table, si);
}

/* ���Ҿ������Ͽ */
const AtomType *MDV_Atom_EnterAtom(MDV_Atom *map, StringID si, double mass,
    double radius, StringID color) {
  AtomType *ap = NULL;

  if ((ap = AtomTable_Search(map->atom_table, si)) == NULL
      && (ap = AtomTable_Insert(map->atom_table, si)) == NULL)
    HeapError();
  ap->mass = mass;
  ap->radius = radius;
  ap->color = color;
  map->changed = 1;

  return ap;
}

/* ���Ҿ���κ��(���ͤ������ο���) */
int MDV_Atom_EraseAtom(MDV_Atom *map, StringID si) {
  BondType *bp;
  CountType *cp;

  if (AtomTable_Search(map->atom_table, si) == NULL)
    return 0;

  /* �б������硢��������κ�� */
  if ((bp = BondTable_Search(map->bond_table, si, -1, -1)) != NULL) {
    while ((bp = BondTable_SearchNext(map->bond_table, si, -1, -1)) != NULL
        && bp->si == si) {
      if (bp->si != bp->sj) {
        if (!IsManualBondShape(bp->sij))
          CountTable_Delete(map->count_table, bp->sj, bp->sij);
        if (!BondTable_Delete(map->bond_table, bp->sj, bp->si, bp->lv))
          MDV_Fatal("MDV_Atom_EraseAtom()");
      }
      if (!IsManualBondShape(bp->sij))
        CountTable_Delete(map->count_table, bp->si, bp->sij);
      if (!BondTable_Delete(map->bond_table, bp->si, bp->sj, bp->lv))
        MDV_Fatal("MDV_Atom_EraseAtom()");
    }
    if ((cp = CountTable_SearchNext(map->count_table, si, -1)) == NULL
        || cp->si != si)
      CountTable_Delete(map->count_table, si, -1);
    if (!BondTable_Delete(map->bond_table, si, -1, -1))
      MDV_Fatal("MDV_Atom_EraseAtom()");
  }
  if (!AtomTable_Delete(map->atom_table, si))
    MDV_Fatal("MDV_Atom_EraseAtom()");
  map->changed = 1;

  return 1;
}

/* �����������õ��(�ʤ����NULL���֤�) */
const BondShapeType *MDV_Atom_SearchBondShape(const MDV_Atom *map,
    StringID sij) {
  return BondShapeTable_Search(map->bondshape_table, sij);
}

/* �������������Ͽ(���Ԥ�NULL���֤�) */
const BondShapeType *MDV_Atom_EnterBondShape(MDV_Atom *map, StringID sij,
    double radius, StringID color) {
  BondShapeType *bsp;

  if ((bsp = BondShapeTable_Search(map->bondshape_table, sij)) == NULL
      && (bsp = BondShapeTable_Insert(map->bondshape_table, sij)) == NULL)
    HeapError();
  bsp->radius = radius;
  bsp->color = color;
  map->changed = 1;

  return bsp;
}

/* ���������Ͽ(���Ԥ�NULL���֤�) */
const BondType *MDV_Atom_EnterBond(MDV_Atom *map, StringID si, StringID sj,
    int lv, double length2, StringID sij) {
  BondType *bp = NULL;

  if (AtomTable_Search(map->atom_table, si) == NULL
      || AtomTable_Search(map->atom_table, sj) == NULL)
    return NULL;

  if ((bp = BondTable_Search(map->bond_table, si, -1, -1)) == NULL
      && (bp = BondTable_Insert(map->bond_table, si, -1, -1)) == NULL)
    HeapError();
  if ((bp = BondTable_Search(map->bond_table, sj, -1, -1)) == NULL
      && (bp = BondTable_Insert(map->bond_table, sj, -1, -1)) == NULL)
    HeapError();

  if (si > sj)
    {StringID t = si; si = sj; sj = t;}
  if ((bp = BondTable_Search(map->bond_table, si, sj, lv)) == NULL) {
    if ((si != sj && BondTable_Insert(map->bond_table, sj, si, lv) == NULL)
        || (bp = BondTable_Insert(map->bond_table, si, sj, lv)) == NULL)
      HeapError();
  }
  bp->length2 = length2;
  bp->sij = sij;
  map->changed = 1;

  return bp;
}

/* ������κ��(���ͤ������ο���) */
int MDV_Atom_EraseBond(MDV_Atom *map, StringID si, StringID sj, int lv) {
  StringID sij;

  if (AtomTable_Search(map->atom_table, si) == NULL
      || AtomTable_Search(map->atom_table, sj) == NULL
      || BondTable_Search(map->bond_table, si, sj, lv) == NULL)
    return 0;

  sij = GetBondShapeStringID(si, sj, lv);
  CountTable_Delete(map->count_table, si, sij);
  if (!BondTable_Delete(map->bond_table, si, sj, lv))
    MDV_Fatal("MDV_Atom_EraseBond()");
  if (si != sj) {
    CountTable_Delete(map->count_table, sj, sij);
    if (!BondTable_Delete(map->bond_table, sj, si, lv))
      MDV_Fatal("MDV_Atom_EraseBond()");
  }
  map->changed = 1;

  return 1;
}

/* ���������õ��(�ʤ����NULL���֤�) */
const CountType *MDV_Atom_SearchCount(const MDV_Atom *map, StringID si,
    StringID sij) {
  return CountTable_Search(map->count_table, si, sij);
}

/* �����������Ͽ(���ͤ������ο���) */
int MDV_Atom_EnterCount(MDV_Atom *map, StringID si, int count,
    StringID color_l, StringID color_h) {
  CountType *cp = NULL;

  if (AtomTable_Search(map->atom_table, si) == NULL)
    return 0;
  if ((cp = CountTable_Search(map->count_table, si, -1)) == NULL
      && (cp = CountTable_Insert(map->count_table, si, -1)) == NULL)
    HeapError();
  cp->count = count;
  cp->color_l = color_l;
  cp->color_h = color_h;
  map->changed = 1;

  return 1;
}

/* ��������κ��(���ͤ������ο���) */
int MDV_Atom_EraseCount(MDV_Atom *map, StringID si) {
  CountType *cp;

  if (AtomTable_Search(map->atom_table, si) == NULL)
    return 0;

  if ((cp = CountTable_Search(map->count_table, si, -1)) != NULL) {
    while ((cp = CountTable_SearchNext(map->count_table, si, -1))
        != NULL && cp->si == si) {
      if (!CountTable_Delete(map->count_table, cp->si, cp->sij))
        MDV_Fatal("MDV_Atom_EraseCount()");
    }
    if (!CountTable_Delete(map->count_table, si, -1))
      MDV_Fatal("MDV_Atom_EraseCount()");
  }
  map->changed = 1;

  return 1;
}

/* ���������Ȥη�������Ͽ(���ͤ������ο���) */
int MDV_Atom_EnterCountN(MDV_Atom *map, StringID si, StringID sij, int count) {
  CountType *cp;

  if ((cp = CountTable_Search(map->count_table, si, sij)) == NULL
      && (cp = CountTable_Insert(map->count_table, si, sij)) == NULL)
    HeapError();
  cp->count = count;
  map->changed = 1;

  return 1;
}

/* ---- ��Ū���� ----------------------------------------------------------- */

/* ��Ū��������� */
void MDV_Atom_SetStaticInfo(MDV_Atom *map) {
  StringID si, sj;
  AtomID ai, aj;
  BondShapeID bsi;
  BondID i, bi;
  CountID ci;
  AtomType **app;
  BondShapeType **bspp;
  BondType **bpp;
  CountType *cp;
  CountType **cpp;
  AtomPairID api;
  AtomPairType **appp;
  double r2;

  if (!map->changed)
    return;

  /* atom_list[]�κ��� */
  if ((map->atom_list_n = AtomTable_Total(map->atom_table)) <= 0)
    MDV_Fatal("SetStaticInfo()");
  if (map->atom_list != NULL)
    free(map->atom_list);
  if ((map->atom_list
      = (AtomType *) malloc(sizeof(AtomType) * map->atom_list_n)) == NULL)
    HeapError();
  app = (AtomType **) MDV_Work_Alloc(sizeof(AtomType *) * map->atom_list_n);
  AtomTable_List(map->atom_table, app);
  for (ai = 0; ai < map->atom_list_n; ai++) {
    app[ai]->id = ai;
    map->atom_list[ai] = *(app[ai]);

    cp = CountTable_Search(map->count_table, map->atom_list[ai].si, -1);
    if (cp != NULL) {
      map->atom_list[ai].count = cp->count;
      map->atom_list[ai].color_l = cp->color_l;
      map->atom_list[ai].color_h = cp->color_h;
    } else {
      map->atom_list[ai].count = 0;
      map->atom_list[ai].color_l = -1;
      map->atom_list[ai].color_h = -1;
    }
  }
  MDV_Work_Free(app);

  /* bondshape_list[]�κ��� */
  if ((map->bondshape_list_n
      = BondShapeTable_Total(map->bondshape_table)) > 0) {
    if (map->bondshape_list != NULL)
      free(map->bondshape_list);
    if ((map->bondshape_list = (BondShapeType *)
        malloc(sizeof(BondShapeType) * map->bondshape_list_n)) == NULL)
      HeapError();
    bspp = (BondShapeType **) MDV_Work_Alloc(sizeof(BondShapeType *)
      * map->bondshape_list_n);
    BondShapeTable_List(map->bondshape_table, bspp);
    for (bsi = 0; bsi < map->bondshape_list_n; bsi++) {
      bspp[bsi]->id = bsi;
      map->bondshape_list[bsi] = *(bspp[bsi]);
    }
    MDV_Work_Free(bspp);
  }

  /* bond_list[]�κ��� */
  if ((map->bond_list_n = BondTable_Total(map->bond_table)) > 0) {
    if (map->bond_list != NULL)
      free(map->bond_list);
    if ((map->bond_list
        = (BondType *) malloc(sizeof(BondType) * map->bond_list_n)) == NULL)
      HeapError();
    bpp = (BondType **) MDV_Work_Alloc(sizeof(BondType *) * map->bond_list_n);
    BondTable_List(map->bond_table, bpp);
    si = -1;
    sj = -1;
    r2 = 0.0;
    bi = 0;
    for (i = 0; i < map->bond_list_n; i++) {
      if (bpp[i]->si != si || bpp[i]->sj != sj) {
        si = bpp[i]->si;
        sj = bpp[i]->sj;
        r2 = 0.0;
      }
      if (bpp[i]->length2 > r2) {
        /* length2�������Ͽ */
        BondShapeType *bsp;
        StringID sij;

        r2 = bpp[i]->length2;
        bpp[i]->id = bi;
        map->bond_list[bi] = *(bpp[i]);

        sij = map->bond_list[bi].sij;
        if ((bsp = BondShapeTable_Search(map->bondshape_table, sij)) == NULL)
          MDV_Fatal("SetStaticInfo()");
        map->bond_list[bi].bsi = bsp->id;

        cp = CountTable_Search(map->count_table, si, sij);
        map->bond_list[bi].count_i = (cp != NULL)? cp->count: 0;
        cp = CountTable_Search(map->count_table, sj, sij);
        map->bond_list[bi].count_j = (cp != NULL)? cp->count: 0;

        bi++;
      }
    }
    map->bond_list_n = bi;
    MDV_Work_Free(bpp);
  }

  /* count_list[]�κ��� */
  if ((map->count_list_n = CountTable_Total(map->count_table)) > 0) {
    if (map->count_list != NULL)
      free(map->count_list);
    if ((map->count_list = (CountType *)
        malloc(sizeof(CountType) * map->count_list_n)) == NULL)
      HeapError();
    cpp = (CountType **) MDV_Work_Alloc(sizeof(CountType *)
      * map->count_list_n);
    CountTable_List(map->count_table, cpp);
    for (ci = 0; ci < map->count_list_n; ci++)
      map->count_list[ci] = *(cpp[ci]);
    MDV_Work_Free(cpp);
  }

  /* atompair_table�κ��� */
  if (map->atompair_table != NULL)
    AtomPairTable_Free(map->atompair_table);
  if ((map->atompair_table = AtomPairTable_Alloc()) == NULL)
    HeapError();
  bi = 0;
  while (bi < map->bond_list_n) {
    BondID bl, br;
    AtomType *aip = NULL, *ajp = NULL;
    AtomPairType *app;

    si = map->bond_list[bi].si;
    sj = map->bond_list[bi].sj;
    if ((aip = AtomTable_Search(map->atom_table, si)) == NULL
        || (ajp = AtomTable_Search(map->atom_table, sj)) == NULL)
      MDV_Fatal("SetStaticInfo()");
    ai = aip->id;
    aj = ajp->id;
    bl = bi;
    do {
      br = bi;
      bi++;
    } while (map->bond_list[bi].si == si && map->bond_list[bi].sj == sj);

    if ((app = AtomPairTable_Insert(map->atompair_table, ai, aj)) == NULL)
      HeapError();
    app->bl = bl;
    app->br = br;
    if (ai != aj) {
      if ((app = AtomPairTable_Insert(map->atompair_table, aj, ai)) == NULL)
        HeapError();
      app->bl = bl;
      app->br = br;
    }
  }

  /* atompair_list[]�κ��� */
  if ((map->atompair_list_n = AtomPairTable_Total(map->atompair_table)) > 0) {
    if (map->atompair_list != NULL)
      free(map->atompair_list);
    if ((map->atompair_list = (AtomPairType *)
        malloc(sizeof(AtomPairType) * map->atompair_list_n)) == NULL)
      HeapError();
    appp = (AtomPairType **)
      MDV_Work_Alloc(sizeof(AtomPairType *) * map->atompair_list_n);
    AtomPairTable_List(map->atompair_table, appp);
    for (api = 0; api < map->atompair_list_n; api++)
      map->atompair_list[api] = *(appp[api]);
    MDV_Work_Free(appp);
  }

  /* ��λ */
  map->changed = 0;
}

/* ����ID��������֤� */
AtomID MDV_Atom_GetAtomListSize(MDV_Atom *map) {
  MDV_Atom_SetStaticInfo(map);
  return map->atom_list_n;
}

/* ���ҥꥹ�Ȥ��֤� */
const AtomType *MDV_Atom_GetAtomList(MDV_Atom *map) {
  MDV_Atom_SetStaticInfo(map);
  return map->atom_list;
}

/* ������ID��������֤� */
BondShapeID MDV_Atom_GetBondShapeListSize(MDV_Atom *map) {
  MDV_Atom_SetStaticInfo(map);
  return map->bondshape_list_n;
}

/* �������ꥹ�Ȥ��֤� */
const BondShapeType *MDV_Atom_GetBondShapeList(MDV_Atom *map) {
  MDV_Atom_SetStaticInfo(map);
  return map->bondshape_list;
}

/* ���ID��������֤� */
BondID MDV_Atom_GetBondListSize(MDV_Atom *map) {
  MDV_Atom_SetStaticInfo(map);
  return map->bond_list_n;
}

/* ���ꥹ�Ȥ��֤� */
const BondType *MDV_Atom_GetBondList(MDV_Atom *map) {
  MDV_Atom_SetStaticInfo(map);
  return map->bond_list;
}

/* ����ID��������֤� */
CountID MDV_Atom_GetCountListSize(MDV_Atom *map) {
  MDV_Atom_SetStaticInfo(map);
  return map->count_list_n;
}

/* �����ꥹ�Ȥ��֤� */
const CountType *MDV_Atom_GetCountList(MDV_Atom *map) {
  MDV_Atom_SetStaticInfo(map);
  return map->count_list;
}

/* ������ID��������֤� */
AtomPairID MDV_Atom_GetAtomPairListSize(MDV_Atom *map) {
  MDV_Atom_SetStaticInfo(map);
  return map->atompair_list_n;
}

/* �����Хꥹ�Ȥ��֤� */
const AtomPairType *MDV_Atom_GetAtomPairList(MDV_Atom *map) {
  MDV_Atom_SetStaticInfo(map);
  return map->atompair_list;
}

/* �ǥХå��ѽ��� */
void MDV_Atom_DebugOutput(MDV_Atom *map, FILE *fp) {
  AtomID ai;
  BondShapeID bsi;
  BondID bi;
  CountID ci;
  AtomType *ap;
  BondShapeType *bsp;
  BondType *bp;
  CountType *cp;

  MDV_Atom_SetStaticInfo(map);

  fprintf(fp, "--- atom list ---\n");
  for (ai = 0; ai < map->atom_list_n; ai++) {
    ap = &(map->atom_list[ai]);
    fprintf(fp, "%d: %s: mass = %f, radius = %f, color = \"%s\"\n",
      ap->id, StringID2Str(ap->si), ap->mass, ap->radius,
      ColorStringID2Str(ap->color));
  }

  fprintf(fp, "--- bond shape table ---\n");
  for (bsi = 0; bsi < map->bondshape_list_n; bsi++) {
    bsp = &(map->bondshape_list[bsi]);
    fprintf(fp, "%d: %s: radius = %f, color = \"%s\"\n",
      bsp->id, StringID2Str(bsp->si), bsp->radius,
      ColorStringID2Str(bsp->color));
  }

  fprintf(fp, "--- bond table ---\n");
  for (bi = 0; bi < map->bond_list_n; bi++) {
    bp = &(map->bond_list[bi]);
    fprintf(fp, "%d: %s-%s: level = %d, length = %f, shape = %s\n",
      bp->id, StringID2Str(bp->si), StringID2Str(bp->sj), bp->lv+1,
      sqrt(bp->length2), StringID2Str(bp->sij));
  }

  fprintf(fp, "--- count table ---\n");
  for (ci = 0; ci < map->count_list_n; ci++) {
    cp = &(map->count_list[ci]);
    if (cp->sij < 0) {
      fprintf(fp, "atom %s: total = %d, color_l = \"%s\", color_h = \"%s\"\n",
        StringID2Str(cp->si), cp->count,
        ColorStringID2Str(cp->color_l), ColorStringID2Str(cp->color_h));
    } else {
      fprintf(fp, "  bond %s: count = %d\n",
        StringID2Str(cp->sij), cp->count);
    }
  }

  fprintf(fp, "\n");
}

/* ==== private��� ======================================================== */

/* ---- AtomType�����ơ��֥� ----------------------------------------------- */

/* AtomType���ν���� */
void AtomType_Init(AtomType *ap, StringID si) {
  ap->si = si;

  ap->mass = -1.0;
  ap->radius = -1.0;
  ap->color = -1;

  ap->id = -1;
  ap->flag = 0;
  ap->count = 0;
  ap->color_l = -1;
  ap->color_h = -1;
}

/* AtomType�ѥ������� */
#define ATOM_CHUNK_SIZE (256*sizeof(AtomType))
static Chunk_Type *atom_chunk = NULL; /* ������: ��λ�����Ͼ�ά */

/* ��Ӵؿ�(StringID������դ������ǡ�����0�ʾ���ͤǤ��뤳�Ȥ�����Ȥ���) */
static int AtomTable_NodeCompare(const void *va, const void *vb) {
  return (int) (((const AtomType *) va)->si - ((const AtomType *) vb)->si);
}

/* �ϥå���ؿ� */
static Hash_Size AtomTable_NodeHash(const void *vp) {
  return (Hash_Size) ((const AtomType *) vp)->si;
}

/* ���ԡ����󥹥ȥ饯�� */
static void *AtomTable_NodeCopy(const void *vp) {
  const AtomType *ap = (const AtomType *) vp;
  AtomType *ret;

  if (vp == NULL || atom_chunk == NULL)
    return NULL;
  if ((ret = (AtomType *) Chunk_NodeAlloc(atom_chunk)) == NULL)
    return NULL;
  *ret = *ap;

  return ret;
}

/* �ǥ��ȥ饯�� */
static void AtomTable_NodeFree(void *vp) {
  if (vp == NULL || atom_chunk == NULL)
    return;
  Chunk_NodeFree(atom_chunk, vp);
}

/* ���󥹥��󥹤κ��� */
AtomTable *AtomTable_Alloc(void) {
  if (atom_chunk == NULL) {
    if ((atom_chunk = Chunk_TypeAlloc(sizeof(AtomType),
        ATOM_CHUNK_SIZE, 0)) == NULL)
      return NULL;
  }
  return Hash_Alloc(0, 0);
}

/* ���󥹥��󥹤ξõ� */
void AtomTable_Free(AtomTable *atp) {
  Hash_Free(atp, AtomTable_NodeFree);
}

/* ���Ҿ����õ�� */
AtomType *AtomTable_Search(const AtomTable *atp, StringID si) {
  AtomType key;

  if (si < 0)
    return NULL;
  AtomType_Init(&key, si);
  return (AtomType *) Hash_Search(atp, &key, AtomTable_NodeCompare,
    AtomTable_NodeHash);
}

/* ���Ҿ������Ͽ */
AtomType *AtomTable_Insert(AtomTable *atp, StringID si) {
  AtomType atom;

  if (si < 0)
    return NULL;
  AtomType_Init(&atom, si);
  return (AtomType *) Hash_Insert(atp, &atom, AtomTable_NodeCompare,
    AtomTable_NodeHash, AtomTable_NodeCopy);
}

/* ���Ҿ���κ�� */
int AtomTable_Delete(AtomTable *atp, StringID si) {
  AtomType key;

  if (si < 0)
    return 0;
  AtomType_Init(&key, si);
  return Hash_Delete(atp, &key, AtomTable_NodeCompare,
    AtomTable_NodeHash, AtomTable_NodeFree);
}

/* �ǡ�������� */ 
AtomID AtomTable_Total(const AtomTable *atp) {
  return Hash_Total(atp);
}

/* ���Ҿ���Υꥹ�ȼ��� */
static int AtomTable_NodePtrCompare(const void *va, const void *vb) {
  return AtomTable_NodeCompare(*((const void **) va), *((const void **) vb));
}
void AtomTable_List(const AtomTable *atp, AtomType **list) {
  AtomID n;

  if ((n = AtomTable_Total(atp)) <= 0)
    return;
  Hash_ReadAll(atp, (void **) list);
  qsort(list, n, sizeof(void *), AtomTable_NodePtrCompare);
}

/* ---- BondShapeType�����ơ��֥� ------------------------------------------- */

/* BondShapeType���ν���� */
void BondShapeType_Init(BondShapeType *bsp, StringID si) {
  bsp->si = si;

  bsp->radius = -1.0;
  bsp->color = -1;

  bsp->id = -1;
}

/* BondShapeType�ѥ������� */
#define BONDSHAPE_CHUNK_SIZE (256*sizeof(BondShapeType))
static Chunk_Type *bondshape_chunk = NULL; /* ������: ��λ�����Ͼ�ά */

/* ��Ӵؿ�(StringID������դ������ǡ�����0�ʾ���ͤǤ��뤳�Ȥ�����Ȥ���) */
static int BondShapeTable_NodeCompare(const void *va, const void *vb) {
  return (int) (((const BondShapeType *) va)->si
    - ((const BondShapeType *) vb)->si);
}

/* �ϥå���ؿ� */
static Hash_Size BondShapeTable_NodeHash(const void *vp) {
  return (Hash_Size) ((const BondShapeType *) vp)->si;
}

/* ���ԡ����󥹥ȥ饯�� */
static void *BondShapeTable_NodeCopy(const void *vp) {
  const BondShapeType *bsp = (const BondShapeType *) vp;
  BondShapeType *ret;

  if (vp == NULL || bondshape_chunk == NULL)
    return NULL;
  if ((ret = (BondShapeType *) Chunk_NodeAlloc(bondshape_chunk)) == NULL)
    return NULL;
  *ret = *bsp;

  return ret;
}

/* �ǥ��ȥ饯�� */
static void BondShapeTable_NodeFree(void *vp) {
  if (vp == NULL || bondshape_chunk == NULL)
    return;
  Chunk_NodeFree(bondshape_chunk, vp);
}

/* ���󥹥��󥹤κ��� */
BondShapeTable *BondShapeTable_Alloc(void) {
  if (bondshape_chunk == NULL) {
    if ((bondshape_chunk = Chunk_TypeAlloc(sizeof(BondShapeType),
        BONDSHAPE_CHUNK_SIZE, 0)) == NULL)
      return NULL;
  }
  return Hash_Alloc(0, 0);
}

/* ���󥹥��󥹤ξõ� */
void BondShapeTable_Free(BondShapeTable *bstp) {
  Hash_Free(bstp, BondShapeTable_NodeFree);
}

/* �����������õ�� */
BondShapeType *BondShapeTable_Search(const BondShapeTable *bstp, StringID si) {
  BondShapeType key;

  if (si < 0)
    return NULL;
  BondShapeType_Init(&key, si);
  return (BondShapeType *) Hash_Search(bstp, &key, BondShapeTable_NodeCompare,
    BondShapeTable_NodeHash);
}

/* �������������Ͽ */
BondShapeType *BondShapeTable_Insert(BondShapeTable *bstp, StringID si) {
  BondShapeType atom;

  if (si < 0)
    return NULL;
  BondShapeType_Init(&atom, si);
  return (BondShapeType *) Hash_Insert(bstp, &atom, BondShapeTable_NodeCompare,
    BondShapeTable_NodeHash, BondShapeTable_NodeCopy);
}

/* ����������κ�� */
int BondShapeTable_Delete(BondShapeTable *bstp, StringID si) {
  BondShapeType key;

  if (si < 0)
    return 0;
  BondShapeType_Init(&key, si);
  return Hash_Delete(bstp, &key, BondShapeTable_NodeCompare,
    BondShapeTable_NodeHash, BondShapeTable_NodeFree);
}

/* �ǡ�������� */ 
BondShapeID BondShapeTable_Total(const BondShapeTable *bstp) {
  return Hash_Total(bstp);
}

/* ����������Υꥹ�ȼ��� */
static int BondShapeTable_NodePtrCompare(const void *va, const void *vb) {
  return BondShapeTable_NodeCompare(*((const void **) va),
    *((const void **) vb));
}
void BondShapeTable_List(const BondShapeTable *bstp, BondShapeType **list) {
  BondShapeID n;

  if ((n = BondShapeTable_Total(bstp)) <= 0)
    return;
  Hash_ReadAll(bstp, (void **) list);
  qsort(list, n, sizeof(void *), BondShapeTable_NodePtrCompare);
}

/* ---- BondType�����ơ��֥� ----------------------------------------------- */

/*
 *  BondType�ˤ�3����ΰ�̣�����롣
 *  (si,-1,-1)�Ȥʤ�Ρ��ɤϡ�si���Τ򥢥��������뤿��Υ���ǥå����Ǥ��롣
 *  si <= sj �Ȥʤ�Ρ��ɤϡ����٤Ƥ����Ǥ�ͭ���Ǥ��롣
 *  si > sj �Ȥʤ�Ρ��ɤϡ����ߡ�����(-1�ʤ�)�����롣
 */

/* BondType���ν���� */
void BondType_Init(BondType *bp, StringID si, StringID sj, int lv) {
  bp->si = si;
  bp->sj = sj;
  bp->lv = lv;

  bp->length2 = -1.0;
  bp->sij = -1;

  bp->id = -1;
  bp->bsi = -1;
  bp->count_i = 0;
  bp->count_j = 0;
}

/* BondType�ѥ������� */
#define BOND_CHUNK_SIZE (256*sizeof(BondType))
static Chunk_Type *bond_chunk = NULL; /* ������: ��λ�����Ͼ�ά */

/* ��Ӵؿ�(StringID������դ������ǡ�����0�ʾ���ͤǤ��뤳�Ȥ�����Ȥ���) */
static int BondTable_NodeCompare(const void *va, const void *vb) {
  const BondType *pa, *pb;

  pa = (const BondType *) va;
  pb = (const BondType *) vb;
  if (pa->si != pb->si)
    return (int) (pa->si - pb->si);
  if (pa->sj != pb->sj)
    return (int) (pa->sj - pb->sj);
  return (int) (pa->lv - pb->lv);
}

/* ���ԡ����󥹥ȥ饯�� */
static void *BondTable_NodeCopy(const void *vp) {
  const BondType *bp = (const BondType *) vp;
  BondType *ret;

  if (vp == NULL || bond_chunk == NULL)
    return NULL;
  if ((ret = (BondType *) Chunk_NodeAlloc(bond_chunk)) == NULL)
    return NULL;
  *ret = *bp;

  return ret;
}

/* �ǥ��ȥ饯�� */
static void BondTable_NodeFree(void *vp) {
  if (vp == NULL || bond_chunk == NULL)
    return;
  Chunk_NodeFree(bond_chunk, vp);
}

/* ���󥹥��󥹤κ��� */
BondTable *BondTable_Alloc(void) {
  if (bond_chunk == NULL) {
    if ((bond_chunk = Chunk_TypeAlloc(sizeof(BondType),
        BOND_CHUNK_SIZE, 0)) == NULL)
      return NULL;
  }
  return TAVL_Alloc();
}

/* ���󥹥��󥹤ξõ� */
void BondTable_Free(BondTable *btp) {
  TAVL_Free(btp, BondTable_NodeFree);
}

/* �������õ�� */
BondType *BondTable_Search(const BondTable *btp, StringID si, StringID sj,
    int lv) {
  BondType key;

  BondType_Init(&key, si, sj, lv);
  return (BondType *) TAVL_Search(btp, &key, BondTable_NodeCompare);
}

/* ���������Ͽ */
BondType *BondTable_Insert(BondTable *btp, StringID si, StringID sj, int lv) {
  BondType bond;

  BondType_Init(&bond, si, sj, lv);
  return (BondType *)
    TAVL_Insert(btp, &bond, BondTable_NodeCompare, BondTable_NodeCopy);
}

/* ������κ�� */
int BondTable_Delete(BondTable *btp, StringID si, StringID sj, int lv) {
  BondType key;

  BondType_Init(&key, si, sj, lv);
  return TAVL_Delete(btp, &key, BondTable_NodeCompare, BondTable_NodeFree);
}

/* �ǡ�������� */ 
BondID BondTable_Total(const BondTable *btp) {
  const TAVL_Node *np;
  BondType *bp;
  BondID i = 0;

  np = TAVL_FirstNode(btp);
  while (np != NULL) {
    if ((bp = (BondType *) (np->p)) != NULL && bp->si <= bp->sj)
      i++;
    np = TAVL_NextNode(btp, np);
  }
  return i;
}

/* ������Υꥹ�ȼ��� */
void BondTable_List(const BondTable *btp, BondType **list) {
  const TAVL_Node *np;
  BondType *bp;
  BondID i = 0;

  np = TAVL_FirstNode(btp);
  while (np != NULL) {
    if ((bp = (BondType *) (np->p)) != NULL && bp->si <= bp->sj) {
      list[i] = bp;
      i++;
    }
    np = TAVL_NextNode(btp, np);
  }
}

/* ���η������õ�� */
BondType *BondTable_SearchNext(const BondTable *btp, StringID si, StringID sj,
    int lv) {
  const TAVL_Node *np;
  BondType key;

  BondType_Init(&key, si, sj, lv);
  if ((np = TAVL_SearchNode(btp, &key, BondTable_NodeCompare)) == NULL
      || (np = TAVL_NextNode(btp, np)) == NULL)
    return NULL;
  return (BondType *) (np->p);
}

/* ---- CountType�����ơ��֥� ----------------------------------------------- */

/*
 *  CountType�ˤ�2����ΰ�̣�����롣
 *  ͭ����si���Ф��ơ�(si,-1)�ϰʲ������Ǥ���ġ�
 *    count   ... ���Ԥ�����Υ�����ȿ�
 *    color_l ... ��­�������ο�
 *    color_h ... ���λ��ο�
 *  ͭ����si,sij���Ф��ơ�(si,sij)�ϰʲ������Ǥ���ġ�
 *    count   ... ���1�Ĥ�����Υ�����ȿ�
 *  ����ʳ����ͤϤ��٤�-1�Ȥʤ롣
 */

/* CountType���ν���� */
static void CountType_Init(CountType *cp, StringID si, StringID sij) {
  cp->si = si;
  cp->sij = sij;
  cp->count = 0;
  cp->color_l = -1;
  cp->color_h = -1;
}

/* CountType�ѥ������� */
#define COUNT_CHUNK_SIZE (256*sizeof(CountType))
static Chunk_Type *count_chunk = NULL; /* ������: ��λ�����Ͼ�ά */

/* ��Ӵؿ�(StringID������դ������ǡ�����0�ʾ���ͤǤ��뤳�Ȥ�����Ȥ���) */
static int CountTable_NodeCompare(const void *va, const void *vb) {
  const CountType *pa, *pb;

  pa = (const CountType *) va;
  pb = (const CountType *) vb;
  if (pa->si != pb->si)
    return (int) (pa->si - pb->si);
  return (int) (pa->sij - pb->sij);
}

/* ���ԡ����󥹥ȥ饯�� */
static void *CountTable_NodeCopy(const void *vp) {
  const CountType *cp = (const CountType *) vp;
  CountType *ret;

  if (vp == NULL || count_chunk == NULL)
    return NULL;
  if ((ret = (CountType *) Chunk_NodeAlloc(count_chunk)) == NULL)
    return NULL;
  *ret = *cp;

  return ret;
}

/* �ǥ��ȥ饯�� */
static void CountTable_NodeFree(void *vp) {
  if (vp == NULL || count_chunk == NULL)
    return;
  Chunk_NodeFree(count_chunk, vp);
}

/* ���󥹥��󥹤κ��� */
CountTable *CountTable_Alloc(void) {
  if (count_chunk == NULL) {
    if ((count_chunk = Chunk_TypeAlloc(sizeof(CountType),
        COUNT_CHUNK_SIZE, 0)) == NULL)
      return NULL;
  }
  return TAVL_Alloc();
}

/* ���󥹥��󥹤ξõ� */
void CountTable_Free(CountTable *ctp) {
  TAVL_Free(ctp, CountTable_NodeFree);
}

/* �����������Ͽ */
CountType *CountTable_Insert(CountTable *ctp, StringID si, StringID sij) {
  CountType count;

  CountType_Init(&count, si, sij);
  return (CountType *)
    TAVL_Insert(ctp, &count, CountTable_NodeCompare, CountTable_NodeCopy);
}

/* ���������õ�� */
CountType *CountTable_Search(const CountTable *ctp, StringID si,
    StringID sij) {
  CountType key;

  CountType_Init(&key, si, sij);
  return (CountType *) TAVL_Search(ctp, &key, CountTable_NodeCompare);
}

/* ��������κ�� */
int CountTable_Delete(CountTable *ctp, StringID si, StringID sij) {
  CountType key;

  CountType_Init(&key, si, sij);
  return TAVL_Delete(ctp, &key, CountTable_NodeCompare, CountTable_NodeFree);
}

/* �ǡ�������� */ 
CountID CountTable_Total(const CountTable *ctp) {
  const TAVL_Node *np;
  CountType *cp;
  CountID i = 0;

  np = TAVL_FirstNode(ctp);
  while (np != NULL && (cp = (CountType *) (np->p)) != NULL) {
    i++;
    np = TAVL_NextNode(ctp, np);
  }
  return i;
}

/* ��������Υꥹ�ȼ��� */
void CountTable_List(const CountTable *ctp, CountType **list) {
  const TAVL_Node *np;
  CountType *cp;
  CountID i = 0;

  np = TAVL_FirstNode(ctp);
  while (np != NULL && (cp = (CountType *) (np->p)) != NULL) {
    list[i] = cp;
    i++;
    np = TAVL_NextNode(ctp, np);
  }
}

/* ���η��������õ�� */
CountType *CountTable_SearchNext(const CountTable *ctp, StringID si,
    StringID sij) {
  const TAVL_Node *np;
  CountType key;

  CountType_Init(&key, si, sij);
  if ((np = TAVL_SearchNode(ctp, &key, CountTable_NodeCompare)) == NULL
      || (np = TAVL_NextNode(ctp, np)) == NULL)
    return NULL;
  return (CountType *) (np->p);
}

/* ---- AtomPairType�����ơ��֥� ------------------------------------------- */

/* AtomPairType�ѥ������� */
#define ATOMPAIR_CHUNK_SIZE (256*sizeof(AtomPairType))
static Chunk_Type *atompair_chunk = NULL; /* ������: ��λ�����Ͼ�ά */

/* AtomPairType���ν���� */
static void AtomPairType_Init(AtomPairType *app, AtomID ai, AtomID aj) {
  app->ai = ai;
  app->aj = aj;
  app->bl = -1;
  app->br = -1;
}

/* ��Ӵؿ�(AtomID������դ������ǡ�����0�ʾ���ͤǤ��뤳�Ȥ�����Ȥ���) */
static int AtomPairTable_NodeCompare(const void *va, const void *vb) {
  const AtomPairType *pa, *pb;

  pa = (const AtomPairType *) va;
  pb = (const AtomPairType *) vb;
  if (pa->ai != pb->ai)
    return (int) (pa->ai - pb->ai);
  return (int) (pa->aj - pb->aj);
}

/* �ϥå���ؿ� */
static Hash_Size AtomPairTable_NodeHash(const void *vp) {
  const AtomPairType *app = (const AtomPairType *) vp;
  unsigned long x; /* 32bit�ʾ������ */

  x = (unsigned long) app->ai;
  x ^= (((x&0xFFFFUL)*0x9C1F7ED1UL) & 0xFFFF0000UL) ^ 0x68270207UL;
  x ^= (((x>>16)*0x834B97C5UL)>>16) ^ 0xCAB1300BUL  ^ (unsigned long) app->aj;

  return (Hash_Size) x;
}

/* ���ԡ����󥹥ȥ饯�� */
static void *AtomPairTable_NodeCopy(const void *vp) {
  const AtomPairType *app = (const AtomPairType *) vp;
  AtomPairType *ret;

  if (vp == NULL || atompair_chunk == NULL)
    return NULL;
  if ((ret = (AtomPairType *) Chunk_NodeAlloc(atompair_chunk)) == NULL)
    return NULL;
  *ret = *app;

  return ret;
}

/* �ǥ��ȥ饯�� */
static void AtomPairTable_NodeFree(void *vp) {
  if (vp == NULL || atompair_chunk == NULL)
    return;
  Chunk_NodeFree(atompair_chunk, vp);
}

/* ���󥹥��󥹤κ��� */
AtomPairTable *AtomPairTable_Alloc(void) {
  if (atompair_chunk == NULL) {
    if ((atompair_chunk = Chunk_TypeAlloc(sizeof(AtomPairType),
        ATOMPAIR_CHUNK_SIZE, 0)) == NULL)
      return NULL;
  }
  return Hash_Alloc(0, 0);
}

/* ���󥹥��󥹤ξõ� */
void AtomPairTable_Free(AtomPairTable *aptp) {
  Hash_Free(aptp, AtomPairTable_NodeFree);
}

/* �������õ�� */
AtomPairType *AtomPairTable_Search(const AtomPairTable *aptp, AtomID ai,
    AtomID aj) {
  AtomPairType key;

  if (ai < 0 || aj < 0)
    return NULL;
  AtomPairType_Init(&key, ai, aj);
  return (AtomPairType *) Hash_Search(aptp, &key, AtomPairTable_NodeCompare,
    AtomPairTable_NodeHash);
}

/* ���������Ͽ */
AtomPairType *AtomPairTable_Insert(AtomPairTable *aptp, AtomID ai, AtomID aj) {
  AtomPairType atompair;

  if (ai < 0 || aj < 0)
    return NULL;
  AtomPairType_Init(&atompair, ai, aj);
  return (AtomPairType *) Hash_Insert(aptp, &atompair,
    AtomPairTable_NodeCompare, AtomPairTable_NodeHash, AtomPairTable_NodeCopy);
}

/* ������κ�� */
int AtomPairTable_Delete(AtomPairTable *aptp, AtomID ai, AtomID aj) {
  AtomPairType key;

  if (ai < 0 || aj < 0)
    return 0;
  AtomPairType_Init(&key, ai, aj);
  return Hash_Delete(aptp, &key, AtomPairTable_NodeCompare,
    AtomPairTable_NodeHash, AtomPairTable_NodeFree);
}

/* �ǡ�������� */ 
AtomPairID AtomPairTable_Total(const AtomPairTable *aptp) {
  return Hash_Total(aptp);
}

/* ������Υꥹ�ȼ��� */
static int AtomPairTable_NodePtrCompare(const void *va, const void *vb) {
  return AtomPairTable_NodeCompare(*((const void **) va),
    *((const void **) vb));
}
void AtomPairTable_List(const AtomPairTable *aptp, AtomPairType **list) {
  AtomPairID n;

  if ((n = AtomPairTable_Total(aptp)) <= 0)
    return;
  Hash_ReadAll(aptp, (void **) list);
  qsort(list, n, sizeof(void *), AtomPairTable_NodePtrCompare);
}

