# tests

[test_buffer.c](test_buffer.c) gồm 12 ca tuần tự, chạy bằng `make test-buffer` hoặc `make test`. Kiểm tra từng kind/id/value với oracle FIFO độc lập, lỗi giữ nguyên dữ liệu, biên N=1, STOP, vòng đời, tràn kích thước và malloc thất bại được tiêm bằng GNU ld wrapper; kiểm tra mảng được free sau mỗi ca. Exit 0 chỉ khi tất cả ca PASS; lỗi trả EXIT_FAILURE.

Bản sanitizer không bật wrapper chạy 11 ca thường. [Minh chứng Tuần 1](../results/week1/huy-validation.md) giữ nguyên.

[test_pc.c](test_pc.c) gọi cùng pc_run với tám preset: default, source-N5, T02, T03, T04, T05, K0, queued-stop. Oracle là dãy tuyến tính append/read, độc lập với buffer vòng; đối chiếu từng kind/id/value/enqueue_seq, tần suất ID, count theo số lần thêm/lấy, actor/wait, STOP và cả hai create/join. T04 bắt buộc có snapshot đầy và WAIT_SLOT; T05 bắt buộc rỗng và WAIT_ITEM; queued-stop bắt buộc có DATA còn đợi sau producer join. Chạy `make test-pc` hoặc `timeout 7s ./bin/pc-test T04 --records`.

[test_pc_faults.c](test_pc_faults.c) dùng GNU ld wrapper tách khỏi production; [check_pc_errors.py](../scripts/check_pc_errors.py) chạy 26 ca riêng với timeout 5s mỗi ca. EINTR cho guard/quyền/trywait phải phục hồi; producer create lỗi được đồng bộ bằng semaphore test để consumer đã vào đường sem_wait(full) sau EAGAIN, không dùng sleep đoán readiness. Kiểm tra join đủ, destroy đúng tài nguyên, cấp phát được free. Ca fatal phải exit 1 cùng thông báo lời gọi lỗi; không giả định đã cleanup sau fatal. Bộ kiểm tra cấu hình gọi API, **không** nghiệm thu parser CLI/T08.

Các test Châu giữ nguyên; [minh chứng Châu](../results/week2/chau-validation.md) là snapshot bàn giao. `make test` hiện chạy ba nhóm đó và test-cli.

[test_cli.c](test_cli.c) là driver parser riêng để thử biên lớn (N=65536/K=1000000/delay=60000) không chạy worker. [test_runtime_log.c](test_runtime_log.c) có 7 ca formatter: số uint64_t lớn, STOP data_id=-, quiet, status=1/verified=1 vẫn FAIL, verified=0 vẫn FAIL và output error. Fixture formatter không phải bằng chứng dữ liệu/FIFO runtime.

[test_main_hooks.c](test_main_hooks.c) chỉ link vào main-hook-test với cùng main production; trap pc_run/pthread_create (exit 99) ở parse sai/help, có positive control. Init/cleanup/verified được tiêm để kiểm tra main trả 1, không PASS và vẫn destroy records ở thường/quiet.

[check_cli_log.py](../scripts/check_cli_log.py) chạy production 16 ca (8 thường/8 quiet) và 116 đầu vào sai + wrapper; parse stdout thành queue tuyến tính độc lập để kiểm tra từng ID/kind/value/FIFO/count/STOP/schema/summary. Quiet giữ records/oracle và được kiểm tra summary cùng các ca lỗi, không có record để làm oracle stdout. [Minh chứng Huy](../results/week2/huy-validation.md) phân biệt hai phạm vi này. T06/T07, runtime nhiều luồng và toàn T09 còn chờ Tuần 3; T08 chỉ CLI 1P/1C đã kiểm tra.
