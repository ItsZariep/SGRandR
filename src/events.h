#ifndef EVENTS_H
#define EVENTS_H

#include <gtk/gtk.h>
#include <ctype.h>

#include "outputmanager.h"
#include "mainwindow.h"

extern guint verbose;

void custom_log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data);
void g_custom_message(const gchar *prefix, const gchar *format, ...);
void combobox_match(GtkComboBoxText *combo, const gchar *input);
void on_entry_changed(GtkEntry *entry, gpointer user_data);
void show_error_dialog(const gchar *error_message);
void on_apply_button_clicked(GtkButton *button, gpointer user_data);
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data);
void on_default_button_clicked(GtkButton *button, gpointer user_data);

#endif
