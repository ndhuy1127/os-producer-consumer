# Thiết kế Tuần 1 — đề xuất của Châu

Trạng thái: **Châu đã hoàn thành thiết kế Tuần 1; chưa được Huy xác nhận và chưa triển khai Producer–Consumer**. Tài liệu giữ tám phần của mẫu để phối hợp với Huy. CLI, cấu trúc/API bộ đệm và log dưới đây là đề xuất cần thống nhất. Hiện `src/` và `include/` chưa có API C của Huy để đối chiếu. Lý thuyết ở [theory-week1-chau.md](theory-week1-chau.md); ví dụ đã viết chỉ là [tạo/join luồng](../examples/thread_lifecycle.c).

## 1. Bài toán và cấu hình

Producer tạo dữ liệu hữu hạn, consumer lấy dữ liệu qua hàng đợi có N chỗ rồi xử lý. Mục tiêu: không ghi quá sức chứa, không đọc rỗng, không mất/lặp item và mọi luồng tự kết thúc.

Đề xuất ký hiệu: sức chứa N > 0; P > 0 producer; C > 0 consumer; mỗi producer tạo K >= 0 item, tổng dữ liệu P*K. Phải kiểm tra tràn phép nhân và N không vượt `SEM_VALUE_MAX` trước khởi tạo. Đơn vị độ trễ đề xuất là millisecond, bằng 0 thì không ngủ. Cú pháp CLI, giá trị mặc định và giới hạn đầu vào **chưa chốt với Huy**; tuần này không viết CLI.

## 2. Bộ đệm vòng FIFO

| Nội dung | Đề xuất để phối hợp |
| --- | --- |
| Cấu trúc | Mảng N item; `head` là vị trí lấy kế tiếp, `tail` là vị trí thêm kế tiếp, `count` là số item hiện có; sau thao tác tăng chỉ số theo modulo N |
| Bất biến | `0 <= count <= N`, `0 <= head, tail < N`; khởi tạo head = tail = count = 0; push/pop trọn vẹn trong guard |
| API | `buffer_init`, `buffer_push`, `buffer_pop`, `buffer_destroy` tuần tự; push từ chối khi đầy, pop từ chối khi trống |
| Bộ nhớ/lỗi | Main sở hữu đối tượng buffer; init cấp phát mảng, destroy thu hồi; kiểm tra N và tràn kích thước cấp phát, không để trạng thái khởi tạo dở gây double-free |

**Chưa có triển khai hoặc kiểm thử bộ đệm tuần tự.** Phần này thuộc Huy. Giao diện C sau chỉ là bản hợp đồng đề xuất, chưa tạo header/nguồn:

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

`struct buffer` và header dùng `<stdint.h>`, `<stddef.h>` sẽ do Huy chốt. Đề xuất các hàm `int` trả 0 khi thành công, mã lỗi khác 0 khi thất bại; xác nhận lại để tương thích nếu Huy chọn API khác. `push` sao chép item theo giá trị; `pop` trả bản sao qua `out`. Item không chứa con trỏ heap nên không có quyền sở hữu payload cần chuyển giữa luồng. Nếu đổi sang payload cấp phát động, phải bổ sung hợp đồng ai giải phóng trước tích hợp.

**Các hàm bộ đệm tuần tự không tự chờ/post semaphore.** Lớp đồng bộ do Châu triển khai Tuần 2 bao quanh push/pop; không thêm semaphore lần nữa bên trong API của Huy. Khi init/destroy chưa có luồng chạy, main không cần guard. Khi chạy đồng thời, mọi đọc/sửa mảng, head/tail/count kể cả thống kê occupancy đều qua guard.

Đối số luồng đề xuất: mỗi producer có struct riêng chứa ID producer, số item, độ trễ, con trỏ đến context và ô kết quả riêng; mỗi consumer có struct riêng chứa ID consumer, độ trễ, context và ô kết quả riêng. Context chung chứa buffer, empty/full/guard và dữ liệu kiểm chứng được bảo vệ. Main sở hữu context, mảng pthread_t, đối số và kết quả đến sau join tất cả; worker không giải phóng context và không trả địa chỉ biến local. Cấu hình chung chỉ đọc sau khi tạo luồng. Ưu tiên dùng API thực tế của Huy khi có.

## 3. Dữ liệu và thứ tự

Đề xuất ID dữ liệu của producer p (0 <= p < P), item thứ j (0 <= j < K) là `p*K + j`; kiểm tra tràn trước khi tạo luồng. ID này không biểu diễn thứ tự FIFO toàn cục. `kind = ITEM_DATA` phân biệt với `kind = ITEM_STOP`; ID/value của STOP không tham gia thống kê, không dùng ID dữ liệu hợp lệ làm mã dừng.

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

Các giả mã phần 5 mô tả **đường chạy thành công**. `wait` nghĩa là gọi `sem_wait`, thử lại chỉ khi `errno == EINTR`; lỗi khác phải báo và chuyển sang xử lý lỗi. Mọi `sem_init`, `sem_post`, `sem_destroy` cũng phải kiểm tra kết quả/errno. Hàm pthread trả mã lỗi trực tiếp, dùng `strerror(error)`; không đọc errno thay cho mã trả về.

Main lưu số luồng create thành công và cờ tài nguyên init thành công; chỉ join các luồng đó và chỉ destroy tài nguyên đã init. Nếu chưa tạo consumer nào mà create lỗi, dọn tài nguyên ngay, không tạo producer. Nếu tạo consumer lỗi giữa chừng, chưa tạo producer: gửi STOP cho số consumer đã tạo, join chúng rồi trả lỗi. Nếu producer create lỗi giữa chừng, join các producer đã tạo, gửi STOP cho các consumer thực có, join chúng và báo phiên chạy không hoàn chỉnh.

Nếu đã lấy quyền empty/full mà bước lấy guard lỗi, trả lại quyền chưa dùng. Nếu push/pop thất bại trước khi thay đổi bộ đệm, nhả guard và hoàn trả quyền đã đặt trước; API cần bảo đảm không cập nhật dở khi trả lỗi. Sau khi buffer đã thay đổi, không được hoàn trả semaphore như thể thao tác chưa xảy ra. Lỗi post/guard hoặc worker bất thường có thể làm hỏng tiến trình đồng bộ; không chỉ return worker rồi để main join vô hạn. Đề xuất Tuần 2 báo lỗi và kết thúc toàn tiến trình với mã lỗi khi không thể bảo đảm đánh thức/dừng các worker; cơ chế dừng có phối hợp để thu hồi sạch mọi tài nguyên cần thiết kế và kiểm thử riêng nếu nhóm chọn bổ sung.

Nếu join lỗi, không coi worker đã dừng, không hủy context/semaphore còn có thể được dùng; báo lỗi và kết thúc tiến trình. Không dùng pthread_cancel tùy tiện vì luồng có thể giữ guard/quyền item. **Ví dụ Tuần 1 chỉ đã viết kiểm tra create/join, join các worker đã tạo và kiểm tra kết quả; chưa viết hoặc thử các đường lỗi semaphore/bộ đệm ở trên.**

## 5. Giả mã

Giả mã dùng API tuần tự của phần 2, với các lời gọi đều phải thành công để đi bước kế tiếp; hướng lỗi tách ở phần 4. Không triển khai các hàm này trong tuần 1.

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
    buffer_init(buffer, N)
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

FIFO bảo đảm mọi DATA đã enqueue trước STOP được **lấy ra** trước khi STOP được lấy. Một consumer có thể đang xử lý DATA ngoài guard trong lúc consumer khác lấy STOP; vì main join mọi consumer, tổng kết chỉ diễn ra khi xử lý DATA đó cũng đã xong. Giả định phép tạo/xử lý dữ liệu là hữu hạn và bộ lập lịch cho các luồng cơ hội chạy. Khi count cuối bằng 0 và mọi luồng đã join mới hủy semaphore/bộ nhớ. STOP không tính vào số DATA hoặc tập ID. Thiết kế chưa được nghiệm thu bằng chương trình thực tế.

## 7. Log và xác minh

Đề xuất sự kiện gồm thứ tự thao tác, enqueue/dequeue, ID luồng, kind, ID DATA và count sau thao tác. Số thứ tự và snapshot phải được chụp trong guard; nếu in ngoài guard thì không dùng thứ tự dòng in để suy ra FIFO. Cách lưu/định dạng log thuộc phần phối hợp với Huy, chưa chốt và chưa cài đặt.

Tổng kết dự kiến ghi cấu hình thực chạy, DATA enqueue/dequeue, ID bị thiếu/lặp, đối chiếu FIFO, min/max occupancy và trạng thái join. Các tiêu chí thuật toán vẫn ở [test-plan.md](test-plan.md), **chưa có ca thuật toán nào được chạy**. Timeout cho Producer–Consumer sẽ chốt theo số item/độ trễ sau triển khai. Timeout 5 giây trong [minh chứng Tuần 1](../results/week1/chau-validation.md) chỉ dành cho ví dụ 3 worker tính tổng.

## 8. Tài liệu lý thuyết đã đối chiếu

Đã đối chiếu Linux man-pages ngày 08/10/2026:

- [pthreads(7)](https://man7.org/linux/man-pages/man7/pthreads.7.html): chia sẻ bộ nhớ và ngăn xếp riêng.
- [pthread_create(3)](https://man7.org/linux/man-pages/man3/pthread_create.3.html), [pthread_join(3)](https://man7.org/linux/man-pages/man3/pthread_join.3.html): chữ ký, đối số, vòng đời và mã lỗi trực tiếp.
- [sem_init(3)](https://man7.org/linux/man-pages/man3/sem_init.3.html): giá trị ban đầu, pshared = 0 và giới hạn giá trị.
- [sem_wait(3)](https://man7.org/linux/man-pages/man3/sem_wait.3.html), [sem_post(3)](https://man7.org/linux/man-pages/man3/sem_post.3.html): chờ/trả quyền, lỗi errno và EINTR khi wait.
- [sem_destroy(3)](https://man7.org/linux/man-pages/man3/sem_destroy.3.html): chỉ hủy semaphore đã init khi không còn luồng đang chờ/sử dụng.

Giáo trình *Modern Operating Systems*, ấn bản 4, Tanenbaum và Bos là nguồn đọc thêm được gợi ý; chưa truy cập để đối chiếu trang/mục. Không lưu PDF vào GitHub. Xem giải thích và ví dụ nguyên lý ở [theory-week1-chau.md](theory-week1-chau.md).
