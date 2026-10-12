#include "ls.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Chuong trinh chinh (Entry point)
 * Tuong thich chuan NetBSD 10.1 ls(1)
 */
int main(int argc, char *argv[]) {
    Options opts;

    /* Phan tich cac co dong lenh */
    int opt_index = parse_arguments(argc, argv, &opts);
    if (opt_index < 0) {
        return 1;
    }

    /* Tinh so luong tham so file/thu muc truyen vao */
    int operand_count = argc - opt_index;
    char **operands = &argv[opt_index];

    /* Tien hanh xu ly va hien thi ket qua */
    process_operands(operand_count, operands, &opts);

    return 0;
}
