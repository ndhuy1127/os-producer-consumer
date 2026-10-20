# Bàn giao Tuần 2 — Châu → Huy

W2-C đã nghiệm thu **lõi 1P/1C** trên máy Châu ngày 12/10/2026; [báo cáo](reports/week2.md), [minh chứng](../results/week2/chau-validation.md). Bộ đệm của Huy và log Tuần 1 giữ nguyên. Các phần dưới mô tả **snapshot bàn giao W2-C trước CLI/log**. Sau đó W2-H đã ghép/kiểm chứng qua Codex theo yêu cầu; [minh chứng Huy](../results/week2/huy-validation.md), [review bản ghép](../results/week2/group-validation.md); [PR #3](https://github.com/ndhuy1127/os-producer-consumer/pull/3) đã merge ở `d125734`. Không ghi Huy trực tiếp phê duyệt.

> **Tuần 3:** API đã mở rộng cho P, C = 1..64 (`pc_record.actor_index`, các trường mới cuối `pc_summary`, record tối đa 2·(P·K+C)); giới hạn "P=C=1" bên dưới chỉ còn đúng với snapshot Tuần 2. Xem [design.md §11](design.md#11-tuần-3--nhiều-producerconsumer).

## API cấu hình và vòng đời

[pc.h](../include/pc.h) công bố:

```c
struct pc_config config = pc_default_config(); /* 8,1,1,20,0,0 */
/* Parser của Huy gán các trường từ đầu vào đã kiểm tra. */
struct pc_result result = {0};
int status = pc_run(&config, &result);
/* Worker đã join nếu pc_run trả về; có thể đọc/in result tại đây. */
pc_result_destroy(&result);
return status;
```

Cấu hình gồm `capacity`, `producers`, `consumers`, `items_per_producer` (size_t) và `producer_delay_ms`, `consumer_delay_ms` (unsigned int). Tuần 2 chấp nhận N=1..65536 (còn giới hạn semaphore/kích thước), **P=C=1**, K=0..1000000, delay=0..60000ms. `pc_run` kiểm tra trước cấp phát/create; các giá trị nhiều luồng trong đặc tả dài hạn chưa được hỗ trợ. Giữ mặc định N=8; N=5/K=20 là ca nguồn riêng. DATA id=0..K-1, value=`id%1000+1`.

Mã trả: 0 khi chạy/xác minh/cleanup thành công; 1 khi tài nguyên/runtime lỗi; 2 khi cấu hình/đối số API sai. Lỗi đồng bộ/join nghiêm trọng in stderr rồi `_Exit(1)` toàn tiến trình, không trả control cho caller với worker còn sống. Producer create lỗi là đường có phục hồi: STOP/join consumer rồi trả 1. `result.summary.verified` chỉ là kết quả dữ liệu/vòng đời; caller phải kiểm tra **status==0** để bao gồm lỗi destroy. Không in PASS chỉ vì verified=1 khi status khác 0.

Result zero-init, sở hữu mảng records cho đến `pc_result_destroy`; destroy sau mọi lần pc_run trả về, kể cả lỗi init/create. Không sao chép result đang sở hữu để dùng/hủy như đối tượng khác. Khi muốn chạy lại, destroy rồi dùng lại result đã về zero. Context nội bộ trong pc.c sở hữu duy nhất buffer, semaphore, metadata và tần suất; caller không cần khóa/đọc trực tiếp context. Giữ API `buffer_init/push/pop/destroy` tuần tự, không thêm semaphore hoặc khóa lần nữa vào buffer.

## Record và tổng kết

`result.records[0..record_count-1]` được chụp trong guard, có sẵn theo thứ tự `op_seq` sau join. Dịch enum để trình bày theo [schema Tuần 1](config-log-week1-huy.md):

| Trường API | Cách trình bày |
| --- | --- |
| `op_seq` | uint64_t từ 1, dùng PRIu64 |
| `op` | PC_ENQUEUE → ENQUEUE; PC_DEQUEUE → DEQUEUE |
| `actor` | PC_PRODUCER → P0; PC_CONSUMER → C0; PC_MAIN → MAIN |
| `item.kind`, `item.id`, `item.value` | DATA/STOP, data_id số hoặc `-` với STOP, value |
| `enqueue_seq` | Từ 1 cho mỗi enqueue cả DATA/STOP; dequeue giữ sequence cùng item |
| `count_after`, `capacity` | Snapshot sau thao tác, size_t |
| `wait_reason` | PC_WAIT_NONE → NONE; PC_WAIT_SLOT → WAIT_SLOT; PC_WAIT_ITEM → WAIT_ITEM |

Context có metadata N ô riêng theo tail/head, không sửa struct item của Huy. Cấp phát trước `2*(K+1)` record sau kiểm tra tràn. Quiet chỉ bỏ in records; vẫn giữ oracle/tổng kết. Tương tự thiết kế chung, in sau join để không đưa I/O vào guard. `print_record` trong [test runner](../tests/test_pc.c) là ví dụ xuất schema phục vụ minh chứng, chưa phải module log production.

Summary có số DATA enqueue/dequeue/processed, thiếu/lặp/ngoài phạm vi, lỗi value/FIFO/count, min/max/final_count, STOP enqueue/dequeue, WAIT_SLOT/WAIT_ITEM, số create/join P/C, DATA còn đợi sau producer join và verified. Chỉ main ghi create/join; record và oracle cập nhật dưới guard. STOP chiếm chỗ nhưng không vào tập ID/thống kê DATA. EAGAIN ghi cờ thiếu quyền, không tự chứng minh block trong kernel hay count=N/0; snapshot count dưới guard chứng minh đầy/rỗng riêng.

## Build, ghép và kiểm tra

```sh
make all
timeout 5s ./bin/producer-consumer
make test
make pc-test
timeout 5s ./bin/pc-test source-N5 --records
timeout 7s ./bin/pc-test T04 --records
timeout 7s ./bin/pc-test T05 --records
```

Ở snapshot W2-C, main chạy không đối số, in tổng kết tối thiểu và từ chối mọi đối số với exit 2. Phần Huy cần ghép parser/help/quiet và trình bày log tại entry point, điền pc_config rồi gọi cùng pc_run; việc này đã hoàn thành ở [main.c](../src/main.c) hiện tại. Nếu thêm module C vào src/, Makefile đưa vào chương trình chính; main của test/ví dụ vẫn build riêng. Lỗi parse phải kết thúc trước pc_run/create; kiểm tra giới hạn kiểu đích theo đặc tả, không dùng atoi. Các ca config API không thay thế T08; parser thật được kiểm chứng trong minh chứng Huy/review chung.

T02–T05 đã đạt **trên lõi**; sau ghép CLI/log, Huy kiểm tra lại đường chạy sản phẩm/cờ/quiet và ghi minh chứng riêng. K0 và queued-stop chỉ kiểm tra kết thúc cơ bản 1P/1C; T09 còn cấu hình nhiều consumer. T06–T08 và mở rộng nhiều luồng Tuần 3 chưa làm. Không gán phần test lõi này thành xác nhận W2-H hoàn thành.
