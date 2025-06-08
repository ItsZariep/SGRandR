#include "outputmanager.h"

DisplayContext *create_display_context(void)
{
	DisplayContext *ctx = g_new0(DisplayContext, 1);
	return ctx;
}

void free_display_context(DisplayContext *ctx)
{}

gchar *get_output_name(DisplayContext *ctx, const gchar *output_name)
{
	gchar *stdout_str = NULL, *stderr_str = NULL;
	gint exit_status;

	// Correct command formatting (no single quotes inside string)
	gchar *command = g_strdup("sh -c \"xrandr --props | edid-decode\"");

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

	gchar **lines = g_strsplit(stdout_str, "\n", -1);
	gchar *result = NULL;

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

	g_strfreev(lines);
	g_free(stdout_str);
	g_free(stderr_str);

	if (result)
		g_print("OUTNAME: %s\n", result);
	else
		g_printerr("Display Product Name not found\n");

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
	if (!target || !turn_on)
	{
		g_printerr("Error: Target not specified or turn_on is false\n");
		return;
	}

	// Build argument list
	GPtrArray *argv = g_ptr_array_new_with_free_func(g_free);
	g_ptr_array_add(argv, g_strdup("xrandr"));
	g_ptr_array_add(argv, g_strdup("--output"));
	g_ptr_array_add(argv, g_strdup(target));

	if (!turn_on)
	{
		g_ptr_array_add(argv, g_strdup("--off"));
	}
	else
	{
		gchar *mode_str = g_strdup_printf("%dx%d", screenx, screeny);
		g_ptr_array_add(argv, g_strdup("--mode"));
		g_ptr_array_add(argv, mode_str);

		gchar *rate_str = g_strdup_printf("%.2f", rrate);
		g_ptr_array_add(argv, g_strdup("--rate"));
		g_ptr_array_add(argv, rate_str);

		gchar *scale_str = g_strdup_printf("%.2f", scale);
		g_print("SCALE: %s", scale_str);
		g_ptr_array_add(argv, g_strdup("--scale"));
		g_ptr_array_add(argv, scale_str);

		// Rotation
		const gchar *rot = "normal";
		switch (rotation)
		{
			case RR_Rotate_90:  rot = "right"; break;
			case RR_Rotate_180: rot = "inverted"; break;
			case RR_Rotate_270: rot = "left"; break;
			default:            rot = "normal"; break;
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


	gchar *auto_cmd[] = { "xrandr", "--auto", NULL };
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

///

	g_ptr_array_add(argv, NULL); // NULL-terminate

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
	gsize stdout_len = 0;
	GError *error = NULL;
	if (!g_spawn_command_line_sync("xrandr", &stdout_str, NULL, NULL, &error))
	{
		g_printerr("Failed to run xrandr: %s\n", error->message);
		g_clear_error(&error);
		return NULL;
	}
	gchar **lines = g_strsplit(stdout_str, "\n", -1);
	g_free(stdout_str);
	GPtrArray *res_array = g_ptr_array_new_with_free_func(g_free);
	GHashTable *unique_res = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
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
			// New output block starts
			if (g_strrstr(line, output_name))
			{
				target_found = TRUE;
				in_block = TRUE;
				continue;
			}
			else if (target_found)
			{
				//moved past the target output block
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
						// Parse width and height
						gchar **res_parts = g_strsplit(tokens[t], "x", 2);
						if (res_parts[0] && res_parts[1])
						{
							int width = atoi(res_parts[0]);
							int height = atoi(res_parts[1]);
							
							// Get aspect ratio label
							const gchar *aspect_ratio = get_aspect_ratio_label(width, height);
							
							// Format as "WxH (aspect_ratio)"
							gchar *formatted_res = g_strdup_printf("%s (%s)", tokens[t], aspect_ratio);
							
							g_hash_table_add(unique_res, g_strdup(tokens[t])); // Use original format as key
							g_ptr_array_add(res_array, formatted_res);
						}
						g_strfreev(res_parts);
					}
				}
			}
			g_strfreev(tokens);
		}
	}
	g_strfreev(lines);
	g_hash_table_destroy(unique_res);
	g_ptr_array_add(res_array, NULL); // NULL-terminate
	return (gchar **)g_ptr_array_free(res_array, FALSE);
}

gchar **get_rates(DisplayContext *ctx, const gchar *output_name, guint width, guint height)
{
	(void)ctx;
	gchar *stdout_str = NULL;
	GError *error = NULL;

	if (!g_spawn_command_line_sync("xrandr", &stdout_str, NULL, NULL, &error))
	{
		g_printerr("Failed to run xrandr: %s\n", error->message);
		g_clear_error(&error);
		return NULL;
	}
	
	gchar **lines = g_strsplit(stdout_str, "\n", -1);
	g_free(stdout_str);
	
	GPtrArray *rates_array = g_ptr_array_new_with_free_func(g_free);
	GHashTable *unique_rates = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	
	gboolean target_found = FALSE;
	gchar resolution_str[32];
	g_snprintf(resolution_str, sizeof(resolution_str), "%dx%d", width, height);
	
	for (gint i = 0; lines[i] != NULL; i++)
	{
		gchar *line = lines[i];
		
		// Skip Screen lines
		if (g_str_has_prefix(line, "Screen"))
		{
			continue;
		}

		if (!g_str_has_prefix(line, " ") && !g_str_has_prefix(line, "\t"))
		{
			if (target_found)
			{
				break;
			}
			
			if (g_strrstr(line, output_name))
			{
				target_found = TRUE;
			}
			continue;
		}
		
		// If we're in the target output block and this is a resolution line
		if (target_found && (g_str_has_prefix(line, " ") || g_str_has_prefix(line, "\t")))
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
					
					// Only add if it's a valid rate (contains digits and possibly a dot)
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
	
	g_strfreev(lines);
	g_hash_table_destroy(unique_rates);
	
	// NULL-terminate the array
	g_ptr_array_add(rates_array, NULL);
	
	return (gchar **)g_ptr_array_free(rates_array, FALSE);
}

gchar *get_display_resolution(DisplayContext *ctx, const gchar *output_name)
{
	gchar *command = g_strdup("xrandr");
	gchar *output = NULL;
	GError *error = NULL;

	if (!g_spawn_command_line_sync(command, &output, NULL, NULL, &error))
	{
		g_warning("Failed to execute xrandr: %s", error->message);
		g_error_free(error);
		g_free(command);
		return NULL;
	}
	
	g_free(command);
	
	if (!output)
	{
		g_warning("No output from xrandr command");
		return NULL;
	}
	
	gchar **lines = g_strsplit(output, "\n", -1);
	g_free(output);
	
	gboolean found_output = FALSE;
	gchar *resolution = NULL;
	
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
	gchar *command = g_strdup("xrandr");
	gchar *output = NULL;
	GError *error = NULL;

	// Execute xrandr command
	if (!g_spawn_command_line_sync(command, &output, NULL, NULL, &error))
	{
		g_warning("Failed to execute xrandr: %s", error->message);
		g_error_free(error);
		g_free(command);
		return NULL;
	}

	g_free(command);

	if (!output)
	{
		g_error("No output from xrandr command");
		return NULL;
	}

	gchar **lines = g_strsplit(output, "\n", -1);
	g_free(output);

	gboolean found_output = FALSE;
	gchar *refresh_rate = NULL;

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

	g_strfreev(lines);

	if (!refresh_rate)
	{
		g_warning("Could not find output or refresh rate for '%s'", output_name);
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

gchar **get_outputs(DisplayContext *ctx)
{
	(void)ctx;

	gchar *stdout_str = NULL;
	gchar *stderr_str = NULL;
	gint exit_status = 0;

	gboolean success = g_spawn_command_line_sync(
		"xrandr --listmonitors",
		&stdout_str,
		&stderr_str,
		&exit_status,
		NULL
	);

	if (!success || exit_status != 0)
	{
		g_printerr("ERROR: Failed to run xrandr --listmonitors: %s\n", stderr_str ? stderr_str : "Unknown error");
		g_free(stdout_str);
		g_free(stderr_str);
		return NULL;
	}

	gchar **lines = g_strsplit(stdout_str, "\n", -1);
	g_free(stdout_str);
	g_free(stderr_str);

	GPtrArray *output_array = g_ptr_array_new_with_free_func(g_free);

	for (int i = 1; lines[i] != NULL; i++)
	{
		gchar *line = g_strstrip(lines[i]);
		if (*line == '\0') continue;

		gchar **tokens = g_strsplit_set(line, " ", -1);
		for (int j = 0; tokens[j] != NULL; j++)
		{
			if (*tokens[j] == '\0') continue;
		}

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

	g_strfreev(lines);
	g_ptr_array_add(output_array, NULL);

	return (gchar **)g_ptr_array_free(output_array, FALSE);
}