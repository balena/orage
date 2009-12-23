/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2008 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2003-2005 Mickael Graf (korbinus at xfce.org)
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

#include <unistd.h>
#include <time.h>
#include <math.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <ical.h>
#include <icalss.h>

#include "orage-i18n.h"
#include "functions.h"
#include "mainbox.h"
#include "reminder.h"
#include "ical-code.h"
#include "event-list.h"
#include "appointment.h"
#include "parameters.h"
#include "interface.h"

/*
#define ORAGE_DEBUG 1
*/



/* in ical-code.c: */
char *ic_generate_uid();

extern icalset *ic_fical;
extern icalcomponent *ic_ical;

extern gboolean ic_file_modified; /* has any ical file been changed */

typedef struct _foreign_ical_files
{;
    icalset *fical;
    icalcomponent *ical;
} ic_foreign_ical_files;

extern ic_foreign_ical_files ic_f_ical[10];

gboolean ic_internal_file_open(icalcomponent **p_ical
                , icalset **p_fical, gchar *file_icalpath, gboolean test);

static gboolean add_event(icalcomponent *c)
{
#undef P_N
#define P_N "add_event: "
    icalcomponent *ca = NULL;
    char *uid;

#ifdef ORAGE_DEBUG
    orage_message(-300, P_N);
#endif
    ca = icalcomponent_new_clone(c);
    if ((uid = (char *)icalcomponent_get_uid(ca)) == NULL) {
        uid = ic_generate_uid();
        icalcomponent_add_property(ca,  icalproperty_new_uid(uid));
        orage_message(15, "Generated UID %s", uid);
        g_free(uid);

    }
    if (!xfical_file_open(FALSE)) {
        orage_message(250, P_N "ical file open failed");
        return(FALSE);
    }
    icalcomponent_add_component(ic_ical, ca);
    ic_file_modified = TRUE;
    icalset_mark(ic_fical);
    icalset_commit(ic_fical);
    xfical_file_close(FALSE);
    return(TRUE);
}

/* pre process the file to rule out some features, which orage does not
 * support so that we can do better conversion 
 */
static gboolean pre_format(char *file_name_in, char *file_name_out)
{
#undef P_N
#define P_N "pre_format: "
    gchar *text, *tmp, *tmp2, *tmp3;
    gsize text_len;
    GError *error = NULL;

#ifdef ORAGE_DEBUG
    orage_message(-200, P_N);
#endif
    orage_message(15, _("Starting import file preprocessing"));
    if (!g_file_get_contents(file_name_in, &text, &text_len, &error)) {
        orage_message(250, P_N "Could not open ical file (%s) error:%s"
                , file_name_in, error->message);
        g_error_free(error);
        return(FALSE);
    }
    /***** Check utf8 coformtability *****/
    if (!g_utf8_validate(text, -1, NULL)) {
        orage_message(250, P_N "is not in utf8 format. Conversion needed.\n (Use iconv and convert it into UTF-8 and import it again.)\n");
        return(FALSE);
        /* we don't know the real characterset, so we can't convert
        tmp = g_locale_to_utf8(text, -1, NULL, &cnt, NULL);
        if (tmp) {
            g_strlcpy(text, tmp, text_len);
            g_free(tmp);
        }*/
    }

    /***** 1: change DCREATED to CREATED *****/
    for (tmp = g_strstr_len(text, text_len, "DCREATED:"); 
         tmp != NULL;
         tmp = g_strstr_len(tmp, strlen(tmp), "DCREATED:")) {
        tmp2 = tmp+strlen("DCREATED:yyyymmddThhmmss");
        if (*tmp2 == 'Z') {
            /* it is already in UTC, so just fix the parameter name */
            *tmp = ' ';
        }
        else {
            /* needs to be converted to UTC also */
            for (tmp3 = tmp; tmp3 < tmp2; tmp3++) {
                *tmp3 = *(tmp3+1); 
            }
            *(tmp3-1) = 'Z'; /* this is 'bad'...but who cares...it is fast */
        }
        orage_message(15, _("... Patched DCREATED to be CREATED."));
    }

    /***** 2: change absolute timezones into libical format *****/
    /* At least evolution uses absolute timezones.
     * We assume format has /xxx/xxx/timezone and we should remove the
     * extra /xxx/xxx/ from it */
    for (tmp = g_strstr_len(text, text_len, ";TZID=/"); 
         tmp != NULL;
         tmp = g_strstr_len(tmp, strlen(tmp), ";TZID=/")) {
        /* tmp = original TZID start
         * tmp2 = end of line 
         * tmp3 = current search and eventually the real tzid */
        tmp = tmp+6; /* 6 = skip ";TZID=" */
        if (!(tmp2 = g_strstr_len(tmp, 100, "\n"))) { /* no end of line */
            orage_message(150, P_N "timezone patch failed 1. no end-of-line found: %s", tmp);
            continue;
        }
        tmp3 = tmp;

        tmp3++; /* skip '/' */
        if (!(tmp3 = g_strstr_len(tmp3, tmp2-tmp3, "/"))) { /* no more '/' */
            orage_message(150, P_N "timezone patch failed 2. no / found: %s", tmp);
            continue;
        }
        tmp3++; /* skip '/' */
        if (!(tmp3 = g_strstr_len(tmp3, tmp2-tmp3, "/"))) { /* no more '/' */
            orage_message(150, P_N "timezone patch failed 3. no / found: %s", tmp);
            continue;
        }

        /* we found the real tzid since we came here */
        tmp3++; /* skip '/' */
        /* move the real tzid (=tmp3) to the beginning (=tmp) */
        for (; tmp3 < tmp2; tmp3++, tmp++)
            *tmp = *tmp3;
        /* fill the end of the line with spaces */
        for (; tmp < tmp2; tmp++)
            *tmp = ' ';
        orage_message(15, _("... Patched timezone to Orage format."));
    }

    /***** All done: write file *****/
    if (!g_file_set_contents(file_name_out, text, -1, NULL)) {
        orage_message(250, P_N "Could not write ical file (%s)", file_name_out);
        return(FALSE);
    }
    g_free(text);
    orage_message(15, _("Import file preprocessing done"));
    return(TRUE);
}

gboolean xfical_import_file(char *file_name)
{
#undef P_N
#define P_N "xfical_import_file: "
    icalset *file_ical = NULL;
    char *ical_file_name = NULL;
    icalcomponent *c1, *c2;
    int cnt1 = 0, cnt2 = 0;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    ical_file_name = g_strdup_printf("%s.orage", file_name);
    if (!pre_format(file_name, ical_file_name)) {
        return(FALSE);
    }
    if ((file_ical = icalset_new_file(ical_file_name)) == NULL) {
        orage_message(250, P_N "Could not open ical file (%s) %s"
                , ical_file_name, icalerror_strerror(icalerrno));
        return(FALSE);
    }
    for (c1 = icalset_get_first_component(file_ical);
         c1 != 0;
         c1 = icalset_get_next_component(file_ical)) {
        if (icalcomponent_isa(c1) == ICAL_VCALENDAR_COMPONENT) {
            cnt1++;
            for (c2=icalcomponent_get_first_component(c1, ICAL_ANY_COMPONENT);
                 c2 != 0;
                 c2=icalcomponent_get_next_component(c1, ICAL_ANY_COMPONENT)){
                if ((icalcomponent_isa(c2) == ICAL_VEVENT_COMPONENT)
                ||  (icalcomponent_isa(c2) == ICAL_VTODO_COMPONENT)
                ||  (icalcomponent_isa(c2) == ICAL_VJOURNAL_COMPONENT)) {
                    cnt2++;
                    add_event(c2);
                }
                /* we ignore TIMEZONE component; Orage only uses internal
                 * timezones from libical */
                else if (icalcomponent_isa(c2) != ICAL_VTIMEZONE_COMPONENT)
                    orage_message(140, P_N "unknown component %s %s"
                            , icalcomponent_kind_to_string(
                                    icalcomponent_isa(c2))
                            , ical_file_name);
            }

        }
        else
            orage_message(140, P_N "unknown icalset component %s in %s"
                    , icalcomponent_kind_to_string(icalcomponent_isa(c1))
                    , ical_file_name);
    }
    if (cnt1 == 0) {
        orage_message(150, P_N "No valid icalset components found");
        return(FALSE);
    }
    if (cnt2 == 0) {
        orage_message(150, P_N "No valid ical components found");
        return(FALSE);
    }

    g_free(ical_file_name);
    return(TRUE);
}

static gboolean export_prepare_write_file(char *file_name)
{
#undef P_N
#define P_N "export_prepare_write_file: "
    gchar *tmp;

#ifdef ORAGE_DEBUG
    orage_message(-200, P_N);
#endif
    if (strcmp(file_name, g_par.orage_file) == 0) {
        orage_message(150, P_N "You do not want to overwrite Orage ical file! (%s)"
                , file_name);
        return(FALSE);
    }
    tmp = g_path_get_dirname(file_name);
    if (g_mkdir_with_parents(tmp, 0755)) { /* octal */
        orage_message(250, P_N "Could not create directories (%s)"
                , file_name);
        return(FALSE);
    }
    g_free(tmp);
    if (g_remove(file_name) == -1) { /* note that it may not exist */
        orage_message(150, P_N "Failed to remove export file %s", file_name);
    }
    return(TRUE);
}

static gboolean export_all(char *file_name)
{
#undef P_N
#define P_N "export_all: "
    gchar *text;
    gsize text_len;

#ifdef ORAGE_DEBUG
    orage_message(-200, P_N);
#endif
    if (!export_prepare_write_file(file_name))
        return(FALSE);
    /* read Orage's ical file */
    if (!g_file_get_contents(g_par.orage_file, &text, &text_len, NULL)) {
        orage_message(250, P_N "Could not open Orage ical file (%s)"
                , g_par.orage_file);
        return(FALSE);
    }
    if (!g_file_set_contents(file_name, text, -1, NULL)) {
        orage_message(150, P_N "Could not write file (%s)"
                , file_name);
        g_free(text);
        return(FALSE);
    }
    g_free(text);
    return(TRUE);
}

static void export_selected_uid(icalcomponent *base, gchar *uid_int
        , icalcomponent *x_ical)
{
#undef P_N
#define P_N "export_selected_uid: "
    icalcomponent *c, *d;
    gboolean key_found = FALSE;
    gchar *uid_ical;

#ifdef ORAGE_DEBUG
    orage_message(-200, P_N);
#endif
    key_found = FALSE;
    for (c = icalcomponent_get_first_component(base, ICAL_ANY_COMPONENT);
         c != 0 && !key_found;
         c = icalcomponent_get_next_component(base, ICAL_ANY_COMPONENT)) {
        uid_ical = (char *)icalcomponent_get_uid(c);
        if (strcmp(uid_int, uid_ical) == 0) { /* we found our uid */
            d = icalcomponent_new_clone(c);
            icalcomponent_add_component(x_ical, d);
            key_found = TRUE;
        }
    }
    if (!key_found)
        orage_message(150, P_N "not found %s from Orage", uid_int);
}

static gboolean export_selected(char *file_name, char *uids)
{
#undef P_N
#define P_N "export_selected: "
    icalcomponent *x_ical = NULL;
    icalset *x_fical = NULL;
    gchar *uid, *uid_end, *uid_int;
    gboolean more_uids;
    int i;

#ifdef ORAGE_DEBUG
    orage_message(-200, P_N);
#endif
    if (!export_prepare_write_file(file_name))
        return(FALSE);
    if (!ORAGE_STR_EXISTS(uids)) {
        orage_message(150, P_N "UID list is empty");
        return(FALSE);
    }
    if (!ic_internal_file_open(&x_ical, &x_fical, file_name, FALSE)) {
        orage_message(150, P_N "Failed to create export file %s"
                , file_name);
        return(FALSE);
    }
    if (!xfical_file_open(TRUE)) {
        return(FALSE);
    }

    /* checks done, let's start the real work */
    more_uids = TRUE;
    for (uid = uids; more_uids; ) {
        uid_int = uid+4;
        uid_end = g_strstr_len((const gchar *)uid, strlen(uid), ",");
        if (uid_end != NULL)
            *uid_end = 0; /* uid ends here */
        /* FIXME: proper messages to screen */
        if (uid[0] == 'O') {
            export_selected_uid(ic_ical, uid_int, x_ical);
        }
        else if (uid[0] == 'F') {
            sscanf(uid, "F%02d", &i);
            if (i < g_par.foreign_count && ic_f_ical[i].ical != NULL) {
                export_selected_uid(ic_f_ical[i].ical, uid_int, x_ical);
            }
            else {
                orage_message(250, P_N "unknown foreign file number %s", uid);
                return(FALSE);
            }

        }
        else {
            orage_message(250, P_N "Unknown uid type %s", uid);
        }
        
        if (uid_end != NULL)  /* we have more uids */
            uid = uid_end+1;  /* next uid starts here */
        else
            more_uids = FALSE;
    }

    xfical_file_close(TRUE);
    icalset_mark(x_fical);
    icalset_commit(x_fical);
    icalset_free(x_fical);
    return(TRUE);
}

gboolean xfical_export_file(char *file_name, int type, char *uids)
{
#undef P_N
#define P_N "xfical_export_file: "
#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    if (type == 0) { /* copy the whole file */
        return(export_all(file_name));
    }
    else if (type == 1) { /* copy only selected appointments */
        return(export_selected(file_name, uids));
    }
    else {
        orage_message(260, P_N "Unknown app count");
        return(FALSE);
    }
}
