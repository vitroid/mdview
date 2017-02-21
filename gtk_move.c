#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include "mdview.h"

static void _move_apply(GtkWidget *clist);
static void pop_view_current(void);
static void push_view_current(void);
static void pop_origin_current(void);
static void push_origin_current(void);
static void unselect_all_list_item(GtkWidget *clist);
static gint cb_move_origin(GtkButton *button, gpointer clist);
static long view_current[2] = {-1, -1};
static int _view_count = 0;
static long origin_current[2] = {-1, -1};
static int _origin_count = 0;
static guint origin_mode = 0;

gint cb_move_open(GtkButton *button, gpointer data) {
	GtkWidget *window, *label, *all, *marked, *apply, *clear, *revert, *done;
	GtkWidget *vbox1, *hbox, *vbox2, *scroll, *clist_move, *origin, *hseparator;
	GSList *group;
	gint i;
	gchar *titles[3] = {"", "Atom", "Num."};
	/*
	 * window on toplevel
	 */
	//window = gtk_window_new(GTK_WINDOW_DIALOG);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "mdview: move");
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
	label = gtk_label_new("Where do you want to move?");
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
	clist_move = gtk_clist_new_with_titles(3, titles);
	gtk_widget_set_usize(clist_move, 100, 350);
	gtk_clist_set_column_width(GTK_CLIST(clist_move), 0, 7);
	gtk_clist_set_shadow_type(GTK_CLIST(clist_move), GTK_SHADOW_OUT);
	GTK_WIDGET_UNSET_FLAGS(clist_move, GTK_CAN_FOCUS);
	gtk_clist_set_column_justification(GTK_CLIST(clist_move), 0, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(clist_move), 1, GTK_JUSTIFY_CENTER);
	gtk_clist_set_column_justification(GTK_CLIST(clist_move), 2, GTK_JUSTIFY_RIGHT);
	gtk_container_add(GTK_CONTAINER(scroll), clist_move);
	gtk_widget_show(clist_move);
	/*
	 * vbox2 on hbox
	 */
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 5);
	gtk_widget_show(vbox2);
	/*
	 * all and marked on vbox2
	 */
	all = gtk_radio_button_new_with_label(NULL, "All");
	GTK_WIDGET_UNSET_FLAGS(all, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2), all, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(all), "pressed",
			GTK_SIGNAL_FUNC(cb_move_all), clist_move);
	gtk_widget_show(all);
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(all));
	marked = gtk_radio_button_new_with_label(group, "Marked");
	GTK_WIDGET_UNSET_FLAGS(marked, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2), marked, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(marked), "pressed",
			GTK_SIGNAL_FUNC(cb_move_marked), clist_move);
	gtk_widget_show(marked);
	/*
	 * hseparator on vbox2
	 */
	hseparator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox2), hseparator, FALSE, FALSE, 0);
	gtk_widget_show(hseparator);
	/*
	 * origin on vbox2
	 */
	origin = gtk_check_button_new_with_label("Origin");
	if (origin_mode) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(origin), TRUE);
		origin_mode = ~0;
	}
	GTK_WIDGET_UNSET_FLAGS(origin, GTK_CAN_FOCUS);
	gtk_signal_connect(GTK_OBJECT(origin), "pressed",
			GTK_SIGNAL_FUNC(cb_move_origin), (gpointer)clist_move);
	gtk_box_pack_start(GTK_BOX(vbox2), origin, FALSE, FALSE, 5);
	gtk_widget_show(origin);
	/*
	 * apply on vbox2
	 */
	apply = gtk_button_new_with_label("Apply");
	GTK_WIDGET_UNSET_FLAGS(apply, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2), apply, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(apply), "clicked",
			GTK_SIGNAL_FUNC(cb_move_apply), clist_move);
	gtk_widget_show(apply);
	/*
	 * clear on vbox2
	 */
	clear = gtk_button_new_with_label("Clear");
	GTK_WIDGET_UNSET_FLAGS(clear, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2), clear, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(clear), "clicked",
			GTK_SIGNAL_FUNC(cb_move_clear), clist_move);
	gtk_widget_show(clear);
	/*
	 * revert on vbox2
	 */
	revert = gtk_button_new_with_label("Revert");
	GTK_WIDGET_UNSET_FLAGS(revert, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox2), revert, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(revert), "clicked",
			GTK_SIGNAL_FUNC(cb_move_revert), clist_move);
	gtk_widget_show(revert);
	/*
	 * done on vbox2
	 */
	done = gtk_button_new_with_label("Done");
	GTK_WIDGET_SET_FLAGS(done, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(done);
	gtk_box_pack_start(GTK_BOX(vbox2), done, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(done), "clicked",
			GTK_SIGNAL_FUNC(cb_move_close), clist_move);
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
	MarkLoad();
	gui_mark_load(clist_move, FALSE, FALSE);
	push_view_current();
	push_origin_current();
	i = origin_mode ? get_origin_current() : get_view_current();
	if (i >= 0) gtk_clist_select_row(GTK_CLIST(clist_move), i, 1);
	return FALSE;
}

gint cb_move_all(GtkButton *button, gpointer clist) {
	gint i;
	gui_mark_load(GTK_WIDGET(clist), FALSE, FALSE);
	i = origin_mode ? get_origin_current() : get_view_current();
	if (i >= 0) gtk_clist_select_row(GTK_CLIST(clist), i, 1);
	UpdateViewData();
	DrawObject();
	return FALSE;
}

gint cb_move_marked(GtkButton *button, gpointer clist) {
	gint natom, i, j;
	gui_mark_load(GTK_WIDGET(clist), FALSE, TRUE);
	natom = TotalAtoms();
	for (i = 0, j = 0; i < natom; i++) {
		/* set browser items */
		if (MarkRead(i)) {
			/* load status */
			if (i == origin_mode ? get_origin_current() : get_view_current()) {
				gtk_clist_select_row(GTK_CLIST(clist), j, 1);
			}
			j++;
		}
	}
	UpdateViewData();
	DrawObject();
	return FALSE;
}

gint cb_move_apply(GtkButton *button, gpointer clist) {
	gdouble vx, vy, ox, oy, oz;
	gint i;
	_move_apply(clist);
	if (origin_mode) {
		i = get_origin_current();
		unset_center_of_mass();
		set_trans_parameter(0.0, 0.0);
		if (i >=0) {
			GetCoordinates(i, &ox, &oy, &oz);
			set_center_of_mass(ox, oy, oz);
		}
		UpdateViewData();
		DrawObject();
	} else {
		i = get_view_current();
		GetParticleCoordinate(i, &vx, &vy);
		set_trans_parameter(-vx, -vy);
		UpdateViewData();
		DrawObject();
	}
	return FALSE;
}

gint cb_move_clear(GtkButton *button, gpointer clist) {
	gdouble vx, vy;
	unselect_all_list_item(GTK_WIDGET(clist));
	if (origin_mode) {
		set_origin_current(-1);
		unset_center_of_mass();
		set_trans_parameter(0.0, 0.0);
		UpdateViewData();
		DrawObject();
	} else {
		set_view_current(-1);
		GetParticleCoordinate(-1, &vx, &vy);
		set_trans_parameter(-vx, -vy);
		UpdateViewData();
		DrawObject();
	}
	return FALSE;
}

gint cb_move_revert(GtkButton *button, gpointer clist) {
	gdouble vx, vy, ox, oy, oz;
	gint i;
	unselect_all_list_item(GTK_WIDGET(clist));
	if (origin_mode) {
		pop_origin_current();
		push_origin_current();
		i = get_origin_current();
		unset_center_of_mass();
		set_trans_parameter(0.0, 0.0);
		if (i >= 0) {
			GetCoordinates(i, &ox, &oy, &oz);
			set_center_of_mass(ox, oy, oz);
		}
		UpdateViewData();
		DrawObject();
	} else {
		pop_view_current();
		push_view_current();
		i = get_view_current();
		GetParticleCoordinate(i, &vx, &vy);
		set_trans_parameter(-vx, -vy);
		UpdateViewData();
		DrawObject();
	}
	if (i >= 0) gtk_clist_select_row(GTK_CLIST(clist), i, 1);
	return FALSE;
}

gint cb_move_close(GtkButton *button, gpointer clist) {
	gdouble vx, vy, ox, oy, oz;
	gint i;
	_move_apply(clist);
	if (origin_mode) {
		i = get_origin_current();
		pop_origin_current();
		pop_view_current();
		set_origin_current(i);
		unset_center_of_mass();
		set_trans_parameter(0.0, 0.0);
		if (i >= 0) {
			GetCoordinates(i, &ox, &oy, &oz);
			set_center_of_mass(ox, oy, oz);
		}
		UpdateViewData();
		DrawObject();
	} else {
		i = get_view_current();
		pop_origin_current();
		pop_view_current();
		set_view_current(i);
		GetParticleCoordinate(i, &vx, &vy);
		set_trans_parameter(-vx, -vy);
		UpdateViewData();
		DrawObject();
	}
	return FALSE;
}

static gint cb_move_origin(GtkButton *button, gpointer clist) {
	gdouble vx, vy, ox, oy, oz;
	gint i;
	unselect_all_list_item(GTK_WIDGET(clist));
	origin_mode = ~origin_mode;
	if (origin_mode) {
		i = get_origin_current();
		unset_center_of_mass();
		set_trans_parameter(0.0, 0.0);
		if (i >= 0) {
			GetCoordinates(i, &ox, &oy, &oz);
			set_center_of_mass(ox, oy, oz);
		}
	} else {
		i = get_view_current();
		GetParticleCoordinate(i, &vx, &vy);
		set_trans_parameter(-vx, -vy);
	}
	if (i >= 0) gtk_clist_select_row(GTK_CLIST(clist), i, 1);
	UpdateViewData();
	DrawObject();
	return FALSE;
}

static void _move_apply(GtkWidget *clist) {
	GList *tmp;
	gint row, natom;
	natom = TotalAtoms();
	tmp = GTK_CLIST(clist)->selection;
	while (tmp != NULL) {
		row = GPOINTER_TO_INT(tmp->data);
		if (origin_mode)  set_origin_current(row);
		else              set_view_current(row);
		tmp = g_list_next(tmp);
	}
	return;
}

static void unselect_all_list_item(GtkWidget *clist) {
	GList *tmp;
	gint row;
	tmp = GTK_CLIST(clist)->selection;
	while (tmp != NULL) {
		row = GPOINTER_TO_INT(tmp->data);
		gtk_clist_unselect_row(GTK_CLIST(clist), row, 1);
		tmp = g_list_next(tmp);
	}
	return;
}

long get_view_current(void) {
	return view_current[_view_count];
}

void set_view_current(long value) {
	view_current[_view_count] = value;
	return;
}

static void pop_view_current() {
	if (_view_count == 1) {
		_view_count = 0;
	} else {
		g_warning("view_current stack will be broken.");
	}
	return;
}

static void push_view_current() {
	if (_view_count == 0) {
		_view_count = 1;
		view_current[1] = view_current[0];
	} else {
		g_warning("view_current stack will be broken.");
	}
	return;
}

long get_origin_current(void) {
	return origin_current[_origin_count];
}

void set_origin_current(long value) {
	origin_current[_origin_count] = value;
	return;
}

void set_origin_mode(guint flag) {
	origin_mode = flag;
	return;
}

static void pop_origin_current() {
	if (_origin_count == 1) {
		_origin_count = 0;
	} else {
		g_warning("origin_current stack will be broken.");
	}
	return;
}

static void push_origin_current() {
	if (_origin_count == 0) {
		_origin_count = 1;
		origin_current[1] = origin_current[0];
	} else {
		g_warning("origin_current stack will be broken.");
	}
	return;
}

