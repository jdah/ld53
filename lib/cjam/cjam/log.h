#pragma once

#include <stdio.h>
#include <stdarg.h>

#include "macros.h"
#include "types.h"

// returns number of written characters excluding null terminator
int log_fmtap(
    char *buf,
    usize n,
    const char *file,
    int line,
    const char *function,
    const char *prefix,
    const char *fmt,
    va_list ap);

// returns number of written characters excluding null terminator
int log_fmt(
    char *buf,
    usize n,
    const char *file,
    int line,
    const char *function,
    const char *prefix,
    const char *fmt,
    ...);

void log_write(
    const char *file,
    int line,
    const char *function,
    FILE *f,
    const char *prefix,
    const char *fmt,
    ...);

// format as log message
#define LOGFMT(_buf, _n, _fmt, ...)                    \
    log_fmt(                                           \
        (_buf), (_n), __FILE__, __LINE__, __FUNCTION__,\
        "LOG", (_fmt), ##__VA_ARGS__)

// debug log message
#define LOG(_fmt, ...)                                 \
    log_write(                                         \
        __FILE__, __LINE__, __FUNCTION__,              \
        stdout, "LOG", (_fmt), ##__VA_ARGS__)

// debug warn
#define WARN(_fmt, ...)                                \
    log_write(                                         \
        __FILE__, __LINE__, __FUNCTION__,              \
        stderr, "WRN", (_fmt), ##__VA_ARGS__)

// debug error
#define ERROR(_fmt, ...)                               \
    log_write(                                         \
        __FILE__, __LINE__, __FUNCTION__,              \
        stderr, "ERR", (_fmt), ##__VA_ARGS__)

#ifdef CJAM_IMPL

int log_fmtap(
    char *buf,
    usize n,
    const char *file,
    int line,
    const char *function,
    const char *prefix,
    const char *fmt,
    va_list ap) {
    char *p = buf, *end = buf + n;
    p += snprintf(p, end - p, "[%s][%s:%d][%s] ", prefix, file, line, function);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    const int m = vsnprintf(p, end - p, fmt, ap);
    p += m;
#pragma clang diagnostic pop

    // truncated log message/vsnprintf fail
    if (m < 0 || m >= (int) n) {
        fprintf(stderr, "%s\n", "truncated log message");
    }

    if (*p != '\n') {
        p += snprintf(p, end - p, "%c", '\n');
    }

    return p - buf;
}

int log_fmt(
    char *buf,
    usize n,
    const char *file,
    int line,
    const char *function,
    const char *prefix,
    const char *fmt,
    ...) {
    va_list ap;
    va_start(ap, fmt);
    const int res = log_fmtap(buf, n, file, line, function, prefix, fmt, ap);
    va_end(ap);
    return res;
}

void log_write(
    const char *file,
    int line,
    const char *function,
    FILE *f,
    const char *prefix,
    const char *fmt,
    ...) {
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    log_fmtap(buf, sizeof(buf), file, line, function, prefix, fmt, ap);
    va_end(ap);
    fprintf(f, "%s", buf);
}

#endif // ifdef CJAM_IMPL
