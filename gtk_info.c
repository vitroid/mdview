#include <gtk/gtk.h>
#include <gtk/gtktext.h>
#include <gdk/gdk.h>
#include "mdview.h"

#include "mdv_file.h" /* GetInformation() */

static GtkWidget *window_info = NULL;
static GtkWidget *text_info = NULL;

static int info_status = ~0;
void GuiSetInformation(int nstep) {
	if (info_status >= 0 && nstep != info_status) {
		gtk_text_freeze(GTK_TEXT(text_info));
		gtk_editable_delete_text(GTK_EDITABLE(text_info), 0, -1);
		gtk_text_insert(GTK_TEXT(text_info), NULL, NULL, NULL, GetInformation(nstep),-1);
		gtk_text_thaw(GTK_TEXT(text_info));
		info_status = nstep;
	}
	return;
}

gint cb_info(GtkButton *button, gpointer button_parent) {
	GtkWidget *scroll;
	if (info_status < 0) {
		info_status = ~info_status;
		/*
		 * window_info on toplevel
		 */
		window_info = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title(GTK_WINDOW(window_info), "mdview: information");
		gtk_window_set_transient_for(GTK_WINDOW(window_info), GTK_WINDOW(GetTopWidget()));
		gtk_widget_add_events(window_info, GDK_KEY_RELEASE_MASK);
		gtk_signal_connect_object (GTK_OBJECT(window_info), "delete_event",
				GTK_SIGNAL_FUNC(gtk_button_clicked), button_parent);
		gtk_signal_connect(GTK_OBJECT(window_info), "key_press_event",
				GTK_SIGNAL_FUNC(special_key_event), NULL);
		gtk_signal_connect(GTK_OBJECT(window_info), "key_release_event",
				GTK_SIGNAL_FUNC(special_key_event), NULL);
		gtk_container_border_width(GTK_CONTAINER(window_info), 0);
		/*
		 * scroll on vbox_info
		 */
		scroll = gtk_scrolled_window_new(NULL, NULL);
		gtk_widget_set_usize(scroll, 200, 80);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(window_info), scroll);
		gtk_widget_show(scroll);
		/*
		 * text_info on scroll
		 */
		text_info = gtk_text_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(scroll), text_info);
		gtk_widget_show(text_info);
		gtk_widget_show(window_info);
	} else {
		gtk_widget_destroy(window_info);
		info_status = ~info_status;
	}
	return FALSE;
}

