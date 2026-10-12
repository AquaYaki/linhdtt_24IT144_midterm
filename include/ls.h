#ifndef LS_H
#define LS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdbool.h>

/*
 * Loai moc thoi gian can su dung cho viec sap xep (-t) hoac hien thi (-l):
 * - TIME_MODIFIED: Thoi diem sua doi noi dung (mtime - mac dinh)
 * - TIME_STATUS:   Thoi diem thay doi trang thai i-node (ctime - khi co co -c)
 * - TIME_ACCESS:   Thoi diem truy cap gan nhat (atime - khi co co -u)
 */
typedef enum {
    TIME_MODIFIED = 0,
    TIME_STATUS,
    TIME_ACCESS
} TimeType;

/*
 * Cau truc luu tru trang thai cac co dong lenh (Command-line Options)
 * Ho tro day du 19 co theo dac ta NetBSD 10.1 ls(1)
 */
typedef struct {
    int opt_A;          /* -A: Hien thi tat ca ngoai tru '.' va '..' */
    int opt_a;          /* -a: Hien thi tat ca file ke ca file an bat dau bang '.' */
    int opt_d;          /* -d: Xem thu muc nhu file thong thuong, khong de quy */
    int opt_F;          /* -F: Them ky hieu loai file sau ten (*, /, @, =, |, %) */
    int opt_f;          /* -f: Khong sap xep (tu dong bat -a) */
    int opt_h;          /* -h: Dinh dang dung luong de doc (human-readable, overrides -k) */
    int opt_i;          /* -i: In so i-node */
    int opt_k;          /* -k: Don vi block theo Kilobyte (overrides -h) */
    int opt_l;          /* -l: Dinh dang chi tiet (long format) */
    int opt_n;          /* -n: Dinh dang chi tiet voi UID/GID so thay vi ten */
    int opt_q;          /* -q: Thay ky tu khong in duoc bang '?' */
    int opt_w;          /* -w: In ky tu nguyen ban (raw) */
    int opt_R;          /* -R: Duyet de quy thu muc con */
    int opt_r;          /* -r: Dao nguoc thu tu sap xep */
    int opt_S;          /* -S: Sap xep theo kich thuoc giam dan */
    int opt_s;          /* -s: In so block dung luong */
    int opt_t;          /* -t: Sap xep theo thoi gian */
    TimeType time_type; /* Luu quyet dinh giua mtime, -c va -u */
} Options;

/*
 * Cau truc luu tru thong tin day du ve mot entry (file/thu muc/symlink)
 */
typedef struct {
    char *name;         /* Ten hien thi (vi du: "main.c") */
    char *path;         /* Duong dan day du (vi du: "src/main.c") */
    struct stat st;     /* Thong tin doc tu lstat() */
    char *link_target;  /* Duong dan dich neu la symbolic link */
    int stat_ok;        /* 1 neu lstat thanh cong, 0 neu loi */
    int err_num;        /* Ma loi errno neu lstat that bai */
} FileInfo;

/* Khoi tao gia tri mac dinh cho Options */
void init_options(Options *opts);

/* Ham phan tich tham so dong lenh bang getopt() */
int parse_arguments(int argc, char *argv[], Options *opts);

/* Ham doc thong tin cua mot file vao FileInfo */
int populate_file_info(FileInfo *fi, const char *path, const char *name);

/* Giai phong bo nho cua mang FileInfo */
void free_file_info_array(FileInfo *files, int count);

/* Ham dieu phoi xu ly danh sach file/thu muc truyen vao tu dong lenh */
void process_operands(int count, char *operands[], const Options *opts);

/* Ham duyet va hien thi noi dung cua mot thu muc (co ho tro de quy -R) */
void traverse_directory(const char *dirpath, const Options *opts, int print_dirname, int is_first_level);

/* Ham sap xep danh sach file theo cac tieu chi */
void sort_files(FileInfo *files, int count, const Options *opts);

/* Ham hien thi danh sach file ra man hinh */
void display_files(const FileInfo *files, int count, const Options *opts, int is_dir_content);

#endif /* LS_H */
