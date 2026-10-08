# Minh chứng Tuần 1

Đã có minh chứng thực chạy cho **ví dụ tạo/join luồng của Châu** trên Ubuntu 24.04/WSL ngày 08/10/2026. Chưa có kiểm thử bộ đệm hoặc chương trình Producer–Consumer; không coi log ví dụ/khung là nghiệm thu thuật toán hay phần Huy.

| File | Nội dung |
| --- | --- |
| [chau-environment.log](chau-environment.log) | Git root/nhánh sau fetch và đồng bộ, working tree ban đầu, Ubuntu/WSL và phiên bản công cụ; thời điểm theo Asia/Saigon |
| [chau-thread-demo.log](chau-thread-demo.log) | Lệnh, đầu ra, mã thoát build/run lặp/clean/build lại/help; make/test chưa triển khai; thử create lỗi; kiểm tra cuối và xử lý timestamp WSL |
| [chau-validation.md](chau-validation.md) | Đánh giá phạm vi, kết quả, giới hạn và cách chạy lại ví dụ |
| [scaffold-check.log](scaffold-check.log) | Minh chứng khởi tạo có sẵn, giữ nguyên; tách biệt với phần Châu |

Log Châu ghi commit nền và SHA256 của ví dụ/Makefile tại thời điểm kiểm tra trước commit. Commit bàn giao là commit chứa các file này, có thể xác nhận bằng `git log -1` trên dev/chau sau khi nhận bàn giao. Không lưu binary/object/cache hoặc wrapper tạm vào Git. Xem [báo cáo Tuần 1](../../docs/reports/week1.md).

`scaffold-check.log` ghi kiểm tra khung ban đầu. Mẫu C kiểm tra công cụ ở lần khởi tạo nằm ngoài repo; khác với ví dụ W1-C hiện lưu trong [examples/thread_lifecycle.c](../../examples/thread_lifecycle.c).
