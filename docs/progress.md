# Theo dõi tiến độ

Trạng thái hợp lệ: **Chưa bắt đầu**, **Đang thực hiện**, **Đã hoàn thành**, **Bị vướng**. Thêm ghi chú **Chưa kiểm chứng** khi thiếu kiểm tra. Kế hoạch không tự chứng minh hoàn thành; không suy ra người thực hiện từ Git author.

## Nhiệm vụ học thuật

| ID | Nhiệm vụ dự kiến | Người phụ trách | Tuần | Trạng thái | Minh chứng |
| --- | --- | --- | --- | --- | --- |
| W1-C | Lý thuyết; giả mã đồng bộ; kết thúc; ví dụ tạo/chờ luồng | Châu | 1 | Đã hoàn thành | [Lý thuyết](theory-week1-chau.md), [thiết kế/giả mã](design.md), [ví dụ](../examples/thread_lifecycle.c), [kiểm tra thực chạy](../results/week1/chau-validation.md); thiết kế chưa được Huy xác nhận |
| W1-H | Môi trường C; Makefile; bộ đệm tuần tự; tham số; kế hoạch kiểm thử | Huy | 1 | Chưa bắt đầu | Chưa có; khung Makefile chưa chứng minh hoàn thành phần việc |
| W2-C | Producer/consumer; semaphore; tích hợp; kết thúc hữu hạn | Châu | 2 | Chưa bắt đầu | Chưa có |
| W2-H | Bộ đệm; tham số; log; kiểm thử đầy/trống/N=1 | Huy | 2 | Chưa bắt đầu | Chưa có |
| W3-C | Nhiều luồng; ID riêng; rà soát đồng bộ; lý thuyết/thuật toán báo cáo | Châu | 3 | Chưa bắt đầu | Chưa có |
| W3-H | Script test; xác thực dữ liệu/FIFO; kết quả; hướng dẫn chạy | Huy | 3 | Chưa bắt đầu | Chưa có |
| W4-C | Bài toán; lý thuyết; thiết kế; thuật toán; nguyên lý/demo | Châu | 4 | Chưa bắt đầu | Chưa có |
| W4-H | Kết quả test; minh chứng; hướng dẫn; gói nộp | Huy | 4 | Chưa bắt đầu | Chưa có |
| W4-G | Đọc chéo code; luyện demo; chuẩn bị trả lời câu hỏi | Cả hai | 4 | Chưa bắt đầu | Chưa có |

W1-C hoàn thành trong phạm vi độc lập: tài liệu, thiết kế và ví dụ tạo/join 3 worker đã chạy 5 lần, rồi clean/build/chạy lại; kết quả 55, 210, 465 đúng, thoát 0. Thử lỗi create ở lần 1 và lần 3 bằng wrapper tạm: chỉ join các worker đã tạo, không báo hoàn tất và thoát 1. W1-H giữ nguyên vì chưa có bằng chứng mới; môi trường máy Huy, bộ đệm tuần tự và việc ghép code chưa kiểm chứng. **Mốc chung Tuần 1 chưa được xác nhận đầy đủ.** Không có ca T01–T09 của Producer–Consumer đã thực hiện. Báo cáo ở [week1](reports/week1.md).

## Thiết lập dự án

Các việc dưới đây do Codex thực hiện theo yêu cầu khởi tạo, không tự gán cho Châu/Huy.

| Nhiệm vụ | Người thực hiện | Giai đoạn | Trạng thái | Minh chứng |
| --- | --- | --- | --- | --- |
| Khung quản lý, kế hoạch, mẫu báo cáo chung Tuần 1–4 | Codex theo yêu cầu | Khởi tạo | Đã hoàn thành | [README](../README.md), [kế hoạch](plan.md), [báo cáo](reports/README.md), log kiểm tra khung bên dưới |
| Kiểm tra cấu trúc/liên kết/ignore và Makefile khung | Codex theo yêu cầu | Khởi tạo | Đã hoàn thành | [Log thực tế](../results/week1/scaffold-check.log) |
| Tạo repository private, giữ nhánh sau merge | Codex theo yêu cầu | Khởi tạo | Đã hoàn thành | [Repository](https://github.com/ndhuy1127/os-producer-consumer); API tạo repo trả private=true và delete_branch_on_merge=false |

Kiểm tra khung: 23 đường dẫn bắt buộc, 56 liên kết tương đối sau cập nhật tài liệu, quy tắc ignore và không có AGENTS.md trong tracked/staged. Trong Ubuntu/WSL, Makefile chưa có nguồn/test nên trả mã lỗi 2 đúng dự kiến; help/clean chạy được. Mẫu C tạm ngoài repo đã biên dịch/liên kết với -pthread và chạy thành công. Đây là kiểm tra công cụ, không phải nghiệm thu chương trình hoặc phần việc của Châu/Huy.

## Xác nhận Git sau khởi tạo

SHA commit, việc push và ba nhánh được xác nhận trực tiếp sau commit bằng các lệnh dưới đây và phần bàn giao, tránh ghi SHA của chính file này trước khi commit tồn tại:

```sh
git rev-parse main dev/chau dev/huy
git ls-remote --heads origin
git status --short
```

Ba SHA nền tảng phải giống nhau ở thời điểm bàn giao. Cấu hình GitHub phải có nhánh mặc định main và repo private. Không dùng sự tồn tại của tài liệu này để suy ra đã push.

Commit và trạng thái remote phải xác nhận bằng Git/GitHub thực tế, không điền SHA giả. Báo cáo tuần ban đầu ghi “Chưa có dữ liệu tiến độ”; danh sách dự kiến ở mục tiêu.
