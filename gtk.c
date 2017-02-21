#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdktypes.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mdview.h"

#include "pixmap/pixmap_1.xpm"
#include "pixmap/pixmap_2.xpm"
#include "pixmap/pixmap_3.xpm"
#include "pixmap/pixmap_4.xpm"
#include "pixmap/pixmap_6.xpm"
#include "pixmap/pixmap_8.xpm"
#include "pixmap/pixmap_14.xpm"
#include "pixmap/pixmap_44.xpm"
#include "pixmap/pixmap_61.xpm"
#include "pixmap/pixmap_66.xpm"
#include "pixmap/pixmap_stop.xpm"
#include "pixmap/pixmap_rotate.xpm"
#include "pixmap/pixmap_plus.xpm"
#include "pixmap/pixmap_minus.xpm"
#include "pixmap/pixmap_help.xpm"
#include "pixmap/pixmap_1_1.xpm"
#include "pixmap/pixmap_1_2.xpm"
#include "pixmap/pixmap_1_3.xpm"
#include "pixmap/pixmap_2_1.xpm"
#include "pixmap/pixmap_2_2.xpm"
#include "pixmap/pixmap_2_3.xpm"
#include "pixmap/pixmap_3_1.xpm"
#include "pixmap/pixmap_3_2.xpm"
#include "pixmap/pixmap_3_3.xpm"
#include "pixmap/pixmap_4_1.xpm"
#include "pixmap/pixmap_4_2.xpm"
#include "pixmap/pixmap_4_3.xpm"
#include "pixmap/pixmap_6_1.xpm"
#include "pixmap/pixmap_6_2.xpm"
#include "pixmap/pixmap_6_3.xpm"
#include "pixmap/pixmap_8_1.xpm"
#include "pixmap/pixmap_8_2.xpm"
#include "pixmap/pixmap_8_3.xpm"
#include "pixmap/marked.xpm"

#define ROTATE_SMALL  (1.0)
#define ROTATE_LARGE (10.0)

/* external valiables */
int window_size = 400;
int frames = 0; 
int outline_mode = 0;
int mark_mode = MARK_SINGLE;
int drag_mode = DRAG_TRANSLATION;
int loop_mode = 0;

//#define GTK_ENTRY_FIT(w, s) gtk_widget_set_usize(GTK_WIDGET(w), 8+gdk_text_width(gtk_widget_get_style(GTK_WIDGET(w))->font, (s), strlen(s)), 4+gdk_text_height(gtk_widget_get_style(GTK_WIDGET(w))->font, (s), strlen(s)))
#define GTK_ENTRY_FIT(w, s) gtk_widget_set_usize(GTK_WIDGET(w), 8+gdk_text_width(gtk_style_get_font(gtk_widget_get_style(GTK_WIDGET(w))), (s), strlen(s)), 4+gdk_text_height(gtk_style_get_font(gtk_widget_get_style(GTK_WIDGET(w))), (s), strlen(s)))
static gint expose_event(GtkWidget *widget, GdkEventExpose *event);
static gint button_event(GtkWidget *widget, GdkEvent *event, gpointer data);
static void KeyOperation(gint keyval);
static gint cb_step_g(GtkButton *button, gpointer data);
static gint cb_step_b(GtkButton *button, gpointer data);
static gint cb_step_k(GtkButton *button, gpointer data);
static void cb_step_changed(GtkEditable *editable, gpointer data);
static void cb_step_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer user_data);
static gint cb_step_j(GtkButton *button, gpointer data);
static gint cb_step_f(GtkButton *button, gpointer data);
static gint cb_step_G(GtkButton *button, gpointer data);
static gint cb_stop(GtkButton *button, gpointer data);
static gint cb_rotate_1(GtkButton *button, gpointer data);
static gint cb_rotate_2(GtkButton *button, gpointer data);
static gint cb_rotate_3(GtkButton *button, gpointer data);
static gint cb_rotate_4(GtkButton *button, gpointer data);
static gint cb_rotate_6(GtkButton *button, gpointer data);
static gint cb_rotate_8(GtkButton *button, gpointer data);
static gint cb_rotate_minus(GtkButton *button, gpointer data);
static gint cb_rotate_plus(GtkButton *button, gpointer data);
static gint cb_rotate(GtkButton *button, gpointer data);
static void cb_euler_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer user_data);
static gint cb_interval(GtkMenuItem *widget, gpointer data);
static gint cb_frame(GtkMenuItem *widget, gpointer data);
static gint cb_drag_mode(GtkButton *button, gpointer data);
static gint cb_outline(GtkButton *button, gpointer data);
static gint cb_convert(GtkButton *button, gpointer data);
static gint cb_help(GtkButton *button, gpointer data);
static void ToggleTooltips(void);

static GtkWidget *draw = NULL;
static GtkWidget *window_main = NULL;
static GtkWidget *euler_theta = NULL, *euler_psi = NULL, *euler_phi = NULL;
static GtkWidget *input = NULL;
static GtkWidget *input_text = NULL;
static GtkWidget *entry_step = NULL;
static GtkWidget *radiobutton_translation = NULL;
static GtkWidget *radiobutton_rotation = NULL;
static GSList *list_tooltips = NULL;
static GdkColor **mdv_color = NULL;
static GdkGC *gc = NULL;
static GdkPixmap *pixmap_screen = NULL;
static GdkPixmap *gpixmap[6][4];
static GdkBitmap *gmask[6][4];
static GtkWidget *gpixmapwid[6];
static double d_rotate = 1.0;
static int interval = 1;
static double view_origin_x = 0.0, view_origin_y = 0.0;
static gint window_width=-1, window_height=-1;

static gchar *comment_string = NULL;
static long comment_string_sec = 0;
static long comment_string_usec = 0;
#define COMMENT_STRING_TIMEOUT_USEC 1000000

GtkWidget *GetTopWidget(void) {
	return window_main;
}

void GuiInitialize(int *pargc, char **argv) {
	/*
	 * initialize GTK+
	 */
	gtk_set_locale();
	gtk_init(pargc, &argv);
	return;
}

#define mdv_draw_rectangle(x, y, w, h, color, filled) \
( \
	gdk_gc_set_foreground(gc, mdv_color[(color)]), \
	gdk_draw_rectangle(pixmap_screen, gc, (filled), (x), (y), (w), (h)) \
)
#define mdv_draw_oval(x, y, w, h, color, filled) \
( \
	gdk_gc_set_foreground(gc, mdv_color[(color)]), \
	gdk_draw_arc(pixmap_screen, gc, (filled), (x), (y), (w), (h), 0, 360*64) \
)
#define mdv_draw_line(x, y, w, h, color) \
( \
	gdk_gc_set_foreground(gc, mdv_color[(color)]), \
	gdk_draw_line(pixmap_screen, gc, (x), (y), (w), (h)) \
)

void DrawObject(void) {
  static int init = 0;
  static long lsec;
  static long lusec;
  long sec, usec;
  long sleeptime;
  double delaytime;
  gint win_w, win_h, a, b, c, d;
  gdouble coef, ox, oy;
  DrawList *dp;
  MDV_Color color;
  gint i;

  if (pixmap_screen == NULL)
    return;

  /* wait if frames is set. */
  if (frames != 0) {
    if (!init) {
      GetTimeval(&lsec, &lusec);
      init = 1;
    } else {
      GetTimeval(&sec, &usec);
      delaytime = 1000000.0 / frames;
      sleeptime = delaytime - (sec-lsec)*1000000 - (usec-lusec);
      if (sleeptime > 0 && sleeptime < delaytime) {
        Usleep(sleeptime);
      }
      GetTimeval(&lsec, &lusec);
    }
  }

  /* draw objects */
  win_w = draw->allocation.width;
  win_h = draw->allocation.height;
  coef = ((win_w < win_h)? win_w: win_h) * GetZoomPercent();
  ox = win_w*0.5;
  oy = win_h*0.5;

  /* clean */
  color = GetBackGroundColor();
  mdv_draw_rectangle(0, 0, draw->allocation.width, draw->allocation.height,
    color, TRUE);

  /* draw all primitives */
  dp = drawlist;
  for (i = drawlist_n-1; i >= 0; i--) { 
      switch (dp->flag) {
          case DRAW_OVALF:
              a = (gint) ((dp->a + view_origin_x)*coef + ox);
              b = (gint) ((dp->b + view_origin_y)*coef + oy);
              c = d = (gint) (dp->c*coef);
              if (a+c >= 0 && a <= win_w && b+d >= 0 && b <= win_h)
                  mdv_draw_oval(a, b, c, d, dp->col, TRUE);
              break;
          case DRAW_OVALL:
              a = (gint) ((dp->a + view_origin_x)*coef + ox);
              b = (gint) ((dp->b + view_origin_y)*coef + oy);
              c = d = (gint) (dp->c*coef);
              if (a+c >= 0 && a <= win_w && b+d >= 0 && b <= win_h)
                  if (c >= 3) mdv_draw_oval(a, b, c, d, dp->col, FALSE);
              break;
          case DRAW_LINE:
              a = (gint) ((dp->a + view_origin_x)*coef + ox);
              b = (gint) ((dp->b + view_origin_y)*coef + oy);
              c = (gint) ((dp->c + view_origin_x)*coef + ox);
              d = (gint) ((dp->d + view_origin_y)*coef + oy);
              if (((a>c)?a:c) >= 0 && ((a<c)?a:c) <= win_w
                      && ((b>d)?b:d) >= 0 && ((b<d)?b:d) <= win_h)
                  mdv_draw_line(a, b, c, d, dp->col);
              break;
      }
      dp++;
  }
  ShowEulerAngle();

  Redraw();
}

void Redraw(void) {
  static GdkFont *font = NULL;

  if (font == NULL) {
    if ((font = gdk_font_load(
        "-adobe-helvetica-medium-r-normal--14-*-*-*-*-*-*-*")) == NULL)
      g_error("Cannot load font.");
  }

  if (pixmap_screen != NULL) {
    gdk_draw_pixmap(draw->window,
      draw->style->fg_gc[GTK_WIDGET_STATE(draw)],
      pixmap_screen,
      0, 0,
      0, 0,
      draw->allocation.width, draw->allocation.height);

    /* comment string */
    if (comment_string != NULL) {
      long sec, usec;
      gint x, y;

      GetTimeval(&sec, &usec);
      if ((sec - comment_string_sec)*1000000 + (usec - comment_string_usec)
          < COMMENT_STRING_TIMEOUT_USEC) {
        gdk_gc_set_foreground(gc, mdv_color[MDV_COLOR_WHITE]);
        x = draw->allocation.width - gdk_string_width(font, comment_string);
        y = draw->allocation.height - gdk_string_height(font, comment_string);
        gdk_draw_string(draw->window, font, gc, x, y, comment_string);
      }
    }
  }
  if (screen_busy) {
    while (gtk_events_pending())
      gtk_main_iteration();
  }
  gdk_flush();
}


void GuiSetEulerAngle(gdouble theta, gdouble psi, gdouble phi) {
	gchar *string1, *string2, *string3;
	string1 = g_strdup_printf("%.3f", theta);
	string2 = g_strdup_printf("%.3f", psi);
	string3 = g_strdup_printf("%.3f", phi);
	gtk_entry_set_text(GTK_ENTRY(euler_theta), string1);
	gtk_entry_set_text(GTK_ENTRY(euler_psi), string2);
	gtk_entry_set_text(GTK_ENTRY(euler_phi), string3);
	g_free(string1);
	g_free(string2);
	g_free(string3);
	return;
}

#define TWO_PI (2.0*3.14159265358979323)
void ShowEulerAngle(void) {
	double theta, psi, phi;
	GetEulerAngle(&theta,&psi,&phi);
	theta *= 360.0/TWO_PI;
	psi *= 360.0/TWO_PI;
	phi *= 360.0/TWO_PI;
	psi = (psi < 359.9995)? psi: 0.0;
	phi = (phi < 359.9995)? phi: 0.0;
	GuiSetEulerAngle(theta, psi, phi);
	return;
}

void GuiSetCounter(int nstep) {
	gchar *string;
	string = g_strdup_printf("%d", nstep+1);
	gtk_entry_set_text(GTK_ENTRY(entry_step), string);
	g_free(string);
	return;
}

void GuiSetMouseMode(int mode) {
	if (mode == DRAG_TRANSLATION) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton_translation), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobutton_rotation), TRUE);
	}
	return;
}

void get_trans_parameter(double *px, double *py){
	*px = view_origin_x;
	*py = view_origin_y;
	return;
}

void set_trans_parameter(double x, double y) {
	view_origin_x = x;
	view_origin_y = y;
	return;
}

void CreateGui(void) {
	GtkWidget *vbox, *hbox, *frame;
	GtkWidget *vbox1, *hbox1, *hbox1a, *label1, *button_step_g, *button_step_b;
	GtkWidget *frame_step, *align_step;
	GtkWidget *button_step_k, *button_step_j, *button_step_f;
	GtkWidget *button_step_G, *button_stop;
	GtkWidget *hbox2, *vbox2a, *hbox2a, *table2a, *table2b;
	GtkWidget *label2a;
	GtkWidget *button_rotate_1, *button_rotate_2, *button_rotate_3;
	GtkWidget *button_rotate_4, *button_rotate_6, *button_rotate_8;
	GtkWidget *button_rotate, *button_rotate_plus, *button_rotate_minus;
	GtkWidget *label2b1, *label2b2, *label2b3;
	GtkWidget *vbox2b, *vbox2c, *vbox2d;
	GtkWidget *label2c, *label2d;
	GtkWidget *step_interval, *delay_choice;
	GtkWidget *menu_interval, *menu_interval_none, *menu_interval_2;
	GtkWidget *menu_interval_3, *menu_interval_5, *menu_interval_10;
	GtkWidget *menu_delay, *menu_delay_none, *menu_delay_8, *menu_delay_10;
	GtkWidget *menu_delay_12, *menu_delay_15, *menu_delay_24, *menu_delay_30;
	GtkWidget *hbox3, *button_zoom, *entry_zoom, *scale3;
	GtkWidget *hbox4, *button_distance, *entry_distance, *scale4;
	GtkWidget *hbox5, *hbox6;
	GtkWidget *button_outline, *button_mark, *button_move, *button_info;
	GtkWidget *button_convert, *button_layer, *button_help;
	GtkWidget *hbox7, *hbox7a, *label7a, *button_quit;
	GtkWidget *pixmapwid;
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GtkTooltips *tooltips;
	GtkStyle *style;
	GSList *group5;
	gchar *string;
	/*
	 * Create pixmap for buttons
	 */
	style = gtk_style_new();
	/*
	 * window_main on toplevel
	 */
	window_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_policy(GTK_WINDOW(window_main), TRUE, TRUE, FALSE);
	gtk_widget_add_events(window_main, GDK_KEY_RELEASE_MASK);
	gtk_signal_connect (GTK_OBJECT(window_main), "delete_event",
			GTK_SIGNAL_FUNC(delete_event), NULL);
	gtk_signal_connect(GTK_OBJECT(window_main), "destroy", 
			GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
	AddKeyEvent();
	gtk_container_border_width(GTK_CONTAINER(window_main), 5);
	/*
	 * hbox on window_main
	 */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window_main), hbox);
	gtk_widget_show(hbox);
	/*
	 * frame on hbox
	 */
	frame = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_widget_show(frame);
	/*
	 * draw on frame
	 */
	draw = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(frame), draw);
	gtk_widget_add_events(draw, GDK_ALL_EVENTS_MASK);
	gtk_signal_connect(GTK_OBJECT(draw), "expose_event",
			(GtkSignalFunc)expose_event, NULL);
	gtk_signal_connect(GTK_OBJECT(draw), "button_press_event",
			(GtkSignalFunc)button_event, NULL);
	gtk_signal_connect(GTK_OBJECT(draw), "button_release_event",
			(GtkSignalFunc)button_event, NULL);
	gtk_signal_connect(GTK_OBJECT(draw), "motion_notify_event",
			(GtkSignalFunc)button_event, NULL);
	gtk_widget_set_usize(draw, window_size, window_size);
	gtk_widget_show(draw);
	/*
	 * vbox on hbox
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);
	gtk_widget_show(vbox);
	/*
	 * vbox1 on vbox
	 */
	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), vbox1, FALSE, FALSE, 5);
	gtk_widget_show(vbox1);
	/*
	 * label1 on vbox1
	 */
	label1 = gtk_label_new("step");
	gtk_misc_set_alignment(GTK_MISC(label1), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox1), label1, FALSE, FALSE, 0);
	gtk_widget_show(label1);
	/*
	 * hbox1 on vbox1
	 */
	hbox1 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 5);
	gtk_widget_show(hbox1);
	/*
	 * hbox1a on hbox1
	 */
	hbox1a = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox1), hbox1a, FALSE, FALSE, 5);
	gtk_widget_show(hbox1a);
	/*
	 * button_step_g on hbox1a
	 */
	button_step_g = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_step_g, GTK_CAN_FOCUS);
	pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_14_xpm);
	pixmapwid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmapwid);
	gtk_container_add(GTK_CONTAINER(button_step_g), pixmapwid);
	gtk_box_pack_start(GTK_BOX(hbox1a), button_step_g, FALSE, FALSE, 0);
	gtkraw_signal_connect_buttons(button_step_g, cb_step_g, FALSE);
	gtk_widget_show(button_step_g);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_step_g, "go to the first step", NULL);
	/*
	 * button_step_b on hbox1a
	 */
	button_step_b = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_step_b, GTK_CAN_FOCUS);
	pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_44_xpm);
	pixmapwid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmapwid);
	gtk_container_add(GTK_CONTAINER(button_step_b), pixmapwid);
	gtk_box_pack_start(GTK_BOX(hbox1a), button_step_b, FALSE, FALSE, 0);
	gtkraw_signal_connect_buttons(button_step_b, cb_step_b, TRUE);
	gtk_widget_show(button_step_b);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_step_b, "backward 25*'inverval' step", NULL);
	/*
	 * button_step_k on hbox1a
	 */
	button_step_k = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_step_k, GTK_CAN_FOCUS);
	pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_4_xpm);
	pixmapwid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmapwid);
	gtk_container_add(GTK_CONTAINER(button_step_k), pixmapwid);
	gtk_box_pack_start(GTK_BOX(hbox1a), button_step_k, FALSE, FALSE, 0);
	gtkraw_signal_connect_buttons(button_step_k, cb_step_k, TRUE);
	gtk_widget_show(button_step_k);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_step_k, "backward 'interval' step", NULL);
	/*
	 * frame_step on hbox1a
	 */
	frame_step = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox1a), frame_step, FALSE, FALSE, 0);
	gtk_widget_set_usize(frame_step, 85, 25);
	gtk_frame_set_label_align(GTK_FRAME(frame_step), 0.5, 0.5);
	gtk_frame_set_shadow_type(GTK_FRAME(frame_step), GTK_SHADOW_ETCHED_OUT);
	gtk_widget_show(frame_step);
	/*
	 * align_step on frame_step
	 */
	align_step = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
	gtk_container_add(GTK_CONTAINER(frame_step), align_step);
	gtk_widget_show(align_step);
	/*
	 * entry_step on align_step
	 */
	entry_step = gtk_entry_new_with_max_length(6);
	gtk_container_add(GTK_CONTAINER(align_step), entry_step);
	gtk_signal_connect(GTK_OBJECT(entry_step), "changed",
			GTK_SIGNAL_FUNC(cb_step_changed), NULL);
	gtk_signal_connect(GTK_OBJECT(entry_step), "focus-in-event",
			GTK_SIGNAL_FUNC(cb_entry_focus_in), NULL);
	gtk_signal_connect(GTK_OBJECT(entry_step), "focus-out-event",
			GTK_SIGNAL_FUNC(cb_step_focus_out), NULL);
	gtk_signal_connect(GTK_OBJECT(entry_step), "activate",
			GTK_SIGNAL_FUNC(cb_entry_activate), NULL);
	gtk_entry_set_text(GTK_ENTRY(entry_step), "1");
	GTK_ENTRY_FIT(GTK_ENTRY(entry_step), "1");
	gtk_widget_show(entry_step);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, entry_step, "Go to the Arbitary Step", NULL);
	/*
	 * button_step_j on hbox1a
	 */
	button_step_j = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_step_j, GTK_CAN_FOCUS);
	pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_6_xpm);
	pixmapwid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmapwid);
	gtk_container_add(GTK_CONTAINER(button_step_j), pixmapwid);
	gtk_box_pack_start(GTK_BOX(hbox1a), button_step_j, FALSE, FALSE, 0);
	gtkraw_signal_connect_buttons(button_step_j, cb_step_j, TRUE);
	gtk_widget_show(button_step_j);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_step_j, "forward 'interval' step", NULL);
	/*
	 * button_step_f on hbox1a
	 */
	button_step_f = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_step_f, GTK_CAN_FOCUS);
	pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_66_xpm);
	pixmapwid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmapwid);
	gtk_container_add(GTK_CONTAINER(button_step_f), pixmapwid);
	gtk_box_pack_start(GTK_BOX(hbox1a), button_step_f, FALSE, FALSE, 0);
	gtkraw_signal_connect_buttons(button_step_f, cb_step_f, TRUE);
	gtk_widget_show(button_step_f);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_step_f, "forward 25*'inverval' step", NULL);
	/*
	 * button_step_G on hbox1a
	 */
	button_step_G = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_step_G, GTK_CAN_FOCUS);
	pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_61_xpm);
	pixmapwid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmapwid);
	gtk_container_add(GTK_CONTAINER(button_step_G), pixmapwid);
	gtk_box_pack_start(GTK_BOX(hbox1a), button_step_G, FALSE, FALSE, 0);
	gtkraw_signal_connect_buttons(button_step_G, cb_step_G, FALSE);
	gtk_widget_show(button_step_G);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_step_G, "go to last step", NULL);
	/*
	 * button_stop on hbox1
	 */
	button_stop = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_stop, GTK_CAN_FOCUS);
	pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_stop_xpm);
	pixmapwid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmapwid);
	gtk_container_add(GTK_CONTAINER(button_stop), pixmapwid);
	gtk_box_pack_start(GTK_BOX(hbox1), button_stop, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button_stop), "pressed",
			GTK_SIGNAL_FUNC(cb_stop), NULL);
	gtk_widget_show(button_stop);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_stop, "interrupt", NULL);
	/*
	 * hbox2 on vbox
	 */
	hbox2 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 10);
	gtk_widget_show(hbox2);
	/*
	 * vbox2a on hbox2
	 */
	vbox2a = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), vbox2a, FALSE, FALSE, 0);
	gtk_widget_show(vbox2a);
	/*
	 * label2a on vbox2a
	 */
	label2a = gtk_label_new("rotate");
	gtk_misc_set_alignment(GTK_MISC(label2a), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox2a), label2a, FALSE, FALSE, 0);
	gtk_widget_show(label2a);
	/*
	 * hbox2a on vbox2a
	 */
	hbox2a = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2a), hbox2a, FALSE, FALSE, 0);
	gtk_widget_show(hbox2a);
	/*
	 * table2a on hbox2a
	 */
	table2a = gtk_table_new(3, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox2a), table2a, FALSE, FALSE, 5);
	gtk_widget_show(table2a);
	/*
	 * button_rotate_1 on table2a
	 */
	button_rotate_1 = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_rotate_1, GTK_CAN_FOCUS);
	gpixmap[0][0] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[0][0],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_1_xpm);
	gpixmap[0][1] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[0][1],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_1_1_xpm);
	gpixmap[0][2] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[0][2],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_1_2_xpm);
	gpixmap[0][3] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[0][3],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_1_3_xpm);
	gpixmapwid[0] = gtk_pixmap_new(gpixmap[0][0], gmask[0][0]);
	gtk_widget_show(gpixmapwid[0]);
	gtk_container_add(GTK_CONTAINER(button_rotate_1), gpixmapwid[0]);
	gtkraw_signal_connect_buttons(button_rotate_1, cb_rotate_1, TRUE);
	gtk_table_attach(GTK_TABLE(table2a), button_rotate_1, 0, 1, 0, 1,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show(button_rotate_1);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_rotate_1, "rotate counterclockwise", NULL);
	/*
	 * button_rotate_8 on table2a
	 */
	button_rotate_8 = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_rotate_8, GTK_CAN_FOCUS);
	gpixmap[5][0] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[5][0],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_8_xpm);
	gpixmap[5][1] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[5][1],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_8_1_xpm);
	gpixmap[5][2] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[5][2],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_8_2_xpm);
	gpixmap[5][3] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[5][3],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_8_3_xpm);
	gpixmapwid[5] = gtk_pixmap_new(gpixmap[5][0], gmask[5][0]);
	gtk_widget_show(gpixmapwid[5]);
	gtk_container_add(GTK_CONTAINER(button_rotate_8), gpixmapwid[5]);
	gtk_table_attach(GTK_TABLE(table2a), button_rotate_8, 1, 2, 0, 1,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtkraw_signal_connect_buttons(button_rotate_8, cb_rotate_8, TRUE);
	gtk_widget_show(button_rotate_8);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_rotate_8, "rotate upper", NULL);
	/*
	 * button_rotate_3 on table2a
	 */
	button_rotate_3 = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_rotate_3, GTK_CAN_FOCUS);
	gpixmap[2][0] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[2][0],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_3_xpm);
	gpixmap[2][1] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[2][1],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_3_1_xpm);
	gpixmap[2][2] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[2][2],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_3_2_xpm);
	gpixmap[2][3] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[2][3],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_3_3_xpm);
	gpixmapwid[2] = gtk_pixmap_new(gpixmap[2][0], gmask[2][0]);
	gtk_widget_show(gpixmapwid[2]);
	gtk_container_add(GTK_CONTAINER(button_rotate_3), gpixmapwid[2]);
	gtk_table_attach(GTK_TABLE(table2a), button_rotate_3, 2, 3, 0, 1,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtkraw_signal_connect_buttons(button_rotate_3, cb_rotate_3, TRUE);
	gtk_widget_show(button_rotate_3);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_rotate_3, "rotate clockwise", NULL);
	/*
	 * button_rotate_4 on table2a
	 */
	button_rotate_4 = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_rotate_4, GTK_CAN_FOCUS);
	gpixmap[3][0] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[3][0],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_4_xpm);
	gpixmap[3][1] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[3][1],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_4_1_xpm);
	gpixmap[3][2] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[3][2],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_4_2_xpm);
	gpixmap[3][3] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[3][3],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_4_3_xpm);
	gpixmapwid[3] = gtk_pixmap_new(gpixmap[3][0], gmask[3][0]);
	gtk_widget_show(gpixmapwid[3]);
	gtk_container_add(GTK_CONTAINER(button_rotate_4), gpixmapwid[3]);
	gtk_table_attach(GTK_TABLE(table2a), button_rotate_4, 0, 1, 1, 2,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtkraw_signal_connect_buttons(button_rotate_4, cb_rotate_4, TRUE);
	gtk_widget_show(button_rotate_4);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_rotate_4, "rotate left", NULL);
	/*
	 * button_rotate on table2a
	 */
	button_rotate = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_rotate, GTK_CAN_FOCUS);
	pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, 
			gdk_colormap_get_system(), &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_rotate_xpm);
	pixmapwid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmapwid);
	gtk_container_add(GTK_CONTAINER(button_rotate), pixmapwid);
	gtk_table_attach(GTK_TABLE(table2a), button_rotate, 1, 2, 1, 2,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtkraw_signal_connect_buttons(button_rotate, cb_rotate, FALSE);
	gtk_widget_show(button_rotate);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_rotate, "turn around", NULL);
	/*
	 * button_rotate_6 on table2a
	 */
	button_rotate_6 = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_rotate_6, GTK_CAN_FOCUS);
	gpixmap[4][0] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[4][0],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_6_xpm);
	gpixmap[4][1] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[4][1],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_6_1_xpm);
	gpixmap[4][2] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[4][2],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_6_2_xpm);
	gpixmap[4][3] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[4][3],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_6_3_xpm);
	gpixmapwid[4] = gtk_pixmap_new(gpixmap[4][0], gmask[4][0]);
	gtk_widget_show(gpixmapwid[4]);
	gtk_container_add(GTK_CONTAINER(button_rotate_6), gpixmapwid[4]);
	gtk_table_attach(GTK_TABLE(table2a), button_rotate_6, 2, 3, 1, 2,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtkraw_signal_connect_buttons(button_rotate_6, cb_rotate_6, TRUE);
	gtk_widget_show(button_rotate_6);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_rotate_6, "rotate right", NULL);
	/*
	 * button_rotate_minus on table2a
	 */
	button_rotate_minus = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_rotate_minus, GTK_CAN_FOCUS);
	pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_minus_xpm);
	pixmapwid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmapwid);
	gtk_container_add(GTK_CONTAINER(button_rotate_minus), pixmapwid);
	gtk_table_attach(GTK_TABLE(table2a), button_rotate_minus, 0, 1, 2, 3,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtkraw_signal_connect_buttons(button_rotate_minus, cb_rotate_minus, FALSE);
	gtk_widget_show(button_rotate_minus);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_rotate_minus, "smaller rotatation angle", NULL);
	/*
	 * button_rotate_2 on table2a
	 */
	button_rotate_2 = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_rotate_2, GTK_CAN_FOCUS);
	gpixmap[1][0] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[1][0],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_2_xpm);
	gpixmap[1][1] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[1][1],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_2_1_xpm);
	gpixmap[1][2] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[1][2],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_2_2_xpm);
	gpixmap[1][3] = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &gmask[1][3],
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_2_3_xpm);
	gpixmapwid[1] = gtk_pixmap_new(gpixmap[1][0], gmask[1][0]);
	gtk_widget_show(gpixmapwid[1]);
	gtk_container_add(GTK_CONTAINER(button_rotate_2), gpixmapwid[1]);
	gtk_table_attach(GTK_TABLE(table2a), button_rotate_2, 1, 2, 2, 3,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtkraw_signal_connect_buttons(button_rotate_2, cb_rotate_2, TRUE);
	gtk_widget_show(button_rotate_2);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_rotate_2, "rotate lower", NULL);
	/*
	 * button_rotate_plus on table2a
	 */
	button_rotate_plus = gtk_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_rotate_plus, GTK_CAN_FOCUS);
	pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, 
			gdk_colormap_get_system(), &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_plus_xpm);
	pixmapwid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmapwid);
	gtk_container_add(GTK_CONTAINER(button_rotate_plus), pixmapwid);
	gtk_table_attach(GTK_TABLE(table2a), button_rotate_plus, 2, 3, 2, 3,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtkraw_signal_connect_buttons(button_rotate_plus, cb_rotate_plus, FALSE);
	gtk_widget_show(button_rotate_plus);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_rotate_plus, "bigger rotatation angle", NULL);
	/*
	 * table2b on hbox2a
	 */
	table2b = gtk_table_new(2, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox2a), table2b, FALSE, FALSE, 5);
	gtk_widget_show(table2b);
	/*
	 * label2b1 on table2b
	 */
	label2b1 = gtk_label_new("theta");
#if 0
	gtk_widget_set_name(label2b1, "symbol font");
#endif
#if 0
	{
		GtkStyle *style_symbol;
		GdkFont *old_font;
		style_symbol = gtk_widget_get_style(label2b1);
		old_font = style_symbol->font;
		style_symbol->font = gdk_font_load("-*-symbol-*-*-*-*-14-*-*-*-*-*-adobe-*");
		if (style_symbol->font)
			gdk_font_unref(old_font);
		else
			style_symbol->font = old_font;
		gtk_widget_set_style(label2b1, style_symbol);
	}
#endif
	gtk_table_attach(GTK_TABLE(table2b), label2b1, 0, 1, 0, 1,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show(label2b1);
	/*
	 * label2b2 on table2b
	 */
	label2b2 = gtk_label_new("psi");
	gtk_table_attach(GTK_TABLE(table2b), label2b2, 0, 1, 1, 2,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show(label2b2);
	/*
	 * label2b3 on table2b
	 */
	label2b3 = gtk_label_new("phi");
	gtk_table_attach(GTK_TABLE(table2b), label2b3, 0, 1, 2, 3,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_widget_show(label2b3);
	/*
	 * euler_theta on table2b
	 */
	euler_theta = gtk_entry_new();
	gtk_widget_set_usize(euler_theta, 60, 25);
	gtk_table_attach(GTK_TABLE(table2b), euler_theta, 1, 2, 0, 1,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(euler_theta), "focus-in-event",
			GTK_SIGNAL_FUNC(cb_entry_focus_in), NULL);
	gtk_signal_connect(GTK_OBJECT(euler_theta), "focus-out-event",
			GTK_SIGNAL_FUNC(cb_euler_focus_out), NULL);
	gtk_signal_connect(GTK_OBJECT(euler_theta), "activate",
			GTK_SIGNAL_FUNC(cb_entry_activate), NULL);
	gtk_widget_show(euler_theta);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, euler_theta, "set Euler angle", NULL);
	/*
	 * euler_psi on table2b
	 */
	euler_psi = gtk_entry_new();
	gtk_widget_set_usize(euler_psi, 60, 25);
	gtk_table_attach(GTK_TABLE(table2b), euler_psi, 1, 2, 1, 2,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(euler_psi), "focus-in-event",
			GTK_SIGNAL_FUNC(cb_entry_focus_in), NULL);
	gtk_signal_connect(GTK_OBJECT(euler_psi), "focus-out-event",
			GTK_SIGNAL_FUNC(cb_euler_focus_out), NULL);
	gtk_signal_connect(GTK_OBJECT(euler_psi), "activate",
			GTK_SIGNAL_FUNC(cb_entry_activate), NULL);
	gtk_widget_show(euler_psi);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, euler_psi, "set Euler angle", NULL);
	/*
	 * euler_phi on table2b
	 */
	euler_phi = gtk_entry_new();
	gtk_widget_set_usize(euler_phi, 60, 25);
	gtk_table_attach(GTK_TABLE(table2b), euler_phi, 1, 2, 2, 3,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	gtk_signal_connect(GTK_OBJECT(euler_phi), "focus-in-event",
			GTK_SIGNAL_FUNC(cb_entry_focus_in), NULL);
	gtk_signal_connect(GTK_OBJECT(euler_phi), "focus-out-event",
			GTK_SIGNAL_FUNC(cb_euler_focus_out), NULL);
	gtk_signal_connect(GTK_OBJECT(euler_phi), "activate",
			GTK_SIGNAL_FUNC(cb_entry_activate), NULL);
	gtk_widget_show(euler_phi);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, euler_phi, "set Euler angle", NULL);
	/*
	 * vbox2b on hbox2
	 */
	vbox2b = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), vbox2b, FALSE, FALSE, 15);
	gtk_widget_show(vbox2b);
	/*
	 * vbox2c on vbox2b
	 */
	vbox2c = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2b), vbox2c, FALSE, FALSE, 0);
	gtk_widget_show(vbox2c);
	/*
	 * label2c on vbox2c
	 */
	label2c = gtk_label_new("Interval");
	gtk_misc_set_alignment(GTK_MISC(label2c), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox2c), label2c, FALSE, FALSE, 0);
	gtk_widget_show(label2c);
	/*
	 * step_interval on vbox2c
	 */
	step_interval = gtk_option_menu_new();
	GTK_WIDGET_UNSET_FLAGS(step_interval, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2c), step_interval, FALSE, FALSE, 0);
	menu_interval = gtk_menu_new();
	menu_interval_none = gtk_menu_item_new_with_label("--");
	menu_interval_2 = gtk_menu_item_new_with_label("2");
	menu_interval_3 = gtk_menu_item_new_with_label("3");
	menu_interval_5 = gtk_menu_item_new_with_label("5");
	menu_interval_10 = gtk_menu_item_new_with_label("10");
	gtk_signal_connect(GTK_OBJECT(menu_interval_none), "activate",
			GTK_SIGNAL_FUNC(cb_interval), GINT_TO_POINTER(1));
	gtk_signal_connect(GTK_OBJECT(menu_interval_2), "activate",
			GTK_SIGNAL_FUNC(cb_interval), GINT_TO_POINTER(2));
	gtk_signal_connect(GTK_OBJECT(menu_interval_3), "activate",
			GTK_SIGNAL_FUNC(cb_interval), GINT_TO_POINTER(3));
	gtk_signal_connect(GTK_OBJECT(menu_interval_5), "activate",
			GTK_SIGNAL_FUNC(cb_interval), GINT_TO_POINTER(5));
	gtk_signal_connect(GTK_OBJECT(menu_interval_10), "activate",
			GTK_SIGNAL_FUNC(cb_interval), GINT_TO_POINTER(10));
	gtk_widget_show(menu_interval_none);
	gtk_widget_show(menu_interval_2);
	gtk_widget_show(menu_interval_3);
	gtk_widget_show(menu_interval_5);
	gtk_widget_show(menu_interval_10);
	gtk_menu_append(GTK_MENU(menu_interval), menu_interval_none);
	gtk_menu_append(GTK_MENU(menu_interval), menu_interval_2);
	gtk_menu_append(GTK_MENU(menu_interval), menu_interval_3);
	gtk_menu_append(GTK_MENU(menu_interval), menu_interval_5);
	gtk_menu_append(GTK_MENU(menu_interval), menu_interval_10);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(step_interval), menu_interval);
	gtk_widget_show(step_interval);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, step_interval, "interval number", NULL);
	/*
	 * vbox2d on vbox2b
	 */
	vbox2d = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2b), vbox2d, FALSE, FALSE, 0);
	gtk_widget_show(vbox2d);
	/*
	 * label2d on vbox2d
	 */
	label2d = gtk_label_new("frame/sec");
	gtk_misc_set_alignment(GTK_MISC(label2d), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox2d), label2d, FALSE, FALSE, 0);
	gtk_widget_show(label2d);
	/*
	 * delay_choice on vbox2c
	 */
	delay_choice = gtk_option_menu_new();
	GTK_WIDGET_UNSET_FLAGS(delay_choice, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2d), delay_choice, FALSE, FALSE, 0);
	menu_delay = gtk_menu_new();
	menu_delay_none = gtk_menu_item_new_with_label("--");
	menu_delay_8 = gtk_menu_item_new_with_label("8");
	menu_delay_10 = gtk_menu_item_new_with_label("10");
	menu_delay_12 = gtk_menu_item_new_with_label("12");
	menu_delay_15 = gtk_menu_item_new_with_label("15");
	menu_delay_24 = gtk_menu_item_new_with_label("24");
	menu_delay_30 = gtk_menu_item_new_with_label("30");
	gtk_signal_connect(GTK_OBJECT(menu_delay_none), "activate",
			GTK_SIGNAL_FUNC(cb_frame), GINT_TO_POINTER(0));
	gtk_signal_connect(GTK_OBJECT(menu_delay_8), "activate",
			GTK_SIGNAL_FUNC(cb_frame), GINT_TO_POINTER(8));
	gtk_signal_connect(GTK_OBJECT(menu_delay_10), "activate",
			GTK_SIGNAL_FUNC(cb_frame), GINT_TO_POINTER(10));
	gtk_signal_connect(GTK_OBJECT(menu_delay_12), "activate",
			GTK_SIGNAL_FUNC(cb_frame), GINT_TO_POINTER(12));
	gtk_signal_connect(GTK_OBJECT(menu_delay_15), "activate",
			GTK_SIGNAL_FUNC(cb_frame), GINT_TO_POINTER(15));
	gtk_signal_connect(GTK_OBJECT(menu_delay_24), "activate",
			GTK_SIGNAL_FUNC(cb_frame), GINT_TO_POINTER(24));
	gtk_signal_connect(GTK_OBJECT(menu_delay_30), "activate",
			GTK_SIGNAL_FUNC(cb_frame), GINT_TO_POINTER(30));
	gtk_widget_show(menu_delay_none);
	gtk_widget_show(menu_delay_8);
	gtk_widget_show(menu_delay_10);
	gtk_widget_show(menu_delay_12);
	gtk_widget_show(menu_delay_15);
	gtk_widget_show(menu_delay_24);
	gtk_widget_show(menu_delay_30);
	gtk_menu_append(GTK_MENU(menu_delay), menu_delay_none);
	gtk_menu_append(GTK_MENU(menu_delay), menu_delay_8);
	gtk_menu_append(GTK_MENU(menu_delay), menu_delay_10);
	gtk_menu_append(GTK_MENU(menu_delay), menu_delay_12);
	gtk_menu_append(GTK_MENU(menu_delay), menu_delay_15);
	gtk_menu_append(GTK_MENU(menu_delay), menu_delay_24);
	gtk_menu_append(GTK_MENU(menu_delay), menu_delay_30);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(delay_choice), menu_delay);
	gtk_widget_show(delay_choice);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, delay_choice, "delay rate", NULL);
	/*
	 * hbox3 on vbox
	 */
	hbox3 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox3, FALSE, FALSE, 5);
	gtk_widget_show(hbox3);
	/*
	 * button_zoom on hbox3
	 */
	button_zoom = zoom_gtk_button();
	gtk_box_pack_start(GTK_BOX(hbox3), button_zoom, FALSE, FALSE, 0);
	gtk_widget_show(button_zoom);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_zoom, "Open Window to Set Range of Zoom", NULL);
	/*
	 * entry_zoom on hbox3
	 */
	entry_zoom = zoom_gtk_entry();
	gtk_box_pack_start(GTK_BOX(hbox3), entry_zoom, FALSE, FALSE, 0);
	gtk_widget_show(entry_zoom);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, entry_zoom, "zoom slider", NULL);
	/*
	 *      * scale3 on hbox3
	 *           */
	scale3 = zoom_gtk_hscale();
	gtk_box_pack_start(GTK_BOX(hbox3), scale3, FALSE, FALSE, 0);
	gtk_widget_show(scale3);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, scale3, "Zoom Slider", NULL);
	/*
	 * hbox4 on vbox
	 */
	hbox4 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox4, FALSE, FALSE, 5);
	gtk_widget_show(hbox4);
	/*
	 * button_distance on hbox4
	 */
	button_distance = distance_gtk_button();
	gtk_box_pack_start(GTK_BOX(hbox4), button_distance, FALSE, FALSE, 0);
	gtk_widget_show(button_distance);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_distance, "Open Window to Set Range of Distance", NULL);
	/*
	 * entry_distance on hbox4
	 */
	entry_distance = distance_gtk_entry();
	gtk_box_pack_start(GTK_BOX(hbox4), entry_distance, FALSE, FALSE, 0);
	gtk_widget_show(entry_distance);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, entry_distance, "Arbitary Distance", NULL);
	/*
	 * scale4 on hbox4
	 */
	scale4 = distance_gtk_hscale();
	gtk_box_pack_start(GTK_BOX(hbox4), scale4, FALSE, FALSE, 0);
	gtk_widget_show(scale4);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, scale4, "Distance Slider", NULL);
	/*
	 * hbox5 on vbox
	 */
	hbox5 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox5, FALSE, FALSE, 5);
	gtk_widget_show(hbox5);
	/* 
	 * radiobutton_translation on hbox5
	 */
	radiobutton_translation = gtk_radio_button_new_with_label(NULL, "Translation");
	GTK_WIDGET_UNSET_FLAGS(radiobutton_translation, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(hbox5), radiobutton_translation, TRUE, TRUE, 25);
	gtk_signal_connect(GTK_OBJECT(radiobutton_translation), "pressed",
			GTK_SIGNAL_FUNC(cb_drag_mode), GINT_TO_POINTER(DRAG_TRANSLATION));
	gtk_widget_show(radiobutton_translation);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, radiobutton_translation, "translation mode", NULL);
	group5 = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton_translation));
	radiobutton_rotation = gtk_radio_button_new_with_label(group5, "Rotation");
	GTK_WIDGET_UNSET_FLAGS(radiobutton_rotation, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(hbox5), radiobutton_rotation, TRUE, TRUE, 25);
	gtk_signal_connect(GTK_OBJECT(radiobutton_rotation), "pressed",
			GTK_SIGNAL_FUNC(cb_drag_mode), GINT_TO_POINTER(DRAG_ROTATION));
	gtk_widget_show(radiobutton_rotation);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, radiobutton_rotation, "rotation mode", NULL);
	/*
	 * hbox6 on vbox
	 */
	hbox6 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox6, FALSE, FALSE, 5);
	gtk_widget_show(hbox6);
	/*
	 * button_outline on hbox6
	 */
	button_outline = gtk_toggle_button_new_with_label("Outline");
	GTK_WIDGET_UNSET_FLAGS(button_outline, GTK_CAN_FOCUS);
#if 0
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button_outline), FALSE);
#endif
	gtk_box_pack_start(GTK_BOX(hbox6), button_outline, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button_outline), "clicked",
			GTK_SIGNAL_FUNC(cb_outline), NULL);
	gtk_widget_show(button_outline);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_outline, "show/hide outline", NULL);
	/*
	 * button_mark on hbox6
	 */
	button_mark = gtk_button_new_with_label("Mark");
	GTK_WIDGET_UNSET_FLAGS(button_mark, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(hbox6), button_mark, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button_mark), "clicked",
			GTK_SIGNAL_FUNC(cb_mark_open), NULL);
	gtk_widget_show(button_mark);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_mark, "mark button", NULL);
	/*
	 * button_move on hbox6
	 */
	button_move = gtk_button_new_with_label("Move");
	GTK_WIDGET_UNSET_FLAGS(button_move, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(hbox6), button_move, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button_move), "clicked",
			GTK_SIGNAL_FUNC(cb_move_open), NULL);
	gtk_widget_show(button_move);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_move, "move button", NULL);
	/*
	 * button_info on hbox6
	 */
	button_info = gtk_toggle_button_new_with_label("Info");
	GTK_WIDGET_UNSET_FLAGS(button_info, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(hbox6), button_info, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button_info), "clicked",
			GTK_SIGNAL_FUNC(cb_info), (gpointer)button_info);
	gtk_widget_show(button_info);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_info, "information button", NULL);
	/*
	 * button_convert on hbox6
	 */
	button_convert = gtk_button_new_with_label("Convert");
	GTK_WIDGET_UNSET_FLAGS(button_convert, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(hbox6), button_convert, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button_convert), "clicked",
			GTK_SIGNAL_FUNC(cb_convert), NULL);
	gtk_widget_show(button_convert);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_convert, "convert current scene to povray", NULL);
	/*
	 * button_layer on hbox6
	 */
	button_layer = gtk_toggle_button_new_with_label("Layer");
	GTK_WIDGET_UNSET_FLAGS(button_layer, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(hbox6), button_layer, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button_layer), "clicked",
			GTK_SIGNAL_FUNC(cb_layer_open), (gpointer)button_layer);
	gtk_widget_show(button_layer);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_layer, "layer window", NULL);
	/*
	 * button_help on hbox6
	 */
	button_help = gtk_toggle_button_new();
	GTK_WIDGET_UNSET_FLAGS(button_help, GTK_CAN_FOCUS);
	pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gdk_colormap_get_system(), &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)pixmap_help_xpm);
	pixmapwid = gtk_pixmap_new(pixmap, mask);
	gtk_widget_show(pixmapwid);
	gtk_container_add(GTK_CONTAINER(button_help), pixmapwid);
	gtk_box_pack_start(GTK_BOX(hbox6), button_help, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button_help), "clicked",
			GTK_SIGNAL_FUNC(cb_help), NULL);
	gtk_widget_show(button_help);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_help, "turn off this help", NULL);
	/*
	 * hbox7 on vbox
	 */
	hbox7 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox7, FALSE, FALSE, 5);
	gtk_widget_show(hbox7);
	/*
	 * hbox7a on hbox7
	 */
	hbox7a = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox7), hbox7a, FALSE, FALSE, 0);
	gtk_widget_show(hbox7a);
	/*
	 * label7a on hbox7a
	 */
	label7a = gtk_label_new("Input");
	gtk_box_pack_start(GTK_BOX(hbox7a), label7a, FALSE, FALSE, 2);
	gtk_widget_show(label7a);
	/*
	 * input on hbox7a
	 */
	input = gtk_entry_new();
	GTK_WIDGET_UNSET_FLAGS(input, GTK_CAN_FOCUS);
	gtk_widget_set_usize(input, 85, 25);
	gtk_entry_set_editable(GTK_ENTRY(input), FALSE);
	gtk_container_add(GTK_CONTAINER(hbox7a), input);
	gtk_widget_show(input);
#if 0
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, input, 
			"j/+: Forward one step.\n"
			"k/-: Backward one step.\n"
			"f/SPACE: Forward 25 steps.\n"
			"b: Backward 25 steps.\n"
			"e/s: Forward to end/Backward to start.\n"
			"g/G: Go to first/last step.\n"
			"z/Z: Zoom out/in.\n"
			"Aa/Dd/Ww/Xx: Rotate left/right/up/down.\n"
			"t: Turn full circle.\n"
			"N%: Zoom N %.\n"
			"Np: Pick up Nth atom.\n"
			"Nv/V: Move to Nth/marked atom.\n"
			"ctrl-c: Stop animation.\n"
			"q Q: Quit mdview.",
			NULL);
#endif
	/*
	 * input_text on hbox7
	 */
	input_text = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox7), input_text, FALSE, FALSE, 0);
	gtk_widget_set_usize(input_text, 85, 25);
	gtk_widget_show(input_text);
	/* 
	 * button_quit on hbox7
	 */
	button_quit = gtk_button_new_with_label("Quit");
	GTK_WIDGET_UNSET_FLAGS(button_quit, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(hbox7), button_quit, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button_quit), "clicked",
			GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
	gtk_widget_set_usize(button_quit, 50, 25);
	gtk_widget_show(button_quit);
	tooltips = gtk_tooltips_new();
	list_tooltips = g_slist_append(list_tooltips, (gpointer)tooltips);
	gtk_tooltips_set_tip(tooltips, button_quit, "quit button", NULL);
	/*
	 * Set initial value for some widget
	 */
	if (outline_mode == 1) {
		gtk_signal_handler_block_by_func(GTK_OBJECT(button_outline), GTK_SIGNAL_FUNC(cb_outline), NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_outline), TRUE);
		gtk_signal_handler_unblock_by_func(GTK_OBJECT(button_outline), GTK_SIGNAL_FUNC(cb_outline), NULL);
	}
	GuiSetDistance(distance);
	/*
	 * Turn off help messages.
	 */
	ToggleTooltips();
	/*
	 * Show Window and Main Loop.
	 */
	gtk_widget_show(window_main);
	/*
	 * Create GC
	 */
	gc = gdk_gc_new(window_main->window);
	/*
	 * Set title
	 */
	string = g_strdup_printf("mdview: %s", file_path);
	gtk_window_set_title(GTK_WINDOW(window_main), string);
	g_free(string);
	return;
}

/*
 * Callback functions for window_main
 */
static gint expose_event(GtkWidget *widget, GdkEventExpose *event) {
	if (draw->allocation.width != window_width || draw->allocation.height != window_height || window_width == -1) {
		window_width = draw->allocation.width;
		window_height = draw->allocation.height;
		if (pixmap_screen != NULL) {
			gdk_pixmap_unref(pixmap_screen);
		}
		pixmap_screen = gdk_pixmap_new(window_main->window, window_width, window_height, -1);
		DrawObject();
	}
	Redraw();
	return FALSE;
}

static gint button_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  static GdkFont *font = NULL;
  static gint isdrag = 0, mouse_x = 0, mouse_y = 0;
  GdkModifierType state;
  gdouble coef, ox, oy;
  gint mx, my, win_w, win_h;
  glong num;

  win_w = draw->allocation.width;
  win_h = draw->allocation.height;
  ox = win_w*0.5;
  oy = win_h*0.5;
  coef = ((win_w < win_h)? win_w: win_h) * GetZoomPercent();
  if (font == NULL) {
    font = gdk_font_load("-adobe-helvetica-medium-r-normal--14-*-*-*-*-*-*-*");
    if (font == NULL)
      g_error("Cannot load font.");
  }
  if (event->motion.is_hint) {
    gdk_window_get_pointer (event->motion.window, &mx, &my, &state);
  } else {
    mx = event->motion.x;
    my = event->motion.y;
    state = event->motion.state;
  }

  if (event->button.type == GDK_2BUTTON_PRESS) {
    if (event->button.button == 1) {
      if (drag_mode == DRAG_TRANSLATION) {
        command_put(COM_MODE_TRANSLATION, 0, 0.0, 0.0);
      } else {
        command_put(COM_MODE_EULER_ANGLE, 0.0, 0.0, 0.0);
      }
    }
  } else if (event->button.type == GDK_BUTTON_PRESS) {
    if (event->button.button == 1) {
      mouse_x = mx;
      mouse_y = my;
      isdrag = 1;
    } else if (!screen_busy) {
      double origin_x, origin_y;

      get_trans_parameter(&origin_x, &origin_y);
      num = WhichParticle((mx-ox)/coef - origin_x, (my-oy)/coef - origin_y);
      if (num >= 0) {
        if (event->button.button == 2) {
          command_put(COM_MODE_MARK, 1, num);
        } else {
          command_put(COM_MODE_COORDINATE, num);
        }
      }
    }
  } else if (state & GDK_BUTTON1_MASK) {
    if (isdrag && (mx != mouse_x || my != mouse_y)) {
      if (drag_mode == DRAG_TRANSLATION) {
        command_put(COM_MODE_TRANSLATION, 1, (mx-mouse_x)/coef,
          (my-mouse_y)/coef);
      } else {
        command_put(COM_MODE_DRAG_ROTATION, TWO_PI*(mx-mouse_x)/coef,
          TWO_PI*(my-mouse_y)/coef);
      }
    }
    mouse_x = mx;
    mouse_y = my;
  } else if (event->button.type == GDK_BUTTON_RELEASE) {
    isdrag = 0;
  }

  return FALSE;
}

gint key_event(GtkWidget *widget, GdkEventKey *event) {
	if (event->type == GDK_KEY_PRESS) {
		switch (event->keyval) {
			case GDK_BackSpace:
			case GDK_Delete:
				KeyOperation(event->keyval);
				break;
			case GDK_Q:
			case GDK_q:
				gtk_main_quit();
			case GDK_C:
			case GDK_c:
				if (gtkraw_key_control()) {
					StopButton();
				} else {
					KeyOperation(event->keyval);
				}
				break;
			default:
				if (event->keyval >=32 && event->keyval <= 127) {
					KeyOperation(event->keyval);
				}
		}
	}
	return FALSE;
}

#define MAX_COMMAND_LINE 8
#define MIN_COMMAND_SIZE MAX_COMMAND_LINE
static void KeyOperation(gint keyval) {
  static GString *command_line = NULL;
  gchar *last_com_str;
  int i, num_l, num, key;

  /* initialize */
  if (command_line == NULL) {
    command_line = g_string_sized_new(MIN_COMMAND_SIZE);
  }

  /* set command line */
  if (keyval == GDK_BackSpace || keyval == GDK_Delete) {
    if (strlen(command_line->str) > 0)
      g_string_erase(command_line, strlen(command_line->str)-1, 1);
  } else if (strlen(command_line->str) < MAX_COMMAND_LINE) {
    g_string_append_c(command_line, (gchar) keyval);
  }

  /* treat command line */
  last_com_str = NULL;
  for (i = 0; i < strlen(command_line->str); ) {
    num_l = strspn(command_line->str, "0123456789");
    if (num_l > 0 && command_line->str[i] == '0') {
      /* ignore first '0' character */
      i++;
      continue;
    }
    num = (num_l == 0)? 1: atoi(command_line->str+i);
    if (command_line->str[i+num_l] == '\0') {
      break; /* exit loop */
    }
    if (last_com_str != NULL) g_free(last_com_str);
    last_com_str = g_strndup(command_line->str+i, num_l+1);
    key = last_com_str[strlen(last_com_str)-1];
    switch (key) {
    case 'j':
    case '+':
      command_put(COM_MODE_RELATIVE_STEP, num, interval);
      break;
    case 'k':
    case '-':
      command_put(COM_MODE_RELATIVE_STEP, num, -interval);
      break;
    case 'f':
    case ' ':
      command_put(COM_MODE_RELATIVE_STEP, 25*num, interval);
      break;
    case 'b':
      command_put(COM_MODE_RELATIVE_STEP, 25*num, -interval);
      break;
    case 'e':
      if (num_l > 0)
        {i += num_l+1; continue;}
      command_put(COM_MODE_AUTO_STEP, 1, interval);
      break;
    case 's':
      if (num_l > 0)
        {i += num_l+1; continue;}
      command_put(COM_MODE_AUTO_STEP, 1, -interval);
      break;
    case 'g':
      if (num_l <= 0) {
        command_put(COM_MODE_GO_TO_STEP, 0, -1);
      } else {
        command_put(COM_MODE_GO_TO_STEP, num-1, interval);
      }
      break;
    case 'G':
      if (num_l <= 0) {
        command_put(COM_MODE_GO_TO_STEP, -1 /* means maxstep */, -1);
      } else {
        command_put(COM_MODE_GO_TO_STEP, num-1, -1);
      }
      break;
    case 't':
      command_put(COM_MODE_ONE_ROUND, num, 2);
      break;
    case 'a':
      command_put(COM_MODE_ROTATE, 1 /* y */, d_rotate * ROTATE_SMALL, num);
      break;
    case 'd':
      command_put(COM_MODE_ROTATE, 1 /* y */, -d_rotate * ROTATE_SMALL, num);
      break;
    case 'x':
      command_put(COM_MODE_ROTATE, 0 /* x */, -d_rotate * ROTATE_SMALL, num);
      break;
    case 'w':
      command_put(COM_MODE_ROTATE, 0 /* x */, d_rotate * ROTATE_SMALL, num);
      break;
    case 'A':
      command_put(COM_MODE_ROTATE, 1 /* y */, d_rotate * ROTATE_LARGE, num);
      break;
    case 'D':
      command_put(COM_MODE_ROTATE, 1 /* y */, -d_rotate * ROTATE_LARGE, num);
      break;
    case 'X':
      command_put(COM_MODE_ROTATE, 0 /* x */, -d_rotate * ROTATE_LARGE, num);
      break;
    case 'W':
      command_put(COM_MODE_ROTATE, 0 /* x */, d_rotate * ROTATE_LARGE, num);
      break;
    case 'z':
      if (num_l > 0) {i += num_l+1; continue;}
      command_put(COM_MODE_ZOOM, 1, exp(-0.1));
      break;
    case 'Z':
      if (num_l > 0) {i += num_l+1; continue;}
      command_put(COM_MODE_ZOOM, 1, exp(0.1));
      break;
    case '%':
      if (num_l <= 0) {i++; continue;}
      if (num >= ZOOM_MIN*100.0 && num <= ZOOM_MAX*100.0) {
        command_put(COM_MODE_ZOOM, 0, (double) num);
      } else {
        i += num_l+1;
        continue;
      }
      break;
    case 'p':
      if (num_l <= 0)
        command_put(COM_MODE_MARK, 0, -1);
      else if (num <= TotalAtoms() && num > 0)
        command_put(COM_MODE_MARK, 0, num-1);
      else
        command_put(COM_MODE_MARK, 0, -1);
      break;
    case 'r':
      command_put(COM_MODE_DRAG, (drag_mode == DRAG_TRANSLATION)? 1: 0);
      break;
    case 'v':
      if (num_l <= 0)
        command_put(COM_MODE_VIEW_PARTICLE, SET_ORIGIN);
      else if (num <= TotalAtoms() && num > 0)
        command_put(COM_MODE_VIEW_PARTICLE, num-1);
      else
        command_put(COM_MODE_VIEW_PARTICLE, SET_ORIGIN);
      break;
    case 'V':
      command_put(COM_MODE_VIEW_PARTICLE, SET_MARKED);
      break;
    case 'o':
      if (num_l <= 0)
        command_put(COM_MODE_ORIGIN, SET_ORIGIN);
      else if (num <= TotalAtoms() && num > 0)
        command_put(COM_MODE_ORIGIN, num-1);
      else
        command_put(COM_MODE_ORIGIN, SET_ORIGIN);
      break;
    case GDK_T:
      command_put(COM_MODE_TREMBLE);
      break;
    default:
      /* extra character, return failure */
      g_string_erase(command_line, 0, strlen(command_line->str));
      goto last;
    }
    i += num_l+1;
  }
  g_string_erase(command_line, 0, i);
last:
  gtk_entry_set_text(GTK_ENTRY(input), command_line->str);
  if (last_com_str != NULL) {
      gtk_label_set_text(GTK_LABEL(input_text), last_com_str);
      g_free(last_com_str);
  }
  return;
}

void gui_mark_load(GtkWidget *clist, gboolean select_marked, gboolean only_marked) {
	static GdkPixmap *pixmap = NULL;
	static GdkBitmap *bitmap = NULL;
	GtkStyle *style;
	long i, j, natom;
	gchar *atomstring, *item[3];
	if (pixmap == NULL && bitmap == NULL) {
		style = gtk_widget_get_style(window_main);
		pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, 
				gdk_colormap_get_system(), &bitmap,
				&style->bg[GTK_STATE_NORMAL], (gchar **)marked_xpm);
	}
	natom = TotalAtoms();
	gtk_clist_freeze(GTK_CLIST(clist));
	gtk_clist_clear(GTK_CLIST(clist));
	for (i = 0, j = 0; i < natom; i++) {
		if ((only_marked && MarkRead(i)) || !only_marked) {
			/* set browser items */
			atomstring = g_strdup_printf("%ld", i+1);
			item[0] = "";
			item[1] = (char *) GetAtomName(i);
			item[2] = atomstring;
			gtk_clist_append(GTK_CLIST(clist), item);
			g_free(atomstring);
			/* load status */
			if (MarkRead(i))
				gtk_clist_set_pixmap(GTK_CLIST(clist), j, 0, pixmap, bitmap);
			if (MarkRead(i) && select_marked)
				gtk_clist_select_row(GTK_CLIST(clist), j, 1);
			j++;
		}
	}
	gtk_clist_thaw(GTK_CLIST(clist));
	return;
}

/* ---- Callback functions ------------------------------------------------- */

static gint cb_step_g(GtkButton *button, gpointer data) {
  if (gtkraw_button_1()) {
    command_put(COM_MODE_AUTO_STEP, 1, -interval);
  } else if (gtkraw_button_2()) {
    command_put(COM_MODE_GO_TO_STEP, 0, -1);
  }
  return FALSE;
}

static gint cb_step_b(GtkButton *button, gpointer data) {
  if (gtkraw_button_1()) {
    command_put(COM_MODE_RELATIVE_STEP, 25, -interval);
  } else if (gtkraw_button_2()) {
    command_put(COM_MODE_RELATIVE_STEP, 1, -25*interval);
  }
  return FALSE;
}

static gint cb_step_k(GtkButton *button, gpointer data) {
  if (gtkraw_button_1() || gtkraw_button_2()) {
    command_put(COM_MODE_RELATIVE_STEP, 1, -interval);
  }
  return FALSE;
}

static void cb_step_changed(GtkEditable *editable, gpointer data) {
	GTK_ENTRY_FIT(GTK_ENTRY(editable), gtk_entry_get_text(GTK_ENTRY(editable)));
	return;
}

static void cb_step_focus_out(GtkWidget *widget, GdkEventFocus *event,
    gpointer user_data) {
  gchar *string;
  int a;

  string = g_strdup_printf("%d",
    atoi(gtk_entry_get_text(GTK_ENTRY(entry_step))));
  a = (string[0] != '\0')? atoi(string)-1: -1;
  g_free(string);
  if (a < 0) {
    command_put(COM_MODE_RELATIVE_STEP, 1, 0);
  } else {
    command_put(COM_MODE_GO_TO_STEP, a, interval);
  }
  AddKeyEvent();
  gtk_widget_set_sensitive(widget, TRUE);
  return;
}

static gint cb_step_j(GtkButton *button, gpointer data) {
  if (gtkraw_button_1() || gtkraw_button_2()) {
    command_put(COM_MODE_RELATIVE_STEP, 1, interval);
  }
  return FALSE;
}

static gint cb_step_f(GtkButton *button, gpointer data) {
  if (gtkraw_button_1()) {
    command_put(COM_MODE_RELATIVE_STEP, 25, interval);
  } else if (gtkraw_button_2()) {
    command_put(COM_MODE_RELATIVE_STEP, 1, 25*interval);
  }
  return FALSE;
}

static gint cb_step_G(GtkButton *button, gpointer data) {
  if (gtkraw_button_1()) {
    command_put(COM_MODE_AUTO_STEP, 1, interval);
  } else if (gtkraw_button_2()) {
    command_put(COM_MODE_GO_TO_STEP, -1 /* means maxstep */, -1);
  }
  return FALSE;
}

static gint cb_stop(GtkButton *button, gpointer data) {
	StopButton();
	return FALSE;
}

static gint cb_rotate_1(GtkButton *button, gpointer data) {
  if (gtkraw_button_1()) {
    command_put(COM_MODE_ROTATE, 2 /* z */, -d_rotate * ROTATE_SMALL, 1);
  } else if (gtkraw_button_2()) {
    command_put(COM_MODE_ROTATE, 2 /* z */, -d_rotate * ROTATE_LARGE, 1);
  }
  return FALSE;
}

static gint cb_rotate_2(GtkButton *button, gpointer data) {
  if (gtkraw_button_1()) {
    command_put(COM_MODE_ROTATE, 0 /* x */, -d_rotate * ROTATE_SMALL, 1);
  } else if (gtkraw_button_2()) {
    command_put(COM_MODE_ROTATE, 0 /* x */, -d_rotate * ROTATE_LARGE, 1);
  }
  return FALSE;
}

static gint cb_rotate_3(GtkButton *button, gpointer data) {
  if (gtkraw_button_1()) {
    command_put(COM_MODE_ROTATE, 2 /* z */, d_rotate * ROTATE_SMALL, 1);
  } else if (gtkraw_button_2()) {
    command_put(COM_MODE_ROTATE, 2 /* z */, d_rotate * ROTATE_LARGE, 1);
  }
  return FALSE;
}

static gint cb_rotate_4(GtkButton *button, gpointer data) {
  if (gtkraw_button_1()) {
    command_put(COM_MODE_ROTATE, 1 /* y */, d_rotate * ROTATE_SMALL, 1);
  } else if (gtkraw_button_2()) {
    command_put(COM_MODE_ROTATE, 1 /* y */, d_rotate * ROTATE_LARGE, 1);
  }
  return FALSE;
}

static gint cb_rotate_6(GtkButton *button, gpointer data) {
  if (gtkraw_button_1()) {
    command_put(COM_MODE_ROTATE, 1 /* y */, -d_rotate * ROTATE_SMALL, 1);
  } else if (gtkraw_button_2()) {
    command_put(COM_MODE_ROTATE, 1 /* y */, -d_rotate * ROTATE_LARGE, 1);
  }
  return FALSE;
}

static gint cb_rotate_8(GtkButton *button, gpointer data) {
  if (gtkraw_button_1()) {
    command_put(COM_MODE_ROTATE, 0 /* x */, d_rotate * ROTATE_SMALL, 1);
  } else if (gtkraw_button_2()) {
    command_put(COM_MODE_ROTATE, 0 /* x */, d_rotate * ROTATE_LARGE, 1);
  }
  return FALSE;
}

static gint cb_rotate_minus(GtkButton *button, gpointer data) {
  if (gtkraw_button_1() || gtkraw_button_2()) {
    command_put(COM_MODE_ROTATE_STEP, 1);
  }
  return FALSE;
}

static gint cb_rotate_plus(GtkButton *button, gpointer data) {
  if (gtkraw_button_1() || gtkraw_button_2()) {
    command_put(COM_MODE_ROTATE_STEP, 2);
  }
  return FALSE;
}

static gint cb_rotate(GtkButton *button, gpointer data) {
  if (gtkraw_button_1()) {
    command_put(COM_MODE_ONE_ROUND, 1, 2);
  } else if (gtkraw_button_2()) {
    command_put(COM_MODE_ONE_ROUND, 1, 4);
  }
  return FALSE;
}

static gint cb_help(GtkButton *button, gpointer data) {
  ToggleTooltips();
  return FALSE;
}

static void cb_euler_focus_out(GtkWidget *widget, GdkEventFocus *event,
    gpointer user_data) {
  AddKeyEvent();
  gtk_widget_set_sensitive(widget, TRUE);
  if (!screen_busy) {
    command_put(COM_MODE_EULER_ANGLE,
      atof(gtk_entry_get_text(GTK_ENTRY(euler_theta))),
      atof(gtk_entry_get_text(GTK_ENTRY(euler_psi))),
      atof(gtk_entry_get_text(GTK_ENTRY(euler_phi))));
  }
  return;
}

static gint cb_interval(GtkMenuItem *menuitem, gpointer data) {
  interval = GPOINTER_TO_INT(data);
  return FALSE;
}

static gint cb_frame(GtkMenuItem *menuitem, gpointer data) {
  frames = GPOINTER_TO_INT(data);
  return FALSE;
}

static gint cb_drag_mode(GtkButton *button, gpointer data) {
  command_put(COM_MODE_DRAG, GPOINTER_TO_INT(data));
  return FALSE;
}

static gint cb_outline(GtkButton *button, gpointer data) {
  command_put(COM_MODE_OUTLINE);
  return FALSE;
}

static gint cb_convert(GtkButton *button, gpointer data) {
  command_put(COM_MODE_CONVERT);
  return FALSE;
}

gint delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  return FALSE;
}

void cb_entry_activate(GtkEditable *editable, gpointer user_data) {
  gtk_widget_set_sensitive(GTK_WIDGET(editable), FALSE);
  return;
}

void cb_entry_focus_in(GtkWidget *widget, GdkEventFocus *event, gpointer user_data) {
  RemoveKeyEvent();
  return; 
}

static void ToggleTooltips(void) {
	static gboolean status=TRUE;
	GtkTooltips *tooltips;
	gint i;
	for (i = 0; i < g_slist_length(list_tooltips); i++) {
		tooltips = (GtkTooltips *)g_slist_nth_data(list_tooltips, i);
		if (status == TRUE) gtk_tooltips_disable(tooltips);
		else                gtk_tooltips_enable(tooltips);
	}
	if (status == TRUE)  status = FALSE;
	else                 status = TRUE;
	return;
}

void ChangeDeltaRotate(double d) {
	/* set d_rotate */
	if (d < 1.0 && d_rotate >= 0.009) {
		d_rotate *= 0.1;
	}else if (d > 1.0 && d_rotate <= 0.9) {
		d_rotate *= 10.0;
	}
	/* set rotate icons */
	if (d_rotate < 0.009) {
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[0]), gpixmap[0][3], gmask[0][3]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[1]), gpixmap[1][3], gmask[1][3]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[2]), gpixmap[2][3], gmask[2][3]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[3]), gpixmap[3][3], gmask[3][3]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[4]), gpixmap[4][3], gmask[4][3]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[5]), gpixmap[5][3], gmask[5][3]);
	} else if (d_rotate < 0.09) {
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[0]), gpixmap[0][2], gmask[0][2]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[1]), gpixmap[1][2], gmask[1][2]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[2]), gpixmap[2][2], gmask[2][2]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[3]), gpixmap[3][2], gmask[3][2]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[4]), gpixmap[4][2], gmask[4][2]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[5]), gpixmap[5][2], gmask[5][2]);
	} else if (d_rotate < 0.9) {
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[0]), gpixmap[0][1], gmask[0][1]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[1]), gpixmap[1][1], gmask[1][1]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[2]), gpixmap[2][1], gmask[2][1]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[3]), gpixmap[3][1], gmask[3][1]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[4]), gpixmap[4][1], gmask[4][1]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[5]), gpixmap[5][1], gmask[5][1]);
	} else {
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[0]), gpixmap[0][0], gmask[0][0]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[1]), gpixmap[1][0], gmask[1][0]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[2]), gpixmap[2][0], gmask[2][0]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[3]), gpixmap[3][0], gmask[3][0]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[4]), gpixmap[4][0], gmask[4][0]);
		gtk_pixmap_set(GTK_PIXMAP(gpixmapwid[5]), gpixmap[5][0], gmask[5][0]);
	}
	return;
}

extern void GetScreenSize(int *pw, int *ph) {
	*pw = draw->allocation.width;
	*ph = draw->allocation.height;
	return;
}


/*=====================================================================*/
int mdv_mapcolor(MDV_Color c, int red, int green, int blue) {
	/*
	 * mdv_mapcolor and fl_mapcolor is the same purpose.
	 */
	static GPtrArray *mdv_color_array = NULL;
	GdkColor *p;
	if (mdv_color_array == NULL)
		mdv_color_array = g_ptr_array_new();
	p = (GdkColor *)g_malloc(sizeof(GdkColor));
	g_ptr_array_add(mdv_color_array, p);
	p->red = red*256;
	p->green = green*256;
	p->blue = blue*256;
	gdk_colormap_alloc_color(gdk_colormap_get_system(), p, FALSE, TRUE);
	mdv_color = (GdkColor **) (mdv_color_array->pdata);
	return c;
}

/* コメント文字列の設定 */
void set_comment_string(gchar *str) {
  GetTimeval(&comment_string_sec, &comment_string_usec);
  if (comment_string != NULL)
    g_free(comment_string);
  if (str != NULL)
    comment_string = g_strdup(str);
}

