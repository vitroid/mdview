#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h> /* usleep() */
#include <gtk/gtk.h>
#include "mdview.h"
#include "mdv_file.h"

int screen_busy = 1;
int use_idle_callback = 0;
char *exec_dir = NULL;

/* for Forms Library options */
static void SeparateArguments(int *pmyargc, char ***pmyargv,
                         int *pflargc, char ***pflargv, int argc, char **argv);
static void EventLoop(void);
static gint cb_idle(gpointer data);

static MDV_FILE *mfp;
static void TermMDV_FILE(void) {MDV_FileClose(mfp);}

#define TWO_PI (2.0*3.14159265358979323)
int main(int argc, char *argv[]){
	int myargc, flargc;
	char **myargv, **flargv;
	char *p;

	/* set execute directory */
	if ((exec_dir = (char *) malloc(strlen(*argv))) == NULL) HeapError();
	strcpy(exec_dir, *argv);
	p = strrchr(exec_dir, '/');
	if (p == NULL)
		exec_dir[0] = '\0';
	else
		p[1] = '\0';

	/* mdview database initialization (temporary) */
	MDV_Init();

	/* read arguments */
	SeparateArguments(&myargc, &myargv, &flargc, &flargv, argc, argv);
	ReadArguments(myargc, myargv);

	/* mdview initialization */
	MDInit();

	/* open data file */
	if ((mfp = MDV_FileOpen(file_path,TotalAtoms())) == NULL) exit(1);
	AtExit(TermMDV_FILE);

	/* set center mass and max radius */
	if (MDV_FileRead(mfp, 0) != MDV_READ_SUCCESS)
		{fprintf(stderr,"Read error.(8)\n"); exit(1);}
	SetCenterMass();

	LoadViewParameter();
	GuiInitialize(&flargc, flargv); 
	command_init();
	CreateGui();
	ViewInit();

	/* execute */
	if (MDV_FileRead(mfp, 0) != MDV_READ_SUCCESS)
		{fprintf(stderr,"Read error.(9)\n"); exit(1);}
	UpdateViewData();
	/* GuiSetInformation(0); */

	EventLoop();

	return EXIT_SUCCESS;
}


static StateType state;
static int nstep;

#define IDLE_LOOP 5
static gint cb_idle(gpointer data) {
  static int cb_idle_busy = 0;
  int ret;
  int i;

  /* 関数のロック */
  if (cb_idle_busy)
    return TRUE;
  cb_idle_busy = 1;

  /* for cancelation */
  if (canceled()) {
    state.mode = COM_MODE_NOTHING;
    state.nview = 0;
    state.njump = 0;
    state.xrot = 0;
    state.yrot = 0;
    state.zrot = 0;
    state.auto_mode = 0;
  }
  cancel_wait();

  for (i = 0; i < IDLE_LOOP; i++) {
    if (state.mode == COM_MODE_NOTHING
        || (state.nview <= 0 && !state.auto_mode)) {
      if (command_read_foreground_task(nstep, &state)) {
        screen_busy = 1;
      } else {
        screen_busy = 0;
      }
    }
    if (command_execute_background_task(&state))
      screen_busy = 1;

    if (screen_busy) {
      /* read data */
      if (loop_mode && state.auto_mode) {
        ret = MDV_FileRead(mfp, nstep+state.njump);
        if (ret == MDV_READ_OVERFLOW) {
          nstep = 0;
          ret = MDV_FileRead(mfp, nstep+state.njump);
        } else if (ret == MDV_READ_UNDERFLOW) {
          nstep = MDV_MaxStep(mfp);
          ret = MDV_FileRead(mfp, nstep+state.njump);
        }
      } else {
        while ((ret = MDV_FileRead(mfp,nstep+state.njump))
            == MDV_READ_OVERFLOW) {
          if (!MDV_FileReload(mfp) || MDV_MaxStep(mfp) >= 0)
            break;
        }
      }
      if (ret == MDV_READ_FAILURE)
        {fprintf(stderr,"Read error.(10)\n"); exit(1);}
      if (ret == MDV_READ_SUCCESS) {
        nstep += state.njump;
        if (!state.auto_mode)
          state.nview--;
      } else {
        /* ジャンプ移動以外は、次の行を実行しないようにしたい。 */
        nstep = (ret == MDV_READ_OVERFLOW)? MDV_MaxStep(mfp): 0;
        if (MDV_FileRead(mfp,nstep) != MDV_READ_SUCCESS)
          {fprintf(stderr,"Read error.(11)\n"); exit(1);}
        state.auto_mode = 0;
        state.nview = 0;
      }

      /* rotation */
      if (state.xrot != 0)
        RotateMatrixX(state.xrot*TWO_PI/360.0);
      if (state.yrot != 0)
        RotateMatrixY(state.yrot*TWO_PI/360.0);
      if (state.zrot != 0)
        RotateMatrixZ(state.zrot*TWO_PI/360.0);

      /* trembling */
      if (TrembleStatus()) {
        TrembleSaveEulerAngle();
        TrembleRotateMatrix ();
      }

      /* draw data */
      GuiSetCounter(nstep);
      GuiSetInformation(nstep);
      UpdateViewData();
      DrawObject();
      if (TrembleStatus())
        TrembleLoadEulerAngle();

    } else if (TrembleStatus()) {
      /* trembling */
      TrembleSaveEulerAngle();
      TrembleRotateMatrix();
      UpdateViewData();
      DrawObject();
      TrembleLoadEulerAngle();
    }
  }

  cb_idle_busy = 0;
  return TRUE;
}

static void EventLoop(void) {
	/* Initialize */
	state.nview = 0;
	state.njump = 0;
	state.xrot = 0;
	state.yrot = 0;
	state.zrot = 0;
	state.auto_mode = 0;
	nstep = 0;
	screen_busy = 0;
	gtk_timeout_add(20, cb_idle, NULL);
	gtk_main();
}

/* ---- GUI Library options ------------------------------------------------ */

typedef enum {
  GUIL_VALUE_NONE,
  GUIL_VALUE_INT,
  GUIL_VALUE_LONG,
  GUIL_VALUE_FLOAT,
  GUIL_VALUE_STRING
} GUIL_ValueType;

typedef struct {
  const char *str;  /* 引数文字列 */
  GUIL_ValueType val; /* 続く引数の型 */
} GUIL_ArgType;

GUIL_ArgType guil_arg_list[] = {
  {"-gtk-module"       , GUIL_VALUE_STRING},
  {"-g-fatal-warnings" , GUIL_VALUE_NONE},
  {"-gtk-debug"        , GUIL_VALUE_NONE},
  {"-gtk-no-debug"     , GUIL_VALUE_NONE},
  {"-gdk-debug"        , GUIL_VALUE_NONE},
  {"-gdk-no-debug"     , GUIL_VALUE_NONE},
  {"-display"          , GUIL_VALUE_STRING},
  {"-sync"             , GUIL_VALUE_NONE},
  {"-no-xshm"          , GUIL_VALUE_NONE},
  {"-name"             , GUIL_VALUE_STRING},
  {"-class"            , GUIL_VALUE_STRING},
  /* 最後の要素はNULLとなる */
  {NULL       , GUIL_VALUE_NONE }
};

/* オプションとそれ以外の分離を行なう */
static void SeparateArguments(int *pmyargc, char ***pmyargv,
    int *pflargc, char ***pflargv, int argc, char **argv) {
  int guil_i, my_i, i, j;
  char **buf;
  const char *p;

  /* 領域確保(両方をまとめて一つの配列にとる) */
  if ((buf = (char **) malloc(sizeof(char *)*(argc+3))) == NULL) HeapError();

  /* オプションを後ろから、それ以外を前から並べる */
  i = my_i = 1;
  buf[0] = argv[0];
  guil_i = argc+1;
  buf[guil_i+1] = NULL;
  buf[guil_i] = argv[0];
  while (i < argc) {
    for (j = 0; (p = guil_arg_list[j].str) != NULL; j++) {
      if (argv[i][0] == '-' && strcmp(p,argv[i]+1) == 0)
        break;
    }
    if (p != NULL) {
      /* オプション */
      buf[--guil_i] = argv[i++];
      switch (guil_arg_list[j].val) {
      case GUIL_VALUE_NONE:
        break;
      default:
        if (i+1 > argc)
          {fprintf(stderr,"-%s: Too few parameters.\n",p); exit(1);}
        buf[--guil_i] = argv[i++];
      }
    } else
      buf[my_i++] = argv[i++];
  }
  buf[my_i] = NULL;
  *pmyargc = my_i;
  *pmyargv = buf;
  *pflargc = argc - my_i + 1;
  *pflargv = buf + my_i + 1;

  /* オプションの並びを元に戻す */
  for (i = 0, j = (*pflargc)-1; i < j; i++, j--) {
    p = (*pflargv)[i];
    (*pflargv)[i] = (*pflargv)[j];
    (*pflargv)[j] = (char *) p;
  }
}
