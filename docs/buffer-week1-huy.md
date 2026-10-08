# Bộ đệm vòng tuần tự — Tuần 1 của Huy

Nguyễn Đức Huy — 20233448 — `dev/huy`. Đã triển khai và kiểm thử tuần tự; hợp đồng chi tiết cần Châu xác nhận trước tích hợp Tuần 2. Tham khảo thiết kế tại [PR #1](https://github.com/ndhuy1127/os-producer-consumer/pull/1), nguồn `origin/dev/chau` ở SHA `2c8d7f61415c1dc730c8e4dbde4c991a37bfdfa6`. Giữ nguyên chữ ký API và kiểu item Châu đề xuất. PR #1 còn mở lúc kiểm tra, không merge hoặc sao chép ví dụ vào nhánh Huy.

## Cấu trúc và FIFO

[item.h](../include/item.h) định nghĩa `enum item_kind { ITEM_DATA, ITEM_STOP }` và `struct item { kind; uint64_t id; int value; }`. ID chỉ có ý nghĩa dữ liệu với DATA. STOP giữ nguyên cả ba trường khi sao chép, chiếm một chỗ bình thường; API không dừng consumer hoặc thay thống kê DATA.

[buffer.h](../include/buffer.h) chứa mảng `items`, `head` (ô lấy tiếp), `tail` (ô thêm tiếp), `count`, `capacity`, đều dùng `size_t` cho kích thước/chỉ số. [buffer.c](../src/buffer.c) cấp phát N ô, không dời cả mảng. Push ghi tại tail rồi `tail = (tail + 1) % N`; pop đọc tại head rồi `head = (head + 1) % N`. Bất biến khi đang hoạt động: `0 <= count <= N`, `head, tail < N`. head == tail có thể là rỗng hoặc đầy; count phân biệt hai trường hợp.

Ví dụ N=3: push A, B, C thì đầy; pop trả A; push D tái dùng ô 0; các lần pop tiếp trả B, C, D. FIFO theo thứ tự push thành công, không theo giá trị ID. Thử nghiệm dùng ID 90, 2, 77 và chuỗi kỳ vọng độc lập để kiểm tra điều đó.

## Hợp đồng API

```c
struct buffer b = {0};
int buffer_init(struct buffer *buffer, size_t capacity);
int buffer_push(struct buffer *buffer, struct item item);
int buffer_pop(struct buffer *buffer, struct item *out);
void buffer_destroy(struct buffer *buffer);
```

| Hàm | Tham số và kết quả |
| --- | --- |
| init | Đối tượng zero và N>0; cấp phát `N * sizeof(struct item)` sau kiểm tra tràn; trả OK hoặc lỗi, không thay đổi đối tượng khi lỗi |
| push | Đối tượng đang hoạt động, kind DATA/STOP; sao chép item theo giá trị; lỗi đầy/đối số giữ nguyên mọi ô và chỉ số |
| pop | Đối tượng đang hoạt động, out trỏ đến một item ghi được, tách khỏi đối tượng buffer và mảng của nó; trả bản sao qua out; lỗi giữ nguyên trạng thái và out |
| destroy | Nhận NULL, đối tượng zero hoặc đối tượng hoạt động; free mảng rồi đặt tất cả trường về zero; được gọi lặp và được init lại sau đó |

| Mã enum (giá trị) | Ý nghĩa |
| --- | --- |
| `BUFFER_OK` (0) | Thành công |
| `BUFFER_INVALID_ARGUMENT` (1) | NULL/zero capacity, push/pop trước init hoặc sau destroy, out NULL, kind không hợp lệ |
| `BUFFER_FULL` (2) | Push vào bộ đệm đầy |
| `BUFFER_EMPTY` (3) | Pop từ bộ đệm rỗng |
| `BUFFER_SIZE_OVERFLOW` (4) | N vượt `SIZE_MAX / sizeof(struct item)` |
| `BUFFER_NO_MEMORY` (5) | malloc thất bại |
| `BUFFER_ALREADY_INITIALIZED` (6) | init trên đối tượng chưa về zero; bảo toàn mảng và dữ liệu cũ |

Các mã trả trực tiếp, không dựa vào errno. Init kiểm tra NULL/N=0 trước, kiểm tra đối tượng zero tiếp theo, rồi tràn kích thước và malloc. Push kiểm tra đối số/kind trước kiểm tra đầy; pop kiểm tra out/đối tượng trước kiểm tra rỗng.

Main sở hữu struct buffer; init sở hữu mảng mới và destroy giải phóng mảng đó. Item không chứa con trỏ heap nên không có payload phải free riêng. Phải zero-initialize đối tượng lần đầu; không truyền struct chưa khởi tạo, không sửa các trường trực tiếp, không sao chép buffer đang sở hữu mảng hoặc free mảng từ ngoài. Destroy bộ đệm còn item vẫn hợp lệ. Sau destroy, push/pop bị từ chối; có thể init lại. C không thể xác minh mọi địa chỉ con trỏ tùy ý: con trỏ dangling, out alias vào mảng, struct bị caller sửa và đối tượng uninitialized nằm ngoài hợp đồng.

## Ranh giới đồng bộ Tuần 2

API không gọi semaphore, không chờ và không tự khóa. Châu sẽ lấy quyền empty/full trước guard, gọi push/pop trong guard, ghi thứ tự và occupancy cùng vùng bảo vệ, rồi post theo thiết kế PR #1. Mọi đọc count hoặc mảng khi dùng đồng thời cũng phải có guard. Init/destroy do main thực hiện khi không còn worker sử dụng. Không xem mã FULL/EMPTY tuần tự là thay thế cho semaphore.

Cần Châu xác nhận: zero-init và vòng đời, enum lỗi trực tiếp, out không alias, trách nhiệm khóa ngoài API, K item mỗi producer và schema log tại [config-log-week1-huy.md](config-log-week1-huy.md). Chưa có bằng chứng Châu đã đọc chéo hoặc nhóm nghiệm thu chung.

## Kiểm thử và phạm vi

[test_buffer.c](../tests/test_buffer.c) có 12 ca: init rỗng; sao chép một item; FIFO/đầy; rỗng; wrap-around; N=1; STOP; đối số lỗi; tràn cấp phát; init lại khi đang hoạt động; destroy/lặp/tái sử dụng; malloc thất bại/phục hồi. Oracle wrap-around là hàng đợi tuyến tính dịch phần tử, độc lập với modulo của triển khai; đối chiếu kind/id/value từng lần pop. Ca đầy đối chiếu cả mảng cũ, ca rỗng kiểm tra out không đổi.

GNU ld `--wrap=malloc,--wrap=free` chỉ dùng trong binary test, đếm cấp phát chưa giải phóng sau mỗi ca và tiêm đúng một lần malloc thất bại. Không thay thuật toán hay allocator sản phẩm. Bản sanitizer không bật wrapper chạy 11 ca thường; cả hai kết quả đã chạy thật ở [minh chứng](../results/week1/huy-validation.md). Không chứng minh mọi lỗi bộ nhớ có thể xảy ra, lịch chạy đa luồng, empty/full/guard hoặc consumer dừng nhờ STOP.
