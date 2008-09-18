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
    /* strncmp, strcmp, strlen, strcat, strncat, strncpy, strdup, strtok */

#include <libgen.h>
    /* dirname, basename */

/* This define is needed to get nftw instead if ftw.
 * Documentation says the define is _XOPEN_SOURCE, but it
 * does not work. __USE_XOPEN_EXTENDED works */
#define _XOPEN_SOURCE 500
#define __USE_XOPEN_EXTENDED 1
#include <ftw.h>
    /* nftw */

#define DEFAULT_ZONEINFO_DIRECTORY "/usr/share/zoneinfo"

int debug = 2; /* more output */

unsigned char *in_buf, *in_head, *in_tail;
int in_file_base_offset = 0;

char *in_file = NULL, *out_file = NULL;

FILE *ical_file;

char *timezone_name;  /* for example Europe/Helsinki */

int ignore_older = /*199*/ 0; /* Ignore rules which are older than this */

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
    int gmt_offset_hh, gmt_offset_mm;
    int prev_gmt_offset_hh, prev_gmt_offset_mm;
    unsigned int is_dst;
    char *tz;
    int hh_diff, mm_diff;
};

struct rdate_prev_data {
    struct ical_timezone_data data;
    struct rdate_prev_data   *next;
};

read_file(const char *file_name, const struct stat *file_stat)
{
    FILE *file;

    if (debug)
        printf("\n***** size of file %s is %d bytes *****\n", file_name
                , file_stat->st_size);
    in_buf = malloc(file_stat->st_size);
    in_head = in_buf;
    in_tail = in_buf + file_stat->st_size - 1;
    file = fopen(file_name, "r");
    fread(in_buf, 1, file_stat->st_size, file);
    fclose(file);
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
    if (debug > 1)
        printf("file id: %s\n", in_head);
    if (strncmp((char *)in_head, "TZif", 4)) { /* we accept version 1 and 2 */
        printf("tz infile does not look like tz file. Ending\n"
                , in_file, in_head);
        return(1);
    }
    /* header */
    in_head += 4; /* type */
    in_head += 16; /* reserved */
    gmtcnt  = get_long();
    if (debug > 1)
        printf("gmtcnt=%u \n", gmtcnt);
    stdcnt  = get_long();
    if (debug > 1)
        printf("stdcnt=%u \n", stdcnt);
    leapcnt = get_long();
    if (debug > 1)
        printf("leapcnt=%u \n", leapcnt);
    timecnt = get_long();
    if (debug > 1)
        printf("number of time changes: timecnt=%u \n", timecnt);
    typecnt = get_long();
    if (debug > 1)
        printf("number of time change types: typecnt=%u \n", typecnt);
    charcnt = get_long();
    if (debug > 1)
        printf("lenght of different timezone names table: charcnt=%u \n"
                , charcnt);
    return(0);
}

process_local_time_table()
{ /* points when time changes */
    unsigned long tmp;
    int i;

    begin_timechanges = in_head;
    if (debug > 1)
        printf("\n***** printing time change dates *****\n");
    for (i = 0; i < timecnt; i++) {
        tmp = get_long();
        if (debug > 1) {
            printf("GMT %d: %u =  %s", i, tmp
                    , asctime(gmtime((const time_t*)&tmp)));
            printf("LOC %d: %u =  %s", i, tmp
                    , asctime(localtime((const time_t*)&tmp)));
        }
    }
}

process_local_time_type_table()
{ /* pointers to table, which explain how time changes */
    unsigned char tmp;
    int i;

    begin_timechangetypeindexes = in_head;
    if (debug > 1)
        printf("\n***** printing time change type indekses *****\n");
    for (i = 0; i < timecnt; i++) { /* we need to walk over the table */
        tmp = in_head[0];
        in_head++;
        if (debug > 1)
            printf("type %d: %d\n", i, (unsigned int)tmp);
    }
}

process_ttinfo_table()
{ /* table of different time changes = types */
    long tmp;
    unsigned char tmp2, tmp3;
    int i;

    begin_timechangetypes = in_head;
    if (debug > 1)
        printf("\n***** printing different time change types *****\n");
    for (i = 0; i < typecnt; i++) { /* we need to walk over the table */
        tmp = get_long();
        tmp2 = in_head[0];
        in_head++;
        tmp3 = in_head[0];
        in_head++;
        if (debug > 1)
            printf("%d: gmtoffset:%d isdst:%d abbr:%d\n", i, tmp
                    , (unsigned int)tmp2, (unsigned int)tmp3);
    }
}

process_abbr_table()
{
    unsigned char *tmp;
    int i;

    begin_timezonenames = in_head;
    if (debug > 1)
        printf("\n***** printing different timezone names *****\n");
    tmp = in_head;
    for (i = 0; i < charcnt; i++) { /* we need to walk over the table */
        if (debug > 1)
            printf("Abbr:%d (%d)(%s)\n", i, strlen((char *)(tmp + i)), tmp + i);
        i += strlen((char *)(tmp + i));
    }
    in_head += charcnt;
}

process_leap_table()
{
    unsigned long tmp, tmp2;
    int i;

    if (debug > 1)
        printf("\n***** printing leap time table *****\n");
    for (i = 0; i < leapcnt; i++) { /* we need to walk over the table */
        tmp = get_long();
        tmp2 = get_long();
        if (debug > 1)
            printf("leaps %d: %u =  %s (%u)", i, tmp
                    , asctime(localtime((const time_t *)&tmp)), tmp2);
    }
}

process_std_table()
{
    unsigned char tmp;
    int i;

    if (debug > 1)
        printf("\n***** printing std table *****\n");
    for (i = 0; i < stdcnt; i++) { /* we need to walk over the table */
        tmp = (unsigned long)in_head[0];
        in_head++;
        if (debug > 1)
            printf("stds %d: %d\n", i, (unsigned int)tmp);
    }
}

process_gmt_table()
{
    unsigned char tmp;
    int i;

    if (debug > 1)
        printf("\n***** printing gmt table *****\n");
    for (i = 0; i < gmtcnt; i++) { /* we need to walk over the table */
        tmp = (unsigned long)in_head[0];
        in_head++;
        if (debug > 1)
            printf("gmts %d: %d\n", i, (unsigned int)tmp);
    }
}

/* go through the contents of the file and find the positions of 
 * needed data. Uses global pointer: in_head */
int process_file()
{
    if (process_header())
        return(1);
    process_local_time_table();
    process_local_time_type_table();
    process_ttinfo_table();
    process_abbr_table();
    process_leap_table();
    process_std_table();
    process_gmt_table();
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

int create_ical_directory(const char *in_file_name)
{
    char *ical_dir_name;

    ical_dir_name = strdup(&in_file_name[in_file_base_offset]);
    if (mkdir(ical_dir_name, 0777))
    {
        printf("create_ical_directory: creating directory=(%s) failed\n"
                , ical_dir_name);
        perror("\tmkdir");
    }
    free(ical_dir_name);
}

int create_ical_file(const char *in_file_name)
{
    struct stat out_stat;
    char ical_ending[]=".ics";
    int in_file_name_len, ical_ending_len, out_file_name_len;

    if (out_file == NULL) { /* this is the normal case */
        in_file_name_len = strlen(&in_file_name[in_file_base_offset]);
        ical_ending_len = strlen(ical_ending);
        out_file_name_len = in_file_name_len + ical_ending_len;

        out_file = malloc(out_file_name_len + 1);
        strncpy(out_file, &in_file_name[in_file_base_offset], in_file_name_len);
        out_file[in_file_name_len] = '\0';
        strncat(out_file, ical_ending, ical_ending_len);

        timezone_name = strdup(&in_file_name[in_file_base_offset 
                + strlen("zoneinfo/")]);
        printf("create_ical_file: infile:(%s). infile-ending:(%s) time_zone:(%s)\n"
                , in_file_name, out_file, timezone_name);
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
                        perror("\tmkdir");
                    }
                    else {
                        printf("create_ical_file: creating directory=(%s) failed\n"
                                , out_file);
                        perror("\tmkdir");
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
        , struct ical_timezone_data prev) {
    unsigned long tc_time;
    long gmt_offset;
    unsigned int tct_i, abbr_i;
    struct ical_timezone_data data;

    /* get timechange time */
    in_head = begin_timechanges;
    in_head += 4*i; /* point to our row */
    tc_time = get_long();
    localtime_r((const time_t *)&tc_time, &data.start_time );

    /* get timechange type index */
    in_head = begin_timechangetypeindexes;
    tct_i = (unsigned int)in_head[i];

    /* get timechange type */
    in_head = begin_timechangetypes;
    in_head += 6*tct_i;
    gmt_offset = get_long();
    data.gmt_offset_hh = gmt_offset / (60*60);
    data.gmt_offset_mm = (gmt_offset - data.gmt_offset_hh * (60*60)) / 60;
    data.is_dst = in_head[0];
    abbr_i =  in_head[1];

    /* get timezone name */
    in_head = begin_timezonenames;
    data.tz = in_head + abbr_i;

    /* ical needs the startime in the previous (=current) time, so we need to
     * adjust by the difference */
    data.hh_diff = prev.gmt_offset_hh - data.gmt_offset_hh;
    data.mm_diff = prev.gmt_offset_mm - data.gmt_offset_mm;

    if (data.hh_diff + data.start_time.tm_hour < 0 
    ||  data.hh_diff + data.start_time.tm_hour > 23
    ||  data.mm_diff + data.start_time.tm_min < 0 
    ||  data.mm_diff + data.start_time.tm_min > 59) {
        if (debug > 1)
            printf("Error counting startime %s:\n", timezone_name);
        data.hh_diff = 0;
        data.mm_diff = 0;
    }
    /* we need to remember also the previous value. Note that this is from
     * dst if we are not in std and vice versa */
    data.prev_gmt_offset_hh = prev.gmt_offset_hh;
    data.prev_gmt_offset_mm = prev.gmt_offset_mm;

    return(data);
}

int wit_get_rrule(struct ical_timezone_data prev
        , struct ical_timezone_data cur) {
    int monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int rrule_day_cnt;
    int leap_year = 0, prev_leap_year = 0;

    printf("wit_get_rrule: start (%d/%d/%d)\n",cur.start_time.tm_year+1900,cur.start_time.tm_mon+1,cur.start_time.tm_mday);
    if (cur.gmt_offset_hh == prev.gmt_offset_hh
    &&  cur.gmt_offset_mm == prev.gmt_offset_mm
    &&  !strcmp(cur.tz, prev.tz)) {
        /* 2) check if we can use RRULE. 
         * We only check yearly same month and same week day changing
         * rules (which should cover most real world cases). */
        if (cur.start_time.tm_year == prev.start_time.tm_year + 1
        &&  cur.start_time.tm_mon  == prev.start_time.tm_mon
        &&  cur.start_time.tm_wday == prev.start_time.tm_wday
        &&  cur.start_time.tm_hour == prev.start_time.tm_hour
        &&  cur.start_time.tm_min  == prev.start_time.tm_min
        &&  cur.start_time.tm_sec  == prev.start_time.tm_sec
        &&  cur.hh_diff == prev.hh_diff
        &&  cur.mm_diff == prev.mm_diff) {
    printf("\twit_get_rrule: yes\n");
            /* so far so good, now check that our weekdays are on
             * the same week */
            if (prev.start_time.tm_mon == 1) {
                /* hopefully we never come here FIXME */
                printf("leap year danger. I can not handle this!\n");
                if (((prev.start_time.tm_year%4) == 0)
                && (((prev.start_time.tm_year%100) != 0) 
                    || ((prev.start_time.tm_year%400) == 0)))
                    prev_leap_year = 1; /* leap year, february has 29 days */
            }
            /* most often these change on the last week day
             * (and on sunday, but that does not matter now) */
            if ((monthdays[cur.start_time.tm_mon] + leap_year - 
                    cur.start_time.tm_mday) < 7
            &&  (monthdays[prev.start_time.tm_mon] + prev_leap_year - 
                    prev.start_time.tm_mday) < 7) {
                /* yep, it is last */
                rrule_day_cnt = -1;
            }
            else if (cur.start_time.tm_mday < 8
                 &&  prev.start_time.tm_mday < 8) {
                /* ok, it is first */
                rrule_day_cnt = 1;
            }
            else if (cur.start_time.tm_mday < 15
                 &&  prev.start_time.tm_mday < 15
                 /* prevent moving from rule 1 (prev) to rule 2 (cur) */
                 &&  cur.start_time.tm_mday >= 8 
                 &&  prev.start_time.tm_mday >= 8) {
                /* fine, it is second */
                rrule_day_cnt = 2;
            }
            else if (cur.start_time.tm_mday < 22
                 &&  prev.start_time.tm_mday < 22
                 /* prevent moving from rule 1 or 2 (prev) to rule 3 (cur) */
                 &&  cur.start_time.tm_mday >= 15 
                 &&  prev.start_time.tm_mday >= 15) {
                /* must be the third then */
                rrule_day_cnt = 3;
            }
            else { 
                /* give up, it did not work after all.
                 * It is quite possible that rule changed, but
                 * in that case we need to write this out anyway */
                printf("failed in finding RRULE! (%d->%d)\n"
                        , prev.start_time.tm_mday, cur.start_time.tm_mday);
                rrule_day_cnt = 100; /* RDATE is still possible */
            }
            if (rrule_day_cnt < 10)
    printf("\t\twit_get_rrule: rrule\n");
            else
    printf("\t\twit_get_rrule: rdate !\n");
        }
        else { /* 2) failed, need to use RDATE */
    printf("\twit_get_rrule: rdate\n");
            rrule_day_cnt = 100;
        }
    }
    else { /* 1) not possible to use RRULE nor RDATE, need new entry */
        rrule_day_cnt = 0;
    }
    return(rrule_day_cnt);
}

void wit_write_data(int rrule_day_cnt, struct rdate_prev_data *rdate
        , struct ical_timezone_data old
        , struct ical_timezone_data first
        , struct ical_timezone_data prev)
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

    printf("wit_write_data: start(%d)\n", rrule_day_cnt);
    if (rrule_day_cnt > 10 && rdate == NULL) {
        /* we actually have nothing to print. This happens seldom, but
         * is possible if we found RDATE rule, but after that we actually
         * found that it is the first RRULE element and we did not push
         * anything into the rdate store */
        return;
    }
    if (prev.is_dst)
        write_ical_str(dst_begin);
    else
        write_ical_str(std_begin);

    len = snprintf(str, 30, "TZOFFSETFROM:%+03d%02d\n"
            , prev.prev_gmt_offset_hh, prev.prev_gmt_offset_mm);
    fwrite(str, 1, len, ical_file);

    len = snprintf(str, 30, "TZOFFSETTO:%+03d%02d\n"
            , prev.gmt_offset_hh, prev.gmt_offset_mm);
    fwrite(str, 1, len, ical_file);

    len = snprintf(str, 99, "TZNAME:%s\n", prev.tz);
    fwrite(str, 1, len, ical_file);

    len = snprintf(str, 30, "DTSTART:%04d%02d%02dT%02d%02d%02d\n"
            , first.start_time.tm_year + 1900
            , first.start_time.tm_mon  + 1
            , first.start_time.tm_mday
            , first.start_time.tm_hour + first.hh_diff
            , first.start_time.tm_min  + first.mm_diff
            , first.start_time.tm_sec);
    fwrite(str, 1, len, ical_file);

    if (rrule_day_cnt) { /* we had repeating appointment */
        if (rrule_day_cnt < 10) { /* RRULE */
            len = snprintf(str, 50, "RRULE:FREQ=YEARLY;BYMONTH=%d;BYDAY=%d%s\n"
                    , first.start_time.tm_mon + 1
                    , rrule_day_cnt
                    , day[first.start_time.tm_wday]);
            fwrite(str, 1, len, ical_file);
        }
        else { /* RDATE */
    printf("\twit_write_data: rdate(%d) (%x)\n", rrule_day_cnt, rdate);
            for (tmp_data = rdate; tmp_data ; ) {
    printf("\twit_write_data: pop(%d)\n", rrule_day_cnt);
                tmp_prev = tmp_data->data;
                len = snprintf(str, 30, "RDATE:%04d%02d%02dT%02d%02d%02d\n"
                        , tmp_prev.start_time.tm_year + 1900
                        , tmp_prev.start_time.tm_mon  + 1
                        , tmp_prev.start_time.tm_mday
                        , tmp_prev.start_time.tm_hour + tmp_prev.hh_diff
                        , tmp_prev.start_time.tm_min  + tmp_prev.mm_diff
                        , tmp_prev.start_time.tm_sec);
                fwrite(str, 1, len, ical_file);

                tmp_data2 = tmp_data;
                tmp_data = tmp_data->next;
                free(tmp_data2);
            }
        }
    }
    else {
        len = snprintf(str, 30, "RDATE:%04d%02d%02dT%02d%02d%02d\n"
                , prev.start_time.tm_year + 1900
                , prev.start_time.tm_mon  + 1
                , prev.start_time.tm_mday
                , prev.start_time.tm_hour + prev.hh_diff
                , prev.start_time.tm_min  + prev.mm_diff
                , prev.start_time.tm_sec);
        fwrite(str, 1, len, ical_file);
    }

    if (prev.is_dst)
        write_ical_str(dst_end);
    else
        write_ical_str(std_end);
}

void write_ical_timezones()
{
    int i;
    struct ical_timezone_data ical_data
        , ical_data_prev = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, NULL, 0, 0}
        , data_cur_std
        , data_prev_std = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, NULL, 0, 0}
        , data_first_std = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, NULL, 0, 0}
        , data_old_std = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, NULL, 0, 0}
        , data_cur_dst
        , data_prev_dst = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, NULL, 0, 0}
        , data_first_dst = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, NULL, 0, 0}
        , data_old_dst = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, NULL, 0, 0};
    int dst_init_done = 0;
    int dst_start = 0;
    int std_init_done = 0;
    int std_start = 0;
    int rrule_day_cnt_dst = 0, rrule_day_cnt_dst_prev = 0;
    int rrule_day_cnt_std = 0, rrule_day_cnt_std_prev = 0;
    struct rdate_prev_data *prev_dst_data = NULL, *prev_std_data = NULL
        , *tmp_data = NULL, *tmp_data2 = NULL;

    printf("write_ical_timezones: start\n");
    /* we are processing "timezone_name" so we know it exists in this system,
     * so that it is safe to use that in the localtime conversion */
    if (setenv("TZ", timezone_name, 1)) {
        printf("changing timezone=(%s) failed\n", timezone_name);
        perror("\tsetenv");
        return;
    }
    for (i = 0; i < timecnt; i++) {
    /***** get data *****/
        ical_data = wit_get_data(i, ical_data_prev);
        if (ical_data.is_dst) {
            data_cur_dst = ical_data;
            dst_start = 1;
        }
        else {
            data_cur_std = ical_data;
            std_start = 1;
        }
        if (i == 0) { /* first round, do starting values */
            ical_data_prev = ical_data;
        }
        if (!dst_init_done && ical_data.is_dst) {
            data_old_dst   = ical_data;
            data_first_dst = ical_data;
            data_prev_dst  = ical_data;
            dst_init_done  = 1;
            if (debug > 0)
                printf("init %d =  (%s)%s", i
                        , ical_data.is_dst ? "dst" : "std"
                        , asctime(&ical_data.start_time));
            continue; /* we never write the first record */
        }
        else if (!std_init_done && !ical_data.is_dst) {
            data_old_std   = ical_data;
            data_first_std = ical_data;
            data_prev_std  = ical_data;
            std_init_done = 1;
            if (debug > 0)
                printf("init %d =  (%s)%s", i
                        , ical_data.is_dst ? "dst" : "std"
                        , asctime(&ical_data.start_time));
            continue;
        }


    /***** check if we need this data *****/
        /* we only take newer than threshold values */
        if (ical_data.start_time.tm_year + 1900 <= ignore_older) {
            if (debug > 0)
                printf("skip %d =  (%s)%s", i
                        , ical_data.is_dst ? "dst" : "std"
                        , asctime(&ical_data.start_time));
            ical_data_prev = ical_data;
            if (ical_data.is_dst) {
                data_old_dst   = data_prev_dst;
                data_first_dst = ical_data;
                data_prev_dst  = ical_data;
                rrule_day_cnt_dst_prev = rrule_day_cnt_dst;
                rrule_day_cnt_dst = wit_get_rrule(data_prev_dst, ical_data);
            }
            else {
                data_old_std   = data_prev_std;
                data_first_std = ical_data;
                data_prev_std  = ical_data;
                rrule_day_cnt_std_prev = rrule_day_cnt_std;
                rrule_day_cnt_std = wit_get_rrule(data_prev_std, ical_data);
            }
            continue;
        }
        if (debug > 0)
            printf("process %d: (%s) from %02d:%02d to %02d:%02d (%s) %s", i
                    , ical_data.is_dst ? "dst" : "std"
                    , ical_data.prev_gmt_offset_hh,ical_data.prev_gmt_offset_mm
                    , ical_data.gmt_offset_hh, ical_data.gmt_offset_mm
                    , ical_data.tz
                    , asctime(&ical_data.start_time));
    /***** check if we can shortcut the entry with RRULE or RDATE *****/
        /* 1) check if it is similar to the previous values */
        if (ical_data.is_dst) {
            rrule_day_cnt_dst_prev = rrule_day_cnt_dst;
            rrule_day_cnt_dst = wit_get_rrule(data_prev_dst, ical_data);
            if (rrule_day_cnt_dst
            && (rrule_day_cnt_dst == rrule_day_cnt_dst_prev
                && rrule_day_cnt_dst_prev)) {
                if (rrule_day_cnt_dst < 10) {
                    /* if we found RRULE, we do not have to do anything 
                     * since this just continues. */
                    data_prev_dst = ical_data;
                }
                else { 
                    /* we actually found RDATE */
            printf("\t\tpush %d =  (%s)%s", i
                    , data_prev_dst.is_dst ? "dst" : "std"
                    , asctime(&data_prev_dst.start_time));
                    for (tmp_data = prev_dst_data; tmp_data ; ) {
            printf("\t\t\tpush skip %d =  (%s)%s", i
                    , tmp_data->data.is_dst ? "dst" : "std"
                    , asctime(&tmp_data->data.start_time));
                        /* find the last one, which is still empty */
                        tmp_data2 = tmp_data;
                        tmp_data  = tmp_data->next;
                    }
                    tmp_data = malloc(sizeof(struct rdate_prev_data));
                    tmp_data->data = data_prev_dst;
                    tmp_data->next = NULL; /* last */
                    if (!prev_dst_data) {
                        prev_dst_data = tmp_data;
                    }
                    else {
                        tmp_data2->next = tmp_data;
                    }
                    data_old_dst  = data_prev_dst;
                    data_prev_dst = ical_data;
                }
            }
            else { /* not RRULE or we changed to/from RRULE from/to RDATE, 
                    * so write previous and init new round */
                wit_write_data(rrule_day_cnt_dst_prev, prev_dst_data
                        , data_old_dst, data_first_dst, data_prev_dst);
                if (rrule_day_cnt_dst_prev > 10 && rrule_day_cnt_dst < 10) {
                    /* we had RDATE and now we found RRULE */
                    /* data_old_dst stays to point the previous RDATE */
                    /* this was actually the first RRULE */
                    data_first_dst = data_prev_dst; 
                }
                else {
                    data_old_dst   = data_prev_dst;
                    data_first_dst = ical_data;
                }
                data_prev_dst  = ical_data;
                prev_dst_data  = NULL;
            } 
        }
        else { /* !(ical_data.is_dst) */
            rrule_day_cnt_std_prev = rrule_day_cnt_std;
            rrule_day_cnt_std = wit_get_rrule(data_prev_std, ical_data);
            if (rrule_day_cnt_std
            && (rrule_day_cnt_std == rrule_day_cnt_std_prev
                && rrule_day_cnt_std_prev)) {
                if (rrule_day_cnt_std < 10) {
                    /* if we found RRULE, we do not have to do anything 
                     * since this just continues. */
                    data_prev_std = ical_data;
                }
                else { 
                    /* we actually found RDATE */
            printf("\t\tpush %d =  (%s)%s", i
                    , data_prev_std.is_dst ? "dst" : "std"
                    , asctime(&data_prev_std.start_time));
                    for (tmp_data = prev_std_data; tmp_data ; ) {
            printf("\t\t\tpush skip %d =  (%s)%s", i
                    , tmp_data->data.is_dst ? "dst" : "std"
                    , asctime(&tmp_data->data.start_time));
                        /* find the last one, which is still empty */
                        tmp_data2 = tmp_data;
                        tmp_data  = tmp_data->next;
                    }
                    tmp_data = malloc(sizeof(struct rdate_prev_data));
                    tmp_data->data = data_prev_std;
                    tmp_data->next = NULL; /* last */
                    if (!prev_std_data) {
                        prev_std_data = tmp_data;
                    }
                    else {
                        tmp_data2->next = tmp_data;
                    }
                    data_old_std  = data_prev_std;
                    data_prev_std = ical_data;
                }
            }
            else { /* not RRULE or we changed to/from RRULE from/to RDATE, 
                    * so write previous and init new round */
                wit_write_data(rrule_day_cnt_std_prev, prev_std_data
                        , data_old_std, data_first_std, data_prev_std);
                if (rrule_day_cnt_std_prev > 10 && rrule_day_cnt_std < 10) {
                    /* we had RDATE and now we founf RRULE */
                    /* data_old_std stays to point the previous RDATE */
                    /* this was actually the first RRULE */
                    data_first_std = data_prev_std; 
                }
                else {
                    data_old_std   = data_prev_std;
                    data_first_std = ical_data;
                }
                data_prev_std  = ical_data;
                prev_std_data  = NULL;
            } 
        }
        ical_data_prev = ical_data;
    } /* for (i = 0; i < timecnt; i++) */
    /* need to write the last one also */
    printf("*****writing last entry*****\n");
    wit_write_data(rrule_day_cnt_std_prev, prev_std_data
            , data_old_std, data_first_std, data_prev_std);
    wit_write_data(rrule_day_cnt_dst_prev, prev_dst_data
            , data_old_dst, data_first_dst, data_prev_dst);
    printf("write_ical_timezones: end\n");
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
    if (create_ical_file(in_file_name))
        return(1);
    write_ical_header();
    write_ical_timezones();
    write_ical_ending();
    fclose(ical_file);
    return(0);
}

int par_version()
{
    printf(
        "orage_tz_to_ical_convert version (Orage utility) 0.1\n"
        "Copyright © 2008 Juha Kautto\n"
        "License: GNU GPL <http://gnu.org/licenses/gpl.html>\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY.\n"
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
        "\t -v, --version\t\t orage_tz_to_ical_convert version\n"
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

int process_parameters(int argc, char *argv[])
{
    int i, ret = 0;

	printf("process_parameters: alkoi argc=%i\n", argc);
    for (i=1; i < argc; i++) { /* skip own program name and start from 1 */
        if (i == 1 && argv[1][0] != '-') {
        /* asume that we got infile name in first parameter */
            in_file = strdup(argv[1]);
            continue; /* this parameter has been processed */
        }
        if (i == 2 && argv[2][0] != '-' && argv[1][0] != '-' ) {
        /* asume that we got outfile name in second parameter */
            out_file = strdup(argv[i]);
            continue; /* this parameter has been processed */
        }
        if (ret)
            return(ret);
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-?") 
        ||  !strcmp(argv[i], "--help"))
            ret = par_help();
        else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
            ret = par_version();
        else if (!strcmp(argv[i], "-i") || !strncmp(argv[i], "--infile", 8)) {
            if (argv[i][1] == '-' && argv[i][8] == '=')
                /* long form: --infile= */
                in_file = strdup((char *)&argv[i][9]);
            else { /* short form => next parameter is file name */
                if ((++i) < argc) /* we have this parameter */
                    in_file = strdup(argv[i]);
                else {
                    printf("Missing tz (=in) file name in %s.\n", argv[i-1]);
                    ret = 2;
                }
            }
        }
        else if (!strcmp(argv[i], "-o") || !strncmp(argv[i], "--outfile", 9)) {
            if (argv[i][1] == '-' && argv[i][9] == '=')
                /* long form: --outfile= */
                out_file = strdup((char *)&argv[i][10]);
            else { /* short form => next parameter is file name */
                if ((++i) < argc) /* we have this parameter */
                    out_file = strdup(argv[i]);
                else {
                    printf("Missing ical (=out) file name in %s.\n", argv[i-1]);
                    ret = 2;
                }
            }
        }
        else if (!strcmp(argv[i], "-d") || !strncmp(argv[i], "--debug", 9)) {
            if (argv[i][1] == '-' && argv[i][7] == '=')
                debug = atoi((char *)&argv[i][8]);
            else
                if ((++i) < argc) /* we have this parameter */
                    debug = atoi(argv[i]);
                else {
                    printf("Missing debug level number %s.\n", argv[i-1]);
                    ret = 2;
                }
        }
        else {
            printf("Unknown parameter %s.\n", argv[i]);
            ret = 2;
        }
    }
	printf("process_parameters: loppui\n");
    return(ret);
}

int file_call(const char *file_name, const struct stat *sb, int flags
        , struct FTW *f)
{
    printf("\tfile_call: name=(%s)\n", file_name);
    /* we are only interested about files and directories we can access */
    if (flags == FTW_F) {
        printf("\t\tfile_call: processing file=(%s)\n", file_name);
        read_file(file_name, sb);
        process_file();
        write_ical_file(file_name, sb);
        free(in_buf);
        free(out_file);
        out_file = NULL;
        free(timezone_name);
    }
    else if (flags == FTW_D) {
        printf("\t\tfile_call: processing directory=(%s)\n", file_name);
        create_ical_directory(file_name);
    }
    else if (flags == FTW_SL)
        printf("\t\tfile_call: skipping symbolic link=(%s)\n", file_name);
    else
        printf("\t\tfile_call: skipping inaccessible file=(%s)\n", file_name);

    return(0); /* continue */
}

int extract_base_directory()
{
    char *s_tz, *last_tz = NULL, tz[]="/zoneinfo";
    int tz_len;

        printf("starting \n");
    if (in_file[0] != '/') {
        printf("in_file name (%s) is not absolute file name. Ending\n"
                , in_file);
        return(1);
    }

    /* find last "/zoneinfo" from the directory name. Normally there is only
     * one. It needs to be at the end of the string or be followed by '/' */
    tz_len = strlen(tz);
    s_tz = in_file;
    for (s_tz = strstr(s_tz, tz); s_tz != NULL; s_tz = strstr(s_tz, tz)) {
        printf("looping (%s)\n", s_tz);
        if (s_tz[tz_len] == '\0' || s_tz[tz_len] == '/')
            last_tz = s_tz;
        *s_tz++;
    }
    if (last_tz == NULL) {
        printf("in_file name (%s) does not contain (%s). Ending\n"
                , in_file, tz);
        return(2);
    }

    in_file_base_offset = last_tz - in_file + 1; /* skip '/' */

    return(0); /* continue */
}

main(int argc, char *argv[])
{
    int exit_code = 0;

	printf("alkoi\n");
    if (exit_code = process_parameters(argc, argv))
        exit(EXIT_FAILURE); /* help, version or error => end processing */

    if (in_file == NULL) /* in file not found */
        in_file = strdup(DEFAULT_ZONEINFO_DIRECTORY);

    printf("directory handing starts\n");
    if (extract_base_directory()) {
        exit(EXIT_FAILURE);
    }
    /* nftw goes through the whole file structure and calls "file_call"
     * with each file */
    exit_code = nftw(in_file, file_call, 10, FTW_PHYS);
    if (exit_code == -1) {
        perror("nftw error in file handling");
        exit(EXIT_FAILURE);
    }
    printf("directory handing ends\n");

    free(in_file);
	printf("loppui\n");
    exit(EXIT_SUCCESS);
}
