/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2013 Juha Kautto  (juha at xfce.org)
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
#include <config.h>
#endif

#include <sys/stat.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <time.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#define ORAGE_MAIN  "orage"

#include "orage-i18n.h"
#include "functions.h"
#include "mainbox.h"
#include "reminder.h"
#include "ical-code.h"
#include "tray_icon.h"
#include "parameters.h"
#include "interface.h"
#ifdef HAVE_DBUS
#include "orage-dbus.h"
#include <dbus/dbus-glib-lowlevel.h>
#endif

/* session client handler */
/*
static SessionClient	*session_client = NULL;
*/
static GdkAtom atom_alive;

#ifdef HAVE_DBUS
static gboolean resume_after_sleep(gpointer user_data)
{
    orage_message(10, "Resuming after sleep");
    alarm_read();
    orage_day_change(&g_par);
    return(FALSE); /* only once */
}

static void resuming_cb(DBusGProxy *proxy, gpointer user_data)
{
    orage_message(10, "Resuming");
    /* we need this delay to prevent updating tray icon too quickly when
       the normal code handles it also */
    g_timeout_add_seconds(2, (GtkFunction) resume_after_sleep, NULL);
}

static void handle_resuming(void)
{
    DBusGConnection *connection;
    GError *error = NULL;
    DBusGProxy *proxy;

    g_type_init();
    connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
    if (connection) {
       proxy = dbus_g_proxy_new_for_name(connection, "org.freedesktop.UPower"
               , "/org/freedesktop/UPower", "org.freedesktop.UPower");
       if (proxy) {
           dbus_g_proxy_add_signal(proxy, "Resuming", G_TYPE_INVALID);
           dbus_g_proxy_connect_signal(proxy, "Resuming"
                   , G_CALLBACK(resuming_cb), NULL, NULL);
       } 
       else {
           g_warning("Failed to create proxy object\n");
       }
    } 
    else {
        g_warning("Failed to connect to D-BUS daemon: %s\n", error->message);
    }
}
#endif


/* This function monitors that we do not loose time.  It checks if longer time
   than expected wakeup time has passed and fixes timings if that is the case.
   This is needed since sometimes hibernate and suspend does not do a good job
   in firing Orage timers and timing gets wrong. 
NOTE: called from parameters.c set_wakeup_timer
*/
gboolean check_wakeup(gpointer user_data)
{
    static time_t tt_prev=0;
    time_t tt_new=0;

    tt_new = time(NULL);
    if (tt_new - tt_prev > ORAGE_WAKEUP_TIMER_PERIOD * 2) {
        /* we very rarely come here. */ 
        /* user_data is normally NULL, but first call it has some value, 
           which means that this is init call */
        if (!user_data) { /* normal timer call */
            orage_message(10, "wakeup timer refreshing");
            alarm_read();
            /* It is quite possible that day did not change, 
               but we need to reset timers */
            orage_day_change(&tt_prev); 
        }
        else {
            orage_message(10, "wakeup timer init %d", tt_prev);
        }
    }
    tt_prev = tt_new;
    return(TRUE);
}

static void send_event(char *event)
{
    GdkEventClient gev;
    Window xwindow;

    memset(&gev, 0, sizeof(gev));

    xwindow = XGetSelectionOwner(GDK_DISPLAY()
            , gdk_x11_atom_to_xatom(atom_alive));

    gev.type = GDK_CLIENT_EVENT;
    gev.window = NULL;
    gev.send_event = TRUE;
    gev.message_type = gdk_atom_intern(event, FALSE);
    gev.data_format = 8;

    gdk_event_send_client_message((GdkEvent *)&gev, (GdkNativeWindow)xwindow);
}

void orage_toggle_visible(void)
{
    send_event("_XFCE_CALENDAR_TOGGLE_HERE");
}

static void raise_orage(void)
{
    send_event("_XFCE_CALENDAR_RAISE");
}

static gboolean mWindow_delete_event_cb(GtkWidget *widget, GdkEvent *event
        , gpointer user_data)
{
    orage_toggle_visible();
    return(TRUE);
}

static void raise_window(void)
{
    GdkWindow *window;
    CalWin *cal = (CalWin *)g_par.xfcal;

    if (g_par.pos_x || g_par.pos_y)
        gtk_window_move(GTK_WINDOW(cal->mWindow)
                , g_par.pos_x, g_par.pos_y);
    if (g_par.select_always_today)
        orage_select_today(GTK_CALENDAR(cal->mCalendar));
    if (g_par.set_stick)
        gtk_window_stick(GTK_WINDOW(cal->mWindow));
    gtk_window_set_keep_above(GTK_WINDOW(cal->mWindow)
            , g_par.set_ontop);
    window = GTK_WIDGET(cal->mWindow)->window;
    gdk_x11_window_set_user_time(window, gdk_x11_get_server_time(window));
    gtk_widget_show(cal->mWindow);
    gtk_window_present(GTK_WINDOW(cal->mWindow));
}

static gboolean client_message_received(GtkWidget *widget
        , GdkEventClient *event, gpointer user_data)
{
    CalWin *cal = (CalWin *)g_par.xfcal;

    if (event->message_type ==
            gdk_atom_intern("_XFCE_CALENDAR_RAISE", FALSE)) {
        raise_window();
        return(TRUE);
    }
    else if (event->message_type ==
            gdk_atom_intern("_XFCE_CALENDAR_TOGGLE_HERE", FALSE)) {
        if (GTK_WIDGET_VISIBLE(cal->mWindow)) {
            write_parameters();
            gtk_widget_hide(cal->mWindow);
            return(TRUE);
        }
        else {
            raise_window();
            return(TRUE);
        }
    }
    else if (event->message_type ==
            gdk_atom_intern("_XFCE_CALENDAR_PREFERENCES", FALSE)) {
        show_parameters();
        return(TRUE);
    }

    return(FALSE);
}

static gboolean keep_tidy(void)
{
#ifdef HAVE_ARCHIVE
    /* move old appointment to other file to keep the active
       calendar file smaller and faster */
    xfical_archive();
#endif
    write_parameters();
    return(TRUE);
}

/*
 * SaveYourself callback
 *
 * This is called when the session manager requests the client to save its
 * state.
 */
/*
void save_yourself_cb(gpointer data, int save_style, gboolean shutdown
        , int interact_style, gboolean fast)
{
    write_parameters();
}
*/

/*
 * Die callback
 *
 * This is called when the session manager requests the client to go down.
 */
/*
void die_cb(gpointer data)
{
    gtk_main_quit();
}
*/

static void print_version(void)
{
    g_print(_("\tThis is %s version %s\n\n")
            , PACKAGE, VERSION);
    g_print(_("\tReleased under the terms of the GNU General Public License.\n"));
    g_print(_("\tCompiled against GTK+-%d.%d.%d, ")
            , GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
    g_print(_("using GTK+-%d.%d.%d.\n")
            , gtk_major_version, gtk_minor_version, gtk_micro_version);
#ifdef HAVE_DBUS
    g_print(_("\tUsing DBUS for import.\n"));
#else
    g_print(_("\tNot using DBUS. Import works only partially.\n"));
#endif
#ifdef HAVE_NOTIFY
    g_print(_("\tUsing libnotify.\n"));
#else
    g_print(_("\tNot using libnotify.\n"));
#endif
#ifdef HAVE_ARCHIVE
    g_print(_("\tUsing automatic archiving.\n"));
#else
    g_print(_("\tNot using archiving.\n"));
#endif
#ifdef HAVE_LIBICAL
    g_print(_("\tUsing operating system package libical.\n"));
#else
    g_print(_("\tUsing Orage local version of libical.\n"));
#endif
    g_print("\n");
}

static void preferences(void)
{
    send_event("_XFCE_CALENDAR_RAISE");
    send_event("_XFCE_CALENDAR_PREFERENCES");
}

static void print_help(void)
{
    g_print(_("Usage: orage [options] [files]\n\n"));
    g_print(_("Options:\n"));
    g_print(_("--version (-v) \t\tshow version of orage\n"));
    g_print(_("--help (-h) \t\tprint this text\n"));
    g_print(_("--preferences (-p) \tshow preferences form\n"));
    g_print(_("--toggle (-t) \t\tmake orage visible/unvisible\n"));
    g_print(_("--add-foreign (-a) file [RW] \tadd a foreign file\n"));
    g_print(_("--remove-foreign (-r) file \tremove a foreign file\n"));
    g_print(_("--export (-e) file [appointment...] \texport appointments from Orage to file\n"));
    g_print("\n");
    g_print(_("files=ical files to load into orage\n"));
#ifndef HAVE_DBUS
    g_print(_("\tdbus not included in orage. \n"));
    g_print(_("\twithout dbus [files] and foreign file options(-a & -r) can only be used when starting orage \n"));
#endif
    g_print("\n");
}

static void import_file(gboolean running, char *file_name, gboolean initialized)
{
    if (running && !initialized) {
        /* let's use dbus since server is running there already */
#ifdef HAVE_DBUS
        if (orage_dbus_import_file(file_name))
            orage_message(40, "import done file=%s", file_name);
        else
            g_warning("import failed file=%s\n", file_name);
#else
        g_warning("Can not do import without dbus. import failed file=%s\n", file_name);
#endif
    }
    else if (!running && initialized) {/* do it self directly */
        if (xfical_import_file(file_name))
            orage_message(40, "import done file=%s", file_name);
        else
            g_warning("import failed file=%s\n", file_name);
    }
}

static void export_file(gboolean running, char *file_name, gboolean initialized
        , gchar *uid_list)
{
    gint type = 0;
    
    if (uid_list)
        type = 1;
    else
        type = 0;
    g_print("export_file: running=%d initialized= %d type=%d, file=%s, uids=%s\n", running, initialized, type, file_name, uid_list);
    if (running && !initialized) {
        /* let's use dbus since server is running there already */
#ifdef HAVE_DBUS
        if (orage_dbus_export_file(file_name, type, uid_list))
            orage_message(40, "export done to file=%s", file_name);
        else
            g_warning("export failed file=%s\n", file_name);
#else
        g_warning("Can not do export without dbus. failed file=%s\n", file_name);
#endif
    }
    else if (!running && initialized) { /* do it self directly */
        if (xfical_export_file(file_name, type, uid_list))
            orage_message(40, "export done to file=%s", file_name);
        else
            g_warning("export failed file=%s\n", file_name);
    }
}

static void add_foreign(gboolean running, char *file_name, gboolean initialized
        , gboolean read_only)
{
    if (running && !initialized) {
        /* let's use dbus since server is running there already */
#ifdef HAVE_DBUS
        if (orage_dbus_foreign_add(file_name, read_only))
            orage_message(40, "add done foreign file=%s", file_name);
        else
            g_warning("add failed foreign file=%s\n", file_name);
#else
        g_warning("Can not do add foreign file without dbus. failed file=%s\n", file_name);
#endif
    }
    else if (!running && initialized) { /* do it self directly */
        if (orage_foreign_file_add(file_name, read_only))
            orage_message(40, "add done foreign file=%s", file_name);
        else
            g_warning("add failed foreign file=%s\n", file_name);
    }
}

static void remove_foreign(gboolean running, char *file_name, gboolean initialized)
{
    if (running && !initialized) {
        /* let's use dbus since server is running there already */
#ifdef HAVE_DBUS
        if (orage_dbus_foreign_remove(file_name))
            orage_message(40, "remove done foreign file=%s", file_name);
        else
            g_warning("remove failed foreign file=%s\n", file_name);
#else
        g_warning("Can not do remove foreign file without dbus. failed file=%s\n", file_name);
#endif
    }
    else if (!running && initialized) { /* do it self directly */
        if (orage_foreign_file_remove(file_name))
            orage_message(40, "remove done foreign file=%s", file_name);
        else
            g_warning("remove failed foreign file=%s\n", file_name);
    }
}

static gboolean process_args(int argc, char *argv[], gboolean running
        , gboolean initialized)
{ 
    int argi;
    gboolean end = FALSE;
    gboolean foreign_file_read_only = TRUE;
    gchar *export_uid_list = NULL;
    gchar *file_name = NULL;

    if (running && argc == 1) { /* no parameters */
        raise_orage();
        return(TRUE);
    }
    end = running;
    for (argi = 1; argi < argc; argi++) {
        if (!strcmp(argv[argi], "--sm-client-id")) {
            argi++; /* skip the parameter also */
        }
        else if (!strcmp(argv[argi], "--version") || 
                 !strcmp(argv[argi], "-v")        ||
                 !strcmp(argv[argi], "-V")) {
            print_version();
            end = TRUE;
        }
        else if (!strcmp(argv[argi], "--help") || 
                 !strcmp(argv[argi], "-h")     ||
                 !strcmp(argv[argi], "-?")) {
            print_help();
            end = TRUE;
        }
        else if (!strcmp(argv[argi], "--preferences") || 
                 !strcmp(argv[argi], "-p")) {
            if (running && !initialized) {
                preferences();
                end = TRUE;
            }
            else if (!running && initialized) {
                preferences();
            }
            /* if (!running && !initialized) Do nothing 
             * if (running && initialized) impossible
             */
        }
        else if (!strcmp(argv[argi], "--toggle") || 
                 !strcmp(argv[argi], "-t")) {
            orage_toggle_visible();
            end = TRUE;
        }
        else if (!strcmp(argv[argi], "--add-foreign") ||
                 !strcmp(argv[argi], "-a")) {
            if (argi+1 >= argc) {
                g_print("\nFile not specified\n\n");
                print_help();
                end = TRUE;
            } 
            else {
                if (argi+2 < argc && (
                    !strcmp(argv[argi+2], "RW") ||
                    !strcmp(argv[argi+2], "READWRITE"))) {
                    foreign_file_read_only = FALSE;
                }
                add_foreign(running, argv[++argi], initialized
                        , foreign_file_read_only);
                if (!foreign_file_read_only)
                    ++argi;
            }
        }
        else if (!strcmp(argv[argi], "--remove-foreign") ||
                 !strcmp(argv[argi], "-r")) {
            if (argi+1 >= argc) {
                g_print("\nFile not specified\n\n");
                print_help();
                end = TRUE;
            } 
            else {
                remove_foreign(running, argv[++argi], initialized);
            }
        }
        else if (!strcmp(argv[argi], "--export") ||
                 !strcmp(argv[argi], "-e")) {
            if (argi+1 >= argc) {
                g_print("\nFile not specified\n\n");
                print_help();
                end = TRUE;
            } 
            else {
                file_name = argv[++argi];
                if (argi+1 < argc) {
                    export_uid_list = argv[++argi];
                }
                export_file(running, file_name, initialized, export_uid_list);
            }
        }
        else if (argv[argi][0] == '-') {
            g_print(_("\nUnknown option %s\n\n"), argv[argi]);
            print_help();
            end = TRUE;
        }
        else {
            import_file(running, argv[argi], initialized);
            raise_orage();
        }
    }
    return(end);
}

static gboolean check_orage_alive(void)
{
    Window xwindow;

    if ((xwindow = XGetSelectionOwner(GDK_DISPLAY()
            , gdk_x11_atom_to_xatom(atom_alive))) != None)
        return(TRUE); /* yes, we got selection owner, so orage must be there */
    else /* no selction owner, so orage is not running yet */
        return(FALSE);
}

static void mark_orage_alive(void)
{
    GtkWidget *hidden;

    hidden = gtk_invisible_new();
    gtk_widget_show(hidden);
    if (!gdk_selection_owner_set(hidden->window, atom_alive
                , gdk_x11_get_server_time(hidden->window), FALSE)) {
        g_error("Unable acquire ownership of selection");
    }
    g_signal_connect(hidden, "client-event"
            , G_CALLBACK(client_message_received), NULL);
}

int main(int argc, char *argv[])
{
    gboolean running, initialized = FALSE;

    /* init i18n = nls to use gettext */
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif
    textdomain(GETTEXT_PACKAGE);

    gtk_init(&argc, &argv);

    atom_alive = gdk_atom_intern("_XFCE_CALENDAR_RUNNING", FALSE);
    running = check_orage_alive();
    if (process_args(argc, argv, running, initialized)) 
        return(EXIT_SUCCESS);
    /* we need to start since orage was not found to be running already */
    mark_orage_alive();

    g_par.xfcal = g_new(CalWin, 1);
    /* Create the main window */
    ((CalWin *)g_par.xfcal)->mWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect((gpointer) ((CalWin *)g_par.xfcal)->mWindow, "delete_event"
            , G_CALLBACK(mWindow_delete_event_cb), (gpointer)g_par.xfcal);

    /* 
    * try to connect to the session manager
    */
    /*
    session_client = client_session_new(argc, argv, NULL
            , SESSION_RESTART_IF_RUNNING, 50);
    session_client->save_yourself = save_yourself_cb;
    session_client->die = die_cb;
    (void)session_init(session_client);
    */

    /*
    * Now it's serious, the application is running, so we create the RC
    * directory and check for config files in old location.
    */
#ifdef HAVE_DBUS
    orage_dbus_start();
#endif

    /*
    * Create the orage.
    */
    read_parameters();
    build_mainWin();
    set_parameters();
    if (g_par.start_visible) {
        gtk_widget_show(((CalWin *)g_par.xfcal)->mWindow);
    }
    else if (g_par.start_minimized) {
        gtk_window_iconify(GTK_WINDOW(((CalWin *)g_par.xfcal)->mWindow));
        gtk_widget_show(((CalWin *)g_par.xfcal)->mWindow);
    }
    else { /* hidden */
        gtk_widget_realize(((CalWin *)g_par.xfcal)->mWindow);
        gtk_widget_hide(((CalWin *)g_par.xfcal)->mWindow);
    }
    alarm_read();
    orage_day_change(NULL); /* first day change after we start */
    mCalendar_month_changed_cb(
            (GtkCalendar *)((CalWin *)g_par.xfcal)->mCalendar, NULL);

    /* start monitoring foreign file updates if we have foreign files */
    if (g_par.foreign_count)
        g_timeout_add_seconds(30, (GtkFunction)orage_foreign_files_check, NULL);

    /* let's check if I got filename as a parameter */
    initialized = TRUE;
    process_args(argc, argv, running, initialized);

#ifdef HAVE_DBUS
    /* day change after resuming */
    handle_resuming();
#endif

    gtk_main();
    keep_tidy();
    return(EXIT_SUCCESS);
}
