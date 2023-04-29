#pragma once

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include "types.h"
#include "macros.h"
#include "config.h"

// TODO: better guard
#if false // TODO
ALWAYS_INLINE size_t strnlen(const char *s, size_t maxlen) {
    size_t len;
	for (len = 0; len < maxlen; len++, s++) {
		if (!*s) { break; }
	}
	return len;
}

// see opensource.apple.com/source/Libc/Libc-262/string/strtok.c.auto.html
ALWAYS_INLINE char *strtok_r(char *s, const char *delim, char **last) {
    char *spanp;
    int c, sc;
    char *tok;

    if (s == NULL && (s = *last) == NULL) {
	    return NULL;
    }

    // skip (span) leading delimiters (s += strspn(s, delim), sort of).
cont:
    c = *s++;
    for (spanp = (char*) delim; (sc = *spanp++) != 0; ) {
        if (c == sc) {
            goto cont;
        }
    }

    // no non-delimiter chars
    if (c == 0) {
	    *last = NULL;
	    return NULL;
    }

    tok = s - 1;

    // scan token (scan for delimiters: s += strcspn(s, delim), sort of).
    // note that delim must have one NUL; we stop if we see that, too.
    for (;;) {
	    c = *s++;
	    spanp = (char*) delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0) {
                    s = NULL;
                } else {
                    char *w = s - 1;
                    *w = '\0';
                }
                *last = s;
                return tok;
            }
        } while (sc != 0);
    }

    // not reached
    return NULL;
}

#endif // ifdef CJAM_EMSCRIPTEN

#define STRNFTIME_MAX 128
ALWAYS_INLINE size_t strnftime(
    char *p,
    usize n,
    const char *fmt,
    const struct tm *t) {
    size_t sz = strftime(p, n, fmt, t);
    if (sz == 0) {
        char buf[STRNFTIME_MAX];
        sz = strftime(buf, sizeof buf, fmt, t);
        if (sz == 0) {
            return 0;
        }
        p[0] = 0;
        strncat(p, buf, n - 1);
    }
    return sz;
}

ALWAYS_INLINE void strntimestamp(char *p, usize n) {
    time_t tm = time(NULL);
    strnftime(p, n, "%c", gmtime(&tm));
}

// reads the next line of 'str' into 'buf', or the entire string if it contains
// no newlines
// returns NULL if buf cannot contain line, otherwise returns pointer to next
// line in str (which can be pointer to \0 if this was the last line)
ALWAYS_INLINE const char *strline(const char *str, char *line, usize n) {
    const char *end = strchr(str, '\n');

    if (end) {
        if ((usize) (end - str) > n) { return NULL; }
        memcpy(line, str, end - str);
        line[end - str] = '\0';
        return end + 1;
    } else {
        const usize len = strlen(str);
        if (len > n) { return NULL; }
        memcpy(line, str, len);
        line[len] = '\0';
        return &str[len - 1];
    }
}

// returns 0 if str is prefixed by or equal to pre, otherwise returns 1
ALWAYS_INLINE int strpre(const char *str, const char *pre) {
    while (*str && *pre && *str == *pre) {
        str++;
        pre++;
    }
    return (!*pre && !*str) || (!*pre && *str) ? 0 : 1;
}

// returns 0 if str is suffixed by or equal to pre, otherwise returns 1
ALWAYS_INLINE int strsuf(const char *str, const char *suf) {
    const char
        *e_str = str + strlen(str) - 1,
        *e_suf = suf + strlen(suf) - 1;

    while (e_str != str && e_suf != suf && *e_str== *e_suf) {
        e_str--;
        e_suf--;
    }

    return
        (e_str == str && e_suf == suf) || (e_str != str && e_suf == suf) ?
            0 : 1;
}

// safe snprintf alternative which can be to concatenate onto the end of a
// buffer
//
// char buf[100];
// snprintf(buf, sizeof(buf), "%s", ...);
// xnprintf(buf, sizeof(buf), "%d %f %x", ...);
//
// returns index of null terminator in buf
ALWAYS_INLINE int xnprintf(char *buf, int n, const char *fmt, ...) {
    va_list args;
    int res = 0;
    int len = strnlen(buf, n);
    va_start(args, fmt);
    if (n - len > 0) {
        res = vsnprintf(buf + len, n - len, fmt, args);
    }
    va_end(args);
    return res + len;
}
