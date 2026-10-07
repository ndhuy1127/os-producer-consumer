# Mẫu thiết kế

Trạng thái: **Chưa có thiết kế được nhóm xác nhận**. Điền nội dung cùng bằng chứng sau khi thực hiện Tuần 1; đây là mẫu, không phải mã đã triển khai.

## 1. Bài toán và cấu hình

- Mô tả producer, consumer và mục tiêu kết thúc hữu hạn: chưa điền.
- Sức chứa N, số producer/consumer, tổng số phần tử hay số phần tử mỗi producer, đơn vị độ trễ: chưa chốt.
- Cú pháp CLI, mặc định, giới hạn số và xử lý đầu vào lỗi: chưa chốt.

## 2. Bộ đệm vòng FIFO

| Nội dung | Cần mô tả |
| --- | --- |
| Cấu trúc | Mảng, kiểu dữ liệu, head/tail/count và quy tắc wrap-around |
| Bất biến | Số phần tử trong [0, N], enqueue/dequeue trong vùng bảo vệ |
| API | Khởi tạo, thêm/lấy, kiểm tra đầy/trống và hủy |
| Bộ nhớ/lỗi | Quyền sở hữu, cấp phát, dọn tài nguyên và lỗi khởi tạo |

Chưa có triển khai hoặc kiểm thử tuần tự.

## 3. Dữ liệu và thứ tự

Chốt cách tạo ID duy nhất cho từng phần tử, phân biệt dữ liệu và tín hiệu dừng, đếm sản xuất/tiêu thụ và xác thực tập ID. FIFO phải so thứ tự **enqueue đã thực hiện trong vùng bảo vệ** với thứ tự dequeue; thứ tự tạo ID hoặc in log ngoài vùng bảo vệ có thể khác khi nhiều luồng.

## 4. Semaphore và vùng tới hạn

Điền vai trò, giá trị ban đầu và thứ tự wait/post cho từng semaphore. Các tên empty/full và cơ chế bảo vệ truy cập bộ đệm là gợi ý để nhóm thảo luận, chưa phải lựa chọn đã nghiệm thu. Mô tả xử lý lỗi, EINTR, tránh deadlock và dọn tài nguyên sau join.

## 5. Giả mã

Chưa điền. Cần mô tả main, producer và consumer; vị trí đọc/sửa bộ đệm, ghi thứ tự thao tác, độ trễ và xử lý lỗi. Không giữ vùng tới hạn trong thời gian ngủ nếu không có lý do thiết kế.

## 6. Kết thúc hữu hạn

Chưa chốt phương án. Cần giải thích khi nào không còn producer, consumer đang chờ được đánh thức thế nào, số lượng tín hiệu dừng (nếu dùng), dữ liệu còn trong bộ đệm được tiêu thụ ra sao và tất cả luồng được join trước hủy semaphore/bộ nhớ. Tín hiệu điều khiển không tính vào số phần tử dữ liệu.

## 7. Log và xác minh

Chốt schema log, cấu hình chạy, thông tin tổng kết, kiểm tra không mất/lặp, FIFO, biên occupancy và timeout của bài kiểm thử. Xem [test-plan.md](test-plan.md).

## 8. Tài liệu lý thuyết đã đối chiếu

Chưa có nguồn/trang/mục được thực sự đối chiếu. Khi bổ sung ghi tên tài liệu, URL/ấn bản, trang hoặc mục và nội dung đã kiểm chứng; không commit toàn bộ PDF giáo trình.
