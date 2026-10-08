# Minh chứng Tuần 1

Đã ghép tài liệu, ví dụ luồng Châu và bộ đệm tuần tự Huy; kiểm tra bản tích hợp mới chạy trên **máy Châu**. Đánh giá hiện tại ở [group-validation.md](group-validation.md), trạng thái bàn giao xem [PR #2](https://github.com/ndhuy1127/os-producer-consumer/pull/2).

| File | Thời điểm/phạm vi |
| --- | --- |
| [group-integration.log](group-integration.log) | Lệnh/đầu ra/exit/hash kiểm tra bản chung trên máy Châu, sau xử lý xung đột |
| [group-validation.md](group-validation.md) | Kết quả tích hợp, quyết định giao diện, giới hạn và cách demo |
| [chau-environment.log](chau-environment.log) | Môi trường và Git của Châu trước phần W1-C |
| [chau-thread-demo.log](chau-thread-demo.log) | Log lịch sử W1-C: create/join, clean/build lại, tiêm lỗi create; khi đó make test còn trả 2 |
| [chau-validation.md](chau-validation.md) | Snapshot đánh giá độc lập trước tích hợp, trạng thái chờ khi đó không phải trạng thái hiện tại |
| [huy-environment.log](huy-environment.log) | Môi trường máy Huy và ví dụ Châu đúng SHA/hash chạy ngoài repo |
| [huy-buffer-test.log](huy-buffer-test.log) | Log lịch sử W1-H: 12 ca tuần tự, sanitizer 11 ca thường, hash nguồn/Git |
| [huy-validation.md](huy-validation.md) | Snapshot đánh giá độc lập của Huy trước tích hợp |
| [scaffold-check.log](scaffold-check.log) | Log lịch sử khởi tạo, không dùng để suy ra trạng thái build/test hiện tại |

Giữ nguyên byte các log cũ. Makefile đã đổi sau tích hợp: make test hiện chạy bộ đệm tuần tự, make mặc định vẫn trả 2 do thiếu src/main.c. Hash ví dụ/bộ đệm dùng lại được đối chiếu; hash Makefile cũ không đại diện Makefile chung mới. Log nhóm ghi hai SHA nền và hash file đang kiểm tra trước commit; SHA tích hợp/merge thật chỉ xác nhận sau khi tồn tại.

Không lưu binary/object/cache/secret/AGENTS.md/file tạm. T01 tuần tự và ví dụ luồng được đánh giá riêng; T02–T09 chưa chạy. Xem [báo cáo](../../docs/reports/week1.md).
