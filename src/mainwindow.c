#include "mainwindow.h"

gint testmode;
gint nocsd;

//logic data
gchar **resolutions;
gchar **rates;
gchar **outputs;
gchar *pm;
GtkAccelGroup *accel_group;

GtkWidget *window;
GtkWidget *headerbar;
GtkWidget *button;
GtkWidget *image;
GtkWidget *wtitle;
GtkWidget *submenu;
GtkWidget *grid;
GtkWidget *rescombo;
GtkWidget *refcombo;
GtkWidget *rotcombo;
GtkWidget *outcombo;
GtkWidget *offon;
GtkWidget *pos;
GtkWidget *outcombo2;
GtkWidget *slider;
GtkWidget *scacombo;
GtkWidget *scalabel;
GtkWidget *outlabel;
GtkWidget *poslabel;
GtkWidget *defbtn;
GtkWidget *applybtn;
GtkWidget *okbtn;
GtkWidget *cancelbtn;
GtkWidget *global_label0;
GtkWidget *global_label3;
GtkWidget *width;
GtkWidget *height;
GtkWidget *rate;
GtkWidget *displayname_label;
GtkWidget *reflcombo;
GtkWidget *primarybtn;

GtkIconTheme *theme;
GtkIconInfo *info;
GdkPixbuf *icon;

GtkTreeModel *model;

DisplayContext *ctx;

void on_default_button_clicked(GtkButton *button, gpointer user_data) 
{
	(void)button;
	(void)user_data;
	gtk_range_set_value(GTK_RANGE(slider), 100);
	gtk_combo_box_set_active(GTK_COMBO_BOX(rescombo), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(refcombo), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(rotcombo), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(scacombo), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(outcombo), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(offon), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(outcombo2), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(pos), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(reflcombo), 0);
	on_apply_button_clicked(GTK_BUTTON(defbtn), NULL);
}
//init locale
int locale(void)
{
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, localedir);
	textdomain(GETTEXT_PACKAGE);
	return 0;
}

void on_add_resolution_clicked(GtkMenuItem *menuitem, gpointer userdata)
{
	(void)userdata;
	(void)menuitem;
	g_custom_message("[ERROR]: ", "Not implemented");
}

void on_submenu_item2_toggled(GtkCheckMenuItem *menu_item,void *ptr, gpointer user_data) 
{
	(void)user_data;

	if (gtk_check_menu_item_get_active(menu_item)) 
	{
		gtk_widget_show(scacombo);
		gtk_widget_show(scalabel);
		int *int_ptr = (int *) ptr;
		*int_ptr = 1;
	} 
	else 
	{
		gtk_widget_hide(scacombo);
		gtk_widget_hide(scalabel);
		int *int_ptr = (int *) ptr;
		*int_ptr = 0;
	}
}

void show_about(GtkMenuItem *menuitem, gpointer userdata) 
{
	(void)menuitem;
	(void)userdata;
	GtkWidget *dialog;
	dialog = gtk_about_dialog_new();

		theme = gtk_icon_theme_get_default();
		info = gtk_icon_theme_lookup_icon(theme, "video-display", 48, 0);
		if (info != NULL)
		{
			icon = gtk_icon_info_load_icon(info, NULL);
			gtk_window_set_icon(GTK_WINDOW(dialog), icon);
			g_object_unref(icon);
			g_object_unref(info);
		}

	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "SGRandR");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), pver);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "Copyright © 2025 ItsZariep");
	#ifdef X11
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), _("Simple GTK Display Configurator\nThis build uses libxrandr"));
	#else
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), _("Simple GTK Display Configurator\nThis build uses xrandr/wlr-randr parsed commands"));
	#endif
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "https://codeberg.org/itszariep/sgrandr");
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(dialog), _("Project WebSite"));
	gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog),GTK_LICENSE_GPL_3_0);
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dialog),"video-display");
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

gint load_outputs(void)
{
	ctx = create_display_context();
	if (!ctx)
	{
		g_error("Failed to initialize display context.\n");
		return 1;
	}

	const gchar *sessiontype = g_getenv("XDG_SESSION_TYPE");
	#ifdef X11
		gint opcode, event, error;
		if (XQueryExtension(ctx->display, "XWAYLAND", &opcode, &event, &error) ||
			g_strcmp0(sessiontype, "wayland") == 0)
		{
			gchar *error = "You're running sgrandr-x11 on a Wayland environment.\nPlease use sgrandr-wayland or sgrandr-nolibs";
			show_error_dialog(error);
			g_error("%s", error);
		}
	#endif


	gint value = 0;
	outputs = get_outputs(ctx);
	for (gint i = 0; outputs[i] != NULL; i++)
	{
		g_custom_message("[OUTPUTMANAGER]:", "Detected output: %s", outputs[i]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(outcombo), outputs[i]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(outcombo2), outputs[i]);
		value++;
		free(outputs[i]);
	}
	free(outputs);

	if (!gtk_widget_is_visible(window))
	{
		return value;
	}

	if (value > 1 || testmode == 1)
	{
		gtk_widget_show(pos);
		gtk_widget_show(offon);
		gtk_widget_show(poslabel);
		gtk_widget_show(outcombo);
		gtk_widget_show(outcombo2);
		gtk_widget_show(outlabel);
		gtk_widget_set_sensitive(offon, TRUE);
		g_custom_message("[OUTPUTMANAGER]:", "Multiple outputs available");
	}
	else
	{
		gtk_widget_set_sensitive(offon, FALSE);
		gtk_widget_hide(pos);
		gtk_widget_hide(poslabel);
		gtk_widget_hide(outcombo2);
		g_custom_message("[OUTPUTMANAGER]:", "Single output available");
	}

	return value;
}


void update_ui(GtkWidget *dummy, gpointer data)
{
	(void)dummy;
	gint semode = GPOINTER_TO_INT(data);

	if (semode == 2)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(outcombo), 0);
		gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(outcombo));
		load_outputs();
		gtk_combo_box_set_active(GTK_COMBO_BOX(outcombo), 0);
	}

	if (gtk_combo_box_get_active(GTK_COMBO_BOX(outcombo)) == -1)
		{return;}
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(rescombo)) == -1)
		{return;}

	gchar *current_output = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(outcombo));
	g_custom_message("[MAINWINDOW]: ", "Selected output: %s ", current_output);


	gint pstatus = is_primary_output(ctx, current_output);
		g_custom_message("[OUTPUTMANAGER]: ", "%s is primary?: %d ", current_output, pstatus);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(primarybtn), pstatus);
	if (semode == 0)
	{
		resolutions = get_resolutions(ctx, current_output);
		if (resolutions == NULL)
		{
			gchar *message = g_strdup_printf
				("ERROR: %s is not available, probably the output was disconnected", current_output);
			show_error_dialog(message);
			gtk_combo_box_set_active(GTK_COMBO_BOX(outcombo), 0);
			gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(outcombo));
			load_outputs();
			gtk_combo_box_set_active(GTK_COMBO_BOX(outcombo), 0);
			return;
		}
		gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(rescombo));
		for (int i = 0; resolutions[i] != NULL; i++)
		{
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rescombo), resolutions[i]);
			free(resolutions[i]);
		}
		gchar *current_resolution = get_display_resolution(ctx, current_output);
		g_custom_message("[OUTPUTMANAGER]: ", "Current Resolution: %s", current_resolution);
		if (current_resolution)
		{
			combobox_match(GTK_COMBO_BOX_TEXT(rescombo), current_resolution);
			g_free(current_resolution);
		}
		free(resolutions);
	}


	gchar *selected_resolution = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(rescombo));
	g_custom_message("[MAINWINDOW]: ", "Selected Resolution: %s", selected_resolution);
	if (selected_resolution == NULL)
	{
		return;
	}
	else
	{
		gchar *space_pos = strchr(selected_resolution, ' ');
		if (space_pos != NULL)
		{
			gchar *cleaned_resolution = g_strndup(selected_resolution, space_pos - selected_resolution);
			g_free(selected_resolution); 
			selected_resolution = cleaned_resolution;
		}
	}

	gint swidth = 0;
	gint sheight = 0;
	gchar **res_parts = g_strsplit(selected_resolution, "x", 2);
	if (res_parts[0] != NULL && res_parts[1] != NULL) {
		swidth = (gint) g_ascii_strtoll(res_parts[0], NULL, 10);
		sheight = (gint) g_ascii_strtoll(res_parts[1], NULL, 10);

		// Now you have width and height as integers
		g_custom_message("[OUTPUTMANAGER]: ", "Selected resolution: W:%d H:%d", swidth, sheight);
	}

	g_strfreev(res_parts); 

	rates = get_rates(ctx, current_output, swidth, sheight);

	if (rates[0] == NULL)
	{
		show_error_dialog("There are no refresh rates available for this resolution");
		gtk_combo_box_set_active(GTK_COMBO_BOX(rescombo), 0);
	}
	else
	{
		gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(refcombo));

		for (int i = 0; rates[i] != NULL; i++)
		{
			g_custom_message("[OUTPUTMANAGER]: ", "Available refresh rate for %s %dx%d: %s",
				current_output, swidth, sheight, rates[i]);
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(refcombo), rates[i]);
			free(rates[i]);
		}
		free(rates);
	}

		gtk_widget_set_sensitive(refcombo, 1);

	if (semode == 0)
	{
		gchar *current_rate = get_display_rate(ctx, current_output);
		if (current_rate)
		{
			g_custom_message("[MAINWINDOW]: ", "Selected Rate: %s", current_rate);
			combobox_match(GTK_COMBO_BOX_TEXT(refcombo), current_rate);
			g_free(current_rate);
		}
	}
	else if (semode == 1)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(refcombo), 0);
		return;
	}

	gchar *current_rotation = get_display_rotation(ctx, current_output);
		g_custom_message("[OUTPUTMANAGER]: ", "Current Rotation for %s: %s ", current_output, current_rotation);
	combobox_match(GTK_COMBO_BOX_TEXT(rotcombo), current_rotation);
	gchar *current_reflection = get_display_reflection(ctx, current_output);
		g_custom_message("[OUTPUTMANAGER]: ", "Current Reflection for %s: %s ", current_output, current_reflection);
	combobox_match(GTK_COMBO_BOX_TEXT(reflcombo), current_reflection);

	gfloat current_scale = get_display_scale(ctx, current_output);
	gfloat slider_value = current_scale * 100.0;
	gtk_range_set_value(GTK_RANGE(slider), slider_value);

	g_custom_message("[OUTPUTMANAGER]: ", "Current Scale for %s: %f ", current_output, current_scale);

	const gchar *combobox2_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(outcombo2));

	//if (gtk_widget_get_visible(outcombo2))
	//{
		if (current_output && combobox2_text && g_strcmp0(current_output, combobox2_text) == 0)
		{
			GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(outcombo2));
			GtkTreeIter iter;
			gint index = 0;
			gboolean has_second = FALSE;

			if (gtk_tree_model_get_iter_first(model, &iter))
			{
				while (gtk_tree_model_iter_next(model, &iter))
				{
					index++;
					if (index == 1)
					{
						has_second = TRUE;
						break;
					}
				}
			}

			if (has_second)
			{
				gtk_combo_box_set_active(GTK_COMBO_BOX(outcombo2), 1);
				gtk_widget_set_sensitive(outcombo2, TRUE);
			}
			else
			{
				gtk_widget_set_sensitive(outcombo2, FALSE);
			}
		}
	//}



	gchar *displayname = get_output_name(ctx, current_output);
	if (displayname)
	{
		gtk_label_set_text(GTK_LABEL(displayname_label), displayname);
		g_free(displayname);
	}
	else
	{
		g_custom_message("[ERROR]", "Monitor name not found.\n");
		gtk_label_set_text(GTK_LABEL(displayname_label), "Unknown display");
	}
}

void create_window(void)
{
	locale();

	if (testmode)
	{
		printf("--testmode is enabled, displaying all options \n");
	}

	//Main window
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	const gchar *title = "%s - SGRandR";
	const gchar *translatedTitle = _("Display Settings");
	gchar *formattedTitle = g_markup_printf_escaped(title, translatedTitle);
	gtk_window_set_title(GTK_WINDOW(window), formattedTitle);

	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_free(formattedTitle);

	theme = gtk_icon_theme_get_default();
	info = gtk_icon_theme_lookup_icon(theme, "video-display", 48, 0);
	if (info != NULL)
	{
		icon = gtk_icon_info_load_icon(info, NULL);
		gtk_window_set_icon(GTK_WINDOW(window), icon);
		g_object_unref(icon);
		g_object_unref(info);
	}

	GtkWidget *confbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

	accel_group = gtk_accel_group_new();
		gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	headerbar = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(headerbar), TRUE);
	const gchar *header = "%s - SGRandR";
	gchar *formattedHeader = g_strdup_printf(header, translatedTitle);
	gtk_header_bar_set_title(GTK_HEADER_BAR(headerbar), formattedHeader);
	g_free(formattedHeader);

	GtkWidget *file_menu = gtk_menu_new();
	gtk_menu_set_reserve_toggle_size(GTK_MENU(file_menu), FALSE);
	GtkWidget *view_menu = gtk_menu_new();
	gtk_menu_set_reserve_toggle_size(GTK_MENU(view_menu), FALSE);
	GtkWidget *help_menu = gtk_menu_new();
	gtk_menu_set_reserve_toggle_size(GTK_MENU(help_menu), FALSE);

	GtkWidget *reload_item = gtk_menu_item_new_with_label(_("Reload Program"));
	GtkWidget *add_resolution_item = gtk_menu_item_new_with_label(_("Add a custom resolution"));
	GtkWidget *save_config_item = gtk_menu_item_new_with_label(_("Save current configuration"));
	GtkWidget *show_scaling_item = gtk_check_menu_item_new_with_label(_("Show \"Scaling mode \" option"));
	GtkWidget *about_item = gtk_menu_item_new_with_label(_("About"));

	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), reload_item);
	//gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());
	//gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), add_resolution_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), show_scaling_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), about_item);

	GtkWidget *file_item = gtk_menu_item_new_with_label(_("File"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
	GtkWidget *view_item = gtk_menu_item_new_with_label(_("View"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_item), view_menu);
	GtkWidget *help_item = gtk_menu_item_new_with_label(_("Help"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);

	GtkWidget *submenu = gtk_menu_new();
	gtk_menu_set_reserve_toggle_size(GTK_MENU(submenu), FALSE);

	GtkWidget *menubar = gtk_menu_bar_new();

	if (nocsd == 0)
	{

		GtkWidget *mainbutton = gtk_menu_button_new();
		GtkWidget *image = gtk_image_new_from_icon_name("video-display", GTK_ICON_SIZE_BUTTON);
		gtk_container_add(GTK_CONTAINER(mainbutton), image);

		gtk_menu_shell_append(GTK_MENU_SHELL(submenu), file_item);
		//gtk_menu_shell_append(GTK_MENU_SHELL(submenu), view_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(submenu), help_item);
		
		gtk_menu_button_set_popup(GTK_MENU_BUTTON(mainbutton), submenu);
		gtk_header_bar_pack_start(GTK_HEADER_BAR(headerbar), mainbutton);
		gtk_window_set_titlebar(GTK_WINDOW(window), headerbar);
		
		gtk_widget_show_all(submenu);
		gtk_widget_show_all(headerbar);
	}
	else
	{

		gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);
		//gtk_menu_shell_append(GTK_MENU_SHELL(menubar), view_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help_item);
		
		gtk_box_pack_start(GTK_BOX(confbox), menubar, FALSE, FALSE, 0);
		gtk_widget_show_all(menubar);
	}

	// Create grid
	grid = gtk_grid_new();
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
	//gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);


	/// Widgets

	displayname_label = gtk_label_new("");
	primarybtn = gtk_check_button_new_with_label("Primary");
	rescombo = gtk_combo_box_text_new();

	refcombo = gtk_combo_box_text_new();
	rotcombo = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rotcombo), _("Normal"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rotcombo), _("Left"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rotcombo), _("Right"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rotcombo), _("Inverted"));

	reflcombo = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(reflcombo), _("No mirror"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(reflcombo), _("Horizontal mirror"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(reflcombo), _("Vertical Mirror"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(reflcombo), _("H+V Mirror"));


	outcombo = gtk_combo_box_text_new();
	offon = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(offon), _("On"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(offon), _("Off"));

	pos = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pos), _("Same as"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pos), _("Left of"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pos), _("Right of"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pos), _("Above of"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pos), _("Below of"));
	outcombo2 = gtk_combo_box_text_new();

	slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 35, 200, 100);
	gtk_scale_set_draw_value(GTK_SCALE(slider), TRUE);
	gtk_widget_set_size_request(slider, 200, 50);
	gtk_scale_add_mark(GTK_SCALE(slider), 35, GTK_POS_TOP, "35");
	gtk_scale_add_mark(GTK_SCALE(slider), 50, GTK_POS_TOP, "50");
	gtk_scale_add_mark(GTK_SCALE(slider), 75, GTK_POS_TOP, "75");
	gtk_scale_add_mark(GTK_SCALE(slider), 100, GTK_POS_TOP, "100");
	gtk_scale_add_mark(GTK_SCALE(slider), 125, GTK_POS_TOP, "125");
	gtk_scale_add_mark(GTK_SCALE(slider), 150, GTK_POS_TOP, "150");
	gtk_scale_add_mark(GTK_SCALE(slider), 175, GTK_POS_TOP, "175");
	gtk_scale_add_mark(GTK_SCALE(slider), 200, GTK_POS_TOP, "200");

	//scacombo = gtk_combo_box_text_new();
		//gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scacombo), _("Full"));
		//gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scacombo), _("Center"));
		//gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scacombo), _("Aspect"));
		//gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scacombo), _("1:1"));
	//scalabel = gtk_label_new(_("Scaling Mode:"));

	outlabel = gtk_label_new(_("Output:"));
	poslabel = gtk_label_new(_("Position:"));

	defbtn    = gtk_button_new_with_label(_("Default"));
		gtk_widget_set_tooltip_text(defbtn, "Ctrl+D");
	applybtn  = gtk_button_new_with_label(_("Apply"));
		gtk_widget_set_tooltip_text(applybtn, "Ctrl+Return");
	cancelbtn  = gtk_button_new_with_label(_("Cancel"));
	okbtn  = gtk_button_new_with_label(_("OK"));

	// Get the Values of comboboxes

	guint multioutput = load_outputs();

	// Open Extra option is there is more than one option
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(outcombo));
	gint num_rows = gtk_tree_model_iter_n_children(model, NULL);

	//Set first option of comboboxes as default
	gtk_combo_box_set_active(GTK_COMBO_BOX(rescombo), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(refcombo), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(rotcombo), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(scacombo), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(outcombo), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(reflcombo), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(offon), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(outcombo2), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(pos), 0);

	gchar *current_output = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(outcombo));
	resolutions = get_resolutions(ctx, current_output);
	for (int i = 0; resolutions[i] != NULL; i++)
	{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rescombo), resolutions[i]);
		free(resolutions[i]);
	}
	free(resolutions);



	gchar *current_resolution = get_display_resolution(ctx, current_output);
	g_custom_message("[OUTPUTMANAGER]:", "Current resolution: %s | %s", current_output, current_resolution);
	
	if (current_resolution)
	{ 
		combobox_match(GTK_COMBO_BOX_TEXT(rescombo), current_resolution);
		g_free(current_resolution);
	}

	gchar *current_rate = get_display_rate(ctx, current_output);
	if (current_rate)
	{
		g_custom_message("[OUTPUTMANAGER]:", "Current resolution: %s", current_rate);
		combobox_match(GTK_COMBO_BOX_TEXT(refcombo), current_rate);
		g_free(current_rate);
	}

	update_ui(window, 0);
	//Items Grid position
	gtk_grid_attach(GTK_GRID(grid), displayname_label, 0, 0, 2, 1);
	gtk_grid_attach(GTK_GRID(grid), primarybtn, 2, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), outlabel, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), outcombo, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), offon, 2, 1, 1, 1);

	gtk_grid_attach(GTK_GRID(grid), gtk_label_new(_("Resolution:")), 0, 2, 1, 1);
	//gtk_grid_attach(GTK_GRID(grid), gtk_label_new(_("Enabled")), 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), rescombo, 1, 2, 1, 1);

	//gtk_grid_attach(GTK_GRID(grid), gtk_label_new(_("Refresh Rate:")), 0, 4, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), refcombo, 2, 2, 1, 1);

	gtk_grid_attach(GTK_GRID(grid), gtk_label_new(_("Rotation:")), 0, 5, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), rotcombo, 1, 5, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), reflcombo, 2, 5, 1, 1);

	gtk_grid_attach(GTK_GRID(grid), gtk_label_new(_("Scale (%):")), 0, 7, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), slider,   1, 7, 2, 1);

	gtk_grid_attach(GTK_GRID(grid), poslabel, 0, 6, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), pos, 1, 6, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), outcombo2, 2, 6, 1, 1);

		//gtk_grid_attach(GTK_GRID(grid), scalabel, 0, 7, 1, 1);
		//gtk_grid_attach(GTK_GRID(grid), scacombo,   1, 7, 1, 1);

	GtkWidget *applybox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

		gtk_box_pack_end(GTK_BOX(applybox), applybtn, FALSE, FALSE, 2);
		gtk_box_pack_end(GTK_BOX(applybox), cancelbtn, FALSE, FALSE, 2);
		gtk_box_pack_end(GTK_BOX(applybox), okbtn, FALSE, FALSE, 2);
		gtk_box_pack_start(GTK_BOX(applybox), defbtn, FALSE, FALSE, 2);

		//gtk_box_pack_start(GTK_BOX(confbox), menubar, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(confbox), grid, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(confbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(confbox), applybox, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(window), confbox);

	g_signal_connect(applybtn, "clicked", G_CALLBACK(on_apply_button_clicked), &num_rows);
	g_signal_connect(okbtn, "clicked", G_CALLBACK(on_apply_button_clicked), &num_rows);
	g_signal_connect(defbtn, "clicked", G_CALLBACK(on_default_button_clicked), NULL);
	g_signal_connect(cancelbtn, "clicked", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(rescombo, "changed", G_CALLBACK(update_ui), GINT_TO_POINTER(1));
	g_signal_connect(outcombo, "changed", G_CALLBACK(update_ui),  GINT_TO_POINTER(0));

		// Connect the submenu items to the callback function
	g_signal_connect(reload_item, "activate", G_CALLBACK(update_ui), GINT_TO_POINTER(2));
	g_signal_connect(add_resolution_item, "activate", G_CALLBACK(on_add_resolution_clicked), NULL);
	g_signal_connect(about_item, "activate", G_CALLBACK(show_about), NULL);

	if (nocsd == 1)
	{
		g_signal_connect(window, "button-press-event", G_CALLBACK(on_button_press), submenu);
	}
	gtk_widget_add_accelerator(add_resolution_item, "activate", accel_group, GDK_KEY_N, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(save_config_item, "activate", accel_group, GDK_KEY_S, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(reload_item, "activate", accel_group, GDK_KEY_R, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	gtk_widget_add_accelerator(defbtn, "activate", accel_group, GDK_KEY_D, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(applybtn, "activate", accel_group, GDK_KEY_Return, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(cancelbtn, "activate", accel_group, GDK_KEY_Escape, 0, GTK_ACCEL_VISIBLE);

	gtk_widget_show_all(window);

	if (multioutput > 1 || testmode == 1)
	{
		gtk_widget_show(pos);
		gtk_widget_show(offon);
		gtk_widget_show(poslabel);
		gtk_widget_show(outcombo);
		gtk_widget_show(outcombo2);
		gtk_widget_show(outlabel);
		//gtk_widget_show(submenu_item1);
	}
	else
	{
		gtk_widget_set_sensitive(offon, FALSE);
		gtk_widget_hide(pos);
		gtk_widget_hide(poslabel);
		gtk_widget_hide(outcombo2);
	}

	//gtk_widget_hide(scacombo);
	//gtk_widget_hide(scalabel);
	//on_rescombo_changed(GTK_COMBO_BOX(rescombo), refcombo);
	//free(outputs);
	gtk_main();
}
