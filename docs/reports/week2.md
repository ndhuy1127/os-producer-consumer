# Báo cáo chung Tuần 2 — Bản cơ bản

**W2-C và W2-H đạt 100% phạm vi cơ bản 1 producer/1 consumer; bản ghép đã được Codex review và kiểm tra đạt theo yêu cầu.** Kiểm chứng ngày 12/10/2026 (Asia/Saigon) trên checkout ChouPro Ubuntu/WSL; không ghi Huy/Châu trực tiếp review/phê duyệt hoặc cả hai đã đọc chéo. Minh chứng Châu/Huy giữ nguyên; kết quả review ghi riêng [group-validation](../../results/week2/group-validation.md). Trạng thái bàn giao main và merge SHA được xác nhận trên GitHub tại [PR #3](https://github.com/ndhuy1127/os-producer-consumer/pull/3), không suy ra từ tài liệu trước merge. Chưa có ngày bắt đầu/hạn nộp.

## 1. Mục tiêu tuần

- Châu: Producer/consumer, semaphore, tích hợp và kết thúc hữu hạn.
- Huy: Bộ đệm, tham số, log, kiểm thử đầy/trống và sức chứa 1.
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

Codex thực hiện phần W2-H theo yêu cầu trên dev/huy, máy ChouPro WSL. Nhận W2-C bằng fast-forward lịch sử từ 9cf345c tới bd00780 trước khi sửa; không sao chép repo/cherry-pick hoặc thay nhánh Châu/main. Không cần viết lại buffer đã đạt, không tìm thấy lỗi lõi cần sửa.

| Nhiệm vụ | Việc thực tế đã làm | Trạng thái | Minh chứng |
| --- | --- | --- | --- |
| Hoàn thiện bộ đệm và tích hợp | Rà hợp đồng/zero-init/copy/sở hữu, giữ byte buffer và pc.c/pc.h, không khóa thêm; ghép đúng parse→run→log→destroy | Đã hoàn thành | [Main](../../src/main.c), [hồi quy](../../results/week2/huy-regression-tests.log), [rà soát](../../results/week2/huy-review.log) |
| CLI và kiểm tra đầu vào | Tám cờ, defaults N=8/1P/1C/K=20; strtoumax/end/ERANGE/kiểu/tràn; từ chối lỗi trước pc_run; P/C>1 báo chưa hỗ trợ Tuần 2 | Đã hoàn thành | [Parser](../../src/cli.c), [116 đầu vào sai và wrapper](../../results/week2/huy-cli-tests.log) |
| Log và tổng kết | Schema theo record trong guard, in sau join; PRIu64/%zu, STOP riêng; quiet vẫn giữ oracle/tổng kết; status=1/verified=1 vẫn FAIL | Đã hoàn thành | [Logger](../../src/runtime_log.c), [kiểm tra formatter/main errors](../../results/week2/huy-log-tests.log) |
| N=1 và đầy/trống | Tám cấu hình qua production, cả thường/quiet; oracle FIFO tuyến tính, ID/count/STOP/create/join, snapshot và thiếu quyền thực | Đã hoàn thành | [16 lần chạy CLI](../../results/week2/huy-runtime-tests.log) |
| Lưu minh chứng và báo cáo | Script chỉ huy-*, môi trường/time/HEAD nền/hash nguồn/exit, hồi quy và ASan/UBSan/leak detection; cập nhật README/design/test-plan/progress | Đã hoàn thành | [Đánh giá Huy](../../results/week2/huy-validation.md), [manifest](../../results/week2/huy-source.sha256) |

## 4. Kết quả tích hợp và kiểm thử của nhóm

**Kết quả bàn giao W2-C dưới đây là lịch sử trước khi có CLI**, giữ nguyên dữ liệu kiểm chứng Châu: lõi ghép buffer Tuần 1 trên máy Châu, Ubuntu 24.04.5/WSL2, GCC 13.3.0, C11/cảnh báo/-pthread. Branch dev/chau, HEAD nền `9cf345c12ca59fb1a6bf476147551cb89f7e043b` cộng nguồn mới; [manifest Châu](../../results/week2/chau-source.sha256) và header log ghi hash thực chạy. Sau đó W2-C được bàn giao ở bd00780; bản Huy dùng HEAD nền đó cộng CLI/log mới, không gán code mới cho SHA nền.

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

**Kết quả bản ghép W2-H qua chương trình chính với CLI/log thật:** mỗi dòng dưới chạy thường và quiet, đều exit 0. Không dùng test runner Châu thay cho kiểm chứng CLI. Cấu hình là N/P/C/K/Dp_ms/Dc_ms; mọi ID đúng một lần, FIFO, count [0,N]/cuối 0, STOP 1/1 và create/join 1P/1C. [Lệnh/record thực](../../results/week2/huy-runtime-tests.log).

| Ca | Cấu hình | Kết quả thường / quiet | Exit |
| --- | --- | --- | --- |
| Không đối số | 8/1/1/20/0/0 | PASS / PASS | 0 / 0 |
| Ca nguồn | 5/1/1/20/0/0 | PASS / PASS | 0 / 0 |
| T02 | 8/1/1/100/0/0 | PASS / PASS | 0 / 0 |
| T03 | 1/1/1/100/0/0 | PASS, 101 đầy/101 rỗng / PASS | 0 / 0 |
| T04 | 2/1/1/40/0/20 | PASS: 40 snapshot đầy, 39 WAIT_SLOT (38 P0) / PASS | 0 / 0 |
| T05 | 2/1/1/40/20/0 | PASS: 41 snapshot rỗng, 40 WAIT_ITEM / PASS | 0 / 0 |
| K=0 | 1/1/1/0/0/0 | PASS: 0 DATA, STOP 1/1 / PASS | 0 / 0 |
| Còn hàng trước STOP | 8/1/1/20/0/20 | PASS: còn 8 DATA sau producer join / PASS | 0 / 0 |

116 đầu vào CLI sai được chạy trên cả production và binary có wrapper chặn pc_run/pthread_create: exit 2, stderr cụ thể, không gọi lõi/worker; positive control wrapper exit 99 đúng dự kiến. Help dùng riêng trên hai binary exit 0; 5 biên parser không chạy worker; formatter 7/7; 6 ca lỗi main ở thường/quiet exit 1, không PASS, destroy result đúng. Hồi quy thread-demo/buffer 12/lõi 8/lỗi Châu 26 đạt. ASan/UBSan/leak detection ngoài sandbox đạt ở production, parser biên, logger và cleanup main. [CLI](../../results/week2/huy-cli-tests.log), [log/errors](../../results/week2/huy-log-tests.log), [build](../../results/week2/huy-build.log), [sanitizer](../../results/week2/huy-sanitizers.log), [đánh giá](../../results/week2/huy-validation.md).

**Review bản ghép theo yêu cầu bàn giao:** Codex đọc diff origin/main→dev/huy, kế hoạch/báo cáo/thiết kế/hợp đồng, nguồn chính, Makefile/tests/checker và manifest/log lịch sử. Không phát hiện lỗi runtime cần sửa trong phạm vi 1P/1C. Đã thực chạy build sạch đủ production/thread-demo/test, thread-demo và một lần make test: buffer 12/12, lõi 8/8, lỗi/API 26/26, production thường/quiet 16/16, 116 CLI sai × production/hook, help/5 biên parser/7 formatter/6 lỗi main đều đạt; mọi target exit 0, child lỗi trả mã đúng dự kiến. Kiểm tra khung/liên kết/diff/hash đạt. [Log mới](../../results/week2/group-review.log), [manifest 22 file](../../results/week2/group-source.sha256) khớp nguồn commit 7c356e9. Chỉ sửa tài liệu/minh chứng; không chạy sanitizer mới, sử dụng log đã bàn giao khớp nguồn với leak detection bật. 31 file minh chứng lịch sử/báo cáo Tuần 1 được đối chiếu nguyên byte.

## 5. Việc chưa hoàn thành hoặc đang vướng

Không còn vướng kỹ thuật trong W2-C/W2-H phạm vi Tuần 2 cơ bản; review và kiểm tra bản ghép đạt. Trạng thái merge/bàn giao main theo PR #3. P/C>1 chưa hỗ trợ; T06/T07 chưa chạy, T08 chỉ CLI hiện hỗ trợ 1P/1C, K0/queued-stop không đánh dấu toàn T09 nhiều luồng PASS. Chưa xác nhận Huy/Châu trực tiếp review/phê duyệt hay hai người đọc chéo. Giáo trình/kế hoạch Word liên quan chưa được truy cập để xác minh, không tuyên bố đã đọc.

## 6. Công việc tiếp theo

Bàn giao qua PR #3 bằng merge commit, giữ commit gốc và hai nhánh thành viên; trạng thái thao tác thực xem GitHub/PR và kết quả bàn giao. Sau merge, đồng bộ main/dev/chau/dev/huy local/remote, đối chiếu tree/hash với bản review, checkout dev/chau rồi smoke test ngắn. Tuần 3 mới triển khai nhiều producer/consumer, T06/T07/phần nhiều luồng T09 và cập nhật giới hạn CLI; không triển khai công việc đó trong lượt chốt Tuần 2. [Tiến độ](../progress.md), [quy tắc báo cáo](README.md).
