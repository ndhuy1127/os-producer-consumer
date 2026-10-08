# Lý thuyết Tuần 1 — Lê Hải Châu

Người phụ trách: **Lê Hải Châu — 20233283 — `dev/chau`**. Đề tài dùng C, POSIX Threads và POSIX semaphore trên Linux. Tài liệu này chuẩn bị nguyên lý cho Tuần 2; chương trình Producer–Consumer chưa được triển khai. Giả mã ở [design.md](design.md), ví dụ tạo/join luồng ở [thread_lifecycle.c](../examples/thread_lifecycle.c).

## 1. Tiến trình và luồng

Tiến trình là một chương trình đang chạy cùng không gian địa chỉ và tài nguyên của nó. Các tiến trình thông thường có không gian địa chỉ riêng; muốn trao đổi dữ liệu cần cơ chế như pipe hoặc bộ nhớ chia sẻ.

Luồng là một luồng thực thi bên trong tiến trình. Nhiều luồng trong cùng tiến trình dùng chung mã chương trình, biến toàn cục, heap và các file descriptor đang mở. Mỗi luồng có ngăn xếp, trạng thái thực thi và ID riêng. Vì cùng truy cập được bộ đệm trên heap, producer và consumer có thể trao đổi item trực tiếp, nhưng phải đồng bộ khi đọc/sửa dữ liệu dùng chung. Nội dung này đã đối chiếu với [pthreads(7)](https://man7.org/linux/man-pages/man7/pthreads.7.html), phần DESCRIPTION.

Biến local thường nằm trên stack của luồng sở hữu, nhưng truyền địa chỉ biến đó cho luồng khác vẫn làm nó có thể được truy cập từ nhiều luồng. Do đó, “biến local” không tự bảo đảm an toàn: phải xét ai đọc/ghi và biến còn sống bao lâu.

## 2. Producer, consumer và hàng đợi có N chỗ

Hình dung một quầy có **N = 3** chỗ đặt hàng. Producer tạo hàng và đưa vào quầy; consumer lấy hàng ra xử lý. Quầy là **bộ đệm giới hạn**: khi đầy, producer phải chờ; khi trống, consumer phải chờ.

Ví dụ tuần tự:

| Thao tác | Hàng đợi từ đầu đến cuối | count |
| --- | --- | --- |
| Ban đầu | `[]` | 0 |
| Đưa A vào | `[A]` | 1 |
| Đưa B vào | `[A, B]` | 2 |
| Đưa C vào | `[A, B, C]` — đầy | 3 |
| Lấy một item | `[B, C]` — lấy A | 2 |
| Đưa D vào | `[B, C, D]` | 3 |

FIFO (First In, First Out) nghĩa là item đã được đưa vào trước thì được lấy ra trước. Với nhiều producer, “đưa vào trước” là thứ tự enqueue thực sự trong vùng găng, không phải thứ tự tạo ID hoặc bắt đầu tính dữ liệu. Với nhiều consumer, thứ tự hoàn tất xử lý có thể khác thứ tự lấy ra.

## 3. Vùng găng và tranh chấp dữ liệu

Vùng găng là đoạn thao tác trên trạng thái dùng chung mà các luồng không được thực hiện đồng thời. Trong bài toán này, nó gồm đọc/sửa ô của bộ đệm, chỉ số `head`, `tail`, `count` và thông tin kiểm chứng gắn với thao tác enqueue/dequeue.

Tranh chấp dữ liệu (data race) xuất hiện khi các luồng truy cập cùng vị trí bộ nhớ, có ít nhất một thao tác ghi và thiếu đồng bộ phù hợp. Trong C, đó là hành vi không xác định. Bảng sau chỉ minh họa một cách lỗi có thể biểu hiện, không khẳng định chương trình có data race luôn cho kết quả như bảng.

Giả sử `tail = 0`, `count = 0`, có hai producer P1 và P2:

| Bước | P1 | P2 | Hậu quả minh họa |
| --- | --- | --- | --- |
| 1 | Đọc `tail = 0` | | |
| 2 | | Đọc `tail = 0` | Cả hai chọn cùng một ô |
| 3 | Ghi A vào ô 0 | | |
| 4 | | Ghi B vào ô 0 | A bị ghi đè |
| 5 | Đọc `count = 0` để tăng | Đọc `count = 0` để tăng | |
| 6 | Ghi `count = 1` | Ghi `count = 1` | Mất một lần tăng dù có hai thao tác |

`count++` gồm đọc, tính và ghi; nó không tự trở thành thao tác nguyên tử. Chỉ bảo vệ ô dữ liệu mà bỏ sót chỉ số hoặc `count` vẫn sai. Toàn bộ cập nhật của một lần push/pop cần nằm trong cùng vùng bảo vệ.

## 4. Vai trò của empty, full và guard

Semaphore là biến đếm được thao tác bằng cơ chế đồng bộ: `wait` lấy một đơn vị nếu còn, nếu chưa có thì chờ; `post` trả/thêm một đơn vị. Đối chiếu API ở [sem_wait(3)](https://man7.org/linux/man-pages/man3/sem_wait.3.html) và [sem_post(3)](https://man7.org/linux/man-pages/man3/sem_post.3.html).

Châu chọn ba semaphore cho thiết kế Tuần 1:

| Semaphore | Giá trị đầu | Ý nghĩa |
| --- | --- | --- |
| `empty` | N | Quyền lấy một chỗ trống trước enqueue |
| `full` | 0 | Quyền lấy một item có sẵn trước dequeue |
| `guard` | 1 | Chỉ một luồng được thao tác trạng thái bộ đệm tại một thời điểm |

`empty` và `full` kiểm soát **có được thêm/lấy hay chưa**, nhưng không bảo vệ các ô/chỉ số. Chẳng hạn `empty = 3`, cả P1 và P2 đều có thể `wait(empty)` thành công và cùng sửa `tail`. Vì vậy vẫn cần `guard` cho tính loại trừ lẫn nhau. `guard` dùng như semaphore nhị phân; luồng lấy nó phải trả đúng một lần sau thao tác.

Thứ tự đúng dự kiến:

```text
Producer: tạo item -> wait(empty) -> wait(guard)
          -> push/cập nhật -> post(guard) -> post(full)
Consumer: wait(full) -> wait(guard)
          -> pop/cập nhật -> post(guard) -> post(empty) -> xử lý item
```

Khi thao tác đã hoàn tất và không có quyền chỗ/item đang được giữ giữa chừng, `empty = N - count`, `full = count`. Khi luồng đã wait nhưng chưa push/pop hoặc chưa post, các số đếm semaphore còn phản ánh quyền đã đặt trước; không giả định hai đẳng thức đúng ở mọi thời điểm.

## 5. Bế tắc và ví dụ lấy guard sai thứ tự

Bế tắc (deadlock) là tình huống các luồng chờ điều kiện mà chính các luồng đang chờ phải tạo ra, nên không bên nào tiến tiếp.

Giả sử N = 3, bộ đệm đầy: `count = 3`, `empty = 0`, `full = 3`, `guard = 1`. Nếu producer lấy `guard` trước `empty`:

| Bước | Producer P | Consumer C |
| --- | --- | --- |
| 1 | `wait(guard)` thành công, giữ guard | |
| 2 | `wait(empty)` bị chặn vì hết chỗ; vẫn giữ guard | |
| 3 | | `wait(full)` thành công |
| 4 | | `wait(guard)` bị chặn vì P đang giữ guard |
| 5 | P cần C lấy item rồi `post(empty)` | C cần P trả guard mới lấy được item |

Cả hai không thể tiến tiếp. Quy tắc tránh lỗi này: **chờ empty/full trước khi lấy guard**. Cũng không giữ guard khi ngủ, xử lý dữ liệu hoặc `pthread_join`.

## 6. Vì sao sleep không đồng bộ

`sleep` chỉ tạo độ trễ minh họa producer/consumer nhanh hoặc chậm. Nó không bảo đảm luồng khác đã xong, không khóa dữ liệu và không đặt ra thứ tự truy cập bộ nhớ. Sau khi thức dậy, hai producer vẫn có thể cùng sửa một ô. Bộ lập lịch và tải máy có thể thay đổi giữa các lần chạy; tăng thời gian ngủ không sửa được data race hay chứng minh hết deadlock.

Đồng bộ dự kiến dùng semaphore; đợi luồng kết thúc dùng join. Nếu bổ sung độ trễ ở Tuần 2, đặt nó ngoài guard. Ví dụ Tuần 1 không cần sleep vì mỗi worker thực hiện một phép tính hữu hạn độc lập.

## 7. Tạo và chờ luồng trong C

Các chữ ký dưới đây dùng cách viết C thông thường, lược chú thích kiểu mở rộng trong man-pages:

```c
#include <pthread.h>
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **retval);

#include <semaphore.h>
int sem_init(sem_t *sem, int pshared, unsigned int value);
int sem_wait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_destroy(sem_t *sem);
```

`pthread_create` tạo luồng chạy hàm worker và truyền `arg` cho hàm đó. Dùng `attr = NULL` để chọn thuộc tính mặc định, tạo luồng joinable. Sau create, luồng nào chạy trước không được bảo đảm. Cần một struct đối số riêng cho từng worker; tránh truyền `&i` của biến vòng lặp bị main sửa liên tục. API đã đối chiếu với [pthread_create(3)](https://man7.org/linux/man-pages/man3/pthread_create.3.html), phần SYNOPSIS, DESCRIPTION và RETURN VALUE.

`pthread_join` chờ một luồng joinable kết thúc. Sau join thành công, main mới đọc kết quả do worker ghi và có thể thu hồi vùng nhớ liên quan. Trong ví dụ, main giữ mảng đối số/kết quả đến hết join, worker ghi vào ô riêng và trả `NULL`; không trả địa chỉ biến local trên stack worker. API đã đối chiếu với [pthread_join(3)](https://man7.org/linux/man-pages/man3/pthread_join.3.html), phần DESCRIPTION, RETURN VALUE và NOTES.

Hai hàm pthread trên trả **mã lỗi trực tiếp**: 0 khi thành công, khác 0 khi lỗi; báo bằng `strerror(error)`, không dùng `perror` để giải thích mã trả về này. Semaphore trả 0 hoặc -1 kèm `errno`. `sem_wait` bị ngắt bởi tín hiệu có thể trả lỗi `EINTR`, khi đó thiết kế sẽ thử lại; lỗi khác cần xử lý và dọn tài nguyên. Dùng `sem_init(..., 0, ...)` vì các luồng cùng tiến trình, theo [sem_init(3)](https://man7.org/linux/man-pages/man3/sem_init.3.html). Chỉ hủy semaphore đã khởi tạo và không còn luồng sử dụng/chờ nó, theo [sem_destroy(3)](https://man7.org/linux/man-pages/man3/sem_destroy.3.html).

## 8. Ví dụ Tuần 1 chứng minh gì?

[Ví dụ](../examples/thread_lifecycle.c) tạo 3 worker tính tổng từ 1 đến 10, 20 và 30, kỳ vọng lần lượt 55, 210 và 465. Mỗi worker có đối số và ô kết quả riêng. Main join các luồng đã tạo, kiểm tra kết quả bằng công thức `n*(n+1)/2`, chỉ báo hoàn tất khi đủ 3 worker đúng. Nếu create lỗi giữa chừng, vẫn join các luồng đã tạo và trả lỗi. Nếu join lỗi, không đọc ô kết quả đó; tiếp tục thử join các luồng còn lại và thoát tiến trình với mã lỗi, không để vùng đối số hết thời gian sống trong khi còn worker chưa xác nhận dừng.

Log và giới hạn kiểm tra ở [minh chứng](../results/week1/chau-validation.md). Ví dụ chứng minh vòng đời tạo/chờ luồng, cách truyền đối số và đọc kết quả sau join; **chưa chứng minh bộ đệm, FIFO, semaphore hoặc kết thúc Producer–Consumer hoạt động**. Các nội dung đó là việc triển khai và kiểm thử ở Tuần 2.

## 9. Nguồn đã đối chiếu và nguồn đọc thêm

Đã truy cập các trang Linux man-pages tại man7.org ngày **08/10/2026**: `pthreads(7)`, `pthread_create(3)`, `pthread_join(3)`, `sem_init(3)`, `sem_wait(3)`, `sem_post(3)` và `sem_destroy(3)`. Các liên kết ở trên chỉ rõ nội dung được đối chiếu; chúng là nguồn cho chữ ký hàm và quy ước lỗi C.

Nguồn đọc thêm được đề bài gợi ý: *Modern Operating Systems*, ấn bản 4, Andrew S. Tanenbaum và Herbert Bos, các mục 2.2.3 POSIX Threads, 2.3.1 Race Conditions, 2.3.2 Critical Regions và 2.3.5 Semaphores. **Chưa truy cập bản giáo trình để kiểm chứng các mục/trang này**, nên không coi chúng là nguồn đã đọc. Không đưa PDF giáo trình vào repository.
