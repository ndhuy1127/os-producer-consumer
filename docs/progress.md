# Theo dõi tiến độ

Trạng thái hợp lệ: **Chưa bắt đầu**, **Đang thực hiện**, **Đã hoàn thành**, **Bị vướng**. Thêm ghi chú **Chưa kiểm chứng** khi thiếu kiểm tra. Kế hoạch không tự chứng minh hoàn thành; không suy ra người thực hiện từ Git author.

## Nhiệm vụ học thuật

| ID | Nhiệm vụ dự kiến | Người phụ trách | Tuần | Trạng thái | Minh chứng |
| --- | --- | --- | --- | --- | --- |
| W1-C | Lý thuyết; giả mã đồng bộ; kết thúc; ví dụ tạo/chờ luồng | Châu | 1 | Đã hoàn thành | [Lý thuyết](theory-week1-chau.md), [thiết kế/giả mã](design.md), [ví dụ](../examples/thread_lifecycle.c), [kiểm tra thực chạy](../results/week1/chau-validation.md); đã tích hợp với API thực tế qua Codex theo yêu cầu Châu |
| W1-H | Môi trường C; Makefile; bộ đệm tuần tự; tham số; kế hoạch kiểm thử | Huy | 1 | Đã hoàn thành | [API](buffer-week1-huy.md), [cấu hình/log](config-log-week1-huy.md), [test-plan](test-plan.md), [minh chứng máy Huy](../results/week1/huy-validation.md); bản chung ở [group-validation](../results/week1/group-validation.md) |
| W2-C | Producer/consumer; semaphore; tích hợp; kết thúc hữu hạn | Châu | 2 | Chưa bắt đầu | Chưa có |
| W2-H | Bộ đệm; tham số; log; kiểm thử đầy/trống/N=1 | Huy | 2 | Chưa bắt đầu | Chưa có |
| W3-C | Nhiều luồng; ID riêng; rà soát đồng bộ; lý thuyết/thuật toán báo cáo | Châu | 3 | Chưa bắt đầu | Chưa có |
| W3-H | Script test; xác thực dữ liệu/FIFO; kết quả; hướng dẫn chạy | Huy | 3 | Chưa bắt đầu | Chưa có |
| W4-C | Bài toán; lý thuyết; thiết kế; thuật toán; nguyên lý/demo | Châu | 4 | Chưa bắt đầu | Chưa có |
| W4-H | Kết quả test; minh chứng; hướng dẫn; gói nộp | Huy | 4 | Chưa bắt đầu | Chưa có |
| W4-G | Đọc chéo code; luyện demo; chuẩn bị trả lời câu hỏi | Cả hai | 4 | Chưa bắt đầu | Chưa có |

W1-C và W1-H hoàn thành phần độc lập, minh chứng riêng được giữ nguyên. W1-C đã chạy 5 lần và clean/build/chạy lại, thử lỗi create lần 1/3. W1-H đã kiểm chứng môi trường máy Huy, ví dụ Châu đúng hash, bộ đệm 12/12 và sanitizer 11/11 ca thường. Codex xử lý xung đột và chốt giao diện theo yêu cầu Châu; không tuyên bố Huy phê duyệt hoặc hai người đã đọc chéo trực tiếp.

**Mốc chung kỹ thuật Tuần 1 đã đạt:** giao diện chốt, bản tích hợp build không cảnh báo compiler, ví dụ đủ 3 worker/join, bộ đệm 12/12, khung/liên kết/ignore/diff hợp lệ. Kết quả mới chạy trên máy Châu ghi riêng ở [group-validation](../results/week1/group-validation.md) và [báo cáo](reports/week1.md); không đổi log lịch sử để khớp hành vi mới. Trạng thái bàn giao Git/merge xem [PR #2](https://github.com/ndhuy1127/os-producer-consumer/pull/2), SHA thật xác nhận trong phần trả kết quả sau thao tác. T01 là kiểm thử tuần tự đã có; T02–T09 chưa chạy, thuộc tuần 2 trở đi.

## Thiết lập dự án

Các việc dưới đây do Codex thực hiện theo yêu cầu khởi tạo, không tự gán cho Châu/Huy.

| Nhiệm vụ | Người thực hiện | Giai đoạn | Trạng thái | Minh chứng |
| --- | --- | --- | --- | --- |
| Khung quản lý, kế hoạch, mẫu báo cáo chung Tuần 1–4 | Codex theo yêu cầu | Khởi tạo | Đã hoàn thành | [README](../README.md), [kế hoạch](plan.md), [báo cáo](reports/README.md), log kiểm tra khung bên dưới |
| Kiểm tra cấu trúc/liên kết/ignore và Makefile khung | Codex theo yêu cầu | Khởi tạo | Đã hoàn thành | [Log thực tế](../results/week1/scaffold-check.log) |
| Tạo repository private, giữ nhánh sau merge | Codex theo yêu cầu | Khởi tạo | Đã hoàn thành | [Repository](https://github.com/ndhuy1127/os-producer-consumer); API tạo repo trả private=true và delete_branch_on_merge=false |

Minh chứng lịch sử lần khởi tạo: 23 đường dẫn bắt buộc, 56 liên kết tương đối sau cập nhật tài liệu, quy tắc ignore và không có AGENTS.md trong tracked/staged. Trong Ubuntu/WSL, Makefile chưa có nguồn/test nên trả mã lỗi 2 đúng dự kiến; help/clean chạy được. Mẫu C tạm ngoài repo đã biên dịch/liên kết với -pthread và chạy thành công. Đây là kiểm tra công cụ, không phải nghiệm thu chương trình hoặc phần việc của Châu/Huy.

## Xác nhận Git sau khởi tạo

SHA commit, việc push và ba nhánh được xác nhận trực tiếp sau commit bằng các lệnh dưới đây và phần bàn giao, tránh ghi SHA của chính file này trước khi commit tồn tại:

```sh
git rev-parse main dev/chau dev/huy
git ls-remote --heads origin
git status --short
```

Đoạn trên là hướng xác nhận ở lần khởi tạo; trạng thái hiện tại phải đọc từ Git/GitHub và PR. Không dùng sự tồn tại của tài liệu này để suy ra đã push hoặc merge. Phần trả kết quả bàn giao sẽ ghi SHA thật sau merge và đồng bộ.

Commit và trạng thái remote phải xác nhận bằng Git/GitHub thực tế, không điền SHA giả. Báo cáo tuần ban đầu ghi “Chưa có dữ liệu tiến độ”; danh sách dự kiến ở mục tiêu.
