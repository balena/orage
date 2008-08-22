/*  
 *  Global Time - Set of clocks showing time in different parts of world.
 *  Copyright 2006 Juha Kautto (kautto.juha@kolumbus.fi)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  To get a copy of the GNU General Public License write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include "globaltime.h"


#define CONFIG_DATA_DIR_NAME  "globaltime" G_DIR_SEPARATOR_S
#define CONFIG_DATA_FILE_NAME "globaltimerc"
#define WIN_HEADING_SIZE 30


extern global_times_struct clocks;

static void write_string(XfceRc *rc, gchar *prop, GString *string)
{
    if (string && string->len)
        xfce_rc_write_entry(rc, prop, string->str);
}

static void write_color(XfceRc *rc, gchar *prop, GdkColor *color)
{
    gchar tmp[100];

    if (color) {
        sprintf(tmp, "%uR %uG %uB", color->red, color->green, color->blue);
        xfce_rc_write_entry(rc, prop, tmp);
    }
}

static void write_attr(XfceRc *rc, text_attr_struct *attr)
{
    if (attr->clock_fg_modified)
        write_color(rc, "fg", attr->clock_fg);
    if (attr->clock_bg_modified)
        write_color(rc, "bg", attr->clock_bg);

    if (attr->name_font_modified)
        write_string(rc, "name_font", attr->name_font);
    if (attr->name_underline_modified)
        write_string(rc, "name_underline", attr->name_underline);

    if (attr->time_font_modified)
        write_string(rc, "time_font", attr->time_font);
    if (attr->time_underline_modified)
        write_string(rc, "time_underline", attr->time_underline);
}

static void write_clock(clock_struct *clockp, XfceRc *rc)
{
    xfce_rc_set_group(rc, clockp->name->str);
    write_string(rc, "name", clockp->name);
    write_string(rc, "tz", clockp->tz);
    write_attr(rc, &clockp->clock_attr);
}

void write_file(void)
{
    gchar *fpath;
    XfceRc *rc;

    fpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG
            , CONFIG_DATA_DIR_NAME CONFIG_DATA_FILE_NAME, TRUE);
    unlink(fpath);
    if ((rc = xfce_rc_simple_open(fpath, FALSE)) == NULL) {
        g_warning("Unable to open RC file.");
        return;
    }
    g_free(fpath);
    gtk_window_get_position(GTK_WINDOW(clocks.window), &clocks.x, &clocks.y);
    xfce_rc_set_group(rc, "Default Values");
    xfce_rc_write_int_entry(rc, "X-pos", clocks.x);
    xfce_rc_write_int_entry(rc, "Y-pos", clocks.y);
    xfce_rc_write_int_entry(rc, "Decorations", clocks.decorations);
    xfce_rc_write_int_entry(rc, "Expand", clocks.expand);
    write_string(rc, "tz", clocks.local_tz);
    write_attr(rc, &clocks.clock_default_attr);

    g_list_foreach(clocks.clock_list, (GFunc) write_clock, rc);
    xfce_rc_close(rc);
}

static gboolean read_string(XfceRc *rc, gchar *prop, GString *result)
{
    gboolean found = FALSE;

    if (xfce_rc_has_entry(rc, prop)) {
        result = g_string_assign(result, xfce_rc_read_entry(rc, prop, ""));
        found = TRUE;
    }
    return(found);
}

static gboolean read_color(XfceRc *rc, gchar *prop, GdkColor **result)
{
    gchar *tmp;
    gboolean found = FALSE;
    unsigned int red, green, blue;

    if (xfce_rc_has_entry(rc, prop)) {
        tmp = (gchar *)xfce_rc_read_entry(rc, prop, "");
        /*
        sscanf(tmp, "%uR %uG %uB", &(*result)->red, &(*result)->green
                , &(*result)->blue);
                */
        sscanf(tmp, "%uR %uG %uB", &red, &green, &blue);
        (*result)->red = red;
        (*result)->green = green;
        (*result)->blue = blue;
        (*result)->pixel = 0;
        found = TRUE;
    }
    return(found);
}

static void read_attr(XfceRc *rc, text_attr_struct *attr)
{
    attr->clock_fg_modified = read_color(rc, "fg", &attr->clock_fg);
    attr->clock_bg_modified = read_color(rc, "bg", &attr->clock_bg);
    attr->name_font_modified = 
            read_string(rc, "name_font", attr->name_font);
    attr->name_underline_modified = 
            read_string(rc, "name_underline", attr->name_underline);
    attr->time_font_modified = 
            read_string(rc, "time_font", attr->time_font);
    attr->time_underline_modified = 
            read_string(rc, "time_underline", attr->time_underline);
}

static void read_clock(XfceRc *rc)
{
    clock_struct *clockp;

    clockp = g_new0(clock_struct, 1);
    clockp->name = g_string_new(
            xfce_rc_read_entry(rc, "name", "no name"));
    clockp->tz = g_string_new(
            xfce_rc_read_entry(rc, "tz", "/etc/localtime"));
    clockp->modified = FALSE;

    init_attr(&clockp->clock_attr);
    read_attr(rc, &clockp->clock_attr);

    clocks.clock_list = g_list_append(clocks.clock_list, clockp);
}

void read_file(void)
{
    gchar *fpath;
    gchar **groups;
    XfceRc *rc;
    gint i;

    fpath = xfce_resource_save_location(XFCE_RESOURCE_CONFIG
            , CONFIG_DATA_DIR_NAME CONFIG_DATA_FILE_NAME, TRUE);

    if ((rc = xfce_rc_simple_open(fpath, TRUE)) == NULL) {
        g_warning("Unable to open (read) RC file.");
        /* let's try to build it */
        if ((rc = xfce_rc_simple_open(fpath, FALSE)) == NULL) {
            /* still failed, can't do more */
            g_warning("Unable to open (write) RC file.");
            return;
        }
    }
    g_free(fpath);

    /* read first default values without group name */
    xfce_rc_set_group(rc, "Default Values");
    clocks.x = xfce_rc_read_int_entry(rc, "X-pos", 0);
    clocks.y = xfce_rc_read_int_entry(rc, "Y-pos", 0);
    clocks.decorations = xfce_rc_read_int_entry(rc, "Decorations", 1);
    clocks.expand = xfce_rc_read_int_entry(rc, "Expand", 0);
    clocks.local_tz = g_string_new(
            xfce_rc_read_entry(rc, "tz", "/etc/localtime"));
    read_attr(rc, &clocks.clock_default_attr);

    /* then clocks */
    groups = xfce_rc_get_groups(rc);
    for (i = 0; groups[i] != NULL; i++) {
        if ((strcmp(groups[i], "[NULL]") != 0)
        &&  (strcmp(groups[i], "Default Values") != 0)) {
            xfce_rc_set_group(rc, groups[i]);
            read_clock(rc);
        }
    }

    g_strfreev(groups);
    xfce_rc_close(rc);
}

