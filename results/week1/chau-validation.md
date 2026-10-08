# Đánh giá kiểm tra W1-C — Lê Hải Châu

Ngày thực chạy: **08/10/2026**, thời điểm trong log theo Asia/Saigon (UTC+07:00). Môi trường: Ubuntu 24.04.5 LTS trên WSL2, GCC 13.3.0, GNU Make 4.3, Git 2.43.0, Python 3.12.3. Chi tiết ở [chau-environment.log](chau-environment.log); lệnh/đầu ra/mã thoát ở [chau-thread-demo.log](chau-thread-demo.log).

Nhánh kiểm tra: `dev/chau`, đã fetch origin và fast-forward từ `2255719` đến `a004c2ddf90999486f87126260c70087026b8972` của origin/main trước khi viết W1-C; giữ chỉnh sửa README trên main. Working tree ban đầu sạch, không có thay đổi người dùng bị gộp. Kiểm tra trên commit nền đó cộng các file W1-C trước commit; log ghi SHA256 của `examples/thread_lifecycle.c` và `Makefile`. SHA commit bàn giao được xác nhận sau khi commit thực sự tồn tại, không dùng SHA nền để tuyên bố đã kiểm tra nội dung mới tại một commit cũ.

## Kết quả theo phạm vi

| Kiểm tra thực chạy | Kết quả | Mã thoát |
| --- | --- | --- |
| `make thread-demo` | Đạt: -Wall -Wextra -Wpedantic; -pthread ở compile/link; không có cảnh báo compiler; build lại sau chỉnh timestamp không cảnh báo make | 0 |
| `timeout 5s bin/thread-demo` — 5 lần | Đạt: mỗi worker 1/2/3 có một dòng kết quả và một dòng join đúng; tổng 55/210/465; completed chỉ ở cuối; không yêu cầu thứ tự chạy cố định | 0 mỗi lần |
| `make help` | Đạt: có hướng dẫn thread-demo | 0 |
| `make clean` | Đạt: build/ và bin/ được dọn | 0 |
| `make thread-demo` rồi `timeout 5s bin/thread-demo` sau clean | Đạt: biên dịch/link lại và đủ 3 worker đúng | 0 cả hai lệnh |
| Tiêm EAGAIN ở create lần 1 | Đạt trong kiểm tra lỗi: không join ID chưa tạo, không báo completed; created/verified = 0/3 | 1 đúng dự kiến |
| Tiêm EAGAIN ở create lần 3 | Đạt trong kiểm tra lỗi: join/kiểm tra worker 1 và 2, không báo completed; created/verified = 2/3 | 1 đúng dự kiến |
| `make`, `make test` | Hành vi khung đúng: báo chưa triển khai chương trình/kiểm thử chính; không tính PASS thuật toán | 2 mỗi lệnh, đúng dự kiến |
| `python3 scripts/check_scaffold.py` | Đạt: 23 file bắt buộc, 92 liên kết tương đối, quy tắc ignore đúng; không có AGENTS.md tracked/staged | 0 |
| `git diff --check` | Đạt: không có lỗi whitespace | 0 |
| Rà soát diff, liên kết, tracked và ignore | Đạt: giữ 6 phần báo cáo, phần Huy và các file phạm vi Huy không đổi; giữ chỉnh sửa README main; các file W1-C không bị ignore; AGENTS.md mọi cấp và file build được ignore | 0 |
| `git diff --cached --check`, danh sách tracked/staged | Đạt: đúng 11 file W1-C đã stage, không AGENTS.md/binary/object/cache/PDF; SHA256 ví dụ và Makefile vẫn khớp bản thực chạy | 0 |

Timeout 5 giây dùng để phát hiện treo ở ví dụ chỉ có ba phép tính nhỏ; không phải thời hạn kiểm thử chương trình Producer–Consumer. Các điều kiện “đạt” của vòng đời luồng đã được kiểm tra tự động trên đầu ra thực tế: mỗi ID xuất hiện đúng một lần ở kết quả và join, giá trị khớp, exit 0 và completed cuối cùng.

Kiểm tra lỗi dùng wrapper linker `--wrap=pthread_create` với mã C/command được lưu trong log. Wrapper và binary ở thư mục tạm `/tmp`, không thay đổi code ví dụ hoặc đưa framework kiểm thử vào repo. Hai lần chạy dùng `FAIL_CREATE_AT=1` và `FAIL_CREATE_AT=3`. Đây là lỗi được tiêm có chủ đích, không tuyên bố hệ điều hành tự hết tài nguyên. Lỗi join, sai kết quả và lỗi semaphore chưa được tiêm để kiểm thử; nhánh xử lý join đã được đọc rà soát.

Lần build đầu gặp cảnh báo make về clock skew vì mtime trên `/mnt/d` nằm trong tương lai so với đồng hồ WSL. Đã chỉnh mtime của các file vừa sửa và file build phát sinh theo đồng hồ WSL, không đổi nội dung; clean rồi build lại. Log giữ đầu ra cảnh báo ban đầu, thao tác chỉnh timestamp và kết quả kiểm tra lại. Đồng hồ WSL có dịch chuyển trong phiên; dùng thứ tự các lệnh trong log để đọc diễn tiến, không suy ra thứ tự worker/FIFO từ timestamp.

## Nội dung chưa kiểm chứng

- Chưa triển khai/kiểm thử bộ đệm vòng, FIFO, empty/full/guard, STOP hoặc CLI của chương trình chính. Chưa chạy T01–T09 trong [test-plan.md](../../docs/test-plan.md).
- Chưa kiểm tra môi trường máy Huy, bộ đệm tuần tự của Huy hay việc ghép code. W1-H giữ nguyên; mốc chung Tuần 1 chưa xác nhận đầy đủ.
- [Thiết kế](../../docs/design.md) là bản Tuần 1 của Châu, chưa được Huy xác nhận; API/cấu trúc/log cần phối hợp. Hướng xử lý lỗi semaphore mới là thiết kế.
- Số worker hoàn thành chỉ chứng minh ví dụ tạo/join hữu hạn, không chứng minh thuật toán Producer–Consumer hoạt động.

## Chạy lại

Từ Git root trong Ubuntu/WSL:

```sh
make thread-demo
timeout 5s ./bin/thread-demo
python3 scripts/check_scaffold.py
make help
make clean
make thread-demo
timeout 5s ./bin/thread-demo
git diff --check
```

Ví dụ không nhận tham số. Kỳ vọng ba kết quả 55, 210, 465 và dòng `completed: all 3 workers joined and verified`, exit 0; thứ tự worker có thể thay đổi. `make` mặc định và `make test` vẫn báo chưa triển khai, exit 2.
