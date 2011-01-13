/*  
 *  Global Time - Set of clocks showing time in different parts of world.
 *  Copyright 2006-2011 Juha Kautto (kautto.juha@kolumbus.fi)
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
#include "globaltime.h"
#include "../src/functions.h"


#define CONFIG_DIR_NAME  "globaltime" G_DIR_SEPARATOR_S
#define CONFIG_FILE_NAME "globaltimerc"
#define CONFIG_DIR_FILE_NAME CONFIG_DIR_NAME CONFIG_FILE_NAME
#define WIN_HEADING_SIZE 30


extern global_times_struct clocks;

static void write_string(OrageRc *rc, gchar *prop, GString *string)
{
    if (string && string->len)
        orage_rc_put_str(rc, prop, string->str);
}

static void write_color(OrageRc *rc, gchar *prop, GdkColor *color)
{
    gchar tmp[100];

    if (color) {
        sprintf(tmp, "%uR %uG %uB", color->red, color->green, color->blue);
        orage_rc_put_str(rc, prop, tmp);
    }
}

static void write_attr(OrageRc *rc, text_attr_struct *attr)
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

static void write_clock(clock_struct *clockp, OrageRc *rc)
{
    orage_rc_set_group(rc, clockp->name->str);
    write_string(rc, "name", clockp->name);
    write_string(rc, "tz", clockp->tz);
    write_attr(rc, &clockp->clock_attr);
}

void write_file(void)
{
    gchar *fpath;
    OrageRc *rc;

    fpath = orage_config_file_location(CONFIG_DIR_FILE_NAME);
    unlink(fpath);
    if ((rc = orage_rc_file_open(fpath, FALSE)) == NULL) {
        g_warning("Unable to open RC file.");
        return;
    }
    g_free(fpath);
    gtk_window_get_position(GTK_WINDOW(clocks.window), &clocks.x, &clocks.y);
    orage_rc_set_group(rc, "Default Values");
    orage_rc_put_int(rc, "X-pos", clocks.x);
    orage_rc_put_int(rc, "Y-pos", clocks.y);
    orage_rc_put_int(rc, "Decorations", clocks.decorations);
    orage_rc_put_int(rc, "Expand", clocks.expand);
    write_string(rc, "tz", clocks.local_tz);
    write_attr(rc, &clocks.clock_default_attr);

    g_list_foreach(clocks.clock_list, (GFunc) write_clock, rc);
    orage_rc_file_close(rc);
}

static gboolean read_string(OrageRc *rc, gchar *prop, GString *result)
{
    gboolean found = FALSE;

    if (orage_rc_exists_item(rc, prop)) {
        result = g_string_assign(result, orage_rc_get_str(rc, prop, ""));
        found = TRUE;
    }
    return(found);
}

static gboolean read_color(OrageRc *rc, gchar *prop, GdkColor **result)
{
    gchar *tmp;
    gboolean found = FALSE;
    unsigned int red, green, blue;

    if (orage_rc_exists_item(rc, prop)) {
        tmp = (gchar *)orage_rc_get_str(rc, prop, "");
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

static void read_attr(OrageRc *rc, text_attr_struct *attr)
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

static void read_clock(OrageRc *rc)
{
    clock_struct *clockp;

    clockp = g_new0(clock_struct, 1);
    clockp->name = g_string_new(
            orage_rc_get_str(rc, "name", "no name"));
    clockp->tz = g_string_new(
            orage_rc_get_str(rc, "tz", "/etc/localtime"));
    clockp->modified = FALSE;

    init_attr(&clockp->clock_attr);
    read_attr(rc, &clockp->clock_attr);

    clocks.clock_list = g_list_append(clocks.clock_list, clockp);
}

void read_file(void)
{
    gchar *fpath;
    gchar **groups;
    OrageRc *rc;
    gint i;

    fpath = orage_config_file_location(CONFIG_DIR_FILE_NAME);

    if ((rc = orage_rc_file_open(fpath, TRUE)) == NULL) {
        g_warning("Unable to open RC file.");
        return;
    }
    g_free(fpath);

    /* read first default values without group name */
    orage_rc_set_group(rc, "Default Values");
    clocks.x = orage_rc_get_int(rc, "X-pos", 0);
    clocks.y = orage_rc_get_int(rc, "Y-pos", 0);
    clocks.decorations = orage_rc_get_int(rc, "Decorations", 1);
    clocks.expand = orage_rc_get_int(rc, "Expand", 0);
    clocks.local_tz = g_string_new(
            orage_rc_get_str(rc, "tz", "/etc/localtime"));
    read_attr(rc, &clocks.clock_default_attr);

    /* then clocks */
    groups = orage_rc_get_groups(rc);
    for (i = 0; groups[i] != NULL; i++) {
        if ((strcmp(groups[i], "[NULL]") != 0)
        &&  (strcmp(groups[i], "Default Values") != 0)) {
            orage_rc_set_group(rc, groups[i]);
            read_clock(rc);
        }
    }

    g_strfreev(groups);
    orage_rc_file_close(rc);
}

