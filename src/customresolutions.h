#ifndef CUSTOMRESOLUTIONS_H
#define CUSTOMRESOLUTIONS_H

#include <gtk/gtk.h>

// custom resolution

extern GtkWidget *customwidth;
extern GtkWidget *customheight;
extern GtkWidget *customrate;
extern GtkWidget *customoutcombo;

void setcustomresolution(guint width, guint height, gchar *target, gdouble rrate);
void on_customapply_clicked(GtkWidget *dummy, gpointer data);
#endif
