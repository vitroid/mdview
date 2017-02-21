#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mdview.h"

#define DISTANCE_MIN (1.0)
#define DISTANCE_MAX (10.0)

/* external valiables */
double distance = 10.0;

static double distance_min = DISTANCE_MIN;
static double distance_max = DISTANCE_MAX;

static void distance_chang_event(GtkAdjustment *adj);
static gint distance_open_event(GtkButton *button, gpointer data);
static void cb_distance_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer user_data);
static gint cb_apply(GtkButton *button, gpointer data);
static GtkWidget *entry_distance = NULL;
static GtkWidget *entry_min = NULL;
static GtkWidget *entry_max = NULL;
static GtkObject *adj_distance = NULL;

void GuiSetDistance(double distance) {
	gchar *string;
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adj_distance), distance);
	string = g_strdup_printf("%.2f", GTK_ADJUSTMENT(adj_distance)->value);
	gtk_entry_set_text(GTK_ENTRY(entry_distance), string);
	g_free(string);
	return;
}

GtkWidget *distance_gtk_button(void) {
	GtkWidget *button;
	button = gtk_button_new_with_label("radius");
	gtk_widget_set_usize(button, 50, 25);
	GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			GTK_SIGNAL_FUNC(distance_open_event), NULL);
	return button;
}

GtkWidget *distance_gtk_entry(void) {
	entry_distance = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry_distance), "10.0");
	gtk_widget_set_usize(entry_distance, 50, 25);
	gtk_signal_connect(GTK_OBJECT(entry_distance), "focus-in-event",
			GTK_SIGNAL_FUNC(cb_entry_focus_in), NULL);
	gtk_signal_connect(GTK_OBJECT(entry_distance), "focus-out-event",
			GTK_SIGNAL_FUNC(cb_distance_focus_out), NULL);
	gtk_signal_connect(GTK_OBJECT(entry_distance), "activate",
			GTK_SIGNAL_FUNC(cb_entry_activate), NULL);
	return entry_distance;
}

GtkWidget *distance_gtk_hscale(void) {
	GtkWidget *scale;
	adj_distance = gtk_adjustment_new (10.0, 1.0, 10.0, 0.01, 0.01, 0.0);
	scale = gtk_hscale_new(GTK_ADJUSTMENT(adj_distance));
	GTK_WIDGET_UNSET_FLAGS(scale, GTK_CAN_FOCUS);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_widget_set_usize(scale, 185, 20);
	gtk_signal_connect(GTK_OBJECT(adj_distance), "value_changed",
			GTK_SIGNAL_FUNC(distance_chang_event), NULL);
	return scale;
}

/*
 * Callback functions for dialog_distance
 */
static gint distance_open_event(GtkButton *button, gpointer data) {
	GtkWidget *dialog_distance, *table, *vbox, *hseparator, *hbox;
	GtkWidget *label_message, *label_min, *label_max;
	GtkWidget *button_distance_done, *button_distance_apply, *button_distance_cancel;
	gchar *string;
	/*
	 * dialog_distance on toplevel
	 */
	dialog_distance = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(dialog_distance), "mdview: distance option");
	gtk_window_set_transient_for(GTK_WINDOW(dialog_distance), GTK_WINDOW(GetTopWidget()));
	gtk_window_set_position(GTK_WINDOW(dialog_distance), GTK_WIN_POS_MOUSE);
	gtk_widget_add_events(dialog_distance, GDK_KEY_RELEASE_MASK);
	gtk_signal_connect(GTK_OBJECT(dialog_distance), "delete_event",
			GTK_SIGNAL_FUNC(delete_event), NULL);
	gtk_signal_connect(GTK_OBJECT(dialog_distance), "key_press_event",
			GTK_SIGNAL_FUNC(special_key_event), NULL);
	gtk_signal_connect(GTK_OBJECT(dialog_distance), "key_release_event",
			GTK_SIGNAL_FUNC(special_key_event), NULL);
	gtk_container_border_width(GTK_CONTAINER(dialog_distance), 5);
	/*
	 * vbox on dialog_distance
	 */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(dialog_distance), vbox);
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
	label_message = gtk_label_new("Set minimum and maximum for distance.");
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
	string = g_strdup_printf("%.2f", distance_min);
	gtk_entry_set_text(GTK_ENTRY(entry_min), string);
	g_free(string);
	gtk_table_attach(GTK_TABLE(table), entry_min, 1, 2, 1, 2,
			GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show(entry_min);
	/*
	 * label_max on dialog_distance
	 */
	label_max = gtk_label_new("Max :");
	gtk_table_attach(GTK_TABLE(table), label_max, 0, 1, 2, 3,
			GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show(label_max);
	/*
	 * entry_max on dialog_distance
	 */
	entry_max = gtk_entry_new();
	gtk_widget_set_usize(entry_max, 30, 25);
	string = g_strdup_printf("%.2f", distance_max);
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
	 * button_distance_done on dialog_distance
	 */
	button_distance_done = gtk_button_new_with_label("Done");
	gtk_box_pack_start(GTK_BOX(hbox), button_distance_done, FALSE, FALSE, 10);
	gtk_signal_connect(GTK_OBJECT(button_distance_done), "clicked",
			GTK_SIGNAL_FUNC(cb_apply), NULL);
	gtk_signal_connect_object(GTK_OBJECT(button_distance_done), "clicked",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)dialog_distance);
	gtk_widget_show(button_distance_done);
	/*
	 * button_distance_apply on dialog_distance
	 */
	button_distance_apply = gtk_button_new_with_label("Apply");
	gtk_box_pack_start(GTK_BOX(hbox), button_distance_apply, FALSE, FALSE, 10);
	gtk_signal_connect(GTK_OBJECT(button_distance_apply), "clicked",
			GTK_SIGNAL_FUNC(cb_apply), NULL);
	gtk_widget_show(button_distance_apply);
	/*
	 * button_distance_cancel on dialog_distance
	 */
	button_distance_cancel = gtk_button_new_with_label("Cancel");
	gtk_box_pack_start(GTK_BOX(hbox), button_distance_cancel, FALSE, FALSE, 10);
	gtk_signal_connect_object(GTK_OBJECT(button_distance_cancel), "clicked",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)dialog_distance);
	gtk_widget_show(button_distance_cancel);
	gtk_widget_show(dialog_distance);
	return FALSE;
}

static void cb_distance_focus_out(GtkWidget *widget, GdkEventFocus *event,
    gpointer user_data) {
  double value;

  value = atof(gtk_entry_get_text(GTK_ENTRY(entry_distance)));
  if (value <= distance_min)
    value = distance_min;
  if (value >= distance_max)
    value = distance_max;
  GuiSetDistance(value);
  command_put(COM_MODE_VIEW_POINT, (double) value);
  AddKeyEvent();
  gtk_widget_set_sensitive(widget, TRUE);
  return;
}

static void distance_chang_event(GtkAdjustment *adj) {
  GuiSetDistance(adj->value);
  command_put(COM_MODE_VIEW_POINT, (double) adj->value);
  return;
}

static gint cb_apply(GtkButton *button, gpointer data) {
	distance_min = atof(gtk_entry_get_text(GTK_ENTRY(entry_min)));
	distance_max = atof(gtk_entry_get_text(GTK_ENTRY(entry_max)));
	if (distance_min >= 0.001 && distance_min < distance_max) {
		/*
		 * Following two lines are durty trick.
		 */
		GTK_ADJUSTMENT(adj_distance)->lower = distance_min;
		GTK_ADJUSTMENT(adj_distance)->upper = distance_max;
	}
	return FALSE;
}
