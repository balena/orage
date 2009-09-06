/*      Orage - Calendar and alarm handler
 *
 * Copyright © 2006-2009 Juha Kautto  (juha at xfce.org)
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

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "tz_zoneinfo_read.h"
#include "timezone_selection.h"


/*
#define ORAGE_DEBUG 1
*/

enum {
    LOCATION,
    LOCATION_ENG,
    OFFSET,
    COUNTRY,
    NEXT_CHANGE,
    N_COLUMNS
};

static GtkTreeStore *tz_button_create_store(gboolean details)
{
#undef P_N
#define P_N "tz_button_create_store: "
#define MAX_AREA_LENGTH 100

    GtkTreeStore *store;
    GtkTreeIter iter1, iter2, main;
    orage_timezone_array tz_a;
    char area_old[MAX_AREA_LENGTH+2]; /*+2 = / + null */
    char s_offset[100], s_country[100], s_next[100];
    gint i, j, offs_hour, offs_min;

    store = gtk_tree_store_new(N_COLUMNS
            , G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING
            , G_TYPE_STRING, G_TYPE_STRING);
    strcpy(area_old, "S T a R T"); /* this never matches */
    tz_a = get_orage_timezones(details);
    /*
    g_print(P_N "number of timezones %d\n", tz_a.count);
    */
    /* Create special "area" for first level timezones, which do not have
     * any real area */
    gtk_tree_store_append(store, &iter1, NULL);
    gtk_tree_store_set(store, &iter1
            , LOCATION, _(" Other")
            , LOCATION_ENG, " Other"
            , OFFSET, " "
            , COUNTRY, " "
            , NEXT_CHANGE, " "
            , -1);
    main = iter1; /* need to remember that */

    for (i=0; i < tz_a.count-2; i++) {
        /* first check area */
        if (! g_str_has_prefix(tz_a.city[i], area_old)) {
            /* we have new area, let's add it */
            for (j=0; tz_a.city[i][j] && tz_a.city[i][j] != '/'
                      && j < MAX_AREA_LENGTH; j++) {
                area_old[j] = tz_a.city[i][j];
            }
        /* now tz_a.city[i][j] is either / or 0 which means not found / */
            if (!tz_a.city[i][j]) { /* end of name = no are code */
                iter1 = main;
            }
            else if (j < MAX_AREA_LENGTH) { /* new area, let's add it */
                area_old[j] = 0;
                gtk_tree_store_append(store, &iter1, NULL);
                gtk_tree_store_set(store, &iter1
                        , LOCATION, _(area_old)
                        , LOCATION_ENG, area_old
                        , OFFSET, " "
                        , COUNTRY, " "
                        , NEXT_CHANGE, " "
                        , -1);
                /* let's make sure we do not match accidentally to those 
                 * plain names on main level. We do this by adding / */
                area_old[j++] = '/';
                area_old[j] = 0;
            }
            else {
                g_print(P_N "too long line in zones.tab %s", tz_a.city[i]);
            }
        }
        /* then city translated and in base form used internally */
        gtk_tree_store_append(store, &iter2, &iter1);
        offs_hour = tz_a.utc_offset[i] / (60*60);
        offs_min = abs((tz_a.utc_offset[i] - offs_hour * (60*60)) / 60);
        /*
        if (offs_min)
            g_print(P_N " %s offset %d hour %d minutes %d\n", tz_a.city[i], tz_a.utc_offset[i], offs_hour, offs_min);
        */
        g_snprintf(s_offset, 100, "%+03d:%02d %s (%s)"
                , offs_hour, offs_min
                , (tz_a.dst[i]) ? "dst" : "std"
                , (tz_a.tz[i]) ? tz_a.tz[i] : "-");
        if (details && tz_a.country[i] && tz_a.cc[i])
            g_snprintf(s_country, 100, "%s (%s)", tz_a.country[i], tz_a.cc[i]);
        else
            strcpy(s_country, " ");
        if (details)
            g_snprintf(s_next, 100, "%s"
                    , (tz_a.next[i]) ? tz_a.next[i] : _("not changing"));
        else
            strcpy(s_next, " ");

        gtk_tree_store_set(store, &iter2
                , LOCATION, _(tz_a.city[i])
                , LOCATION_ENG, tz_a.city[i]
                , OFFSET, s_offset
                , COUNTRY, s_country
                , NEXT_CHANGE, s_next
                , -1);
    }
    return(store);
}

static gint sortEvent_comp(GtkTreeModel *model
        , GtkTreeIter *i1, GtkTreeIter *i2, gpointer data)
{
    gint col = GPOINTER_TO_INT(data);
    gint ret;
    gchar *text1, *text2;

    gtk_tree_model_get(model, i1, col, &text1, -1);
    gtk_tree_model_get(model, i2, col, &text2, -1);
    ret = strcmp(text1, text2);
    g_free(text1);
    g_free(text2);
    return(ret);
}

GtkWidget *tz_button_create_view(gboolean details, GtkTreeStore *store)
{
#undef P_N
#define P_N "tz_button_create_view: "
#define MAX_AREA_LENGTH 100

    GtkWidget *tree;
    GtkTreeSortable  *TreeSortable;
    GtkCellRenderer *rend;
    GtkTreeViewColumn *col;

    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    TreeSortable = GTK_TREE_SORTABLE(store);
    gtk_tree_sortable_set_sort_func(TreeSortable, LOCATION
            , sortEvent_comp, GINT_TO_POINTER(LOCATION), NULL);
    gtk_tree_sortable_set_sort_column_id(TreeSortable
            , LOCATION, GTK_SORT_ASCENDING);

    rend = gtk_cell_renderer_text_new();
    col  = gtk_tree_view_column_new_with_attributes(_("Location")
            , rend, "text", LOCATION, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);

    rend = gtk_cell_renderer_text_new();
    col  = gtk_tree_view_column_new_with_attributes(_("Location")
            , rend, "text", LOCATION_ENG, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
    gtk_tree_view_column_set_visible(col, FALSE);

    rend = gtk_cell_renderer_text_new();
    col  = gtk_tree_view_column_new_with_attributes(_("GMT Offset")
            , rend, "text", OFFSET, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);

    if (details) {
        rend = gtk_cell_renderer_text_new();
        col  = gtk_tree_view_column_new_with_attributes(_("Country")
                , rend, "text", COUNTRY, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);

        rend = gtk_cell_renderer_text_new();
        col  = gtk_tree_view_column_new_with_attributes(_("Next Change")
                , rend, "text", NEXT_CHANGE, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
    }
    return(tree);
}

gboolean orage_timezone_button_clicked(GtkButton *button, GtkWindow *parent
        , gchar **tz)
{
#undef P_N
#define P_N "orage_timezone_button_clicked: "

    GtkTreeStore *store;
    GtkWidget *tree;
    GtkWidget *window;
    GtkWidget *sw;
    int result;
    char *loc, *loc_eng;
    GtkTreeSelection *sel;
    GtkTreeModel     *model;
    GtkTreeIter       iter;
    gboolean    changed = FALSE;
    gboolean    details = FALSE;

    store = tz_button_create_store(details);
    tree = tz_button_create_view(details, store);

    /* show it */
    window =  gtk_dialog_new_with_buttons(_("Pick timezone")
            , parent
            , GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
            , _("Change mode"), 4
            , _("UTC"), 1
            /* , _("floating"), 2 */
            /* , _(g_par.local_timezone), 3 */
            , GTK_STOCK_OK, GTK_RESPONSE_ACCEPT
            , NULL);
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tree);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), sw, TRUE, TRUE, 0);
    gtk_window_set_default_size(GTK_WINDOW(window), 610, 500);

    gtk_widget_show_all(window);
    do {
        result = gtk_dialog_run(GTK_DIALOG(window));
        switch (result) {
            case GTK_RESPONSE_ACCEPT:
                sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
                if (gtk_tree_selection_get_selected(sel, &model, &iter))
                    if (gtk_tree_model_iter_has_child(model, &iter))
                        result = 0;
                    else {
                        gtk_tree_model_get(model, &iter, LOCATION, &loc, -1);
                        gtk_tree_model_get(model, &iter, LOCATION_ENG, &loc_eng
                                , -1);                     }
                else {
                    loc = g_strdup(_(*tz));
                    loc_eng = g_strdup(*tz);
                }
                break;
            case 1:
                loc = g_strdup(_("UTC"));
                loc_eng = g_strdup("UTC");
                break;
                /*
            case 2:
                loc = g_strdup(_("floating"));
                loc_eng = g_strdup("floating");
                break;
            case 3:
                loc = g_strdup(_(g_par.local_timezone));
                loc_eng = g_strdup(g_par.local_timezone);
                break;
                */
            case 4:
                free_orage_timezones(details);
                details = !details;
                /* gtk_widget_destroy(GTK_WIDGET(store)); */
                gtk_widget_destroy(tree);
                store = tz_button_create_store(details);
                tree = tz_button_create_view(details, store);
                gtk_container_add(GTK_CONTAINER(sw), tree);
                gtk_widget_show_all(tree);
                result = 0;
                break;
            default:
                loc = g_strdup(_(*tz));
                loc_eng = g_strdup(*tz);
                break;
        }
    } while (result == 0);
    if (g_ascii_strcasecmp(loc, (gchar *)gtk_button_get_label(button)) != 0)
        changed = TRUE;
    gtk_button_set_label(button, loc);

    if (*tz)
        g_free(*tz);
    *tz = g_strdup(loc_eng);
    g_free(loc);
    g_free(loc_eng);
    gtk_widget_destroy(window);
    return(changed);
}
