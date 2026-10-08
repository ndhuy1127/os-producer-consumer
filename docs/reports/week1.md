# Báo cáo chung Tuần 1 — Chuẩn bị và thiết kế

**Đã có kết quả phần Tuần 1 độc lập của Châu; mốc chung chưa được xác nhận đầy đủ.** Chưa có ngày bắt đầu/hạn nộp. Kết quả thực chạy ngày 08/10/2026 (Asia/Saigon); nội dung mục tiêu vẫn là kế hoạch của nhóm.

## 1. Mục tiêu tuần

- Châu (dự kiến): Lý thuyết luồng/semaphore, giả mã đồng bộ, thiết kế kết thúc, ví dụ tạo và chờ luồng.
- Huy (dự kiến): Môi trường C, Makefile, bộ đệm vòng thử tuần tự, tham số và kế hoạch kiểm thử.
- Mốc dự kiến: Có thiết kế, bộ đệm thử tuần tự và môi trường biên dịch hoạt động.

## 2. Công việc Châu đã hoàn thành

Hoàn thành W1-C theo phạm vi tài liệu, thiết kế và ví dụ vòng đời luồng. Thiết kế chưa được Huy xác nhận và chưa phải chương trình Producer–Consumer đã triển khai.

| Nhiệm vụ | Việc thực tế đã làm | Trạng thái | Minh chứng |
| --- | --- | --- | --- |
| Lý thuyết luồng/semaphore | Viết tiếng Việt về tiến trình/luồng, hàng đợi N chỗ và FIFO, vùng găng/data race, deadlock, empty/full/guard, sleep và create/join; đối chiếu Linux man-pages, ghi rõ chưa đọc giáo trình | Đã hoàn thành | [Lý thuyết](../theory-week1-chau.md) |
| Thiết kế và giả mã | Chọn ba semaphore pshared = 0; viết main/producer/consumer/enqueue STOP; nêu bất biến, FIFO, quyền sở hữu và lỗi; đề xuất API bộ đệm tuần tự để ghép với Huy | Đã hoàn thành; chưa được Huy xác nhận | [Thiết kế](../design.md) |
| Ví dụ C và build | Tạo 3 worker có struct đối số và ô kết quả riêng, tính tổng hữu hạn, join rồi kiểm tra; xử lý mã lỗi pthread và create lỗi giữa chừng; thêm target thread-demo, giữ target mặc định/test chưa triển khai | Đã hoàn thành | [Mã C](../../examples/thread_lifecycle.c), [Makefile](../../Makefile) |
| Chạy kiểm tra và lưu minh chứng | Biên dịch với cảnh báo và -pthread; chạy 5 lần, clean/build/chạy lại, kiểm tra kết quả; thử create lỗi ở lần 1/3 bằng wrapper tạm ngoài repo; cập nhật tài liệu và log thực tế | Đã hoàn thành | [Môi trường](../../results/week1/chau-environment.log), [log](../../results/week1/chau-thread-demo.log), [đánh giá](../../results/week1/chau-validation.md) |

## 3. Công việc Huy đã hoàn thành

Chưa có dữ liệu tiến độ.

| Nhiệm vụ | Việc thực tế đã làm | Trạng thái | Minh chứng |
| --- | --- | --- | --- |
| Chưa ghi nhận | Chưa có dữ liệu tiến độ | Chưa bắt đầu | Chưa có |

## 4. Kết quả tích hợp và kiểm thử của nhóm

Các kiểm tra dưới đây chạy trên Ubuntu 24.04.5 LTS/WSL2 của phiên làm phần Châu, GCC 13.3.0. Revision được kiểm tra là commit nền `a004c2ddf90999486f87126260c70087026b8972` cộng các file W1-C chưa commit; log ghi SHA256 mã ví dụ/Makefile. Không gán các kết quả này cho máy Huy hoặc bộ đệm/semaphore.

| Lệnh/cấu hình | Kết quả và mã thoát | Phạm vi/minh chứng |
| --- | --- | --- |
| `make thread-demo` | Đạt; bật -Wall -Wextra -Wpedantic, -pthread ở compile/link; build lại sau xử lý timestamp WSL không cảnh báo; exit 0 | Ví dụ Châu; [log](../../results/week1/chau-thread-demo.log) |
| `timeout 5s bin/thread-demo`, 5 lần và 1 lần sau clean/build | 3 worker xuất hiện, join và kết quả 55/210/465 đúng; báo hoàn tất sau join; cả 6 lần exit 0 | Chỉ vòng đời luồng; không yêu cầu thứ tự worker cố định |
| Wrapper `--wrap=pthread_create`, lỗi ở lần 1/3 | Thoát 1 đúng dự kiến; lần 3 vẫn join/kiểm tra 2 worker đã tạo; không in completed | Đường lỗi create của ví dụ; wrapper và binary tạm ngoài repo |
| `make help`, `make clean`, build/chạy lại | Help có thread-demo; clean xóa build/bin; ví dụ build/chạy lại được; exit 0 | Build/tooling cho ví dụ |
| `make`, `make test` | Exit 2 và thông báo chưa triển khai đúng dự kiến | Kiểm tra hành vi khung, không phải PASS thuật toán |
| `python3 scripts/check_scaffold.py`, `git diff --check` | Đạt: 23 file bắt buộc, 92 liên kết tương đối, ignore đúng và không có AGENTS.md tracked/staged; diff không lỗi whitespace; exit 0 | Chỉ khung/liên kết/ignore và định dạng diff; [đánh giá](../../results/week1/chau-validation.md) |

**Chưa tích hợp code; chưa kiểm thử bộ đệm tuần tự, FIFO, empty/full/guard hay chương trình Producer–Consumer.** Không có ca T01–T09 đạt PASS; mốc chung Tuần 1 chưa được xác nhận đầy đủ. Minh chứng ở [results/week1](../../results/week1/README.md).

## 5. Việc chưa hoàn thành hoặc đang vướng

- W1-H chưa có bằng chứng mới: bộ đệm tuần tự, tham số và môi trường máy Huy chưa kiểm chứng; giữ nguyên phần Huy ở mục 3.
- CLI, cấu trúc/API bộ đệm và schema log cần phối hợp với Huy. Thiết kế Tuần 1 là đề xuất của Châu, chưa được nhóm xác nhận.
- Chưa có chương trình Producer–Consumer hoặc kiểm thử thuật toán; các hướng xử lý lỗi semaphore chưa viết/chạy. Lỗi join chưa được tiêm để kiểm thử.
- Đã xử lý cảnh báo clock skew khi build bằng chỉnh timestamp file vừa tạo theo đồng hồ WSL, không đổi nội dung; log giữ cả cảnh báo ban đầu và lần kiểm tra lại.

## 6. Công việc tiếp theo

Huy hoàn thiện/xác nhận phần W1-H và API bộ đệm tuần tự; hai thành viên thống nhất cấu hình và log trước tích hợp. Tuần 2, Châu triển khai producer/consumer, lớp đồng bộ ba semaphore và STOP theo thiết kế đã thống nhất; ghép với bộ đệm của Huy, kiểm tra 1 producer/1 consumer, đầy/trống, N = 1 và kết thúc hữu hạn. Cập nhật [progress](../progress.md) và minh chứng theo kết quả thực chạy; quy tắc báo cáo ở [README báo cáo](README.md).
