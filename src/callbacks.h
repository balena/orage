#include <libxfce4mcs/mcs-client.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

void pretty_window (char *text);

void apply_settings();

void settings_set_showCal(GtkWidget *w);

int keep_tidy(void);

void
on_btClose_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_btSave_clicked                      (GtkButton       *button,
                                        gpointer         user_data);
void
on_btPrevious_clicked                  (GtkButton       *button, 
                                        gpointer user_data);
void
on_btToday_clicked                     (GtkButton       *button, 
                                        gpointer user_data);

void
on_btNext_clicked                      (GtkButton       *button, 
                                        gpointer user_data);
void
changeSelectedDate                     (GtkButton       *button, 
                                        gint direction);

void
on_btClose_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

gboolean 
on_wAppointment_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_btOkInfo_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_wInfo_delete_event                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_btDelete_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_btCreate_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_cancelbutton1_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_okbutton2_clicked                   (GtkButton       *button,
                                        gpointer         user_data);
void
on_btOkReminder_clicked                  (GtkButton       *button,
                                        gpointer         user_data);
gboolean
bisextile(guint year);

void
manageAppointment(GtkCalendar *calendar, GtkWidget *appointment);

GdkFilterReturn
client_event_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data);

