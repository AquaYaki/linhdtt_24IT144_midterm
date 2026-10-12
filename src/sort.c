#include "ls.h"
#include <string.h>
#include <stdlib.h>

/* Con tro toan cuc dung trong ham so sanh cua qsort */
static const Options *g_sort_opts = NULL;

/*
 * Lay moc thoi gian can so sanh tuy thuoc vao co:
 * mac dinh mtime, co -c la ctime, co -u la atime
 */
static time_t get_file_timestamp(const FileInfo *fi, TimeType type) {
    if (!fi || !fi->stat_ok) return 0;

    switch (type) {
        case TIME_STATUS:
            return fi->st.st_ctime;
        case TIME_ACCESS:
            return fi->st.st_atime;
        case TIME_MODIFIED:
        default:
            return fi->st.st_mtime;
    }
}

/*
 * Ham so sanh hai FileInfo cho qsort()
 */
static int compare_file_info(const void *a, const void *b) {
    const FileInfo *fa = (const FileInfo *)a;
    const FileInfo *fb = (const FileInfo *)b;
    int res = 0;

    /* 1. Uu tien sap xep theo kich thuoc giam dan (-S) */
    if (g_sort_opts && g_sort_opts->opt_S) {
        if (fa->st.st_size > fb->st.st_size) {
            res = -1; /* Lon nhat dung truoc */
        } else if (fa->st.st_size < fb->st.st_size) {
            res = 1;
        } else {
            /* Kich thuoc bang nhau: tie-breaker theo thu tu tu vung */
            res = strcmp(fa->name, fb->name);
        }
    }
    /* 2. Sap xep theo thoi gian moi nhat dung truoc (-t) */
    else if (g_sort_opts && g_sort_opts->opt_t) {
        time_t ta = get_file_timestamp(fa, g_sort_opts->time_type);
        time_t tb = get_file_timestamp(fb, g_sort_opts->time_type);

        if (ta > tb) {
            res = -1; /* Moi nhat dung truoc */
        } else if (ta < tb) {
            res = 1;
        } else {
            /* Thoi gian bang nhau: tie-breaker theo thu tu tu vung */
            res = strcmp(fa->name, fb->name);
        }
    }
    /* 3. Mac dinh: Sap xep theo thu tu tu vung (lexicographical order) */
    else {
        res = strcmp(fa->name, fb->name);
    }

    /* 4. Dao nguoc thu tu neu co co -r */
    if (g_sort_opts && g_sort_opts->opt_r) {
        res = -res;
    }

    return res;
}

/*
 * Ham sap xep danh sach FileInfo
 */
void sort_files(FileInfo *files, int count, const Options *opts) {
    if (!files || count <= 1) return;

    /* Neu co co -f: Dau ra khong sap xep */
    if (opts && opts->opt_f) return;

    g_sort_opts = opts;
    qsort(files, (size_t)count, sizeof(FileInfo), compare_file_info);
    g_sort_opts = NULL;
}
