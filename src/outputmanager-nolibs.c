#include "outputmanager.h"
#include "events.h"

DisplayContext *create_display_context(void)
{
	DisplayContext *ctx = g_new0(DisplayContext, 1);
	const gchar *sessiontype = g_getenv("XDG_SESSION_TYPE");
	outmode = g_strcmp0(sessiontype, "x11") == 0 ? 0 : 1;
	g_custom_message("[OUTPUTMANAGER]:", "Display mode: %s", sessiontype);
	return ctx;
}

void free_display_context(DisplayContext *ctx)
{}

gchar *get_output_name(DisplayContext *ctx, const gchar *output_name)
{
	gchar *stdout_str = NULL, *stderr_str = NULL;
	gint exit_status;

	gchar *command = NULL;

	if (outmode == 0)
	{
		command = g_strdup("sh -c \"xrandr --props | edid-decode\"");
	}
	else if (outmode == 1)
	{
		command = g_strdup("wlr-randr");
	}
	else
	{
		g_printerr("Invalid output mode: %d\n", outmode);
		return NULL;
	}

	if (!g_spawn_command_line_sync(command, &stdout_str, &stderr_str, &exit_status, NULL))
	{
		g_printerr("Failed to run command\n");
		g_free(command);
		return NULL;
	}
	g_free(command);

	if (exit_status != 0)
	{
		g_printerr("Command failed with status %d\n", exit_status);
		if (stderr_str)
		{
			g_printerr("Error output: %s\n", stderr_str);
			g_free(stderr_str);
		}
		g_free(stdout_str);
		return NULL;
	}

	gchar *result = NULL;
	gchar **lines = g_strsplit(stdout_str, "\n", -1);

	if (outmode == 0)
	{
		for (gint i = 0; lines[i] != NULL; i++)
		{
			if (g_str_has_prefix(lines[i], "    Display Product Name: "))
			{
				gchar *start = strchr(lines[i], '\'');
				if (start)
				{
					gchar *end = strchr(start + 1, '\'');
					if (end && end > start)
					{
						result = g_strndup(start + 1, end - start - 1);
						break;
					}
				}
			}
		}
	}
	else if (outmode == 1)
	{
		for (gint i = 0; lines[i] != NULL; i++)
		{
			if (g_str_has_prefix(lines[i], "  Model: "))
			{
				result = g_strdup(lines[i] + strlen("  Model: "));
				break;
			}
		}
	}

	g_strfreev(lines);
	g_free(stdout_str);
	g_free(stderr_str);

	if (result)
	{
		g_custom_message("[OUTPUTMANAGER]:", "Output name: %s", result);
	}
	else
	{
		g_warning("Display name not found\n");
	}

	return result;
}


typedef enum
{
	RR_None = 1 << 4  // Custom addition for clarity (not part of Xrandr)
} RotationCustom;

XRROutputInfo* get_output_info(Display *display, XRRScreenResources *res, const char *name, RROutput *out)
{
	return NULL;
}

void setresolution(guint screenx, guint screeny, gdouble rrate, gchar *target,
	Rotation rotation, Rotation reflection, gboolean turn_on, gboolean make_primary,
	const gchar *relative_to, const gchar *position, guint multiple, double scale)
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

		if (turn_on)
		{
			if (outmode == 1)
			{
				g_ptr_array_add(argv, g_strdup("--on"));
			}
		}
		else
		{
			g_ptr_array_add(argv, g_strdup("--off"));
		}

	if (!turn_on)
	{
		//g_ptr_array_add(argv, g_strdup("--off"));
	}
	else
	{
		gchar *mode_str;
		if (outmode == 0)
		{
			mode_str = g_strdup_printf("%dx%d", screenx, screeny);
		}
		else
		{
			mode_str = g_strdup_printf("%dx%d@%.6f", screenx, screeny, rrate);
		}

		g_ptr_array_add(argv, g_strdup("--mode"));
		g_ptr_array_add(argv, mode_str);

		if (outmode == 0)
		{
			gchar *rate_str = g_strdup_printf("%.2f", rrate);
			g_ptr_array_add(argv, g_strdup("--rate"));
			g_ptr_array_add(argv, rate_str);
		}
			gchar *scale_str = g_strdup_printf("%.2f", scale);
			g_print("SCALE: %s", scale_str);
			g_ptr_array_add(argv, g_strdup("--scale"));
			g_ptr_array_add(argv, scale_str);

		if (outmode == 0)
		{
			// Rotation
			const gchar *rot = "normal";
			switch (rotation)
			{
				case RR_Rotate_90:  rot = "right"; break;
				case RR_Rotate_180: rot = "inverted"; break;
				case RR_Rotate_270: rot = "left"; break;
				default: rot = "normal"; break;
			}


			g_ptr_array_add(argv, g_strdup("--rotate"));
			g_ptr_array_add(argv, g_strdup(rot));

			// Reflection
			if (reflection == RR_Reflect_X)
			{
				g_ptr_array_add(argv, g_strdup("--reflect"));
				g_ptr_array_add(argv, g_strdup("x"));
			}
			else if (reflection == RR_Reflect_Y)
			{
				g_ptr_array_add(argv, g_strdup("--reflect"));
				g_ptr_array_add(argv, g_strdup("y"));
			}
			else if (reflection == (RR_Reflect_X | RR_Reflect_Y))
			{
				g_ptr_array_add(argv, g_strdup("--reflect"));
				g_ptr_array_add(argv, g_strdup("xy"));
			}
			else
			{
				g_ptr_array_add(argv, g_strdup("--reflect"));
				g_ptr_array_add(argv, g_strdup("normal"));
			}

			// Position
			if (multiple && relative_to && position)
			{
				if (g_strcmp0(position, "Right of") == 0)
				{
					g_ptr_array_add(argv, g_strdup("--right-of"));
					g_ptr_array_add(argv, g_strdup(relative_to));
				}
				else if (g_strcmp0(position, "Left of") == 0)
				{
					g_ptr_array_add(argv, g_strdup("--left-of"));
					g_ptr_array_add(argv, g_strdup(relative_to));
				}
				else if (g_strcmp0(position, "Above of") == 0)
				{
					g_ptr_array_add(argv, g_strdup("--above"));
					g_ptr_array_add(argv, g_strdup(relative_to));
				}
				else if (g_strcmp0(position, "Below of") == 0)
				{
					g_ptr_array_add(argv, g_strdup("--below"));
					g_ptr_array_add(argv, g_strdup(relative_to));
				}
				else if (g_strcmp0(position, "Same as") == 0)
				{
					g_ptr_array_add(argv, g_strdup("--same-as"));
					g_ptr_array_add(argv, g_strdup(relative_to));
				}
			}

			if (make_primary)
			{
				g_ptr_array_add(argv, g_strdup("--primary"));
			}
		}
		else
		{
			// Rotation
			const gchar *rot = "normal";
			switch (rotation)
			{
				case RR_Rotate_90:  rot = "90"; break;
				case RR_Rotate_180: rot = "180"; break;
				case RR_Rotate_270: rot = "270"; break;
				default: rot = "normal"; break;
			}

			gchar *transform;

			// Reflection
			if (reflection != RR_Reflect_X)
			{
				transform = g_strdup(rot);
			}
			else if (rotation == RR_Rotate_0)
			{
				transform = g_strdup("flipped");
			}
			else
			{
				transform = g_strdup_printf("flipped-%s", rot);
			}

			g_ptr_array_add(argv, g_strdup("--transform"));
			g_ptr_array_add(argv, g_strdup(transform));

			// Position
			if (multiple && relative_to && position)
			{
				if (g_strcmp0(position, "Right of") == 0)
				{
					g_ptr_array_add(argv, g_strdup("--right-of"));
					g_ptr_array_add(argv, g_strdup(relative_to));
				}
				else if (g_strcmp0(position, "Left of") == 0)
				{
					g_ptr_array_add(argv, g_strdup("--left-of"));
					g_ptr_array_add(argv, g_strdup(relative_to));
				}
				else if (g_strcmp0(position, "Above of") == 0)
				{
					g_ptr_array_add(argv, g_strdup("--above"));
					g_ptr_array_add(argv, g_strdup(relative_to));
				}
				else if (g_strcmp0(position, "Below of") == 0)
				{
					g_ptr_array_add(argv, g_strdup("--below"));
					g_ptr_array_add(argv, g_strdup(relative_to));
				}
				else if (g_strcmp0(position, "Same as") == 0)
				{
					//g_ptr_array_add(argv, g_strdup("--same-as"));
					//g_ptr_array_add(argv, g_strdup(relative_to));
				}
			}
		}
	}

	gchar **auto_cmd;


	if (turn_on)
	{
		if (outmode == 0)
		{
			auto_cmd = (gchar*[]) { "xrandr", "--output", target, "--auto", NULL };
		}
		else
		{
			auto_cmd = (gchar*[]) { "wlr-randr", "--output", target, "--preferred", NULL };
		}

		gint auto_exit_status = 0;
		GError *auto_error = NULL;

		gboolean auto_success = g_spawn_sync(
			NULL,
			auto_cmd,
			NULL,
			G_SPAWN_SEARCH_PATH,
			NULL,
			NULL,
			NULL,
			NULL,
			&auto_exit_status,
			&auto_error
		);

		if (!auto_success)
		{
			g_printerr("Failed to run 'xrandr --auto': %s\n", auto_error->message);
			g_error_free(auto_error);
		}
		else if (auto_exit_status != 0)
		{
			g_printerr("'xrandr --auto' exited with status %d\n", auto_exit_status);
		}
	}

///
	g_ptr_array_add(argv, NULL); // NULL-terminate

	if (godefault == 0)
	{
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
			g_printerr("Failed to run xrandr: %s\n", error->message);
			g_error_free(error);
		}
		else if (exit_status != 0)
		{
			g_printerr("xrandr exited with status %d\n", exit_status);
			if (stderr_str && *stderr_str)
			{
				g_printerr("xrandr error output: %s\n", stderr_str);
			}
		}

		g_free(stdout_str);
		g_free(stderr_str);
		g_ptr_array_free(argv, TRUE);
	}
}

typedef struct
{
	int w, h;
	const char *label;
} AspectRatioMap;


static const AspectRatioMap aspect_ratios[] =
{
	{16, 9, "16:9"},
	{4, 3, "4:3"},
	{5, 4, "5:4"},
	{8, 5, "16:10"},
	{21, 9, "21:9"},
	{3, 2, "3:2"},
	{32, 9, "32:9"},
	{1, 1, "1:1"},
	{0, 0, NULL}
};

int gcd(int a, int b)
{
	while (b != 0)
	{
		int t = b;
		b = a % b;
		a = t;
	}
	return a;
}

const gchar *get_aspect_ratio_label(int w, int h)
{
	int d = gcd(w, h);
	int rw = w / d;
	int rh = h / d;

	for (int i = 0; aspect_ratios[i].label != NULL; i++)
	{
		if (aspect_ratios[i].w == rw && aspect_ratios[i].h == rh)
		{
			return aspect_ratios[i].label;
		}
	}

	static char fallback[16];
	snprintf(fallback, sizeof(fallback), "%d:%d", rw, rh);
	return fallback;
}

gchar **get_resolutions(DisplayContext *ctx, const gchar *output_name)
{
	(void)ctx;
	gchar *stdout_str = NULL;
	//gsize stdout_len = 0;
	GError *error = NULL;
	gchar *command = NULL;

	if (outmode == 0)
	{
		command = g_strdup("xrandr");
	}
	else
	{
		command = g_strdup_printf("wlr-randr --output %s", output_name);
	}

	if (!g_spawn_command_line_sync(command, &stdout_str, NULL, NULL, &error))
	{
		g_printerr("Failed to run command: %s\n", error->message);
		g_clear_error(&error);
		g_free(command);
		return NULL;
	}
	g_free(command);

	gchar **lines = g_strsplit(stdout_str, "\n", -1);
	g_free(stdout_str);

	GPtrArray *res_array = g_ptr_array_new_with_free_func(g_free);
	GHashTable *unique_res = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	if (outmode == 0)
	{
		gboolean target_found = FALSE;
		gboolean in_block = FALSE;
		for (gint i = 0; lines[i] != NULL; i++)
		{
			gchar *line = lines[i];
			if (g_str_has_prefix(line, "Screen"))
			{
				continue;
			}
			if (!g_str_has_prefix(line, " "))
			{
				if (g_strrstr(line, output_name))
				{
					target_found = TRUE;
					in_block = TRUE;
					continue;
				}
				else if (target_found)
				{
					break;
				}
				else
				{
					in_block = FALSE;
				}
			}
			else if (in_block)
			{
				gchar **tokens = g_strsplit_set(line, " \t", -1);
				for (gint t = 0; tokens[t] != NULL; t++)
				{
					if (g_regex_match_simple("^[0-9]+x[0-9]+$", tokens[t], 0, 0))
					{
						if (!g_hash_table_contains(unique_res, tokens[t]))
						{
							gchar **res_parts = g_strsplit(tokens[t], "x", 2);
							if (res_parts[0] && res_parts[1])
							{
								int width = atoi(res_parts[0]);
								int height = atoi(res_parts[1]);
								const gchar *aspect_ratio = get_aspect_ratio_label(width, height);
								gchar *formatted_res = g_strdup_printf("%s (%s)", tokens[t], aspect_ratio);
								g_hash_table_add(unique_res, g_strdup(tokens[t]));
								g_ptr_array_add(res_array, formatted_res);
							}
							g_strfreev(res_parts);
						}
					}
				}
				g_strfreev(tokens);
			}
		}
	}
	else
	{
		gboolean target_found = FALSE;
		for (gint i = 0; lines[i] != NULL; i++)
		{
			gchar *line = lines[i];
			if (!g_str_has_prefix(line, " "))
			{
				if (g_strrstr(line, output_name))
				{
					target_found = TRUE;
				}
				else if (target_found)
				{
					break;
				}
				else
				{
					target_found = FALSE;
				}
			}

			// Process resolution lines (4 spaces + contains "px")
			else if (target_found && g_str_has_prefix(line, "    ") && g_strstr_len(line, -1, "px"))
			{
				gchar **tokens = g_strsplit_set(line, " \t", -1);
				for (gint t = 0; tokens[t] != NULL; t++)
				{
					if (g_regex_match_simple("^[0-9]+x[0-9]+$", tokens[t], 0, 0))
					{
						if (!g_hash_table_contains(unique_res, tokens[t]))
						{
							gchar **res_parts = g_strsplit(tokens[t], "x", 2);
							if (res_parts[0] && res_parts[1])
							{
								int width = atoi(res_parts[0]);
								int height = atoi(res_parts[1]);
								const gchar *aspect_ratio = get_aspect_ratio_label(width, height);
								gchar *formatted_res = g_strdup_printf("%s (%s)", tokens[t], aspect_ratio);
								g_hash_table_add(unique_res, g_strdup(tokens[t]));
								g_ptr_array_add(res_array, formatted_res);
							}
							g_strfreev(res_parts);
						}
						break;
					}
				}
				g_strfreev(tokens);
			}
		}
	}

	g_strfreev(lines);
	g_hash_table_destroy(unique_res);
	g_ptr_array_add(res_array, NULL);
	return (gchar **)g_ptr_array_free(res_array, FALSE);
}

gchar **get_rates(DisplayContext *ctx, const gchar *output_name, guint width, guint height)
{
	(void)ctx;
	gchar *stdout_str = NULL;
	GError *error = NULL;

	gchar **lines = NULL;

	if (outmode == 0)
	{
		if (!g_spawn_command_line_sync("xrandr", &stdout_str, NULL, NULL, &error))
		{
			g_error("Cannot get refresh rates: %s\n", error->message);
			g_clear_error(&error);
			return NULL;
		}
	}
	else
	{
		gchar *cmd = g_strdup_printf("wlr-randr --output %s", output_name);
		if (!g_spawn_command_line_sync(cmd, &stdout_str, NULL, NULL, &error))
		{
			g_error("Cannot get refresh rates (wlr-randr): %s\n", error->message);
			g_clear_error(&error);
			g_free(cmd);
			return NULL;
		}
		g_free(cmd);
	}

	lines = g_strsplit(stdout_str, "\n", -1);
	g_free(stdout_str);

	GPtrArray *rates_array = g_ptr_array_new_with_free_func(g_free);
	GHashTable *unique_rates = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	gchar resolution_str[32];
	g_snprintf(resolution_str, sizeof(resolution_str), "%dx%d", width, height);

	if (outmode == 0)
	{
		gboolean target_found = FALSE;
		for (gint i = 0; lines[i] != NULL; i++)
		{
			gchar *line = lines[i];
			if (g_str_has_prefix(line, "Screen"))
				continue;

			if (!g_str_has_prefix(line, " ") && !g_str_has_prefix(line, "\t"))
			{
				if (target_found)
					break;
				if (g_strrstr(line, output_name))
					target_found = TRUE;
				continue;
			}

			if (target_found)
			{
				gchar *trimmed_line = g_strstrip(g_strdup(line));
				gchar **tokens = g_strsplit_set(trimmed_line, " \t", -1);
				if (tokens[0] && g_strcmp0(tokens[0], resolution_str) == 0)
				{
					for (gint t = 1; tokens[t] != NULL; t++)
					{
						if (strlen(tokens[t]) == 0) continue;
						gchar *rate_token = g_strdup(tokens[t]);
						gsize len = strlen(rate_token);
						while (len > 0 && (rate_token[len - 1] == '*' || rate_token[len - 1] == '+'))
						{
							rate_token[len - 1] = '\0';
							len--;
						}

						if (len > 0 && (g_ascii_isdigit(rate_token[0]) || rate_token[0] == '.'))
						{
							if (!g_hash_table_contains(unique_rates, rate_token))
							{
								g_hash_table_add(unique_rates, g_strdup(rate_token));
								gchar *formatted_rate = g_strdup_printf("%s Hz", rate_token);
								g_ptr_array_add(rates_array, formatted_rate);
							}
						}

						g_free(rate_token);
					}
				}
				g_strfreev(tokens);
				g_free(trimmed_line);
			}
		}
	}
	else
{
	gboolean target_found = FALSE;
	for (gint i = 0; lines[i] != NULL; i++)
	{
		gchar *line = lines[i];

		if (!g_str_has_prefix(line, " "))
		{
			if (g_strrstr(line, output_name))
			{
				target_found = TRUE;
			}
			else if (target_found)
			{
				// We've moved to a different output, stop parsing
				break;
			}
			else
			{
				target_found = FALSE;
			}
		}

		// Process resolution lines (4 spaces + contains target resolution)
		else if (target_found && g_str_has_prefix(line, "    "))
		{
			gchar *trimmed_line = g_strstrip(g_strdup(line));
			if (g_str_has_prefix(trimmed_line, resolution_str))
			{
				gchar **parts = g_strsplit(trimmed_line, ",", -1);
				for (gint j = 1; parts[j] != NULL; j++)
				{
					gchar *rate_part = g_strstrip(parts[j]);
					gchar *hz_pos = g_strrstr(rate_part, "Hz");
					if (hz_pos)
					{
						*hz_pos = '\0';
						gchar *rate = g_strstrip(rate_part);
						if (!g_hash_table_contains(unique_rates, rate))
						{
							g_hash_table_add(unique_rates, g_strdup(rate));
							gchar *formatted_rate = g_strdup_printf("%s Hz", rate);
							g_ptr_array_add(rates_array, formatted_rate);
						}
					}
				}
				g_strfreev(parts);
			}
			g_free(trimmed_line);
		}
	}
}

	g_strfreev(lines);
	g_hash_table_destroy(unique_rates);
	g_ptr_array_add(rates_array, NULL);

	return (gchar **)g_ptr_array_free(rates_array, FALSE);
}


gchar *get_display_resolution(DisplayContext *ctx, const gchar *output_name)
{
	gchar *command;
	gchar *output = NULL;
	GError *error = NULL;

	if (outmode == 0)
	{
		command = g_strdup("xrandr");
	}
	else
	{
		command = g_strdup_printf("wlr-randr --output %s", output_name);
	}

	if (!g_spawn_command_line_sync(command, &output, NULL, NULL, &error))
	{
		g_warning("Cannot get display resolution: %s", error->message);
		g_error_free(error);
		g_free(command);
		return NULL;
	}

	g_free(command);

	if (!output)
	{
		g_warning("No output from command");
		return NULL;
	}

	gchar **lines = g_strsplit(output, "\n", -1);
	g_free(output);

	gchar *resolution = NULL;

	if (outmode == 0)
	{
		gboolean found_output = FALSE;

		for (int i = 0; lines[i] != NULL; i++)
		{
			gchar *line = g_strstrip(g_strdup(lines[i]));
			if (lines[i][0] != ' ' && lines[i][0] != '\t' && strlen(line) > 0)
			{
				if (g_str_has_prefix(line, output_name))
				{
					found_output = TRUE;
					g_free(line);
					continue;
				}
				else
				{
					found_output = FALSE;
				}
			}
			else if (found_output && (lines[i][0] == ' ' || lines[i][0] == '\t'))
			{
				if (strstr(line, "*"))
				{
					gchar **tokens = g_strsplit(g_strstrip(line), " ", -1);
					if (tokens && tokens[0])
					{
						resolution = g_strdup(tokens[0]);
						g_strfreev(tokens);
						g_free(line);
						break;
					}
					g_strfreev(tokens);
				}
			}
			g_free(line);
		}
	}
	else
	{
		for (int i = 0; lines[i] != NULL; i++)
		{
			gchar *line = g_strstrip(g_strdup(lines[i]));
			if (strstr(line, "current"))
			{
				gchar **tokens = g_strsplit(line, " ", -1);
				if (tokens && tokens[0])
				{
					resolution = g_strdup(tokens[0]);
					g_strfreev(tokens);
					g_free(line);
					break;
				}
				g_strfreev(tokens);
			}
			g_free(line);
		}
	}

	g_strfreev(lines);

	if (!resolution)
	{
		g_warning("Could not find output or resolution for '%s'", output_name);
		return NULL;
	}

	return resolution;
}

gchar *get_display_rate(DisplayContext *ctx, const gchar *output_name)
{
	gchar *command;
	gchar *output = NULL;
	GError *error = NULL;

	if (outmode == 0)
	{
		command = g_strdup("xrandr");
	}
	else
	{
		command = g_strdup_printf("wlr-randr --output %s", output_name);
	}

	// Execute command
	if (!g_spawn_command_line_sync(command, &output, NULL, NULL, &error))
	{
		g_warning("Cannot get display rate: %s", error->message);
		g_error_free(error);
		g_free(command);
		return NULL;
	}

	g_free(command);

	if (!output)
	{
		g_warning("No output from display command");
		return NULL;
	}

	gchar **lines = g_strsplit(output, "\n", -1);
	g_free(output);

	gchar *refresh_rate = NULL;

	if (outmode == 0)
	{
		gboolean found_output = FALSE;

		for (int i = 0; lines[i] != NULL; i++)
		{
			gchar *line = g_strstrip(g_strdup(lines[i]));

			if (lines[i][0] != ' ' && lines[i][0] != '\t' && strlen(line) > 0)
			{
				if (g_str_has_prefix(line, output_name))
				{
					found_output = TRUE;
					g_free(line);
					continue;
				}
				else
				{
					found_output = FALSE;
				}
			}
			else if (found_output && (lines[i][0] == ' ' || lines[i][0] == '\t'))
			{
				if (strstr(line, "*"))
				{
					gchar **tokens = g_strsplit_set(g_strstrip(line), " \t", -1);
					for (int j = 0; tokens && tokens[j] != NULL; j++)
					{
						if (strstr(tokens[j], "*"))
						{
							gchar *rate_with_asterisk = tokens[j];
							gchar *rate = g_strdup(rate_with_asterisk);
							gchar *asterisk_pos = strchr(rate, '*');
							if (asterisk_pos)
							{
								*asterisk_pos = '\0';
							}
							refresh_rate = g_strdup(rate);
							g_free(rate);
							break;
						}
					}
					g_strfreev(tokens);
					g_free(line);
					break;
				}
			}

			g_free(line);
		}
	}
	else
	{
		for (int i = 0; lines[i] != NULL; i++)
		{
			gchar *line = g_strstrip(g_strdup(lines[i]));
			if (strstr(line, "current"))
			{
				gchar **tokens = g_strsplit(line, ",", -1);
				if (tokens && tokens[1])
				{
					gchar *rate_str = g_strstrip(tokens[1]);
					gchar **subtokens = g_strsplit(rate_str, " ", -1);
					if (subtokens && subtokens[0])
					{
						refresh_rate = g_strdup(subtokens[0]);
						g_strfreev(subtokens);
					}
					g_strfreev(tokens);
				}
				g_free(line);
				break;
			}
			g_free(line);
		}
	}

	g_strfreev(lines);

	if (!refresh_rate)
	{
		g_warning("Could not find refresh rate for '%s'", output_name);
		return NULL;
	}

	return refresh_rate;
}

gchar *get_display_rotation(DisplayContext *ctx, const gchar *output_name)
{
	gchar *stdout_str = NULL;
	GError *error = NULL;

	if (!g_spawn_command_line_sync("xrandr", &stdout_str, NULL, NULL, &error))
	{
		g_printerr("Failed to run xrandr: %s\n", error->message);
		g_error_free(error);
		return g_strdup("Normal");
	}

	gchar **lines = g_strsplit(stdout_str, "\n", -1);
	g_free(stdout_str);

	for (gint i = 0; lines[i] != NULL; i++)
	{
		if (g_strstr_len(lines[i], -1, output_name))
		{
			if (g_strstr_len(lines[i], -1, "0 left "))
			{
				g_strfreev(lines);
				return g_strdup("Left");
			}
			else if (g_strstr_len(lines[i], -1, "0 right "))
			{
				g_strfreev(lines);
				return g_strdup("Right");
			}
			else if (g_strstr_len(lines[i], -1, "0 inverted "))
			{
				g_strfreev(lines);
				return g_strdup("Inverted");
			}
			else if (g_strstr_len(lines[i], -1, "0 normal "))
			{
				g_strfreev(lines);
				return g_strdup("Normal");
			}
			else
			{
				g_strfreev(lines);
				return g_strdup("Normal");
			}
		}
	}

	g_strfreev(lines);
	return g_strdup("Normal");
}

gchar *get_display_reflection(DisplayContext *ctx, const gchar *output_name)
{
	return NULL;
}

double get_display_scale(DisplayContext *ctx, const gchar *output_name)
{
	return 1;
}

gint is_primary_output(DisplayContext *ctx, const gchar *output_name)
{
	if (outmode == 0)
	{
		gchar *stdout_str = NULL;
		GError *error = NULL;

		if (!g_spawn_command_line_sync("xrandr", &stdout_str, NULL, NULL, &error))
		{
			g_printerr("Failed to run xrandr: %s\n", error->message);
			g_error_free(error);
			return 0;
		}


		gchar **lines = g_strsplit(stdout_str, "\n", -1);
		g_free(stdout_str);

		for (gint i = 0; lines[i] != NULL; i++)
		{
			if (g_strstr_len(lines[i], -1, output_name))
			{
				if (g_strstr_len(lines[i], -1, "primary"))
				{
					g_strfreev(lines);
					return 1;
				}
				break;
			}
		}

		g_strfreev(lines);
		return 0;
	}
	else
	{
		return 0;
	}
}

gchar **get_outputs(DisplayContext *ctx)
{
	(void)ctx;

	gchar *stdout_str = NULL;
	gchar *stderr_str = NULL;
	gint exit_status = 0;
	const gchar *command = (outmode == 0) ? "xrandr --listmonitors" : "wlr-randr";

	gboolean success = g_spawn_command_line_sync(
		command,
		&stdout_str,
		&stderr_str,
		&exit_status,
		NULL
	);

	if (!success || exit_status != 0)
	{
		g_warning("ERROR: Failed to run %s: %s\n", command, stderr_str ? stderr_str : "Unknown error");
		g_free(stdout_str);
		g_free(stderr_str);
		return NULL;
	}

	gchar **lines = g_strsplit(stdout_str, "\n", -1);
	g_free(stdout_str);
	g_free(stderr_str);

	GPtrArray *output_array = g_ptr_array_new_with_free_func(g_free);

	if (outmode == 0)
	{
		for (int i = 1; lines[i] != NULL; i++)
		{
			gchar *line = g_strstrip(lines[i]);
			if (*line == '\0') continue;

			gchar **tokens = g_strsplit_set(line, " ", -1);

			for (int j = 0; tokens[j] != NULL; j++)
			{
				if (tokens[j + 1] == NULL)
				{
					g_ptr_array_add(output_array, g_strdup(tokens[j]));
					break;
				}
			}

			g_strfreev(tokens);
		}
	}
	else if (outmode == 1)
	{
		for (int i = 0; lines[i] != NULL; i++)
		{
			if (lines[i][0] == ' ' || lines[i][0] == '\t' || lines[i][0] == '\0')
				continue;

			gchar **tokens = g_strsplit_set(lines[i], " ", -1);
			for (int j = 0; tokens[j] != NULL; j++)
			{
				if (*tokens[j] != '\0')
				{
					g_ptr_array_add(output_array, g_strdup(tokens[j]));
					break;
				}
			}

			g_strfreev(tokens);
		}
	}

	g_strfreev(lines);
	g_ptr_array_add(output_array, NULL);

	return (gchar **)g_ptr_array_free(output_array, FALSE);
}
