#include "sgrandr.h"

gboolean sv = FALSE;

GOptionContext *context;
GOptionEntry entries[] = 
{
	{ "nocsd", 0, 0, G_OPTION_ARG_NONE, &nocsd, "Disable CSD and use fallback display", NULL },
	{ "testmode", 0, 0, G_OPTION_ARG_NONE, &testmode, "Enable test mode", NULL },
	{ "verbose", 0, 0, G_OPTION_ARG_NONE, &verbose, " Show verbose program messages", NULL },
	{ "version", 0, 0, G_OPTION_ARG_NONE, &sv, "Show program version", NULL },
	{ NULL, 0, 0, 0, NULL, NULL, NULL }
};

int main(int argc, char *argv[])
{
	context = g_option_context_new("- Simple GTK display settings");
	g_option_context_add_main_entries(context, entries, NULL);


	GError *error = NULL;
	if (!g_option_context_parse(context, &argc, &argv, &error))
	{
		g_printerr("Option parsing failed: %s\n", error->message);
		g_error_free(error);
		return 1;
	}

	if (sv)
	{
		gchar *sgrandr_platform;
		#ifdef X11
			sgrandr_platform = "x11";
		#elif WAYLAND
			sgrandr_platform = "Wayland";
		#else
			sgrandr_platform = "xrandr/wlr-randr";
		#endif

		gchar *version_msg = g_strdup_printf("SGRandr %s - %s ", sgrandr_platform, pver);
		g_print("%s\n", version_msg);
		return 0;
	}


	gtk_init(&argc, &argv);
	create_window();
}
