/* xfcalendar
 *
 * Copyright (C) 2002 Mickael Graf (korbinus@linux.se)
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  This program is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.  You
 * should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

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

#include <libxfce4util/util.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

static GtkWidget *appointment;
static GtkWidget *info;
static GtkWidget *clearwarn;

void
on_quit1_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_main_quit();
}

void
on_about1_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	info = create_wInfo();
	gtk_widget_show(info);
}

gboolean
on_XFCalendar_delete_event(GtkWidget *widget, GdkEvent *event,
                           gpointer user_data)
{
	gtk_main_quit();
	return(FALSE);
}

void
on_calendar1_day_selected_double_click (GtkCalendar *calendar,
                                        gpointer user_data)
{
	guint year, month, day;
	char title[12], text[4096];
	gchar *fpath;
	FILE *fapp;
	GtkTextView *tv;
	GtkTextBuffer *tb = gtk_text_buffer_new(NULL);
	GtkTextIter *end;
	GtkTextIter ctl_start, ctl_end;
	char *ctl_text;
  
	gtk_calendar_get_date(calendar, &year, &month, &day);
	g_snprintf(title, 12, "%d-%02d-%02d", year, ++month, day);
	fpath = xfce_get_userfile("xfcalendar", title, NULL);
	appointment = create_wAppointment();

	tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(appointment),"textview1"));

	if ((fapp = fopen(fpath, "r")) != NULL) {
		end = g_new0(GtkTextIter, 1);
		
		do {
			fgets(text, 4096, fapp);
			g_print("Appointment content: %s\n", text);
			gtk_text_buffer_get_end_iter(tb, end);
			gtk_text_buffer_insert(tb, end, text, strlen(text));
		} while (!feof(fapp));
		
		fclose(fapp);
		
		gtk_window_set_title (GTK_WINDOW (appointment), _(title));
		gtk_text_buffer_get_bounds(tb, &ctl_start, &ctl_end);
		ctl_text = gtk_text_iter_get_text(&ctl_start, &ctl_end);
		g_print("Content from GtkTextBuffer: %s\n", ctl_text);
	}
	else {
		g_warning("Cannot open file %s.\n", fpath);
	}
	
	gtk_text_view_set_buffer(tv, tb);
	gtk_window_set_title (GTK_WINDOW (appointment), _(title));
	gtk_widget_show(appointment);

	g_free(fpath);
}

void
on_btClose_clicked(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(appointment); /* destroy the appointment window */
}

/*
 * Glade put a gboolean here, causing problems, but I just want to hide the
 * window, so I changed it to void. <- FIXME: Thats not a solution!
 */
void 
on_wAppointment_delete_event(GtkWidget *widget, GdkEvent *event,
                             gpointer user_data)
{
	gtk_widget_destroy(appointment); /* destroy the appointment window */
}

void
on_btSave_clicked(GtkButton *button, gpointer user_data)
{
	gchar *fpath;
	FILE *fapp;
	GtkTextView *tv;
	GtkTextBuffer *tb;
	GtkTextIter start, end;
	char *text;
	G_CONST_RETURN gchar *title;
	
	tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(appointment),"textview1"));
	tb = gtk_text_view_get_buffer(tv);
	gtk_text_buffer_get_bounds(tb, &start, &end);
	text = gtk_text_iter_get_text(&start, &end);
	g_print("Appointment content: %s\n", text);
	
	title = gtk_window_get_title(GTK_WINDOW(appointment));
	g_print("Date created: %s\n", title);
	
	fpath = xfce_get_userfile("xfcalendar", title, NULL);
	
	if (gtk_text_buffer_get_modified(tb) != NULL) {
		if ((fapp = fopen(fpath, "w")) == NULL) {
			g_warning("Cannot open file %s\n", fpath);
		}
		else {
			fputs(text, fapp);
			fclose(fapp);
			gtk_text_buffer_set_modified(tb, FALSE);
		}
	}

	g_free(fpath);

#ifdef DEBUG 
	g_print("Procedure on_btSave_clicked finished\n");
#endif
}

void
on_okbutton1_clicked(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(info);
}

void
on_wInfo_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	gtk_widget_destroy(info);
}


void
on_btDelete_clicked(GtkButton *button, gpointer user_data)
{
	clearwarn = create_wClearWarn();
	gtk_widget_show(clearwarn);
}

void
on_cancelbutton1_clicked(GtkButton *button, gpointer user_data)
{
	g_print("Clear textbuffer not chosen (pffiou!)\n");
	gtk_widget_destroy(clearwarn);
}


void
on_okbutton2_clicked(GtkButton *button, gpointer user_data)
{
	gchar *fpath;
	GtkTextView *tv;
	GtkTextBuffer *tb;
	GtkTextIter start, end;
	G_CONST_RETURN gchar *title;
	
	gtk_widget_destroy(clearwarn);
	
	g_print("Clear textbuffer chosen (oops!)\n");

	title = gtk_window_get_title(GTK_WINDOW(appointment));
	tv = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(appointment),"textview1"));
	tb = gtk_text_view_get_buffer(tv);
	gtk_text_buffer_get_bounds(tb, &start, &end);

	gtk_text_buffer_delete(tb, &start, &end);
	gtk_text_buffer_set_modified(tb, FALSE);
	g_print("Text buffer empty\n");

	fpath = xfce_get_userfile("xfcalendar", title, NULL);

	if (remove(fpath) < 0 ) {
		g_warning("Unable to remove file %s: %s", fpath,
				g_strerror(errno));
	}
#ifdef DEBUG
	else {
		g_print("File %s removed\n", fpath);
	}
#endif

	g_free(fpath);
}
