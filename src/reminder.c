/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2008 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2003-2006 Mickael Graf (korbinus at xfce.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the 
       Free Software Foundation
       51 Franklin Street, 5th Floor
       Boston, MA 02110-1301 USA

 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#ifdef HAVE_NOTIFY
#include <libnotify/notify.h>
#endif

#include "orage-i18n.h"
#include "functions.h"
#include "mainbox.h"
#include "ical-code.h"
#include "event-list.h"
#include "appointment.h"
#include "reminder.h"
#include "tray_icon.h"
#include "parameters.h"

/*
#define ORAGE_DEBUG 1
*/

static void create_notify_reminder(alarm_struct *alarm);
void create_reminders(alarm_struct *alarm);
static void reset_orage_alarm_clock();

static void alarm_free(gpointer galarm, gpointer dummy)
{
#undef P_N
#define P_N "alarm_free: "
    alarm_struct *alarm = (alarm_struct *)galarm;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    g_free(alarm->alarm_time);
    g_free(alarm->uid);
    g_free(alarm->title);
    g_free(alarm->description);
    g_free(alarm->sound);
    g_free(alarm->cmd);
    g_free(alarm->active_alarm);
    g_free(alarm);
}

void alarm_list_free()
{
#undef P_N
#define P_N "alarm_free_all: "

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    g_list_foreach(g_par.alarm_list, alarm_free, NULL);
    g_list_free(g_par.alarm_list);
    g_par.alarm_list = NULL;
}

static void alarm_free_memory(alarm_struct *alarm)
{
#undef P_N
#define P_N "alarm_free_memory: "

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    if (!alarm->display_orage && !alarm->display_notify && !alarm->audio)
        /* all gone, need to clean memory */
        alarm_free(alarm, NULL);
    else if (!alarm->display_orage && !alarm->display_notify)
        /* if both visuals are gone we can't stop audio anymore, so stop it 
         * now before it is too late */
        alarm->repeat_cnt = 0;
}

/************************************************************/
/* persistent alarms start                                  */
/************************************************************/

static OrageRc *orage_persistent_file_open(gboolean read_only)
{
#undef P_N
#define P_N "orage_persistent_file_open: "
    gchar *fpath;
    OrageRc *orc;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    fpath = orage_data_file_location(ORAGE_PERSISTENT_ALARMS_FILE);
    if (!read_only)  /* we need to empty it before each write */
        g_remove(fpath);
    if ((orc = (OrageRc *)orage_rc_file_open(fpath, read_only)) == NULL) {
        orage_message(150, P_N "persistent alarms file open failed.");
    }
    g_free(fpath);

    return(orc);
}

static alarm_struct *alarm_read_next_alarm(OrageRc *orc, gchar *time_now)
{
#undef P_N
#define P_N "alarm_read_next_alarm: "
    alarm_struct *new_alarm;
    gchar *alarm_time;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif

    /* let's first check if the time has gone so that we need to
     * send that delayed alarm or can we just ignore it since it is
     * still in the future */
    alarm_time = orage_rc_get_str(orc, "ALARM_TIME", "0000");
    if (strcmp(time_now, alarm_time) < 0) { /* alarm has not happened, ignore */
        g_free(alarm_time);
        return(NULL);
    }

    /* this alarm has gone, so read it and show it */
    new_alarm = g_new0(alarm_struct, 1);
    new_alarm->alarm_time = alarm_time;

    new_alarm->title = orage_rc_get_str(orc, "TITLE", NULL);
    new_alarm->description = orage_rc_get_str(orc, "DESCRIPTION", NULL);
    new_alarm->persistent = TRUE; /* this must be */
    new_alarm->display_orage = orage_rc_get_bool(orc, "DISPLAY_ORAGE", FALSE);

#ifdef HAVE_NOTIFY
    new_alarm->display_notify = orage_rc_get_bool(orc, "DISPLAY_NOTIFY", FALSE);
    new_alarm->notify_refresh = orage_rc_get_bool(orc, "NOTIFY_REFRESH", FALSE);
    new_alarm->notify_timeout = orage_rc_get_int(orc, "NOTIFY_TIMEOUT", FALSE);
#endif

    new_alarm->audio = orage_rc_get_bool(orc, "AUDIO", FALSE);
    new_alarm->sound = orage_rc_get_str(orc, "SOUND", NULL);
    new_alarm->repeat_cnt = orage_rc_get_int(orc, "REPEAT_CNT", 0);
    new_alarm->repeat_delay = orage_rc_get_int(orc, "REPEAT_DELAY", 2);
    new_alarm->procedure = orage_rc_get_bool(orc, "PROCEDURE", FALSE);
    new_alarm->cmd = orage_rc_get_str(orc, "CMD", NULL);

    return(new_alarm);
}

void alarm_read()
{
#undef P_N
#define P_N "alarm_read: "
    alarm_struct *new_alarm;
    OrageRc *orc;
    struct tm *t;
    gchar *time_now;
    gchar **alarm_groups;
    gint i;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif

    t = orage_localtime();
    time_now = orage_tm_time_to_icaltime(t);
    orc = orage_persistent_file_open(TRUE);
    alarm_groups = orage_rc_get_groups(orc);
    /* alarm_groups[0] is special [NULL] entry always */
    for (i=1; alarm_groups[i] != NULL; i++) {
        orage_rc_set_group(orc, alarm_groups[i]);
        if ((new_alarm = alarm_read_next_alarm(orc, time_now)) != NULL) {
            new_alarm->uid =  alarm_groups[i];
            create_reminders(new_alarm);
            alarm_free(new_alarm, NULL);
        }
    }
    orage_rc_file_close(orc);
}

static void alarm_store(gpointer galarm, gpointer par)
{
#undef P_N
#define P_N "alarm_store: "
    alarm_struct *alarm = (alarm_struct *)galarm;
    OrageRc *orc = (OrageRc *)par;

#ifdef ORAGE_DEBUG
    orage_message(-200, P_N);
#endif
    if (!alarm->persistent)
        return; /* only store persistent alarms */

    orage_rc_set_group(par, alarm->uid);

    orage_rc_put_str(orc, "ALARM_TIME", alarm->alarm_time);
    orage_rc_put_str(orc, "TITLE", alarm->title);
    orage_rc_put_str(orc, "DESCRIPTION", alarm->description);
    orage_rc_put_bool(orc, "DISPLAY_ORAGE", alarm->display_orage);

#ifdef HAVE_NOTIFY
    orage_rc_put_bool(orc, "DISPLAY_NOTIFY", alarm->display_notify);
    /* FIXME: is NOTIFY_REFRESH really needed here? I doubt that.
     * Probably should be set to TRUE ?? */
    orage_rc_put_bool(orc, "NOTIFY_REFRESH", alarm->notify_refresh);
    orage_rc_put_int(orc, "NOTIFY_TIMEOUT", alarm->notify_timeout);
#endif

    orage_rc_put_bool(orc, "AUDIO", alarm->audio);
    orage_rc_put_str(orc, "SOUND", alarm->sound);
    orage_rc_put_int(orc, "REPEAT_CNT", alarm->repeat_cnt);
    orage_rc_put_int(orc, "REPEAT_DELAY", alarm->repeat_delay);
    orage_rc_put_bool(orc, "PROCEDURE", alarm->procedure);
    orage_rc_put_str(orc, "CMD", alarm->cmd);
}

static void store_persistent_alarms()
{
#undef P_N
#define P_N "store_persistent_alarms: "
    OrageRc *orc;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    orc = orage_persistent_file_open(FALSE);
    g_list_foreach(g_par.alarm_list, alarm_store, (gpointer)orc);
    orage_rc_file_close(orc);
}

/************************************************************/
/* persistent alarms end                                    */
/************************************************************/

#ifdef HAVE_NOTIFY
static void notify_action_open(NotifyNotification *n, const char *action
        , gpointer par)
{
#undef P_N
#define P_N "notify_action_open: "
    alarm_struct *alarm = (alarm_struct *)par;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    /* 
     * These two lines would keep notify window active and make it possible
     * to start several times the appointment or start it and still be 
     * able to control sound. Not sure if they should be there or not.
    alarm->notify_refresh = TRUE;
    create_notify_reminder(alarm);
    */
    create_appt_win("UPDATE", alarm->uid);
}
#endif

static gboolean sound_alarm(gpointer data)
{
#undef P_N
#define P_N "sound_alarm: "
    alarm_struct *alarm = (alarm_struct *)data;
    GError *error = NULL;
    gboolean status;
    GtkWidget *stop;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    /* note: -1 loops forever */
    if (alarm->repeat_cnt != 0) {
        if (alarm->active_alarm->sound_active) {
            return(TRUE);
        }
        status = orage_exec(alarm->sound
                , &alarm->active_alarm->sound_active, &error);
        if (!status) {
            g_warning("reminder: play failed (%s)", alarm->sound);
            alarm->repeat_cnt = 0; /* one warning is enough */
        }
        else if (alarm->repeat_cnt > 0)
            alarm->repeat_cnt--;
    }
    else { /* repeat_cnt == 0 */
        if (alarm->display_orage 
        && ((stop = alarm->active_alarm->stop_noise_reminder) != NULL)) {
            gtk_widget_set_sensitive(GTK_WIDGET(stop), FALSE);
        }
#ifdef HAVE_NOTIFY
        if (alarm->display_notify
        &&  alarm->active_alarm->notify_stop_noise_action) {
            /* We need to remove the silence button from notify window.
             * This is not nice method, but it is not possible to access
             * the timeout so we just need to start it from all over */
            alarm->notify_refresh = TRUE;
            notify_notification_close(alarm->active_alarm->active_notify, NULL);
            create_notify_reminder(alarm);
        }
#endif
        alarm->audio = FALSE;
        alarm_free_memory(alarm);
        status = FALSE; /* no more alarms, end timeouts */
    }
        
    return(status);
}

static void create_sound_reminder(alarm_struct *alarm)
{
#undef P_N
#define P_N "create_sound_reminder: "

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    /* done in initialisation 
    g_string_prepend(alarm->sound, " \"");
    g_string_prepend(alarm->sound, g_par.sound_application);
    g_string_append(alarm->sound, "\"");
    */

    alarm->active_alarm->sound_active = FALSE;
    if (alarm->repeat_cnt == 0) {
        alarm->repeat_cnt++; /* need to do it once */
    }

    g_timeout_add_seconds(alarm->repeat_delay, (GtkFunction) sound_alarm
            , (gpointer) alarm);
}

#ifdef HAVE_NOTIFY
static void notify_closed(NotifyNotification *n, gpointer par)
{
#undef P_N
#define P_N "notify_closed: "
    alarm_struct *alarm = (alarm_struct *)par;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    /*
    g_print("notify_closed: start %d %d\n",  alarm->audio, alarm->display_notify);
    */
    if (alarm->notify_refresh) {
        /* this is not really closing notify, but only a refresh, so
         * we do not clean the memory now */
        alarm->notify_refresh = FALSE;
    }
    else {
        alarm->display_notify = FALSE; /* I am gone */
        alarm_free_memory(alarm);
    }
}

static void notify_action_silence(NotifyNotification *n, const char *action
        , gpointer par)
{
#undef P_N
#define P_N "notify_action_silence: "
    alarm_struct *alarm = (alarm_struct *)par;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    alarm->notify_refresh = TRUE;
    alarm->repeat_cnt = 0;
    create_notify_reminder(alarm);
}
#endif

static void create_notify_reminder(alarm_struct *alarm) 
{
#undef P_N
#define P_N "create_notify_reminder: "

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
#ifdef HAVE_NOTIFY
    char heading[250];
    NotifyNotification *n;

    if (!notify_init("Orage")) {
        g_warning("Notify init failed\n");
        return;
    }

    strncpy(heading,  _("Reminder "), 199);
    if (alarm->title)
        g_strlcat(heading, alarm->title, 50);
    n = notify_notification_new(heading, alarm->description, NULL, NULL);
    alarm->active_alarm->active_notify = n;
    if (g_par.trayIcon 
    && gtk_status_icon_is_embedded((GtkStatusIcon *)g_par.trayIcon))
        notify_notification_attach_to_status_icon(n
                , (GtkStatusIcon *)g_par.trayIcon);

    if (alarm->notify_timeout == -1)
        notify_notification_set_timeout(n, NOTIFY_EXPIRES_NEVER);
    else if (alarm->notify_timeout == 0)
        notify_notification_set_timeout(n, NOTIFY_EXPIRES_DEFAULT);
    else
        notify_notification_set_timeout(n, alarm->notify_timeout*1000);

    if (alarm->uid) 
        notify_notification_add_action(n, "open", _("Open")
                , (NotifyActionCallback)notify_action_open
                , alarm, NULL);
    if ((alarm->audio) && (alarm->repeat_cnt > 1)) {
        notify_notification_add_action(n, "stop", "Silence"
                , (NotifyActionCallback)notify_action_silence
                , alarm, NULL);
        alarm->active_alarm->notify_stop_noise_action = TRUE;
    }
    (void)g_signal_connect(G_OBJECT(n), "closed"
           , G_CALLBACK(notify_closed), alarm);

    if (!notify_notification_show(n, NULL)) {
        g_warning("failed to send notification");
    }
#else
    g_warning("libnotify not linked in. Can't use notifications.");
#endif
}

static void destroy_orage_reminder(GtkWidget *wReminder, gpointer user_data)
{
#undef P_N
#define P_N "destroy_orage_reminder: "
    alarm_struct *alarm = (alarm_struct *)user_data;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    alarm->display_orage = FALSE; /* I am gone */
    alarm_free_memory(alarm);
}

static void on_btStopNoiseReminder_clicked(GtkButton *button
        , gpointer user_data)
{
#undef P_N
#define P_N "on_btStopNoiseReminder_clicked: "
    alarm_struct *alarm = (alarm_struct *)user_data;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    alarm->repeat_cnt = 0;
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
}

static void on_btOkReminder_clicked(GtkButton *button, gpointer user_data)
{
#undef P_N
#define P_N "on_btOkReminder_clicked: "
    GtkWidget *wReminder = (GtkWidget *)user_data;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    gtk_widget_destroy(wReminder); /* destroy the specific appointment window */
}

static void on_btOpenReminder_clicked(GtkButton *button, gpointer user_data)
{
#undef P_N
#define P_N "on_btOpenReminder_clicked: "
    alarm_struct *alarm = (alarm_struct *)user_data;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    create_appt_win("UPDATE", alarm->uid);
}

static void create_orage_reminder(alarm_struct *alarm)
{
#undef P_N
#define P_N "create_orage_reminder: "
    GtkWidget *wReminder;
    GtkWidget *vbReminder;
    GtkWidget *lbReminder;
    GtkWidget *daaReminder;
    GtkWidget *btOpenReminder;
    GtkWidget *btStopNoiseReminder;
    GtkWidget *btOkReminder;
    GtkWidget *swReminder;
    GtkWidget *hdReminder;
    gchar *tmp;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    wReminder = gtk_dialog_new();
    gtk_widget_set_size_request(wReminder, 300, 250);
    gtk_window_set_title(GTK_WINDOW(wReminder),  _("Reminder - Orage"));
    gtk_window_set_position(GTK_WINDOW(wReminder), GTK_WIN_POS_CENTER);
    gtk_window_set_modal(GTK_WINDOW(wReminder), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(wReminder), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(wReminder), TRUE);

    vbReminder = GTK_DIALOG(wReminder)->vbox;

    hdReminder = gtk_label_new(NULL);
    gtk_label_set_selectable(GTK_LABEL(hdReminder), TRUE);
    tmp = g_markup_printf_escaped("<b>%s</b>", alarm->title);
    gtk_label_set_markup(GTK_LABEL(hdReminder), tmp);
    g_free(tmp);
    gtk_box_pack_start(GTK_BOX(vbReminder), hdReminder, FALSE, TRUE, 0);

    swReminder = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swReminder)
            , GTK_SHADOW_NONE);
    gtk_box_pack_start(GTK_BOX(vbReminder), swReminder, TRUE, TRUE, 5);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swReminder)
            , GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    lbReminder = gtk_label_new(alarm->description);
    gtk_label_set_line_wrap(GTK_LABEL(lbReminder), TRUE);
    gtk_label_set_selectable(GTK_LABEL(lbReminder), TRUE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swReminder)
            , lbReminder);

    daaReminder = GTK_DIALOG(wReminder)->action_area;
    gtk_dialog_set_has_separator(GTK_DIALOG(wReminder), FALSE);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(daaReminder), GTK_BUTTONBOX_END);

    if (alarm->uid) {
        btOpenReminder = gtk_button_new_from_stock("gtk-open");
        gtk_dialog_add_action_widget(GTK_DIALOG(wReminder), btOpenReminder
                , GTK_RESPONSE_OK);
        g_signal_connect((gpointer) btOpenReminder, "clicked"
                , G_CALLBACK(on_btOpenReminder_clicked), alarm);
    }

    btOkReminder = gtk_button_new_from_stock("gtk-close");
    gtk_dialog_add_action_widget(GTK_DIALOG(wReminder), btOkReminder
            , GTK_RESPONSE_OK);
    GTK_WIDGET_SET_FLAGS(btOkReminder, GTK_CAN_DEFAULT);

    g_signal_connect((gpointer) btOkReminder, "clicked"
            , G_CALLBACK(on_btOkReminder_clicked), wReminder);

    if ((alarm->audio) && (alarm->repeat_cnt > 1)) {
        btStopNoiseReminder = gtk_button_new_from_stock("gtk-stop");
        alarm->active_alarm->stop_noise_reminder = btStopNoiseReminder;
        gtk_dialog_add_action_widget(GTK_DIALOG(wReminder)
                , btStopNoiseReminder, GTK_RESPONSE_OK);
        g_signal_connect((gpointer)btStopNoiseReminder, "clicked",
            G_CALLBACK(on_btStopNoiseReminder_clicked), alarm);
    }
    g_signal_connect(G_OBJECT(wReminder), "destroy",
        G_CALLBACK(destroy_orage_reminder), alarm);
    gtk_widget_show_all(wReminder);
}

static void create_procedure_reminder(alarm_struct *alarm)
{
#undef P_N
#define P_N "create_procedure_reminder: "
    /*
    gboolean status, active; / * active not used * /
    GError *error = NULL;
    */
    gchar *cmd;
    gint status;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    /*
    status = orage_exec(alarm->cmd, &active, &error);
        */
    cmd = g_strconcat(alarm->cmd, " &", NULL);
    status = system(cmd);
    if (status)
        g_warning(P_N "cmd failed(%s) status:%d", alarm->cmd, status);
    g_free(cmd);
}

void create_reminders(alarm_struct *alarm)
{
#undef P_N
#define P_N "create_reminders: "
    alarm_struct *n_alarm;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    /* FIXME: instead of copying this new private version of the alarm,
     * g_list_remove(GList *g_par.alarm_list, gconstpointer alarm);
     * remove it and use the original. saves time */
    n_alarm = g_new0(alarm_struct, 1);
    /* alarm_time is not copied. It was only used to find out when alarm
     * happens and while we are here, it happened already */
    n_alarm->uid = g_strdup(alarm->uid);
    n_alarm->title = g_strdup(alarm->title);
    n_alarm->description = g_strdup(alarm->description);
    n_alarm->display_orage = alarm->display_orage;
    n_alarm->display_notify = alarm->display_notify;
    n_alarm->notify_refresh = alarm->notify_refresh;
    n_alarm->notify_timeout = alarm->notify_timeout;
    n_alarm->audio = alarm->audio;
    if (alarm->sound != NULL)
        /* note that this changes here !*/
        /* n_alarm->sound = g_string_new(alarm->sound->str); */
        n_alarm->sound = g_strconcat(g_par.sound_application, " \""
                , alarm->sound, "\"", NULL);
    n_alarm->repeat_cnt = alarm->repeat_cnt;
    n_alarm->repeat_delay = alarm->repeat_delay;
    n_alarm->procedure = alarm->procedure;
    if (alarm->cmd != NULL)
        n_alarm->cmd = g_strdup(alarm->cmd);
    n_alarm->active_alarm = g_new0(active_alarm_struct, 1);

    if (n_alarm->audio)
        create_sound_reminder(n_alarm);
    if (n_alarm->display_orage)
        create_orage_reminder(n_alarm);
    if (n_alarm->display_notify)
        create_notify_reminder(n_alarm);
    if (n_alarm->procedure)
        create_procedure_reminder(n_alarm);
}

/* fire after the date has changed and setup the icon 
 * and change the date in the mainwindow
 */
static gboolean orage_day_change(gpointer user_data)
{
#undef P_N
#define P_N "orage_day_change: "
    struct tm *t;
    static guint previous_year=0, previous_month=0, previous_day=0;
    guint selected_year=0, selected_month=0, selected_day=0;
    guint current_year=0, current_month=0, current_day=0;
                                                                                
#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    t = orage_localtime();
  /* See if the day just changed */
    if (previous_day != t->tm_mday
    || previous_month != t->tm_mon
    || previous_year != t->tm_year + 1900) {
        current_year  = t->tm_year + 1900;
        current_month = t->tm_mon;
        current_day   = t->tm_mday;
      /* Get the selected date from calendar */
        gtk_calendar_get_date(GTK_CALENDAR(((CalWin *)g_par.xfcal)->mCalendar),
                 &selected_year, &selected_month, &selected_day);
        if (selected_year == previous_year 
        && selected_month == previous_month 
        && selected_day == previous_day) {
            /* previous day was indeed selected, 
               keep it current automatically */
            orage_select_date(GTK_CALENDAR(((CalWin *)g_par.xfcal)->mCalendar)
                    , current_year, current_month, current_day);
        }
        previous_year  = current_year;
        previous_month = current_month;
        previous_day   = current_day;
        refresh_TrayIcon();
        xfical_alarm_build_list(TRUE);  /* new alarm list when date changed */
        reset_orage_day_change(TRUE); /* setup for next time */
    }
    else { 
        /* we should very seldom come here since we schedule us to happen
         * after the date has changed, but it is possible that the clock
         * in this machine is not accurate and we end here too early.
         * */
        reset_orage_day_change(FALSE);
    }
    return(FALSE); /* we started new timer, so we end here */
}

void reset_orage_day_change(gboolean changed)
{
#undef P_N
#define P_N "reset_orage_day_change: "
    struct tm *t;
    gint secs_left;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    if (changed) { /* date was change, need to count next change time */
        t = orage_localtime();
        /* t format is 23:59:59 -> 00:00:00 so we can use 
         * 24:00:00 to represent next day.
         * Let's find out how much time we have until it happens */
        secs_left = 60*60*(24 - t->tm_hour) - 60*t->tm_min - t->tm_sec;
    }
    else { /* the change did not happen. Need to try again asap. */
        secs_left = 1;
    }
    g_par.day_timer = g_timeout_add_seconds(secs_left
            , (GtkFunction) orage_day_change, NULL);
}

/* check and raise alarms if there are any */
static gboolean orage_alarm_clock(gpointer user_data)
{
#undef P_N
#define P_N "orage_alarm_clock: "
    struct tm *t;
    GList *alarm_l;
    alarm_struct *cur_alarm;
    gboolean alarm_raised=FALSE;
    gboolean more_alarms=TRUE;
    gchar *time_now;
                                                                                
#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    t = orage_localtime();
    time_now = orage_tm_time_to_icaltime(t);
  /* Check if there are any alarms to show */
    alarm_l = g_par.alarm_list;
    for (alarm_l = g_list_first(alarm_l);
         alarm_l != NULL && more_alarms;
         alarm_l = g_list_next(alarm_l)) {
        /* remember that it is sorted list */
        cur_alarm = (alarm_struct *)alarm_l->data;
        if (strcmp(time_now, cur_alarm->alarm_time) > 0) {
            create_reminders(cur_alarm);
            alarm_raised = TRUE;
        }
        else /* sorted so scan can be stopped */
            more_alarms = FALSE; 
    }
    if (alarm_raised)  /* at least one alarm processed, need new list */
        xfical_alarm_build_list(FALSE); /* this calls reset_orage_alarm_clock */
    else
        reset_orage_alarm_clock(); /* need to setup next timer */
    return(FALSE); /* only once */
}

static void reset_orage_alarm_clock()
{
#undef P_N
#define P_N "reset_orage_alarm_clock: "
    struct tm *t, t_alarm;
    GList *alarm_l;
    alarm_struct *cur_alarm;
    gchar *next_alarm;
    gint secs_to_alarm;
    gint dd;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    if (g_par.alarm_timer) /* need to stop it if running */
        g_source_remove(g_par.alarm_timer);
    if (g_par.alarm_list) { /* we have alarms */
        t = orage_localtime();
        t->tm_mon++;
        t->tm_year = t->tm_year + 1900;
        alarm_l = g_list_first(g_par.alarm_list);
        cur_alarm = (alarm_struct *)alarm_l->data;
        next_alarm = cur_alarm->alarm_time;
        t_alarm = orage_icaltime_to_tm_time(next_alarm, FALSE);
        /* let's find out how much time we have until alarm happens */
        dd = orage_days_between(t, &t_alarm);
        secs_to_alarm = t_alarm.tm_sec  - t->tm_sec
                  + 60*(t_alarm.tm_min  - t->tm_min)
                  + 60*60*(t_alarm.tm_hour - t->tm_hour)
                  + 24*60*60*dd;
        secs_to_alarm += 1; /* alarm needs to come a bit later */
        if (secs_to_alarm < 1) /* were rare, but possible */
            secs_to_alarm = 1;
        g_par.alarm_timer = g_timeout_add_seconds(secs_to_alarm
                , (GtkFunction) orage_alarm_clock, NULL);
    }
}

/* refresh trayicon tooltip once per minute */
static gboolean orage_tooltip_update(gpointer user_data)
{
#undef P_N
#define P_N "orage_tooltip_update: "
    struct tm *t;
    GList *alarm_l;
    alarm_struct *cur_alarm;
    gboolean more_alarms=TRUE;
    GString *tooltip=NULL, *tooltip_highlight_helper=NULL;
    gint alarm_cnt=0;
    gint tooltip_alarm_limit=5;
    gint year, month, day, hour, minute, second;
    gint dd, hh, min;
    GDate *g_now, *g_alarm;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    if (!(g_par.trayIcon 
    && gtk_status_icon_is_embedded((GtkStatusIcon *)g_par.trayIcon))) {
           /* no trayicon => no need to update the tooltip */
        return(FALSE);
    }
    t = orage_localtime();
    tooltip = g_string_new(_("Next active alarms:"));
#if GTK_CHECK_VERSION(2,16,0)
    g_string_prepend(tooltip, ("<span foreground=\"blue\" weight=\"bold\" underline=\"single\">"));
    g_string_append_printf(tooltip, (" </span>"));
#endif
  /* Check if there are any alarms to show */
    alarm_l = g_par.alarm_list;
    for (alarm_l = g_list_first(alarm_l);
         alarm_l != NULL && more_alarms;
         alarm_l = g_list_next(alarm_l)) {
        /* remember that it is sorted list */
        cur_alarm = (alarm_struct *)alarm_l->data;
        if (alarm_cnt < tooltip_alarm_limit) {
            sscanf(cur_alarm->alarm_time, XFICAL_APPT_TIME_FORMAT
                    , &year, &month, &day, &hour, &minute, &second);
            g_now = g_date_new_dmy(t->tm_mday, t->tm_mon + 1
                    , t->tm_year + 1900);
            g_alarm = g_date_new_dmy(day, month, year);
            dd = g_date_days_between(g_now, g_alarm);
            g_date_free(g_now);
            g_date_free(g_alarm);
            hh = hour - t->tm_hour;
            min = minute - t->tm_min;
            if (min < 0) {
                min += 60;
                hh -= 1;
            }
            if (hh < 0) {
                hh += 24;
                dd -= 1;
            }
#if GTK_CHECK_VERSION(2,16,0)
            g_string_append_printf(tooltip, ("<span weight=\"bold\">"));
            tooltip_highlight_helper = g_string_new(" </span>");
            g_string_append_printf(tooltip_highlight_helper, 
                    (" %s"), cur_alarm->title);
            g_string_append_printf(tooltip, 
                    _("\n%02d d %02d h %02d min to: %s"),
                    dd, hh, min, tooltip_highlight_helper->str);
            g_string_free(tooltip_highlight_helper, TRUE);
#else
            g_string_append_printf(tooltip, 
                    _("\n%02d d %02d h %02d min to: %s"),
                    dd, hh, min, cur_alarm->title);
#endif
            alarm_cnt++;
        }
        else /* sorted so scan can be stopped */
            more_alarms = FALSE; 
    }
    if (alarm_cnt == 0)
        g_string_append_printf(tooltip, _("\nNo active alarms found"));
    /* deprecated since version 2.16 */
    /* after 2.16 use gtk_status_icon_set_tooltip_markup to get nicer text */
#if GTK_CHECK_VERSION(2,16,0)
    gtk_status_icon_set_tooltip_markup((GtkStatusIcon *)g_par.trayIcon, tooltip->str);
#else
    gtk_status_icon_set_tooltip((GtkStatusIcon *)g_par.trayIcon, tooltip->str);
#endif
    g_string_free(tooltip, TRUE);
    return(TRUE);
}

/* start timer to fire every minute to keep tooltip accurate */
static gboolean start_orage_tooltip_update(gpointer user_data)
{
#undef P_N
#define P_N "start_orage_tooltip_update: "

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    if (g_par.tooltip_timer) { /* need to stop it if running */
        g_source_remove(g_par.tooltip_timer);
    }

    orage_tooltip_update(NULL);
    g_par.tooltip_timer = g_timeout_add_seconds(60
            , (GtkFunction) orage_tooltip_update, NULL);
    return(FALSE);
}

/* adjust the call to happen when minute changes */
static gboolean reset_orage_tooltip_update()
{
#undef P_N
#define P_N "reset_orage_tooltip_update: "
    struct tm *t;
    gint secs_left;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    t = orage_localtime();
    secs_left = 60 - t->tm_sec;
    if (secs_left > 10) 
        orage_tooltip_update(NULL);
    /* FIXME: do not start this, if it is already in progress.
     * Minor thing and does not cause any real trouble and happens
     * only when appoinments are updated in less than 1 minute apart. 
     * Perhaps not worth fixing. 
     * Should add another timer or static time to keep track of this */
    g_timeout_add_seconds(secs_left
            , (GtkFunction) start_orage_tooltip_update, NULL);
    return(FALSE);
}

void setup_orage_alarm_clock()
{
#undef P_N
#define P_N "setup_orage_alarm_clock: "

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    reset_orage_alarm_clock();
    store_persistent_alarms(); /* keep track of alarms when orage is down */
    /* We need to use timer since for some reason it does not work if we
     * do it here directly. Ugly, I know, but it works. */
    g_timeout_add_seconds(1, (GtkFunction) reset_orage_tooltip_update, NULL);
}
