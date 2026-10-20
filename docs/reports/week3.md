# Báo cáo chung Tuần 3 — Nhiều luồng và kiểm thử

**Bản nhiều producer/consumer đã hoàn thành và đạt toàn bộ kiểm thử đã chạy; có báo cáo nháp.** Cả phần W3-C (phân công của Châu) và W3-H (phân công của Huy) đều do **Claude Code thực hiện theo yêu cầu của Huy** trên nhánh `dev/huy`, kiểm chứng ngày 20/10/2026 (Asia/Saigon) trên máy của Huy. **Châu chưa review/xác nhận phần W3-C**; hai thành viên chưa đọc chéo code. Chưa có ngày bắt đầu/hạn nộp.

## 1. Mục tiêu tuần

- Châu: nhiều producer/consumer, mã dữ liệu riêng, rà soát đồng bộ, phần lý thuyết/thuật toán của báo cáo.
- Huy: script kiểm thử, xác thực dữ liệu/FIFO, bảng kết quả, hướng dẫn chạy.
- Mốc: bản nhiều luồng được kiểm thử và có báo cáo nháp.

## 2. Công việc phần Châu (W3-C)

Thực hiện bởi Claude Code theo yêu cầu của Huy, commit `c99d131`; Châu chưa review. Giữ nguyên ba semaphore, thứ tự lấy quyền trước `guard`, API bộ đệm và các đường lỗi của Tuần 2.

| Nhiệm vụ | Việc thực tế đã làm | Trạng thái | Minh chứng |
| --- | --- | --- | --- |
| Nhiều producer/consumer | `pc_run` nhận P, C = 1..64, K ≤ 1000000, P·K ≤ 1000000 (kiểm tra tràn trước nhân/cấp phát); tạo C consumer rồi P producer; join mọi producer, gửi C STOP, join mọi consumer. Tạo luồng lỗi giữa chừng: dừng và join đúng các luồng đã chạy rồi trả 1 | Đã hoàn thành, chờ Châu review | [pc.c](../../src/pc.c), [pc.h](../../include/pc.h), [thiết kế §11](../design.md#11-tuần-3--nhiều-producerconsumer) |
| Mã dữ liệu riêng | Producer p sinh ID p·K+j; bản ghi có chỉ số luồng; tự kiểm chứng chủ sở hữu ID và thứ tự j trong từng producer (`order_errors`), mỗi consumer đúng một STOP | Đã hoàn thành, chờ Châu review | [pc.c](../../src/pc.c), [16 preset lõi](../../results/week3/core-tests.log) |
| Rà soát đồng bộ | Rà lại mọi truy cập dữ liệu chung khi có nhiều luồng; ThreadSanitizer trên 9 cấu hình chương trình chính và 16 preset lõi không phát hiện data race; 32 ca tiêm lỗi gồm `EINTR` khi 3P/3C và tạo luồng lỗi giữa chừng | Đã hoàn thành, chờ Châu review | [sanitizers.log](../../results/week3/sanitizers.log), [core-tests.log](../../results/week3/core-tests.log) |
| Lý thuyết/thuật toán cho báo cáo | Báo cáo nháp mục 2–5: lý thuyết, thiết kế, giả mã nhiều luồng, lập luận không tràn/rỗng, không mất/lặp, FIFO, không bế tắc, kết thúc hữu hạn, giới hạn về công bằng; đối chiếu lại `sem_overview(7)`, `sem_wait(3)`, `sem_post(3)` | Đã hoàn thành, chờ Châu đọc và xác nhận | [Báo cáo nháp](../final-report/bao-cao-nhap.md) |

## 3. Công việc phần Huy (W3-H)

Thực hiện bởi Claude Code theo yêu cầu của Huy, commit `3bda04b` (code/script) và commit minh chứng/tài liệu kèm báo cáo này.

| Nhiệm vụ | Việc thực tế đã làm | Trạng thái | Minh chứng |
| --- | --- | --- | --- |
| CLI và log nhiều luồng | `--producers`/`--consumers` 1..64, từ chối P·K > 1000000 trước khi chạy lõi; log in `P<p>`/`C<c>`/`MAIN`, thêm `order_errors` và tổng kết theo từng luồng | Đã hoàn thành | [cli.c](../../src/cli.c), [runtime_log.c](../../src/runtime_log.c), [cli-tests.log](../../results/week3/cli-tests.log) |
| Script kiểm thử | `check_cli_log.py` mở rộng lên 15 cấu hình; `check_stress.py` (`make test-stress`) chạy lặp 7 cấu hình × 20; `validate_week3.py` ghi minh chứng gồm ASan/UBSan và TSan | Đã hoàn thành | [scripts](../../scripts/README.md), [stress-tests.log](../../results/week3/stress-tests.log) |
| Xác thực dữ liệu/FIFO | Oracle hàng đợi tuyến tính dựng từ log kiểm tra từng ID, FIFO, chủ sở hữu và thứ tự j, count trong [0, N], STOP từng consumer, khớp tổng kết theo luồng | Đã hoàn thành | [check_cli_log.py](../../scripts/check_cli_log.py), [runtime-tests.log](../../results/week3/runtime-tests.log) |
| Bảng kết quả | Bảng sinh tự động từ lần chạy thật: từng cấu hình, chạy lặp, sanitizer | Đã hoàn thành | [results-table.md](../../results/week3/results-table.md) |
| Hướng dẫn chạy | Cài đặt, build, tham số, ví dụ, cách đọc log, các lệnh kiểm thử kèm thời gian đo, sự cố thường gặp | Đã hoàn thành | [run-guide.md](../run-guide.md) |

## 4. Kết quả tích hợp và kiểm thử của nhóm

Chạy bằng `python3 scripts/validate_week3.py` ngày 20/10/2026 lúc 23:17 (Asia/Saigon), WSL2 Ubuntu 24.04, GCC 13.3.0, Make 4.3, Python 3.12.3; mã nguồn đúng commit `3bda04b`, cây nguồn sạch; manifest 23 file ở [source.sha256](../../results/week3/source.sha256). Script exit 0.

| Lệnh/nhóm | Cấu hình | Kết quả | Exit |
| --- | --- | --- | --- |
| `make clean`; build mọi target | C11, `-Wall -Wextra -Wpedantic`, `-pthread` | Không cảnh báo | 0 |
| `timeout 5s ./bin/thread-demo`; `make test-buffer` | Ví dụ Tuần 1; 12 ca bộ đệm | Đạt; 12/12 | 0 |
| `make test-pc` | 16 preset: 8 cấu hình 1P/1C cũ, T06a, T06b, T07, T09a, T09b và biến thể nhiều luồng đầy/rỗng/còn hàng | 16/16 | 0 |
| `make test-errors` | 32 ca tiêm lỗi | 32/32; các ca lỗi nghiêm trọng thoát 1 đúng dự kiến | 0 |
| `make test-cli` | 15 cấu hình × thường/quiet; 114 tham số sai × 2 binary; 6 biên; formatter 8; lỗi main 6 | Đạt hết | 0 |
| `check_stress.run_stress(20)` | T06a, T06b, T07, T09a, T09b, 8P8C N=1, 64P64C | 140/140 lần đạt oracle | 0 |
| ASan+UBSan, TSan | 9 cấu hình chương trình chính + 16 preset lõi mỗi công cụ | Không cảnh báo | 0 |
| Kiểm tra khung, `git diff --check`, Tuần 1–2 không đổi | — | Đạt | 0 |

Quan sát chính (số của lần chạy này, phụ thuộc bộ lập lịch): T04-multi có 74 snapshot bộ đệm đầy và 71 lần producer thiếu chỗ; T05-multi có 63 snapshot rỗng và 66 lần consumer thiếu phần tử; T09a (N=1, 4 consumer, K=0) có WAIT_ITEM = 4 ở cả 20/20 lần chạy lặp, tức cả bốn consumer đều đang chờ khi STOP tới; queued-stop-multi còn 8 DATA trong bộ đệm khi main bắt đầu gửi STOP và tất cả được lấy trước STOP. T07 (40000 DATA) chạy 0,118–0,205 s; một consumer xử lý 11779–14437 phần tử, cho thấy không có bảo đảm chia đều. Chi tiết ở [bảng kết quả](../../results/week3/results-table.md) và [minh chứng Tuần 3](../../results/week3/README.md).

Trước khi sửa code, bản Tuần 2 đã merge (`d125734`) được chạy lại trên máy Huy và đạt, ghi ở [báo cáo Tuần 2](week2.md) (commit `d85b6d1`).

## 5. Việc chưa hoàn thành hoặc đang vướng

- Châu chưa review W3-C và chưa đọc/xác nhận mục 2–5 của báo cáo nháp; hai thành viên chưa đọc chéo code.
- Commit `c99d131` khi build riêng có hai cảnh báo `-Wmissing-field-initializers` trong fixture của `tests/test_runtime_log.c` (không ở code chương trình); commit `3bda04b` ngay sau đã sửa. Lịch sử không được viết lại.
- Kiểm thử, kể cả TSan, chỉ quan sát những lịch chạy đã xảy ra; lập luận ở báo cáo nháp bổ sung nhưng chưa được Châu kiểm tra.
- Chưa đối chiếu giáo trình *Modern Operating Systems*; chưa có slide.
- Các thay đổi Tuần 3 mới nằm trên `dev/huy`, chưa push/tạo PR vào main.

## 6. Công việc tiếp theo

Push `dev/huy` và tạo PR vào main khi Huy đồng ý; Châu review W3-C và báo cáo nháp trước khi merge. Tuần 4 theo [kế hoạch](../plan.md): hoàn thiện báo cáo và slide, đọc chéo code, luyện demo, chuẩn bị gói nộp. [Tiến độ](../progress.md), [quy tắc báo cáo](README.md).
