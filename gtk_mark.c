#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include "mdview.h"

static void gui_mark_apply(GtkWidget *clist);
static GtkWidget *clist_mark = NULL;

/*
 * Callback functions for dialog_mark
 */
gint cb_mark_open(GtkButton *button, gpointer data) {
	GtkWidget *window, *label, *single, *multi, *apply, *clear, *revert, *done;
	GtkWidget *vbox1, *hbox, *vbox2, *scroll;
	GSList *group;
	gchar *titles[3] = {"", "Atom", "Num."};
	/*
	 * window on toplevel
	 */
	//window = gtk_window_new(GTK_WINDOW_DIALOG);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "mdview: mark");
	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(GetTopWidget()));
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
	gtk_widget_add_events(window, GDK_KEY_RELEASE_MASK);
	gtk_signal_connect(GTK_OBJECT(window), "key_press_event",
			GTK_SIGNAL_FUNC(special_key_event), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "key_release_event",
			GTK_SIGNAL_FUNC(special_key_event), NULL);
	gtk_container_border_width(GTK_CONTAINER(window), 10);
	/*
	 * vbox1 on window
	 */
	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox1);
	gtk_widget_show(vbox1);
	/*
	 * label on vbox1
	 */
	label = gtk_label_new("Atoms to be marked");
	gtk_box_pack_start(GTK_BOX(vbox1), label, FALSE, FALSE, 5);
	gtk_widget_show(label);
	/*
	 * hbox on vbox1
	 */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(vbox1), hbox);
	gtk_widget_show(hbox);
	/* 
	 * scroll on hbox
	 */
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(hbox), scroll, FALSE, FALSE, 5);
	gtk_widget_show(scroll);
	/*
	 * clist on scroll
	 */
	clist_mark = gtk_clist_new_with_titles(3, titles);
	gtk_widget_set_usize(clist_mark, 100, 350);
	gtk_clist_set_column_width(GTK_CLIST(clist_mark), 0, 7);
	gtk_clist_set_shadow_type(GTK_CLIST(clist_mark), GTK_SHADOW_OUT);
	GTK_WIDGET_UNSET_FLAGS(clist_mark, GTK_CAN_FOCUS);
	gtk_clist_set_column_justification(GTK_CLIST(clist_mark), 0, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(clist_mark), 1, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(clist_mark), 2, GTK_JUSTIFY_RIGHT);
	gtk_container_add(GTK_CONTAINER(scroll), clist_mark);
	gtk_widget_show(clist_mark);
	/*
	 * vbox2 on hbox
	 */
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 5);
	gtk_widget_show(vbox2);
	/*
	 * single and multi on vbox2
	 */
	single = gtk_radio_button_new_with_label(NULL, "single");
	GTK_WIDGET_UNSET_FLAGS(single, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2), single, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(single), "pressed",
			GTK_SIGNAL_FUNC(cb_mark_single), single);
	gtk_widget_show(single);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(single));
	multi = gtk_radio_button_new_with_label(group, "multi");
	GTK_WIDGET_UNSET_FLAGS(multi, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2), multi, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(multi), "pressed",
			GTK_SIGNAL_FUNC(cb_mark_multi), multi);
	gtk_widget_show(multi);
	/*
	 * apply on vbox2
	 */
	apply = gtk_button_new_with_label("Apply");
	GTK_WIDGET_UNSET_FLAGS(apply, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2), apply, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(apply), "clicked",
			GTK_SIGNAL_FUNC(cb_mark_apply), clist_mark);
	gtk_widget_show(apply);
	/*
	 * clear on vbox2
	 */
	clear = gtk_button_new_with_label("Clear");
	GTK_WIDGET_UNSET_FLAGS(clear, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2), clear, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(clear), "clicked",
			GTK_SIGNAL_FUNC(cb_mark_clear), clist_mark);
	gtk_widget_show(clear);
	/*
	 * revert on vbox2
	 */
	revert = gtk_button_new_with_label("Revert");
	GTK_WIDGET_UNSET_FLAGS(revert, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2), revert, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(revert), "clicked",
			GTK_SIGNAL_FUNC(cb_mark_revert), clist_mark);
	gtk_widget_show(revert);
	/*
	 * done on vbox2
	 */
	done = gtk_button_new_with_label("Done");
	GTK_WIDGET_SET_FLAGS(done, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(done);
	gtk_box_pack_start(GTK_BOX(vbox2), done, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(done), "clicked",
			GTK_SIGNAL_FUNC(cb_mark_close), clist_mark);
	gtk_signal_connect_object(GTK_OBJECT(done), "clicked",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)window);
	gtk_widget_show(done);
	/*
	 * Connect object
	 */
	gtk_signal_connect_object(GTK_OBJECT(window), "delete_event",
			GTK_SIGNAL_FUNC(gtk_button_clicked), (gpointer)done);
	/*
	 * Show window
	 */
	gtk_widget_show(window);
	/*
	 * Set attributes
	 */
	MarkPush();
	MarkLoad();
	if (GetMarkMode() == MARK_SINGLE) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(single), TRUE);
		gtk_clist_set_selection_mode(GTK_CLIST(clist_mark), GTK_SELECTION_SINGLE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(multi), TRUE);
		gtk_clist_set_selection_mode(GTK_CLIST(clist_mark), GTK_SELECTION_MULTIPLE);
	}
	gui_mark_load(clist_mark, TRUE, FALSE);
	return FALSE;
}

gint cb_mark_single(GtkButton *button, gpointer data) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
	gtk_clist_set_selection_mode(GTK_CLIST(clist_mark), GTK_SELECTION_SINGLE);
	SetMarkMode(MARK_SINGLE);
	gui_mark_load(clist_mark, TRUE, FALSE);
	UpdateViewData();
	DrawObject();
	return FALSE;
}

gint cb_mark_multi(GtkButton *button, gpointer data) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), TRUE);
	gtk_clist_set_selection_mode(GTK_CLIST(clist_mark), GTK_SELECTION_MULTIPLE);
	SetMarkMode(MARK_MULTIPLE);
	gui_mark_load(clist_mark, TRUE, FALSE);
	UpdateViewData();
	DrawObject();
	return FALSE;
}

gint cb_mark_apply(GtkButton *button, gpointer data) {
	gui_mark_apply((GtkWidget *)data);
	UpdateViewData();
	DrawObject();
	return FALSE;
}

gint cb_mark_clear(GtkButton *button, gpointer data) {
	glong i, natom;
	MarkClear();
	natom = TotalAtoms();
	gtk_clist_freeze(GTK_CLIST(data));
	for (i = 0; i < natom; i++) {
		gtk_clist_unselect_row(GTK_CLIST(data), i, 1);
	}
	gtk_clist_thaw(GTK_CLIST(data));
	UpdateViewData();
	DrawObject();
	return FALSE;
}

gint cb_mark_revert(GtkButton *button, gpointer data) {
	MarkLoad();
	gui_mark_load((GtkWidget *)data, TRUE, FALSE);
	UpdateViewData();
	DrawObject();
	return FALSE;
}

gint cb_mark_close(GtkButton *button, gpointer data) {
	gui_mark_apply((GtkWidget *)data);
	MarkSave();
	MarkPop();
	UpdateViewData();
	DrawObject();
	return FALSE;
}

static void gui_mark_apply(GtkWidget *clist) {
	GList *tmp;
	gint row, natom;
	natom = TotalAtoms();
	for (row = 0; row < natom; row++) {
		MarkWrite(0, row);
	}
	tmp = GTK_CLIST(clist)->selection;
	while (tmp != NULL) {
		row = GPOINTER_TO_INT(tmp->data);
		MarkWrite(1, row);
		tmp = g_list_next(tmp);
	}
	return;
}

