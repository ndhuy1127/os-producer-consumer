# Đề xuất cấu hình và log — Tuần 1 của Huy

Đây là **đề xuất cần Châu xác nhận**, chưa có CLI parser hoặc log runtime. Dựa trên PR #1/thiết kế Châu ở SHA `2c8d7f61415c1dc730c8e4dbde4c991a37bfdfa6`: P producer, C consumer, **K item mỗi producer**, tổng DATA = P*K, ID = p*K+j (p,j từ 0). STOP không tính vào tổng DATA. API đã có ở [buffer-week1-huy.md](buffer-week1-huy.md); kế hoạch kiểm thử ở [test-plan.md](test-plan.md).

## CLI dự kiến cho Tuần 2

Tên chương trình dự kiến `bin/producer-consumer`; hiện chưa tồn tại. Các giới hạn dưới đây nhằm giới hạn tài nguyên demo, còn cần xác nhận; không mô tả parser đã chạy.

| Cờ | Mặc định | Phạm vi đề xuất | Ý nghĩa |
| --- | --- | --- | --- |
| `--capacity N` | 8 | 1..65536, đồng thời <= SEM_VALUE_MAX và SIZE_MAX/sizeof(struct item) | Sức chứa gồm DATA và STOP |
| `--producers P` | 1 | 1..64 | Số producer |
| `--consumers C` | 1 | 1..64 | Số consumer |
| `--items-per-producer K` | 20 | 0..1000000, tổng P*K <=1000000 | DATA của mỗi producer; K=0 chỉ kiểm tra dừng |
| `--producer-delay-ms Dp` | 0 | 0..60000 | Độ trễ sau enqueue DATA, ngoài guard |
| `--consumer-delay-ms Dc` | 0 | 0..60000 | Độ trễ sau xử lý DATA, ngoài guard |
| `--quiet` | tắt | Cờ không có giá trị | Giảm dòng chi tiết; vẫn xác minh và in tổng kết/lỗi |
| `--help` | — | Dùng riêng | In trợ giúp, exit 0 |

Đầu vào số chỉ gồm chữ số thập phân không dấu, không chấp nhận số âm, dấu +, khoảng trắng, chuỗi rỗng, số thực hoặc hậu tố. Tuần 2 dùng chuyển đổi có end pointer và kiểm tra ERANGE, giới hạn kiểu đích; không dùng atoi. Kiểm tra trước phép nhân `P*K` bằng phép chia giới hạn (xử lý K=0), kiểm tra ID uint64_t, kích thước mảng và các bộ đếm gồm `P*K+C` STOP và tổng số thao tác `2*(P*K+C)` để không tràn. Kiểm tra giới hạn semaphore của môi trường đích và mọi cấp phát trước tạo luồng. Chuyển ms sang thời gian ngủ bằng chia/lấy dư, kiểm tra kiểu thời gian, xử lý EINTR khi cần.

Đề xuất từ chối cờ lạ, cờ lặp, thiếu giá trị, đối số vị trí hoặc giá trị ngoài giới hạn: thông báo lỗi cụ thể ra stderr, exit 2 và chưa tạo worker. Lỗi tài nguyên/runtime: exit 1; chỉ exit 0 khi mọi xác minh và join thành công. Không bổ sung chế độ chạy vô hạn. Chưa thử các quy tắc này vì parser chưa triển khai.

## Schema log đề xuất

Mỗi sự kiện thao tác thành công gồm: `op_seq` (toàn cục từ 1), `op=ENQUEUE|DEQUEUE`, `actor=P0|C0|MAIN`, `kind=DATA|STOP`, `data_id` (số hoặc `-` với STOP), `value`, `enqueue_seq` (thứ tự enqueue gắn với item), `count_after`, `capacity`. `enqueue_seq` có thể lưu trong ô metadata riêng của context, không đổi struct item đã thống nhất. Với DEQUEUE phải lấy metadata cùng ô trước khi cập nhật head. Log có thể thêm dequeue_seq riêng; không dùng timestamp làm chứng cứ FIFO.

Ví dụ minh họa schema, **không phải log thực chạy**:

```text
op_seq=1 op=ENQUEUE actor=P0 kind=DATA data_id=0 value=7 enqueue_seq=1 count_after=1 capacity=8
op_seq=2 op=DEQUEUE actor=C0 kind=DATA data_id=0 value=7 enqueue_seq=1 count_after=0 capacity=8
```

Tăng số thứ tự và chụp record ngay sau push/pop thành công **trong guard**, gồm cả STOP. Có thể in/lưu record sau nhả guard nhưng phải đối chiếu theo op_seq/enqueue_seq đã chụp; thứ tự dòng in ngoài khóa không chứng minh FIFO. Tuần 2 cần chọn cách lưu hữu hạn, kiểm tra tràn/kích thước và báo lỗi nếu không lưu đủ bằng chứng. Quiet chỉ bỏ việc in chi tiết, không bỏ dữ liệu xác minh hoặc biến lỗi thành thành công.

Tổng kết đề xuất: cấu hình thực chạy, DATA enqueue/dequeue, tần suất ID thiếu/lặp/ngoài phạm vi, FIFO theo hai dãy thao tác trong guard, min/max count (kể cả trạng thái đầu 0), STOP enqueue/dequeue, số worker create/join và count cuối 0. Không chỉ so tổng số item.

## Bằng chứng chờ đầy/trống

Tuần 2 có thể dùng `sem_trywait(empty|full)` trước `sem_wait`: thành công thì đã lấy một quyền và không wait thêm; EINTR thử lại; EAGAIN ghi sự kiện `WAIT_SLOT`/`WAIT_ITEM` rồi gọi sem_wait thử lại EINTR. Lỗi khác chuyển xử lý lỗi. EAGAIN chứng minh tại thời điểm thử chưa có quyền, không chứng minh sem_wait sau đó chắc chắn bị block hoặc count luôn bằng N/0 (có quyền đang đặt trước). Không đếm thời gian block chính xác từ sự kiện này.

Không lấy quyết định an toàn từ đọc count không khóa hoặc `sem_getvalue` riêng lẻ. Muốn ghi sự kiện `BUFFER_FULL`/`BUFFER_EMPTY`, ghi snapshot count=N/0 **trong guard** gắn với thao tác thành công gây trạng thái ấy; dùng EAGAIN để chứng minh thiếu quyền. Không giữ guard trong lúc chờ empty/full hoặc sleep. Phương án này chưa triển khai/kiểm thử; cần Châu xem xét cùng hướng xử lý lỗi semaphore.
