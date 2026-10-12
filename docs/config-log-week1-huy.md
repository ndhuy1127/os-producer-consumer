# Đặc tả cấu hình và log — tích hợp Tuần 1

Đặc tả Tuần 1 do Huy chuẩn bị, được Codex rà soát/chốt theo yêu cầu Châu; không ghi phê duyệt hoặc đọc chéo trực tiếp. Cơ sở PR #1/thiết kế Châu ở `2c8d7f6`: P producer, C consumer, K DATA mỗi producer, tổng P*K, ID=p*K+j; STOP không tính DATA. **Tuần 2 đã triển khai CLI/log production trên dev/huy qua Codex theo yêu cầu**, dùng nguyên lõi W2-C; [minh chứng mới](../results/week2/huy-validation.md) tách khỏi [minh chứng Châu](../results/week2/chau-validation.md). API buffer giữ nguyên, không khóa thêm bên trong.

## CLI thực hiện Tuần 2

`bin/producer-consumer` dùng [cli.c](../src/cli.c) qua [main.c](../src/main.c). Mặc định và giới hạn dữ liệu/độ trễ giữ theo Tuần 1. P/C=1..64 là đặc tả dài hạn; **bản Tuần 2 chỉ P=C=1**, >1 từ chối trước pc_run với thông báo chưa hỗ trợ. Không quảng cáo runtime nhiều luồng.

| Cờ | Mặc định | Phạm vi đã chốt | Ý nghĩa |
| --- | --- | --- | --- |
| `--capacity N` | 8 | 1..65536, đồng thời <= SEM_VALUE_MAX và SIZE_MAX/sizeof(struct item) | Sức chứa gồm DATA và STOP |
| `--producers P` | 1 | Tuần 2: chỉ 1; kế hoạch nhiều luồng: 1..64 | Số producer |
| `--consumers C` | 1 | Tuần 2: chỉ 1; kế hoạch nhiều luồng: 1..64 | Số consumer |
| `--items-per-producer K` | 20 | 0..1000000, tổng P*K <=1000000 | DATA của mỗi producer; K=0 chỉ kiểm tra dừng |
| `--producer-delay-ms Dp` | 0 | 0..60000 | Độ trễ sau enqueue DATA, ngoài guard |
| `--consumer-delay-ms Dc` | 0 | 0..60000 | Độ trễ sau xử lý DATA, ngoài guard |
| `--quiet` | tắt | Cờ không có giá trị | Giảm dòng chi tiết; vẫn xác minh và in tổng kết/lỗi |
| `--help` | — | Dùng riêng | In trợ giúp, exit 0 |

Chỉ nhận `--cờ giá_trị`; dạng `--cờ=giá_trị` bị từ chối và help ghi rõ. Số chỉ gồm chữ số ASCII thập phân không dấu, không chấp nhận âm, dấu +, whitespace, rỗng, số thực hoặc hậu tố. Parser dùng strtoumax với end pointer/errno/ERANGE và giới hạn kiểu đích, không dùng atoi. Sau kiểm tra từng giá trị, kiểm tra trước phép nhân P*K (K=0 riêng), cộng C STOP, nhân hai thao tác và kích thước record; kiểm tra ID/tần suất/buffer/metadata và semaphore giống điều kiện lõi. pc_run vẫn kiểm tra cuối trước cấp phát/create, không đổi quy tắc lõi. Delay ms và EINTR khi nanosleep do pc.c thực hiện ngoài guard.

Cờ lạ/lặp/thiếu, đối số vị trí, giá trị ngoài giới hạn và --help kết hợp cờ khác: stderr cụ thể, exit 2 trước pc_run/create. --help dùng riêng exit 0, không worker. Lỗi tài nguyên/runtime/kiểm chứng/output exit 1; thành công exit 0 chỉ khi status==PC_OK và summary.verified. Không có chạy vô hạn. 116 đầu vào sai đã thử trên production và binary dùng cùng main với wrapper chặn pc_run/pthread_create; 5 ca parser riêng thử biên lớn không chạy worker. [Log CLI](../results/week2/huy-cli-tests.log).

## Schema log đã chốt cho Tuần 2

Mỗi sự kiện thao tác thành công gồm: `op_seq` (toàn cục từ 1), `op=ENQUEUE|DEQUEUE`, `actor=P0|C0|MAIN`, `kind=DATA|STOP`, `data_id` (số hoặc `-` với STOP), `value`, `enqueue_seq` (thứ tự enqueue gắn với item), `count_after`, `capacity`, `wait_reason=NONE|WAIT_SLOT|WAIT_ITEM`. `enqueue_seq` lưu trong mảng metadata N ô do context sở hữu, theo cùng ô tail/head của buffer; không đổi struct item đã thống nhất. Với DEQUEUE phải lấy metadata cùng ô trước khi cập nhật head. Không cần thêm dequeue_seq: lấy các DEQUEUE theo op_seq; không dùng timestamp làm chứng cứ FIFO.

Ví dụ minh họa schema, **không phải log thực chạy**:

```text
op_seq=1 op=ENQUEUE actor=P0 kind=DATA data_id=0 value=7 enqueue_seq=1 count_after=1 capacity=8 wait_reason=NONE
op_seq=2 op=DEQUEUE actor=C0 kind=DATA data_id=0 value=7 enqueue_seq=1 count_after=0 capacity=8 wait_reason=NONE
```

Lõi Châu tăng số thứ tự/lưu record sau push/pop trong guard, gồm STOP; cấp phát trước tối đa 2*(K+1) cho 1P/1C sau kiểm tra tràn. [runtime_log.c](../src/runtime_log.c) đọc result sau pc_run/join và in theo op_seq, dùng PRIu64/%zu đúng kiểu. Không đưa I/O vào guard hoặc thêm semaphore. FIFO theo record trong guard, không dùng timestamp/thứ tự xử lý. Quiet bỏ record chi tiết nhưng giữ oracle/record nội bộ, kiểm chứng và tổng kết; main luôn pc_result_destroy sau kết quả có phục hồi, không sao chép result sở hữu.

Tổng kết thực: cấu hình; DATA enqueue/dequeue/processed; thiếu/lặp/ngoài phạm vi, value_errors/FIFO_errors/count_errors; min/max/final count gồm ban đầu 0; STOP enqueue/dequeue; WAIT_SLOT/WAIT_ITEM; record_count; DATA còn đợi sau producer join; số worker create/join; verification và run_status. PASS chỉ khi status=0 và verified đúng. Ca tiêm sem_destroy lỗi có verified=1 nhưng status=1 vẫn FAIL, kể cả quiet; [log kiểm tra](../results/week2/huy-log-tests.log).

## Bằng chứng chờ đầy/trống

Chốt helper chờ empty/full dùng `sem_trywait(empty|full)` trước `sem_wait`: thành công thì đã lấy một quyền và không wait thêm; EINTR thử lại; EAGAIN đặt cờ `WAIT_SLOT`/`WAIT_ITEM` cục bộ rồi gọi sem_wait thử lại EINTR; gắn cờ vào record thao tác khi đã lấy guard, không tăng số thứ tự chung ngoài khóa. Lỗi khác chuyển xử lý lỗi. EAGAIN chứng minh tại thời điểm thử chưa có quyền, không chứng minh sem_wait sau đó chắc chắn bị block hoặc count luôn bằng N/0 (có quyền đang đặt trước). Không đếm thời gian block chính xác từ sự kiện này.

Không lấy quyết định an toàn từ count không khóa hoặc sem_getvalue. Record count_after=N/0 trong guard chứng minh đầy/trống riêng; EAGAIN chứng minh thiếu quyền. Không giữ guard khi chờ quyền/sleep. Helper Châu đã triển khai/kiểm chứng; guard dùng sem_wait trực tiếp. T04 qua CLI có 40 snapshot đầy, 39 WAIT_SLOT (38 P0); T05 có 41 snapshot rỗng/40 WAIT_ITEM trong [lần chạy thường đã lưu](../results/week2/huy-runtime-tests.log). Không suy ra thời gian block kernel.

Timeout W2-H dùng 5+ceil(2*K*(Dp_ms+Dc_ms)/1000) giây cho 1P/1C (Python integer không tràn): 5s ca không delay, 7s T04/T05, 6s queued-stop. Đây là ngân sách kiểm thử bảo thủ, không bảo đảm scheduler. [Test-plan](test-plan.md) ghi phạm vi CLI thực; T06/T07 và T09 nhiều luồng chưa nghiệm thu.
