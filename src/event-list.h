#include <gtk/gtk.h>
#include <gdk/gdk.h>

GtkWidget* create_wEventlist(void);
void recreate_wEventlist(GtkWidget *wEventlist);
void manage_wEventlist(GtkCalendar *calendar, GtkWidget *wEventlist);
