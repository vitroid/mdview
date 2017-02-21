#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include "mdview.h"

typedef struct {
	guint *flags;
	guint status;
	gchar *name;
} LayerData;

#define TIMEOUT_REDRAW 10

static void SetLayerList(void);
static gint cb_layer_new(GtkButton *button, gpointer data);
static gint cb_layer_duplicate(GtkButton *button, gpointer data);
static gint cb_layer_rename(GtkButton *button, gpointer data);
static gint cb_rename_close(GtkButton *button, gpointer data);
static gint cb_layer_delete(GtkButton *button, gpointer data);
static gint cb_layer_show_hide(GtkButton *button, gpointer data);
static gint cb_invert(GtkButton *button, gpointer data);
static gint cb_done(GtkButton *button, gpointer button_parent);
static void cb_layer_select(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer user_data);
static void cb_layer_unselect(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer user_data);
static void cb_atoms_select(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer user_data);
static void cb_atoms_unselect(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer user_data);
static gint cb_redraw(gpointer data);
static void SetRedrawTimer (void);
static void CheckAtomNumber(void);
static void ParseString(char *itemstring, glong *last, guint **flags);
static void IncreaseLayerAtoms (glong n);
static GtkWidget *window_layer = NULL;
static guint layer_status = ~0;
static guint cache_status = ~0;
static guint *invisiable = NULL;
static GList *list_layer = NULL;
static glong max_atom_number = 0;
static gint selected_row = -1;
static gint layer_counter = 0;
static GtkWidget *clist_layer = NULL;
static GtkWidget *clist_atoms = NULL;
static guint redraw_id = 0;

guint InvisibleAtom(glong k) {
	LayerData *p;
	GList *tmp;
	glong i;
	if (list_layer == NULL) {
		return 0;
	} else {
		CheckAtomNumber();
		if (cache_status) {
			cache_status = 0;
			for (i = 0; i < max_atom_number; i++) {
				invisiable[i] = ~0;
			}
			tmp = list_layer;
			while (tmp != NULL) {
				p = tmp->data;
				for (i = 0; i < max_atom_number; i++) {
					if (p->status && p->flags[i]) {
						invisiable[i] = 0;
					}
				}
				tmp = g_list_next(tmp);
			}
		}
		return invisiable[k];
	}
}

gint cb_layer_open(GtkButton *button, gpointer button_parent) {
	GtkWidget *vpaned, *hbox1, *vbox1, *scroll1, *scroll2;
	GtkWidget *button_layer_new, *button_layer_duplicate;
	GtkWidget *button_layer_rename, *button_layer_delete;
	GtkWidget *button_layer_show_hide, *button_invert, *button_done;
	gchar *titles1[2] = {"", "Layer"};
	gchar *titles2[3] = {"", "No.", "Atom"};
	if (layer_status) {
		layer_status = ~layer_status;
		/*
		 * window_layer on toplevel
		 */
		//window_layer = gtk_window_new(GTK_WINDOW_DIALOG);
		window_layer = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title(GTK_WINDOW(window_layer), "mdview: layer");
		gtk_window_set_transient_for(GTK_WINDOW(window_layer), GTK_WINDOW(GetTopWidget()));
		gtk_signal_connect_object (GTK_OBJECT(window_layer), "delete_event",
				GTK_SIGNAL_FUNC(gtk_button_clicked), button_parent);
#if 0
		gtk_window_set_position(GTK_WINDOW(window_layer), GTK_WIN_POS_MOUSE);
		gtk_widget_add_events(window_layer, GDK_KEY_RELEASE_MASK);
		gtk_signal_connect(GTK_OBJECT(window_layer), "key_press_event",
				GTK_SIGNAL_FUNC(special_key_event), NULL);
		gtk_signal_connect(GTK_OBJECT(window_layer), "key_release_event",
				GTK_SIGNAL_FUNC(special_key_event), NULL);
#endif
		gtk_container_border_width(GTK_CONTAINER(window_layer), 10);
		/*
		 * hbox1 on window_layer
		 */
		hbox1 = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(window_layer), hbox1);
		gtk_widget_show(hbox1);
		/*
		 * vpaned on hbox1
		 */
		vpaned = gtk_vpaned_new();
		gtk_box_pack_start(GTK_BOX(hbox1), vpaned, FALSE, FALSE, 5);
		gtk_paned_set_gutter_size(GTK_PANED(vpaned), 10);
		gtk_widget_show(vpaned);
		/* 
		 * scroll1 on vpaned
		 */
		scroll1 = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll1),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
		gtk_paned_add1(GTK_PANED(vpaned), scroll1);
		gtk_widget_show(scroll1);
		/*
		 * clist_layer on scroll1
		 */
		clist_layer = gtk_clist_new_with_titles(2, titles1);
		gtk_widget_set_usize(clist_layer, 100, 150);
		gtk_clist_set_column_width(GTK_CLIST(clist_layer), 0, 7);
		gtk_clist_set_shadow_type(GTK_CLIST(clist_layer), GTK_SHADOW_OUT);
		GTK_WIDGET_UNSET_FLAGS(clist_layer, GTK_CAN_FOCUS);
		gtk_clist_set_column_justification(GTK_CLIST(clist_layer), 0, GTK_JUSTIFY_CENTER);
		gtk_container_add(GTK_CONTAINER(scroll1), clist_layer);
		gtk_signal_connect(GTK_OBJECT(clist_layer), "select-row",
				GTK_SIGNAL_FUNC(cb_layer_select), NULL);
		gtk_signal_connect(GTK_OBJECT(clist_layer), "unselect-row",
				GTK_SIGNAL_FUNC(cb_layer_unselect), NULL);
		gtk_widget_show(clist_layer);
		/* 
		 * scroll2 on vpaned
		 */
		scroll2 = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll2),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
		gtk_paned_add2(GTK_PANED(vpaned), scroll2);
		gtk_widget_show(scroll2);
		/*
		 * clist_atoms on scroll2
		 */
		clist_atoms = gtk_clist_new_with_titles(3, titles2);
		gtk_widget_set_usize(clist_atoms, 100, 350);
		gtk_clist_set_column_width(GTK_CLIST(clist_atoms), 0, 7);
		gtk_clist_set_shadow_type(GTK_CLIST(clist_atoms), GTK_SHADOW_OUT);
		GTK_WIDGET_UNSET_FLAGS(clist_atoms, GTK_CAN_FOCUS);
		gtk_clist_set_selection_mode(GTK_CLIST(clist_atoms), GTK_SELECTION_MULTIPLE);
		gtk_clist_set_column_justification(GTK_CLIST(clist_atoms), 0, GTK_JUSTIFY_CENTER);
		gtk_clist_set_column_justification(GTK_CLIST(clist_atoms), 1, GTK_JUSTIFY_CENTER);
		gtk_clist_set_column_justification(GTK_CLIST(clist_atoms), 2, GTK_JUSTIFY_RIGHT);
		gtk_container_add(GTK_CONTAINER(scroll2), clist_atoms);
		gtk_signal_connect(GTK_OBJECT(clist_atoms), "select-row",
				GTK_SIGNAL_FUNC(cb_atoms_select), NULL);
		gtk_signal_connect(GTK_OBJECT(clist_atoms), "unselect-row",
				GTK_SIGNAL_FUNC(cb_atoms_unselect), NULL);
		gtk_widget_show(clist_atoms);
		/*
		 * vbox1 on hbox1
		 */
		vbox1 = gtk_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hbox1), vbox1, FALSE, FALSE, 5);
		gtk_widget_show(vbox1);
		/*
		 * button_layer_new on vbox1
		 */
		button_layer_new = gtk_button_new_with_label("New");
		GTK_WIDGET_UNSET_FLAGS(button_layer_new, GTK_CAN_FOCUS);
		gtk_box_pack_start(GTK_BOX(vbox1), button_layer_new, FALSE, FALSE, 5);
		gtk_signal_connect(GTK_OBJECT(button_layer_new), "clicked",
				GTK_SIGNAL_FUNC(cb_layer_new), NULL);
		gtk_widget_show(button_layer_new);
		/*
		 * button_layer_duplicate on vbox1
		 */
		button_layer_duplicate = gtk_button_new_with_label("Duplicate");
		GTK_WIDGET_UNSET_FLAGS(button_layer_duplicate, GTK_CAN_FOCUS);
		gtk_box_pack_start(GTK_BOX(vbox1), button_layer_duplicate, FALSE, FALSE, 5);
		gtk_signal_connect(GTK_OBJECT(button_layer_duplicate), "clicked",
				GTK_SIGNAL_FUNC(cb_layer_duplicate), NULL);
		gtk_widget_show(button_layer_duplicate);
		/*
		 * button_rename on vbox1
		 */
		button_layer_rename = gtk_button_new_with_label("Rename");
		GTK_WIDGET_UNSET_FLAGS(button_layer_rename, GTK_CAN_FOCUS);
		gtk_box_pack_start(GTK_BOX(vbox1), button_layer_rename, FALSE, FALSE, 5);
		gtk_signal_connect(GTK_OBJECT(button_layer_rename), "clicked",
				GTK_SIGNAL_FUNC(cb_layer_rename), NULL);
		gtk_widget_show(button_layer_rename);
		/*
		 * button_delete on vbox1
		 */
		button_layer_delete = gtk_button_new_with_label("Delete");
		GTK_WIDGET_UNSET_FLAGS(button_layer_delete, GTK_CAN_FOCUS);
		gtk_box_pack_start(GTK_BOX(vbox1), button_layer_delete, FALSE, FALSE, 5);
		gtk_signal_connect(GTK_OBJECT(button_layer_delete), "clicked",
				GTK_SIGNAL_FUNC(cb_layer_delete), NULL);
		gtk_widget_show(button_layer_delete);
		/*
		 * button_layer_show_hide on vbox1
		 */
		button_layer_show_hide = gtk_button_new_with_label("Show/Hide");
		GTK_WIDGET_UNSET_FLAGS(button_layer_show_hide, GTK_CAN_FOCUS);
		gtk_box_pack_start(GTK_BOX(vbox1), button_layer_show_hide, FALSE, FALSE, 5);
		gtk_signal_connect(GTK_OBJECT(button_layer_show_hide), "clicked",
				GTK_SIGNAL_FUNC(cb_layer_show_hide), NULL);
		gtk_widget_show(button_layer_show_hide);
		/*
		 * button_invert on vbox1
		 */
		button_invert = gtk_button_new_with_label("Invert");
		gtk_box_pack_start(GTK_BOX(vbox1), button_invert, FALSE, FALSE, 5);
		gtk_signal_connect(GTK_OBJECT(button_invert), "clicked",
				GTK_SIGNAL_FUNC(cb_invert), NULL);
		gtk_widget_show(button_invert);
		/*
		 * button_done on vbox1
		 */
		button_done = gtk_button_new_with_label("Done");
		gtk_box_pack_end(GTK_BOX(vbox1), button_done, FALSE, FALSE, 5);
		gtk_signal_connect(GTK_OBJECT(button_done), "clicked",
				GTK_SIGNAL_FUNC(cb_done), button_parent);
		gtk_widget_show(button_done);
		/*
		 * Set initial status
		 */
		SetLayerList();
		/*
		 * Show window_layer
		 */
		gtk_widget_show(window_layer);
	} else {
		gtk_widget_destroy(window_layer);
		layer_status = ~layer_status;
	}
	return FALSE;
}


void AddNewLayer(char *name, char *itemstring) {
	LayerData *layer_data;
	guint *flags;
	glong last;
	glong i;
	ParseString(itemstring, &last, &flags);
	IncreaseLayerAtoms(last);
	cache_status = ~0;
	/* set each items */
	layer_data = g_malloc(sizeof(LayerData));
	layer_data->flags = g_malloc(sizeof(guint) * max_atom_number);
	layer_data->status = ~0;
	layer_data->name = g_strdup(name);
	for (i = 0; i < last; i++) {
		layer_data->flags[i] = flags[i]; 
	}
	g_free(flags);
	for (i = last; i < max_atom_number; i++) {
		layer_data->flags[i] = 0;
	}
	/* add data to layer */
	list_layer = g_list_append(list_layer, layer_data);
	if (!layer_status) {
		/* redraw sub window when sub window exists */
		SetLayerList();
		gtk_clist_select_row(GTK_CLIST(clist_layer), g_list_length(list_layer)-1, 1);
	}
	return;
}

extern void EraseLayer(char *name) {
	LayerData *p;
	GList *tmp, *next;
	guint refresh = 0;
	tmp = list_layer;
	while (tmp != NULL) {
		p = tmp->data;
		next = g_list_next(tmp);
		if (strcmp(p->name, name) == 0) {
			if (p->flags != NULL) {
				g_free(p->flags);
				p->flags = NULL;
			}
			if (p->name != NULL) {
				g_free(p->name);
				p->name = NULL;
			}
			list_layer = g_list_remove(list_layer, p);
			refresh = ~0;
		}
		tmp = next;
	}
	if (!layer_status && refresh) {
		SetLayerList();
		selected_row = -1;
		gtk_clist_freeze(GTK_CLIST(clist_atoms));
		gtk_clist_clear(GTK_CLIST(clist_atoms));
		gtk_clist_thaw(GTK_CLIST(clist_atoms));
	}
	return;
}

extern void SetLayerStatus(char *itemstring) {
	LayerData *p;
	gchar **names;
	glong i, j, list_length;
	names = g_strsplit(itemstring, ",", 0);
	list_length = g_list_length(list_layer);
	/* Once all layers are invisible */
	for (i = 0; i < list_length; i++) {
		p = g_list_nth_data(list_layer, i);
		p->status = 0;
	}
	/* Then, if the name is the same, it become visible */
	for (j = 0; names[j] != NULL; j++) {
		for (i = 0; i < list_length; i++) {
			p = g_list_nth_data(list_layer, i);
			if (strcmp(p->name, names[j]) == 0) p->status = ~0;
		}
	}
	g_strfreev(names);
	if (!layer_status) {
		SetLayerList();
		selected_row = -1;
		gtk_clist_freeze(GTK_CLIST(clist_atoms));
		gtk_clist_clear(GTK_CLIST(clist_atoms));
		gtk_clist_thaw(GTK_CLIST(clist_atoms));
	}
	return;
}

extern void KillLayers(void) {
	LayerData *p;
	GList *tmp, *next;
	guint refresh = 0;
	tmp = list_layer;
	while (tmp != NULL) {
		p = tmp->data;
		next = g_list_next(tmp);
		if (p->flags != NULL) {
			g_free(p->flags);
			p->flags = NULL;
		}
		if (p->name != NULL) {
			g_free(p->name);
			p->name = NULL;
		}
		list_layer = g_list_remove(list_layer, p);
		refresh = ~0;
		tmp = next;
	}
	if (!layer_status && refresh) {
		SetLayerList();
		selected_row = -1;
		gtk_clist_freeze(GTK_CLIST(clist_atoms));
		gtk_clist_clear(GTK_CLIST(clist_atoms));
		gtk_clist_thaw(GTK_CLIST(clist_atoms));
	}
	return;
}

static gint cb_layer_new(GtkButton *button, gpointer data) {
	LayerData *layer_data;
	GList *tmp;
	glong i;
	CheckAtomNumber();
	cache_status = ~0;
	layer_data = g_malloc(sizeof(LayerData));
	layer_data->flags = g_malloc(sizeof(guint) * max_atom_number);
	layer_data->status = ~0;
	layer_data->name = g_strdup_printf("Layer%d", ++layer_counter);
	/* Once set all atoms to be on. */
	for (i = 0; i < max_atom_number; i++) {
		layer_data->flags[i] = ~0;
	}
	/* Then set off if atom is already on in other layers. */
	tmp = list_layer;
	while (tmp != NULL) {
		for (i = 0; i < max_atom_number; i++) {
			if (((LayerData *)tmp->data)->flags[i]) {
				layer_data->flags[i] = 0;
			}
		}
		tmp = g_list_next(tmp);
	}
	list_layer = g_list_append(list_layer, layer_data);
	SetLayerList();
	gtk_clist_select_row(GTK_CLIST(clist_layer), g_list_length(list_layer)-1, 1);
	return FALSE;
}

static gint cb_layer_duplicate(GtkButton *button, gpointer data) {
	LayerData *origin, *dist;
	glong i;
	CheckAtomNumber();
	if (selected_row >= 0) {
		origin = g_list_nth_data(list_layer, selected_row);
		dist = g_malloc(sizeof(LayerData));
		dist->flags = g_malloc(sizeof(guint) * max_atom_number);
		dist->status = ~0;
		dist->name = g_strdup_printf("Layer%d", ++layer_counter);
		for (i = 0; i < max_atom_number; i++) {
			dist->flags[i] = origin->flags[i];
		}
		list_layer = g_list_append(list_layer, dist);
		SetLayerList();
		gtk_clist_select_row(GTK_CLIST(clist_layer), g_list_length(list_layer)-1, 1);
	}
	return FALSE;
}

static gint cb_layer_rename(GtkButton *button, gpointer data) {
	LayerData *p;
	GtkWidget *dialog, *label, *entry, *button_done, *button_cancel;
	CheckAtomNumber();
	if (selected_row >= 0) {
		p = g_list_nth_data(list_layer, selected_row);
		/*
		 * dialog on toplevel
		 */
		dialog = gtk_dialog_new();
		gtk_window_set_title(GTK_WINDOW(dialog), "mdview: layer rename");
		gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
		gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(GetTopWidget()));
		gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
		gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
				GTK_SIGNAL_FUNC(delete_event), NULL);
		/*
		 * label on dialog
		 */
		label = gtk_label_new("Enter name of layer.");
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, FALSE, 10);
		gtk_widget_show(label);
		/*
		 * entry on dialog
		 */
		entry = gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(entry), p->name);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, FALSE, 10);
		gtk_signal_connect(GTK_OBJECT(entry), "activate",
				GTK_SIGNAL_FUNC(cb_rename_close), entry);
		gtk_signal_connect_object(GTK_OBJECT(entry), "activate",
				GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)dialog);
		gtk_widget_show(entry);
		/*
		 * button_done on dialog
		 */
		button_done = gtk_button_new_with_label("Done");
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button_done, FALSE, FALSE, 10);
		gtk_signal_connect(GTK_OBJECT(button_done), "clicked",
				GTK_SIGNAL_FUNC(cb_rename_close), entry);
		gtk_signal_connect_object(GTK_OBJECT(button_done), "clicked",
				GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)dialog);
		gtk_widget_show(button_done);
		/*
		 * button_cancel on dialog
		 */
		button_cancel = gtk_button_new_with_label("Cancel");
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button_cancel, FALSE, FALSE, 10);
		gtk_signal_connect_object(GTK_OBJECT(button_cancel), "clicked",
				GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)dialog);
		gtk_widget_show(button_cancel);
		/*
		 * show
		 */
		gtk_widget_show(dialog);
	}
	return FALSE;
}

static gint cb_rename_close(GtkButton *button, gpointer data) {
	LayerData *p;
	if (selected_row >= 0) {
		p = g_list_nth_data(list_layer, selected_row);
		if (p->name != NULL)  g_free(p->name);
		p->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(data)));
		SetLayerList();
		gtk_clist_select_row(GTK_CLIST(clist_layer), selected_row, 1);
	}
	return FALSE;
}

static gint cb_layer_delete(GtkButton *button, gpointer data) {
	LayerData *p;
	CheckAtomNumber();
	if (selected_row >= 0) {
		cache_status = ~0;
		p = g_list_nth_data(list_layer, selected_row);
		if (p->flags != NULL) {
			g_free(p->flags);
			p->flags = NULL;
		}
		if (p->name != NULL) {
			g_free(p->name);
			p->name = NULL;
		}
		list_layer = g_list_remove(list_layer, p);
		SetLayerList();
		selected_row = -1;
		gtk_clist_freeze(GTK_CLIST(clist_atoms));
		gtk_clist_clear(GTK_CLIST(clist_atoms));
		gtk_clist_thaw(GTK_CLIST(clist_atoms));
		SetRedrawTimer();
	}
	return FALSE;
}

static gint cb_layer_show_hide(GtkButton *button, gpointer data) {
	LayerData *p;
	gchar *c;
	CheckAtomNumber();
	if (selected_row >= 0) {
		cache_status = ~0;
		p = g_list_nth_data(list_layer, selected_row);
		p->status = ~p->status;
		if (p->status)  c = "*";
		else            c = "";
		gtk_clist_freeze(GTK_CLIST(clist_layer));
		gtk_clist_set_text(GTK_CLIST(clist_layer), selected_row, 0, c);
		gtk_clist_thaw(GTK_CLIST(clist_layer));
		SetRedrawTimer();
	}
	return FALSE;
}

static gint cb_invert(GtkButton *button, gpointer data) {
	LayerData *p;
	glong i;
	CheckAtomNumber();
	if (selected_row >= 0) {
		cache_status = ~0;
		p = g_list_nth_data(list_layer, selected_row);
		gtk_clist_freeze(GTK_CLIST(clist_atoms));
		for (i = 0; i < max_atom_number; i++) {
			p->flags[i] = ~p->flags[i];
			if (p->flags[i])
				gtk_clist_select_row(GTK_CLIST(clist_atoms), i, 1);
			else
				gtk_clist_unselect_row(GTK_CLIST(clist_atoms), i, 1);
		}
		gtk_clist_thaw(GTK_CLIST(clist_atoms));
	}
	return FALSE;
}

static gint cb_done(GtkButton *button, gpointer button_parent) {
	/* I want to do following, but I cannot... */
	/*
	 * gtk_signal_connect(GTK_OBJECT(button_done), "clicked",
	 *		GTK_SIGNAL_FUNC(gtk_button_clicked), button_parent);
	 */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_parent), FALSE);
	return FALSE;
}

static void SetLayerList(void) {
	GList *tmp;
	gchar *item[2];
	gtk_clist_freeze(GTK_CLIST(clist_layer));
	gtk_clist_clear(GTK_CLIST(clist_layer));
	tmp = list_layer;
	while (tmp != NULL) {
		if (((LayerData*)tmp->data)->status) item[0] = "*";
		else             item[0] = "";
		item[1] = ((LayerData*)tmp->data)->name;
		gtk_clist_append(GTK_CLIST(clist_layer), item);
		tmp = g_list_next(tmp);
	}
	gtk_clist_thaw(GTK_CLIST(clist_layer));
	return;
}

static void cb_layer_select(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer user_data) {
	LayerData *p;
	glong i;
	p = g_list_nth_data(list_layer, row);
	selected_row = row;
	gui_mark_load(clist_atoms, FALSE, FALSE);
	gtk_clist_freeze(GTK_CLIST(clist_atoms));
	for (i = 0; i < max_atom_number; i++) {
		if (p->flags[i]) {
			gtk_clist_select_row(GTK_CLIST(clist_atoms), i, column);
		}
	}
	gtk_clist_thaw(GTK_CLIST(clist_atoms));
	return;
}

static void cb_layer_unselect(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer user_data) {
	selected_row = -1;
	gtk_clist_freeze(GTK_CLIST(clist_atoms));
	gtk_clist_clear(GTK_CLIST(clist_atoms));
	gtk_clist_thaw(GTK_CLIST(clist_atoms));
	return;
}

static void cb_atoms_select(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer user_data) {
	LayerData *p;
	CheckAtomNumber();
	if (selected_row >= 0) {
		cache_status = ~0;
		p = g_list_nth_data(list_layer, selected_row);
		p->flags[row] = ~0;
		SetRedrawTimer();
	}
	return;
}

static void cb_atoms_unselect(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer user_data) {
	LayerData *p;
	CheckAtomNumber();
	if (selected_row >= 0) {
		cache_status = ~0;
		p = g_list_nth_data(list_layer, selected_row);
		p->flags[row] = 0;
		SetRedrawTimer();
	}
	return;
}

static gint cb_redraw(gpointer data) {
	redraw_id = 0;
	UpdateViewData();
	DrawObject();
	return FALSE;
}

static void SetRedrawTimer (void) {
	if (redraw_id > 0) {
		gtk_timeout_remove(redraw_id);
	}
	redraw_id = gtk_timeout_add(TIMEOUT_REDRAW, cb_redraw, NULL);
	return;
}

static void IncreaseLayerAtoms (glong n) {
	LayerData *p;
	GList *tmp;
	glong  i;
	if (max_atom_number < n) {
		cache_status = ~0;
		tmp = list_layer;
		while (tmp != NULL) {
			p = tmp->data;
			p->flags = g_realloc(p->flags, sizeof(LayerData)*n);
			tmp = g_list_next(tmp);
			for (i = max_atom_number; i < n; i++) {
				p->flags[i] = 0;
			}
		}
		/* It is not neccessary to initialize additional invisiable[] */
		invisiable = g_realloc(invisiable, sizeof(guint)*n);
		max_atom_number = n;
	}
}

static void CheckAtomNumber(void) {
	IncreaseLayerAtoms(TotalAtoms());
	return;
}

static void ParseString(char *itemstring, glong *last, guint **pflags) {
	gchar **splited, **range;
	glong i, j, ntmp, ns, ne, max = 0;
	guint *flags = NULL;
	splited = g_strsplit(itemstring, ",", 0);
	for (i = 0; splited[i] != NULL; i++) {
		if (strstr(splited[i], "-") == NULL) {
			/* Only one number */
			ns = atol(splited[i]);
			ne = ns;
		} else {
			/* treatment of range */
			range = g_strsplit(splited[i], "-", 2);
			ns = atol(range[0]);
			ne = atol(range[1]);
			g_strfreev(range);
			if (ns > ne) {
				ntmp = ns;
				ns = ne;
				ne = ntmp;
			}
			if (ns <= 0) ns = 1;
			if (ne <= 0) ne = 1;
		}
		if (ns > 0 && ne > 0) {
			if (ne > max) {
				if (flags == NULL) {
					flags = g_malloc(sizeof(guint) * ne);
				} else {
					flags = g_realloc(flags, sizeof(guint) * ne);
				}
				for (j = max; j < ne; j++) {
					flags[j] = 0;
				}
				max = ne;
			}
			for (j = ns-1; j < ne; j++) {
				flags[j] = ~0;
			}
		}
	}
	g_strfreev(splited);
	*last = max;
	*pflags = flags;
	return;
}
