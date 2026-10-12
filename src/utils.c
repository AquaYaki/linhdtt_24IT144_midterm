#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/*
 * Chuyen doi st_mode thanh chuoi quyen 10 ky tu giong chuan BSD/UNIX:
 * Vi du: drwxr-xr-x, -rwsr-xr-x, -rwxr-xr-t
 */
void format_mode_string(mode_t mode, char *out_str) {
    if (!out_str) return;

    /* 1. Xac dinh loai file (ky tu thu nhat) */
    if (S_ISDIR(mode)) {
        out_str[0] = 'd';
    } else if (S_ISLNK(mode)) {
        out_str[0] = 'l';
    } else if (S_ISCHR(mode)) {
        out_str[0] = 'c';
    } else if (S_ISBLK(mode)) {
        out_str[0] = 'b';
    } else if (S_ISFIFO(mode)) {
        out_str[0] = 'p';
#ifdef S_ISSOCK
    } else if (S_ISSOCK(mode)) {
        out_str[0] = 's';
#endif
    } else {
        out_str[0] = '-'; /* Regular file */
    }

    /* 2. Quyen chu so huu (Owner: rwx) */
    out_str[1] = (mode & S_IRUSR) ? 'r' : '-';
    out_str[2] = (mode & S_IWUSR) ? 'w' : '-';
    if (mode & S_ISUID) {
        out_str[3] = (mode & S_IXUSR) ? 's' : 'S';
    } else {
        out_str[3] = (mode & S_IXUSR) ? 'x' : '-';
    }

    /* 3. Quyen nhom (Group: rwx) */
    out_str[4] = (mode & S_IRGRP) ? 'r' : '-';
    out_str[5] = (mode & S_IWGRP) ? 'w' : '-';
    if (mode & S_ISGID) {
        out_str[6] = (mode & S_IXGRP) ? 's' : 'S';
    } else {
        out_str[6] = (mode & S_IXGRP) ? 'x' : '-';
    }

    /* 4. Quyen nguoi khac (Other: rwx) */
    out_str[7] = (mode & S_IROTH) ? 'r' : '-';
    out_str[8] = (mode & S_IWOTH) ? 'w' : '-';
    if (mode & S_ISVTX) {
        out_str[9] = (mode & S_IXOTH) ? 't' : 'T';
    } else {
        out_str[9] = (mode & S_IXOTH) ? 'x' : '-';
    }

    out_str[10] = '\0';
}

/*
 * Dinh dang dung luong dang nguoi doc duoc (-h):
 * Hien thi don vi B, K, M, G, T
 */
void format_human_size(off_t size, char *out_buf, size_t buf_size) {
    if (!out_buf || buf_size == 0) return;

    const char *units[] = {"B", "K", "M", "G", "T", "P"};
    int unit_idx = 0;
    double d_size = (double)size;

    while (d_size >= 1024.0 && unit_idx < 5) {
        d_size /= 1024.0;
        unit_idx++;
    }

    if (unit_idx == 0) {
        snprintf(out_buf, buf_size, "%lldB", (long long)size);
    } else if (d_size < 10.0) {
        snprintf(out_buf, buf_size, "%.1f%s", d_size, units[unit_idx]);
    } else {
        snprintf(out_buf, buf_size, "%.0f%s", d_size, units[unit_idx]);
    }
}

/*
 * Dinh dang thoi gian theo man page NetBSD:
 * abbreviated month, day-of-month, hour and minute (vd: "Oct 27 14:30")
 */
void format_file_time(time_t ftime, char *out_buf, size_t buf_size) {
    if (!out_buf || buf_size == 0) return;

    struct tm *tm_info = localtime(&ftime);
    if (!tm_info) {
        snprintf(out_buf, buf_size, "??? ?? ??:??");
        return;
    }

    strftime(out_buf, buf_size, "%b %e %H:%M", tm_info);
}

/*
 * Ky hieu loai file cho co -F
 */
char get_file_indicator(mode_t mode) {
    if (S_ISDIR(mode)) return '/';
    if (S_ISLNK(mode)) return '@';
#ifdef S_ISSOCK
    if (S_ISSOCK(mode)) return '=';
#endif
    if (S_ISFIFO(mode)) return '|';
    if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) return '*';
    return '\0';
}

/*
 * Thay the cac ky tu khong in duoc thanh '?' khi bat co -q
 */
void sanitize_filename(const char *src, char *dst, size_t dst_size, int opt_q, int opt_w) {
    if (!src || !dst || dst_size == 0) return;

    size_t i = 0;
    for (; src[i] != '\0' && i < dst_size - 1; i++) {
        unsigned char c = (unsigned char)src[i];
        if (opt_q && !opt_w && !isprint(c)) {
            dst[i] = '?';
        } else {
            dst[i] = src[i];
        }
    }
    dst[i] = '\0';
}

/*
 * Lay gia tri bien moi truong BLOCKSIZE (mac dinh 512 bytes)
 */
long get_env_blocksize(void) {
    char *env_val = getenv("BLOCKSIZE");
    if (env_val != NULL && *env_val != '\0') {
        long val = strtol(env_val, NULL, 10);
        if (val > 0) return val;
    }
    return 512;
}

/*
 * Tinh so luong block hien thi tuy theo co -k hoac BLOCKSIZE
 * st_blocks trong POSIX struct stat mac dinh tinh bang don vi 512 bytes
 */
long long calculate_display_blocks(const struct stat *st, int opt_k) {
    if (!st) return 0;

    if (opt_k) {
        /* Chuyen sang block 1024 bytes (1 KB) */
        return (st->st_blocks + 1) / 2;
    }

    long bsize = get_env_blocksize();
    if (bsize == 512) {
        return st->st_blocks;
    }

    /* Quy doi tu don vi 512 bytes sang don vi bsize, lam tron len */
    long long total_bytes = (long long)st->st_blocks * 512;
    return (total_bytes + bsize - 1) / bsize;
}
