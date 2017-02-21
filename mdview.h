#ifndef mdview_h_
#define mdview_h_

#ifdef HAVE_CONFIG_H
# include "config.h"
#else
# error config.h is required.
#endif

#if STDC_HEADERS
# include <stdio.h>
#else
# error the ANSI headers are required.
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>

/* ---- mdv_type.h --------------------------------------------------------- */
#include "mdv_type.h"

/* ---- main.c ------------------------------------------------------------- */
extern int use_idle_callback;
extern int screen_busy;
extern char *exec_dir;

/* ---- gtk.c -------------------------------------------------------------- */
#define ZOOM_MIN 0.10
#define ZOOM_MAX 10.00
#define MARK_SINGLE   0
#define MARK_MULTIPLE 1
#define DRAG_TRANSLATION 0
#define DRAG_ROTATION    1

#define MDV_COLOR_BLACK 0
#define MDV_COLOR_RED 1
#define MDV_COLOR_GREEN 2
#define MDV_COLOR_YELLOW 3
#define MDV_COLOR_BLUE 4
#define MDV_COLOR_MAGENTA 5
#define MDV_COLOR_CYAN 6
#define MDV_COLOR_WHITE 7
#define MDV_COLOR_FREE_COL1 8

extern int window_size;
extern int frames;
extern int outline_mode;
extern int mark_mode;
extern int drag_mode;
extern int loop_mode;

extern void GuiInitialize(int *pargc, char **argv);
extern void CreateGui(void);
extern void DrawObject(void);
extern void Redraw(void);
extern void GuiSetEulerAngle(double theta, double psi, double phi);
extern void ShowEulerAngle(void);
extern void GuiSetCounter(int nstep);
extern void GuiSetDistance(double distance);
extern void GuiSetMouseMode(int mode);
extern void ChangeDeltaRotate(double d);
extern void GetScreenSize(int *pw, int *ph);
extern void get_trans_parameter(double *px, double *py);
extern void set_trans_parameter(double x, double y);
extern void set_comment_string(gchar *str);
extern int mdv_mapcolor(MDV_Color c, int red, int green, int blue);
extern GtkWidget *GetTopWidget(void);
extern gint delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
extern gint key_event(GtkWidget *widget, GdkEventKey *event);
extern void cb_entry_activate(GtkEditable *editable, gpointer user_data);
extern void cb_entry_focus_in(GtkWidget *widget, GdkEventFocus *event, gpointer user_data);

/* ---- gtk_convert.c ------------------------------------------------------ */
extern char *FileSelection(char *title, char *pattern);
extern int  AlertDialog(const char *message1, const char *message2);

/* ---- gtk_info.c --------------------------------------------------------- */
extern void GuiSetInformation(int nstep);
extern gint cb_info(GtkButton *button, gpointer data);

/* ---- gtk_layer.c -------------------------------------------------------- */
extern gint cb_layer_open(GtkButton *button, gpointer button_parent);
extern guint InvisibleAtom(glong i);
extern void AddNewLayer(char *name, char *itemstring);
extern void EraseLayer(char *name);
extern void SetLayerStatus(char *itemstring);
extern void KillLayers(void);

/* ---- gtk_mark.c --------------------------------------------------------- */
extern gint cb_mark_open(GtkButton *button, gpointer data);
extern gint cb_mark_single(GtkButton *button, gpointer data);
extern gint cb_mark_multi(GtkButton *button, gpointer data);
extern gint cb_mark_apply(GtkButton *button, gpointer data);
extern gint cb_mark_clear(GtkButton *button, gpointer data);
extern gint cb_mark_revert(GtkButton *button, gpointer data);
extern gint cb_mark_close(GtkButton *button, gpointer data);
extern void gui_mark_load(GtkWidget *clist, gboolean select_marked, gboolean only_marked);

/* ---- gtk_move.c --------------------------------------------------------- */
extern gint cb_move_open(GtkButton *button, gpointer data);
extern gint cb_move_all(GtkButton *button, gpointer data);
extern gint cb_move_marked(GtkButton *button, gpointer data);
extern gint cb_move_apply(GtkButton *button, gpointer data);
extern gint cb_move_clear(GtkButton *button, gpointer data);
extern gint cb_move_revert(GtkButton *button, gpointer data);
extern gint cb_move_close(GtkButton *button, gpointer data);
extern long get_view_current(void);
extern void set_view_current(long value);
extern long get_origin_current(void);
extern void set_origin_current(long value);
extern void set_origin_mode(guint flag);

/*-------------------------------gtk_distance.c----------------------------*/
extern double distance;
extern void GuiSetDistance(double distance);
extern GtkWidget *distance_gtk_button(void);
extern GtkWidget *distance_gtk_entry(void);
extern GtkWidget *distance_gtk_hscale(void);

/*----------------------------------gtk_zoom.c------------------------------*/
extern void GuiSetZoom(double zoom);
extern void GuiSetZoomLabel(double zoom);
extern void SetZoomPercent(double value);
extern double GetZoomPercent(void);
extern double GetZoomMin(void);
extern double GetZoomMax(void);
extern GtkWidget *zoom_gtk_button(void);
extern GtkWidget *zoom_gtk_entry(void);
extern GtkWidget *zoom_gtk_hscale(void);

/* ---- gtkraw.c ----------------------------------------------------------- */
extern gint special_key_event(GtkWidget *widget, GdkEventKey *event);
extern void AddKeyEvent(void);
extern void RemoveKeyEvent(void);
extern int gtkraw_button_1(void);
extern int gtkraw_button_2(void);
extern int gtkraw_key_control(void);
extern void gtkraw_signal_connect_buttons(GtkWidget *a, gpointer b, int repeat);

/*---------------------------------tremble.c---------------------------------*/
extern unsigned int TrembleStatus (void);
extern void TrembleSaveEulerAngle(void);
extern void TrembleLoadEulerAngle(void);
extern void TrembleToggle (void);
extern void TrembleRotateMatrix (void);

/* ---- command.c ---------------------------------------------------------- */
typedef enum {
  COM_MODE_NOTHING = 0,
  COM_MODE_RELATIVE_STEP,
  COM_MODE_AUTO_STEP,
  COM_MODE_GO_TO_STEP,
  COM_MODE_ZOOM,
  COM_MODE_ROTATE,
  COM_MODE_ONE_ROUND,
  COM_MODE_MARK,
  COM_MODE_CONVERT,
  COM_MODE_EULER_ANGLE,
  COM_MODE_OUTLINE,
  COM_MODE_ROTATE_STEP,
  COM_MODE_VIEW_POINT,
  COM_MODE_DRAG,
  COM_MODE_VIEW_PARTICLE,
  COM_MODE_ORIGIN,
  COM_MODE_TREMBLE,
  COM_MODE_TRANSLATION,
  COM_MODE_DRAG_ROTATION,
  COM_MODE_COORDINATE,
  COM_MODE_STOP = 999
} CommandMode;

typedef struct {
  CommandMode mode;
  int nview,
      njump,
      auto_mode,
      mark;
  double xrot,
         yrot,
         zrot;
} StateType;

typedef union {
  int i;
  double d;
} CommandData;

#define COM_VALUE_MAX 3
typedef struct {
  CommandMode mode;
  CommandData val[COM_VALUE_MAX];
} CommandType;

#define COM_QUEUE_ALL (-1)
#define COM_QUEUE_FOREGROUND 0
#define COM_QUEUE_BACKGROUND 1

#define MAXSTEP (~0U>>1)

#define SET_ORIGIN (-1)
#define SET_MARKED (-2)

extern int  command_read_foreground_task(int nstep, StateType *pstate);
extern int  command_execute_background_task(StateType *pstate);
extern void command_init(void);
extern int  command_exist(void);
extern void command_put(CommandMode mode, ...);
extern int  canceled(void);
extern void cancel_wait(void);
extern int  get_frame_id(int);
extern void StopButton(void);

/* ---- object.c ----------------------------------------------------------- */
void LoadViewParameter(void);
void UpdateViewData(void);
void set_center_of_mass(double x, double y, double z);
void unset_center_of_mass(void);

/* ---- mdv.c -------------------------------------------------------------- */
extern void MDV_Init(void);

/* ---- mdv_arg.c ---------------------------------------------------------- */
extern char *file_path;

extern void ReadArguments(int argc,char **argv);
extern long TotalAtoms(void);

/* ---- mdv_data.c --------------------------------------------------------- */
extern void MDInit(void);
extern void SetCenterMass(void);
extern void SetEulerAngle(double theta,double psi,double phi);
extern void GetEulerAngle(double *theta,double *psi,double *phi);
extern void RotateMatrixX(double phi);
extern void RotateMatrixY(double phi);
extern void RotateMatrixZ(double phi);
void RotateMatrixFree(double x1, double y1, double z1,
                      double x2, double y2, double z2);
extern int GetCoordinates(long num, double *px, double *py, double *pz);

/* ---- mdv_draw.c --------------------------------------------------------- */
#define DRAW_OVALF 0
#define DRAW_OVALL 1
#define DRAW_LINE  2
typedef struct {
  int flag;
  double a,b,c,d;
  MDV_Color col;
  long i;
} DrawList;
extern DrawList *drawlist;
extern int drawlist_n;

extern MDV_Color GetBackGroundColor(void);

extern void ViewInit(void);
extern void ViewData(void);
extern void GetParticleCoordinate(long i, double *px, double *py);
extern const char *GetAtomName(long i);
extern long WhichParticle(double x, double y);
extern void ConvertPovray(FILE *fp_pov,FILE *fp_inc,const char *str_inc);

extern void MarkInit(int mode);
extern int  GetMarkMode(void);
extern int  SetMarkMode(int mode);
extern void MarkClear(void);
extern void MarkSet(long x);
extern long MarkRead1st(void);
extern long MarkReadNext(long x);
extern int  MarkRead(long x);
extern void MarkWrite(int val,long x);
extern int  MarkPush(void);
extern int  MarkPop(void);
extern int  MarkLoad(void);
extern int  MarkSave(void);

/* ---- mdv_util.c --------------------------------------------------------- */
#include "mdv_util.h"

/* ---- machine.c ---------------------------------------------------------- */
#include "machine.h"


#endif /* mdview_h_ */
