# os-producer-consumer

Bài tập lớn môn Hệ điều hành: **Xây dựng chương trình giải quyết bài toán Người sản xuất – Người tiêu thụ với bộ đệm giới hạn bằng luồng và semaphore trên Linux**.

Giảng viên: **Nguyễn Quang Minh**.

| Thành viên | Mã sinh viên | Nhánh làm việc |
| --- | --- | --- |
| Lê Hải Châu | 20233283 | `dev/chau` |
| Nguyễn Đức Huy | 20233448 | `dev/huy` |

## Mục tiêu và trạng thái

Sản phẩm dự kiến là chương trình dòng lệnh bằng C, dùng POSIX Threads và POSIX semaphore: bộ đệm vòng FIFO, một hoặc nhiều producer/consumer, cấu hình sức chứa/số luồng/số phần tử/độ trễ, log hoạt động, tổng kết kiểm tra dữ liệu và kết thúc hữu hạn.

Phần Tuần 1 của Châu đã có [lý thuyết](docs/theory-week1-chau.md), [thiết kế và giả mã](docs/design.md) cùng [ví dụ tạo/join 3 luồng](examples/thread_lifecycle.c) đã biên dịch/chạy trên Ubuntu 24.04 trong WSL. Minh chứng ở [results/week1](results/week1/README.md). **Chưa triển khai chương trình Producer–Consumer, bộ đệm vòng, CLI hoặc bộ kiểm thử thuật toán.** Thiết kế/API/log còn cần Huy xác nhận; W1-H và mốc chung Tuần 1 chưa được kiểm chứng đầy đủ. Kế hoạch kéo dài 4 tuần; chưa có ngày bắt đầu hoặc hạn nộp. Xem [kế hoạch](docs/plan.md), [tiến độ](docs/progress.md) và [báo cáo tuần](docs/reports/README.md).

## Môi trường và build

Môi trường phát triển/demo: Ubuntu trong WSL; có thể dùng VS Code. Trong Ubuntu, chuẩn bị công cụ:

```sh
sudo apt update
sudo apt install build-essential git
cc --version
make --version
```

Từ Git root trong Ubuntu:

```sh
make help
make
make test
make clean
```

Hiện `make` mặc định và `make test` vẫn trả mã lỗi 2 và thông báo chưa triển khai chương trình/kiểm thử chính. Khi có nguồn trong `src/*.c`, Makefile biên dịch với `-pthread` và tạo `bin/producer-consumer`; cú pháp tham số sẽ cập nhật sau triển khai.

Build/chạy ví dụ Tuần 1 độc lập của Châu, không cần tham số:

```sh
make thread-demo
timeout 5s ./bin/thread-demo
```

Target này tạo `bin/thread-demo` từ `examples/thread_lifecycle.c`, bật `-Wall -Wextra -Wpedantic` và `-pthread` ở cả biên dịch/liên kết. Ba worker tính tổng 1..10, 1..20, 1..30; main join và kiểm tra kết quả 55, 210, 465, cuối cùng in `completed: all 3 workers joined and verified`, thoát 0. Thứ tự worker chạy có thể thay đổi. Lỗi create/join hoặc kết quả sai khiến ví dụ trả lỗi; `make clean` dọn cả ví dụ lẫn file build. Đây là kiểm tra vòng đời luồng, chưa kiểm thử bộ đệm/semaphore hoặc thuật toán Producer–Consumer.

Kiểm tra khung quản lý với Python 3 nếu có:

```sh
python3 scripts/check_scaffold.py
```

Kiểm tra này chỉ xác nhận cấu trúc, liên kết và quy tắc Git, không nghiệm thu thuật toán.

## Cấu trúc

| Đường dẫn | Vai trò |
| --- | --- |
| `README.md` | Giới thiệu, trạng thái, môi trường và cách build/chạy |
| `AGENTS.md` (chỉ local) | Hướng dẫn Codex; không commit, push hoặc đưa vào gói nộp |
| [.gitignore](.gitignore) | Bỏ file build, cache, file tạm, secret và mọi `AGENTS.md` |
| [.gitattributes](.gitattributes) | Giữ dòng LF để Makefile và script dùng được khi clone từ Windows sang WSL |
| [Makefile](Makefile) | Build ví dụ bằng `make thread-demo`; chuẩn bị chương trình chính và dọn file build |
| [examples/thread_lifecycle.c](examples/thread_lifecycle.c) | Ví dụ Tuần 1 của Châu: tạo/join luồng, đối số và kết quả riêng |
| [src/](src/README.md), [include/](include/README.md) | Nguồn C và header khi triển khai |
| [tests/](tests/README.md) | Kiểm thử bộ đệm và chương trình khi triển khai |
| [scripts/](scripts/README.md) | Kiểm tra khung, script kiểm thử và demo |
| [docs/plan.md](docs/plan.md) | Kế hoạch 4 tuần và phân công |
| [docs/theory-week1-chau.md](docs/theory-week1-chau.md) | Lý thuyết luồng, vùng găng, semaphore, deadlock và nguồn đã đối chiếu |
| [docs/design.md](docs/design.md) | Thiết kế Tuần 1 của Châu: giả mã, semaphore, STOP và API đề xuất cần phối hợp |
| [docs/test-plan.md](docs/test-plan.md) | Ca kiểm thử dự kiến và tiêu chí |
| [docs/progress.md](docs/progress.md) | Nhiệm vụ, người phụ trách, trạng thái và minh chứng |
| [docs/reports/](docs/reports/README.md) | Hướng dẫn và báo cáo chung tuần 1–4 |
| [docs/final-report/](docs/final-report/README.md) | Báo cáo cuối kỳ khi có |
| [docs/slides/](docs/slides/README.md) | Slide khi có |
| [results/week1/](results/week1/README.md) – [week4/](results/week4/README.md) | Kết quả, log và ảnh thực tế cần lưu làm minh chứng |
| [.github/PULL_REQUEST_TEMPLATE.md](.github/PULL_REQUEST_TEMPLATE.md) | Mẫu PR và bằng chứng kiểm tra |
