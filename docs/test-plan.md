# Kế hoạch kiểm thử

Trạng thái: **Dự kiến; chưa có chương trình hoặc bộ kiểm thử**. Chưa có ca PASS. Chốt CLI và cấu hình chính xác sau [thiết kế](design.md).

| ID | Tình huống dự kiến | Tiêu chí cần quan sát | Trạng thái |
| --- | --- | --- | --- |
| T01 | Bộ đệm chạy tuần tự: enqueue/dequeue, wrap-around | Dữ liệu đúng, FIFO, occupancy không vượt N | Chưa bắt đầu |
| T02 | 1 producer / 1 consumer | Mọi ID được tiêu thụ đúng một lần; tự kết thúc | Chưa bắt đầu |
| T03 | N = 1 | Không mất/lặp; occupancy chỉ 0/1; không deadlock | Chưa bắt đầu |
| T04 | Producer nhanh, consumer chậm để bộ đệm đầy | Producer chờ khi đầy, không ghi vượt N; tiếp tục sau dequeue | Chưa bắt đầu |
| T05 | Consumer nhanh, producer chậm để bộ đệm trống | Consumer chờ khi trống, không đọc rỗng; tiếp tục sau enqueue | Chưa bắt đầu |
| T06 | Nhiều producer/consumer, số lượng khác nhau | ID duy nhất; tập dữ liệu khớp; FIFO theo enqueue thực tế; mọi luồng kết thúc | Chưa bắt đầu |
| T07 | Nhiều dữ liệu và chạy lặp lại | Không mất/lặp, không treo; lưu cấu hình và số lần lặp thực tế | Chưa bắt đầu |
| T08 | CLI thiếu/sai, số âm, 0, quá lớn hoặc tràn số | Từ chối theo quy tắc đã chốt; báo lỗi và mã thoát phù hợp | Chưa bắt đầu |
| T09 | Kết thúc khi consumer đang chờ; còn dữ liệu cần tiêu thụ | Tiêu thụ hết dữ liệu; mọi luồng join; tín hiệu dừng không làm sai thống kê | Chưa bắt đầu |

## Cách đánh giá khi đã triển khai

- Dữ liệu: đối chiếu tập ID sản xuất/tiêu thụ và tần suất; chỉ tổng số bằng nhau chưa đủ chứng minh không mất/lặp.
- FIFO: đối chiếu thứ tự enqueue/dequeue trong vùng bảo vệ; không dùng thứ tự in log hoặc thứ tự sinh ID như thứ tự FIFO toàn cục.
- Giới hạn: quan sát occupancy sau mỗi thao tác; luôn 0 ≤ count ≤ N.
- Kết thúc: đặt timeout có lý do theo cấu hình và kiểm tra mọi luồng kết thúc; timeout chưa chốt.
- Full/empty: có bằng chứng thực sự đạt trạng thái cần kiểm tra, không chỉ thay đổi độ trễ rồi suy đoán.
- Lưu lệnh/cấu hình, môi trường, commit được kiểm tra, đầu vào, kết quả quan sát và mã thoát vào results/weekN/. Không bịa log hoặc đánh dấu PASS khi chưa chạy.

Kiểm tra khung bằng `python3 scripts/check_scaffold.py` và thông báo chưa triển khai của Makefile được ghi riêng ở [progress.md](progress.md); không thay các ca thuật toán trên.
