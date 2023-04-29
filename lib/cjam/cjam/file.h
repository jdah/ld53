#pragma once

#include <stdio.h>

#include "types.h"
#include "macros.h"

// TODO: windows support
#ifdef PLATFORM_POSIX

#include <sys/stat.h>
#include <unistd.h>

// returns true if file exists
bool file_exists(const char *path);

// get file parent path, returns 0 on success
int file_parent(char *dst, usize n, const char *path);

// returns true if all directories leading up to file exists
bool file_parent_exists(const char *path);

// returns true if path is directory
bool file_isdir(const char *path);

// read entire file into buffer AND null terminate
int file_read_str(const char *path, char **pdata, usize *psz);

// read entire file into buffer
int file_read(const char *path, char **pdata, usize *psz);

// create backup of file at path with specified extension
// NOTE: THIS WILL OVERWRITE IF PATH ALREADY EXISTS WITH EX
int file_makebak(const char *path, const char *ext);

// write string data to file
int file_write_str(const char *path, const char *data);

#ifdef CJAM_IMPL
bool file_exists(const char *path) {
    struct stat s;
    return !stat(path, &s);
}

int file_parent(char *dst, usize n, const char *path) {
    // TODO: bad
    const int res = snprintf(dst, n, "%s", path);
    if (res < 0 || res > (int) n) { return -1; }
    char *end = dst + strlen(dst) - 1;
    while (end != dst && *end != '/') {
        end--;
    }
    if (end == dst) { return -2; }
    *end = '\0';
    return 0;
}

bool file_parent_exists(const char *path) {
    char ppath[1024];
    if (!file_parent(ppath, sizeof(ppath), path)) { return false; }
    return file_exists(ppath);
}

bool file_isdir(const char *path) {
    struct stat s;
    if (!stat(path, &s)) { return false; }
    return !S_ISREG(s.st_mode);
}

// INTERNAL USAGE ONLY
// read entire file into buffer, returns 0 on success
static int _file_read_internal(
    const char *path,
    bool isstr,
    char **pdata,
    usize *psz) {
    FILE *f = fopen(path, "r");
    if (!f) { return -1; }

    fseek(f, 0, SEEK_END);
    const usize sz = ftell(f);
    *psz = sz + (isstr ? 1 : 0);
    rewind(f);
    *pdata = CJAM_ALLOC(*psz, NULL);

    if (fread(*pdata, sz, 1, f) != 1) {
        fclose(f);
        CJAM_FREE(*pdata);
        return -2;
    }

    fclose(f);

    if (isstr) {
        (*pdata)[*psz - 1] = '\0';
    }

    return 0;
}

int file_read_str(const char *path, char **pdata, usize *psz) {
    return _file_read_internal(path, true, pdata, psz);
}

int file_read(const char *path, char **pdata, usize *psz) {
    return _file_read_internal(path, false, pdata, psz);
}

int file_makebak(const char *path, const char *ext) {
    if (!file_exists(path) || file_isdir(path)) { return -1; }

    char newpath[1024];
    const int c = snprintf(newpath, sizeof(newpath), "%s%s", path, ext);
    if (c < 0 || c >= (int) sizeof(newpath)) {
        return -2;
    }

    int res = 0;
    FILE *oldfile = fopen(path, "rb"), *newfile = fopen(newpath, "wb");
    if (!oldfile || !newfile) {
        res = -3;
        goto done;
    }

    // TODO: horrible
    u8 byte;
    int n;
    while ((n = fread(&byte, sizeof(byte), 1, oldfile))) {
        fwrite(&byte, sizeof(byte), 1, newfile);
    }

done:
    if (oldfile) { fclose(oldfile); }
    if (newfile) { fclose(newfile); }
    return res;
}

int file_write_str(const char *path, const char *data) {
    FILE *f = fopen(path, "wb");
    if (!f) { return -1; }
    fprintf(f, "%s", data);
    fclose(f);
    return 0;
}
#endif // ifdef CJAM_IMPL
#endif // ifdef PLATFORM_POSIX
