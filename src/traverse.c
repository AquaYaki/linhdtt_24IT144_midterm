#include "ls.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/*
 * Doc thong tin chi tiet cua file/thu muc vao FileInfo
 */
int populate_file_info(FileInfo *fi, const char *path, const char *name) {
    if (!fi || !path || !name) return -1;

    fi->name = strdup(name);
    fi->path = strdup(path);
    fi->link_target = NULL;
    fi->stat_ok = 0;
    fi->err_num = 0;

    /* Bat buoc dung lstat() de lay thong tin symlink thay vi file dich */
    if (lstat(path, &fi->st) == 0) {
        fi->stat_ok = 1;

        if (S_ISLNK(fi->st.st_mode)) {
            char target[1024];
            ssize_t len = readlink(path, target, sizeof(target) - 1);
            if (len != -1) {
                target[len] = '\0';
                fi->link_target = strdup(target);
            }
        }
    } else {
        fi->stat_ok = 0;
        fi->err_num = errno;
    }

    return fi->stat_ok ? 0 : -1;
}

/*
 * Giai phong mang FileInfo
 */
void free_file_info_array(FileInfo *files, int count) {
    if (!files) return;

    for (int i = 0; i < count; i++) {
        if (files[i].name) free(files[i].name);
        if (files[i].path) free(files[i].path);
        if (files[i].link_target) free(files[i].link_target);
    }
    free(files);
}

/*
 * Kiem tra co nen giu entry lai de hien thi hay khong tuy thuoc vao co -a va -A
 */
static int should_include_entry(const char *name, const Options *opts) {
    if (opts->opt_a) {
        return 1; /* Hien thi tat ca */
    }

    if (opts->opt_A) {
        /* Hien thi tat ca ngoai tru '.' va '..' */
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            return 0;
        }
        return 1;
    }

    /* Mac dinh: Bo qua cac file bat dau bang dau '.' */
    if (name[0] == '.') {
        return 0;
    }

    return 1;
}

/*
 * Duyet va hien thi noi dung cua mot thu muc (co ho tro de quy -R)
 */
void traverse_directory(const char *dirpath, const Options *opts, int print_dirname, int is_first_level) {
    (void)is_first_level;

    if (print_dirname) {
        printf("%s:\n", dirpath);
    }

    DIR *dir = opendir(dirpath);
    if (!dir) {
        fprintf(stderr, "ls: %s: %s\n", dirpath, strerror(errno));
        return;
    }

    FileInfo *entries = NULL;
    int count = 0;
    int capacity = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!should_include_entry(entry->d_name, opts)) {
            continue;
        }

        if (count >= capacity) {
            capacity = (capacity == 0) ? 32 : capacity * 2;
            FileInfo *new_entries = realloc(entries, capacity * sizeof(FileInfo));
            if (!new_entries) {
                fprintf(stderr, "ls: out of memory\n");
                closedir(dir);
                free_file_info_array(entries, count);
                return;
            }
            entries = new_entries;
        }

        /* Xay dung duong dan day du toi file */
        char full_path[2048];
        size_t dlen = strlen(dirpath);
        if (dlen > 0 && dirpath[dlen - 1] == '/') {
            snprintf(full_path, sizeof(full_path), "%s%s", dirpath, entry->d_name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", dirpath, entry->d_name);
        }

        populate_file_info(&entries[count], full_path, entry->d_name);
        count++;
    }

    closedir(dir);

    /* Sap xep noi dung thu muc */
    sort_files(entries, count, opts);

    /* Hien thi danh sach file trong thu muc */
    display_files(entries, count, opts, 1);

    /* Xu ly de quy (-R) */
    if (opts->opt_R) {
        for (int i = 0; i < count; i++) {
            if (entries[i].stat_ok && S_ISDIR(entries[i].st.st_mode)) {
                /* Khong de quy vao '.' va '..' de tranh lap vo tan */
                if (strcmp(entries[i].name, ".") != 0 && strcmp(entries[i].name, "..") != 0) {
                    printf("\n");
                    traverse_directory(entries[i].path, opts, 1, 0);
                }
            }
        }
    }

    free_file_info_array(entries, count);
}

/*
 * Xu ly danh sach cac file/thu muc truyen vao tu dong lenh
 */
void process_operands(int count, char *operands[], const Options *opts) {
    /* Truong hop 1: Khong truyen tham so -> hien thi thu muc hien tai "." */
    if (count == 0) {
        traverse_directory(".", opts, 0, 1);
        return;
    }

    /* Truong hop 2: Co co -d -> tat ca operands duoc coi nhu file thuong */
    if (opts->opt_d) {
        FileInfo *files = malloc(count * sizeof(FileInfo));
        if (!files) return;

        for (int i = 0; i < count; i++) {
            populate_file_info(&files[i], operands[i], operands[i]);
        }
        sort_files(files, count, opts);
        display_files(files, count, opts, 0);
        free_file_info_array(files, count);
        return;
    }

    /*
     * Truong hop 3: Phan loai operands thanh File vs Thu muc
     * Theo man page:
     * "non-directory operands are displayed first; directory and non-directory
     * operands are sorted separately and in lexicographical order."
     */
    FileInfo *file_list = malloc(count * sizeof(FileInfo));
    FileInfo *dir_list = malloc(count * sizeof(FileInfo));
    int file_cnt = 0;
    int dir_cnt = 0;

    for (int i = 0; i < count; i++) {
        FileInfo fi;
        populate_file_info(&fi, operands[i], operands[i]);

        /* Kiem tra xem operand co phai thu muc hay khong */
        int is_dir = 0;
        if (fi.stat_ok) {
            if (S_ISDIR(fi.st.st_mode)) {
                is_dir = 1;
            } else if (S_ISLNK(fi.st.st_mode)) {
                /* Neu la symlink tro toi thu muc va khong phai -l, POSIX ls se mo thu muc */
                struct stat target_st;
                if (!opts->opt_l && !opts->opt_n && !opts->opt_F && stat(operands[i], &target_st) == 0) {
                    if (S_ISDIR(target_st.st_mode)) {
                        is_dir = 1;
                    }
                }
            }
        }

        if (is_dir) {
            dir_list[dir_cnt++] = fi;
        } else {
            file_list[file_cnt++] = fi;
        }
    }

    /* 1. Hien thi nhom non-directory truoc */
    if (file_cnt > 0) {
        sort_files(file_list, file_cnt, opts);
        display_files(file_list, file_cnt, opts, 0);
    }

    /* 2. Hien thi nhom directory sau */
    if (dir_cnt > 0) {
        sort_files(dir_list, dir_cnt, opts);

        int print_hdr = (count > 1 || opts->opt_R);
        for (int i = 0; i < dir_cnt; i++) {
            if (file_cnt > 0 || i > 0) {
                printf("\n");
            }
            traverse_directory(dir_list[i].path, opts, print_hdr, 1);
        }
    }

    free_file_info_array(file_list, file_cnt);
    free_file_info_array(dir_list, dir_cnt);
}
