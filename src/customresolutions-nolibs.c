#include "customresolutions.h"

#include "events.h"

void setcustomresolution(guint screenx, guint screeny, gchar *target, gdouble rrate) 
{
	if (!target)
	{
		g_printerr("Error: Target not specified \n");
		return;
	}

	// Build argument list
	GPtrArray *argv = g_ptr_array_new_with_free_func(g_free);
	if (outmode == 0)
	{
		g_ptr_array_add(argv, g_strdup("xrandr"));
	}
	else
	{
		g_ptr_array_add(argv, g_strdup("wlr-randr"));
	}

	g_ptr_array_add(argv, g_strdup("--output"));
	g_ptr_array_add(argv, g_strdup(target));

	gchar *mode_str;
	if (outmode == 0)
	{
		mode_str = g_strdup_printf("%dx%d", screenx, screeny);
	}
	else
	{
		mode_str = g_strdup_printf("%dx%d@%.6f", screenx, screeny, rrate);
	}

	g_ptr_array_add(argv, g_strdup("--custom-mode"));
	g_ptr_array_add(argv, mode_str);

	if (outmode == 0)
	{
		gchar *rate_str = g_strdup_printf("%.2f", rrate);
		g_ptr_array_add(argv, g_strdup("--rate"));
		g_ptr_array_add(argv, rate_str);
	}


		g_ptr_array_add(argv, NULL); 
		// Print the command to be run
		g_printerr("Running command:");
		for (guint i = 0; i < argv->len; i++)
		{
			g_printerr(" %s", (gchar *)g_ptr_array_index(argv, i));
		}
		g_printerr("\n");

		gchar *stdout_str = NULL;
		gchar *stderr_str = NULL;
		gint exit_status = 0;
		GError *error = NULL;

		gboolean success = g_spawn_sync(
			NULL,
			(gchar **)argv->pdata,
			NULL,
			G_SPAWN_SEARCH_PATH,
			NULL,
			NULL,
			&stdout_str,
			&stderr_str,
			&exit_status,
			&error
		);

		if (!success)
		{
			g_printerr("Failed to run command: %s\n", error->message);
			g_error_free(error);
		}
		else if (exit_status != 0)
		{
			g_printerr("command exited with status %d\n", exit_status);
			if (stderr_str && *stderr_str)
			{
				g_printerr("command error output: %s\n", stderr_str);
			}
		}

		g_free(stdout_str);
		g_free(stderr_str);
		g_ptr_array_free(argv, TRUE);
}

void on_customapply_clicked(GtkWidget *dummy, gpointer data)
{
	gchar *output = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(customoutcombo));
		g_custom_message("[EVENTS]: ", "output = %s", output);
	const gchar *prex = gtk_entry_get_text(GTK_ENTRY(customwidth));
		g_custom_message("[EVENTS]: ", "Custom X = %s", prex);
	const gchar *prey = gtk_entry_get_text(GTK_ENTRY(customheight));
		g_custom_message("[EVENTS]: ", "Custom Y = %s", prey);
	const gchar *prerefresh_rate = gtk_entry_get_text(GTK_ENTRY(customrate));
		g_custom_message("[EVENTS]: ", "Custom Refresh Rate = %s", prerefresh_rate);

		gchar *endptr = NULL;
		gdouble rrate = g_ascii_strtod(prerefresh_rate, &endptr);
		guint screenx = (guint) g_ascii_strtoull(prex, NULL, 10); 
		guint screeny = (guint) g_ascii_strtoull(prey, NULL, 10); 


	setcustomresolution(screenx, screeny, output, rrate);
}
