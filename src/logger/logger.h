#ifndef __LOGGER_H__
#define __LOGGER_H__
#include <stdarg.h>
#include <stdio.h>

#define LOG_CACHE_ITEMS 20
#define LOG_CACHE_SIZE 100

#ifdef __cplusplus
extern "C" {
#endif

int benchmark_start(void);
void benchmark_elapsed(const char *label, int start_time);
void time_from_boot(const char *label);

void init_logger(void);
void log_formatted(const char *format, ...);
#define LOGF(...) log_formatted(__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // __LOGGER_H__