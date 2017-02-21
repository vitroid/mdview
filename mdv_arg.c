/* mdv_arg.c */

#include <stdio.h>
#include <stdlib.h> /* strtod(), qsort(), bsearch() */
#include <string.h> /* strlen(), strcpy(), strchr(), strcmp(), strncmp() */
#include <ctype.h>  /* isspace(), isalpha(), isupper(), islower() */
#include <math.h>   /* atof(), fabs() */
#include <sys/stat.h> /* stat() */
#include <stdarg.h>
#include "mdview.h"
#include "mdv.h"
#include "mdv_form.h"
#include "mdv_farg.h"
#include "hash.h"
#include "tavltree.h"
#include "chunk.h"

/* ==== 引数処理 =========================================================== */

/* 大域変数 */
char *file_path = NULL;
int debug_mode = 0;
int formatted_mode = 1;
int color_level = 1;
MDV_Size max_particle = 0;
int terminate_blank_line = 0;
int povray_version = 0;

/* 原子、結合表 */
void EnterAtom(MDV_Atom *map, const char * const argv[],
  const MDV_Status *msp);
void EraseAtom(MDV_Atom *map, const char * const argv[]);
void EnterBond(MDV_Atom *map, const char * const argv[],
  const MDV_Status *msp);
void EraseBond(MDV_Atom *map, const char * const argv[]);
void EnterBondShape(MDV_Atom *map, const char * const argv[],
  const MDV_Status *msp);
void EnterCount(MDV_Atom *map, const char * const argv[],
  const MDV_Status *msp);
void EraseCount(MDV_Atom *map, const char * const argv[]);
void EnterCountN(MDV_Atom *map, const char * const argv[]);

/* 粒子表関係 */
int SetFormat(char *form);
MDV_Size TotalAtoms(void);
void InitAtom(void);

/* 引数へのアクセス */
void ArgvInit(const char * const argv[]);
const char *ArgvRead(void);
const char *ArgvShift(void);
void ArgvSetCurrent(void);
void ArgError(const char *format, ...);

/* その他 */
void Usage(void);
void Version(void);

/* ---- ArgumentID関連 ----------------------------------------------------- */

/* 文字列の引数への対応 */
typedef struct {
  const char *str; /* 引数文字列 */
  ArgumentID arg;  /* 対応する引数 */
} ArgStrType;

static ArgStrType arg_list[] = {
  {"-"           ,  ARG_CANCEL       },

  {"au"          ,  ARG_ATOMIC_UNIT  },
  {"ang"         ,  ARG_ANGSTROM     },
  {"arg"         ,  ARG_ARG_VERSION  },
  {"atom"        ,  ARG_ATOM         },
  {"atom-"       , _ARG_ATOM         },
  {"bg"          ,  ARG_BACKGROUND   },
  {"blank_line"  ,  ARG_BLANK_LINE   },
  {"blank_line-" , _ARG_BLANK_LINE   },
  {"bond"        ,  ARG_BOND         },
  {"bond-"       , _ARG_BOND         },
  {"bond_shape"  ,  ARG_BOND_SHAPE   },
  {"center"      ,  ARG_CENTER       },
  {"center-"     , _ARG_CENTER       },
  {"clear"       ,  ARG_CLEAR_ATOM   },
  {"clear-"      , _ARG_CLEAR_ATOM   },
  {"color"       ,  ARG_COLOR_LEVEL  },
  {"count"       ,  ARG_COUNT        },
  {"count-"      , _ARG_COUNT        },
  {"count_n"     ,  ARG_COUNT_N      },
  {"euler"       ,  ARG_EULER_ANGLE  },
  {"fold"        ,  ARG_FOLD         },
  {"fold-"       , _ARG_FOLD         },
  {"format"      ,  ARG_FORMAT       },
  {"format-"     , _ARG_FORMAT       },
  {"frame"       ,  ARG_FRAMES       },
  {"help"        ,  ARG_HELP         },
  {"-help"       ,  ARG_HELP         },
  {"idle"        ,  ARG_IDLE_CALL    },
  {"idle-"       , _ARG_IDLE_CALL    },
  {"layer"       ,  ARG_LAYER        },
  {"layer-"      , _ARG_LAYER        },
  {"visible"     ,  ARG_LAYER_VISIBLE},
  {"layer_kill"  ,  ARG_LAYER_KILL   },
  {"length"      ,  ARG_LENGTH       },
  {"length-"     , _ARG_LENGTH       },
  {"loop"        ,  ARG_LOOP         },
  {"loop-"       , _ARG_LOOP         },
  {"mark"        ,  ARG_MARK         },
  {"max"         ,  ARG_MAX_PARTICLE },
  {"multi"       ,  ARG_MARK_MULTI   },
  {"multi-"      , _ARG_MARK_MULTI   },
  {"mol3"        ,  ARG_CONVERT_MOL3 },
  {"povray"      ,  ARG_POVRAY       },
  {"range"       ,  ARG_RANGE        },
  {"specify"     ,  ARG_MARK         },
  {"version"     ,  ARG_VERSION      },
  {"-version"    ,  ARG_VERSION      },
  {"window"      ,  ARG_WINDOW_SIZE  },
  {"c"           ,  ARG_CLEAR_ATOM   },
  {"c-"          , _ARG_CLEAR_ATOM   },
  {"d"           ,  ARG_DISTANCE     },
  {"e"           ,  ARG_EULER_ANGLE  },
  {"p"           ,  ARG_MARK         },
  {"w"           ,  ARG_WINDOW_SIZE  },

  /* 最後の要素はNULLとなる */
  {NULL          ,  ARG_UNKNOWN      }
};

static int init_arg_list = 0;
static int arg_list_n = (sizeof(arg_list)/sizeof(ArgStrType)) - 1;

static int ArgStrCompare(const void *pa, const void *pb) {
  return strcmp(((const ArgStrType *) pa)->str,
    ((const ArgStrType *) pb)->str);
}

/* 文字列を読んで引数の識別子を返す */
ArgumentID GetArgumentID(const char *str) {
  ArgStrType tmp, *parg;

  if (str[0] != '-' || (!isalpha(str[1]) && str[1] != '-'))
    return ARG_ILLEGAL;

  /* 探索のための初期化 */
  if (!init_arg_list) {
    qsort(arg_list, arg_list_n, sizeof(ArgStrType), ArgStrCompare);
    init_arg_list = 1;
  }
  tmp.str = &str[1];
  tmp.arg = ARG_UNKNOWN;

  /* ２分探索 */
  parg = bsearch(&tmp, arg_list, arg_list_n, sizeof(ArgStrType),
    ArgStrCompare);

  return (parg != NULL)? parg->arg: ARG_UNKNOWN;
}

/* コマンド引数とそれに続く引数の数の対応 */
int GetArgumentSize(ArgumentID arg, const MDV_Status *msp) {
  int ret;

  switch (arg) {
  case  ARG_ATOMIC_UNIT:  ret = 0; break;
  case  ARG_ANGSTROM:     ret = 0; break;
  case  ARG_ARG_VERSION:  ret = 1; break;
  case  ARG_ATOM:         ret = 4; break;
  case _ARG_ATOM:         ret = 1; break;
  case  ARG_BACKGROUND:   ret = 1; break;
  case  ARG_BLANK_LINE:   ret = 0; break;
  case _ARG_BLANK_LINE:   ret = 0; break;
  case  ARG_BOND:
    if (msp->arg_version < 23)
      ret = 3;
    else if (msp->arg_version < 60)
      ret = 4;
    else
      ret = 3;
    break;
  case _ARG_BOND:         ret = 1; break;
  case  ARG_BOND_SHAPE:   ret = 3; break;
  case  ARG_CENTER:       ret = 3; break;
  case _ARG_CENTER:       ret = 1; break;
  case  ARG_CLEAR_ATOM:   ret = 0; break;
  case _ARG_CLEAR_ATOM:   ret = 0; break;
  case  ARG_COLOR_LEVEL:  ret = 1; break;
  case  ARG_CONVERT_MOL3: ret = 0; break;
  case  ARG_COUNT:
    if (msp->arg_version < 60)
      ret = 5;
    else
      ret = 4;
    break;
  case _ARG_COUNT:        ret = 1; break;
  case  ARG_COUNT_N:      ret = 3; break;
  case  ARG_DISTANCE:     ret = 1; break;
  case  ARG_EULER_ANGLE:  ret = 3; break;
  case  ARG_FOLD:         ret = 0; break;
  case _ARG_FOLD:         ret = 0; break;
  case  ARG_FORMAT:       ret = 1; break;
  case _ARG_FORMAT:       ret = 0; break;
  case  ARG_FRAMES:       ret = 1; break;
  case  ARG_HELP:         ret = 0; break;
  case  ARG_IDLE_CALL:    ret = 0; break;
  case _ARG_IDLE_CALL:    ret = 0; break;
  case  ARG_LAYER:        ret = 2; break;
  case _ARG_LAYER:        ret = 1; break;
  case  ARG_LAYER_VISIBLE:ret = 1; break;
  case  ARG_LAYER_KILL:   ret = 0; break;
  case  ARG_LENGTH:       ret = 1; break;
  case _ARG_LENGTH:       ret = 0; break;
  case  ARG_LOOP:         ret = 0; break;
  case _ARG_LOOP:         ret = 0; break;
  case  ARG_MARK:         ret = 1; break;
  case  ARG_MARK_MULTI:   ret = 0; break;
  case _ARG_MARK_MULTI:   ret = 0; break;
  case  ARG_MAX_PARTICLE: ret = 1; break;
  case  ARG_POVRAY:       ret = 1; break;
  case  ARG_RANGE:        ret = 1; break;
  case  ARG_VERSION:      ret = 0; break;
  case  ARG_WINDOW_SIZE:  ret = 1; break;
  default:                ret = -1;
  }

  return ret;
};

#define MAXOPTARGS 5              /* オプション引数の最大数 */
static char *optargv[MAXOPTARGS]; /* オプション引数へのポインタ配列 */
/* オプション引数の読み込み(返値は成功の真偽) */
char **SetOptArgv(int argc, const char *(*f_getarg)(void)) {
  static int init = 0;
  static MDV_Array *buf[MAXOPTARGS]; /* 暫定版: 解放しない */
  int i;
  const char *s;
  ArgumentID t;

  if (!init) {
    for (i = 0; i < MAXOPTARGS; i++) {
      buf[i] = MDV_Array_Alloc(1);
      MDV_Array_SetSize(buf[i], 0);
      MDV_Array_AppendChar(buf[i], '\0');
      optargv[i] = (char *) buf[i]->p;
    }
    init = 1;
  }

  if (argc > MAXOPTARGS) {
    fprintf(stderr, "SetOptArgv(): Illegal argument detected.\n");
    exit(1);
  }

  for (i = 0; i < argc; i++) {
    if ((s = f_getarg()) == NULL)
      break;
    t = GetArgumentID(s);
    if (t == ARG_CANCEL) {
      if ((s = f_getarg()) == NULL)
        break;
    } else if (t != ARG_ILLEGAL)
      break;

    MDV_Array_SetSize(buf[i], 0);
    do {
      MDV_Array_AppendChar(buf[i], *s);
    } while (*s++ != '\0');
    optargv[i] = (char *) buf[i]->p;
  }
  if (i < argc)
    return NULL;

  return optargv;
}

/* ---- 引数解釈関係 ------------------------------------------------------- */

/* 数値の読み込み */
static int my_err = 0; /* NUL文字か空白文字で終わっていない場合はエラー */
static double my_atof(const char *str) {
  char *end;
  double ret;

  ret = strtod(str, &end);
  my_err = (*end == '\0' || isspace(*end))? 0: 1;
  return ret;
}
static long my_atol(const char *str) {
  char *end;
  long ret;

  ret = strtol(str, &end, 10);
  my_err = (*end == '\0' || isspace(*end))? 0: 1;
  return ret;
}
#define my_atoi(str) ((int) my_atol(str))

/* MDV_Status型の更新(返値は成功の真偽) */
int MDV_StatusUpdate(MDV_Status *msp, ArgumentID arg, const char *str_arg,
    char * optargv[]) {
  switch (arg) {
  case ARG_ATOMIC_UNIT:
    msp->length_unit = 0.529177249;
    break;
  case ARG_ANGSTROM:
    msp->length_unit = 1.0;
    break;
  case ARG_CENTER:
    msp->center_auto = 0;
    msp->center_coord.x = my_atof(optargv[0]) * msp->length_unit;
    if (my_err)
      ArgError("%s: x = %s is not supported.", str_arg, optargv[0]);
    msp->center_coord.y = my_atof(optargv[1]) * msp->length_unit;
    if (my_err)
      ArgError("%s: y = %s is not supported.", str_arg, optargv[1]);
    msp->center_coord.z = my_atof(optargv[2]) * msp->length_unit;
    if (my_err)
      ArgError("%s: z = %s is not supported.", str_arg, optargv[2]);
    break;
  case _ARG_CENTER:
    msp->center_auto = 1;
    break;
  case ARG_DISTANCE:
    if ((msp->distance = my_atof(optargv[0])) <= 0 || my_err)
      ArgError("%s: %s is not supported.", str_arg, optargv[0]);
    break;
  case ARG_EULER_ANGLE:
    if ((msp->euler_theta = my_atof(optargv[0])) < 0.0
        || msp->euler_theta > 180.0
        || my_err)
      ArgError("%s: theta = %s (deg.) is not supported.", str_arg, optargv[0]);
    msp->euler_psi = my_atof(optargv[1]);
    if (my_err)
      ArgError("%s: psi = %s (deg.) is not supported.", str_arg, optargv[1]);
    msp->euler_phi = my_atof(optargv[2]);
    if (my_err)
      ArgError("%s: phi = %s (deg.) is not supported.", str_arg, optargv[2]);
    break;
  case ARG_FOLD:
    msp->fold_mode = 1;
    break;
  case _ARG_FOLD:
    msp->fold_mode = 0;
    break;
  case ARG_LAYER:
    AddNewLayer(optargv[0], optargv[1]);
    break;
  case _ARG_LAYER:
    EraseLayer(optargv[0]);
    break;
  case ARG_LAYER_VISIBLE:
    SetLayerStatus(optargv[0]);
    break;
  case ARG_LAYER_KILL:
    KillLayers();
    break;
  case ARG_LENGTH:
    if (optargv[0][0] != '(') {
      /* length */
      double length;

      if ((length = my_atof(optargv[0])) < 0.0 || my_err)
        ArgError("%s: %s is not supported.", str_arg, optargv[0]);
      length *= msp->length_unit;
      MDV_Status_SetPeriodLength1(msp, length);
    } else {
      MDV_3D vx, vy, vz;
      char *endp;

      endp = optargv[0]+1;
      vx.x = strtod(endp, &endp);
      while (isspace(*endp))
        endp++;
      if (*endp++ != ',')
        ArgError("%s: %s is not supported.", str_arg, optargv[0]);
      vx.y = strtod(endp, &endp);
      while (isspace(*endp))
        endp++;
      if (*endp++ != ',')
        ArgError("%s: %s is not supported.", str_arg, optargv[0]);
      vx.z = strtod(endp, &endp);
      while (isspace(*endp))
        endp++;
      if (*endp == ')') {
        /* (x, y, z) */
        vx.x *= msp->length_unit;
        vx.y *= msp->length_unit;
        vx.z *= msp->length_unit;
        MDV_Status_SetPeriodLength3(msp, vx.x, vx.y, vx.z);
      } else {
        /* (xx, xy, xz, yx, yy, yz, zx, zy, zz) */
        if (*endp++ != ',')
          ArgError("%s: %s is not supported.", str_arg, optargv[0]);
        vy.x = strtod(endp, &endp);
        while (isspace(*endp))
          endp++;
        if (*endp++ != ',')
          ArgError("%s: %s is not supported.", str_arg, optargv[0]);
        vy.y = strtod(endp, &endp);
        while (isspace(*endp))
          endp++;
        if (*endp++ != ',')
          ArgError("%s: %s is not supported.", str_arg, optargv[0]);
        vy.z = strtod(endp, &endp);
        while (isspace(*endp))
          endp++;
        if (*endp++ != ',')
          ArgError("%s: %s is not supported.", str_arg, optargv[0]);
        vz.x = strtod(endp, &endp);
        while (isspace(*endp))
          endp++;
        if (*endp++ != ',')
          ArgError("%s: %s is not supported.", str_arg, optargv[0]);
        vz.y = strtod(endp, &endp);
        while (isspace(*endp))
          endp++;
        if (*endp++ != ',')
          ArgError("%s: %s is not supported.", str_arg, optargv[0]);
        vz.z = strtod(endp, &endp);
        while (isspace(*endp))
          endp++;
        if (*endp++ != ')')
          ArgError("%s: %s is not supported.", str_arg, optargv[0]);
        vx.x *= msp->length_unit;
        vx.y *= msp->length_unit;
        vx.z *= msp->length_unit;
        vy.x *= msp->length_unit;
        vy.y *= msp->length_unit;
        vy.z *= msp->length_unit;
        vz.x *= msp->length_unit;
        vz.y *= msp->length_unit;
        vz.z *= msp->length_unit;
        MDV_Status_SetPeriodLength9(msp, vx.x, vx.y, vx.z,
          vy.x, vy.y, vy.z, vz.x, vz.y, vz.z);
      }
    }
    break;
  case _ARG_LENGTH:
    MDV_Status_SetPeriodLength1(msp, 0.0);
    break;
  case ARG_RANGE:
    if ((msp->max_radius = my_atof(optargv[0])) < 0.0 || my_err)
      ArgError("%s: %s is not supported.", str_arg, optargv[0]);
    msp->max_radius *= msp->length_unit;
    break;
  default:
    return 0;
  }

  return 1;
}

/* 引数を読んで各種初期設定を行なう */
void MDV_StatusSet(MDV_Atom *map, MDV_Status *msp, char **argv) {
  ArgumentID arg;
  const char *p;
  char str_arg[MAXARGLEN];
  char *form = NULL;
  char **optargv;
  int ret;

  /* 引数読み込みシステムの初期化 */
  ArgvInit((const char * const *) argv);

  /* default設定 */
  MDV_StatusClear(msp);

  if ((p = ArgvShift()) == NULL) Usage();
  debug_mode <<= 1; /* 引数表示モード開始 */

  while ((p = ArgvRead()) != NULL && (arg = GetArgumentID(p)) != ARG_ILLEGAL) {
    ArgvSetCurrent();
    p = ArgvShift();
    if (Strlcpy(str_arg, p, MAXARGLEN) >= MAXARGLEN)
      str_arg[MAXARGLEN-2] = '*';

    ret = GetArgumentSize(arg, msp);
    if (ret < 0)
      ArgError("Illegal Argument.");
    if ((optargv = SetOptArgv(ret, ArgvShift)) == NULL)
      ArgError("%s: Too few argument(s).", str_arg);

    switch (arg) {
    case ARG_ARG_VERSION:
      msp->arg_version = (int) ((my_atof(optargv[0])-2.999)*100.0);
      if (msp->arg_version < 0
          || msp->arg_version > atoi(MDV_REVISION) || my_err)
        ArgError("%s: \"%s\" is not supported.", str_arg, optargv[0]);
      break;
    case ARG_ATOM:
      EnterAtom(map, (const char * const *) optargv, msp);
      break;
    case _ARG_ATOM:
      EraseAtom(map, (const char * const *) optargv);
      break;
    case ARG_BACKGROUND:
      if ((msp->background_color = Str2ColorStringID(optargv[0])) < 0)
        ArgError("%s: \"%s\" is not supported.", str_arg, optargv[0]);
      break;
    case ARG_BLANK_LINE:
      terminate_blank_line = 1;
      break;
    case _ARG_BLANK_LINE:
      terminate_blank_line = 0;
      break;
    case ARG_BOND:
      EnterBond(map, (const char * const *) optargv, msp);
      break;
    case _ARG_BOND:
      EraseBond(map, (const char * const *) optargv);
      break;
    case ARG_BOND_SHAPE:
      EnterBondShape(map, (const char * const *) optargv, msp);
      break;
    case ARG_CLEAR_ATOM:
      msp->outline_mode = 1;
      break;
    case _ARG_CLEAR_ATOM:
      msp->outline_mode = 0;
      break;
    case ARG_COLOR_LEVEL:
      if ((color_level = my_atoi(optargv[0])) < 0
          || color_level > MAXCOLORLEVEL || my_err)
        ArgError("%s: %s is not supported.", str_arg, optargv[0]);
      break;
    case ARG_CONVERT_MOL3:
      ArgError("%s: Sorry, not supported.", str_arg);
      break;
    case ARG_COUNT:
      EnterCount(map, (const char * const *) optargv, msp);
      break;
    case _ARG_COUNT:
      EraseCount(map, (const char * const *) optargv);
      break;
    case ARG_COUNT_N:
      EnterCountN(map, (const char * const *) optargv);
      break;
    case ARG_FORMAT:
      if (form != NULL) free(form);
      if ((form = (char *) malloc(strlen(optargv[0])+1)) == NULL)
        HeapError();
      strcpy(form, optargv[0]);
      formatted_mode = 1;
      break;
    case _ARG_FORMAT:
      if (form != NULL) {free(form); form = NULL;}
      formatted_mode = 0;
      break;
    case ARG_FRAMES:
      if (get_frame_id(msp->frames = my_atoi(optargv[0])) < 0 || my_err)
        ArgError("%s: %s is not supported.", str_arg, optargv[0]);
      break;
    case ARG_IDLE_CALL:
      use_idle_callback = 1;
      break;
    case _ARG_IDLE_CALL:
      use_idle_callback = 0;
      break;
    case ARG_LOOP:
      loop_mode = 1;
      break;
    case _ARG_LOOP:
      loop_mode = 0;
      break;
    case ARG_MARK_MULTI:
      msp->mark_mode = MARK_MULTIPLE;
      break;
    case _ARG_MARK_MULTI:
      msp->mark_mode = MARK_SINGLE;
      break;
    case ARG_MARK:
      if ((msp->mark_color = Str2ColorStringID(optargv[0])) < 0)
        ArgError("%s: \"%s\" is not supported.", str_arg, optargv[0]);
      break;
    case ARG_MAX_PARTICLE:
      if ((max_particle = my_atol(optargv[0])) < 0 || my_err)
        ArgError("%s: %s is not supported.", str_arg, optargv[0]);
      break;
    case ARG_POVRAY:
      if (optargv[0][0] != '3' || optargv[0][1] != '.'
          || (povray_version = my_atoi(optargv[0]+2)) < 0 || my_err)
        ArgError("%s: \"%s\" is not supported.", str_arg, optargv[0]);
      break;
    case ARG_VERSION:
      Version();
      break;
    case ARG_WINDOW_SIZE:
      if ((msp->window_size = my_atoi(optargv[0])) < 100
          || msp->window_size > 1200 || my_err)
        ArgError("%s: %s is not supported.", str_arg, optargv[0]);
      break;
    default:
      /* 更新可能コマンド引数の処理 */
      if (!MDV_StatusUpdate(msp, arg, str_arg, optargv))
        Usage();
    }
  }
  debug_mode >>= 1; /* 引数表示モード終了 */
  if (debug_mode) {fputc('\n', stderr);}
  ArgvSetCurrent();

  /* 色設定の確認 */
  if (msp->background_color < 0)
    ArgError("-bg option is not found.");
  if (msp->mark_color < 0)
    ArgError("-mark option is not found.");

  /* atom[]初期化 */
  if (formatted_mode) {
    if (form == NULL)
      ArgError("-format option is not found.");
    ret = SetFormat(form);
    free(form); form = NULL;
  } else {
    InitAtom();
  }

  /* 原子関連情報のデバッグ出力 */
  if (debug_mode)
    MDV_Atom_DebugOutput(map, stderr);

  /* データファイルパスの設定 */
  if ((p = ArgvShift()) == NULL || ArgvShift() != NULL)
    Usage();
  if (file_path != NULL) free(file_path);
  if ((file_path = (char *) malloc(strlen(p)+1)) == NULL)
    HeapError();
  strcpy(file_path, p);

  return;
}

/* ---- 原子関連情報の設定 ------------------------------------------------- */

/* 原子名文字列をstr[]から切り出す。str[]は'\0'で終わっていなくとも良い。 */
static const char *GetAtomString(const char *str) {
  static MDV_Array *p = NULL; /* 暫定版: 解放しない */
  int i;

  if (p == NULL)
    p = MDV_Array_Alloc(1);

  /* 原子名と思われる部分のみを代入し、'\0'で終端する */
  i = 0;
  if (!isupper(str[0]))
    return NULL;
  MDV_Array_SetSize(p, 0);
  MDV_Array_AppendChar(p, *str++);
  while (islower(*str))
    MDV_Array_AppendChar(p, *str++);
  MDV_Array_AppendChar(p, '\0');

  return (const char *) p->p;
}

/* 原子名のチェック(大文字から始まり、小文字が任意個続く) */
static int IsAtomStr(const char *str) {
  if (!isupper(str[0])) return -1;
  str++;
  while (*str != '\0' && islower(*str))
    str++;
  return (*str == '\0');
}

/* 原子の登録(原子名, 質量, 半径, 色) */
void EnterAtom(MDV_Atom *map, const char * const argv[],
    const MDV_Status *msp) {
  StringID si;
  double mass, radius;
  StringID color;

  /* 原子名のチェック */
  if (!IsAtomStr(argv[0]))
    ArgError("-atom: Illegal format.");

  /* 原子識別番号 */
  if ((si = Str2StringID(GetAtomString(argv[0]))) < 0)
    MDV_Fatal("EnterAtom()");

  /* 質量 */
  if ((mass = my_atof(argv[1])) <= 0.0 || my_err)
    ArgError("-atom: Illegal mass = %s.", argv[1]);

  /* 半径 */
  if ((radius = my_atof(argv[2]) * msp->length_unit) < 0 || my_err)
    ArgError("-atom: Illegal radius = %s.", argv[2]);

  /* 色ID */
  if ((color = Str2ColorStringID(argv[3])) < 0)
    ArgError("-atom: Illegal color = %s.", argv[3]);

  /* 登録 */
  if (!MDV_Atom_EnterAtom(map, si, mass, radius, color))
    MDV_Fatal("EnterAtom()");
}

/* 原子の削除(原子名) */
void EraseAtom(MDV_Atom *map, const char * const argv[]) {
  StringID si;

  /* 削除 */
  if ((si = SearchStringID(GetAtomString(argv[0]))) < 0
      || !MDV_Atom_EraseAtom(map, si))
    ArgError("-atom-: Illegal format.");
}

/* 文字列から結合形状を示す文字列を生成する */
#define BONDSHAPE_MANUAL_CHAR ' '
StringID SearchBondShapeStringID(const char *str) {
  char *work;
  int len;
  StringID sij;

  len = 1 + strlen(str) + 1;
  work = MDV_Work_Alloc(len);
  work[0] = BONDSHAPE_MANUAL_CHAR;
  work[1] = '\0';
  Strlcat(work, str, len);
  sij = SearchStringID(work);
  MDV_Work_Free(work);

  return sij;
}
StringID Str2BondShapeStringID(const char *str) {
  char *work;
  int len;
  StringID sij;

  len = 1 + strlen(str) + 1;
  work = MDV_Work_Alloc(len);
  work[0] = BONDSHAPE_MANUAL_CHAR;
  work[1] = '\0';
  Strlcat(work, str, len);
  sij = Str2StringID(work);
  MDV_Work_Free(work);

  return sij;
}

/* 結合の登録(原子名-原子名, 長さ, 結合形状名) */
#define _min(x, y) ((x)<=(y)?(x):(y))
void EnterBond(MDV_Atom *map, const char * const argv[],
    const MDV_Status *msp) {
  StringID si, sj, sij;
  const AtomType *aip = NULL;
  const AtomType *ajp = NULL;
  StringID color;
  int k, lv;
  double length, length2, radius;
  const char **pargv = (const char **) argv;

  /* 原子si */
  if ((si = SearchStringID(GetAtomString(argv[0]))) < 0
      || (aip = MDV_Atom_SearchAtom(map, si)) == NULL)
    ArgError("-bond: Unknown atom %s is found. (left side)", argv[0]);

  /* 結合レベルlv */
  for (k = 0; argv[0][k] != '\0' && argv[0][k] != '-'; k++)
    ;
  if (argv[0][k] == '\0')
    ArgError("-bond: Illegal format.");
  for (lv = 0; argv[0][k+lv] == '-'; lv++)
    ;
  lv--;
  if (lv < 0 || lv >= MAXBONDTYPE)
    ArgError("-bond %s: Illegal bond level = %d.", argv[0], lv+1);

  /* 原子sj */
  if ((sj = SearchStringID(argv[0]+k+lv+1)) < 0
      || (ajp = MDV_Atom_SearchAtom(map, sj)) == NULL)
    ArgError("-bond: Unknown atom %s is found. (right side)", argv[0]);

  /* 長さ */
  length = my_atof(argv[1]) * msp->length_unit;
  if (my_err)
    ArgError("-bond: Illegal length = %s.", argv[1]);
  length2 = length*length;

  if (msp->arg_version < 60) {
    /* 半径 */
    if (msp->arg_version < 23) {
      /* 従来の方法 */
      radius = _min(aip->radius, ajp->radius)*0.3;
    } else {
      /* 引数から設定 */
      radius = my_atof(argv[2]) * msp->length_unit;
      if (my_err)
        ArgError("-bond: Illegal radius = %s.", argv[2]);
      pargv++;
    }

    /* 色ID */
    if ((color = Str2ColorStringID(pargv[2])) < 0)
      ArgError("-bond: Illegal color = %s.", pargv[2]);

    /* 結合形状の登録 */
    sij = GetBondShapeStringID(si, sj, lv);
    if (MDV_Atom_EnterBondShape(map, sij, radius, color) == NULL)
      MDV_Fatal("EnterBond()");
  } else {
    sij = Str2BondShapeStringID(argv[2]);
  }

  /* 登録 */
  if (MDV_Atom_EnterBond(map, si, sj, lv, length2, sij) == NULL)
    MDV_Fatal("EnterBond()");
}

/* 結合の削除(原子名-原子名) */
void EraseBond(MDV_Atom *map, const char * const argv[]) {
  StringID si, sj;
  int k, lv;

  /* 原子si */
  if ((si = SearchStringID(GetAtomString(argv[0]))) < 0
      || MDV_Atom_SearchAtom(map, si) == NULL)
    ArgError("-bond-: Unknown atom %s is found. (left side)", argv[0]);

  /* 結合レベルlv */
  for (k = 0; argv[0][k] != '\0' && argv[0][k] != '-'; k++)
    ;
  if (argv[0][k] == '\0')
    ArgError("-bond-: Illegal format.");
  for (lv = 0; argv[0][k+lv] == '-'; lv++)
    ;
  lv--;
  if (lv < 0 || lv >= MAXBONDTYPE)
    ArgError("-bond-: illegal bond level = %d.", lv+1);

  /* 原子sj */
  if ((sj = SearchStringID(argv[0]+k+lv+1)) < 0
      || MDV_Atom_SearchAtom(map, sj) == NULL)
    ArgError("-bond-: Unknown atom %s is found. (right side)", argv[0]);

  /* 削除 */
  MDV_Atom_EraseBond(map, si, sj, lv);
}

/* 結合形状設定の登録(結合形状名, 半径, 色) */
void EnterBondShape(MDV_Atom *map, const char * const argv[],
    const MDV_Status *msp) {
  StringID sij;
  double radius;
  StringID color;

  /* 結合形状sij */
  sij = Str2BondShapeStringID(argv[0]);

  /* 半径 */
  if ((radius = my_atof(argv[1]) * msp->length_unit) < 0 || my_err)
    ArgError("-bond_shape: Illegal radius = %s.", argv[1]);

  /* 色ID */
  if ((color = Str2ColorStringID(argv[2])) < 0)
    ArgError("-bond_shape: Illegal color = %s.", argv[2]);

  /* 登録 */
  if (!MDV_Atom_EnterBondShape(map, sij, radius, color))
    MDV_Fatal("EnterBondShape()");
}

/* 結合数設定の登録(原子名, 期待する結合数, 不足時の色, 超過時の色) */
void EnterCount(MDV_Atom *map, const char * const argv[],
    const MDV_Status *msp) {
  StringID si, sj, sij;
  StringID color_l, color_h;
  int k, lv;
  int count;
  const char **pargv = (const char **) argv;

  /* 原子si */
  if ((si = SearchStringID(argv[0])) < 0
      || MDV_Atom_SearchAtom(map, si) == NULL)
    ArgError("-count: Unknown atom %s is found.", argv[0]);

  if (msp->arg_version < 60) {
    /* 原子sj */
    if ((sj = SearchStringID(GetAtomString(argv[1]))) < 0
        || MDV_Atom_SearchAtom(map, sj) == NULL)
      ArgError("-count: Unknown atom %s is found.", GetAtomString(argv[1]));

    /* 結合レベルlv */
    for (k = 0; argv[1][k] != '\0' && argv[1][k] != '-'; k++)
      ;
    if (argv[1][k] == '\0')
      ArgError("-count: Illegal format.");
    for (lv = 0; argv[1][k+lv] == '-'; lv++)
      ;
    lv--;
    if (lv < 0 || lv >= MAXBONDTYPE)
      ArgError("-count: Illegal bond level = %d.", lv+1);

    /* 結合形状の登録 */
    sij = GetBondShapeStringID(si, sj, lv);
    count = 1;
    if (!MDV_Atom_EnterCountN(map, si, sij, count))
      MDV_Fatal("EnterCount()");

    pargv++;
  }

  /* 期待する結合数 */
  count = my_atoi(pargv[1]);
  if (my_err)
    ArgError("-count: Illegal count = %s.", pargv[1]);

  /* 色ID */
  if ((color_l = Str2ColorStringID(pargv[2])) < 0)
    ArgError("-count: Illegal color = %s.", pargv[2]);
  if ((color_h = Str2ColorStringID(pargv[3])) < 0)
    ArgError("-count: Illegal color = %s.", pargv[3]);

  /* 登録 */
  if (!MDV_Atom_EnterCount(map, si, count, color_l, color_h))
    MDV_Fatal("EnterCount()");
}

/* 結合数設定の削除(原子名) */
void EraseCount(MDV_Atom *map, const char * const argv[]) {
  StringID si;

  /* 原子si */
  if ((si = SearchStringID(argv[0])) < 0
      || MDV_Atom_SearchAtom(map, si) == NULL)
    ArgError("-count-: Unknown atom %s found.", argv[0]);

  /* 削除 */
  if (!MDV_Atom_EraseCount(map, si))
    MDV_Fatal("EraseCount()");
}

/* 結合形状ごとの結合数の登録(原子名, 結合形状名, 結合数) */
void EnterCountN(MDV_Atom *map, const char * const argv[]) {
  StringID si, sij;
  int count;

  /* 原子si */
  if ((si = SearchStringID(argv[0])) < 0
      || MDV_Atom_SearchAtom(map, si) == NULL)
    ArgError("-count_n: Unknown atom %s found.", argv[0]);

  /* 結合形状sij */
  if ((sij = SearchBondShapeStringID(argv[1])) < 0
      || MDV_Atom_SearchBondShape(map, sij) == NULL)
    ArgError("-count_n: Unknown bond shape %s found.", argv[1]);

  /* 結合数 */
  count = my_atoi(argv[2]);
  if (my_err)
    ArgError("-count_n: illegal count = %s.", argv[2]);

  /* 登録 */
  if (!MDV_Atom_EnterCountN(map, si, sij, count))
    MDV_Fatal("EnterCountN()");
}

/* ---- 色 ----------------------------------------------------------------- */

/* Color_Type型 */
typedef struct {
  StringID si;
  ColorID ci;
} Color_Type;

typedef Hash_Type Color_Table;
Color_Table *Color_Table_Alloc(void);
void Color_Table_Free(Color_Table *ctp);
Color_Type *Color_Table_Search(const Color_Table *ctp, StringID si);
Color_Type *Color_Table_Insert(Color_Table *ctp, StringID si);

static Color_Table *color_table = NULL; /* 暫定版: 解放しない */

typedef MDV_Array Color_List;
#define Color_ListP(a)          ((Color_Type *) (a)->p)
#define Color_ListAlloc()       MDV_Array_Alloc(sizeof(Color_Type))
#define Color_ListFree(a)       MDV_Array_Free(a)
#define Color_ListGetSize(a)    ((a)->n)
#define Color_ListSetSize(a, n) MDV_Array_SetSize((a), (n))

static Color_List *color_list = NULL; /* 暫定版: 解放しない */

/* 文字列に対応する色文字列IDを取得する。未知の文字列は失敗。(失敗は負の値) */
StringID SearchColorStringID(const char *str) {
  StringID si;

  if (str == NULL)
    MDV_Fatal("SearchColorStringID()");

  if (color_table == NULL || (si = SearchStringID(str)) < 0
      || Color_Table_Search(color_table, si) == NULL)
    return -1;

  return si;
}

/* 文字列に対応する色文字列IDを取得する。未知の文字列は登録。(失敗は負の値) */
StringID Str2ColorStringID(const char *str) {
  StringID si;
  Color_Type *cp;

  if (str == NULL)
    MDV_Fatal("Str2ColorStringID()");

  if (color_table == NULL) {
    color_table = Color_Table_Alloc();
    color_list = Color_ListAlloc();
  }

  if ((si = SearchStringID(str)) < 0
      || (cp = Color_Table_Search(color_table, si)) == NULL) {
    int col_r, col_g, col_b;
    int color_list_n;

    if (!Str2Rgb(str, &col_r, &col_g, &col_b) || (si = Str2StringID(str)) < 0)
      return -1;
    if ((cp = Color_Table_Insert(color_table, si)) == NULL)
      MDV_Fatal("Str2ColorStringID()");
    color_list_n = Color_ListGetSize(color_list);
    cp->ci = color_list_n;
    Color_ListSetSize(color_list, color_list_n+1);
    *(Color_ListP(color_list)+color_list_n) = *cp;
  }

  return si;
}

/* 色文字列IDに対応する文字列を取得する */
const char *ColorStringID2Str(StringID si) {
  const char *str = NULL;

  if (color_table == NULL || si < 0 || (str = StringID2Str(si)) == NULL
      || Color_Table_Search(color_table, si) == NULL)
    MDV_Fatal("ColorStringID2Str()");
  return str;
}

/* 色文字列IDに対応する色IDを取得する */
ColorID ColorStringID2ColorID(StringID si) {
  Color_Type *cp = NULL;

  if (color_table == NULL || color_list == NULL || si < 0
      || (cp = Color_Table_Search(color_table, si)) == NULL)
    MDV_Fatal("ColorStringID2ColorID()");
  return cp->ci;
}

/* 色IDに対応する色文字列IDを取得する */
StringID ColorID2ColorStringID(ColorID ci) {
  if (color_list == NULL || ci < 0 || ci >= Color_ListGetSize(color_list))
    MDV_Fatal("ColorID2ColorStringID()");
  return (Color_ListP(color_list)+ci)->si;
}

/* 色IDの総数を返す */
ColorID TotalColorID(void) {
  if (color_list == NULL)
    MDV_Fatal("TotalColorID()");
  return Color_ListGetSize(color_list);
}

/* ---- Color_Table型 ------------------------------------------------------ */

/* Color_Type型の初期化 */
void Color_Type_Init(Color_Type *cp, StringID si) {
  cp->si = si;
}

/* Color_Type用アロケータ */
#define COLOR_CHUNK_SIZE (256*sizeof(Color_Type))
static Chunk_Type *color_chunk = NULL; /* 暫定版: 終了処理は省略 */

/* 比較関数(StringIDが符号付き整数で、かつ0以上の値であることを前提とする) */
static int Color_Table_NodeCompare(const void *va, const void *vb) {
  return (int) (((const Color_Type *) va)->si - ((const Color_Type *) vb)->si);
}

/* ハッシュ関数 */
static Hash_Size Color_Table_NodeHash(const void *vp) {
  return (Hash_Size) ((const Color_Type *) vp)->si;
}

/* コピーコンストラクタ */
static void *Color_Table_NodeCopy(const void *vp) {
  const Color_Type *cp = (const Color_Type *) vp;
  Color_Type *ret;

  if (vp == NULL || color_chunk == NULL)
    return NULL;
  if ((ret = (Color_Type *) Chunk_NodeAlloc(color_chunk)) == NULL)
    return NULL;
  *ret = *cp;

  return ret;
}

/* デストラクタ */
static void Color_Table_NodeFree(void *vp) {
  if (vp == NULL || color_chunk == NULL)
    return;
  Chunk_NodeFree(color_chunk, vp);
}

/* インスタンスの作成 */
Color_Table *Color_Table_Alloc(void) {
  if (color_chunk == NULL) {
    if ((color_chunk = Chunk_TypeAlloc(sizeof(Color_Type),
        COLOR_CHUNK_SIZE, 0)) == NULL)
      return NULL;
  }
  return Hash_Alloc(0, 0);
}

/* インスタンスの消去 */
void Color_Table_Free(Color_Table *ctp) {
  Hash_Free(ctp, Color_Table_NodeFree);
}

/* Color情報の探索 */
Color_Type *Color_Table_Search(const Color_Table *ctp, StringID si) {
  Color_Type key;

  if (si < 0)
    return NULL;
  Color_Type_Init(&key, si);
  return (Color_Type *) Hash_Search(ctp, &key, Color_Table_NodeCompare,
    Color_Table_NodeHash);
}

/* Color情報の登録 */
Color_Type *Color_Table_Insert(Color_Table *ctp, StringID si) {
  Color_Type color;

  if (si < 0)
    return NULL;
  Color_Type_Init(&color, si);
  return (Color_Type *) Hash_Insert(ctp, &color, Color_Table_NodeCompare,
    Color_Table_NodeHash, Color_Table_NodeCopy);
}

/* ==== 粒子表 ============================================================= */

MDV_VAtomID *md_atom = NULL;

/* 総粒子数(未設定の時は0になる) */
MDV_Size TotalAtoms(void) {
  return MDV_VAtomIDGetSize(md_atom);
}

/* atom[]の確保 */
static int _init_md_atom = 0;
static void TermAtom(void) {MDV_VAtomIDFree(md_atom); md_atom = NULL;}
void InitAtom(void) {
  if (!_init_md_atom) {
    if (max_particle < 0) {
      fprintf(stderr, "InitAtom(): max_particle = %ld\n", (long) max_particle);
      exit(1);
    }
    md_atom = MDV_VAtomIDAlloc();
    AtExit(TermAtom);
    _init_md_atom = 1;
  }
}

/* 原子IDを得る。'\0'で終わっていなくとも良い。(失敗は負を返す) */
static AtomID SearchAtomID(const char *str) {
  StringID si;
  const AtomType *ap;

  if ((si = SearchStringID(GetAtomString(str))) < 0
      || (ap = MDV_Atom_SearchAtom(mdv_atom, si)) == NULL)
    return -1;
  return ap->id;
}

#define MINMOLSTR 1024
/* フォーマット文字列を読んでatom[]を設定する(返値は読んだ引数の数) */
int SetFormat(char *form) {
  AtomID *atom;
  static int maxmolstr = MINMOLSTR;
  char *mol = NULL;
  char *mol2 = NULL;
  ParameterList param;
  int v_param[MAXPARAMETER];
  const char *p;
  MDV_Size n;
  int ret, n_param = 0;
  int i;

  MDV_Atom_SetStaticInfo(mdv_atom); /* for SearchAtomID() */
  InitAtom();

  /* mol, mol2のメモリの確保 */
  mol = (char *) MDV_Work_Alloc(maxmolstr);
  mol2 = (char *) MDV_Work_Alloc(maxmolstr);

  if (debug_mode) {
    fprintf(stderr, "--- format information ---\n");
    fprintf(stderr, "\"%s\"\n", form);
  }

  /* 評価 */
  while ((ret = ReadMolExpression(form, mol, maxmolstr)) != MOL_SUCCESS) {
    switch (ret) {
    case MOL_TOO_LONG:
      /* ワークエリアを拡張、リトライする */
      MDV_Work_Free(mol2);
      MDV_Work_Free(mol);
      maxmolstr *= 2;
      mol = (char *) MDV_Work_Alloc(maxmolstr);
      mol2 = (char *) MDV_Work_Alloc(maxmolstr);
      continue;
    case MOL_ILL_EXPR:
      ArgError("-format %s:\nIllegal mol expression.", form);
    case MOL_ILL_FACT:
      ArgError("-format %s:\nIllegal mol factor.", form);
    case MOL_ILL_CFACT:
      ArgError("-format %s:\nIllegal coef factor.", form);
    case MOL_ILL_LEFT:
    case MOL_ILL_LEFT1:
    case MOL_ILL_LEFT2:
    case MOL_ILL_LEFT3:
      ArgError("-format %s:\nIllegal left character.", form);
    case MOL_ILL_RIGHT:
      ArgError("-format %s:\nIllegal right character.", form);
    default:
      ArgError("-format %s:\nError detected. (code = %d)", form, ret);
    }
  }

  if (debug_mode)
    fprintf(stderr, " -> \"%s\"\n", mol);

  if ((ret = GetParameters(mol, &param)) < 0)
    ArgError("Can't get parameter(s).");
  n_param = ret;
  if (debug_mode)
    fprintf(stderr, "%d parameter(s) detected.\n", n_param);

  for (i = 0; i < ret && (p = ArgvShift()) != NULL; i++)
    v_param[i] = atoi(p);
  if (i < ret)
    ArgError("Too few parameter(s).");
  if (!SetParameterValue(ret, v_param, &param) && param.n < ret)
    ArgError("Can't set parameter(s).");
  if (debug_mode) {
    for (i = 0; i < ret; i++)
      fprintf(stderr, "parameter %c = %d\n", param.c[i], param.v[i]);
  }

  if ((ret = Substitute(mol, mol2, maxmolstr, &param)) < 0)
    ArgError("Can't substitute parameter(s).");
  if (ret == 0)
    ArgError("Null format string.");
  if (debug_mode)
    fprintf(stderr, " -> \"%s\"\n", mol2);

  if ((n = TotalParticles(mol2, SearchAtomID)) < 0)
    ArgError("Can't get total particle number.");
  if (debug_mode)
    fprintf(stderr, "Total %ld particle(s).\n", (long) n);

  max_particle = n;
  MDV_VAtomIDSetSize(md_atom, n);
  atom = MDV_VAtomIDP(md_atom);

  if (SetAtomTable(mol2, atom, SearchAtomID) < 0)
    ArgError("Can't set atom[].");

  if (debug_mode) {
    /* テスト出力 */
    fprintf(stderr, "atom[] =\n");
    for (i = 0; i < n; i++)
      fprintf(stderr, "%d ", atom[i]);
    fprintf(stderr, "\n");
  }

  MDV_Work_Free(mol2);
  MDV_Work_Free(mol);

  return n_param;
}

/* ==== 引数ファイル ======================================================= */

/* MDV_Atom型のインスタンス定義(暫定版) */
MDV_Atom *mdv_atom = NULL;

#define FARG_FILEOPT "-f"
#define ARG_SUCCESS    (0)
#define ARG_HEAP_ERROR (-1)
#define ARG_TOO_LONG   (-2)
#define ARG_TOO_MANY   (-3)

/* システム標準デフォルト引数ファイル */
#ifndef DEFARGFILE
#define DEFARGFILE "/usr/local/share/mdview/mdviewrc"
#endif

/* 引数ファイル名 */
static char *arg_file_str[] = {
  ".mdviewrc",
  "mdview.arg",
  NULL
};

/* 引数ファイルorデフォルト引数の取り込みを行なう */
void ReadArguments(int argc, char **argv) {
  struct stat sb;
  char **myargv;
  int myargc;
  char *path = NULL;
  int npath = 0;
  int i;

  /* 初期化 */
  myargc = 0;
  if ((myargv = (char **) malloc(sizeof(char *)*(argc+3))) == NULL)
    HeapError();

  /* 最初の引数、プログラム名を取り込む */
  myargv[myargc++] = *argv;
  argc--; argv++;

  /* デバッグモードの判定 */
  debug_mode = 0;
  if (argc >= 1 && strcmp(argv[0], "-debug_mdv") == 0) {
    debug_mode = 1;
    argc--; argv++;
  }

  /* 引数ファイルの準備 */
  if (debug_mode) fprintf(stderr, "loading... ");
  if (argc && strcmp(*argv, FARG_FILEOPT) == 0) {
    /* 最初の引数がFARG_FILEOPT */
    if (argc <= 1) ArgError("-f: Too few argument(s).");
    npath = strlen(argv[1])+1;
    if ((path = (char *) malloc(npath)) == NULL)
      HeapError();
    Strlcpy(path, argv[1], npath);
    argc--; argv++;
    argc--; argv++;
  } else {
    char *home;
    char *p;

    /* カレントで探す */
    for (i = 0; (p = arg_file_str[i]) != NULL && stat(p, &sb) != 0; i++)
      ;
    if (p != NULL) {
      npath = strlen(p)+1;
      if ((path = (char *) malloc(npath)) == NULL)
        HeapError();
      Strlcpy(path, p, npath);
    } else if ((home = getenv("HOME")) != NULL) {
      /* ホームで探す */
      for (i = 0; (p = arg_file_str[i]) != NULL; i++) {
        npath = strlen(home)+strlen(p)+2;
        if ((path = (char *) malloc(npath)) == NULL)
          HeapError();
        Strlcpy(path, home, npath);
        Strlcat(path, "/", npath);
        Strlcat(path, p, npath);
        if (stat(path, &sb) == 0) break;
        free(path);
        path = NULL;
        npath = 0;
      }
    }
    if (path == NULL) {
      /* システム標準デフォルト引数ファイルを読む */
      if ((DEFARGFILE)[0] != '/') {
        /* 相対path。実行ファイルのディレクトリを基準とする */
        npath = strlen(exec_dir)+strlen(DEFARGFILE)+1;
        if ((path = (char *)malloc(npath)) == NULL)
          HeapError();
        Strlcpy(path, exec_dir, npath);
        Strlcat(path, DEFARGFILE, npath);
      } else {
        /* 絶対path */
        npath = strlen(DEFARGFILE)+1;
        if ((path = (char *)malloc(npath)) == NULL)
          HeapError();
        Strlcpy(path, DEFARGFILE, npath);
      }
      if (stat(path, &sb) != 0)
        {fprintf(stderr, "%s: Not found.\n", path); exit(1);}
    }
  }
  myargv[myargc++] = FARG_FILEOPT;
  myargv[myargc++] = path;
  if (debug_mode) fprintf(stderr, "\"%s\"\n", path);

  /* プログラム名と-f関連を除いた引数を取り込む(その数がargcに入っている) */
  for (i = 0; i < argc; i++)
    myargv[myargc++] = argv[i];
  myargv[myargc] = NULL;

  /* mdv_atomの(再)初期化 */
  if (mdv_atom != NULL)
    MDV_Atom_Free(mdv_atom);
  if ((mdv_atom = MDV_Atom_Alloc()) == NULL)
    HeapError();

  /* 引数の解釈 */
  MDV_StatusSet(mdv_atom, mdv_status, myargv);
  MDV_StatusCopy(mdv_status_default, mdv_status);

  /* 解放 */
  free(myargv);

  return;
}

/* ---- FARGシステムのラッピング ------------------------------------------- */

static const char *_current_argf = NULL; /* 現在の引数ファイル */
static long _current_argl = 0;           /* 現在の行番号 */

/* 実引数を内部引数に設定。 */
void ArgvInit(const char * const argv[]) {
  FARG_Init((const char * const *) argv);
  if (debug_mode)
    fprintf(stderr, "--- argument information ---");
}

/* 引数を先頭から1つ読む(返値は取り出した値へのリファレンス。失敗はNULL) */
const char *ArgvRead(void) {
  return FARG_Read();
}

/* 引数を先頭から1つ取り出す(返値は取り出した値へのリファレンス。失敗はNULL) */
const char *ArgvShift(void) {
  const char *p;

  if ((p = FARG_Shift()) == NULL) return NULL;
  if (debug_mode >= 2) {
    fputc((p[0] == '-')? '\n': ' ', stderr);
    fprintf(stderr, "%s", p);
  }
  return p;
}

/* エラー表示の際の参照位置の設定 */
void ArgvSetCurrent(void) {
  _current_argf = FARG_CurrentFileName();
  _current_argl = FARG_CurrentLine();
}

/* 引数に関するエラー出力 */
void ArgError(const char *format, ...) {
  va_list ap;

  if (_current_argf != NULL)
    fprintf(stderr, "%s, line %ld:\n", _current_argf, _current_argl+1);
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
}

/* ==== その他 ============================================================= */

/* 使用法の表示 */
void Usage(void) {
  fprintf(stderr, "--- mdview v%s.%s.%s ---\n",
    MDV_VERSION, MDV_REVISION, MDV_FIXLEVEL);
  fprintf(stderr,
    "Usage:\n"
    "    mdview [-f ArgFile] [Arg ...] {-format String [Value ...] | -format-} DataFile\n");
  fprintf(stderr,
    "Arguments:\n"
    "    -ang\n"
    "    -atom   Name Mass Radius Color\n"
    "    -au\n"
    "    -bg     Color\n"
    "    -bond   Name-Name Length Color\n"
    "    -c\n"
    "    -center X Y Z\n"
    "    -color  Level\n"
    "    -count  Name Name- Total ColorL ColorH\n"
    "    -d      Distance\n"
    "    -e      Theta Psi Phi\n"
    "    -fold\n"
    "    -frame  Number\n"
    "    -help\n"
    "    -layer  Name NumberList\n"
    "    -layer- Name\n"
    "    -layer_kill\n"
    "    -length Length\n"
    "    -mark   Color\n"
    "    -max    Particles\n"
    "    -multi\n"
    "    -range  Length\n"
    "    -version\n"
    "    -visible NumberList\n"
    "    -w      Size\n");
  exit(0);
}

/* バージョン表示 */
void Version(void) {
  fprintf(stderr, "mdview v%s.%s.%s\n",
    MDV_VERSION, MDV_REVISION, MDV_FIXLEVEL);
  exit(0);
}
