# Hướng dẫn chạy và kiểm thử

Áp dụng cho bản nhiều producer/consumer từ Tuần 3. Mọi lệnh chạy trong Ubuntu (máy thật hoặc WSL2), tại thư mục gốc của repository.

## 1. Chuẩn bị

```sh
sudo apt update
sudo apt install build-essential git python3
git clone https://github.com/ndhuy1127/os-producer-consumer.git
cd os-producer-consumer
```

Repository ép dòng LF (`.gitattributes`), nên clone từ Windows rồi mở trong WSL vẫn chạy được Makefile và script. Các test dùng tùy chọn `--wrap` của GNU ld, nên cần GCC/binutils của Linux; không chạy được bằng trình biên dịch Windows.

## 2. Build

| Lệnh | Kết quả |
| --- | --- |
| `make` | Tạo `bin/producer-consumer` (C11, `-Wall -Wextra -Wpedantic`, `-pthread`) |
| `make thread-demo` | Tạo `bin/thread-demo`, ví dụ tạo/join 3 luồng của Tuần 1 |
| `make clean` | Xóa `build/` và `bin/` |
| `make help` | Liệt kê các target |

## 3. Chạy chương trình

| Tham số | Mặc định | Giới hạn |
| --- | --- | --- |
| `--capacity N` | 8 | 1..65536 |
| `--producers P` | 1 | 1..64 |
| `--consumers C` | 1 | 1..64 |
| `--items-per-producer K` | 20 | 0..1000000 và P·K ≤ 1000000; K = 0 hợp lệ |
| `--producer-delay-ms Dp` | 0 | 0..60000 ms, ngủ sau mỗi lần đưa vào |
| `--consumer-delay-ms Dc` | 0 | 0..60000 ms, ngủ sau mỗi lần xử lý |
| `--quiet` | tắt | Không in từng thao tác, vẫn kiểm chứng và in tổng kết |
| `--help` | — | Chỉ dùng một mình |

Chỉ nhận dạng `--tham-số giá-trị` (không nhận `--tham-số=giá-trị`). Giá trị chỉ gồm chữ số 0–9; số âm, dấu `+`, khoảng trắng, số thực, hậu tố, tràn số, tham số lạ hoặc lặp đều bị từ chối trước khi tạo luồng.

Ví dụ:

```sh
./bin/producer-consumer                                    # 1 producer, 1 consumer, N=8, K=20
./bin/producer-consumer --capacity 3 --producers 3 --consumers 2 --items-per-producer 100
./bin/producer-consumer --capacity 2 --producers 3 --consumers 2 --items-per-producer 40 --consumer-delay-ms 20   # bộ đệm hay đầy
./bin/producer-consumer --capacity 2 --producers 2 --consumers 3 --items-per-producer 40 --producer-delay-ms 20   # bộ đệm hay rỗng
./bin/producer-consumer --capacity 1 --consumers 4 --items-per-producer 0               # chỉ có STOP, C > N
./bin/producer-consumer --capacity 16 --producers 4 --consumers 3 --items-per-producer 10000 --quiet
```

Mã thoát: **0** khi chạy xong và kiểm chứng đạt (hoặc `--help`); **1** khi lỗi tài nguyên/runtime, kiểm chứng sai hoặc lỗi ghi output; **2** khi tham số sai.

## 4. Đọc kết quả

Output thật của lệnh `./bin/producer-consumer --capacity 2 --producers 2 --consumers 2 --items-per-producer 2` (chạy ngày 20/10/2026, commit `3bda04b`; thứ tự thao tác thay đổi giữa các lần chạy):

```text
config: N=2 P=2 C=2 K=2 Dp_ms=0 Dc_ms=0
op_seq=1 op=ENQUEUE actor=P0 kind=DATA data_id=0 value=1 enqueue_seq=1 count_after=1 capacity=2 wait_reason=NONE
op_seq=2 op=ENQUEUE actor=P0 kind=DATA data_id=1 value=2 enqueue_seq=2 count_after=2 capacity=2 wait_reason=NONE
op_seq=3 op=DEQUEUE actor=C0 kind=DATA data_id=0 value=1 enqueue_seq=1 count_after=1 capacity=2 wait_reason=WAIT_ITEM
op_seq=4 op=DEQUEUE actor=C0 kind=DATA data_id=1 value=2 enqueue_seq=2 count_after=0 capacity=2 wait_reason=NONE
op_seq=5 op=ENQUEUE actor=P1 kind=DATA data_id=2 value=3 enqueue_seq=3 count_after=1 capacity=2 wait_reason=NONE
op_seq=6 op=ENQUEUE actor=P1 kind=DATA data_id=3 value=4 enqueue_seq=4 count_after=2 capacity=2 wait_reason=NONE
op_seq=7 op=DEQUEUE actor=C0 kind=DATA data_id=2 value=3 enqueue_seq=3 count_after=1 capacity=2 wait_reason=WAIT_ITEM
op_seq=8 op=DEQUEUE actor=C0 kind=DATA data_id=3 value=4 enqueue_seq=4 count_after=0 capacity=2 wait_reason=NONE
op_seq=9 op=ENQUEUE actor=MAIN kind=STOP data_id=- value=0 enqueue_seq=5 count_after=1 capacity=2 wait_reason=NONE
op_seq=10 op=ENQUEUE actor=MAIN kind=STOP data_id=- value=0 enqueue_seq=6 count_after=2 capacity=2 wait_reason=NONE
op_seq=11 op=DEQUEUE actor=C0 kind=STOP data_id=- value=0 enqueue_seq=5 count_after=1 capacity=2 wait_reason=WAIT_ITEM
op_seq=12 op=DEQUEUE actor=C1 kind=STOP data_id=- value=0 enqueue_seq=6 count_after=0 capacity=2 wait_reason=WAIT_ITEM
DATA: enqueued=4 dequeued=4 processed=4 missing=0 duplicate=0 out_of_range=0 value_errors=0 FIFO_errors=0 order_errors=0
buffer: min=0 max=2 final=0 count_errors=0; STOP: enqueued=2 dequeued=2
waits: WAIT_SLOT=0 WAIT_ITEM=4 records=12 data_pending_after_producer_join=0
producers(DATA): P0=2 P1=2
consumers(DATA/STOP): C0=4/1 C1=0/1
workers: created=2P/2C joined=2P/2C; verification=PASS run_status=0
```

| Trường | Ý nghĩa |
| --- | --- |
| `op_seq` | Thứ tự thao tác, chụp **trong vùng găng** nên đúng thứ tự thật |
| `actor` | `P<p>` producer thứ p, `C<c>` consumer thứ c, `MAIN` khi main gửi STOP |
| `data_id`, `value` | ID = p·K + j; `value = ID mod 1000 + 1`; STOP có `data_id=-` |
| `enqueue_seq` | Thứ tự phần tử được đưa vào; khi lấy ra phải tăng dần 1, 2, 3… (FIFO) |
| `count_after` | Số phần tử trong bộ đệm sau thao tác, luôn trong [0, N] |
| `wait_reason` | `WAIT_SLOT`/`WAIT_ITEM`: lúc thử, luồng chưa có quyền chỗ trống/phần tử nên phải chờ (`sem_trywait` trả `EAGAIN`) |

Dòng tổng kết: `missing`/`duplicate` đếm ID bị mất/lặp; `FIFO_errors` đếm lần lấy ra sai thứ tự; `order_errors` đếm ID không do đúng producer đưa vào hoặc sai thứ tự j; `data_pending_after_producer_join` là số phần tử còn trong bộ đệm khi main bắt đầu gửi STOP; `producers(DATA)` và `consumers(DATA/STOP)` cho biết mỗi luồng đã đưa vào/xử lý bao nhiêu (mỗi consumer phải có đúng `/1` STOP). `verification=PASS` chỉ xuất hiện khi mọi số lỗi bằng 0, count cuối bằng 0 và mọi luồng đã join.

Log được in **sau khi** mọi luồng kết thúc, không phải theo thời gian thực. Với nhiều phần tử nên dùng `--quiet`.

## 5. Kiểm thử

| Lệnh | Nội dung | Kết quả mong đợi | Thời gian tham khảo |
| --- | --- | --- | --- |
| `make test-buffer` | 12 ca bộ đệm tuần tự | `Sequential buffer tests: 12/12 PASS` | < 1 s |
| `make test-pc` | 16 cấu hình lõi (1P/1C và nhiều luồng) | `Core preset tests: 16/16 PASS` | ~5 s |
| `make test-errors` | 32 ca tiêm lỗi | `Error tests: 32/32 PASS` | < 1 s |
| `make test-cli` | Chương trình chính 15 cấu hình × thường/quiet, 114 tham số sai, biên, formatter, lỗi main | `CLI/log PASS: runtime=30 invalid=114 ...` | ~11 s |
| `make test` | Bốn nhóm trên | exit 0 | ~16 s |
| `make test-stress` | 7 cấu hình nhiều luồng × 20 lần, in bảng | `Stress PASS: 7 configurations, 140 runs` | ~18 s |
| `python3 scripts/check_stress.py --runs 100` | Như trên, số lần tùy chọn | exit 0 | tăng theo số lần |
| `python3 scripts/validate_week3.py` | Toàn bộ, thêm ASan/UBSan và TSan; **ghi đè** `results/week3/` | `Week 3 validation completed, exit=0` | vài phút |

Thời gian đo ngày 20/10/2026 trên máy của Huy (đã build sẵn); máy khác có thể khác. Các ca lỗi cố ý (tham số sai, lỗi tiêm vào) có tiến trình con thoát mã 1 hoặc 2 đúng dự kiến; script chỉ thất bại khi mã thoát khác dự kiến.

Chạy ThreadSanitizer thủ công:

```sh
gcc -Iinclude -std=c11 -pthread -g -O1 -fsanitize=thread src/*.c -o /tmp/pc-tsan
setarch "$(uname -m)" -R /tmp/pc-tsan --capacity 3 --producers 3 --consumers 2 --items-per-producer 100 --quiet
```

## 6. Sự cố thường gặp

| Hiện tượng | Cách xử lý |
| --- | --- |
| `make: warning: Clock skew detected` khi repo nằm trên `/mnt/c` hoặc `/mnt/d` | Đồng hồ ổ Windows lệch với WSL; `make clean` rồi build lại. `validate_week3.py` tự chuẩn hóa mtime, không đổi nội dung |
| TSan dừng với `FATAL: ThreadSanitizer: unexpected memory mapping` | Kernel bật ASLR entropy cao; chạy qua `setarch "$(uname -m)" -R` |
| `CLI: unknown option or positional argument '--capacity=5'` | Viết `--capacity 5` |
| `CLI: total DATA P*K ... must be <= 1000000` | Giảm P hoặc K |
| Lỗi liên kết `--wrap` khi build test | Cần GNU ld trên Linux/WSL |

`scripts/validate_week2.py` và `scripts/validate_week2_huy.py` là script ghi minh chứng lịch sử của Tuần 2; **không chạy lại**, vì chúng ghi đè `results/week2` và kỳ vọng hành vi CLI cũ (P = C = 1).
