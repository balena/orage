#include <gtk/gtk.h>
#include <gdk/gdk.h>

GtkWidget* create_wEventlist(void);
GtkWidget* create_wClearWarn(GtkWidget *parent);
void recreate_wEventlist(GtkWidget *wEventlist);

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
                                        gpointer        user_data,
                                        gint direction);

void
on_btClose_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

gboolean 
on_wEventlist_delete_event             (GtkWidget       *widget,
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
on_btspin_changed                      (GtkSpinButton   *button,
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
manage_wEventlist(GtkCalendar *calendar, GtkWidget *wEventlist);
