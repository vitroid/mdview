#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mdview.h"

static void cb_zoom_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer user_data);
static void zoom_change_event(GtkAdjustment *adj);
static gint zoom_open_event(GtkButton *button, gpointer data);
static gint cb_apply(GtkButton *button, gpointer data);
static GtkWidget *entry_zoom = NULL;
static GtkWidget *entry_min = NULL;
static GtkWidget *entry_max = NULL;
static GtkObject *adj_zoom = NULL;
static double zoom_percent = 1.0;
static double zoom_min = 0.1;
static double zoom_max = 10.0;

void GuiSetZoom(double zoom) {
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adj_zoom), log(zoom));
	GuiSetZoomLabel(zoom);
	return;
}

void GuiSetZoomLabel(double zoom) {
	gchar *string;
	string = g_strdup_printf("%d%%", (int)(zoom*100.0+0.5));
	gtk_entry_set_text(GTK_ENTRY(entry_zoom), string);
	g_free(string);
	return;
}

void SetZoomPercent(double value) {
	zoom_percent = value;
	return;
}

double GetZoomPercent(void) {
	return zoom_percent;
}

double GetZoomMin(void) {
	return zoom_min;
}

double GetZoomMax(void) {
	return zoom_max;
}

GtkWidget *zoom_gtk_button(void) {
	GtkWidget *button;
	button = gtk_button_new_with_label("zoom");
	gtk_widget_set_usize(button, 50, 25);
	GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			GTK_SIGNAL_FUNC(zoom_open_event), NULL);
	return button;
}

GtkWidget *zoom_gtk_entry(void) {
	entry_zoom = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry_zoom), "100%");
	gtk_widget_set_usize(entry_zoom, 50, 25);
	gtk_signal_connect(GTK_OBJECT(entry_zoom), "focus-in-event",
			GTK_SIGNAL_FUNC(cb_entry_focus_in), NULL);
	gtk_signal_connect(GTK_OBJECT(entry_zoom), "focus-out-event",
			GTK_SIGNAL_FUNC(cb_zoom_focus_out), NULL);
	gtk_signal_connect(GTK_OBJECT(entry_zoom), "activate",
			GTK_SIGNAL_FUNC(cb_entry_activate), NULL);
	return entry_zoom;
}

GtkWidget *zoom_gtk_hscale(void) {
	GtkWidget *scale;
	adj_zoom = gtk_adjustment_new(0.0, log(zoom_min), log(zoom_max), 0.01, 0.10, 0.0);
	scale = gtk_hscale_new(GTK_ADJUSTMENT(adj_zoom));
	GTK_WIDGET_UNSET_FLAGS(scale, GTK_CAN_FOCUS);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_widget_set_usize(scale, 185, 20);
	gtk_signal_connect(GTK_OBJECT(adj_zoom), "value_changed",
			GTK_SIGNAL_FUNC(zoom_change_event), NULL);
	return scale;
}

static void zoom_change_event(GtkAdjustment *adj) {
  double zoom;

  zoom = exp(adj->value);
  GuiSetZoom(zoom);
  command_put(COM_MODE_ZOOM, 0, (double) zoom);
  return;
}

static void cb_zoom_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer user_data) {
  gdouble z;

  z = 0.01 * atof(gtk_entry_get_text(GTK_ENTRY(entry_zoom)));
  if (z <= zoom_min)
    z = zoom_min;
  if (z >= zoom_max)
    z = zoom_max;
  GuiSetZoom(z);
  command_put(COM_MODE_ZOOM, 0, (double) z);
  AddKeyEvent();
  gtk_widget_set_sensitive(widget, TRUE);
  return;
}

static gint zoom_open_event(GtkButton *button, gpointer data) {
	GtkWidget *dialog_zoom, *table, *vbox, *hseparator, *hbox;
	GtkWidget *label_message, *label_min, *label_max;
	GtkWidget *button_zoom_done, *button_zoom_apply, *button_zoom_cancel;
	gchar *string;
	/*
	 * dialog_zoom on toplevel
	 */
	dialog_zoom = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(dialog_zoom), "mdview: zoom option");
	gtk_window_set_transient_for(GTK_WINDOW(dialog_zoom), GTK_WINDOW(GetTopWidget()));
	gtk_window_set_position(GTK_WINDOW(dialog_zoom), GTK_WIN_POS_MOUSE);
	gtk_widget_add_events(dialog_zoom, GDK_KEY_RELEASE_MASK);
	gtk_signal_connect(GTK_OBJECT(dialog_zoom), "delete_event",
			GTK_SIGNAL_FUNC(delete_event), NULL);
	gtk_signal_connect(GTK_OBJECT(dialog_zoom), "key_press_event",
			GTK_SIGNAL_FUNC(special_key_event), NULL);
	gtk_signal_connect(GTK_OBJECT(dialog_zoom), "key_release_event",
			GTK_SIGNAL_FUNC(special_key_event), NULL);
	gtk_container_border_width(GTK_CONTAINER(dialog_zoom), 5);
	/*
	 * vbox on dialog_zoom
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(dialog_zoom), vbox);
	gtk_widget_show(vbox);
	/*
	 * table on vbox
	 */
	table = gtk_table_new(2, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
	gtk_widget_show(table);
	/*
	 * label_message on table
	 */
	label_message = gtk_label_new("Set minimum and maximum for zoom.");
	gtk_table_attach(GTK_TABLE(table), label_message, 0, 2, 0, 1,
			GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show(label_message);
	/*
	 * label_min on table
	 */
	label_min = gtk_label_new("Min :");
	gtk_table_attach(GTK_TABLE(table), label_min, 0, 1, 1, 2,
			GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show(label_min);
	/*
	 * entry_min on table
	 */
	entry_min = gtk_entry_new();
	gtk_widget_set_usize(entry_min, 30, 25);
	string = g_strdup_printf("%.2f", zoom_min*100.0);
	gtk_entry_set_text(GTK_ENTRY(entry_min), string);
	g_free(string);
	gtk_table_attach(GTK_TABLE(table), entry_min, 1, 2, 1, 2,
			GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show(entry_min);
	/*
	 * label_max on dialog_zoom
	 */
	label_max = gtk_label_new("Max :");
	gtk_table_attach(GTK_TABLE(table), label_max, 0, 1, 2, 3,
			GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show(label_max);
	/*
	 * entry_max on dialog_zoom
	 */
	entry_max = gtk_entry_new();
	gtk_widget_set_usize(entry_max, 30, 25);
	string = g_strdup_printf("%.2f", zoom_max*100.0);
	gtk_entry_set_text(GTK_ENTRY(entry_max), string);
	g_free(string);
	gtk_table_attach(GTK_TABLE(table), entry_max, 1, 2, 2, 3,
			GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show(entry_max);
	/*
	 * hseparator on vbox
	 */
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, FALSE, 5);
	gtk_widget_show(hseparator);
	/*
	 * hbox on vbox
	 */
	hbox = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);
	/*
	 * button_zoom_done on dialog_zoom
	 */
	button_zoom_done = gtk_button_new_with_label("Done");
	gtk_box_pack_start(GTK_BOX(hbox), button_zoom_done, FALSE, FALSE, 10);
	gtk_signal_connect(GTK_OBJECT(button_zoom_done), "clicked",
			GTK_SIGNAL_FUNC(cb_apply), NULL);
	gtk_signal_connect_object(GTK_OBJECT(button_zoom_done), "clicked",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)dialog_zoom);
	gtk_widget_show(button_zoom_done);
	/*
	 * button_zoom_apply on dialog_zoom
	 */
	button_zoom_apply = gtk_button_new_with_label("Apply");
	gtk_box_pack_start(GTK_BOX(hbox), button_zoom_apply, FALSE, FALSE, 10);
	gtk_signal_connect(GTK_OBJECT(button_zoom_apply), "clicked",
			GTK_SIGNAL_FUNC(cb_apply), NULL);
	gtk_widget_show(button_zoom_apply);
	/*
	 * button_zoom_cancel on dialog_zoom
	 */
	button_zoom_cancel = gtk_button_new_with_label("Cancel");
	gtk_box_pack_start(GTK_BOX(hbox), button_zoom_cancel, FALSE, FALSE, 10);
	gtk_signal_connect_object(GTK_OBJECT(button_zoom_cancel), "clicked",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)dialog_zoom);
	gtk_widget_show(button_zoom_cancel);
	gtk_widget_show(dialog_zoom);
	return FALSE;
}

static gint cb_apply(GtkButton *button, gpointer data) {
	zoom_min = atof(gtk_entry_get_text(GTK_ENTRY(entry_min))) * 0.01;
	zoom_max = atof(gtk_entry_get_text(GTK_ENTRY(entry_max))) * 0.01;
	if (zoom_min < zoom_max) {
		/*
		 * Following two lines are durty trick.
		 */
		GTK_ADJUSTMENT(adj_zoom)->lower = log(zoom_min);
		GTK_ADJUSTMENT(adj_zoom)->upper = log(zoom_max);
	}
	return FALSE;
}

