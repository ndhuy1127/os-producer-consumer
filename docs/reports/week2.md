# Báo cáo chung Tuần 2 — Bản cơ bản

**W2-C của Lê Hải Châu đạt 100% phạm vi 1 producer/1 consumer.** Kiểm chứng thực ngày 12/10/2026 (Asia/Saigon), Codex thực hiện theo yêu cầu Châu trên máy Châu. W2-H và mốc chung Tuần 2 chưa xác nhận hoàn thành; không ghi Huy đã phê duyệt. Chưa có ngày bắt đầu/hạn nộp.

## 1. Mục tiêu tuần

- Châu: Producer/consumer, semaphore, tích hợp và kết thúc hữu hạn.
- Huy theo kế hoạch: Bộ đệm, tham số, log, kiểm thử đầy/trống và sức chứa 1.
- Mốc dự kiến: 1 producer/1 consumer đúng, không mất/lặp dữ liệu, tự kết thúc.

## 2. Công việc Châu đã hoàn thành

Đã đọc lại Tuần 1 trước khi sửa, tái sử dụng nguyên API buffer và DATA/STOP của Huy; không thêm semaphore trong buffer, không sửa log lịch sử.

| Nhiệm vụ | Việc thực tế đã làm | Trạng thái | Minh chứng |
| --- | --- | --- | --- |
| Producer/consumer và ba semaphore | empty=N/full=0/guard=1, pshared=0; bảo vệ toàn bộ buffer/metadata/oracle/record; item consumer riêng; delay/xử lý ngoài guard; trywait và sem_wait xử lý EINTR | Đã hoàn thành | [Lõi](../../src/pc.c), [API](../../include/pc.h), [thiết kế](../design.md) |
| Main và kết thúc hữu hạn | Entry point thật chạy N=8/1P/1C/K=20; create consumer trước producer, join producer, STOP qua enqueue thường, join consumer, xác minh rồi cleanup | Đã hoàn thành | [main](../../src/main.c), [runtime](../../results/week2/chau-runtime-tests.log) |
| Xử lý lỗi và vòng đời | Kiểm tra cấp phát/buffer/semaphore/pthread; init flags; producer create lỗi vẫn STOP/join consumer; lỗi sync/join nghiêm trọng kết thúc toàn tiến trình | Đã hoàn thành | [26 ca lỗi](../../results/week2/chau-error-tests.log) |
| Kiểm chứng và bàn giao | Tám cấu hình, oracle FIFO độc lập, đầy/trống/STOP, Tuần 1 chạy lại, sanitizer và hash nguồn; API/record để Huy ghép CLI/log | Đã hoàn thành | [Nghiệm thu](../../results/week2/chau-validation.md), [bàn giao](../handoff-week2-chau.md) |

## 3. Công việc Huy đã hoàn thành

Chưa có dữ liệu tiến độ.

| Nhiệm vụ | Việc thực tế đã làm | Trạng thái | Minh chứng |
| --- | --- | --- | --- |
| Chưa ghi nhận | Chưa có dữ liệu tiến độ | Chưa bắt đầu | Chưa có |

## 4. Kết quả tích hợp và kiểm thử của nhóm

Kiểm tra tích hợp **lõi W2-C với buffer Tuần 1**, trên máy Châu; chưa ghép CLI/log W2-H. Ubuntu 24.04.5/WSL2, GCC 13.3.0, C11/cảnh báo/-pthread. Branch dev/chau, HEAD nền `9cf345c12ca59fb1a6bf476147551cb89f7e043b` cộng nguồn working tree mới; [manifest](../../results/week2/chau-source.sha256) và header log ghi hash thực chạy, không gán code mới cho SHA nền. Commit/push xác nhận từ Git sau thao tác trong phần trả kết quả.

| Lệnh/preset thực chạy | Cấu hình N/P/C/K/Dp_ms/Dc_ms | Kết quả | Exit |
| --- | --- | --- | --- |
| make clean; make all thread-demo buffer-test pc-test pc-error-test | C11/cảnh báo/-pthread | Build sạch không cảnh báo | 0 mỗi lệnh |
| timeout 5s bin/thread-demo; make test-buffer | Ví dụ/12 ca buffer tuần tự | 55/210/465, đủ join; 12/12 PASS | 0 mỗi lệnh |
| bin/producer-consumer; bin/pc-test default | 8/1/1/20/0/0 | PASS 20 DATA, 1 STOP, create/join 2 | 0 |
| bin/pc-test source-N5 --records | 5/1/1/20/0/0 | PASS ca nguồn | 0 |
| bin/pc-test T02 | 8/1/1/100/0/0 | PASS 100 ID đúng một lần | 0 |
| bin/pc-test T03 | 1/1/1/100/0/0 | PASS count chỉ 0/1 | 0 |
| bin/pc-test T04 --records | 2/1/1/40/0/20 | PASS: 40 snapshot đầy, 39 WAIT_SLOT | 0 |
| bin/pc-test T05 --records | 2/1/1/40/20/0 | PASS: 41 snapshot rỗng, 41 WAIT_ITEM | 0 |
| bin/pc-test K0 --records | 1/1/1/0/0/0 | PASS: không DATA, 1 STOP, consumer được join | 0 |
| bin/pc-test queued-stop --records | 8/1/1/20/0/20 | PASS: còn 8 DATA sau producer join, tiêu thụ đủ trước STOP | 0 |
| python3 scripts/check_pc_errors.py | 26 ca wrapper/API, timeout 5s mỗi ca | 26/26 PASS; lỗi fatal child exit 1 đúng dự kiến | 0 tổng |
| ASan/UBSan/leak detection ngoài sandbox | 8 ca lõi, main mặc định, 11 ca buffer thường | PASS, không diagnostics | 0 mỗi build/run |
| scaffold; diff/check | Cấu trúc/liên kết/ignore/whitespace | Đạt | 0 |

Mỗi ca runtime có timeout, đối chiếu từng ID/value/kind, FIFO theo thao tác trong guard, count trong [0,N]/cuối 0, STOP riêng và create/join đủ. Record EAGAIN chứng minh thiếu quyền; snapshot dưới guard chứng minh đầy/trống riêng, không suy ra sem_wait đã block. [Log runtime](../../results/week2/chau-runtime-tests.log), [build](../../results/week2/chau-build.log), [sanitizer](../../results/week2/chau-sanitizers.log), [đánh giá chi tiết](../../results/week2/chau-validation.md). Log LSan bị ptrace trong sandbox được giữ riêng; lần ngoài sandbox đạt với detect_leaks=1.

## 5. Việc chưa hoàn thành hoặc đang vướng

Không còn vướng kỹ thuật trong nghiệm thu W2-C. CLI parser/help/quiet và trình bày log của Huy chưa có; chương trình chính hiện chỉ chạy mặc định, API/record sẵn để ghép. Không xác nhận W2-H hoặc mốc chung Tuần 2 hoàn thành. T06–T08 chưa chạy; K0/queued-stop chỉ nghiệm thu phần kết thúc cơ bản 1P/1C, chưa đánh dấu toàn T09 nhiều luồng PASS. Giáo trình/kế hoạch Word liên quan chưa truy cập được trong môi trường đã kiểm tra, không tuyên bố đã đọc.

## 6. Công việc tiếp theo

Huy ghép parser/log vào [API W2-C](../handoff-week2-chau.md), kiểm tra đường CLI thực và ghi minh chứng W2-H riêng. Sau khi bản cơ bản được ghép, thực hiện nhiều producer/consumer và T06–T09 theo phạm vi Tuần 3. Bàn giao phần Châu bằng commit/push dev/chau, không merge main hoặc đổi dev/huy. [Tiến độ](../progress.md), [quy tắc báo cáo](README.md).
