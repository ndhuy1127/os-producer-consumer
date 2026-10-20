# tests

Chạy tất cả bằng `make test`; từng nhóm như bên dưới. Hướng dẫn đầy đủ ở [run-guide.md](../docs/run-guide.md), kết quả thực chạy Tuần 3 ở [results/week3](../results/week3/README.md).

| File | Lệnh | Nội dung |
| --- | --- | --- |
| [test_buffer.c](test_buffer.c) | `make test-buffer` | 12 ca bộ đệm tuần tự: FIFO với oracle độc lập, đầy/rỗng, quay vòng, N=1, STOP, tham số sai, vòng đời, tràn kích thước, malloc lỗi (GNU ld wrapper); kiểm tra mảng được free sau mỗi ca |
| [test_pc.c](test_pc.c) | `make test-pc`, hoặc `./bin/pc-test <preset> [--records]` | 16 preset gọi `pc_run`: 8 cấu hình 1P/1C của Tuần 2 và T06a, T06b, T07, T09a, T09b, T04-multi, T05-multi, queued-stop-multi |
| [test_pc_faults.c](test_pc_faults.c) | `make test-errors` | 32 ca tiêm lỗi bằng GNU ld wrapper, mỗi ca một tiến trình, timeout 5 s |
| [test_cli.c](test_cli.c) | trong `make test-cli` | Driver chỉ parse tham số, thử biên lớn mà không chạy luồng |
| [test_runtime_log.c](test_runtime_log.c) | trong `make test-cli` | 8 ca formatter: uint64 lớn, STOP `data_id=-`, chỉ số luồng C63, quiet, status/verified sai vẫn FAIL, lỗi output, chỉ số luồng ≥ 64 bị từ chối |
| [test_main_hooks.c](test_main_hooks.c) | trong `make test-cli` | Link với `main` thật; chặn `pc_run`/`pthread_create` để chứng minh tham số sai không chạy lõi; tiêm lỗi init/cleanup/verified |

**Oracle của test_pc.c** là một hàng đợi tuyến tính chỉ thêm, độc lập với bộ đệm vòng. Với từng bản ghi, nó kiểm tra kind/id/value/enqueue_seq so với phần tử đưa vào tương ứng; ID p·K+j phải do producer p đưa vào theo thứ tự j và được lấy ra theo cùng thứ tự; count bằng số thêm trừ số lấy và nằm trong [0, N]; mọi DATA lấy ra trước STOP đầu tiên; mỗi consumer nhận đúng một STOP; tổng kết theo luồng khớp số đếm của oracle; P+C luồng được tạo và join. T04/T04-multi bắt buộc có snapshot đầy và WAIT_SLOT; T05/T05-multi bắt buộc có snapshot rỗng và WAIT_ITEM; queued-stop và queued-stop-multi bắt buộc còn DATA khi main gửi STOP.

**Tiêm lỗi (test_pc_faults.c):** `EINTR` ở `sem_wait`/`sem_trywait` (cả khi 3P/3C) phải được thử lại; tạo consumer/producer lỗi ở luồng đầu tiên hoặc giữa chừng phải join đúng các luồng đã chạy; lỗi khởi tạo semaphore/cấp phát phải dọn đúng phần đã tạo; tham số sai (N=0/65537, P/C=0/65, K quá lớn, P·K > 1000000, độ trễ > 60000) bị từ chối trước khi tạo luồng; 7 lỗi nghiêm trọng phải thoát mã 1 kèm tên lời gọi lỗi. Mọi ca có phục hồi kiểm tra số lần tạo = số lần join, số semaphore khởi tạo = số lần hủy, không còn vùng nhớ chưa free. Ca chờ `EINTR` dùng semaphore riêng của test để biết consumer đã vào `sem_wait(full)`, không dùng `sleep` để đoán.

Wrapper chỉ được link vào binary test, không có trong `bin/producer-consumer`. Fixture của formatter là dữ liệu dựng sẵn để thử định dạng, không phải bằng chứng FIFO khi chạy thật.
