# Thiết kế Producer–Consumer — Tuần 1 đến Tuần 3

Tuần 1 đã ghép thiết kế Châu với API thực tế của Huy, chốt đặc tả và kiểm tra bản chung; PR #1/#2 đã merge ở nền `9cf345c`. Lý thuyết ở [theory-week1-chau.md](theory-week1-chau.md), minh chứng lịch sử ở [group-validation.md](../results/week1/group-validation.md). Codex thực hiện theo yêu cầu Châu, không thay sự phê duyệt của Huy hoặc đọc chéo trực tiếp.

**Tuần 2 đã ghép và kiểm chứng lõi 1P/1C, STOP cùng CLI/log production; W2-C và W2-H hoàn thành phạm vi cơ bản, bản ghép đã được Codex review/kiểm tra theo yêu cầu.** Phần 1–8 giữ cơ sở thiết kế; phần 9 là bàn giao Châu, phần 10 là tích hợp Huy. [Minh chứng W2-C](../results/week2/chau-validation.md), [bàn giao API](handoff-week2-chau.md), [minh chứng W2-H](../results/week2/huy-validation.md), [review chung](../results/week2/group-validation.md). [PR #3](https://github.com/ndhuy1127/os-producer-consumer/pull/3) đã merge vào main ở `d125734`.

**Tuần 3 mở rộng lên P producer và C consumer (1..64 mỗi loại)**, giữ nguyên các nguyên tắc ở phần 1–7; chi tiết ở [phần 11](#11-tuần-3--nhiều-producerconsumer). Phần 9–10 mô tả snapshot Tuần 2. W3-C và W3-H do Claude Code thực hiện theo yêu cầu của Huy; Châu chưa review.

## 1. Bài toán và cấu hình

Producer tạo dữ liệu hữu hạn, consumer lấy dữ liệu qua hàng đợi có N chỗ rồi xử lý. Mục tiêu: không ghi quá sức chứa, không đọc rỗng, không mất/lặp item và mọi luồng tự kết thúc.

Chốt K là số DATA mỗi producer, tổng DATA = P*K; P,C > 0, K >= 0. Mặc định `(N,P,C,K,Dp_ms,Dc_ms) = (8,1,1,20,0,0)`. Giới hạn: N=1..65536 (đồng thời <= SEM_VALUE_MAX và SIZE_MAX/sizeof(item)); P,C=1..64; K=0..1000000 và P*K<=1000000; mỗi độ trễ 0..60000 **millisecond**. K=0 hợp lệ, chỉ cần dừng consumer bằng STOP. Trước tạo worker phải kiểm tra tràn phép nhân, ID, kích thước cấp phát và bộ đếm gồm STOP.

Tên cờ và quy tắc từ chối lỗi, quiet/help, mã thoát 0/1/2 ở [config-log-week1-huy.md](config-log-week1-huy.md); cấu hình/kiểm tra thực ở [test-plan.md](test-plan.md). Parser đã ghép; Tuần 2 chỉ nhận P=C=1, từ Tuần 3 API/CLI nhận P, C = 1..64 ([phần 11](#11-tuần-3--nhiều-producerconsumer)).

## 2. Bộ đệm vòng FIFO

| Nội dung | Giao diện đã chốt theo code |
| --- | --- |
| Cấu trúc | Mảng N item; `head` là vị trí lấy kế tiếp, `tail` là vị trí thêm kế tiếp, `count` là số item hiện có; sau thao tác tăng chỉ số theo modulo N |
| Bất biến | `0 <= count <= N`, `0 <= head, tail < N`; khởi tạo head = tail = count = 0; push/pop trọn vẹn trong guard |
| API | `buffer_init`, `buffer_push`, `buffer_pop`, `buffer_destroy` tuần tự; push từ chối khi đầy, pop từ chối khi trống |
| Bộ nhớ/lỗi | Main sở hữu đối tượng buffer; init cấp phát mảng, destroy thu hồi; kiểm tra N và tràn kích thước cấp phát, không để trạng thái khởi tạo dở gây double-free |

Đã có [item.h](../include/item.h), [buffer.h](../include/buffer.h), [buffer.c](../src/buffer.c), [test_buffer.c](../tests/test_buffer.c) của Huy. 12 ca tuần tự và sanitizer 11 ca thường đã chạy trên máy Huy; nguồn/hash được đối chiếu khi kiểm tra bản chung. Giữ nguyên mã C/header vì phù hợp hợp đồng, không cài đặt đồng bộ vào buffer. Chữ ký thực tế:

```c
enum item_kind { ITEM_DATA, ITEM_STOP };
struct item {
    enum item_kind kind;
    uint64_t id;       /* Chi co nghia voi ITEM_DATA. */
    int value;
};

int buffer_init(struct buffer *buffer, size_t capacity);
int buffer_push(struct buffer *buffer, struct item item);
int buffer_pop(struct buffer *buffer, struct item *out);
void buffer_destroy(struct buffer *buffer);
```

`struct buffer` có `struct item *items` và các trường size_t `head`, `tail`, `count`, `capacity`. **Phải zero-init trước init đầu tiên**, ví dụ `struct buffer buffer = {0};`. Mã lỗi trả trực tiếp theo enum trong buffer.h: BUFFER_OK=0, INVALID_ARGUMENT=1, FULL=2, EMPTY=3, SIZE_OVERFLOW=4, NO_MEMORY=5, ALREADY_INITIALIZED=6; không dựa vào errno và không dùng strerror để giải nghĩa enum riêng này.

Push/pop sao chép item theo giá trị. `out` phải trỏ tới một item ghi được riêng, **không alias** vào đối tượng buffer/mảng; consumer dùng item local rồi xử lý bản sao sau khi nhả guard. Lỗi init/push/pop bảo toàn trạng thái; pop lỗi còn bảo toàn out. Buffer sở hữu mảng malloc; destroy nhận NULL/đối tượng zero/live, free mảng và reset mọi trường về zero, có thể gọi lặp hoặc init lại. Không sao chép buffer đang sở hữu mảng để dùng/hủy như đối tượng thứ hai, không tự free/sửa mảng từ ngoài. Các snapshot trong test chỉ dùng đối chiếu trường, không được dùng như buffer có quyền sở hữu. Caller không được dùng con trỏ dangling/struct chưa init.

Item không chứa con trỏ heap nên không có payload cần free/chuyển quyền sở hữu. Hợp đồng chi tiết và thứ tự kiểm tra lỗi ở [buffer-week1-huy.md](buffer-week1-huy.md).

**Các hàm bộ đệm tuần tự không tự chờ/post semaphore.** Lớp đồng bộ do Châu triển khai Tuần 2 bao quanh push/pop; không thêm semaphore lần nữa bên trong API của Huy. Khi init/destroy chưa có luồng chạy, main không cần guard. Khi chạy đồng thời, mọi đọc/sửa mảng, head/tail/count kể cả thống kê occupancy đều qua guard.

Đối số luồng đề xuất: mỗi producer có struct riêng chứa ID producer, số item, độ trễ, con trỏ đến context và ô kết quả riêng; mỗi consumer có struct riêng chứa ID consumer, độ trễ, context và ô kết quả riêng. Context chung chứa buffer, empty/full/guard và dữ liệu kiểm chứng được bảo vệ. Main sở hữu context, mảng pthread_t, đối số và kết quả đến sau join tất cả; worker không giải phóng context và không trả địa chỉ biến local. Cấu hình chung chỉ đọc sau khi tạo luồng. Dùng API thực tế của Huy nêu trên; main giữ quyền sở hữu duy nhất.

## 3. Dữ liệu và thứ tự

Chốt ID dữ liệu của producer p (0 <= p < P), item thứ j (0 <= j < K) là `p*K + j`; kiểm tra tràn trước khi tạo luồng. ID này không biểu diễn thứ tự FIFO toàn cục. `kind = ITEM_DATA` phân biệt với `kind = ITEM_STOP`; ID/value của STOP không tham gia thống kê, không dùng ID dữ liệu hợp lệ làm mã dừng.

FIFO kiểm tra theo hai dãy ID ghi tại **enqueue/dequeue thực sự trong guard**. Thứ tự tạo ID, thứ tự worker bắt đầu hoặc thứ tự in log sau khi nhả guard có thể khác. Đếm số lượng DATA và đối chiếu tập/tần suất ID để phát hiện mất/lặp; chỉ tổng số bằng nhau chưa đủ. STOP vẫn chiếm một chỗ trong count/occupancy nhưng không tính là dữ liệu sản xuất/tiêu thụ.

## 4. Semaphore và vùng tới hạn

Châu chọn ba POSIX semaphore không đặt tên cho thiết kế này:

| Semaphore | Khởi tạo dự kiến | Vai trò |
| --- | --- | --- |
| empty | `sem_init(&empty, 0, N)` | Quyền chỗ trống trước enqueue |
| full | `sem_init(&full, 0, 0)` | Quyền item có sẵn trước dequeue |
| guard | `sem_init(&guard, 0, 1)` | Loại trừ lẫn nhau cho trạng thái bộ đệm |

`pshared = 0` vì mọi luồng cùng tiến trình; context đặt ở vùng bộ nhớ có thể truy cập bởi tất cả luồng. empty/full không thay guard: nhiều producer có thể đồng thời lấy quyền chỗ trống nhưng vẫn cần lần lượt sửa tail/count.

Không giữ guard khi chờ empty/full, ngủ minh họa, xử lý dữ liệu hoặc join. Lấy guard rồi chờ empty khi đầy sẽ gây bế tắc: producer giữ guard cần consumer giải phóng chỗ, trong khi consumer cần guard mới lấy được item. Ví dụ diễn tiến chi tiết ở [lý thuyết](theory-week1-chau.md).

### Hướng xử lý lỗi cho Tuần 2

Các giả mã phần 5 mô tả **đường chạy thành công**. `wait(guard)` gọi sem_wait trực tiếp; `wait(empty/full)` dùng helper probe đã chốt ở phần 7 để lấy đúng một quyền. Sem_wait thử lại chỉ khi errno == EINTR; lỗi khác phải báo và chuyển sang xử lý lỗi. Mọi sem_init, sem_post, sem_destroy cũng phải kiểm tra kết quả/errno. Hàm pthread trả mã lỗi trực tiếp, dùng strerror(error); không đọc errno thay cho mã trả về.

Main lưu số luồng create thành công và cờ tài nguyên init thành công; chỉ join các luồng đó và chỉ destroy tài nguyên đã init. Nếu chưa tạo consumer nào mà create lỗi, dọn tài nguyên ngay, không tạo producer. Nếu tạo consumer lỗi giữa chừng, chưa tạo producer: gửi STOP cho số consumer đã tạo, join chúng rồi trả lỗi. Nếu producer create lỗi giữa chừng, join các producer đã tạo, gửi STOP cho các consumer thực có, join chúng và báo phiên chạy không hoàn chỉnh.

Nếu đã lấy quyền empty/full mà bước lấy guard lỗi, trả lại quyền chưa dùng. Nếu push/pop thất bại trước khi thay đổi bộ đệm, nhả guard và hoàn trả quyền đã đặt trước; API hiện tại bảo toàn trạng thái khi trả lỗi theo hợp đồng đã kiểm tra tuần tự. Sau khi buffer đã thay đổi, không được hoàn trả semaphore như thể thao tác chưa xảy ra. Lỗi post/guard hoặc worker bất thường có thể làm hỏng tiến trình đồng bộ; không chỉ return worker rồi để main join vô hạn. Đề xuất Tuần 2 báo lỗi và kết thúc toàn tiến trình với mã lỗi khi không thể bảo đảm đánh thức/dừng các worker; cơ chế dừng có phối hợp để thu hồi sạch mọi tài nguyên cần thiết kế và kiểm thử riêng nếu nhóm chọn bổ sung.

Nếu join lỗi, không coi worker đã dừng, không hủy context/semaphore còn có thể được dùng; báo lỗi và kết thúc tiến trình. Không dùng pthread_cancel tùy tiện vì luồng có thể giữ guard/quyền item. **Ví dụ Tuần 1 chỉ đã viết kiểm tra create/join, join các worker đã tạo và kiểm tra kết quả; chưa viết hoặc thử các đường lỗi semaphore/bộ đệm ở trên.**

## 5. Giả mã

Giả mã dùng API tuần tự thực tế của phần 2 (DATA/STOP là cách viết ngắn của ITEM_DATA/ITEM_STOP); `buffer` là con trỏ đến đối tượng `b` do main sở hữu. Các lời gọi đều phải thành công để đi bước kế tiếp; hướng lỗi tách ở phần 4. Không triển khai các hàm này trong tuần 1.

```text
enqueue_with_semaphores(item):         // producer và main gửi STOP đều dùng
    wait(empty)                       // có thể chờ; chưa giữ guard
    wait(guard)
    buffer_push(buffer, item)         // sửa ô, tail, count
    kiểm tra 0 <= count <= N
    ghi thứ tự enqueue và occupancy trong guard
    nếu item.kind == DATA: tăng số DATA đã enqueue
    post(guard)
    post(full)

producer(args):
    lặp j từ 0 đến K-1:
        tạo item DATA với ID riêng ngoài vùng găng
        enqueue_with_semaphores(item)
        nếu có độ trễ minh họa: ngủ ngoài guard
    trả kết quả/trạng thái qua ô riêng có thời gian sống đủ lâu

consumer(args):
    lặp:
        wait(full)                    // có thể chờ; chưa giữ guard
        wait(guard)
        buffer_pop(buffer, &item)     // sửa ô, head, count
        kiểm tra 0 <= count <= N
        ghi thứ tự dequeue và occupancy trong guard
        nếu item.kind == DATA: tăng số DATA đã dequeue
        post(guard)
        post(empty)                   // cả STOP cũng trả chỗ trống
        nếu item.kind == STOP: kết thúc vòng lặp
        xử lý DATA ngoài guard, ghi kết quả riêng của consumer
        nếu có độ trễ minh họa: ngủ ngoài guard
    trả kết quả/trạng thái qua ô riêng

send_stop_signals(number_of_consumers):
    lặp number_of_consumers lần:
        tạo item với kind = STOP       // không dùng ID DATA để nhận diện
        enqueue_with_semaphores(item) // không bỏ qua empty/guard/full

main:
    kiểm tra cấu hình và tràn số; chuẩn bị đối số/kết quả có vòng đời đủ dài
    struct buffer b = {0}; buffer = &b; buffer_init(buffer, N), kiểm tra BUFFER_OK
    cấp phát oracle, metadata N ô và record log hữu hạn; kiểm tra lỗi/tràn
    khởi tạo empty=N, full=0, guard=1 với pshared=0; ghi cờ init thành công
    tạo C consumer; ghi nhận từng lần pthread_create thành công
    tạo P producer; ghi nhận từng lần pthread_create thành công
    pthread_join từng producer đã tạo, không giữ guard
    send_stop_signals(số consumer đã tạo)
    pthread_join từng consumer đã tạo, không giữ guard
    kiểm tra kết quả DATA, tập ID, FIFO, count cuối = 0; in tổng kết
    hủy các semaphore đã init sau khi mọi worker đã join
    buffer_destroy(buffer); thu hồi context/đối số/kết quả do main sở hữu
    trả mã thành công chỉ khi đủ luồng, đủ dữ liệu và mọi kiểm tra đúng
```

## 6. Kết thúc hữu hạn

Phương án Châu chọn: **main join producer rồi đưa một STOP cho mỗi consumer**. Lúc producer đều đã join, không còn luồng nào enqueue DATA; mọi DATA trong buffer nằm trước STOP theo FIFO. Main có thể chờ empty khi buffer đầy vì consumer vẫn đang lấy dữ liệu và main không giữ guard trong lúc chờ.

Consumer dequeue STOP, nhả guard, `post(empty)` rồi mới kết thúc. Mỗi consumer dừng ở STOP đầu tiên nên không lấy nhiều STOP; C STOP đủ cho C consumer. C có thể lớn hơn N: main đưa STOP dần qua enqueue thông thường, không cần buffer chứa tất cả cùng lúc.

FIFO bảo đảm mọi DATA đã enqueue trước STOP được **lấy ra** trước khi STOP được lấy. Một consumer có thể đang xử lý DATA ngoài guard trong lúc consumer khác lấy STOP; vì main join mọi consumer, tổng kết chỉ diễn ra khi xử lý DATA đó cũng đã xong. Giả định phép tạo/xử lý dữ liệu là hữu hạn và bộ lập lịch cho các luồng cơ hội chạy. Khi count cuối bằng 0 và mọi luồng đã join mới hủy semaphore/bộ nhớ. STOP không tính vào số DATA hoặc tập ID. Tuần 2 nghiệm thu trường hợp 1P/1C; Tuần 3 đã triển khai và kiểm thử nhiều producer/consumer, kể cả C > N.

## 7. Log và xác minh

Schema chốt gồm `op_seq`, `op=ENQUEUE|DEQUEUE`, `actor=P<p>|C<c>|MAIN` (Tuần 2 chỉ có P0/C0), `kind=DATA|STOP`, `data_id` (số hoặc `-` cho STOP), `value`, `enqueue_seq`, `count_after`, `capacity`, `wait_reason=NONE|WAIT_SLOT|WAIT_ITEM`. Tăng số thứ tự, chụp record/occupancy tại thao tác thành công **trong guard**; gồm cả STOP. Enqueue_seq tăng cho mỗi enqueue, lưu trong mảng metadata N ô của context theo ô tail; dequeue lấy metadata cùng ô head trước pop. Không đổi struct item hoặc API buffer. Cấp phát trước tối đa `2*(P*K+C)` record thao tác sau kiểm tra tràn; main in theo op_seq sau join, quiet bỏ in chi tiết nhưng giữ oracle và tổng kết.

Chốt quan sát thiếu quyền: helper chờ empty/full thử sem_trywait trước; thành công thì không wait thêm; EINTR thử lại; EAGAIN gán cờ chờ local rồi sem_wait (thử lại EINTR). Cờ local được gắn vào record thao tác sau khi lấy guard; guard dùng sem_wait trực tiếp. EAGAIN chỉ chứng minh thiếu quyền tại lần thử, không chứng minh sem_wait đã block hoặc count=N/0. Snapshot count=N/0 trong guard chứng minh trạng thái đầy/rỗng riêng. Không dùng timestamp, thứ tự dòng in ngoài guard hoặc sem_getvalue để suy ra FIFO/an toàn. Cách probe không đổi thứ tự lấy quyền trước guard trong giả mã.

Tổng kết gồm cấu hình, DATA/processed và tần suất ID, lỗi value/FIFO/count, min/max/final count, STOP, WAIT_SLOT/WAIT_ITEM và worker create/join. **Record/helper của Châu và CLI/log production của Huy đã ghép.** Tuần 3 thêm `order_errors` và tổng kết theo từng luồng; T01–T09 đều đã chạy đạt, xem [test-plan](test-plan.md). Script dùng timeout 5+ceil(2·P·K·(Dp_ms+Dc_ms)/1000) giây cho mỗi lần chạy (bằng công thức Tuần 2 khi P=1); đây là ngân sách kiểm thử, không phải bảo đảm của bộ lập lịch.

## 8. Tài liệu lý thuyết đã đối chiếu

Đã đối chiếu Linux man-pages ngày 08/10/2026:

- [pthreads(7)](https://man7.org/linux/man-pages/man7/pthreads.7.html): chia sẻ bộ nhớ và ngăn xếp riêng.
- [pthread_create(3)](https://man7.org/linux/man-pages/man3/pthread_create.3.html), [pthread_join(3)](https://man7.org/linux/man-pages/man3/pthread_join.3.html): chữ ký, đối số, vòng đời và mã lỗi trực tiếp.
- [sem_init(3)](https://man7.org/linux/man-pages/man3/sem_init.3.html): giá trị ban đầu, pshared = 0 và giới hạn giá trị.
- [sem_wait(3)](https://man7.org/linux/man-pages/man3/sem_wait.3.html), [sem_post(3)](https://man7.org/linux/man-pages/man3/sem_post.3.html): chờ/trả quyền, lỗi errno và EINTR khi wait.
- [sem_destroy(3)](https://man7.org/linux/man-pages/man3/sem_destroy.3.html): chỉ hủy semaphore đã init khi không còn luồng đang chờ/sử dụng.

Ngày 20/10/2026, khi soạn báo cáo nháp Tuần 3, đối chiếu lại [sem_overview(7)](https://man7.org/linux/man-pages/man7/sem_overview.7.html) mục DESCRIPTION, sem_wait(3) mục DESCRIPTION/ERRORS và sem_post(3) mục DESCRIPTION/ERRORS. sem_post(3) chỉ nói một luồng đang chờ được đánh thức, không quy định luồng nào, nên thiết kế không giả định công bằng giữa các consumer.

Giáo trình *Modern Operating Systems*, ấn bản 4, Tanenbaum và Bos là nguồn đọc thêm được gợi ý; chưa truy cập để đối chiếu trang/mục. Không lưu PDF vào GitHub. Xem giải thích và ví dụ nguyên lý ở [theory-week1-chau.md](theory-week1-chau.md).

## 9. Triển khai Tuần 2 — W2-C

[pc.h](../include/pc.h) công bố `pc_default_config`, `pc_run`, `pc_result_destroy`, cấu hình, record và tổng kết. [pc.c](../src/pc.c) giữ context nội bộ gọn: buffer zero-init sở hữu duy nhất, empty/full/guard, cờ init, metadata enqueue_seq theo N ô, hai mảng tần suất K ID, record tối đa `2*(K+1)`. Cấu hình bất biến trong khi chạy; kiểm tra giới hạn/kích thước/uint64_t trước cấp phát và trước create. DATA P0 có id=0..K-1, value=`id%1000+1`; STOP có kind riêng, không tính vào oracle DATA.

`pc_run` sống đồng bộ đến hết join: init/cấp phát → create consumer → create producer → join producer → lấy guard chụp số DATA còn đợi → enqueue một STOP qua cùng helper → join consumer → xác minh → cleanup. Đối số/kết quả worker ở stack của pc_run tồn tại đến sau join; không trả địa chỉ local. Consumer lấy item bằng bản sao riêng, tăng kết quả xử lý riêng ngoài guard; main đọc sau join. Tất cả buffer, metadata, record và tần suất dùng chung được đọc/ghi trong guard khi worker còn chạy. Trường create/join chỉ main ghi; từng bộ đếm thao tác chỉ được cập nhật trong guard. Sau tất cả join, main đối chiếu tần suất và hai dãy item/sequence của record, count/STOP và vòng đời.

Helper `take_permission` dùng trywait/EINTR/EAGAIN theo phần 7, chỉ lấy một quyền; `wait_retry` thử lại sem_wait khi EINTR cho cả guard lẫn empty/full. `nanosleep` theo ms, thử lại EINTR và luôn ngoài guard. Không có sem_getvalue, busy-wait, pthread_cancel hoặc dùng sleep để đồng bộ.

Đường lỗi đã triển khai:

- Init/cấp phát lỗi: dọn những tài nguyên đã init thành công, chưa tạo worker; result vẫn phải destroy nếu đã có record.
- Consumer create lỗi: không tạo producer, dọn tài nguyên. Producer create lỗi: consumer đã tạo được dừng bằng STOP rồi join, phiên chạy trả 1, không báo xác minh hoàn chỉnh.
- Guard wait lỗi sau khi giữ quyền: post hoàn trả empty/full trước khi kết thúc tiến trình. Push/pop lỗi (hợp đồng giữ nguyên buffer): nhả guard và hoàn trả quyền. Sau push/pop thành công chỉ post semaphore đối ứng; không rollback quyền như chưa sửa buffer.
- Lỗi semaphore khi chạy hoặc join lỗi: in tên lời gọi/mã lỗi, flush stderr rồi `_Exit(1)` toàn tiến trình. Không hủy bộ nhớ/semaphore khi chưa xác nhận worker dừng, không để main join vô hạn. Đây là lựa chọn kết thúc nghiêm trọng đã nêu ở phần 4, chưa phải cơ chế shutdown có phối hợp nhiều luồng.
- Destroy sau join lỗi: kiểm tra và báo mọi sem_destroy, trả 1. Bộ đệm giữ nguyên API lỗi enum trực tiếp; pthread dùng mã trả về, semaphore dùng errno.

Ở snapshot bàn giao W2-C, main chỉ chạy mặc định N=8,1P/1C,K=20 và từ chối đối số; test runner dùng pc_run để chạy N=5/T02–T05/K0/queued-stop. Minh chứng Châu giữ nguyên tại [results/week2](../results/week2/README.md). T04 có snapshot đầy/WAIT_SLOT, T05 rỗng/WAIT_ITEM; queued-stop còn 8 DATA sau producer join, tiêu thụ đủ trước STOP. CLI mới được ghép trong phần 10; lõi pc.c/pc.h và các test Châu giữ nguyên.

## 10. Tích hợp Tuần 2 — W2-H

[cli.c](../src/cli.c)/[cli.h](../include/cli.h) phân tích argv vào struct cli_options, dùng pc_default_config; chỉ nhận --option value, --quiet và --help dùng riêng. Chỉ ASCII digits, strtoumax/end pointer/errno/ERANGE, giới hạn kiểu/range, bitmap chống cờ lặp; lỗi cụ thể exit 2 trước pc_run. P/C>1 báo chưa hỗ trợ Tuần 2 (Tuần 3 bỏ giới hạn này). Kiểm tra phép nhân DATA, cộng STOP, số/kích thước record, semaphore và buffer/metadata theo cùng điều kiện của lõi; không cấp phát hay tạo worker khi parse. Biên lớn chỉ kiểm tra bằng driver parser.

Main parse → help (không pc_run) hoặc zero-init result → pc_run → kiểm tra status/verified → [runtime_log_result](../src/runtime_log.c) sau mọi join → pc_result_destroy → trả status. Result sở hữu duy nhất records; logger mượn snapshot, không sao chép/free hay sửa context. Luồng, semaphore, buffer và oracle không đổi. Lỗi có phục hồi vẫn destroy result; status=1/verified=1 do cleanup lỗi vẫn FAIL, kể cả quiet. Logger kiểm tra lỗi stdio/fflush và main trả 1 khi output lỗi.

Logger in config trước, record theo op_seq rồi tổng kết DATA, buffer/STOP, waits/record_count/DATA pending và worker/status. PRIu64/%zu đúng kiểu; STOP data_id=-, vẫn chiếm count. Quiet chỉ bỏ dòng op_seq, không bỏ records/oracle/summary. In sau chạy xong; không có I/O trong guard, log không biểu diễn thời gian thực hoặc thời gian block kernel.

[check_cli_log.py](../scripts/check_cli_log.py) kiểm tra production với oracle queue tuyến tính từ stdout, đối chiếu schema/kind/id/value/enqueue_seq/FIFO/count và summary; 8 cấu hình thường + 8 quiet. Wrapper dùng cùng main để chặn pc_run/pthread_create ở đầu vào sai/help, có positive control exit 99; tiêm init/cleanup/verified để kiểm tra FAIL/thu hồi result. Không ghép wrapper vào production. [validate_week2_huy.py](../scripts/validate_week2_huy.py) build/hồi quy/CLI/log/sanitizer và chỉ ghi huy-*, không gọi writer lịch sử Châu. 16 runtime, 116 đầu vào sai, 5 biên parser, formatter 7 ca và main errors 6 ca đạt; [minh chứng Huy](../results/week2/huy-validation.md). Mốc kỹ thuật cơ bản và review bản ghép đạt; đã bàn giao qua PR #3 (`d125734`).

## 11. Tuần 3 — nhiều producer/consumer

Claude Code thực hiện theo yêu cầu của Huy: W3-C ở commit `c99d131`, W3-H ở commit `3bda04b`. Châu chưa review. Các nguyên tắc ở phần 1–7 giữ nguyên: ba semaphore empty/full/guard, lấy quyền empty/full trước guard, không giữ guard khi chờ hoặc ngủ, một STOP cho mỗi consumer, bản ghi chụp trong guard và in sau join.

### 11.1 Lõi (W3-C)

- **Giới hạn chung** trong [pc.h](../include/pc.h): `PC_MAX_CAPACITY` 65536, `PC_MAX_WORKERS` 64 (mỗi loại), `PC_MAX_TOTAL_ITEMS` 1000000, `PC_MAX_DELAY_MS` 60000. `config_valid` kiểm tra P, C ∈ [1, 64], rồi P·K ≤ 1000000 bằng phép chia **trước** khi nhân, rồi kích thước 2·(P·K + C) bản ghi.
- **Context**: `total_items` = P·K; hai mảng tần suất P·K phần tử; `record_capacity` = 2·(P·K + C). Thứ tự `calloc` giữ như Tuần 2 để các ca tiêm lỗi cấp phát vẫn đúng vị trí.
- **Luồng**: mảng `pthread_t` và `worker_args {context, index, completed}` cỡ 64 nằm trên stack của `pc_run`, sống tới sau mọi join. Producer p sinh ID p·K+j, value = ID mod 1000 + 1. Consumer c dừng ở STOP đầu tiên.
- **Trình tự**: tạo C consumer → tạo P producer → join P producer → chụp `count` trong guard → C lần enqueue STOP với actor MAIN → join C consumer → kiểm chứng → hủy semaphore/bộ nhớ.
- **Tạo luồng lỗi giữa chừng**: consumer thứ i lỗi với i > 0 thì không tạo producer nào, gửi i STOP, join i consumer, trả 1; i = 0 thì dọn ngay. Producer thứ j lỗi thì join j producer đã chạy, gửi C STOP, join mọi consumer, trả 1; dữ liệu của j producer vẫn được tiêu thụ hết nhưng kết quả không được tính là đạt. Lỗi semaphore/join khi đang chạy vẫn kết thúc toàn tiến trình với mã 1 như Tuần 2.
- **Bản ghi và tổng kết**: `pc_record` thêm `actor_index`; `pc_summary` thêm `order_errors`, `producer_items[64]`, `consumer_items[64]`, `consumer_stops[64]`. Các trường mới đặt ở **cuối** struct để các khởi tạo theo vị trí có sẵn vẫn đúng.
- **Kiểm chứng sau join** thêm: ID p·K+j phải do producer p đưa vào, và j tăng dần cho từng producer ở cả phía đưa vào lẫn lấy ra; mỗi consumer nhận đúng một STOP; số luồng tạo/join bằng P/C; số STOP đưa vào/lấy ra bằng C.

### 11.2 Rà soát đồng bộ khi có nhiều luồng

| Dữ liệu dùng chung | Ai ghi | Cách bảo vệ |
| --- | --- | --- |
| Bộ đệm, `slot_sequence`, `enqueues`/`dequeues`, bản ghi, mảng tần suất, bộ đếm theo thao tác (DATA/STOP, chờ, min/max, lỗi FIFO/count, `consumer_stops`) | Producer, consumer, main khi enqueue/dequeue | Chỉ đọc/ghi khi giữ guard |
| `worker_args[i].completed` | Chỉ luồng i | Không chia sẻ; main đọc sau khi join luồng i |
| Cấu hình, `total_items`, `record_capacity` | Main, trước khi tạo luồng | Chỉ đọc khi luồng chạy |
| Số luồng đã tạo/join | Chỉ main | Ô nhớ riêng; luồng khác không đọc |
| DATA còn đợi khi gửi STOP | Main | Đọc `count` trong guard vì consumer còn chạy |
| Kết quả kiểm chứng (`missing`, `duplicate`, `order_errors`, số DATA theo luồng…) | Main | Sau mọi join, không còn luồng nào |

Nhiều producer cùng chờ `empty` hay nhiều consumer cùng chờ `full` là an toàn vì semaphore đếm số quyền; việc sửa `tail`/`head`/`count` chỉ diễn ra trong guard. ThreadSanitizer không phát hiện data race trên 9 cấu hình nhiều luồng của chương trình chính và 16 preset lõi ([minh chứng](../results/week3/README.md)). Kết quả này chỉ bao phủ các lịch chạy đã xảy ra.

### 11.3 CLI và log (W3-H)

- [cli.c](../src/cli.c) nhận `--producers`/`--consumers` 1..64 và từ chối P·K > 1000000 (exit 2, chưa gọi `pc_run`); giá trị mặc định không đổi.
- [runtime_log.c](../src/runtime_log.c) in `actor=P<p>|C<c>|MAIN`, từ chối chỉ số luồng ngoài 0..63; dòng DATA thêm `order_errors`; thêm `producers(DATA): P0=… P1=…` và `consumers(DATA/STOP): C0=x/1 …`. Tổng kết có 7 dòng (config, DATA, buffer, waits, producers, consumers, workers).

### 11.4 Kiểm thử

16 preset lõi, 32 ca tiêm lỗi, chương trình chính 15 cấu hình × thường/quiet, chạy lặp 7 cấu hình × 20 lần, ASan/UBSan và TSan đều đạt ngày 20/10/2026. Cấu hình và kết quả ở [test-plan](test-plan.md), [bảng kết quả](../results/week3/results-table.md); lập luận tính đúng ở [báo cáo nháp](final-report/bao-cao-nhap.md).
