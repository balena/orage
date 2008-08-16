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

#define _XOPEN_SOURCE 500
#define __USE_XOPEN_EXTENDED 1
#include <ftw.h>
    /* nftw */

#define DEFAULT_ZONEINFO_DIRECTORY "/usr/share/zoneinfo"

int debug = 1; /* more output */

unsigned char *in_buf, *in_head, *in_tail;
int in_file_base_offset = 0;

char *in_file = NULL, *out_file = NULL;

FILE *ical_file;

char *timezone_name;  /* for example Europe/Helsinki */

int ignore_older = 1990; /* Ignore rules which are older than this */

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
    if (strncmp(in_head, "TZif", 4)) { /* we accept version 1 and 2 */
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
            printf("GMT %d: %u =  %s", i, tmp, asctime(gmtime(&tmp)));
            printf("LOC %d: %u =  %s", i, tmp, asctime(localtime(&tmp)));
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
            printf("Abbr:%d (%d) (%s)\n", i, strlen(tmp + i), tmp + i);
        i += strlen(tmp + i);
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
            printf("leaps %d: %u =  %s (%u)", i, tmp, asctime(localtime(&tmp))
                    , tmp2);
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
        printf("creating directory=(%s) failed\n", ical_dir_name);
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
                printf("creating directory=(%s)\n", out_file);
                if (mkdir(out_file, 0777))
                {
                    printf("creating directory=(%s) failed\n", out_file);
                    perror("\tmkdir");
                    return(2);
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

void write_ical_header()
{
    char *vcalendar = "BEGIN:VCALENDAR\nPRODID:-//Xfce//NONSGML Orage Olson-VTIMEZONE Converter//EN\nVERSION:2.0\n";
    char *vtimezone = "BEGIN:VTIMEZONE\nTZID:/softwarestudio.org/Olson_20011030_5/";
    char *xlic = "X-LIC-LOCATION:";
    char *line = "\n";
    int len;

    len = strlen(vcalendar);
    fwrite(vcalendar, 1, len, ical_file);

    len = strlen(vtimezone);
    fwrite(vtimezone, 1, len, ical_file);

    len = strlen(timezone_name);
    fwrite(timezone_name, 1, len, ical_file);

    len = strlen(line);
    fwrite(line, 1, len, ical_file);

    len = strlen(xlic);
    fwrite(xlic, 1, len, ical_file);

    len = strlen(timezone_name);
    fwrite(timezone_name, 1, len, ical_file);

    len = strlen(line);
    fwrite(line, 1, len, ical_file);
}

void write_ical_timezones()
{
    unsigned long tc_time;
    struct tm *start_time;
    long gmt_offset, gmt_offset_hh, gmt_offset_mm;
    static long prev_gmt_offset_hh=0, prev_gmt_offset_mm=0;
    unsigned int tct_i, is_dst, abbr_i;
    unsigned char *tz;
    char str[100];
    int i, len, hh_diff, mm_diff;
    char *dst_begin="BEGIN:DAYLIGHT\n";
    char *dst_end="END:DAYLIGHT\n";
    char *std_begin="BEGIN:STANDARD\n";
    char *std_end="END:STANDARD\n";

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

    /* get timechange time */
    in_head = begin_timechanges;
    in_head += 4*i;
    tc_time = get_long();
    start_time = localtime(&tc_time);

    /* get timechange type index */
    in_head = begin_timechangetypeindexes;
    tct_i = (unsigned int)in_head[i];

    /* get timechange type */
    in_head = begin_timechangetypes;
    in_head += 6*tct_i;
    gmt_offset = get_long();
    gmt_offset_hh = gmt_offset / (60*60);
    gmt_offset_mm = (gmt_offset - gmt_offset_hh * (60*60)) / 60;
    is_dst = in_head[0];
    abbr_i =  in_head[1];

    /* ical needs the startime in the previous (=current) time, so we need to
     * adjust by the difference */
    if (i==0) {
        prev_gmt_offset_hh = gmt_offset_hh;
        prev_gmt_offset_mm = gmt_offset_mm;
    }
    hh_diff = prev_gmt_offset_hh - gmt_offset_hh;
    mm_diff = prev_gmt_offset_mm - gmt_offset_mm;

    if (hh_diff + start_time->tm_hour < 0 || hh_diff + start_time->tm_hour > 23
    ||  mm_diff + start_time->tm_min < 0 || mm_diff + start_time->tm_min > 59) {
        if (debug > 1)
            printf("Error counting startime %s:\n", timezone_name);
        hh_diff = 0;
        mm_diff = 0;
    }

    /* get timezone name */
    in_head = begin_timezonenames;
    tz = in_head + abbr_i;

    /***** write data *****/

    if (start_time->tm_year + 1900 < ignore_older) {
        if (debug > 2)
            printf("\tskip %d: %u =  %s", i, tc_time, asctime(start_time));
        prev_gmt_offset_hh = gmt_offset_hh;
        prev_gmt_offset_mm = gmt_offset_mm;
        continue;
    }

    /* write timezone entry */
    if (is_dst) {
        len = strlen(dst_begin);
        fwrite(dst_begin, 1, len, ical_file);
    }
    else {
        len = strlen(std_begin);
        fwrite(std_begin, 1, len, ical_file);
    }

    snprintf(str, 99, "TZOFFSETFROM:%+03d%02d\n"
            , prev_gmt_offset_hh, prev_gmt_offset_mm);
    len = strlen(str);
    fwrite(str, 1, len, ical_file);

    snprintf(str, 99, "TZOFFSETTO:%+03d%02d\n", gmt_offset_hh, gmt_offset_mm);
    len = strlen(str);
    fwrite(str, 1, len, ical_file);

    prev_gmt_offset_hh = gmt_offset_hh;
    prev_gmt_offset_mm = gmt_offset_mm;

    len = snprintf(str, 99, "TZNAME:%s\n", tz);
    fwrite(str, 1, len, ical_file);

    len = snprintf(str, 99, "DTSTART:%04d%02d%02dT%02d%02d%02d\n"
            , start_time->tm_year + 1900, start_time->tm_mon + 1
            , start_time->tm_mday
            , start_time->tm_hour + hh_diff, start_time->tm_min + mm_diff
            , start_time->tm_sec);
    fwrite(str, 1, len, ical_file);

    len = snprintf(str, 99, "RDATE:%04d%02d%02dT%02d%02d%02d\n"
            , start_time->tm_year + 1900, start_time->tm_mon + 1
            , start_time->tm_mday
            , start_time->tm_hour + hh_diff, start_time->tm_min + mm_diff
            , start_time->tm_sec);
    fwrite(str, 1, len, ical_file);

    if (is_dst) {
        len = strlen(dst_end);
        fwrite(dst_end, 1, len, ical_file);
    }
    else {
        len = strlen(std_end);
        fwrite(std_end, 1, len, ical_file);
    }
    }
    printf("write_ical_timezones: end\n");
}

void write_ical_ending()
{
    char *end= "END:VTIMEZONE\nEND:VCALENDAR\n";
    int len;

    len = strlen(end);
    fwrite(end, 1, len, ical_file);
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