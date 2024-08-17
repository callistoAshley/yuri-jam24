#include "log.h"

#define STREAM stdout

#define LOG(type)                    \
    {                                \
        va_list args;                \
        va_start(args, str);         \
                                     \
        printf(type " ");            \
        print_time();                \
        vfprintf(STREAM, str, args); \
                                     \
        fprintf(STREAM, "\n");       \
                                     \
        va_end(args);                \
    }

static void print_time(void)
{
    time_t t = time(NULL);
    struct tm *time_info = localtime(&t);

    fprintf(
        STREAM, 
        time_info->tm_sec > 9 ? "%d:%d:%d" : "%d:%d:0%d",        // present an extra 0 before the seconds column if necessary for cleanliness 
        time_info->tm_hour, time_info->tm_min, time_info->tm_sec
    );
    fprintf(STREAM, " ");
}

void log_info(char *str, ...)
{
    LOG("\x1b[1m[INFO]\x1b[0m"); 
}

void log_warn(char *str, ...)
{
    LOG(LOG_YELLOW("[WARN]"));
}

void log_error(char *str, ...)
{
    LOG(LOG_RED("[ERROR]"));
    FATAL("Aborting due to previous error.\n");
}
