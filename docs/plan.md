# Kế hoạch 4 tuần

Đây là kế hoạch và phân công, không phải bằng chứng đã hoàn thành. Chưa có ngày bắt đầu/hạn nộp. Trạng thái thực tế ở [progress.md](progress.md); báo cáo chung ở [reports/README.md](reports/README.md).

| Tuần | Châu | Huy | Mốc dự kiến |
| --- | --- | --- | --- |
| 1 — Chuẩn bị và thiết kế | Lý thuyết luồng/semaphore; giả mã đồng bộ; thiết kế kết thúc; ví dụ tạo và chờ luồng | Môi trường C; Makefile; bộ đệm vòng thử tuần tự; tham số; kế hoạch kiểm thử | Thiết kế, bộ đệm thử tuần tự và môi trường biên dịch hoạt động |
| 2 — Bản cơ bản | Producer/consumer; semaphore; tích hợp; kết thúc hữu hạn | Bộ đệm; tham số; log; kiểm thử đầy/trống và sức chứa 1 | 1 producer/1 consumer đúng, không mất/lặp dữ liệu, tự kết thúc |
| 3 — Nhiều luồng và kiểm thử | Nhiều producer/consumer; mã dữ liệu riêng; rà soát đồng bộ; phần lý thuyết/thuật toán báo cáo | Script kiểm thử; xác thực dữ liệu/FIFO; bảng kết quả; hướng dẫn chạy | Bản nhiều luồng được kiểm thử và báo cáo nháp |
| 4 — Hoàn thiện | Bài toán, lý thuyết, thiết kế, thuật toán; trình bày nguyên lý/demo | Kết quả kiểm thử, minh chứng, hướng dẫn sử dụng, gói nộp | Mã nguồn, hướng dẫn, kết quả kiểm thử, báo cáo và slide hoàn chỉnh |

Tuần 4, cả hai đọc chéo code, luyện demo và chuẩn bị trả lời câu hỏi. Mỗi tuần chỉ xác nhận mốc khi có file/code, lệnh và kết quả kiểm tra thực tế. Việc có Makefile ở khung ban đầu chưa chứng minh môi trường C hoặc bộ đệm đã hoàn thành.

Ca kiểm thử dự kiến được mô tả ở [test-plan.md](test-plan.md). Thiết kế và lựa chọn chưa chốt dùng [design.md](design.md). Không có chương trình thì không đánh dấu PASS.
