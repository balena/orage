#include <libxfce4mcs/mcs-client.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

typedef struct
{
  gboolean showCal;
  gboolean showTaskbar;
  gboolean showPager;
  gboolean startMonday;
  GtkCalendarDisplayOptions dispOptions; 
} settings;

void init_settings(GtkWidget *w);

void apply_settings();

void settings_set_showCal(GtkWidget *w);

void set_mainWin(GtkWidget *w);

void set_cal(GtkWidget *w);

int mark_appointments(GtkWidget *w);

void setup_signals(GtkWidget *w);

int keep_tidy(void);

gint alarm_clock(gpointer p);

void
on_new1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_open1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_as1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_close1_activate                     (GtkMenuItem     *menuitem, 
                                        gpointer        user_data);

void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void 
on_preferences_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_selectToday_activate                (GtkMenuItem     *menuitem,
					gpointer         user_data);

void
on_weekMonday_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cut1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_copy1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_paste1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_delete1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_XFCalendar_delete_event             (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_Today_activate (GtkMenuItem *menuitem,
				    gpointer user_data);

void
on_calendar1_day_selected_double_click (GtkCalendar     *calendar,
                                        gpointer         user_data);
void
on_calendar1_scroll                    (GtkCalendar     *calendar,
					GdkEventScroll *event);
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
on_textview1_insert_at_cursor          (GtkTextView     *textview,
                                        gchar           *string,
                                        gpointer         user_data);

void
on_textview1_cut_clipboard             (GtkTextView     *textview,
                                        gpointer         user_data);

void
on_textview1_delete_from_cursor        (GtkTextView     *textview,
                                        GtkDeleteType    type,
                                        gint             count,
                                        gpointer         user_data);

void
on_textview1_paste_clipboard           (GtkTextView     *textview,
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

void
watch_cb(Window window, Bool is_start, long mask, void *cb_data);

void 
notify_cb(const char *name, const char *channel_name, McsAction action, McsSetting * setting, void *data);
