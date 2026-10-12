#include "ls.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
#include <sys/types.h>
#else
#include <sys/sysmacros.h>
#endif

#ifndef major
#define major(dev) ((int)(((dev) >> 8) & 0xff))
#endif
#ifndef minor
#define minor(dev) ((int)((dev) & 0xff))
#endif

/*
 * Tinh tong so block cua tat ca cac file trong danh sach de in dong "total <sum>"
 */
static long long calculate_total_blocks(const FileInfo *files, int count, const Options *opts) {
    long long total = 0;
    for (int i = 0; i < count; i++) {
        if (files[i].stat_ok) {
            total += calculate_display_blocks(&files[i].st, opts->opt_k);
        }
    }
    return total;
}

/*
 * Lay chuoi owner (ten hoac so UID)
 */
static void get_owner_string(uid_t uid, int opt_n, char *out, size_t size) {
    if (!opt_n) {
        struct passwd *pw = getpwuid(uid);
        if (pw && pw->pw_name) {
            snprintf(out, size, "%s", pw->pw_name);
            return;
        }
    }
    snprintf(out, size, "%u", (unsigned int)uid);
}

/*
 * Lay chuoi group (ten hoac so GID)
 */
static void get_group_string(gid_t gid, int opt_n, char *out, size_t size) {
    if (!opt_n) {
        struct group *gr = getgrgid(gid);
        if (gr && gr->gr_name) {
            snprintf(out, size, "%s", gr->gr_name);
            return;
        }
    }
    snprintf(out, size, "%u", (unsigned int)gid);
}

/*
 * In danh sach cac file ra man hinh
 */
void display_files(const FileInfo *files, int count, const Options *opts, int is_dir_content) {
    if (!files || count == 0) return;

    int is_long_format = (opts->opt_l || opts->opt_n);

    /*
     * Theo man page:
     * "In addition, for each directory whose contents are displayed, the total number
     * of blocks in units of 512 bytes or BLOCKSIZE used by the files in the directory
     * is displayed on a line by itself immediately before the information for the files."
     * Hoac khi bat co -s va dau ra la terminal:
     * "If the output is to a terminal, a total sum for all the file sizes is output on a line before the listing."
     */
    if (is_dir_content && (is_long_format || (opts->opt_s && isatty(STDOUT_FILENO)))) {
        long long total_blocks = calculate_total_blocks(files, count, opts);
        printf("total %lld\n", total_blocks);
    }

    /*
     * Quet truoc de tinh do rong cot lon nhat cho viec can le dep mat (Padding)
     */
    int max_ino_w = 0, max_blk_w = 0, max_nlink_w = 0;
    int max_owner_w = 0, max_group_w = 0, max_size_w = 0;

    char tmp_buf[256];
    for (int i = 0; i < count; i++) {
        if (!files[i].stat_ok) continue;

        if (opts->opt_i) {
            int w = snprintf(tmp_buf, sizeof(tmp_buf), "%llu", (unsigned long long)files[i].st.st_ino);
            if (w > max_ino_w) max_ino_w = w;
        }
        if (opts->opt_s) {
            long long blk = calculate_display_blocks(&files[i].st, opts->opt_k);
            int w = snprintf(tmp_buf, sizeof(tmp_buf), "%lld", blk);
            if (w > max_blk_w) max_blk_w = w;
        }
        if (is_long_format) {
            int w_link = snprintf(tmp_buf, sizeof(tmp_buf), "%lu", (unsigned long)files[i].st.st_nlink);
            if (w_link > max_nlink_w) max_nlink_w = w_link;

            get_owner_string(files[i].st.st_uid, opts->opt_n, tmp_buf, sizeof(tmp_buf));
            int w_owner = (int)strlen(tmp_buf);
            if (w_owner > max_owner_w) max_owner_w = w_owner;

            get_group_string(files[i].st.st_gid, opts->opt_n, tmp_buf, sizeof(tmp_buf));
            int w_grp = (int)strlen(tmp_buf);
            if (w_grp > max_group_w) max_group_w = w_grp;

            if (S_ISCHR(files[i].st.st_mode) || S_ISBLK(files[i].st.st_mode)) {
                int w_dev = snprintf(tmp_buf, sizeof(tmp_buf), "%d, %d", major(files[i].st.st_rdev), minor(files[i].st.st_rdev));
                if (w_dev > max_size_w) max_size_w = w_dev;
            } else if (opts->opt_h) {
                format_human_size(files[i].st.st_size, tmp_buf, sizeof(tmp_buf));
                int w_sz = (int)strlen(tmp_buf);
                if (w_sz > max_size_w) max_size_w = w_sz;
            } else {
                int w_sz = snprintf(tmp_buf, sizeof(tmp_buf), "%lld", (long long)files[i].st.st_size);
                if (w_sz > max_size_w) max_size_w = w_sz;
            }
        }
    }

    /*
     * Vong lap in tung entry
     */
    char name_clean[1024];
    for (int i = 0; i < count; i++) {
        const FileInfo *fi = &files[i];

        /* Neu lstat that bai, in thong bao loi va bo qua */
        if (!fi->stat_ok) {
            fprintf(stderr, "ls: %s: No such file or directory\n", fi->name ? fi->name : "file");
            continue;
        }

        /* 1. In i-node neu co -i */
        if (opts->opt_i) {
            printf("%*llu ", max_ino_w, (unsigned long long)fi->st.st_ino);
        }

        /* 2. In so block neu co -s */
        if (opts->opt_s) {
            long long blk = calculate_display_blocks(&fi->st, opts->opt_k);
            printf("%*lld ", max_blk_w, blk);
        }

        /* 3. In dinh dang dai (-l hoac -n) */
        if (is_long_format) {
            char mode_str[12];
            format_mode_string(fi->st.st_mode, mode_str);

            char owner_str[128];
            char group_str[128];
            get_owner_string(fi->st.st_uid, opts->opt_n, owner_str, sizeof(owner_str));
            get_group_string(fi->st.st_gid, opts->opt_n, group_str, sizeof(group_str));

            char time_str[64];
            time_t target_time = fi->st.st_mtime;
            if (opts->time_type == TIME_STATUS) {
                target_time = fi->st.st_ctime;
            } else if (opts->time_type == TIME_ACCESS) {
                target_time = fi->st.st_atime;
            }
            format_file_time(target_time, time_str, sizeof(time_str));

            printf("%s %*lu %-*s  %-*s  ",
                   mode_str,
                   max_nlink_w, (unsigned long)fi->st.st_nlink,
                   max_owner_w, owner_str,
                   max_group_w, group_str);

            /* In dung luong hoac major, minor device */
            if (S_ISCHR(fi->st.st_mode) || S_ISBLK(fi->st.st_mode)) {
                char dev_buf[64];
                snprintf(dev_buf, sizeof(dev_buf), "%d, %d", major(fi->st.st_rdev), minor(fi->st.st_rdev));
                printf("%*s ", max_size_w, dev_buf);
            } else if (opts->opt_h) {
                char h_size[32];
                format_human_size(fi->st.st_size, h_size, sizeof(h_size));
                printf("%*s ", max_size_w, h_size);
            } else {
                printf("%*lld ", max_size_w, (long long)fi->st.st_size);
            }

            printf("%s ", time_str);
        }

        /* 4. Xu ly ten file va ky hieu -F */
        sanitize_filename(fi->name, name_clean, sizeof(name_clean), opts->opt_q, opts->opt_w);
        printf("%s", name_clean);

        if (opts->opt_F) {
            char indicator = get_file_indicator(fi->st.st_mode);
            if (indicator != '\0') {
                putchar(indicator);
            }
        }

        /* Neu la symbolic link va o che do long format, in target link */
        if (is_long_format && S_ISLNK(fi->st.st_mode) && fi->link_target) {
            char target_clean[1024];
            sanitize_filename(fi->link_target, target_clean, sizeof(target_clean), opts->opt_q, opts->opt_w);
            printf(" -> %s", target_clean);
        }

        /* Xuong dong theo chuan NetBSD (one entry per line) */
        putchar('\n');
    }
}
