# Báo cáo nháp — Người sản xuất – Người tiêu thụ với bộ đệm giới hạn

Bài tập lớn môn Hệ điều hành. Giảng viên: Nguyễn Quang Minh. Nhóm: Lê Hải Châu (20233283), Nguyễn Đức Huy (20233448).

> **Trạng thái:** bản nháp cuối Tuần 3. Phần lý thuyết/thuật toán (mục 2, 4, 5) thuộc phân công của Châu, phần kiểm thử/kết quả/hướng dẫn chạy (mục 7, 8) thuộc phân công của Huy. Toàn bộ bản nháp do Claude Code soạn theo yêu cầu của Huy, dựa trên mã nguồn và minh chứng thực chạy trong repository; **Châu chưa đọc và xác nhận**. Bản hoàn chỉnh, slide và phần trình bày thuộc Tuần 4.

## 1. Bài toán

Có P luồng sản xuất (producer) và C luồng tiêu thụ (consumer) dùng chung một hàng đợi có N chỗ. Mỗi producer tạo K phần tử dữ liệu rồi đưa vào hàng đợi; consumer lấy phần tử ra và xử lý. Chương trình phải bảo đảm:

1. không bao giờ ghi khi hàng đợi đã đủ N phần tử và không đọc khi hàng đợi rỗng;
2. mỗi phần tử được tiêu thụ **đúng một lần** (không mất, không lặp);
3. phần tử được lấy ra theo đúng thứ tự đưa vào (FIFO);
4. mọi luồng tự kết thúc sau khi hết dữ liệu, chương trình không chạy vô hạn.

Chương trình dòng lệnh `producer-consumer` viết bằng C11, dùng POSIX Threads và POSIX semaphore, chạy trên Ubuntu (WSL2). Tham số: N = 1..65536, P và C = 1..64, K = 0..1000000 với tổng P·K ≤ 1000000, độ trễ minh họa của producer/consumer 0..60000 ms.

## 2. Cơ sở lý thuyết

Phần này tóm tắt; giải thích chi tiết và ví dụ ở [lý thuyết Tuần 1](../theory-week1-chau.md).

**Luồng và vùng găng.** Các luồng trong cùng tiến trình dùng chung bộ nhớ heap, nên producer và consumer trao đổi phần tử trực tiếp qua bộ đệm. Đoạn mã đọc/sửa ô nhớ của bộ đệm cùng các chỉ số `head`, `tail`, `count` là **vùng găng**: nếu hai luồng cùng thực hiện, có thể xảy ra tranh chấp dữ liệu (data race), ví dụ hai producer cùng đọc `tail` rồi ghi đè lên một ô, hoặc mất một lần tăng `count`.

**Semaphore.** Semaphore POSIX là một số nguyên không bao giờ âm với hai thao tác: `sem_post` tăng giá trị lên 1, `sem_wait` giảm giá trị đi 1 và **chờ** nếu giá trị đang bằng 0 cho tới khi giảm được (sem_overview(7), sem_wait(3)). `sem_trywait` giống `sem_wait` nhưng trả lỗi `EAGAIN` thay vì chờ. `sem_wait` có thể trả `EINTR` khi bị một trình xử lý tín hiệu ngắt; khi đó chương trình thử lại. Khi `sem_post` làm giá trị lớn hơn 0, **một** luồng đang chờ được đánh thức; tài liệu không quy định luồng nào (sem_post(3)), nên không có bảo đảm công bằng giữa các luồng chờ.

**Bế tắc.** Bế tắc xảy ra khi các luồng chờ lẫn nhau theo vòng. Ví dụ kinh điển: producer giữ khóa bộ đệm rồi chờ chỗ trống, trong khi consumer cần chính khóa đó để lấy phần tử và tạo chỗ trống. Quy tắc tránh: **luôn chờ quyền chỗ trống/phần tử trước, chỉ lấy khóa bộ đệm sau**, và không giữ khóa khi ngủ hoặc xử lý dữ liệu.

**`sleep` không phải đồng bộ.** Độ trễ chỉ dùng để tạo tình huống producer nhanh/consumer chậm (hoặc ngược lại); nó không bảo vệ dữ liệu và không bảo đảm thứ tự.

## 3. Thiết kế

### 3.1 Dữ liệu

| Thành phần | Mô tả |
| --- | --- |
| `struct item` | `kind` (DATA hoặc STOP), `id` (uint64), `value` |
| Bộ đệm vòng | Mảng N ô; `head` là ô lấy kế tiếp, `tail` là ô thêm kế tiếp, `count` là số phần tử; chỉ số tăng theo modulo N |
| Context dùng chung | Bộ đệm, ba semaphore, mảng `enqueue_seq` N ô, mảng tần suất ID đã thêm/đã lấy, mảng bản ghi thao tác |
| Bản ghi thao tác | `op_seq`, ENQUEUE/DEQUEUE, luồng thực hiện (`P<p>`, `C<c>`, `MAIN`), phần tử, `enqueue_seq`, `count` sau thao tác, lý do chờ |

API bộ đệm (`buffer_init/push/pop/destroy`) là tuần tự, không tự khóa; toàn bộ đồng bộ nằm ở lớp ngoài (`pc.c`).

### 3.2 Ba semaphore

| Semaphore | Giá trị đầu | Ý nghĩa |
| --- | --- | --- |
| `empty` | N | Số quyền chiếm một chỗ trống |
| `full` | 0 | Số quyền lấy một phần tử đã có |
| `guard` | 1 | Khóa loại trừ lẫn nhau cho trạng thái bộ đệm và dữ liệu kiểm chứng |

Tất cả là semaphore không tên, `pshared = 0` vì chỉ dùng giữa các luồng của một tiến trình. `empty`/`full` chỉ cho biết **có được** thêm/lấy hay không; chúng không ngăn hai producer cùng sửa `tail`, nên vẫn cần `guard`.

### 3.3 Định danh dữ liệu

Producer p (0 ≤ p < P) tạo phần tử thứ j (0 ≤ j < K) với ID = **p·K + j** và `value = ID mod 1000 + 1`. Hai cặp (p, j) khác nhau luôn cho ID khác nhau, và từ ID suy ngược được producer sở hữu (ID div K) cùng thứ tự j (ID mod K). Nhờ vậy có thể kiểm tra mất/lặp bằng tần suất ID, và kiểm tra thứ tự riêng của từng producer.

### 3.4 Kết thúc bằng tín hiệu STOP

Main tạo C consumer trước, sau đó P producer. Khi **mọi producer đã join**, không còn ai đưa DATA vào, main đưa **C phần tử STOP** vào hàng đợi bằng đúng thao tác enqueue thông thường. Mỗi consumer dừng ngay khi lấy được STOP đầu tiên. STOP vẫn chiếm một chỗ trong bộ đệm nhưng không được tính là dữ liệu. Main join tất cả consumer, kiểm chứng kết quả rồi mới hủy semaphore và giải phóng bộ nhớ.

## 4. Thuật toán

```text
enqueue(item, actor):                    // producer và main dùng chung
    wait(empty)                          // chờ quyền chỗ trống, chưa giữ guard
    wait(guard)
    buffer_push(item); ghi enqueue_seq và bản ghi thao tác
    post(guard)
    post(full)

dequeue(c):
    wait(full)                           // chờ quyền phần tử, chưa giữ guard
    wait(guard)
    item = buffer_pop(); ghi bản ghi thao tác
    post(guard)
    post(empty)                          // STOP cũng trả lại chỗ
    return item

producer(p):
    for j in 0..K-1:
        enqueue(DATA(id = p*K + j), P<p>)
        ngủ Dp ms (ngoài guard)

consumer(c):
    loop:
        item = dequeue(c)
        if item là STOP: return          // mỗi consumer nhận đúng một STOP
        xử lý item, ngủ Dc ms (ngoài guard)

main:
    kiểm tra tham số và tràn số; cấp phát; init empty=N, full=0, guard=1
    tạo C consumer, rồi P producer
    join mọi producer
    lặp C lần: enqueue(STOP, MAIN)
    join mọi consumer
    kiểm chứng; hủy semaphore; giải phóng bộ nhớ
```

`wait(empty/full)` được cài bằng `sem_trywait` trước: thành công thì đã có quyền; nếu `EAGAIN` thì ghi nhận "đã phải chờ" (WAIT_SLOT/WAIT_ITEM) rồi gọi `sem_wait`. Mọi `sem_wait` thử lại khi gặp `EINTR`.

**Xử lý lỗi.** Lỗi cấp phát hoặc khởi tạo semaphore: dọn phần đã tạo, chưa tạo luồng nào. Tạo consumer lỗi giữa chừng: không tạo producer, gửi STOP cho các consumer đã chạy và join chúng. Tạo producer lỗi giữa chừng: join các producer đã chạy, gửi C STOP, join mọi consumer; phiên chạy trả mã lỗi 1. Lỗi semaphore hoặc join trong khi chạy không thể phục hồi an toàn, nên chương trình in lỗi và kết thúc toàn tiến trình với mã 1 thay vì treo hoặc giải phóng bộ nhớ khi luồng khác còn dùng.

## 5. Lập luận tính đúng

**(a) Không ghi tràn, không đọc rỗng.** Producer chỉ `push` sau khi lấy được một quyền `empty`. `empty` bắt đầu bằng N và chỉ được trả lại sau một lần `pop`, nên tại mọi thời điểm số lần push trừ số lần pop không vượt quá N. Tương tự, consumer chỉ `pop` sau khi lấy một quyền `full`, và `full` chỉ tăng sau một lần push hoàn tất, nên số lần pop không vượt quá số lần push. Vì push/pop đều nằm trong `guard`, chúng không xen kẽ nhau: 0 ≤ count ≤ N.

**(b) Không mất, không lặp, FIFO.** `guard` làm cho các thao tác trên bộ đệm diễn ra lần lượt; bộ đệm vòng là hàng đợi, nên thứ tự lấy ra đúng bằng thứ tự đưa vào (theo thứ tự thực sự trong `guard`, không phải thứ tự tạo ID hay thứ tự in). Mỗi push ghi vào một ô chưa bị chiếm và mỗi pop đọc đúng một ô rồi giải phóng nó, nên mỗi phần tử được lấy đúng một lần. Một producer đưa các phần tử của mình vào theo j tăng dần, nên chúng cũng được lấy ra theo j tăng dần.

**(c) Không bế tắc.** Mỗi luồng giữ nhiều nhất một quyền `empty` hoặc `full` khi chờ `guard`, và không bao giờ chờ `empty`/`full` trong lúc giữ `guard`; `guard` chỉ được giữ trong một đoạn push/pop hữu hạn. Do đó không có vòng chờ. Nếu mọi producer đang chờ `empty` thì mọi chỗ đã bị chiếm bởi phần tử đã push xong, nên `full` > 0 và consumer (chưa nhận STOP) lấy được phần tử. Nếu consumer chờ `full` sau khi producer hết việc thì main sẽ đưa STOP vào.

**(d) Kết thúc hữu hạn.** Mỗi producer chỉ lặp K lần nên kết thúc. Sau khi join hết producer, mọi DATA đã nằm trong hàng đợi trước mọi STOP, nên theo FIFO consumer lấy hết DATA trước khi gặp STOP đầu tiên. Có đúng C STOP cho C consumer, mỗi consumer dừng ngay ở STOP đầu tiên nên lấy nhiều nhất một STOP; khi còn consumer sống thì hàng đợi vẫn được rút nên main luôn đưa được STOP tiếp theo vào, kể cả khi C > N. Vì vậy mỗi consumer nhận đúng một STOP và main join được tất cả.

**(e) Giới hạn.** Không có bảo đảm công bằng: `sem_post` không quy định luồng nào được đánh thức, nên số DATA mỗi consumer xử lý thay đổi giữa các lần chạy (đo được ở mục 7). Điều này không ảnh hưởng tính đúng vì tổng dữ liệu là hữu hạn. Lập luận trên giả định bộ lập lịch cho mọi luồng sẵn sàng có cơ hội chạy.

## 6. Cài đặt

| Mô-đun | Vai trò |
| --- | --- |
| [buffer.c](../../src/buffer.c) | Bộ đệm vòng tuần tự, mã lỗi trả trực tiếp |
| [pc.c](../../src/pc.c) | Luồng producer/consumer, ba semaphore, STOP, bản ghi thao tác và kiểm chứng sau join |
| [cli.c](../../src/cli.c) | Đọc và kiểm tra tham số dòng lệnh trước khi tạo luồng |
| [runtime_log.c](../../src/runtime_log.c) | In bản ghi và tổng kết sau khi mọi luồng đã join |
| [main.c](../../src/main.c) | parse → chạy → in log → giải phóng; mã thoát 0/1/2 |

Bản ghi thao tác được chụp **trong `guard`** tại đúng thời điểm push/pop nên phản ánh thứ tự thật; log được in sau khi chạy xong để không đưa I/O vào vùng găng. Sau khi join, `pc.c` tự kiểm chứng: tần suất mỗi ID, FIFO giữa hai dãy enqueue/dequeue, chủ sở hữu và thứ tự j của từng producer, count trong [0, N] và bằng 0 lúc cuối, mỗi consumer nhận đúng một STOP, số luồng tạo/join. Chương trình chỉ in `verification=PASS` và thoát 0 khi mọi điều kiện đều đúng.

## 7. Kiểm thử và kết quả

### 7.1 Phương pháp

Kiểm thử chia thành nhiều lớp, lớp sau dùng chương trình thật thay vì giả lập:

| Lớp | Lệnh | Nội dung |
| --- | --- | --- |
| Bộ đệm tuần tự | `make test-buffer` | 12 ca: FIFO, đầy/rỗng, quay vòng chỉ số, N = 1, STOP, tham số sai, lỗi cấp phát |
| Lõi đồng bộ | `make test-pc` | 16 cấu hình gọi trực tiếp `pc_run`; đối chiếu bản ghi với một hàng đợi tuyến tính độc lập |
| Tiêm lỗi | `make test-errors` | 32 ca: `EINTR`, tạo luồng lỗi (kể cả giữa chừng khi nhiều luồng), lỗi cấp phát/semaphore, tham số sai, lỗi nghiêm trọng phải thoát mã 1 trong 5 giây |
| Chương trình chính | `make test-cli` | 15 cấu hình × chế độ thường/quiet; script dựng lại hàng đợi từ log; 114 tham số sai; 6 ca biên |
| Chạy lặp | `make test-stress` | 7 cấu hình nhiều luồng × 20 lần, mỗi lần kiểm tra đầy đủ |
| Sanitizer | `scripts/validate_week3.py` | AddressSanitizer + UndefinedBehaviorSanitizer (lỗi bộ nhớ, rò rỉ, hành vi không xác định); ThreadSanitizer (data race) |

Với mỗi lần chạy, oracle kiểm tra: mỗi ID xuất hiện đúng một lần ở cả hai phía; dãy lấy ra trùng dãy đưa vào; ID p·K+j do đúng producer p đưa vào theo thứ tự j; count sau từng thao tác bằng số thêm trừ số lấy, nằm trong [0, N] và bằng 0 lúc cuối; đúng C STOP, mỗi consumer nhận một; P + C luồng được tạo và join. Chỉ so sánh tổng số là chưa đủ, vì mất một phần tử và lặp một phần tử khác vẫn cho cùng tổng.

### 7.2 Kết quả

Chạy ngày 20/10/2026 trên WSL2 Ubuntu 24.04, GCC 13.3.0, mã nguồn commit `3bda04b`. Mọi lần chạy đều đạt: 16/16 preset lõi, 32/32 ca lỗi, 30/30 lần chạy chương trình chính, 140/140 lần chạy lặp; ASan/UBSan và TSan không có cảnh báo trên 9 cấu hình nhiều luồng và 16 preset lõi. Một số cấu hình nhiều luồng tiêu biểu (cấu hình ghi theo N/P/C/K/Dp/Dc, Dp/Dc tính bằng ms):

| Ca | Cấu hình | Mục đích | Quan sát (lần chạy thường) |
| --- | --- | --- | --- |
| T06a | 3/3/2/100/0/0 | Nhiều producer và consumer | 300 DATA đúng một lần; count tối đa 3; consumer xử lý 154 và 146 |
| T06b | 2/2/5/100/0/0 | Số consumer lớn hơn N | 200 DATA; 5 STOP đưa dần qua bộ đệm 2 chỗ; mỗi consumer 35–44 DATA |
| T04-multi | 2/3/2/40/0/20 | Producer nhanh, bộ đệm đầy | 74 snapshot count = N; 72 lần thiếu chỗ (WAIT_SLOT), trong đó 71 lần của producer, 1 lần của main khi gửi STOP |
| T05-multi | 2/2/3/40/20/0 | Consumer nhanh, bộ đệm rỗng | 63 snapshot count = 0; 66 lần consumer thiếu phần tử (WAIT_ITEM) |
| T09a | 1/1/4/0/0/0 | Kết thúc khi consumer đang chờ | Không có DATA; cả 4 consumer đều đã phải chờ (WAIT_ITEM = 4) rồi được STOP đánh thức |
| queued-stop-multi | 8/2/3/40/0/20 | Còn dữ liệu khi gửi STOP | 8 DATA còn trong bộ đệm lúc main bắt đầu gửi STOP; tất cả được lấy trước STOP đầu tiên |

Chạy lặp 20 lần mỗi cấu hình:

| Cấu hình | N/P/C/K/Dp/Dc | Đạt | DATA mỗi lần | Thời gian (s) | DATA ít/nhiều nhất của một consumer |
| --- | --- | --- | --- | --- | --- |
| T06a | 3/3/2/100/0/0 | 20/20 | 300 | 0,004–0,009 | 123–177 |
| T06b | 2/2/5/100/0/0 | 20/20 | 200 | 0,003–0,02 | 27–53 |
| T07 | 16/4/3/10000/0/0 | 20/20 | 40000 | 0,118–0,205 | 11779–14437 |
| T09a | 1/1/4/0/0/0 | 20/20 | 0 | 0,001–0,005 | 0 |
| T09b | 2/2/3/30/0/10 | 20/20 | 60 | 0,208–0,233 | 20 |
| 8P8C, N = 1 | 1/8/8/500/0/0 | 20/20 | 4000 | 0,038–0,08 | 409–774 |
| 64P64C | 64/64/64/100/0/0 | 20/20 | 6400 | 0,021–0,045 | 18–216 |

Bảng đầy đủ: [results-table.md](../../results/week3/results-table.md); log và môi trường: [minh chứng Tuần 3](../../results/week3/README.md).

### 7.3 Nhận xét

- Tình huống đầy và rỗng thực sự xảy ra khi có nhiều luồng (snapshot count = N hoặc 0 trong `guard`), không chỉ được suy ra từ độ trễ, và chương trình vẫn tiếp tục đúng sau đó.
- STOP hoạt động cả khi C > N (T06b, T09a) và khi consumer đang chờ (T09a: WAIT_ITEM = 4 ở cả 20/20 lần lặp).
- Phân chia công việc giữa các consumer không đều và thay đổi theo lần chạy (T07: một consumer xử lý từ 11779 đến 14437 trong 40000 phần tử; 64P64C: từ 18 đến 216 trong 6400), đúng với việc `sem_post` không quy định luồng nào được đánh thức. Riêng T09b (consumer ngủ 10 ms sau mỗi phần tử), ba consumer nhận đều 20 phần tử ở cả 20 lần; nhiều khả năng vì consumer vừa xử lý xong còn đang ngủ nên phần tử kế tiếp rơi vào consumer khác, nhưng điều này chưa được kiểm chứng riêng và không phải bảo đảm của thuật toán.
- 40000 phần tử với 7 luồng xử lý trong khoảng 0,12–0,21 giây (không có độ trễ minh họa).

## 8. Hướng dẫn chạy

Xem [hướng dẫn chạy và kiểm thử](../run-guide.md). Tóm tắt trong Ubuntu/WSL từ thư mục gốc repository:

```sh
make
./bin/producer-consumer --capacity 3 --producers 3 --consumers 2 --items-per-producer 100
make test
make test-stress
```

## 9. Hạn chế và việc còn lại

- Kiểm thử hữu hạn: các lần chạy và sanitizer chỉ quan sát những lịch chạy đã thực sự xảy ra, không chứng minh mọi lịch chạy. Lập luận ở mục 5 bổ sung cho phần này.
- Lỗi semaphore/join khi đang chạy dẫn tới kết thúc toàn tiến trình, không phải cơ chế dừng có phối hợp.
- Bản ghi thao tác chiếm bộ nhớ O(P·K + C); log in sau khi chạy, không phải log thời gian thực.
- Chưa đối chiếu giáo trình *Modern Operating Systems* (Tanenbaum, Bos); nguồn đã đối chiếu chỉ là các trang man Linux bên dưới.
- Tuần 4: Châu đọc và xác nhận/sửa mục 2–5; hai thành viên đọc chéo code; hoàn thiện báo cáo, slide và demo.

## Tài liệu tham khảo

Các trang Linux man-pages tại man7.org:

- Châu đối chiếu ngày 08/10/2026: [pthreads(7)](https://man7.org/linux/man-pages/man7/pthreads.7.html), [pthread_create(3)](https://man7.org/linux/man-pages/man3/pthread_create.3.html), [pthread_join(3)](https://man7.org/linux/man-pages/man3/pthread_join.3.html), [sem_init(3)](https://man7.org/linux/man-pages/man3/sem_init.3.html), [sem_destroy(3)](https://man7.org/linux/man-pages/man3/sem_destroy.3.html).
- Đối chiếu lại ngày 20/10/2026 khi soạn bản nháp: [sem_overview(7)](https://man7.org/linux/man-pages/man7/sem_overview.7.html) mục DESCRIPTION (định nghĩa semaphore, semaphore không tên dùng giữa các luồng); [sem_wait(3)](https://man7.org/linux/man-pages/man3/sem_wait.3.html) mục DESCRIPTION và ERRORS (chờ khi bằng 0, `sem_trywait` trả `EAGAIN`, `EINTR`); [sem_post(3)](https://man7.org/linux/man-pages/man3/sem_post.3.html) mục DESCRIPTION và ERRORS (đánh thức một luồng đang chờ, không quy định luồng nào; `EOVERFLOW`).
