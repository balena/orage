/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2008-2011 Juha Kautto  (juha at xfce.org)
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
#ifndef FTW_ACTIONRETVAL
/* BSD systems lack FTW_ACTIONRETVAL, so we need to define needed
 * things like FTW_CONTINUE locally */
#define FTW_CONTINUE 0
#endif

#define DEFAULT_ZONEINFO_DIRECTORY  "/usr/share/zoneinfo"
#define DEFAULT_ZONETAB_FILE        "/usr/share/zoneinfo/zone.tab"
#define TZ_CONVERT_PARAMETER_FILE_NAME "zoneinfo/tz_convert.par"

int debug = 1; /* bigger number => more output */
char version[] = "1.4.4";
int file_cnt = 0; /* number of processed files */

unsigned char *in_buf, *in_head, *in_tail;
int in_file_base_offset = 0;

char *in_file = NULL, *out_file = NULL;
int in_file_is_dir = 0;
int only_one_level = 0;
int excl_dir_cnt = 5;
int no_rrule = 0;
char **excl_dir = NULL;

FILE *ical_file;

/* in_timezone_name is the real timezone name from the infile 
 * we are processing.
 * timezone_name is the timezone we are writing. Usually it is the same
 * than in_timezone_name. 
 * timezone name is for example Europe/Helsinki */
char *in_timezone_name = NULL;
char *timezone_name = NULL;  

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

void read_file(const char *file_name, const struct stat *file_stat)
{
    FILE *file;

    if (debug > 1) {
        printf("read_file: start\n");
        printf("\n***** size of file %s is %d bytes *****\n\n", file_name
                , (int)file_stat->st_size);
    }
    in_buf = malloc(file_stat->st_size);
    in_head = in_buf;
    in_tail = in_buf + file_stat->st_size - 1;
    file = fopen(file_name, "r");
    if (!fread(in_buf, 1, file_stat->st_size, file))
        if (ferror(file)) {
            printf("read_file: file read failed (%s)\n", file_name);
            fclose(file);
            perror("\tfread");
            return;
        }
    fclose(file);
    if (debug > 1)
        printf("read_file: end\n");
}

int get_long()
{
    int tmp;

    tmp = (((int)in_head[0]<<24)
         + ((int)in_head[1]<<16)
         + ((int)in_head[2]<<8)
         +  (int)in_head[3]);
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
        printf("gmtcnt=%lu \n", gmtcnt);
    stdcnt  = get_long();
    if (debug > 2)
        printf("stdcnt=%lu \n", stdcnt);
    leapcnt = get_long();
    if (debug > 2)
        printf("leapcnt=%lu \n", leapcnt);
    timecnt = get_long();
    if (debug > 2)
        printf("number of time changes: timecnt=%lu \n", timecnt);
    typecnt = get_long();
    if (debug > 2)
        printf("number of time change types: typecnt=%lu \n", typecnt);
    charcnt = get_long();
    if (debug > 2)
        printf("lenght of different timezone names table: charcnt=%lu \n"
                , charcnt);
    return(0);
}

void process_local_time_table()
{ /* points when time changes */
    time_t tmp;
    int i;

    begin_timechanges = in_head;
    if (debug > 3)
        printf("\n***** printing time change dates *****\n");
    for (i = 0; i < timecnt; i++) {
        tmp = get_long();
        if (debug > 3) {
            printf("GMT %d: %d =  %s", i, (int)tmp
                    , asctime(gmtime((const time_t*)&tmp)));
            printf("\tLOC %d: %d =  %s", i, (int)tmp
                    , asctime(localtime((const time_t*)&tmp)));
        }
    }
}

void process_local_time_type_table()
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

void process_ttinfo_table()
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
            printf("%d: gmtoffset:%ld isdst:%d abbr:%d\n", i, tmp
                    , (unsigned int)tmp2, (unsigned int)tmp3);
    }
}

void process_abbr_table()
{
    unsigned char *tmp;
    int i;

    begin_timezonenames = in_head;
    if (debug > 3)
        printf("\n***** printing different timezone names *****\n");
    tmp = in_head;
    for (i = 0; i < charcnt; i++) { /* we need to walk over the table */
        if (debug > 3)
            printf("Abbr:%d (%d)(%s)\n", i, (int)strlen((char *)(tmp + i))
                    , tmp + i);
        i += strlen((char *)(tmp + i));
    }
    in_head += charcnt;
}

void process_leap_table()
{
    unsigned long tmp, tmp2;
    int i;

    if (debug > 3)
        printf("\n***** printing leap time table *****\n");
    for (i = 0; i < leapcnt; i++) { /* we need to walk over the table */
        tmp = get_long();
        tmp2 = get_long();
        if (debug > 3)
            printf("leaps %d: %lu =  %s (%lu)", i, tmp
                    , asctime(localtime((const time_t *)&tmp)), tmp2);
    }
}

void process_std_table()
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

void process_gmt_table()
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
        printf("\n\nprocess_file: start\n");
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
        printf("\nprocess_file: end\n\n\n");
    return(0); /* ok */
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

        /* FIXME: it is possible that in_timezone_name and timezone_name
         * do not get any value! Move them outside of this if */
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
                s_dir++;
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
        printf("create_ical_file: end\n\n");
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
    struct ical_timezone_data 
        data = { {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, NULL};

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
    /*
    data.gmt_offset_mm = (data.gmt_offset - data.gmt_offset_hh * (60*60)) / 60;
    */
    data.gmt_offset_mm = abs((data.gmt_offset - data.gmt_offset_hh * (60*60)) 
            / 60);
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

/* Check if we can use repeat rules. We can use them if entries are 
 * similar enough. Possible values to return:
 * RRULE < 10 (We only check if the time happens in same weekday on same week:)
 *          (-1 == last week, 1 == first week, 2 == second week, 3 == 3rd week)
 * RDATE == 100
 * no repeat possible == 0
 * */
int wit_get_repeat_rule(struct ical_timezone_data *prev
        , struct ical_timezone_data *cur) {
    int monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int repeat_rule;
    int cur_leap_year = 0, prev_leap_year = 0;

    if (cur->gmt_offset == prev->gmt_offset
    &&  !strcmp(cur->tz, prev->tz)) {
        /* 2) check if we can use RRULE. 
         * We only check yearly same month and same week day changing
         * rules (which should cover most real world cases). */
        if (!no_rrule
        &&  cur->start_time.tm_year == prev->start_time.tm_year + 1
        &&  cur->start_time.tm_mon  == prev->start_time.tm_mon
        &&  cur->start_time.tm_wday == prev->start_time.tm_wday
        &&  cur->start_time.tm_hour == prev->start_time.tm_hour
        &&  cur->start_time.tm_min  == prev->start_time.tm_min
        &&  cur->start_time.tm_sec  == prev->start_time.tm_sec) {
            /* so far so good, now check that our weekdays are on
             * the same week */
            if (cur->start_time.tm_mon == 1) {
                if (((cur->start_time.tm_year%4) == 0)
                && (((cur->start_time.tm_year%100) != 0) 
                    || ((cur->start_time.tm_year%400) == 0)))
                    cur_leap_year = 1; /* leap year, february has 29 days */
            }
            if (prev->start_time.tm_mon == 1) {
                if (((prev->start_time.tm_year%4) == 0)
                && (((prev->start_time.tm_year%100) != 0) 
                    || ((prev->start_time.tm_year%400) == 0)))
                    prev_leap_year = 1; /* leap year, february has 29 days */
            }
            /* most often these change on the last week day
             * (and on sunday, but that does not matter now) */
            if ((monthdays[cur->start_time.tm_mon] + cur_leap_year - 
                    cur->start_time.tm_mday) < 7
            &&  (monthdays[prev->start_time.tm_mon] + prev_leap_year - 
                    prev->start_time.tm_mday) < 7) {
                /* yep, it is last */
                repeat_rule = -1;
            }
            else if (cur->start_time.tm_mday < 8
                 &&  prev->start_time.tm_mday < 8) {
                /* ok, it is first */
                repeat_rule = 1;
            }
            else if (cur->start_time.tm_mday < 15
                 &&  prev->start_time.tm_mday < 15
                 /* prevent moving from rule 1 (prev) to rule 2 (cur) */
                 &&  cur->start_time.tm_mday >= 8 
                 &&  prev->start_time.tm_mday >= 8) {
                /* fine, it is second */
                repeat_rule = 2;
            }
            else if (cur->start_time.tm_mday < 22
                 &&  prev->start_time.tm_mday < 22
                 /* prevent moving from rule 1 or 2 (prev) to rule 3 (cur) */
                 &&  cur->start_time.tm_mday >= 15 
                 &&  prev->start_time.tm_mday >= 15) {
                /* must be the third then */
                repeat_rule = 3;
            }
            else { 
                /* give up, it did not work after all.
                 * It is quite possible that rule changed, but
                 * in that case we need to write this out anyway */
                repeat_rule = 100; /* RDATE is still possible */
            }
        }
        else { /* 2) failed, need to use RDATE */
            repeat_rule = 100;
        }
    }
    else { /* 1) not possible to use RRULE nor RDATE, need new entry */
        repeat_rule = 0;
    }
    return(repeat_rule);
}

void wit_write_data(int repeat_rule, struct rdate_prev_data **rdate
        , struct ical_timezone_data *first, struct ical_timezone_data *prev
        , struct ical_timezone_data *ical_data)
{
    char str[100], until_date[31];
    int len;
    char *dst_begin="BEGIN:DAYLIGHT\n";
    char *dst_end="END:DAYLIGHT\n";
    char *std_begin="BEGIN:STANDARD\n";
    char *std_end="END:STANDARD\n";
    char *day[] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA" };
    int monthdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    struct rdate_prev_data *tmp_data = NULL, *tmp_data2;
    struct ical_timezone_data tmp_prev;
    struct tm until_time;

    if (debug > 3)
        printf("\tWriting data (%s) %s"
                , first->is_dst ? "dst" : "std"
                , asctime(&first->start_time));

    /****** First check that we really need to write this record *****/
    /* when we have last write (ical_data == NULL), we always need to write */
    if (ical_data) { /* not last, check if write can be omitted */
        if (repeat_rule == 100) { /* RDATE rule */
            for (tmp_data = *rdate; tmp_data ; ) {
                tmp_prev = tmp_data->data;
                if ((tmp_prev.start_time.tm_year + 1900) <= ignore_older) {
                    tmp_data2 = tmp_data;
                    tmp_data = tmp_data->next;
                    *rdate = tmp_data; /* new start */
                    free(tmp_data2);
                    if (debug > 3)
                        printf("\tWrite skipped. Too old RDATE row\n");
                }
                else { /* rows are in order, so we are done */
                    tmp_data = tmp_data->next;
                    if (debug > 3)
                        printf("\tWrite kept. Fresh RDATE row\n");
                }
            }
            if (*rdate == NULL) {
            /* we actually have nothing left to print. */
                if (debug > 2)
                    printf("\tWrite skipped. Too old RDATE entry\n");
                return;
            }
        }
        else if (repeat_rule == 0) { /* non repeating rule */
            if ((prev->start_time.tm_year + 1900) <= ignore_older) {
            /* We have nothing to print. We got non repeating case, which
             * happens before our limit year */
                if (debug > 2)
                    printf("\tWrite skipped. Too old non repeating year\n");
                return;
            }
        }
        /* FIXME: adjust DTSTART with RDATE but also with RRULE */
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

    if (repeat_rule) { /* we had repeating appointment */
        if (repeat_rule < 10) { /* RRULE */
            if (debug > 3)
                printf("\t\t...RRULE\n");
            /* All RRULE ends. We invent end time to be close after last 
             * included date = prev */
            until_time = prev->start_time;
            if (until_time.tm_mday > 28 && until_time.tm_mon == 1) {
                /* we are in february, which is special */
                if (((until_time.tm_year%4) == 0)
                && (((until_time.tm_year%100) != 0) 
                    || ((until_time.tm_year%400) == 0)))
                    ++monthdays[1]; /* leap year, february has 29 days */
            }
            /* goto next day. Note that all our RRULE are yearly, so this
             * is very safe. (We set time also to the end of the day.) */
            if (++until_time.tm_mday > monthdays[until_time.tm_mon]) {
                if (++until_time.tm_mon > 11) {
                    ++until_time.tm_year;
                    until_time.tm_mon = 0;
                }
                until_time.tm_mday = 1;
            }
            len = snprintf(until_date, 30, "%04d%02d%02dT235959Z"
                    , until_time.tm_year + 1900
                    , until_time.tm_mon  + 1
                    , until_time.tm_mday);
            len = snprintf(str, 80
                    , "RRULE:FREQ=YEARLY;BYMONTH=%d;BYDAY=%d%s;UNTIL=%s\n"
                    , first->start_time.tm_mon + 1
                    , repeat_rule
                    , day[first->start_time.tm_wday]
                    , until_date);
            fwrite(str, 1, len, ical_file);
        }
        else { /* RDATE */
            if (debug > 3)
                printf("\t\t...RDATE\n");
            for (tmp_data = *rdate; tmp_data ; ) {
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
        /* dtstart tells the first time */
    }

    if (prev->is_dst)
        write_ical_str(dst_end);
    else
        write_ical_str(std_end);
}

void wit_push_to_rdate_queue(struct rdate_prev_data **p_rdate_data
        , struct ical_timezone_data *p_data_prev)
{
    struct rdate_prev_data *tmp_data = NULL, *tmp_data2 = NULL;

    for (tmp_data = *p_rdate_data; tmp_data ; ) {
        /* find the last one, which is still empty */
        tmp_data2 = tmp_data;
        tmp_data  = tmp_data->next;
    }
    tmp_data = malloc(sizeof(struct rdate_prev_data));
    tmp_data->data = *p_data_prev;
    tmp_data->next = NULL; /* last */
    if (!*p_rdate_data) {
        *p_rdate_data = tmp_data;
    }
    else {
        tmp_data2->next = tmp_data;
    }
}

void write_ical_timezones()
{
    int i;
    /* ical_data        contains the just read new record being processed
     * ical_data_prev   is previous record (needed when getting next data)
     * data_prev_std    is previous std (standard time) record
     * data_prev_dst    is previous dst (daylight saving time) record
     * data_first_std   is first std record before within repeating group.
     * data_first_dst   is first dst record before within repeating group
     *                  DTSTART is based on "first", so it is needed always,
     *                  but it stops incrementing with RRULE and RDATE. For non 
     *                  repeating groups, it is actually the same than prev.
     *                  */
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
    int repeat_rule_std_cur = 0, repeat_rule_std_prev = 0;
    int repeat_rule_dst_cur = 0, repeat_rule_dst_prev = 0;
        /* pointers either to repeat_rule_std_cur and repeat_rule_std_prev
         * or to repeat_rule_dst_cur and repeat_rule_dst_prev. 
         * These avoid having separate code paths for std and dst */
    int *p_repeat_rule_cur = NULL, *p_repeat_rule_prev = NULL;
    /* lists to maintain repeating RDATE group actual dates */
    struct rdate_prev_data *rdate_data_dst = NULL, *rdate_data_std = NULL
        , **p_rdate_data; /* points to rdate_data_dst or to rdate_data_std */

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
                    printf("init time change %d: (%s) from %02d:%02d to %02d:%02d (%s) %s"
                            , i, ical_data.is_dst ? "dst" : "std"
                            , ical_data.prev_gmt_offset_hh, ical_data.prev_gmt_offset_mm
                            , ical_data.gmt_offset_hh, ical_data.gmt_offset_mm
                            , ical_data.tz
                            , asctime(&ical_data.start_time));
                        continue; /* we never write the first record */
                    }
            p_data_first = &data_first_dst;
            p_data_prev = &data_prev_dst;
            p_repeat_rule_cur = &repeat_rule_dst_cur;
            p_repeat_rule_prev = &repeat_rule_dst_prev;
            p_rdate_data = &rdate_data_dst;
        }
        else { /* !dst == std */
            if (!std_init_done) {
                data_first_std = ical_data;
                data_prev_std  = ical_data;
                std_init_done  = 1;
                if (debug > 2)
                    printf("init time change %d: (%s) from %02d:%02d to %02d:%02d (%s) %s"
                            , i, ical_data.is_dst ? "dst" : "std"
                            , ical_data.prev_gmt_offset_hh, ical_data.prev_gmt_offset_mm
                            , ical_data.gmt_offset_hh, ical_data.gmt_offset_mm
                            , ical_data.tz
                            , asctime(&ical_data.start_time));
                continue; /* we never write the first record */
            }
            p_data_first = &data_first_std;
            p_data_prev = &data_prev_std;
            p_repeat_rule_cur = &repeat_rule_std_cur;
            p_repeat_rule_prev = &repeat_rule_std_prev;
            p_rdate_data = &rdate_data_std;
        }


        if (debug > 2)
            printf("time change %d: (%s) from %02d:%02d to %02d:%02d (%s) %s"
                    , i, ical_data.is_dst ? "dst" : "std"
                    , ical_data.prev_gmt_offset_hh, ical_data.prev_gmt_offset_mm
                    , ical_data.gmt_offset_hh, ical_data.gmt_offset_mm
                    , ical_data.tz
                    , asctime(&ical_data.start_time));

    /***** check if we can shortcut the entry with RRULE or RDATE *****/
        /* We have the following possibilities shown by p_repeat_rule_prev/cur:
         *      RRULE == < 10    RDATE == 100     NONE == 0
         *      remember to check first == 0 as RRULE matches that, too
         * (We also know that those pointers always have a value, so null check
         *  is not needed)
         * prev     cur     action
         * RRULE    RRULE   => a) same rule => continue
         *                     b) different rule => write, init new RRULE
         *                          (not possible now as we do not have 
         *                          overlappin rules)
         * RDATE    RRULE   => write, end RDATE, init RRULE
         * NONE     RRULE   => init RRULE
         * RRULE    RDATE   => write, init RDATE
         * RDATE    RDATE   => push RDATE to queue, continue
         * NONE     RDATE   => init RDATE
         * RRULE    NONE    => write
         * RDATE    NONE    => write
         * NONE     NONE    => write
         * */
        *p_repeat_rule_prev = *p_repeat_rule_cur;
        *p_repeat_rule_cur = wit_get_repeat_rule(p_data_prev, &ical_data);
        if (debug > 3)
            printf("\t\t\t***** repeating values prev:%d cur:%d\n"
                    , *p_repeat_rule_prev, *p_repeat_rule_cur);

        if (*p_repeat_rule_cur == 0) {
        /***** NONE begin *****/
            if (*p_repeat_rule_prev == 0) { /* also previously NONE */
                wit_write_data(*p_repeat_rule_prev, p_rdate_data
                        , p_data_first, p_data_prev, &ical_data);
                *p_data_first = ical_data;
                *p_data_prev  = ical_data;
                if (debug > 3)
                    printf("\tNONE -> NONE\n");
            } /* end-ef NONE */
            else if (*p_repeat_rule_prev < 10) { /* previously RRULE */
                wit_write_data(*p_repeat_rule_prev, p_rdate_data
                        , p_data_first, p_data_prev, &ical_data);
                *p_data_first = ical_data;
                *p_data_prev  = ical_data;
                if (debug > 3)
                    printf("\tRRULE -> NONE\n");
            } /* end-of RRULE */
            else if (*p_repeat_rule_prev == 100) { /* previously RDATE */
                /* need to push the last RDATE and write them all. 
                   If we had only one RDATE, it is not mandatory to push it,
                   but it works equally well, so not worth special check */
                wit_push_to_rdate_queue(p_rdate_data, p_data_prev);
                wit_write_data(*p_repeat_rule_prev, p_rdate_data
                        , p_data_first, p_data_prev, &ical_data);
                /* wit_write_data freed rdate memory already */
                *p_rdate_data = NULL;
                *p_data_first = ical_data;
                *p_data_prev  = ical_data;
                if (debug > 3)
                    printf("\tRDATE -> NONE\n");
            } /* end-of RDATE */
            else { /* error, unknown value */
                perror("***** Unknown value in repeating rule *****");
            }
        /***** NONE end *****/
        } 
        else if (*p_repeat_rule_cur < 10 ) {  
        /***** RRULE begin *****/
            if (*p_repeat_rule_prev == 0) { /* previously NONE */
                /* do not write the previous, use it as first RRULE element */
                *p_data_first = *p_data_prev;
                *p_data_prev  = ical_data;
                if (debug > 3)
                    printf("\tNONE -> RRULE\n");
            } /* end-ef NONE */
            else if (*p_repeat_rule_prev < 10) { /* also previously RRULE */
                if (*p_repeat_rule_prev == *p_repeat_rule_cur) {
                /* we had same RRULE previously, so we do not have to do 
                 * anything since this just continues. */
                    *p_data_prev = ical_data;
                }
                else { /* different RRULE (not possible now as we do
                          not have overlapping rules) */
                    printf("\t***** ERROR: change from RRULE to different RRULE \n");
                    wit_write_data(*p_repeat_rule_prev, p_rdate_data
                            , p_data_first, p_data_prev, &ical_data);
                    *p_data_first = ical_data;
                    *p_data_prev  = ical_data;
                }
                if (debug > 3)
                    printf("\tRULE -> RRULE\n");
            } /* end-of RRULE */
            else if (*p_repeat_rule_prev == 100) { /* previously RDATE */
                /* Note that we have not pushed the previous RDATE into the 
                 * rdate queue yet. As we now see that it fist our RRULE, 
                 * we will steal it and use in RRULE.
                 * We do not have to write anything if we only had this one
                 * RDATE */
                if (*p_rdate_data != NULL) {
                    /* we had more than one rdate, need to write those */
                    wit_write_data(*p_repeat_rule_prev, p_rdate_data
                            , p_data_first, p_data_prev, &ical_data);
                }
                /* wit_write_data freed rdate memory already */
                *p_rdate_data = NULL;
                *p_data_first = *p_data_prev;
                *p_data_prev  = ical_data;
                if (debug > 3)
                    printf("\tRDATE -> RRULE\n");
            } /* end-of RDATE */
            else { /* error, unknown value */
                perror("***** Unknown value in repeating rule *****");
            }
        /***** RRULE end *****/
        }
        else if (*p_repeat_rule_cur == 100) {  
        /***** RDATE begin *****/
            if (*p_repeat_rule_prev == 0) { /* previously NONE */
                /* do not write the previous, use it as first RDATE element */
                wit_push_to_rdate_queue(p_rdate_data, p_data_prev);
                *p_data_first = *p_data_prev;
                *p_data_prev  = ical_data;
                if (debug > 3)
                    printf("\tNONE -> RDATE\n");
            } /* end-ef NONE */
            else if (*p_repeat_rule_prev < 10) { /* previously RRULE */
                /* write old RRULE as we rather use it than RDATE, but do not 
                 * push this new value to rdate queue as it is now our first 
                 * rdate and we may not get more. */
                wit_write_data(*p_repeat_rule_prev, p_rdate_data
                        , p_data_first, p_data_prev, &ical_data);
                *p_data_first = ical_data;
                *p_data_prev  = ical_data;
                if (debug > 3)
                    printf("\tRRULE -> RDATE\n");
            } /* end-of RRULE */
            else if (*p_repeat_rule_prev == 100) { /* also previously RDATE */
                /* we only add prev data into the queue and that is it */
                wit_push_to_rdate_queue(p_rdate_data, p_data_prev);
                *p_data_prev  = ical_data;
                if (debug > 3)
                    printf("\tRDATE -> RDATE\n");
            } /* end-of RDATE */
            else { /* error, unknown value */
                perror("***** Unknown value in repeating rule *****");
            }
        /***** RDATE end *****/
        }
        ical_data_prev = ical_data;
    } /* for (i = 0; i < timecnt; i++) */

    /* need to write the last one also */
    if (debug > 3)
        printf("writing last values:\n");
    if (std_init_done)
        wit_write_data(repeat_rule_std_prev, &rdate_data_std
                , &data_first_std, &data_prev_std, NULL);
    else if (debug > 3)
        printf("\tskipping last std write as std init has not been done:\n");

    if (dst_init_done)
        wit_write_data(repeat_rule_dst_prev, &rdate_data_dst
                , &data_first_dst, &data_prev_dst, NULL);
    else if (debug > 3)
        printf("\tskipping last dst write as dst init has not been done:\n");
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
        printf("***** write_ical_file: start *****\n\n");
    if (create_ical_file(in_file_name))
        return(1);
    write_ical_header();
    write_ical_timezones();
    write_ical_ending();
    fclose(ical_file);
    if (debug > 1)
        printf("\n***** write_ical_file: end *****\n\n\n");
    return(0);
}

void write_parameters(const char *par_file_name)
{
    /* FIXME: currently only writing the location of system tz files.
     * It would be good to have a way to influence other parameters also.*/
    FILE *par_file;
    int len;

    if (debug > 1)
        printf("write_parameters: start\n");
    par_file = fopen(par_file_name, "w");
    if (par_file == NULL) { /* error */
        perror("\fopen");
        return; 
    }

    len = in_file_base_offset + strlen("zoneinfo");
    if (in_file[len-1] == '/')
        len--; /* remove trailing / */
    if (len <= strlen(in_file)) {
        fwrite(in_file, 1, len, par_file);
    }
    else {
        printf("write_parameters: error with (%s) %d (%d).\n"
                , in_file, len, in_file_base_offset);
    }

    fclose(par_file);
    if (debug > 1)
        printf("write_parameters: end\n");
}

/*
void read_parameters(const char *par_file_name)
{
    FILE *par_file;
    struct stat par_file_stat;
    char *par_buf;

    if (debug > 1)
        printf("read_parameters: start\n");
    par_file = fopen(ical_zone, "r");
    if (par_file == NULL) { / * does not exist or other error * /
        return; / *nothing to read, just return * /
    }
    if (stat(par_file_name, &par_file_stat) == -1) {
        perror("\tstat");
        fclose(par_file);
        return;
    }

    par_buf = malloc(par_file_stat.st_size+1);
    fread(par_buf, 1, par_file_stat.st_size, par_file);
    if (ferror(par_file)) {
        printf("read_parameters: error reading (%s).\n", par_file_name);
        perror("\tfread");
        free(par_buf);
        fclose(par_file);
        return;
    }

    fclose(ical_zone, "r");
    if (debug > 1)
        printf("read_parameters: end\n");
}
*/

int par_version()
{
    printf(
        "tz_convert version (Orage utility) %s\n"
        "Copyright © 2008-2009 Juha Kautto\n"
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
        "tz_convert converts operating system timezone (tz) files\n"
        "to ical format. Often you find tz files in /usr/share/zoneinfo/\n\n"
        "Usage: tz_convert [in_file] [out_file] [parameters]...\n"
        "parameters:\n"
        "\t -h, -?, --help\t\t this help text\n"
        "\t -V, --version\t\t tz_convert version\n"
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
            , "tz_convert version", NULL},
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
            , "limit processing to newer than this year."
            , "year"},
        {"timezone", 't', POPT_ARG_STRING, &tmp_str, 7
            , "timezone name. For example Europe/Helsinki"
            , "filename"},
        {"norecursive", 'r', POPT_ARG_INT, &only_one_level, 8
            , "process only main directory, instead of all subdirectories."
              " 0 = recursive  1 = only main directory (0=default)."
            , "level"},
        {"exclude count", 'c', POPT_ARG_INT, &excl_dir_cnt, 9
            , "number of excluded directories."
              " 5 = default (You only need this if you have more"
              " than 5 excluded directories)."
            , "count"},
        {"exclude", 'x', POPT_ARG_STRING, &tmp_str, 10
            , "do not process this directory, skip it."
              " You can give several directories with separate parameters."
              " By default directories right and posix are excluded, but if"
              " you use this parameter, you have to specify those also."
              " *** NOTE: This does not work in BSD due to lack of"
              " FTW_ACTIONRETVAL feature in nftw function ***"
            , "directory"},
        {"norrule", 'u', POPT_ARG_INT, &no_rrule, 11
            , "do not use RRULE ical repeating rule, but use RDATE instead."
              " Not all calendars are able to understand RRULE correctly"
              " with timezones. "
              " (Orage should work fine with RRULE)"
              "    0 = use RRULE   1 = do not use RRULE (0=default)."
            , "level"},
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
            case 11:
                if (no_rrule)
                    printf("Not using RRULE (using RDATE instead)\n");
                else
                    printf("Using RRULE when possible\n");
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
    FILE *ical_zone_tab;
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
    if (!fread(ical_zone_buf, 1, ical_zone_stat.st_size, ical_zone_tab)
    && (ferror(ical_zone_tab))) {
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
    if (flags == FTW_F) { /* we got file */
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
        if (process_file(file_name)) { /* we skipped this file */
            free(in_buf);
            return(FTW_CONTINUE);
        }
        write_ical_file(file_name, sb);
        add_zone_tabs();

        free(in_buf);
        free(out_file);
        out_file = NULL;
        free(in_timezone_name);
        free(timezone_name);
    }
    else if (flags == FTW_D) { /* this is directory */
        if (debug > 0)
            printf("\tfile_call: processing directory=(%s)\n", file_name);
#ifdef FTW_ACTIONRETVAL
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
#else
        /* not easy to do that in BSD, where we do not have FTW_ACTIONRETVAL
         * features. It can be done by checking differently */
        if (debug > 0)
            printf("FIXME: this directory should be skipped\n");
#endif
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
        s_tz++;
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
        printf("\tusing rrule: %s\n"
                , no_rrule ? "NO" : "YES");
        printf("***** Parameters *****\n\n");
    }
    if (debug > 1)
        printf("check_parameters: end\n");
    return(0); /* continue */
}

int main(int argc, const char **argv)
{
    if (debug > 1)
        printf("main: start\n");
    /* if (exit_code = process_parameters(argc, argv)) */
    /*
    read_parameters(TZ_CONVERT_PARAMETER_FILE_NAME);
    */
    if (get_parameters_popt(argc, argv))
        exit(EXIT_FAILURE); /* help, version or error => end processing */
    if (check_parameters())
        exit(EXIT_FAILURE);

    /* nftw goes through the whole file structure and calls "file_call"
     * with each file. It returns 0 when everything has been done and -1
     * if it run into an error.
     * BSD lacks FTW_ACTIONRETVAL, so we only use it when available. */
#ifdef FTW_ACTIONRETVAL
    if (nftw(in_file, file_call, 10, FTW_PHYS | FTW_ACTIONRETVAL) == -1) {
#else
    if (nftw(in_file, file_call, 10, FTW_PHYS) == -1) {
#endif
        perror("nftw error in file handling");
        exit(EXIT_FAILURE);
    }

    printf("Processed %d files\n", file_cnt);
    write_parameters(TZ_CONVERT_PARAMETER_FILE_NAME);

    free(in_file);
    if (debug > 1)
        printf("main: end\n");
    exit(EXIT_SUCCESS);
}
