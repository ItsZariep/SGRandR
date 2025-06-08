#ifndef OUTPUTMANAGER_H
#define OUTPUTMANAGER_H

#include <gtk/gtk.h>
#include <math.h>

#ifdef X11
	#include <X11/extensions/Xrandr.h>
	#include <X11/Xlib.h>

	typedef struct {
		Display *display;
		Window root;
		XRRScreenResources *screenResources;
	} DisplayContext;
#else
	typedef struct {
		int *display;
		int root;
		int *screenResources;
	} DisplayContext;
	#define Rotation int
	#define RR_Rotate_0 0
	#define RR_Rotate_270 3
	#define RR_Rotate_90 1
	#define RR_Rotate_180 2
	#define RR_Reflect_X 1
	#define RR_Reflect_Y 2
	#define XRROutputInfo char
	#define Display int
	#define XRRScreenResources int
	#define RROutput int
	
#endif

DisplayContext *create_display_context(void);
void free_display_context(DisplayContext *ctx);
void setresolution(guint screenx, guint screeny, gdouble rrate, gchar *target,
	Rotation rotation, Rotation reflection, gboolean turn_on, gboolean make_primary,
	const gchar *relative_to, const gchar *position, guint multiple, double scale);
gchar *get_output_name(DisplayContext *ctx, const gchar *output_name);
gchar **get_resolutions(DisplayContext *ctx, const gchar *output_name);
gchar *get_display_resolution(DisplayContext *ctx, const gchar *output_name);
gchar **get_rates(DisplayContext *ctx, const gchar *output_name, guint width, guint height);
gchar *get_display_rate(DisplayContext *ctx, const gchar *output_name);
gchar *get_display_rotation(DisplayContext *ctx, const gchar *output_name);
gchar *get_display_reflection(DisplayContext *ctx, const gchar *output_name);
double get_display_scale(DisplayContext *ctx, const gchar *output_name);
gint is_primary_output(DisplayContext *ctx, const gchar *output_name);
gchar **get_outputs(DisplayContext *ctx);
#endif
