#include "events.h"

guint verbose = 0;

void custom_log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	(void)log_domain;
	(void)log_level;
	gint64 now_us = g_get_real_time();

	time_t seconds = (time_t)(now_us / G_USEC_PER_SEC);
	gint milliseconds = (gint)((now_us % G_USEC_PER_SEC) / 1000);

	struct tm *tm_info = localtime(&seconds);

	gchar time_buffer[32];

	if (tm_info != NULL)
	{
		strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", tm_info);

		size_t len = strlen(time_buffer);
		snprintf(time_buffer + len, sizeof(time_buffer) - len, ".%03d", milliseconds);
	}
	else
	{
		strncpy(time_buffer, "??:??:??.???", sizeof(time_buffer));
		time_buffer[sizeof(time_buffer) - 1] = '\0';
	}

	const gchar *prefix = (const gchar *)user_data;

	g_print("** \033[0;32m%s\033[0m: \033[0;34m%s\033[0m: %s\n", prefix, time_buffer, message);
}

void g_custom_message(const gchar *prefix, const gchar *format, ...)
{
	if (verbose == 1)
	{
		va_list args;
		va_start(args, format);

		gchar *formatted_message = g_strdup_vprintf(format, args);
		va_end(args);

		g_log_set_handler(NULL, G_LOG_LEVEL_MESSAGE, (GLogFunc) custom_log_handler, (gpointer) prefix);
		g_message("%s", formatted_message);
		g_free(formatted_message);
	}
}

void combobox_match(GtkComboBoxText *combo, const gchar *input)
{
	gint count = gtk_tree_model_iter_n_children(gtk_combo_box_get_model(GTK_COMBO_BOX(combo)), NULL);
	gboolean matched = FALSE;

	// Create a copy of input and trim after first space
	gchar *input_trimmed = g_strdup(input);
	gchar *space = g_strstr_len(input_trimmed, -1, " ");
	if (space) *space = '\0';

	for (gint i = 0; i < count; ++i)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);
		gchar *item_text = gtk_combo_box_text_get_active_text(combo);

		if (item_text)
		{
			// Trim item_text after first space
			char *item_trimmed = g_strdup(item_text);
			char *item_space = strchr(item_trimmed, ' ');
			if (item_space) *item_space = '\0';

			if (g_strcmp0(item_trimmed, input_trimmed) == 0)
			{
				matched = TRUE;
				g_free(item_trimmed);
				g_free(item_text);
				break;
			}

			g_free(item_trimmed);
			g_free(item_text);
		}
	}

	g_custom_message("[MAINWINDOW]:", "Focus combobox to %s", input);
	g_free(input_trimmed);

	if (!matched)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	}
}


void on_entry_changed(GtkEntry *entry, gpointer user_data) 
{
	(void)user_data;
	const gchar *text = gtk_entry_get_text(entry);
	guint length = strlen(text);

	guint valid = 1;
	for (guint i = 0; i < length; i++) 
	{
		if (!isdigit(text[i])) 
		{
			valid = 0;
			break;
		}
	}
	if (length > 5 || !valid) 
	{
		gtk_entry_set_text(entry, length > 0 ? g_strndup(text, length - 1) : "");
	}
}

void show_error_dialog(const gchar *error_message)
{
	g_custom_message("[ERROR]: ", "%s", error_message);
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new(NULL,
		GTK_DIALOG_MODAL,
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_OK,
		"%s",
		error_message);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

	theme = gtk_icon_theme_get_default();
	info = gtk_icon_theme_lookup_icon(theme, "dialog-error", 48, 0);
	if (info != NULL)
	{
		icon = gtk_icon_info_load_icon(info, NULL);
		gtk_window_set_icon(GTK_WINDOW(dialog), icon);
		g_object_unref(icon);
		g_object_unref(info);
	}
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void on_apply_button_clicked(GtkButton *button, gpointer user_data)
{
	(void)user_data;

	//guint mode_id = 0;
	Rotation rotation = RR_Rotate_0; // Default to normal rotation
	Rotation reflection = RR_Reflect_X;

	// Retrieve user settings
	gchar *output = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(outcombo));
		g_custom_message("[EVENTS]: ", "output = %s", output);
	gchar *opwr = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(offon));
		g_custom_message("[EVENTS]: ", "Output power = %s", opwr);
	gchar *preresolution = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(rescombo));
		g_custom_message("[EVENTS]: ", "Resolution = %s", preresolution);
	gchar *prerefresh_rate = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(refcombo));
		g_custom_message("[EVENTS]: ", "Refresh Rate = %s", prerefresh_rate);
	guint prerotation = gtk_combo_box_get_active(GTK_COMBO_BOX(rotcombo));
		g_custom_message("[EVENTS]: ", "Rotation = %d", prerotation);
	guint prereflection = gtk_combo_box_get_active(GTK_COMBO_BOX(reflcombo));
		g_custom_message("[EVENTS]: ", "Reflection = %d", prereflection);
	const gchar *prepos = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(pos));
		g_custom_message("[EVENTS]: ", "Position = %s", prepos);
	const gchar *prerelative = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(outcombo2));
		g_custom_message("[EVENTS]: ", "Relative to = %s", prerelative);
	guint preprimary = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(primarybtn));
		g_custom_message("[EVENTS]: ", "Primary to = %d", preprimary);
	guint preoffon = gtk_combo_box_get_active(GTK_COMBO_BOX(offon));
		g_custom_message("[EVENTS]: ", "Output power = %d", preoffon);
	gdouble prescale = gtk_range_get_value(GTK_RANGE(slider))/100;
		g_custom_message("[EVENTS]: ", "Output power = %f", prescale);
	guint multiple = gtk_widget_get_visible(outcombo2);
		g_custom_message("[EVENTS]: ", "Multiple outputs = %d", multiple);

	preoffon = (preoffon == 1) ? 0 : 1;
	g_custom_message("[EVENTS]: ", "Output power = %d", preoffon);

	if (!output || !opwr || !preresolution || !prerefresh_rate)
	{
		show_error_dialog("Error: One or more combo box selections are invalid.");
		g_free(output);
		g_free(opwr);
		g_free(preresolution);
		g_free(prerefresh_rate);
		return;
	}

	// Clean and convert refresh rate
	gchar *hz_pos = g_strrstr(prerefresh_rate, "Hz");
	if (hz_pos)
	{
		// Trim trailing spaces before "Hz"
		for (gchar *p = hz_pos - 1; p >= prerefresh_rate && *p == ' '; --p)
			*p = '\0';
	}

	gdouble trate = g_ascii_strtod(prerefresh_rate, NULL);

	// Parse resolution
	guint twidth = 0, theight = 0;
	gchar **parts = g_strsplit(preresolution, "x", 2);
	if (parts && parts[0] && parts[1])
	{
		twidth = g_ascii_strtoull(parts[0], NULL, 10);
		theight = g_ascii_strtoull(parts[1], NULL, 10);
	}
	g_strfreev(parts);


	switch (prerotation)
	{
		case 0: rotation = RR_Rotate_0; break;
		case 1: rotation = RR_Rotate_270; break;
		case 2: rotation = RR_Rotate_90; break;
		case 3: rotation = RR_Rotate_180; break;
		default: rotation = RR_Rotate_0; break;
	}

	switch (prereflection)
	{
		case 0: reflection = RR_Rotate_0; break;
		case 1: reflection = RR_Reflect_X; break;
		case 2: reflection = RR_Reflect_Y; break;
		case 3: reflection =  RR_Reflect_X | RR_Reflect_Y; break;
		default: reflection = RR_Rotate_0 ; break;
	}

	g_custom_message("[EVENTS]: ", "Setting resolution");
	setresolution(twidth, theight, trate, output, rotation, reflection,preoffon,
		preprimary,prerelative, prepos, multiple, prescale);

	g_free(output);
	g_free(opwr);
	g_free(preresolution);
	g_free(prerefresh_rate);

	if (g_strcmp0(gtk_button_get_label(button), "OK") == 0)
	{
		gtk_main_quit();
	}
}

gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	(void)widget;
	if (event->type == GDK_BUTTON_PRESS && event->button == 3)
	{
		GtkWidget *submenu = GTK_WIDGET(data);
		gtk_menu_popup_at_pointer(GTK_MENU(submenu), NULL);
		return TRUE;
	}
	return FALSE;
}
