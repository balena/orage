/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2008 Juha Kautto  (juha at xfce.org)
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

#include <error.h>
#include <errno.h>
    /* errno */

#include <stdlib.h>
    /* malloc, atoi, free, setenv */

#include <stdio.h>
    /* printf, fopen, fread, fclose, perror, rename */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
    /* stat, mkdir */

#include <time.h>
    /* localtime, gmtime, asctime */

#include <string.h>
    /* strncmp, strcmp, strlen, strncat, strncpy, strdup, strstr */

#include <popt.h>
    /* poptGetContext */

/* This define is needed to get nftw instead if ftw.
 * Documentation says the define is _XOPEN_SOURCE, but it
 * does not work. __USE_XOPEN_EXTENDED works 
 * Same with _GNU_SOURCE and __USE_GNU */
#define _XOPEN_SOURCE 500
#define __USE_XOPEN_EXTENDED 1
#define _GNU_SOURCE 1
#define __USE_GNU 1
#include <ftw.h>
    /* nftw */

#define DEFAULT_ZONEINFO_DIRECTORY  "/usr/share/zoneinfo"
#define DEFAULT_ZONETAB_FILE        "/usr/share/zoneinfo/zone.tab"

int debug = 1; /* bigger number => more output */
char version[] = "1.0.0";
int file_cnt = 0; /* number of processed files */

unsigned char *in_buf, *in_head, *in_tail;
int in_file_base_offset = 0;

char *in_file = NULL, *out_file = NULL;
int in_file_is_dir = 0;
int only_one_level = 0;
int excl_dir_cnt = 5;
char **excl_dir = NULL;

FILE *ical_file;

/* in_timezone_name is the real timezone name from the infile 
 * we are processing.
 * in_timezone_name is the timezone we are writing. Usually it is the same
 * than in_timezone_name. 
 * timezone name is for example Europe/Helsinki */
char *timezone_name = NULL;  
char *in_timezone_name = NULL;

int ignore_older = 1970; /* Ignore rules which are older or equal than this */

/* time change table starts here */
unsigned char *begin_timechanges;           

/* time change type index table starts here */
unsigned char *begin_timechangetypeindexes; 

/* time change type table starts here */
unsigned char *begin_timechangetypes;       

/* timezone name table */
unsigned char *begin_timezonenames;         

unsigned long gmtcnt;
unsigned long stdcnt;
unsigned long leapcnt;
unsigned long timecnt;  /* points when time changes */
unsigned long typecnt;  /* table of different time changes = types */
unsigned long charcnt;  /* length of timezone name table */

struct ical_timezone_data {
    struct tm start_time;
    int gmt_offset, gmt_offset_hh, gmt_offset_mm;
    int prev_gmt_offset, prev_gmt_offset_hh, prev_gmt_offset_mm;
    unsigned int is_dst;
    char *tz;
};

struct rdate_prev_data {
    struct ical_timezone_data data;
    struct rdate_prev_data   *next;
};

read_file(const char *file_name, const struct stat *file_stat)
{
    FILE *file;

    if (debug > 1) {
        printf("read_file: start\n");
        printf("\n***** size of file %s is %d bytes *****\n\n", file_name
                , file_stat->st_size);
    }
    in_buf = malloc(file_stat->st_size);
    in_head = in_buf;
    in_tail = in_buf + file_stat->st_size - 1;
    file = fopen(file_name, "r");
    fread(in_buf, 1, file_stat->st_size, file);
    fclose(file);
    if (debug > 1)
        printf("read_file: end\n");
}

long get_long()
{
    unsigned long tmp;

    tmp = (((long)in_head[0]<<24)
         + ((long)in_head[1]<<16)
         + ((long)in_head[2]<<8)
         +  (long)in_head[3]);
    in_head += 4;
    return(tmp);
}

int process_header()
{
    if (debug > 2)
        printf("file id: %s\n", in_head);
    if (strncmp((char *)in_head, "TZif", 4)) { /* we accept version 1 and 2 */
        return(1);
    }
    /* header */
    in_head += 4; /* type */
    in_head += 16; /* reserved */
    gmtcnt  = get_long();
    if (debug > 2)
        printf("gmtcnt=%u \n", gmtcnt);
    stdcnt  = get_long();
    if (debug > 2)
        printf("stdcnt=%u \n", stdcnt);
    leapcnt = get_long();
    if (debug > 2)
        printf("leapcnt=%u \n", leapcnt);
    timecnt = get_long();
    if (debug > 2)
        printf("number of time changes: timecnt=%u \n", timecnt);
    typecnt = get_long();
    if (debug > 2)
        printf("number of time change types: typecnt=%u \n", typecnt);
    charcnt = get_long();
    if (debug > 2)
        printf("lenght of different timezone names table: charcnt=%u \n"
                , charcnt);
    return(0);
}

process_local_time_table()
{ /* points when time changes */
    unsigned long tmp;
    int i;

    begin_timechanges = in_head;
    if (debug > 3)
        printf("\n***** printing time change dates *****\n");
    for (i = 0; i < timecnt; i++) {
        tmp = get_long();
        if (debug > 3) {
            printf("GMT %d: %u =  %s", i, tmp
                    , asctime(gmtime((const time_t*)&tmp)));
            printf("\tLOC %d: %u =  %s", i, tmp
                    , asctime(localtime((const time_t*)&tmp)));
        }
    }
}

process_local_time_type_table()
{ /* pointers to table, which explain how time changes */
    unsigned char tmp;
    int i;

    begin_timechangetypeindexes = in_head;
    if (debug > 3)
        printf("\n***** printing time change type indekses *****\n");
    for (i = 0; i < timecnt; i++) { /* we need to walk over the table */
        tmp = in_head[0];
        in_head++;
        if (debug > 3)
            printf("type %d: %d\n", i, (unsigned int)tmp);
    }
}

process_ttinfo_table()
{ /* table of different time changes = types */
    long tmp;
    unsigned char tmp2, tmp3;
    int i;

    begin_timechangetypes = in_head;
    if (debug > 3)
        printf("\n***** printing different time change types *****\n");
    for (i = 0; i < typecnt; i++) { /* we need to walk over the table */
        tmp = get_long();
        tmp2 = in_head[0];
        in_head++;
        tmp3 = in_head[0];
        in_head++;
        if (debug > 3)
            printf("%d: gmtoffset:%d isdst:%d abbr:%d\n", i, tmp
                    , (unsigned int)tmp2, (unsigned int)tmp3);
    }
}

process_abbr_table()
{
    unsigned char *tmp;
    int i;

    begin_timezonenames = in_head;
    if (debug > 3)
        printf("\n***** printing different timezone names *****\n");
    tmp = in_head;
    for (i = 0; i < charcnt; i++) { /* we need to walk over the table */
        if (debug > 3)
            printf("Abbr:%d (%d)(%s)\n", i, strlen((char *)(tmp + i)), tmp + i);
        i += strlen((char *)(tmp + i));
    }
    in_head += charcnt;
}

process_leap_table()
{
    unsigned long tmp, tmp2;
    int i;

    if (debug > 3)
        printf("\n***** printing leap time table *****\n");
    for (i = 0; i < leapcnt; i++) { /* we need to walk over the table */
        tmp = get_long();
        tmp2 = get_long();
        if (debug > 3)
            printf("leaps %d: %u =  %s (%u)", i, tmp
                    , asctime(localtime((const time_t *)&tmp)), tmp2);
    }
}

process_std_table()
{
    unsigned char tmp;
    int i;

    if (debug > 3)
        printf("\n***** printing std table *****\n");
    for (i = 0; i < stdcnt; i++) { /* we need to walk over the table */
        tmp = (unsigned long)in_head[0];
        in_head++;
        if (debug > 3)
            printf("stds %d: %d\n", i, (unsigned int)tmp);
    }
}

process_gmt_table()
{
    unsigned char tmp;
    int i;

    if (debug > 3)
        printf("\n***** printing gmt table *****\n");
    for (i = 0; i < gmtcnt; i++) { /* we need to walk over the table */
        tmp = (unsigned long)in_head[0];
        in_head++;
        if (debug > 3)
            printf("gmts %d: %d\n", i, (unsigned int)tmp);
    }
}

/* go through the contents of the file and find the positions of 
 * needed data. Uses global pointer: in_head */
int process_file(const char *file_name)
{
    if (debug > 1)
        printf("process_file: start\n");
    if (process_header(file_name)) {
        printf("File (%s) does not look like tz file. Skipping it.\n"
                , file_name);
        return(1);
    }
    process_local_time_table();
    process_local_time_type_table();
    process_ttinfo_table();
    process_abbr_table();
    process_leap_table();
    process_std_table();
    process_gmt_table();
    if (debug > 1)
        printf("process_file: end\n");
}

void create_backup_file(char *out_file)
{
    char *backup_out_file, backup_ending[]=".old";
    int out_file_name_len, backup_ending_len, backup_out_file_name_len;

    out_file_name_len = strlen(out_file);
    backup_ending_len = strlen(backup_ending);
    backup_out_file_name_len = out_file_name_len + backup_ending_len;

    backup_out_file = malloc(backup_out_file_name_len + 1);
    strncpy(backup_out_file, out_file, out_file_name_len);
    backup_out_file[out_file_name_len] = '\0';
    strncat(backup_out_file, backup_ending, backup_ending_len);

    if (rename(out_file, backup_out_file)) {
        printf("Error creating backup file %s:\n", backup_out_file);
        perror("\trename");
    }
    free(backup_out_file);
}

void create_ical_directory(const char *in_file_name)
{
    char *ical_dir_name;

    if (debug > 1)
        printf("create_ical_directory: start\n");
    ical_dir_name = strdup(&in_file_name[in_file_base_offset]);
    if (mkdir(ical_dir_name, 0777)) {
        if (errno == EEXIST) {
            if (debug > 2) {
                printf("create_ical_directory: ignoring error (%s)\n"
                        , ical_dir_name);
                perror("\tcreate_ical_directory: mkdir");
            }
        }
        else {
            printf("create_ical_directory: creating directory=(%s) failed\n"
                    , ical_dir_name);
            perror("\tcreate_ical_directory: mkdir");
        }
    }
    free(ical_dir_name);
    if (debug > 1)
        printf("create_ical_directory: end\n");
}

int create_ical_file(const char *in_file_name)
{
    struct stat out_stat;
    char ical_ending[]=".ics";
    int in_file_name_len, ical_ending_len, out_file_name_len;

    if (debug > 1)
        printf("create_ical_file: start\n");
    if (out_file == NULL) { /* this is the normal case */
        in_file_name_len = strlen(&in_file_name[in_file_base_offset]);
        ical_ending_len = strlen(ical_ending);
        out_file_name_len = in_file_name_len + ical_ending_len;

        out_file = malloc(out_file_name_len + 1);
        strncpy(out_file, &in_file_name[in_file_base_offset], in_file_name_len);
        out_file[in_file_name_len] = '\0';
        strncat(out_file, ical_ending, ical_ending_len);

        in_timezone_name = strdup(&in_file_name[in_file_base_offset 
                + strlen("zoneinfo/")]);
        timezone_name = strdup(in_timezone_name);
        if (debug > 1) {
            printf("create_ical_file:\n\tinfile:(%s)\n\toutfile:(%s)\n\tin timezone:(%s)\n\tout timezone:(%s)\n"
                    , in_file_name, out_file, in_timezone_name, timezone_name);
        }
    }

    if (stat(out_file, &out_stat) == -1) { /* error */
        if (errno != ENOENT) { /* quite ok. we just need to create it */
            perror("\tstat");
            return(1);
        }
    }
    else { /* file exists, need to create backup */
        create_backup_file(out_file);
    }

    if (!(ical_file = fopen(out_file, "w"))) {
        if (errno == ENOENT) {
            /* This error probably happens since we miss some directories
             * before the actual file name. Let's try to create those and
             * then create the file again */
            char *s_dir;

            s_dir = out_file;
            for (s_dir = strchr(s_dir, '/'); s_dir != NULL
                    ; s_dir = strchr(s_dir, '/')) {
                *s_dir = '\0';
                printf("create_ical_file: creating directory=(%s)\n", out_file);
                if (mkdir(out_file, 0777))
                {
                    if (errno == EEXIST) {
                        printf("create_ical_file: ignoring error (%s)\n"
                                , out_file);
                        perror("\tcreate_ical_file: mkdir 1");
                    }
                    else {
                        printf("create_ical_file: creating directory=(%s) failed\n"
                                , out_file);
                        perror("\tcreate_ical_file: mkdir 2");
                        return(2);
                    }
                }
                *s_dir = '/';
                *s_dir++;
            }
            if (!(ical_file = fopen(out_file, "w"))) {
                /* still failed; real error, which we can not fix */
                printf("create_ical_file: outfile creation failed (%s)\n"
                        , out_file);
                perror("\tfopen2");
                return(3);
            }
        }
        else {
            printf("create_ical_file: outfile creation failed (%s)\n"
                    , out_file);
            perror("\tfopen1");
            return(4);
        }
    }
    if (debug > 1)
        printf("create_ical_file: end\n");
    return(0);
}

void write_ical_str(char *data)
{
    int len;

    len = strlen(data);
    fwrite(data, 1, len, ical_file);
}

void write_ical_header()
{
    char *vcalendar = "BEGIN:VCALENDAR\nPRODID:-//Xfce//NONSGML Orage Olson-VTIMEZONE Converter//EN\nVERSION:2.0\n";
    char *vtimezone = "BEGIN:VTIMEZONE\nTZID:/softwarestudio.org/Olson_20011030_5/";
    char *xlic = "X-LIC-LOCATION:";
    char *line = "\n";

    write_ical_str(vcalendar);
    write_ical_str(vtimezone);
    write_ical_str(timezone_name);
    write_ical_str(line);
    write_ical_str(xlic);
    write_ical_str(timezone_name);
    write_ical_str(line);
}

struct ical_timezone_data wit_get_data(int i
        , struct ical_timezone_data *prev) {
    unsigned long tc_time;
    unsigned int tct_i, abbr_i;
    struct ical_timezone_data data;

    /* get timechange time */
    in_head = begin_timechanges;
    in_head += 4*i; /* point to our row */
    tc_time = get_long();
    /* later...when we have all the data
    localtime_r((const time_t *)&tc_time, &data.start_time );
    */

    /* get timechange type index */
    in_head = begin_timechangetypeindexes;
    tct_i = (unsigned int)in_head[i];

    /* get timechange type */
    in_head = begin_timechangetypes;
    in_head += 6*tct_i;
    data.gmt_offset = (int)get_long();
    data.gmt_offset_hh = data.gmt_offset / (60*60);
    data.gmt_offset_mm = (data.gmt_offset - data.gmt_offset_hh * (60*60)) / 60;
    data.is_dst = in_head[0];
    abbr_i =  in_head[1];

    /* get timezone name */
    in_head = begin_timezonenames;
    data.tz = (char *)in_head + abbr_i;

    /* ical needs the startime in the previous (=current) time, so we need to
     * adjust by the difference. 
     * First round we do not have the prev data, so we can not do this. */
    if (i) { 
        tc_time += (prev->gmt_offset - data.gmt_offset);
    }
    localtime_r((const time_t *)&tc_time, &data.start_time );
    /* we need to remember also the previous value. Note that this is from
     * dst if we are in std and vice versa */
    data.prev_gmt_offset_hh = prev->gmt_offset_hh;
    data.prev_gmt_offset_mm = prev->gmt_offset_mm;

    return(data);
}

int wit_get_rrule(struct ical_timezone_data *prev
        , struct ical_timezone_data *cur) {
    int monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int rrule_day_cnt;
    int leap_year = 0, prev_leap_year = 0;

    if (cur->gmt_offset == prev->gmt_offset
    &&  !strcmp(cur->tz, prev->tz)) {
        /* 2) check if we can use RRULE. 
         * We only check yearly same month and same week day changing
         * rules (which should cover most real world cases). */
        if (cur->start_time.tm_year == prev->start_time.tm_year + 1
        &&  cur->start_time.tm_mon  == prev->start_time.tm_mon
        &&  cur->start_time.tm_wday == prev->start_time.tm_wday
        &&  cur->start_time.tm_hour == prev->start_time.tm_hour
        &&  cur->start_time.tm_min  == prev->start_time.tm_min
        &&  cur->start_time.tm_sec  == prev->start_time.tm_sec) {
            /* so far so good, now check that our weekdays are on
             * the same week */
            if (prev->start_time.tm_mon == 1) {
                if (((prev->start_time.tm_year%4) == 0)
                && (((prev->start_time.tm_year%100) != 0) 
                    || ((prev->start_time.tm_year%400) == 0)))
                    prev_leap_year = 1; /* leap year, february has 29 days */
            }
            /* most often these change on the last week day
             * (and on sunday, but that does not matter now) */
            if ((monthdays[cur->start_time.tm_mon] + leap_year - 
                    cur->start_time.tm_mday) < 7
            &&  (monthdays[prev->start_time.tm_mon] + prev_leap_year - 
                    prev->start_time.tm_mday) < 7) {
                /* yep, it is last */
                rrule_day_cnt = -1;
            }
            else if (cur->start_time.tm_mday < 8
                 &&  prev->start_time.tm_mday < 8) {
                /* ok, it is first */
                rrule_day_cnt = 1;
            }
            else if (cur->start_time.tm_mday < 15
                 &&  prev->start_time.tm_mday < 15
                 /* prevent moving from rule 1 (prev) to rule 2 (cur) */
                 &&  cur->start_time.tm_mday >= 8 
                 &&  prev->start_time.tm_mday >= 8) {
                /* fine, it is second */
                rrule_day_cnt = 2;
            }
            else if (cur->start_time.tm_mday < 22
                 &&  prev->start_time.tm_mday < 22
                 /* prevent moving from rule 1 or 2 (prev) to rule 3 (cur) */
                 &&  cur->start_time.tm_mday >= 15 
                 &&  prev->start_time.tm_mday >= 15) {
                /* must be the third then */
                rrule_day_cnt = 3;
            }
            else { 
                /* give up, it did not work after all.
                 * It is quite possible that rule changed, but
                 * in that case we need to write this out anyway */
                rrule_day_cnt = 100; /* RDATE is still possible */
            }
        }
        else { /* 2) failed, need to use RDATE */
            rrule_day_cnt = 100;
        }
    }
    else { /* 1) not possible to use RRULE nor RDATE, need new entry */
        rrule_day_cnt = 0;
    }
    return(rrule_day_cnt);
}

void wit_write_data(int rrule_day_cnt, struct rdate_prev_data *rdate
        , struct ical_timezone_data *first
        , struct ical_timezone_data *prev)
{
    char str[100];
    int len;
    char *dst_begin="BEGIN:DAYLIGHT\n";
    char *dst_end="END:DAYLIGHT\n";
    char *std_begin="BEGIN:STANDARD\n";
    char *std_end="END:STANDARD\n";
    char *day[] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA" };
    struct rdate_prev_data *tmp_data = NULL, *tmp_data2;
    struct ical_timezone_data tmp_prev;

    if (debug > 3)
        printf("\tWriting data (%s) %s"
                , first->is_dst ? "dst" : "std"
                , asctime(&first->start_time));

    /****** First Check that we really need to write this record *****/
    if (rrule_day_cnt == 100) { /* RDATE rule */
        if (rdate == NULL) {
        /* we actually have nothing to print. This happens seldom, but
         * is possible if we found RDATE rule, but after that we actually
         * found that it is the first RRULE element and we did not push
         * anything into the rdate store */
            if (debug > 3)
                printf("\tWrite aborted. RDATE rule changed to RRULE\n");
            return;
        }
    }
    else if (rrule_day_cnt == 0) { /* non repeating rule */
        if ((prev->start_time.tm_year + 1900) <= ignore_older) {
        /* We again have nothing to print. We got non repeating case, which
         * happens before our limit year */
            if (debug > 3)
                printf("\tWrite aborted. Too old non repeating year\n");
            return;
        }
    }

    /****** Write the data ******/
    if (prev->is_dst)
        write_ical_str(dst_begin);
    else
        write_ical_str(std_begin);

    len = snprintf(str, 30, "TZOFFSETFROM:%+03d%02d\n"
            , prev->prev_gmt_offset_hh, prev->prev_gmt_offset_mm);
    fwrite(str, 1, len, ical_file);

    len = snprintf(str, 30, "TZOFFSETTO:%+03d%02d\n"
            , prev->gmt_offset_hh, prev->gmt_offset_mm);
    fwrite(str, 1, len, ical_file);

    len = snprintf(str, 99, "TZNAME:%s\n", prev->tz);
    fwrite(str, 1, len, ical_file);

    len = snprintf(str, 30, "DTSTART:%04d%02d%02dT%02d%02d%02d\n"
            , first->start_time.tm_year + 1900
            , first->start_time.tm_mon  + 1
            , first->start_time.tm_mday
            , first->start_time.tm_hour
            , first->start_time.tm_min
            , first->start_time.tm_sec);
    fwrite(str, 1, len, ical_file);

    if (rrule_day_cnt) { /* we had repeating appointment */
        if (rrule_day_cnt < 10) { /* RRULE */
            if (debug > 3)
                printf("\t\t...RRULE\n");
            len = snprintf(str, 50, "RRULE:FREQ=YEARLY;BYMONTH=%d;BYDAY=%d%s\n"
                    , first->start_time.tm_mon + 1
                    , rrule_day_cnt
                    , day[first->start_time.tm_wday]);
            fwrite(str, 1, len, ical_file);
        }
        else { /* RDATE */
            if (debug > 3)
                printf("\t\t...RDATE\n");
            for (tmp_data = rdate; tmp_data ; ) {
                tmp_prev = tmp_data->data;
                len = snprintf(str, 30, "RDATE:%04d%02d%02dT%02d%02d%02d\n"
                        , tmp_prev.start_time.tm_year + 1900
                        , tmp_prev.start_time.tm_mon  + 1
                        , tmp_prev.start_time.tm_mday
                        , tmp_prev.start_time.tm_hour
                        , tmp_prev.start_time.tm_min
                        , tmp_prev.start_time.tm_sec);
                fwrite(str, 1, len, ical_file);

                tmp_data2 = tmp_data;
                tmp_data = tmp_data->next;
                free(tmp_data2);
            }
        }
    }
    else {
        if (debug > 3)
            printf("\t\t...single\n");
        len = snprintf(str, 30, "RDATE:%04d%02d%02dT%02d%02d%02d\n"
                , prev->start_time.tm_year + 1900
                , prev->start_time.tm_mon  + 1
                , prev->start_time.tm_mday
                , prev->start_time.tm_hour
                , prev->start_time.tm_min
                , prev->start_time.tm_sec);
        fwrite(str, 1, len, ical_file);
    }

    if (prev->is_dst)
        write_ical_str(dst_end);
    else
        write_ical_str(std_end);
}

void write_ical_timezones()
{
    int i;
    struct ical_timezone_data ical_data
        , ical_data_prev = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, NULL}
        , data_prev_std = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, NULL}
        , data_first_std = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, NULL}
        , data_prev_dst = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, NULL}
        , data_first_dst = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, NULL};
        /* pointers either to data_prev_std and data_first_std
         * or to data_prev_dst and data_first_dst. 
         * These avoid having separate code paths for std and dst */
    struct ical_timezone_data *p_data_prev, *p_data_first;

    int std_init_done = 0;
    int dst_init_done = 0;
    int rrule_day_cnt_std = 0, rrule_day_cnt_std_prev = 0;
    int rrule_day_cnt_dst = 0, rrule_day_cnt_dst_prev = 0;
        /* pointers either to rrule_day_cnt_std and rrule_day_cnt_std_prev
         * or to rrule_day_cnt_dst and rrule_day_cnt_dst_prev. 
         * These avoid having separate code paths for std and dst */
    int *p_rrule_day_cnt = 0, *p_rrule_day_cnt_prev = 0;
    struct rdate_prev_data *prev_dst_data = NULL, *prev_std_data = NULL
        /* points either to prev_dst_data or to prev_std_data */
        , **p_prev_data 
        , *tmp_data = NULL, *tmp_data2 = NULL;

    /* we are processing "in_timezone_name" so we know it exists in this 
     * system and it is then safe to use that in the localtime conversion */
    if (setenv("TZ", in_timezone_name, 1)) {
        printf("changing timezone=(%s) failed\n", in_timezone_name);
        perror("\tsetenv");
        return;
    }
    for (i = 0; i < timecnt; i++) {
    /***** get data *****/
        ical_data = wit_get_data(i, &ical_data_prev);
        if (i == 0) { /* first round, do starting values */
            ical_data_prev = ical_data;
        }

        if (ical_data.is_dst) {
            if (!dst_init_done) {
                data_first_dst = ical_data;
                data_prev_dst  = ical_data;
                dst_init_done  = 1;
                if (debug > 2)
                    printf("init %d = (%s) %s", i
                            , ical_data.is_dst ? "dst" : "std"
                            , asctime(&ical_data.start_time));
                continue; /* we never write the first record */
            }
            p_data_first = &data_first_dst;
            p_data_prev = &data_prev_dst;
            p_rrule_day_cnt = &rrule_day_cnt_dst;
            p_rrule_day_cnt_prev = &rrule_day_cnt_dst_prev;
            p_prev_data = &prev_dst_data;
        }
        else { /* !dst == std */
            if (!std_init_done) {
                data_first_std = ical_data;
                data_prev_std  = ical_data;
                std_init_done  = 1;
                if (debug > 2)
                    printf("init %d = (%s) %s", i
                            , ical_data.is_dst ? "dst" : "std"
                            , asctime(&ical_data.start_time));
                continue; /* we never write the first record */
            }
            p_data_first = &data_first_std;
            p_data_prev = &data_prev_std;
            p_rrule_day_cnt = &rrule_day_cnt_std;
            p_rrule_day_cnt_prev = &rrule_day_cnt_std_prev;
            p_prev_data = &prev_std_data;
        }

    /***** check if we need this data *****/
        /* we only take newer than threshold values */
        if (ical_data.start_time.tm_year + 1900 <= ignore_older) {
            if (debug > 2)
                printf("skip %d = (%s) %s", i
                        , ical_data.is_dst ? "dst" : "std"
                        , asctime(&ical_data.start_time));
            ical_data_prev = ical_data;
            *p_data_first = ical_data;
            *p_data_prev = ical_data;
            *p_rrule_day_cnt_prev = *p_rrule_day_cnt;
            *p_rrule_day_cnt = wit_get_rrule(p_data_prev, &ical_data);
            /* without p_ variables this looked like:
            if (ical_data.is_dst) {
                data_first_dst = ical_data;
                data_prev_dst  = ical_data;
                rrule_day_cnt_dst_prev = rrule_day_cnt_dst;
                rrule_day_cnt_dst = wit_get_rrule(&data_prev_dst, &ical_data);
            }
            else {
                data_first_std = ical_data;
                data_prev_std  = ical_data;
                rrule_day_cnt_std_prev = rrule_day_cnt_std;
                rrule_day_cnt_std = wit_get_rrule(&data_prev_std, &ical_data);
            }
            */
            continue;
        }
        if (debug > 2)
            printf("process %d: (%s) from %02d:%02d to %02d:%02d (%s) %s", i
                    , ical_data.is_dst ? "dst" : "std"
                    , ical_data.prev_gmt_offset_hh,ical_data.prev_gmt_offset_mm
                    , ical_data.gmt_offset_hh, ical_data.gmt_offset_mm
                    , ical_data.tz
                    , asctime(&ical_data.start_time));
    /***** check if we can shortcut the entry with RRULE or RDATE *****/
        /* 1) check if it is similar to the previous values */
        *p_rrule_day_cnt_prev = *p_rrule_day_cnt;
        *p_rrule_day_cnt = wit_get_rrule(p_data_prev, &ical_data);
        if (*p_rrule_day_cnt
        && (*p_rrule_day_cnt == *p_rrule_day_cnt_prev
            && *p_rrule_day_cnt_prev)) {
            /* we continue with either real RRULE or RDATE */
            if (*p_rrule_day_cnt < 10) {
                /* we found RRULE, so we do not have to do anything 
                 * since this just continues. */
                if (debug > 3)
                    printf("\tRRULE value found\n");
                *p_data_prev = ical_data;
            }
            else { /* we actually found RDATE */
                if (debug > 3)
                    printf("\tRDATE value found\n");
                if (p_data_prev->start_time.tm_year + 1900 <= ignore_older) {
                    /* do not push old values into queue, ignore them */
                    if (debug > 3)
                        printf("\tAborted push to RRULE queue, too old (%s) %s"
                                , p_data_prev->is_dst ? "dst" : "std"
                                , asctime(&p_data_prev->start_time));
                }
                else {
                    if (debug > 3)
                        printf("\tpushed to RRULE queue (%s) %s"
                                , p_data_prev->is_dst ? "dst" : "std"
                                , asctime(&p_data_prev->start_time));
                    for (tmp_data = *p_prev_data; tmp_data ; ) {
                        /* find the last one, which is still empty */
                        tmp_data2 = tmp_data;
                        tmp_data  = tmp_data->next;
                    }
                    tmp_data = malloc(sizeof(struct rdate_prev_data));
                    tmp_data->data = *p_data_prev;
                    tmp_data->next = NULL; /* last */
                    if (!*p_prev_data) {
                        *p_prev_data = tmp_data;
                    }
                    else {
                        tmp_data2->next = tmp_data;
                    }
                }
                *p_data_prev = ical_data;
            }
        }
        else { /* not RRULE or we changed to/from RRULE from/to RDATE, 
                * so write previous and init new round */
            if (debug > 3)
                printf("\tnon repeating value found\n");
            wit_write_data(*p_rrule_day_cnt_prev, *p_prev_data
                    , p_data_first, p_data_prev);
            if (*p_rrule_day_cnt_prev > 10 && *p_rrule_day_cnt < 10) {
                /* we had RDATE and now we found RRULE */
                /* so this was actually the first RRULE */
                if (debug > 3)
                    printf("\tchanged from RDATE to RRULE\n");
                if (p_data_prev->start_time.tm_year + 1900 <= ignore_older) {
                    /* we ignore too old record and take current as first */
                    if (debug > 3) {
                        printf("\tIgnoring too old (%s) %s"
                                , p_data_prev->is_dst ? "dst" : "std"
                                , asctime(&p_data_prev->start_time));
                        printf("\tUsing RRULE (%s) %s"
                                , ical_data.is_dst ? "dst" : "std"
                                , asctime(&ical_data.start_time));
                    }
                    *p_data_first = ical_data; 
                }
                else {
                    if (debug > 3)
                        printf("\tUsing RRULE (%s) %s"
                                , p_data_prev->is_dst ? "dst" : "std"
                                , asctime(&p_data_prev->start_time));
                    *p_data_first = *p_data_prev; 
                }
            }
            else {
                *p_data_first = ical_data;
            }
            *p_data_prev  = ical_data;
            *p_prev_data  = NULL;
        } 
        ical_data_prev = ical_data;
    } /* for (i = 0; i < timecnt; i++) */
    /* need to write the last one also */
    if (debug > 3)
        printf("writing last values:\n");
    wit_write_data(rrule_day_cnt_std_prev, prev_std_data
            , &data_first_std, &data_prev_std);
    wit_write_data(rrule_day_cnt_dst_prev, prev_dst_data
            , &data_first_dst, &data_prev_dst);
}

void write_ical_ending()
{
    char *end= "END:VTIMEZONE\nEND:VCALENDAR\n";

    write_ical_str(end);
}

/* FIXME: need to check that if OUTFILE is given as a parameter,
 * INFILE is not a directory (or make outfile to act like directory also ? */
int write_ical_file(const char *in_file_name, const struct stat *in_file_stat)
{
    if (debug > 1)
        printf("write_ical_file: start\n");
    if (create_ical_file(in_file_name))
        return(1);
    write_ical_header();
    write_ical_timezones();
    write_ical_ending();
    fclose(ical_file);
    if (debug > 1)
        printf("write_ical_file: end\n");
    return(0);
}

int par_version()
{
    printf(
        "orage_tz_to_ical_convert version (Orage utility) %s\n"
        "Copyright © 2008 Juha Kautto\n"
        "License: GNU GPL <http://gnu.org/licenses/gpl.html>\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY.\n"
        , version
    );
    return(1);
}

int par_help()
{
    printf(
        "orage_tz_to_ical_convert converts operating system timezone (tz) files\n"
        "to ical format. Often you find tz files in /usr/share/zoneinfo/\n\n"
        "Usage: orage_tz_to_ical_convert [in_file] [out_file] [parameters]...\n"
        "parameters:\n"
        "\t -h, -?, --help\t\t this help text\n"
        "\t -V, --version\t\t orage_tz_to_ical_convert version\n"
        "\t -i, --infile\t\t tz file name from operating system.\n"
        "\t\t\t\t If this is directory, all files in it are\n"
        "\t\t\t\t processed\n"
        "\t -o, --outfile\t\t ical file name to be written\n"
        "\t\t\t\t If this is directory, file is written into\n"
        "\t\t\t\t it using the same name, but adding .ics to\n"
        "\t\t\t\t the end of it.\n"
    );
    return(1);
}

int get_parameters_popt(int argc, const char **argv)
{
    int par_type = 0, val, res = 0, i;
    char *tmp_str = NULL;
    poptContext popt_con;
    struct poptOption parameters[] = {
        /*
        {"help", 'h', POPT_ARG_NONE, &par_type, 1
            , "this help text", NULL}
            */
        {"version", 'V', POPT_ARG_NONE, &par_type, 2
            , "orage_tz_to_ical_convert version", NULL},
        {"infile", 'i', POPT_ARG_STRING, &tmp_str, 3
            , "tz file name from operating system."
              " If this is directory, all files in it are processed"
            , "filename"},
        {"outfile", 'o', POPT_ARG_STRING, &tmp_str, 4
            , "ical file name to be written."
              " This can not be directory."
              " It is meant to be used together with timezone parameter."
            , "filename"},
        {"message", 'm', POPT_ARG_INT, &debug, 5
            , "message level. How much exra information is shown."
              " 0 is least and 10 is highest (1=default)."
            , "level"},
        {"limit", 'l', POPT_ARG_INT, &ignore_older, 6
            , "limit proocessing to newer than this year."
            , "year"},
        {"timezone", 't', POPT_ARG_STRING, &tmp_str, 7
            , "timezone name. For example Europe/Helsinki"
            , "filename"},
        {"norecursive", 'r', POPT_ARG_INT, &only_one_level, 8
            , "process only main directory, instead of all subdirectories."
              " 0 = recursive  1 = only main directory (0=default)."
            , "level"},
        {"excluce count", 'c', POPT_ARG_INT, &excl_dir_cnt, 9
            , "number of excluded directories."
              " 5 = default (You only need this if you have more"
              " than 5 excluded directories)."
            , "count"},
        {"exclude", 'x', POPT_ARG_STRING, &tmp_str, 10
            , "do not process this directory, skip it."
              " You can give several directories with separate parameters."
              " By default directories right and posix are excluded, but if"
              " you use this parameter, you have to specify those also."
            , "directory"},
        POPT_AUTOHELP
        {NULL, '\0', POPT_ARG_NONE, NULL, 0, NULL, NULL}
    };

    if (debug > 1)
        printf("get_parameters_popt: start\n");
    popt_con = poptGetContext("Orage", argc, argv, parameters, 0);
    poptSetOtherOptionHelp(popt_con, "[OPTION...] [INFILE]");
    while ((val = poptGetNextOpt(popt_con)) > 0) {
        if (debug > 2)
            printf("Read parameter %d (%s)\n", val, tmp_str);
        switch (val) {
            case 1:
                par_help();
                res = val;
                break;
            case 2:
                par_version();
                res = val;
                break;
            case 3:
                in_file = strdup(tmp_str);
                break;
            case 4:
                out_file = strdup(tmp_str);
                break;
            case 5:
                printf("Debug level set to %d\n", debug);
                break;
            case 6:
                printf("Only including newer than year %d time changes\n"
                        , ignore_older);
                break;
            case 7:
                timezone_name = strdup(tmp_str);
                break;
            case 8:
                if (only_one_level)
                    printf("Processing only main directory\n");
                else
                    printf("Processing recursively all directories\n");
                break;
            case 9:
                if (excl_dir) 
                printf("Number of excluded directories set to %d\n"
                        , excl_dir_cnt);
                break;
            case 10:
                printf("Skipping directory (%s)\n", tmp_str);
                if (!excl_dir) {
                    excl_dir = calloc(excl_dir_cnt + 1, sizeof(char *));
                }
                for (i = 0; (i <= excl_dir_cnt) && excl_dir[i]; i++)
                    /* loop until array ends or we find empty place */
                    ;
                if (i < excl_dir_cnt) {
                    excl_dir[i] = strdup(tmp_str);
                }
                else {
                    printf("not enough room (%s). Use bigger exclude count\n"
                            , tmp_str);
                    res = val;
                }
                break;
            default:
                res = val;
                printf("unknown parameter\n");
        }
    }
    if (val != -1) {
        res = val;
        printf("Error in parameter handling (popt) %s: %s\n"
                , poptBadOption(popt_con, POPT_BADOPTION_NOALIAS)
                , poptStrerror(val));
    }
    else { /* process optional arguments: infile, outfile */
        if ((tmp_str = (char *)poptGetArg(popt_con))) {
            if (in_file)
                free(in_file);
            in_file = strdup(tmp_str);
            while ((tmp_str = (char *)poptGetArg(popt_con))) {
                printf("ignoring leftover parameter (%s)\n", tmp_str);
            }
        }
    }
    poptFreeContext(popt_con);
    if (debug > 1)
        printf("get_parameters_popt: end\n");
    return(res);
}

void add_zone_tabs()
{
    /* ical index filename is zoneinfo/zones.tab 
     * and os file is zone.tab */
    char ical_zone[]="zoneinfo/zones.tab";
    FILE *os_zone_tab, *ical_zone_tab;
    struct stat ical_zone_stat;
    char *ical_zone_buf, *line_end, *buf;
    int offset; /* offset to next timezone in libical zones.tab */
    int before; /* current timezone appears before ours in zones.tab */
    int len = strlen(timezone_name), buf_len;

    if (debug > 1)
        printf("add_zone_tabs: start\n");
    ical_zone_tab = fopen(ical_zone, "r+"); 
    if (ical_zone_tab == NULL) { /* does not exist or other error */
        printf("Error opening (%s) file, creating it.\n", ical_zone);
        perror("\tfopen");
        /*  let's try to create it */
        ical_zone_tab = fopen(ical_zone, "w"); 
        if (ical_zone_tab == NULL) { /* real, unrecoverable error */
            printf("Error creating (%s) file. Do it manually.\n", ical_zone);
            perror("\tfopen");
            return; /* we do not update it */
        }
        else { /* success, reopen to correct mode */
            fclose(ical_zone_tab);
            ical_zone_tab = fopen(ical_zone, "r+"); 
            if (ical_zone_tab == NULL) { /* does not exist or other error */
                printf("Error opening (%s) file. Update it manually.\n"
                        , ical_zone);
                perror("\tfopen");
                return; /* we do not update it */
            }
        }
    }
    if (stat(ical_zone, &ical_zone_stat) == -1) {
        perror("\tstat");
        fclose(ical_zone_tab);
        return;
    }

    ical_zone_buf = malloc(ical_zone_stat.st_size+1);
    fread(ical_zone_buf, 1, ical_zone_stat.st_size, ical_zone_tab);
    if (ferror(ical_zone_tab)) {
        printf("add_zone_tabs: error reading (%s).\n", ical_zone);
        perror("\tfread");
        free(ical_zone_buf);
        fclose(ical_zone_tab);
        return;
    }
    ical_zone_buf[ical_zone_stat.st_size] = 0; /* end with null */
    if (strstr(ical_zone_buf, timezone_name)) {
        if (debug > 1)
            printf("add_zone_tabs: timezone exists already, returning.\n");
        free(ical_zone_buf);
        fclose(ical_zone_tab);
        return;
    }
    for (offset = 18, before = 1; 
            offset < ical_zone_stat.st_size && before;
            offset = line_end - ical_zone_buf + 19) {
        line_end = strchr(&ical_zone_buf[offset], '\n');
        if (line_end == NULL)
            break; /* end of file */
        if (strncmp(&ical_zone_buf[offset], timezone_name, len) > 0) {
            before = 0;
            break;
        }
    }
    buf_len=len+18+1; /* +1=add \n */
    buf = malloc(buf_len+1); /* +1=add \0 */
    sprintf(buf, "+0000000 -0000000 %s\n", timezone_name);
    if (before) {
        if (fseek(ical_zone_tab, 0l, SEEK_END))
            perror("\tfseek-end");
        else
            fwrite(buf, 1, buf_len, ical_zone_tab);
    }
    else {
        if (fseek(ical_zone_tab, offset-18, SEEK_SET))
            perror("\tfseek-set");
        else {
            fwrite(buf, 1, buf_len, ical_zone_tab);
            buf_len = strlen(&ical_zone_buf[offset-18]);
            fwrite(&ical_zone_buf[offset-18], 1, buf_len, ical_zone_tab);
        }
    }

    free(buf);
    free(ical_zone_buf);
    fclose(ical_zone_tab);
    if (debug > 1)
        printf("add_zone_tabs: end\n");
}

/* The main code. This is called once per each file found */
int file_call(const char *file_name, const struct stat *sb, int flags
        , struct FTW *f)
{
    int i;

    if (debug > 1)
        printf("file_call: start\n");
    file_cnt++;
    /* we are only interested about files and directories we can access */
    if (flags == FTW_F) {
        if (debug > 0)
            printf("\t\tfile_call: processing file=(%s)\n", file_name);
        if (only_one_level && (f->level > 1)) {
            /* we never actually come here since we use FTW_SKIP_SUBTREE
             * in the directory level */
            if (debug > 0)
                printf("\t\tfile_call: skipping it, not on top level\n");
            return(FTW_CONTINUE);
        }
        read_file(file_name, sb);
        process_file(file_name);
        write_ical_file(file_name, sb);
        add_zone_tabs();
        free(in_buf);
        free(out_file);
        out_file = NULL;
        free(in_timezone_name);
        free(timezone_name);
    }
    else if (flags == FTW_D) {
        if (debug > 0)
            printf("\tfile_call: processing directory=(%s)\n", file_name);
        if (only_one_level && f->level > 0) {
            if (debug > 0)
                printf("\t\tfile_call: skipping it, not on top level\n");
            return(FTW_SKIP_SUBTREE);
        }
        /* need to check if we have excluded directory */
        for (i = 0; (i <= excl_dir_cnt) && excl_dir[i]; i++) {
            if (strcmp(excl_dir[i],  file_name+f->base) == 0) {
                if (debug > 0)
                    printf("\t\tfile_call: skipping excluded directory (%s)\n"
                            , file_name+f->base);
                return(FTW_SKIP_SUBTREE);
            }
        }
        create_ical_directory(file_name);
    }
    else if (flags == FTW_SL) {
        if (debug > 0) {
            printf("\t\tfile_call: skipping symbolic link=(%s)\n", file_name);
        }
    }
    else {
        if (debug > 0) {
            printf("\t\tfile_call: skipping inaccessible file=(%s)\n", file_name);
        }
    }

    if (debug > 1)
        printf("file_call: end\n");
    return(FTW_CONTINUE);
}

/* check the parameters and use defaults when possible */
int check_parameters()
{
    char *s_tz, *last_tz = NULL, tz[]="/zoneinfo", tz2[]="zoneinfo/";
    int tz_len, i;
    struct stat in_stat;

    if (debug > 1)
        printf("check_parameters: start\n");
    if (in_file == NULL) /* in file not found */
        in_file = strdup(DEFAULT_ZONEINFO_DIRECTORY);

    if (in_file[0] != '/') {
        printf("check_parameters: in_file name (%s) is not absolute file name. Ending\n"
                , in_file);
        return(1);
    }
    if (stat(in_file, &in_stat) == -1) { /* error */
        perror("\tcheck_parameters: stat");
        return(2);
    }
    if (S_ISDIR(in_stat.st_mode)) {
        in_file_is_dir = 1;
        if (timezone_name) {
            printf("\tcheck_parameters: when infile (%s) is directory, you can not specify timezone name (%s), but it is copied from each in file. Ending\n"
                , in_file, timezone_name);
            return(3);
        }
        if (out_file) {
            printf("\tcheck_parameters: when infile (%s) is directory, you can not specify outfile name (%s), but it is copied from each in file. Ending\n"
                , in_file, out_file);
            return(3);
        }
    }
    else {
        in_file_is_dir = 0;
        if (!S_ISREG(in_stat.st_mode)) {
            printf("\tcheck_parameters: in_file (%s) is not directory nor normal file. Ending\n"
                , in_file);
            return(3);
        }
    }

    if (excl_dir == NULL) { /* use default */
        excl_dir_cnt = 5; /* just in case it was changed by parameter */
        excl_dir = calloc(3, sizeof(char *));
        excl_dir[0] = strdup("posix");
        excl_dir[1] = strdup("right");
    }

    /* find last "/zoneinfo" from the infile (directory) name. 
     * Normally there is only one. 
     * It needs to be at the end of the string or be followed by '/' */
    tz_len = strlen(tz);
    s_tz = in_file;
    for (s_tz = strstr(s_tz, tz); s_tz != NULL; s_tz = strstr(s_tz, tz)) {
        if (s_tz[tz_len] == '\0' || s_tz[tz_len] == '/')
            last_tz = s_tz;
        *s_tz++;
    }
    if (last_tz == NULL) {
        printf("check_parameters: in_file name (%s) does not contain (%s). Ending\n"
                , in_file, tz);
        return(4);
    }

    in_file_base_offset = last_tz - in_file + 1; /* skip '/' */

    if (!in_file_is_dir) {
        in_timezone_name = strdup(&in_file[in_file_base_offset + strlen(tz2)]);
        if (timezone_name == NULL)
            timezone_name = strdup(in_timezone_name);
    }

    if (debug > 1) {
        printf("\n***** Parameters *****\n");
        printf("\tversion: %s\n", version);
        printf("\tdebug level: %d\n", debug);
        printf("\tyear limit: %d\n", ignore_older);
        printf("\tinfile: (%s) %s\n", in_file
                , in_file_is_dir ? "directory" : "normal file");
        printf("\tinfile timezone: (%s)\n", in_timezone_name);
        printf("\toutfile: (%s)\n", out_file);
        printf("\toutfile timezone: (%s)\n", timezone_name);
        printf("\trecursive directories: %s\n"
                , only_one_level ? "FALSE" : "TRUE");
        printf("\tmaximum exclude directory count: (%d)\n", excl_dir_cnt);
        for (i = 0; (i <= excl_dir_cnt) && excl_dir[i];i++)
            printf("\t\texclude directory %d: (%s)\n"
                    , i, excl_dir[i]);
        printf("***** Parameters *****\n\n");
    }
    if (debug > 1)
        printf("check_parameters: end\n");
    return(0); /* continue */
}

main(int argc, const char **argv)
{
    if (debug > 1)
        printf("main: start\n");
    /* if (exit_code = process_parameters(argc, argv)) */
    if (get_parameters_popt(argc, argv))
        exit(EXIT_FAILURE); /* help, version or error => end processing */
    if (check_parameters())
        exit(EXIT_FAILURE);

    /* nftw goes through the whole file structure and calls "file_call"
     * with each file. It returns 0 when everything has been done and -1
     * if it run into an error. */
    if (nftw(in_file, file_call, 10, FTW_PHYS | FTW_ACTIONRETVAL) == -1) {
        perror("nftw error in file handling");
        exit(EXIT_FAILURE);
    }

    printf("Processed %d files\n", file_cnt);

    free(in_file);
    if (debug > 1)
        printf("main: end\n");
    exit(EXIT_SUCCESS);
}
