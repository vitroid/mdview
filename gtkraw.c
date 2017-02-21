#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include "mdview.h"

#define KEY_STATE_SHIFT (1<<1)
#define KEY_STATE_CONTROL (1<<2)
#define KEY_STATE_ALT (1<<3)

#define TIMEOUT_REPEAT (50)

static gint cb_button_preevent(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint cb_press_button(GtkButton *button, gpointer data);
static gint cb_release_button(GtkButton *button, gpointer data);

static guint key_state = 0;
static guint button_number = 0;

gint special_key_event(GtkWidget *widget, GdkEventKey *event) {
	if (event->type == GDK_KEY_PRESS) {
		switch (event->keyval) {
			case GDK_Shift_L:
			case GDK_Shift_R:
				key_state |= KEY_STATE_SHIFT;
				break;
			case GDK_Control_L:
			case GDK_Control_R:
				key_state |= KEY_STATE_CONTROL;
				break;
			case GDK_Alt_L:
			case GDK_Alt_R:
				key_state |= KEY_STATE_ALT;
				break;
		}
	} else if (event->type == GDK_KEY_RELEASE) {
		switch (event->keyval) {
			case GDK_Shift_L:
			case GDK_Shift_R:
				key_state |= KEY_STATE_SHIFT;
				key_state -= KEY_STATE_SHIFT;
				break;
			case GDK_Control_L:
			case GDK_Control_R:
				key_state |= KEY_STATE_CONTROL;
				key_state -= KEY_STATE_CONTROL;
				break;
			case GDK_Alt_L:
			case GDK_Alt_R:
				key_state |= KEY_STATE_ALT;
				key_state -= KEY_STATE_ALT;
				break;
		}
	}
	return FALSE;
}

static guint handler_id_1;
static guint handler_id_2;
static guint handler_id_3;
static guint handler_id_4;
static gboolean accept_key_event;

void AddKeyEvent(void) {
	if (accept_key_event == FALSE) {
		accept_key_event = TRUE;
		handler_id_1 = gtk_signal_connect(GTK_OBJECT(GetTopWidget()),
				"key_press_event", GTK_SIGNAL_FUNC(special_key_event), NULL);
		handler_id_2 = gtk_signal_connect(GTK_OBJECT(GetTopWidget()),
				"key_release_event", GTK_SIGNAL_FUNC(special_key_event), NULL);
		handler_id_3 = gtk_signal_connect(GTK_OBJECT(GetTopWidget()),
				"key_press_event", GTK_SIGNAL_FUNC(key_event), NULL);
		handler_id_4 = gtk_signal_connect(GTK_OBJECT(GetTopWidget()),
				"key_release_event", GTK_SIGNAL_FUNC(key_event), NULL);
	}
}

void RemoveKeyEvent(void) {
	if (accept_key_event) {
		accept_key_event = FALSE;
		gtk_signal_disconnect(GTK_OBJECT(GetTopWidget()), handler_id_1);
		gtk_signal_disconnect(GTK_OBJECT(GetTopWidget()), handler_id_2);
		gtk_signal_disconnect(GTK_OBJECT(GetTopWidget()), handler_id_3);
		gtk_signal_disconnect(GTK_OBJECT(GetTopWidget()), handler_id_4);
	}
}

int gtkraw_button_1(void){
	return (key_state == 0 && button_number == 1);
}

int gtkraw_button_2(void) {
	return ((button_number == 1 && key_state == KEY_STATE_SHIFT) || (button_number == 2 && key_state == 0));
}

int gtkraw_key_control(void) {
	return (key_state & KEY_STATE_CONTROL);
}

void gtkraw_signal_connect_buttons(GtkWidget *a, gpointer b, int repeat) {
	gtk_signal_connect(GTK_OBJECT(a), "button_press_event",
			GTK_SIGNAL_FUNC(cb_button_preevent), a);
	gtk_signal_connect(GTK_OBJECT(a), "button_release_event",
			GTK_SIGNAL_FUNC(cb_button_preevent), a);
	if (GPOINTER_TO_UINT(repeat) == TRUE) {
		gtk_signal_connect(GTK_OBJECT(a), "pressed",
				GTK_SIGNAL_FUNC(cb_press_button), NULL);
		gtk_signal_connect(GTK_OBJECT(a), "released",
				GTK_SIGNAL_FUNC(cb_release_button), NULL);
	}
	gtk_signal_connect(GTK_OBJECT(a), "pressed",
			GTK_SIGNAL_FUNC(b), NULL);
	return;
}

static gint cb_button_preevent(GtkWidget *widget, GdkEventButton *event, gpointer data){
	button_number = event->button;
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		StopButton();
	}
	if (data != NULL && event->button ==2) {
		if (event->type == GDK_BUTTON_PRESS) {
			gtk_button_pressed(data);
		} else if (event->type == GDK_BUTTON_RELEASE) {
			gtk_button_released(data);
		}
	}
	return FALSE;
}

#define GTK_SET_REPEAT_TIMER(a) (gtk_object_set_data(GTK_OBJECT(a), "id", GUINT_TO_POINTER(gtk_timeout_add(TIMEOUT_REPEAT, cb_repeat_button, a))))
static gint cb_repeat_button(gpointer button) {
	guint count;
	count = GPOINTER_TO_UINT(gtk_object_get_data(GTK_OBJECT(button), "count"));
	if (count > 3) {
		gtk_button_pressed(button);
	} else {
		gtk_object_set_data(GTK_OBJECT(button), "count", GUINT_TO_POINTER(count+1));
		GTK_SET_REPEAT_TIMER(button);
	}
	return FALSE;
}

static gint cb_press_button(GtkButton *button, gpointer data) {
	GTK_SET_REPEAT_TIMER(button);
	return FALSE;
}

#define GTK_UNSET_REPEAT_TIMER(a) gtk_timeout_remove(GPOINTER_TO_UINT(gtk_object_get_data(GTK_OBJECT(a), "id")));
static gint cb_release_button(GtkButton *button, gpointer data) {
	GTK_UNSET_REPEAT_TIMER(button);
	gtk_object_set_data(GTK_OBJECT(button), "id", GUINT_TO_POINTER(0));
	gtk_object_set_data(GTK_OBJECT(button), "count", GUINT_TO_POINTER(0));
	return FALSE;
}

