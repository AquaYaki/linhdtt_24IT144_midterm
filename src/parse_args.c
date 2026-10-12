#include "ls.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Khoi tao cac gia tri mac dinh cho Options
 */
void init_options(Options *opts) {
    if (!opts) return;

    opts->opt_A = 0;
    opts->opt_a = 0;
    opts->opt_d = 0;
    opts->opt_F = 0;
    opts->opt_f = 0;
    opts->opt_h = 0;
    opts->opt_i = 0;
    opts->opt_k = 0;
    opts->opt_l = 0;
    opts->opt_n = 0;
    opts->opt_q = 0;
    opts->opt_w = 0;
    opts->opt_R = 0;
    opts->opt_r = 0;
    opts->opt_S = 0;
    opts->opt_s = 0;
    opts->opt_t = 0;
    opts->time_type = TIME_MODIFIED;

    /*
     * Theo man page:
     * - Co -A mac dinh luon bat doi voi super-user (root)
     */
    if (geteuid() == 0) {
        opts->opt_A = 1;
    }

    /*
     * Theo man page:
     * - Co -q la mac dinh khi stdout la mot terminal
     * - Co -w la mac dinh khi stdout khong phai terminal (vi du ghi ra pipe / file)
     */
    if (isatty(STDOUT_FILENO)) {
        opts->opt_q = 1;
    } else {
        opts->opt_w = 1;
    }
}

/*
 * Phan tich cac co dong lenh su dung ham POSIX getopt()
 * Tra ve chi so optind (vi tri bat dau cua cac tham so file/thu muc)
 * Tra ve -1 neu gap co hop le hoac sai cu phap
 */
int parse_arguments(int argc, char *argv[], Options *opts) {
    init_options(opts);

    int ch;
    while ((ch = getopt(argc, argv, "AacdFfhiklnqRrSstuw")) != -1) {
        switch (ch) {
            case 'A':
                opts->opt_A = 1;
                break;
            case 'a':
                opts->opt_a = 1;
                break;
            case 'c':
                /* -c va -u ghi de nhau; co nao xuat hien sau cung se thang */
                opts->time_type = TIME_STATUS;
                break;
            case 'u':
                opts->time_type = TIME_ACCESS;
                break;
            case 'd':
                /* -R va -d ghi de nhau */
                opts->opt_d = 1;
                opts->opt_R = 0;
                break;
            case 'R':
                opts->opt_R = 1;
                opts->opt_d = 0;
                break;
            case 'F':
                opts->opt_F = 1;
                break;
            case 'f':
                /* -f: Dau ra khong duoc sap xep, tu dong bat -a */
                opts->opt_f = 1;
                opts->opt_a = 1;
                break;
            case 'h':
                /* -h va -k ghi de nhau */
                opts->opt_h = 1;
                opts->opt_k = 0;
                break;
            case 'k':
                opts->opt_k = 1;
                opts->opt_h = 0;
                break;
            case 'i':
                opts->opt_i = 1;
                break;
            case 'l':
                /* -l va -n ghi de nhau */
                opts->opt_l = 1;
                opts->opt_n = 0;
                break;
            case 'n':
                opts->opt_n = 1;
                opts->opt_l = 0;
                break;
            case 'q':
                /* -q va -w ghi de nhau */
                opts->opt_q = 1;
                opts->opt_w = 0;
                break;
            case 'w':
                opts->opt_w = 1;
                opts->opt_q = 0;
                break;
            case 'r':
                opts->opt_r = 1;
                break;
            case 'S':
                /* Sap xep theo kich thuoc */
                opts->opt_S = 1;
                opts->opt_t = 0;
                break;
            case 't':
                /* Sap xep theo thoi gian */
                opts->opt_t = 1;
                opts->opt_S = 0;
                break;
            case 's':
                opts->opt_s = 1;
                break;
            default:
                fprintf(stderr, "usage: ls [-AacdFfhiklnqRrSstuw] [file ...]\n");
                return -1;
        }
    }

    return optind;
}
