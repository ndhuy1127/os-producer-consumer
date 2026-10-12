# tests

[test_buffer.c](test_buffer.c) gồm 12 ca tuần tự, chạy bằng `make test-buffer` hoặc `make test`. Kiểm tra từng kind/id/value với oracle FIFO độc lập, lỗi giữ nguyên dữ liệu, biên N=1, STOP, vòng đời, tràn kích thước và malloc thất bại được tiêm bằng GNU ld wrapper; kiểm tra mảng được free sau mỗi ca. Exit 0 chỉ khi tất cả ca PASS; lỗi trả EXIT_FAILURE.

Bản sanitizer không bật wrapper chạy 11 ca thường. [Minh chứng Tuần 1](../results/week1/huy-validation.md) giữ nguyên.

[test_pc.c](test_pc.c) gọi cùng pc_run với tám preset: default, source-N5, T02, T03, T04, T05, K0, queued-stop. Oracle là dãy tuyến tính append/read, độc lập với buffer vòng; đối chiếu từng kind/id/value/enqueue_seq, tần suất ID, count theo số lần thêm/lấy, actor/wait, STOP và cả hai create/join. T04 bắt buộc có snapshot đầy và WAIT_SLOT; T05 bắt buộc rỗng và WAIT_ITEM; queued-stop bắt buộc có DATA còn đợi sau producer join. Chạy `make test-pc` hoặc `timeout 7s ./bin/pc-test T04 --records`.

[test_pc_faults.c](test_pc_faults.c) dùng GNU ld wrapper tách khỏi production; [check_pc_errors.py](../scripts/check_pc_errors.py) chạy 26 ca riêng với timeout 5s mỗi ca. EINTR cho guard/quyền/trywait phải phục hồi; producer create lỗi được đồng bộ bằng semaphore test để consumer đã vào đường sem_wait(full) sau EAGAIN, không dùng sleep đoán readiness. Kiểm tra join đủ, destroy đúng tài nguyên, cấp phát được free. Ca fatal phải exit 1 cùng thông báo lời gọi lỗi; không giả định đã cleanup sau fatal. Bộ kiểm tra cấu hình gọi API, **không** nghiệm thu parser CLI/T08.

`make test` chạy cả ba nhóm. [Minh chứng Tuần 2](../results/week2/chau-validation.md) gồm sanitizer tám ca lõi, production mặc định và 11 ca buffer; T06–T08 và T09 nhiều luồng còn chờ.
