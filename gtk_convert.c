#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdktypes.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include "mdview.h"

static gint cb_fdialog(GtkButton *button, gpointer data);
static gint cb_alert(GtkButton *button, gpointer data);
static char *filename = NULL;
char *FileSelection(char *title, char *pattern) {
	GtkWidget *fdialog;
	filename = NULL;
	fdialog = gtk_file_selection_new(title);
	gtk_window_set_position(GTK_WINDOW(fdialog), GTK_WIN_POS_MOUSE);
	gtk_object_set_data(GTK_OBJECT(fdialog), "pattern", pattern);
	gtk_file_selection_complete(GTK_FILE_SELECTION(fdialog), pattern);
	gtk_widget_add_events(fdialog, GDK_KEY_RELEASE_MASK);
	gtk_signal_connect(GTK_OBJECT(fdialog), "destroy",
			GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fdialog)->ok_button),
			"clicked", GTK_SIGNAL_FUNC(cb_fdialog), fdialog);
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fdialog)->ok_button), "clicked",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)fdialog);
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fdialog)->cancel_button), "clicked",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)fdialog);
	gtk_signal_connect(GTK_OBJECT(fdialog), "key_press_event",
			GTK_SIGNAL_FUNC(special_key_event), NULL);
	gtk_signal_connect(GTK_OBJECT(fdialog), "key_release_event",
			GTK_SIGNAL_FUNC(special_key_event), NULL);
	gtk_widget_show(fdialog);
	gtk_grab_add(GTK_WIDGET(fdialog));
	gtk_main();
	return filename;
}

static gint cb_fdialog(GtkButton *button, gpointer data) {
	gchar *fullpath, *fname, *pattern, *suffix;
	int suffix_len, fname_len;
	if (filename != NULL) g_free(filename);
	filename = NULL;
	fullpath = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(data)));
	pattern = (char *)gtk_object_get_data(GTK_OBJECT(data), "pattern");
	suffix = pattern + 1;
	fname = g_strdup(Path2File(fullpath));
	if (strlen(fname) > 0 && strcmp(fname, pattern) != 0) {
		fname_len = strlen(fname);
		suffix_len = strlen(suffix);
		if (fname_len >= suffix_len && strcmp(fname+fname_len-suffix_len, suffix) != 0) {
			/* add suffix */
			filename = g_strconcat(fullpath, suffix, NULL);
		} else {
			filename = g_strdup(fullpath);
		}
	}
	g_free(fname);
	g_free(fullpath);
	return FALSE;
}

/*=====================================================================*/
static gboolean answer = FALSE;
int AlertDialog(const char *string1, const char *string2) {
	GtkWidget *dialog, *label1, *label2, *button_yes, *button_no;
	/*
	 * dialog on toplevel
	 */
	dialog = gtk_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(GetTopWidget()));
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
			GTK_SIGNAL_FUNC(delete_event), NULL);
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
	gtk_container_border_width(GTK_CONTAINER(dialog), 10);
	/*
	 * label1 on vbox
	 */
	label1 = gtk_label_new(string1);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label1, FALSE, FALSE, 10);
	gtk_widget_show(label1);
	/*
	 * label2 on vbox
	 */
	label2 = gtk_label_new(string2);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label2, FALSE, FALSE, 10);
	gtk_widget_show(label2);
	/*
	 * button_yes on action_area
	 */
	button_yes = gtk_button_new_with_label("YES");
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button_yes, FALSE, FALSE, 10);
	gtk_signal_connect(GTK_OBJECT(button_yes), "clicked",
			GTK_SIGNAL_FUNC(cb_alert), GINT_TO_POINTER(1));
	gtk_signal_connect_object(GTK_OBJECT(button_yes), "clicked",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)dialog);
	gtk_widget_show(button_yes);
	/*
	 * button_no on dialog_step
	 */
	button_no = gtk_button_new_with_label("NO");
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button_no, FALSE, FALSE, 10);
	gtk_signal_connect(GTK_OBJECT(button_no), "clicked",
			GTK_SIGNAL_FUNC(cb_alert), GINT_TO_POINTER(0));
	gtk_signal_connect_object(GTK_OBJECT(button_no), "clicked",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)dialog);
	gtk_widget_show(button_no);
	gtk_widget_show(dialog);
	gtk_grab_add(GTK_WIDGET(dialog));
	gtk_main();
	return answer;
}

static gint cb_alert(GtkButton *button, gpointer data) {
	answer = GPOINTER_TO_INT(data);
	return FALSE;
}

