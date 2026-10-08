# Báo cáo chung Tuần 1 — Chuẩn bị và thiết kế

**Đã có dữ liệu phần Tuần 1 độc lập của Huy.** Chưa có ngày bắt đầu/hạn nộp. Nội dung mục tiêu dưới đây là kế hoạch. Phần Châu trên nhánh nền được giữ nguyên; [PR #1 của Châu](https://github.com/ndhuy1127/os-producer-consumer/pull/1) còn mở lúc kiểm tra và chưa tích hợp vào báo cáo này.

## 1. Mục tiêu tuần

- Châu (dự kiến): Lý thuyết luồng/semaphore, giả mã đồng bộ, thiết kế kết thúc, ví dụ tạo và chờ luồng.
- Huy (dự kiến): Môi trường C, Makefile, bộ đệm vòng thử tuần tự, tham số và kế hoạch kiểm thử.
- Mốc dự kiến: Có thiết kế, bộ đệm thử tuần tự và môi trường biên dịch hoạt động.

## 2. Công việc Châu đã hoàn thành

Chưa có dữ liệu tiến độ.

| Nhiệm vụ | Việc thực tế đã làm | Trạng thái | Minh chứng |
| --- | --- | --- | --- |
| Chưa ghi nhận | Chưa có dữ liệu tiến độ | Chưa bắt đầu | Chưa có |

## 3. Công việc Huy đã hoàn thành

Nguyễn Đức Huy — 20233448; thực hiện trên `dev/huy`, Codex hỗ trợ theo yêu cầu. Phần độc lập W1-H đã hoàn thành; chưa có xác nhận đọc chéo của Châu.

| Nhiệm vụ | Việc thực tế đã làm | Trạng thái | Minh chứng |
| --- | --- | --- | --- |
| Môi trường C trên máy Huy | Kiểm tra Ubuntu 24.04.5/WSL2, GCC 13.3.0, Make 4.3, Git 2.43.0, Python 3.12.3; build/chạy ví dụ Châu từ đúng SHA ngoài repo, kết quả 55/210/465 và đủ join | Đã hoàn thành | [Log môi trường](../../results/week1/huy-environment.log) |
| API bộ đệm tuần tự | DATA/STOP; mảng, head/tail/count/capacity; bốn hàm tương thích chữ ký Châu; lỗi và quyền sở hữu rõ ràng | Đã hoàn thành | [Hợp đồng](../buffer-week1-huy.md), [nguồn](../../src/buffer.c) |
| Makefile và T01 | Build riêng, test tuần tự, không tạo main giả; 12/12 ca qua hai build sạch; sanitizer 11/11 ca thường | Đã hoàn thành | [Log test](../../results/week1/huy-buffer-test.log), [đánh giá](../../results/week1/huy-validation.md) |
| Đề xuất tham số/log/test Tuần 2 | K DATA mỗi producer, mặc định/giới hạn/lỗi CLI, schema log trong guard, quan sát chờ bằng semaphore; T02–T09 có cấu hình nhưng chưa chạy | Đã hoàn thành | [Cấu hình/log](../config-log-week1-huy.md), [test-plan](../test-plan.md) |

## 4. Kết quả tích hợp và kiểm thử của nhóm

Chưa tích hợp PR #1 và PR Huy, chưa nghiệm thu chung. Kết quả do Huy kiểm tra trên máy Huy ngày 08/10/2026 theo Asia/Saigon: `make clean`, `make buffer-test`, `make test-buffer` hai chu kỳ đều exit 0, 12/12 PASS tuần tự; `make test` exit 0 với cùng phạm vi. Bản sanitizer 11/11 ca thường exit 0. `make` mặc định exit 2 đúng dự kiến vì thiếu src/main.c.

Ví dụ Châu từ `2c8d7f61415c1dc730c8e4dbde4c991a37bfdfa6` được gcc build với -pthread rồi `timeout 5s ./thread-demo` trong /tmp, exit 0; đủ 3 worker và 3 join đúng. Kiểm tra này chỉ chứng minh vòng đời luồng và môi trường, chưa chứng minh semaphore/Producer–Consumer. Nguồn bộ đệm được kiểm tra là commit nền `a004c2d` cộng các file W1-H trước commit, SHA256 trong [log](../../results/week1/huy-buffer-test.log). Kiểm tra khung/liên kết/Git ghi tại [minh chứng](../../results/week1/huy-validation.md); không tính là PASS thuật toán. T02–T09 chưa chạy.

## 5. Việc chưa hoàn thành hoặc đang vướng

Chờ Châu xác nhận hợp đồng API chi tiết, K mỗi producer, giới hạn CLI và schema log; chờ đọc chéo kết quả và tích hợp hai PR. Mốc chung Tuần 1 chưa nghiệm thu. Chưa viết producer/consumer, semaphore, CLI hoặc runtime log; không dùng PASS T01 để suy ra các phần đó hoạt động.

## 6. Công việc tiếp theo

Đọc chéo và thống nhất đề xuất, reviewer xử lý hai PR vào main; khi tích hợp giữ cả target thread-demo và test-buffer, giữ nội dung của hai người trong tài liệu chung. Sau merge cập nhật từng nhánh từ origin/main trước việc mới. Tuần 2 triển khai đồng bộ ngoài API, CLI/log rồi chạy T02–T05/T09 theo [kế hoạch](../plan.md) và [test-plan](../test-plan.md). Quy tắc cập nhật ở [README báo cáo](README.md).
