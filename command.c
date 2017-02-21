#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> /* stat() */
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include "mdview.h"

static void cancel_recieve(void);
static void command_clear(void);
static int command_exist_id(int id);
static CommandType command_get_id(int id);

/* ---- for command management --------------------------------------------- */

#define TWO_PI (2.0*3.14159265358979323)
#define ToggleOutline() (outline_mode^=1)

/*
 * mode                      val[0]       val[1]       val[2]
 * ==========================================================
 * COM_MODE_RELATIVE_STEP    (i)nview     (i)njump        ***
 * COM_MODE_AUTO_STEP        (i)auto_mode (i)njump        ***
 * COM_MODE_GO_TO_STEP       (i)number    (i)kind         ***
 * COM_MODE_ROTATE           (i)0,1,2     (d)angle   (i)nview
 * COM_MODE_ONE_ROUND        (i)nround    (i)ndrot        ***
 */

/* フォアグラウンドタスクの読み込み(返り値はタスクの有無) */
int command_read_foreground_task(int nstep, StateType *p) {
  CommandType com;

  p->mode = COM_MODE_NOTHING;
  p->nview = 1;
  p->njump = 0;
  p->xrot = 0.0;
  p->yrot = 0.0;
  p->zrot = 0.0;
  p->auto_mode = 0;

  if (!command_exist_id(COM_QUEUE_FOREGROUND))
    return 0;

  com = command_get_id(COM_QUEUE_FOREGROUND);

  p->mode = com.mode;
  switch (com.mode) {
  case COM_MODE_RELATIVE_STEP:
    p->nview = com.val[0].i;
    p->njump = com.val[1].i;
    break;
  case COM_MODE_AUTO_STEP:
    p->auto_mode = com.val[0].i;
    p->njump = com.val[1].i;
    break;
  case COM_MODE_GO_TO_STEP:
    if (com.val[1].i < 0) { /* 直接移動 */
      if (com.val[0].i < 0)
        p->njump = (MAXSTEP-1) - nstep;
      else
        p->njump = com.val[0].i - nstep;
      p->nview = 1;
    } else { /* 連続移動 */ 
      if (com.val[0].i > nstep) {
        if (com.val[0].i - nstep < com.val[1].i) {
          p->njump = 0;
          p->nview = 1;
        } else {
          p->njump = com.val[1].i;
          p->nview = (com.val[0].i - nstep) / com.val[1].i;
        }
      } else {
        if (nstep - com.val[0].i < com.val[1].i) {
          p->njump = 0;
          p->nview = 1;
        } else {
          p->njump = -com.val[1].i;
          p->nview = (nstep - com.val[0].i) / com.val[1].i;
        }
      }
    }
    break;
  case COM_MODE_ROTATE:
    switch (com.val[0].i) {
    case 0:
      /* x */
      p->xrot = com.val[1].d;
      break;
    case 1:
      /* y */
      p->yrot = com.val[1].d;
      break;
    case 2:
      /* z */
      p->zrot = com.val[1].d;
      break;
    }
    p->nview = com.val[2].i;
    break;
  case COM_MODE_ONE_ROUND:
    p->nview = 360*com.val[0].i / com.val[1].i;
    p->yrot = com.val[1].i;
    break;
  default:
    fprintf(stderr, "Unknown command %d.\n", (int) com.mode);
    exit(1);
  }

  return 1;
}

/*
 * mode                      val[0]       val[1]       val[2]
 * ==========================================================
 * COM_MODE_ZOOM             (i)0,1       (d)percent      ***
 * COM_MODE_MARK             (i)0,1       (i)mark         ***
 * COM_MODE_CONVERT             ***          ***          ***
 * COM_MODE_EULER_ANGLE      (d)theta     (d)psi       (d)phi
 * COM_MODE_OUTLINE             ***          ***          ***
 * COM_MODE_ROTATE_STEP      (i)1,2          ***          ***
 * COM_MODE_VIEW_POINT       (d)distance     ***          ***
 * COM_MODE_DRAG             (i)0,1          ***          ***
 * COM_MODE_VIEW_PARTICLE    (i)mark         ***          ***
 * COM_MODE_ORIGIN           (i)mark         ***          ***
 * COM_MODE_TREMBLE             ***          ***          ***
 * COM_MODE_TRANSLATION      (i)0,1       (d)ox        (d)oy
 * COM_MODE_DRAG_ROTATION    (d)rx        (d)ry
 * COM_MODE_COORDINATE       (i)particle
 */

/* バックグラウンドタスクの実行 */
int command_execute_background_task(StateType *p) {
  CommandType com;
  double theta, psi, phi, vx, vy, ox, oy, oz;
  int ret;

  if (!command_exist_id(COM_QUEUE_BACKGROUND))
    return 0;

  while (command_exist_id(COM_QUEUE_BACKGROUND)) {
    com = command_get_id(COM_QUEUE_BACKGROUND);

    switch (com.mode) {
    case COM_MODE_ZOOM:
      if (com.val[0].i == 0) { /* 直接変更 */
        SetZoomPercent(com.val[1].d);
      } else { /* 相対的変更 */
        double zoom;

        SetZoomPercent(com.val[1].d * GetZoomPercent());
        zoom = GetZoomPercent();
        if (zoom < GetZoomMin()) SetZoomPercent(GetZoomMin());
        if (zoom > GetZoomMax()) SetZoomPercent(GetZoomMax());
      }
      GuiSetZoomLabel(GetZoomPercent());
      break;
    case COM_MODE_MARK:
      if (com.val[0].i == 0) {
        if (com.val[1].i < 0)
          MarkClear();
        else
          MarkSet(com.val[1].i);
      } else {
        if (MarkRead(com.val[1].i))
          MarkWrite(0, com.val[1].i);
        else
          MarkSet(com.val[1].i);
      }
      break;
    case COM_MODE_STOP:
      GuiSetZoomLabel(GetZoomPercent());
      GuiSetDistance(distance);
      break;
    case COM_MODE_CONVERT:
      /* convert to povray */
      if (p->mode == COM_MODE_NOTHING) {
        static char *str_inc = NULL;
        FILE *fp_pov, *fp_inc;
        struct stat sb;
        char *str_pov;

        if (str_inc == NULL) {
          str_inc = FileSelection("include file", "*.inc");
          if (str_inc == NULL)
            break; /* cancel and exit */
          if (stat(str_inc,&sb) == 0) {
            gchar *message;
            /* already exist */
            message = g_strdup_printf("'%s' already exists.", str_inc);
            ret = AlertDialog(message, "Do you want to overwrite it?");
            g_free(message);
          } else
            ret = 1;
          switch (ret) {
          case 1: /* overwrite */
            if ((fp_inc = fopen(str_inc,"w")) != NULL) {
              ConvertPovray(NULL,fp_inc,str_inc);
              fclose(fp_inc); fp_inc = NULL;
            } else
              fprintf(stderr,"%s: Can't open.(1)\n",str_inc);
            break;
          case 0: /* read only */
            if ((fp_inc = fopen(str_inc,"r")) != NULL) {
              fclose(fp_inc); fp_inc = NULL;
            } else
              fprintf(stderr,"%s: Can't open.(2)\n",str_inc);
            break;
          default: /* cancel */
            ;
          }
        }
        if (str_inc == NULL)
          break;
        str_pov = FileSelection("povray file", "*.pov");
        if (str_pov == NULL)
          break; /* cancel and exit */
        if ((fp_pov = fopen(str_pov,"w")) != NULL) {
          ConvertPovray(fp_pov,NULL,str_inc);
          fclose(fp_pov); fp_pov = NULL;
        } else {
          fprintf(stderr,"%s: Can't open.(3)\n",str_pov);
        }
        g_free(str_pov);
      }
      break;
    case COM_MODE_EULER_ANGLE:
      theta = com.val[0].d*TWO_PI/360.0;
      psi = com.val[1].d*TWO_PI/360.0;
      phi = com.val[2].d*TWO_PI/360.0;
      SetEulerAngle(theta, psi, phi);
      break;
    case COM_MODE_OUTLINE:
      ToggleOutline();
      break;
    case COM_MODE_ROTATE_STEP:
      switch (com.val[0].i) {
      case 1:
        ChangeDeltaRotate(0.1);
        break;
      case 2:
        ChangeDeltaRotate(10.0);
        break;
      default:
        ;
      }
      break;
    case COM_MODE_VIEW_POINT:
      distance = com.val[0].d;
      break;
    case COM_MODE_DRAG:
      if (com.val[0].i == 0) {
        drag_mode = DRAG_TRANSLATION;
        GuiSetMouseMode(DRAG_TRANSLATION);
      } else {
        drag_mode = DRAG_ROTATION;
        GuiSetMouseMode(DRAG_ROTATION);
      }
      break;
    case COM_MODE_VIEW_PARTICLE:
      if (com.val[0].i == SET_ORIGIN) {
        set_trans_parameter(0.0, 0.0);
      } else if (com.val[0].i == SET_MARKED) {
        set_view_current(MarkReadNext(get_view_current()));
        GetParticleCoordinate(get_view_current(), &vx, &vy);
        set_trans_parameter(-vx, -vy);
      } else {
        set_view_current(com.val[0].i);
        GetParticleCoordinate(com.val[0].i, &vx, &vy);
        set_trans_parameter(-vx, -vy);
      }
      set_origin_mode(0);
      break;
    case COM_MODE_ORIGIN:
      unset_center_of_mass();
      set_trans_parameter(0.0, 0.0);
      if (com.val[0].i != SET_ORIGIN) {
        set_origin_current(com.val[0].i);
        GetCoordinates(com.val[0].i, &ox, &oy, &oz);
        set_center_of_mass(ox, oy, oz);
      }
      set_origin_mode(~0);
      break;
    case COM_MODE_TREMBLE:
      TrembleToggle();
      break;
    case COM_MODE_TRANSLATION:
      {
        gchar *string;
        double ox, oy;

        if (com.val[0].i == 0) {
          /* 絶対座標 */
          ox = com.val[1].d;
          oy = com.val[2].d;
        } else {
          /* 相対座標 */
          get_trans_parameter(&ox, &oy);
          ox += com.val[1].d;
          oy += com.val[2].d;
        }
        set_trans_parameter(ox, oy);
        string = g_strdup_printf("(% 6.3f, % 6.3f)", ox, oy);
        set_comment_string(string);
        g_free(string);
      }
      break;
    case COM_MODE_DRAG_ROTATION:
      {
        double rx, ry, rz, r, t;

        if (p->mode != COM_MODE_ONE_ROUND && p->mode != COM_MODE_ROTATE) {
          rx = com.val[0].d;
          ry = com.val[1].d;
          r = sqrt(rx*rx + ry*ry);
          t = rx / r;
          if (t > 1.0)
            rz = 0.0;
          else if (t < -1.0)
            rz = TWO_PI/2.0;
          else
            rz = acos(t);
          if (ry < 0)
            rz = -rz;
          RotateMatrixZ(-rz);
          RotateMatrixY(-r);
          RotateMatrixZ(rz);
        }
      }
      break;
    case COM_MODE_COORDINATE:
      {
        gchar *string;
        double xn, yn, zn;

        GetCoordinates(com.val[0].i, &xn, &yn, &zn);
        string = g_strdup_printf("%ld: (%#6.3g, %#6.3g, %#6.3g)",
          (long) com.val[0].i+1, xn, yn, zn);
        set_comment_string(string);
        g_free(string);
      }
      break;
    default:
      fprintf(stderr, "Unknown command %d.\n", (int) com.mode);
      exit(1);
    }
  }

  return 1;
}

#define DELAY_NUM 7
static int _frame[DELAY_NUM]={0,8,10,12,15,24,30};

int get_frame_id(int frame) {
  int i;

  for (i = 0; i < DELAY_NUM && _frame[i] != frame; i++)
    ;
  return (i >= DELAY_NUM)? -1: i+1;
}

void StopButton(void) {
  command_clear();
  cancel_recieve();
  command_put(COM_MODE_STOP);
}

/* ---- command queue ------------------------------------------------------ */

#define COM_QUEUE_MAX 64
#define COM_QUEUE_ID_MAX 2

static int com_queue_head[COM_QUEUE_ID_MAX];
static int com_queue_length[COM_QUEUE_ID_MAX];
static CommandType com_queue[COM_QUEUE_ID_MAX][COM_QUEUE_MAX];

/* コマンドキューの初期化 */
void command_init(void) {
  int id;
  for (id = 0; id < COM_QUEUE_ID_MAX; id++) {
    com_queue_head[id] = 0;
    com_queue_length[id] = 0;
  }
}

/* id番目のキュー(負なら全体)のコマンドがあるかの検査(返り値はコマンド数) */
int command_exist(void) {
  int id;
  int ret = 0;

  for (id = 0; id < COM_QUEUE_ID_MAX; id++)
    ret += com_queue_length[id];
  return ret;
}

/* id番目のキューのコマンドがあるかの検査(返り値はコマンド数) */
int command_exist_id(int id) {
  if (id < 0 || id >= COM_QUEUE_ID_MAX)
    {fprintf(stderr, "command_exist_id(): system error.\n"); exit(1);}
  return com_queue_length[id];
}

/* コマンドを蓄える */
void command_put(CommandMode mode, ...) {
  va_list ap;
  CommandType com;
  int id;
  int i;

  com.mode = mode;
  for (i = 0; i < COM_VALUE_MAX; i++)
    com.val[i].i = 0;

  va_start(ap, mode);

  /* queue idの決定 */
  switch (mode) {
  case COM_MODE_AUTO_STEP:
  case COM_MODE_GO_TO_STEP:
  case COM_MODE_ONE_ROUND:
  case COM_MODE_RELATIVE_STEP:
  case COM_MODE_ROTATE:
    id = COM_QUEUE_FOREGROUND;
    break;
  default:
    id = COM_QUEUE_BACKGROUND;
  }

  /* 引数の処理 */
  switch (mode) {
  case COM_MODE_STOP:
  case COM_MODE_CONVERT:
  case COM_MODE_OUTLINE:
  case COM_MODE_TREMBLE:
    break;
  case COM_MODE_ROTATE_STEP:
  case COM_MODE_DRAG:
  case COM_MODE_VIEW_PARTICLE:
  case COM_MODE_ORIGIN:
  case COM_MODE_COORDINATE:
    com.val[0].i = va_arg(ap, int);
    break;
  case COM_MODE_VIEW_POINT:
    com.val[0].d = va_arg(ap, double);
    break;
  case COM_MODE_MARK:
  case COM_MODE_RELATIVE_STEP:
  case COM_MODE_AUTO_STEP:
  case COM_MODE_GO_TO_STEP:
  case COM_MODE_ONE_ROUND:
    com.val[0].i = va_arg(ap, int);
    com.val[1].i = va_arg(ap, int);
    break;
  case COM_MODE_ZOOM:
    com.val[0].i = va_arg(ap, int);
    com.val[1].d = va_arg(ap, double);
    break;
  case COM_MODE_DRAG_ROTATION:
    com.val[0].d = va_arg(ap, double);
    com.val[1].d = va_arg(ap, double);
    break;
  case COM_MODE_ROTATE:
    com.val[0].i = va_arg(ap, int);
    com.val[1].d = va_arg(ap, double);
    com.val[2].i = va_arg(ap, int);
    break;
  case COM_MODE_TRANSLATION:
    com.val[0].i = va_arg(ap, int);
    com.val[1].d = va_arg(ap, double);
    com.val[2].d = va_arg(ap, double);
    break;
  case COM_MODE_EULER_ANGLE:
    com.val[0].d = va_arg(ap, double);
    com.val[1].d = va_arg(ap, double);
    com.val[2].d = va_arg(ap, double);
    break;
  default:
    fprintf(stderr,"command_put(): Illegal argument.\n");
    return;
  }
  va_end(ap);

  if (com_queue_length[id] >= COM_QUEUE_MAX)
    fprintf(stderr, "Warning: Input buffer overflow.\n");
  com_queue[id][(com_queue_head[id]+com_queue_length[id]++) % COM_QUEUE_MAX]
    = com;
}

/* コマンドを取り出す */
static CommandType command_get_id(int id) {
  CommandType ret;
  int i;

  if (id < 0 || id >= COM_QUEUE_ID_MAX)
    {fprintf(stderr, "command_get_id(): system error.\n"); exit(1);}

  if (com_queue_length[id] > 0) {
    ret = com_queue[id][com_queue_head[id]];
    com_queue_head[id] = (com_queue_head[id]+1) % COM_QUEUE_MAX;
    com_queue_length[id]--;
  } else {
    ret.mode = COM_MODE_NOTHING;
    for (i = 0; i < COM_VALUE_MAX; i++)
      ret.val[i].i = 0;
  }
  return ret;
}

/* 蓄えているコマンドを消去する */
static void command_clear(void) {
  int id;

  for (id = 0; id < COM_QUEUE_ID_MAX; id++) {
    com_queue_head[id] = 0;
    com_queue_length[id] = 0;
  }
}

/* ---- for cancel key '^C' ------------------------------------------------ */

static int cancel_flag = 0;

int canceled(void) {
  return cancel_flag;
}

void cancel_wait(void) {
  cancel_flag = 0;
  return;
}

static void cancel_recieve(void) {
  cancel_flag = 1;
  return;
}
