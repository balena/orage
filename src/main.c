/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2007 Juha Kautto  (juha at xfce.org)
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

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfcegui4/netk-trayicon.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#define ORAGE_MAIN  "orage"

#include "functions.h"
#include "mainbox.h"
#include "reminder.h"
#include "ical-code.h"
#include "tray_icon.h"
#include "parameters.h"
#include "orage-dbus.h"


/* session client handler */
static SessionClient	*session_client = NULL;
static GdkAtom atom_alive;

/*
void program_log (const char *format, ...)
{
        va_list args;
        char *formatted, *str;

        va_start (args, format);
        formatted = g_strdup_vprintf (format, args);
        va_end (args);

        str = g_strdup_printf ("MARK: %s: %s", g_get_prgname(), formatted);
        g_free (formatted);

        access (str, F_OK);
        g_free (str);
} 
*/

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

void orage_toggle_visible()
{
    send_event("_XFCE_CALENDAR_TOGGLE_HERE");
}

void raise_orage()
{
    send_event("_XFCE_CALENDAR_RAISE");
}

gboolean mWindow_delete_event_cb(GtkWidget *widget, GdkEvent *event
        , gpointer user_data)
{
    orage_toggle_visible();
    return(TRUE);
}

static void raise_window()
{
    GdkScreen *screen;
    GdkWindow *window;

    screen = xfce_gdk_display_locate_monitor_with_pointer(NULL, NULL);
    gtk_window_set_screen(GTK_WINDOW(g_par.xfcal->mWindow)
            , screen ? screen : gdk_screen_get_default());
    if (g_par.pos_x || g_par.pos_y)
        gtk_window_move(GTK_WINDOW(g_par.xfcal->mWindow)
                , g_par.pos_x, g_par.pos_y);
    if (g_par.select_always_today)
        orage_select_today(GTK_CALENDAR(g_par.xfcal->mCalendar));
    if (g_par.set_stick)
        gtk_window_stick(GTK_WINDOW(g_par.xfcal->mWindow));
    gtk_window_set_keep_above(GTK_WINDOW(g_par.xfcal->mWindow)
            , g_par.set_ontop);
    window = GTK_WIDGET(g_par.xfcal->mWindow)->window;
    gdk_x11_window_set_user_time(window, gdk_x11_get_server_time(window));
    gtk_window_present(GTK_WINDOW(g_par.xfcal->mWindow));
}

static gboolean client_message_received(GtkWidget *widget
        , GdkEventClient *event, gpointer user_data)
{
    if (event->message_type ==
            gdk_atom_intern("_XFCE_CALENDAR_RAISE", FALSE)) {
        raise_window();
        return TRUE;
    }
    else if (event->message_type ==
            gdk_atom_intern("_XFCE_CALENDAR_TOGGLE_HERE", FALSE)) {
        if (GTK_WIDGET_VISIBLE(g_par.xfcal->mWindow)) {
            write_parameters();
            gtk_widget_hide(g_par.xfcal->mWindow);
            return TRUE;
        }
        else {
            raise_window();
            return TRUE;
        }
    }
    else if (event->message_type ==
            gdk_atom_intern("_XFCE_CALENDAR_PREFERENCES", FALSE)) {
        show_parameters();
        return TRUE;
    }

    return FALSE;
}

gboolean keep_tidy(void)
{
    /* move old appointment to other file to keep the active
       calendar file smaller and faster */
    xfical_archive();
    return TRUE;
}

/*
 * SaveYourself callback
 *
 * This is called when the session manager requests the client to save its
 * state.
 */
void save_yourself_cb(gpointer data, int save_style, gboolean shutdown
        , int interact_style, gboolean fast)
{
    write_parameters();
}

/*
 * Die callback
 *
 * This is called when the session manager requests the client to go down.
 */
void die_cb(gpointer data)
{
    gtk_main_quit();
}

static void ensure_basedir_spec(void)
{
    char *newdir, *olddir;
    GError *error = NULL;
    GDir *gdir;

    newdir = xfce_resource_save_location(XFCE_RESOURCE_CONFIG, ORAGE_DIR, FALSE);

  /* if new directory exist, assume old config has been copied */
    if (g_file_test(newdir, G_FILE_TEST_IS_DIR)) {
        g_free(newdir);
        return;
    }

    if (!xfce_mkdirhier(newdir, 0700, &error)) {
        g_critical("Cannot create directory %s: %s", newdir, error->message);
        g_error_free(error);
        g_free(newdir);
        exit(EXIT_FAILURE);
    }

    olddir = xfce_get_userfile("xfcalendar", NULL);

    if ((gdir = g_dir_open(olddir, 0, NULL)) != NULL) {
        const char *name;

        while ((name = g_dir_read_name(gdir)) != NULL) {
            FILE *r, *w;
            char *path;

            path = g_build_filename(olddir, name, NULL);
            r = fopen(path, "r");
            g_free(path);

            path = g_build_filename(newdir, name, NULL);
            w = fopen(path, "w");
            g_free(path);

            if (r && w) {
                char c;

                while ((c = getc(r)) != EOF)
                    putc (c, w);
            }

            if (r)
                fclose(r);
            if (w)
                fclose(w);
        }
    }

    g_free(newdir);
    g_free(olddir);
}

static void print_version(void)
{
    g_print("\tThis is %s version %s for Xfce %s\n\n"
            , PACKAGE, VERSION, xfce_version_string());
    g_print("\tReleased under the terms of the GNU General Public License.\n");
    g_print("\tCompiled against GTK+-%d.%d.%d, "
            , GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
    g_print("using GTK+-%d.%d.%d.\n"
            , gtk_major_version, gtk_minor_version, gtk_micro_version);
    g_print("\n");
}

static void preferences(void)
{
    send_event("_XFCE_CALENDAR_RAISE");
    send_event("_XFCE_CALENDAR_PREFERENCES");
}

static void print_help(void)
{
    g_print("Usage: orage [options] files\n\n");
    g_print("Options:\n");
    g_print("--version (-v) \t\tshow version of orage\n");
    g_print("--help (-h) \t\tprint this text\n");
    g_print("--preferences (-p) \tshow preferences form\n");
    g_print("--toggle (-t) \t\tmake orage visible/unvisible\n");
    g_print("\n");
    g_print("files=ical files to load into orage\n");
    g_print("\n");
}

static void import_file(gboolean running, char *file_name, gboolean initialized)
{
    if (running && !initialized) 
        /* let's use dbus since server is running there already */
        if (orage_dbus_import_file(file_name))
            g_message("import done file=%s", file_name);
        else
            g_warning("import failed file=%s\n", file_name);
    else if (!running && initialized) /* do it self directly */
        if (xfical_import_file(file_name))
            g_message("import done file=%s", file_name);
        else
            g_warning("import failed file=%s\n", file_name);
}

static gboolean process_args(int argc, char *argv[], gboolean running
        , gboolean initialized)
{ /* we need dbus if orage is already running and we just want to add file */
    int argi;
    gboolean end = FALSE;

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
                 !strcmp(argv[argi], "-v")) {
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
        else if (argv[argi][0] == '-') {
            g_print(_("\nUnknown parameter %s\n\n"), argv[argi]);
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

static gboolean check_orage_alive()
{
    Window xwindow;

    if ((xwindow = XGetSelectionOwner(GDK_DISPLAY()
            , gdk_x11_atom_to_xatom(atom_alive))) != None)
        return(TRUE); /* yes, we got selection owner, so orage must be there */
    else /* no selction owner, so orage is not running yet */
        return(FALSE);
}

static gboolean mark_orage_alive()
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

    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    gtk_init(&argc, &argv);

    atom_alive = gdk_atom_intern("_XFCE_CALENDAR_RUNNING", FALSE);
    running = check_orage_alive();
    if (process_args(argc, argv, running, initialized)) 
        return(EXIT_SUCCESS);
    /* we need to start since orage was not found to be running already */
    mark_orage_alive();

    g_par.xfcal = g_new(CalWin, 1);
    /* Create the main window */
    g_par.xfcal->mWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect((gpointer) g_par.xfcal->mWindow, "delete_event"
            , G_CALLBACK(mWindow_delete_event_cb), (gpointer)g_par.xfcal);

    /* 
    * try to connect to the session manager
    */
    session_client = client_session_new(argc, argv, NULL
            , SESSION_RESTART_IF_RUNNING, 50);
    session_client->save_yourself = save_yourself_cb;
    session_client->die = die_cb;
    (void)session_init(session_client);

    /*
    * Now it's serious, the application is running, so we create the RC
    * directory and check for config files in old location.
    */
    ensure_basedir_spec();
    orage_dbus_start();

    /*
    * Create the orage.
    */
    read_parameters();
    build_mainWin();
    g_par.trayIcon = create_TrayIcon(g_par.xfcal);
    set_parameters();
    if (g_par.start_visible)
        gtk_widget_show(g_par.xfcal->mWindow);
    else if (g_par.start_minimized) {
        gtk_window_iconify(GTK_WINDOW(g_par.xfcal->mWindow));
        gtk_widget_show(g_par.xfcal->mWindow);
    }
    else { /* hidden */
        gtk_widget_realize(g_par.xfcal->mWindow);
        gtk_widget_hide(g_par.xfcal->mWindow);
    }
    orage_mark_appointments();

    /* start alarm monitoring timeout */
    g_timeout_add_full(0, 1000, (GtkFunction) orage_alarm_clock
            , (gpointer) g_par.xfcal, NULL);
                                                        
    /* let's check if I got filename as a parameter */
    initialized = TRUE;
    process_args(argc, argv, running, initialized);

    gtk_main();
    keep_tidy();
    return(EXIT_SUCCESS);
}
