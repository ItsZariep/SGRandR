#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gtk/gtk.h>

#include "libintl.h"
#include "locale.h"

#include "outputmanager.h"
#include "events.h"
#include "customresolutions.h"

//locale data
#define _(String) gettext(String)
#define GETTEXT_PACKAGE "sgrandr"
#define localedir "/usr/share/locale"

extern gint testmode;
extern gint nocsd;



//logic data
extern gchar **resolutions;
extern gchar **rates;
extern gchar **outputs;
extern gchar *pm;
extern GtkAccelGroup *accel_group;

extern GtkWidget *window;
extern GtkWidget *headerbar;
extern GtkWidget *button;
extern GtkWidget *image;
extern GtkWidget *wtitle;
extern GtkWidget *submenu;
extern GtkWidget *grid;
extern GtkWidget *rescombo;
extern GtkWidget *refcombo;
extern GtkWidget *rotcombo;
extern GtkWidget *outcombo;
extern GtkWidget *offon;
extern GtkWidget *pos;
extern GtkWidget *outcombo2;
extern GtkWidget *slider;
extern GtkWidget *scacombo;
extern GtkWidget *scalabel;
extern GtkWidget *outlabel;
extern GtkWidget *poslabel;
extern GtkWidget *defbtn;
extern GtkWidget *applybtn;
extern GtkWidget *global_label0;
extern GtkWidget *global_label3;
extern GtkWidget *width;
extern GtkWidget *height;
extern GtkWidget *rate;
extern GtkWidget *reflcombo;
extern GtkWidget *primarybtn;

extern GtkIconTheme *theme;
extern GtkIconInfo *info;
extern GdkPixbuf *icon;

extern GtkTreeModel *model;

int locale(void);

extern DisplayContext *ctx;

void on_submenu_item1_selected(GtkMenuItem *menuitem, gpointer userdata);
void on_submenu_item2_toggled(GtkCheckMenuItem *menu_item,void *ptr, gpointer user_data);
void on_submenu_item3_selected(GtkMenuItem *menuitem, gpointer userdata);
void create_window(void);

#endif
