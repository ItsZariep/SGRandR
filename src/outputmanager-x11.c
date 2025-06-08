#include "outputmanager.h"

DisplayContext *create_display_context(void)
{
	DisplayContext *ctx = g_new0(DisplayContext, 1);
	ctx->display = XOpenDisplay(NULL);
	if (!ctx->display)
	{
		g_free(ctx);
		return NULL;
	}
	ctx->root = DefaultRootWindow(ctx->display);
	ctx->screenResources = XRRGetScreenResources(ctx->display, ctx->root);
	if (!ctx->screenResources)
	{
		XCloseDisplay(ctx->display);
		g_free(ctx);
		return NULL;
	}
	return ctx;
}

void free_display_context(DisplayContext *ctx)
{
	if (ctx)
	{
		if (ctx->screenResources)
			XRRFreeScreenResources(ctx->screenResources);
		if (ctx->display)
			XCloseDisplay(ctx->display);
		g_free(ctx);
	}
}

gchar *get_output_name(DisplayContext *ctx, const gchar *output_name)
{
	if (!ctx || !ctx->display || !ctx->screenResources)
	{
		fprintf(stderr, "ERROR: Invalid display context\n");
		return NULL;
	}

	gchar *monitor_name = NULL;

	for (int i = 0; i < ctx->screenResources->noutput; i++)
	{
		XRROutputInfo *info = XRRGetOutputInfo(ctx->display, ctx->screenResources, ctx->screenResources->outputs[i]);
		if (!info)
			continue;

		if (info->connection != RR_Connected || g_strcmp0(info->name, output_name) != 0)
		{
			XRRFreeOutputInfo(info);
			continue;
		}

		Atom edid_atom = XInternAtom(ctx->display, "EDID", True);
		if (edid_atom == None)
		{
			XRRFreeOutputInfo(info);
			break;
		}

		Atom type;
		int format;
		unsigned long nitems, bytes_after;
		unsigned char *prop = NULL;

		if (XRRGetOutputProperty(ctx->display, ctx->screenResources->outputs[i], edid_atom,
			0, 100, False, False, AnyPropertyType,
			 &type, &format, &nitems, &bytes_after, &prop) == Success && prop)
		{
			for (int j = 54; j <= 108; j += 18)
			{
				if (prop[j] == 0x00 && prop[j+1] == 0x00 && prop[j+2] == 0x00 && prop[j+3] == 0xFC)
				{
					gchar tmp[14];
					memcpy(tmp, &prop[j + 5], 13);
					tmp[13] = '\0';
					monitor_name = g_strdup(g_strstrip(tmp));
					break;
				}
			}
			XFree(prop);
		}

		XRRFreeOutputInfo(info);
		break;
	}

	return monitor_name;
}

typedef enum
{
	RR_None = 1 << 4  // Custom addition for clarity (not part of Xrandr)
} RotationCustom;

XRROutputInfo* get_output_info(Display *dpy, XRRScreenResources *res, const gchar *name, RROutput *out_id)
{
	for (int i = 0; i < res->noutput; i++)
	{
		XRROutputInfo *info = XRRGetOutputInfo(dpy, res, res->outputs[i]);
		if (info->name && strcmp(info->name, name) == 0)
		{
			*out_id = res->outputs[i];
			return info;
		}
		XRRFreeOutputInfo(info);
	}
	return NULL;
}

void setdefaultres(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Window root = DefaultRootWindow(dpy);

	XRRScreenResources *res = XRRGetScreenResources(dpy, root);

	for (int i = 0; i < res->noutput; ++i)
	{
		RROutput output = res->outputs[i];
		XRROutputInfo *output_info = XRRGetOutputInfo(dpy, res, output);

		if (output_info->connection == RR_Connected && output_info->crtc)
		{
			XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(dpy, res, output_info->crtc);

			if (output_info->nmode > 0)
			{
				RRMode preferred_mode = output_info->modes[0];
				XRRSetCrtcConfig(dpy, res, output_info->crtc,
					CurrentTime,
					crtc_info->x, crtc_info->y,
					preferred_mode,
					crtc_info->rotation,
					&output, 1);
			}

			XRRFreeCrtcInfo(crtc_info);
		}

		XRRFreeOutputInfo(output_info);
	}

	XRRFreeScreenResources(res);
	XCloseDisplay(dpy);
}


void setresolution(guint screenx, guint screeny, gdouble rrate, gchar *target,
	Rotation rotation, Rotation reflection, gboolean turn_on, gboolean make_primary,
	const gchar *relative_to, const gchar *position, guint multiple, double scale)
{
	Display *display = XOpenDisplay(NULL);
	if (!display)
	{
		fprintf(stderr, "Error: Can't open display\n");
		return;
	}

	Window root = DefaultRootWindow(display);
	XRRScreenResources *res = XRRGetScreenResources(display, root);
	if (!res)
	{
		fprintf(stderr, "Error: can't get screen resources\n");
		XCloseDisplay(display);
		return;
	}

	RROutput output = None;
	XRROutputInfo *output_info = get_output_info(display, res, target, &output);
	if (!output_info)
	{
		fprintf(stderr, "Error: Output %s is not available\n", target);
		XRRFreeScreenResources(res);
		XCloseDisplay(display);
		return;
	}

	RRCrtc crtc = output_info->crtc;

	// Turn off output before changes
	if (crtc != None)
	{
		XRRSetCrtcConfig(display, res, crtc, CurrentTime, 0, 0, None, RR_Rotate_0, NULL, 0);
	}

	if (!turn_on)
	{
		XRRFreeOutputInfo(output_info);
		XRRFreeScreenResources(res);
		XCloseDisplay(display);
		return;
	}

	// Find a compatible free CRTC
	RRCrtc compatible_crtc = None;
	for (int i = 0; i < output_info->ncrtc; i++)
	{
		XRRCrtcInfo *ci = XRRGetCrtcInfo(display, res, output_info->crtcs[i]);
		if (ci && ci->noutput == 0)
		{
			compatible_crtc = output_info->crtcs[i];
			XRRFreeCrtcInfo(ci);
			break;
		}
		if (ci) XRRFreeCrtcInfo(ci);
	}
	if (compatible_crtc == None)
	{
		fprintf(stderr, "Error: No free CRTC found\n");
		XRRFreeOutputInfo(output_info);
		XRRFreeScreenResources(res);
		XCloseDisplay(display);
		return;
	}

	// Use native resolution mode
	RRMode mode = None;
	for (int i = 0; i < res->nmode; i++)
	{
		XRRModeInfo mi = res->modes[i];
		if (mi.width == screenx && mi.height == screeny)
		{
			double refresh = (double)mi.dotClock / (mi.hTotal * mi.vTotal);
			if (fabs(refresh - rrate) < 0.1)
			{
				mode = mi.id;
				break;
			}
		}
	}
	if (mode == None)
	{
		fprintf(stderr, "Warning: No exact matching mode for %dx%d @ %.2fHz, using first available mode\n", screenx, screeny, rrate);
		for (int i = 0; i < res->nmode; i++)
		{
			XRRModeInfo mi = res->modes[i];
			if (mi.width == screenx && mi.height == screeny)
			{
				mode = mi.id;
				break;
			}
		}
		if (mode == None)
		{
			fprintf(stderr, "Error: No usable mode for resolution %dx%d\n", screenx, screeny);
			XRRFreeOutputInfo(output_info);
			XRRFreeScreenResources(res);
			XCloseDisplay(display);
			return;
		}
	}

	// Adjust dimensions based on rotation
	int target_width = (rotation == RR_Rotate_90 || rotation == RR_Rotate_270) ? screeny : screenx;
	int target_height = (rotation == RR_Rotate_90 || rotation == RR_Rotate_270) ? screenx : screeny;

	int pos_x = 0, pos_y = 0;

	// Relative positioning
	if (multiple == 1 && relative_to && position)
	{
		RROutput ref_output;
		XRROutputInfo *ref_info = get_output_info(display, res, relative_to, &ref_output);
		if (!ref_info || ref_info->crtc == None)
		{
			fprintf(stderr, "Error: Reference output %s is not active\n", relative_to);
			if (ref_info) XRRFreeOutputInfo(ref_info);
			XRRFreeOutputInfo(output_info);
			XRRFreeScreenResources(res);
			XCloseDisplay(display);
			return;
		}

		XRRCrtcInfo *ref_crtc_info = XRRGetCrtcInfo(display, res, ref_info->crtc);

		if (strcmp(position, "Right of") == 0)
		{
			pos_x = ref_crtc_info->x + ref_crtc_info->width;
			pos_y = ref_crtc_info->y;
		}
		else if (strcmp(position, "Left of") == 0)
		{
			pos_x = ref_crtc_info->x - target_width;
			pos_y = ref_crtc_info->y;
		}
		else if (strcmp(position, "Above of") == 0)
		{
			pos_x = ref_crtc_info->x;
			pos_y = ref_crtc_info->y - target_height;
		}
		else if (strcmp(position, "Below of") == 0)
		{
			pos_x = ref_crtc_info->x;
			pos_y = ref_crtc_info->y + ref_crtc_info->height;
		}
		else // "Same as"
		{
			pos_x = ref_crtc_info->x;
			pos_y = ref_crtc_info->y;
		}

		XRRFreeOutputInfo(ref_info);
		XRRFreeCrtcInfo(ref_crtc_info);
	}

	// Apply transform for scaling
	XTransform transform =
	{{
		{ XDoubleToFixed(scale), XDoubleToFixed(0),     XDoubleToFixed(0) },
		{ XDoubleToFixed(0),     XDoubleToFixed(scale), XDoubleToFixed(0) },
		{ XDoubleToFixed(0),     XDoubleToFixed(0),     XDoubleToFixed(1) }
	}};
	XRRSetCrtcTransform(display, compatible_crtc, &transform, "bilinear", NULL, 0);

	// Set CRTC config with scaling
	Rotation combined_rotation = rotation;
	if (reflection == RR_Reflect_X || reflection == RR_Reflect_Y || reflection == (RR_Reflect_X | RR_Reflect_Y))
	{
		combined_rotation |= reflection;
	}

	Status status = XRRSetCrtcConfig(display, res, compatible_crtc, CurrentTime,
		pos_x, pos_y, mode, combined_rotation, &output, 1);
	if (status != Success)
	{
		fprintf(stderr, "Error: Failed to set mode\n");
		setdefaultres();
		XRRFreeOutputInfo(output_info);
		XRRFreeScreenResources(res);
		XCloseDisplay(display);
		return;
	}

	// Compute virtual screen bounds
	int min_x = pos_x, min_y = pos_y;
	int max_x = pos_x + (int)(target_width * scale);
	int max_y = pos_y + (int)(target_height * scale);

	for (int i = 0; i < res->ncrtc; i++)
	{
		XRRCrtcInfo *ci = XRRGetCrtcInfo(display, res, res->crtcs[i]);
		if (ci->mode != None && res->crtcs[i] != compatible_crtc)
		{
			if (ci->x < min_x) min_x = ci->x;
			if (ci->y < min_y) min_y = ci->y;
			if ((int)(ci->x + ci->width) > max_x) max_x = (int)(ci->x + ci->width);
			if ((int)(ci->y + ci->height) > max_y) max_y = (int)(ci->y + ci->height);
		}
		XRRFreeCrtcInfo(ci);
	}

	int virtual_width = max_x - min_x;
	int virtual_height = max_y - min_y;

	int width_mm = DisplayWidthMM(display, DefaultScreen(display));
	int height_mm = DisplayHeightMM(display, DefaultScreen(display));
	XRRSetScreenSize(display, root, virtual_width, virtual_height, width_mm, height_mm);

	if (make_primary)
	{
		XRRSetOutputPrimary(display, root, output);
	}

	XRRFreeOutputInfo(output_info);
	XRRFreeScreenResources(res);
	XCloseDisplay(display);
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
	if (!ctx || !ctx->display || !ctx->screenResources)
	{
		fprintf(stderr, "ERROR: Invalid display context\n");
		return NULL;
	}

	RROutput target_output = None;
	for (int i = 0; i < ctx->screenResources->noutput; i++)
	{
		XRROutputInfo *outputInfo = XRRGetOutputInfo(ctx->display, ctx->screenResources, ctx->screenResources->outputs[i]);
		if (outputInfo && outputInfo->name && strcmp(outputInfo->name, output_name) == 0)
		{
			target_output = ctx->screenResources->outputs[i];
			XRRFreeOutputInfo(outputInfo);
			break;
		}
		if (outputInfo)
			XRRFreeOutputInfo(outputInfo);
	}

	if (target_output == None)
	{
		fprintf(stderr, "ERROR: Output '%s' not found\n", output_name);
		return NULL;
	}

	XRROutputInfo *outputInfo = XRRGetOutputInfo(ctx->display, ctx->screenResources, target_output);
	if (outputInfo == NULL || outputInfo->nmode == 0)
	{
		fprintf(stderr, "ERROR: No modes found for output '%s'\n", output_name);
		if (outputInfo)
			XRRFreeOutputInfo(outputInfo);
		return NULL;
	}

	gchar **resolutions = malloc((outputInfo->nmode + 1) * sizeof(char *));
	gchar **unique_resolutions = malloc((outputInfo->nmode + 1) * sizeof(char *));
	guint unique_count = 0;

	for (int i = 0; i < outputInfo->nmode; i++)
	{
		RRMode modeID = outputInfo->modes[i];

		for (int j = 0; j < ctx->screenResources->nmode; j++)
		{
			if (ctx->screenResources->modes[j].id == modeID)
			{
				XRRModeInfo *modeInfo = &ctx->screenResources->modes[j];

				int w = modeInfo->width;
				int h = modeInfo->height;

				const char *aspect = get_aspect_ratio_label(w, h);

				char resolutionStr[64];
				snprintf(resolutionStr, sizeof(resolutionStr), "%dx%d (%s)", w, h, aspect);

				int is_unique = 1;
				for (guint k = 0; k < unique_count; k++)
				{
					if (strcmp(unique_resolutions[k], resolutionStr) == 0)
					{
						is_unique = 0;
						break;
					}
				}

				if (is_unique)
				{
					resolutions[unique_count] = strdup(resolutionStr);
					unique_resolutions[unique_count] = strdup(resolutionStr);
					unique_count++;
				}
				break;
			}
		}
	}

	resolutions[unique_count] = NULL;

	for (guint j = 0; j < unique_count; j++)
	{
		free(unique_resolutions[j]);
	}
	free(unique_resolutions);

	XRRFreeOutputInfo(outputInfo);
	return resolutions;
}

gchar **get_rates(DisplayContext *ctx, const gchar *output_name, guint width, guint height)
{
	if (!ctx || !ctx->display || !ctx->screenResources)
	{
		fprintf(stderr, "ERROR: Invalid display context\n");
		return NULL;
	}

	RROutput selected_output = None;
	for (int i = 0; i < ctx->screenResources->noutput; i++)
	{
		XRROutputInfo *outputInfo = XRRGetOutputInfo(ctx->display, ctx->screenResources, ctx->screenResources->outputs[i]);
		if (outputInfo == NULL)
			continue;

		if (outputInfo->name && strcmp(outputInfo->name, output_name) == 0 &&
			outputInfo->connection == RR_Connected)
		{
			selected_output = ctx->screenResources->outputs[i];
			XRRFreeOutputInfo(outputInfo);
			break;
		}

		XRRFreeOutputInfo(outputInfo);
	}

	if (selected_output == None)
	{
		fprintf(stderr, "ERROR: Output '%s' not found or not connected\n", output_name);
		return NULL;
	}

	XRROutputInfo *selectedOutputInfo = XRRGetOutputInfo(ctx->display, ctx->screenResources, selected_output);
	if (selectedOutputInfo == NULL)
	{
		fprintf(stderr, "ERROR: Could not get output info for '%s'\n", output_name);
		return NULL;
	}

	gchar **rates = malloc((selectedOutputInfo->nmode + 1) * sizeof(char *));
	if (rates == NULL)
	{
		fprintf(stderr, "ERROR: Memory allocation error\n");
		XRRFreeOutputInfo(selectedOutputInfo);
		return NULL;
	}

	int unique_count = 0;
	for (int i = 0; i < selectedOutputInfo->nmode; i++)
	{
		RRMode mode_id = selectedOutputInfo->modes[i];
		for (int j = 0; j < ctx->screenResources->nmode; j++)
		{
			XRRModeInfo *modeInfo = &ctx->screenResources->modes[j];
			if (modeInfo->id != mode_id)
				continue;

			if (modeInfo->width != width || modeInfo->height != height)
				continue;

			if (modeInfo->hTotal == 0 || modeInfo->vTotal == 0)
				continue;

			double refresh_rate = (double)modeInfo->dotClock / (modeInfo->hTotal * modeInfo->vTotal);
			char rateStr[32];
			snprintf(rateStr, sizeof(rateStr), "%.2f Hz", refresh_rate);

			int is_unique = 1;
			for (int k = 0; k < unique_count; k++)
			{
				if (strcmp(rateStr, rates[k]) == 0)
				{
					is_unique = 0;
					break;
				}
			}

			if (is_unique)
				rates[unique_count++] = strdup(rateStr);

			break;
		}
	}

	rates[unique_count] = NULL;
	XRRFreeOutputInfo(selectedOutputInfo);
	return rates;
}

gchar *get_display_resolution(DisplayContext *ctx, const gchar *output_name)
{
	for (int i = 0; i < ctx->screenResources->noutput; i++)
	{
		XRROutputInfo *outputInfo = XRRGetOutputInfo(ctx->display, ctx->screenResources, ctx->screenResources->outputs[i]);
		if (!outputInfo) continue;

		if (outputInfo->connection == RR_Connected &&
			outputInfo->name && strcmp(outputInfo->name, output_name) == 0)
		{
			if (outputInfo->crtc != 0)
			{
				XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(ctx->display, ctx->screenResources, outputInfo->crtc);
				if (crtcInfo)
				{
					gchar *resolution = g_strdup_printf("%dx%d", crtcInfo->width, crtcInfo->height);
					XRRFreeCrtcInfo(crtcInfo);
					XRRFreeOutputInfo(outputInfo);
					return resolution;
				}
			}
		}
		XRRFreeOutputInfo(outputInfo);
	}

	g_warning("Could not find output or resolution for '%s'\n", output_name);
	return NULL;
}


gchar *get_display_rate(DisplayContext *ctx, const gchar *output_name)
{
	if (!ctx || !ctx->display || !ctx->screenResources)
	{
		fprintf(stderr, "ERROR: Invalid display context\n");
		return NULL;
	}

	for (int i = 0; i < ctx->screenResources->noutput; i++)
	{
		XRROutputInfo *outputInfo = XRRGetOutputInfo(ctx->display, ctx->screenResources, ctx->screenResources->outputs[i]);
		if (outputInfo == NULL)
			continue;

		if (outputInfo->connection == RR_Connected &&
			outputInfo->name != NULL &&
			strcmp(outputInfo->name, output_name) == 0)
		{
			if (outputInfo->crtc != 0)
			{
				XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(ctx->display, ctx->screenResources, outputInfo->crtc);
				if (crtcInfo != NULL)
				{
					for (int j = 0; j < ctx->screenResources->nmode; j++)
					{
						if (ctx->screenResources->modes[j].id == crtcInfo->mode)
						{
							if (ctx->screenResources->modes[j].hTotal == 0 ||
								ctx->screenResources->modes[j].vTotal == 0)
							{
								fprintf(stderr, "ERROR: Invalid mode timing data (division by zero)\n");
								break;
							}

							double rate = (double)ctx->screenResources->modes[j].dotClock /
											(ctx->screenResources->modes[j].hTotal *
											ctx->screenResources->modes[j].vTotal);

							gchar *rateStr = g_strdup_printf("%.2f", rate);

							XRRFreeCrtcInfo(crtcInfo);
							XRRFreeOutputInfo(outputInfo);
							return rateStr;
						}
					}
					XRRFreeCrtcInfo(crtcInfo);
				}
			}
		}

		XRRFreeOutputInfo(outputInfo);
	}

	fprintf(stderr, "ERROR: Could not find refresh rate for '%s'\n", output_name);
	return NULL;
}

gchar *get_display_rotation(DisplayContext *ctx, const gchar *output_name)
{
	if (!ctx || !ctx->display || !ctx->screenResources)
	{
		fprintf(stderr, "ERROR: Invalid display context\n");
		return NULL;
	}

	for (int i = 0; i < ctx->screenResources->noutput; i++)
	{
		XRROutputInfo *outputInfo = XRRGetOutputInfo(ctx->display, ctx->screenResources, ctx->screenResources->outputs[i]);
		if (outputInfo == NULL)
			continue;

		if (outputInfo->connection == RR_Connected &&
			outputInfo->name != NULL &&
			strcmp(outputInfo->name, output_name) == 0)
		{
			if (outputInfo->crtc != 0)
			{
				XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(ctx->display, ctx->screenResources, outputInfo->crtc);
				if (crtcInfo != NULL)
				{
					Rotation rotation = crtcInfo->rotation;

					const gchar *rotationStr = NULL;
					switch (rotation & 0xf) // Mask to get only rotation bits
					{
						case RR_Rotate_0:   rotationStr = "Normal"; break;
						case RR_Rotate_90:  rotationStr = "Right"; break;
						case RR_Rotate_180: rotationStr = "Inverted"; break;
						case RR_Rotate_270: rotationStr = "Left"; break;
						default:            rotationStr = "Normal"; break;
					}

					gchar *result = g_strdup(rotationStr);
					XRRFreeCrtcInfo(crtcInfo);
					XRRFreeOutputInfo(outputInfo);
					return result;
				}
			}
		}

		XRRFreeOutputInfo(outputInfo);
	}

	fprintf(stderr, "ERROR: Could not find rotation for '%s'\n", output_name);
	return NULL;
}

gchar *get_display_reflection(DisplayContext *ctx, const gchar *output_name)
{
	if (!ctx || !ctx->display || !ctx->screenResources)
	{
		fprintf(stderr, "ERROR: Invalid display context\n");
		return NULL;
	}

	for (int i = 0; i < ctx->screenResources->noutput; i++)
	{
		XRROutputInfo *outputInfo = XRRGetOutputInfo(ctx->display, ctx->screenResources, ctx->screenResources->outputs[i]);
		if (outputInfo == NULL)
			continue;

		if (outputInfo->connection == RR_Connected &&
			outputInfo->name != NULL &&
			strcmp(outputInfo->name, output_name) == 0)
		{
			if (outputInfo->crtc != 0)
			{
				XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(ctx->display, ctx->screenResources, outputInfo->crtc);
				if (crtcInfo != NULL)
				{
					Rotation reflection = crtcInfo->rotation;

					GString *reflectionStr = g_string_new(NULL);

					if ((reflection & RR_Reflect_X) && (reflection & RR_Reflect_Y))
					{
						g_string_append(reflectionStr, "H+V Mirror");
					}
					else if (reflection & RR_Reflect_X)
					{
						g_string_append(reflectionStr, "Horizontal Mirror");
					}
					else if (reflection & RR_Reflect_Y)
					{
						g_string_append(reflectionStr, "Vertical Mirror");
					}
					else
					{
						g_string_append(reflectionStr, "No mirror");
					}

					gchar *result = g_strdup(reflectionStr->str);
					g_string_free(reflectionStr, TRUE);
					XRRFreeCrtcInfo(crtcInfo);
					XRRFreeOutputInfo(outputInfo);
					return result;
				}
			}
		}

		XRRFreeOutputInfo(outputInfo);
	}

	fprintf(stderr, "ERROR: Could not find reflection for '%s'\n", output_name);
	return NULL;
}

double get_display_scale(DisplayContext *ctx, const gchar *output_name)
{
	if (!ctx || !ctx->display || !ctx->screenResources)
	{
		fprintf(stderr, "ERROR: Invalid display context\n");
		return 1;
	}

	for (int i = 0; i < ctx->screenResources->noutput; i++)
	{
		XRROutputInfo *outputInfo = XRRGetOutputInfo(ctx->display, ctx->screenResources, ctx->screenResources->outputs[i]);
		if (outputInfo == NULL)
			continue;

		if (outputInfo->connection == RR_Connected &&
			outputInfo->name != NULL &&
			strcmp(outputInfo->name, output_name) == 0)
		{
			if (outputInfo->crtc != 0)
			{
				XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(ctx->display, ctx->screenResources, outputInfo->crtc);
				if (crtcInfo != NULL)
				{
					// Find the active mode for this output
					for (int j = 0; j < outputInfo->nmode; j++)
					{
						RRMode modeId = outputInfo->modes[j];
						for (int k = 0; k < ctx->screenResources->nmode; k++)
						{
							XRRModeInfo modeInfo = ctx->screenResources->modes[k];
							if (modeInfo.id == modeId && modeInfo.id == crtcInfo->mode)
							{
								//double scale_x = (double)crtcInfo->width / modeInfo.width;
								double scale_y = (double)crtcInfo->height / modeInfo.height;

								//gchar *result = g_strdup_printf("%.3f", scale_y);
								XRRFreeCrtcInfo(crtcInfo);
								XRRFreeOutputInfo(outputInfo);
								return scale_y;
							}
						}
					}
					XRRFreeCrtcInfo(crtcInfo);
				}
			}
		}

		XRRFreeOutputInfo(outputInfo);
	}

	fprintf(stderr, "ERROR: Could not find scale for '%s'\n", output_name);
	return 1;
}


gint is_primary_output(DisplayContext *ctx, const gchar *output_name)
{
	if (!ctx || !ctx->display || !ctx->screenResources)
	{
		fprintf(stderr, "ERROR: Invalid display context\n");
		return 0;
	}

	RROutput primary = XRRGetOutputPrimary(ctx->display, ctx->root);
	if (primary == None)
	{
		fprintf(stderr, "ERROR: No primary output found\n");
		return 0;
	}

	for (int i = 0; i < ctx->screenResources->noutput; i++)
	{
		XRROutputInfo *outputInfo = XRRGetOutputInfo(ctx->display, ctx->screenResources, ctx->screenResources->outputs[i]);
		if (outputInfo == NULL)
			continue;

		if (outputInfo->name != NULL &&
			strcmp(outputInfo->name, output_name) == 0)
		{
			int result = (ctx->screenResources->outputs[i] == primary) ? 1 : 0;
			XRRFreeOutputInfo(outputInfo);
			return result;
		}

		XRRFreeOutputInfo(outputInfo);
	}

	fprintf(stderr, "ERROR: Output '%s' not found\n", output_name);
	return 0;
}


gchar **get_outputs(DisplayContext *ctx)
{
	if (!ctx || !ctx->display || !ctx->screenResources)
	{
		fprintf(stderr, "ERROR: Invalid display context\n");
		return NULL;
	}

	gchar **outputs = malloc((ctx->screenResources->noutput + 1) * sizeof(char *));
	if (outputs == NULL)
	{
		fprintf(stderr, "ERROR: Memory allocation error\n");
		return NULL;
	}

	int outputCount = 0;
	for (int i = 0; i < ctx->screenResources->noutput; i++)
	{
		XRROutputInfo *outputInfo = XRRGetOutputInfo(ctx->display, ctx->screenResources, ctx->screenResources->outputs[i]);
		if (outputInfo == NULL)
		{
			fprintf(stderr, "ERROR: Could not get details of the output\n");
			for (int j = 0; j < outputCount; j++)
				free(outputs[j]);
			free(outputs);
			return NULL;
		}

		if (outputInfo->connection == RR_Connected)
		{
			outputs[outputCount] = strdup(outputInfo->name);
			outputCount++;
		}

		XRRFreeOutputInfo(outputInfo);
	}

	outputs[outputCount] = NULL;
	return outputs;
}
