#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <time.h>

/* Chuyen doi st_mode thanh chuoi 10 ky tu quyen han (vd: "drwxr-xr-x") */
void format_mode_string(mode_t mode, char *out_str);

/* Dinh dang kich thuoc theo don vi de doc (human-readable B, K, M, G, T) */
void format_human_size(off_t size, char *out_buf, size_t buf_size);

/* Dinh dang thoi gian theo chuan man page: Thang, Ngay, Gio:Phut */
void format_file_time(time_t ftime, char *out_buf, size_t buf_size);

/* Lay ky tu dac ta loai file theo co -F ('/', '*', '@', '=', '|', hoac '\0') */
char get_file_indicator(mode_t mode);

/* Xu ly ten file dua vao co -q va -w (thay the ky tu khong in duoc bang '?') */
void sanitize_filename(const char *src, char *dst, size_t dst_size, int opt_q, int opt_w);

/* Lay kich thuoc block he thong tu bien moi truong BLOCKSIZE (mac dinh 512 bytes) */
long get_env_blocksize(void);

/* Tinh so luong block thuc te theo don vi quy dinh boi cờ -k hoac BLOCKSIZE */
long long calculate_display_blocks(const struct stat *st, int opt_k);

#endif /* UTILS_H */
