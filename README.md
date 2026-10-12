# BÁO CÁO BÀI TẬP GIỮA KỲ: HIỆN THỰC LỆNH `ls(1)` TỪ ĐẦU (NETBSD 10.1)

**Môn học:** Lập trình hệ thống (Systems Programming)  
**Sinh viên:** linhdtt - 24IT144  
**Môi trường thử nghiệm:** NetBSD 10.1 (x86_64)

---

## 1. Giới thiệu tổng quan
Dự án nhằm mục đích tự tay xây dựng lại công cụ dòng lệnh `ls(1)` của hệ điều hành UNIX/NetBSD từ đầu bằng **ngôn ngữ C**. Chương trình tương tác trực tiếp với các System Call của nhân hệ điều hành (`lstat`, `opendir`, `readdir`, `readlink`) để duyệt cây thư mục, trích xuất siêu dữ liệu tập tin (`struct stat`) và định dạng kết quả hiển thị chuẩn xác theo đặc tả của man page NetBSD 10.1.

---

## 2. Cấu trúc thư mục dự án
Dự án được tổ chức theo cấu trúc module hóa (Modular Architecture):

```text
linhdtt_24IT144_midterm/
├── include/
│   ├── ls.h         # Định nghĩa các struct Options, FileInfo và nguyên mẫu hàm cốt lõi
│   └── utils.h      # Khai báo các tiện ích định dạng quyền file, dung lượng, thời gian
├── src/
│   ├── main.c       # Điểm khởi đầu chương trình, phân luồng tham số
│   ├── parse_args.c # Xử lý 19 cờ dòng lệnh bằng hàm POSIX getopt() và luật đè cờ
│   ├── traverse.c   # Quét thư mục, phân loại tập tin và duyệt đệ quy (-R)
│   ├── sort.c       # Sắp xếp danh sách tập tin (-t, -S, -r, -f, thứ tự từ vựng)
│   ├── display.c    # Căn lề và hiển thị danh sách (-l, -n, -i, -s, -F, -q, -w)
│   └── utils.c      # Hiện thực các hàm xử lý chuỗi quyền hạn, human-readable (-h)
├── Makefile         # Tự động hóa biên dịch với gcc/clang và cờ cảnh báo nghiêm ngặt
├── .gitignore       # Loại bỏ file nhị phân và file đối tượng (.o) khi commit Git
└── README.md        # Báo cáo chi tiết đồ án
```

---

## 3. Danh sách 19 cờ đã hiện thực
Chương trình hiện thực đầy đủ **19 tùy chọn (options)** được quy định trong tài liệu `ls.pdf`:

| Cờ | Ý nghĩa & Hành vi |
| :---: | :--- |
| `-A` | Hiển thị tất cả tập tin, ngoại trừ `.` và `..`. Mặc định luôn bật với Super-user (`root`). |
| `-a` | Hiển thị tất cả tập tin, bao gồm cả các tập tin ẩn bắt đầu bằng dấu chấm (`.`). |
| `-c` | Dùng mốc thời gian thay đổi trạng thái file (`ctime`) thay vì `mtime` để sắp xếp hoặc in (`-l`). |
| `-d` | Xem thư mục như tập tin bình thường (không mở vào trong) và không giải tham chiếu symlink. |
| `-F` | Thêm ký hiệu nhận diện sau tên file: `/` (thư mục), `*` (thực thi), `@` (symlink), `=` (socket), `\|` (FIFO). |
| `-f` | Tắt hoàn toàn việc sắp xếp file (giữ nguyên thứ tự đọc từ ổ đĩa) và tự động bật cờ `-a`. |
| `-h` | Hiển thị dung lượng file dạng con người dễ đọc (`B`, `K`, `M`, `G`, `T`). Ghi đè cờ `-k`. |
| `-i` | In số định danh i-node của mỗi file. |
| `-k` | Báo cáo dung lượng block theo đơn vị Kilobyte. Ghi đè cờ `-h`. |
| `-l` | Định dạng hiển thị chi tiết (Long format): quyền, số link, user, group, kích thước, ngày giờ, tên. |
| `-n` | Định dạng chi tiết giống `-l`, nhưng hiển thị UID và GID dạng số thay vì tra cứu tên. |
| `-q` | Thay thế các ký tự không in được trong tên file bằng dấu `?` (mặc định khi xuất ra terminal). |
| `-w` | In nguyên bản các ký tự trong tên file (mặc định khi xuất ra pipe/file). |
| `-R` | Duyệt đệ quy tất cả các thư mục con cấp dưới. |
| `-r` | Đảo ngược thứ tự sắp xếp kết quả. |
| `-S` | Sắp xếp danh sách theo kích thước giảm dần (file lớn nhất đứng trước). |
| `-s` | In số block hệ thống tập tin thực tế được sử dụng (theo 512 bytes hoặc biến `BLOCKSIZE`). |
| `-t` | Sắp xếp theo mốc thời gian mới nhất trước. |
| `-u` | Dùng mốc thời gian truy cập gần nhất (`atime`) thay vì `mtime` để sắp xếp hoặc in (`-l`). |

---

## 4. Quy tắc xử lý đè cờ & Edge Cases (Flags Precedence)
Tuân thủ nghiêm ngặt các quy tắc trong man page NetBSD:
1. **`-w` và `-q`:** Cờ nào xuất hiện sau cùng trên dòng lệnh sẽ quyết định cách in ký tự.
2. **`-l` và `-n`:** Cờ nào xuất hiện sau cùng sẽ quyết định hiển thị tên hay ID số.
3. **`-c` và `-u`:** Cờ nào xuất hiện sau cùng sẽ xác định mốc thời gian sử dụng (`ctime` hay `atime`).
4. **`-R` và `-d`:** Cờ nào xuất hiện sau cùng sẽ quyết định có duyệt đệ quy hay xem thư mục như file.
5. **`-h` và `-k`:** Cờ xuất hiện ngoài cùng bên phải sẽ đè cờ trước đó.
6. **Thứ tự ưu tiên Operands:** Khi người dùng truyền nhiều đường dẫn đầu vào, các tập tin không phải thư mục (files) luôn được hiển thị trước; các thư mục được hiển thị sau. Cả hai nhóm đều được sắp xếp riêng biệt.
7. **Dòng tổng block (`total <blocks>`):** Được in trước nội dung của thư mục khi dùng cờ `-l` hoặc khi có `-s` trên terminal.

---

## 5. Hướng dẫn biên dịch và sử dụng

### Biên dịch:
Chạy lệnh `make` tại thư mục gốc:
```bash
make
```
Hệ thống sẽ biên dịch ra file thực thi `./ls` và tạo liên kết `./myls`.

### Chạy chương trình:
```bash
# Xem danh sách cơ bản
./ls

# Xem dạng chi tiết kèm dung lượng dễ đọc và ký hiệu file
./ls -lhF

# Xem đầy đủ file ẩn và số inode
./ls -ai

# Sắp xếp theo dung lượng giảm dần
./ls -lS

# Sắp xếp theo thời gian truy cập gần nhất đảo ngược
./ls -lutr

# Duyệt đệ quy toàn bộ thư mục
./ls -R /tmp
```

### Xóa các file build tạm:
```bash
make clean
```
