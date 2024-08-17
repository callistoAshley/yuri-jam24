#pragma once
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include "macros.h"

#define LOG_BLACK(str) "\x1b[1;30m" str "\x1b[0m"
#define LOG_RED(str) "\x1b[1;31m" str "\x1b[0m"
#define LOG_GREEN(str) "\x1b[1;32m" str "\x1b[0m"
#define LOG_YELLOW(str) "\x1b[1;33m" str "\x1b[0m"
#define LOG_BLUE(str) "\x1b[1;34m" str "\x1b[0m"
#define LOG_MAGENTA(str) "\x1b[1;35m" str "\x1b[0m"
#define LOG_CYAN(str) "\x1b[1;36m" str "\x1b[0m"

void log_info(char *str, ...);
void log_warn(char *str, ...);
void log_error(char *str, ...);
