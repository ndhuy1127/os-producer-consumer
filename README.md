# os-producer-consumer

Bài tập lớn môn Hệ điều hành: **Xây dựng chương trình giải quyết bài toán Người sản xuất – Người tiêu thụ với bộ đệm giới hạn bằng luồng và semaphore trên Linux**.

Giảng viên: **Nguyễn Quang Minh**.

| Thành viên | Mã sinh viên | Nhánh làm việc |
| --- | --- | --- |
| Lê Hải Châu | 20233283 | `dev/chau` |
| Nguyễn Đức Huy | 20233448 | `dev/huy` |

## Mục tiêu và trạng thái

Sản phẩm dự kiến là chương trình dòng lệnh bằng C, dùng POSIX Threads và POSIX semaphore: bộ đệm vòng FIFO, một hoặc nhiều producer/consumer, cấu hình sức chứa/số luồng/số phần tử/độ trễ, log hoạt động, tổng kết kiểm tra dữ liệu và kết thúc hữu hạn.

Tuần 1 độc lập của Huy đã có [bộ đệm vòng tuần tự](docs/buffer-week1-huy.md), 12 ca kiểm thử thực chạy và [đề xuất cấu hình/log](docs/config-log-week1-huy.md). Môi trường Ubuntu/WSL và ví dụ tạo/join luồng của Châu đã được kiểm tra trên máy Huy; [minh chứng](results/week1/huy-validation.md) ghi rõ nguồn và phạm vi. **Chưa triển khai chương trình Producer–Consumer, semaphore hoặc CLI.** [PR #1 của Châu](https://github.com/ndhuy1127/os-producer-consumer/pull/1) còn mở lúc kiểm tra; nhánh này chưa chứa lý thuyết/giả mã/ví dụ của PR đó. API chi tiết và mốc chung Tuần 1 còn cần đọc chéo, xác nhận của hai thành viên. Kế hoạch kéo dài 4 tuần; chưa có ngày bắt đầu hoặc hạn nộp. Xem [kế hoạch](docs/plan.md), [tiến độ](docs/progress.md) và [báo cáo tuần](docs/reports/README.md).

## Môi trường và build

Môi trường phát triển/demo: Ubuntu trong WSL; có thể dùng VS Code. Trong Ubuntu, chuẩn bị công cụ:

```sh
sudo apt update
sudo apt install build-essential git python3
cc --version
make --version
```

Từ Git root trong Ubuntu:

```sh
make help
make buffer-test
make test-buffer
make test
```

`make buffer-test` tạo `bin/buffer-test`; `make test-buffer` và `make test` chạy **kiểm thử bộ đệm tuần tự**, kỳ vọng `Sequential buffer tests: 12/12 PASS`, exit 0. Cờ build: `-std=c11 -Wall -Wextra -Wpedantic`, chuẩn bị `-pthread` ở compile/link. Test dùng GNU ld wrapper malloc/free để tiêm lỗi cấp phát và kiểm tra thu hồi mảng; không thêm wrapper vào chương trình chính.

Kiểm tra lại từ bản build sạch:

```sh
make clean
make buffer-test
make test-buffer
```

`make` mặc định vẫn báo chương trình chưa triển khai và exit 2 khi chưa có `src/main.c`, kể cả khi đã có `src/buffer.c`. Khi có entry point thật, target chính sẽ build `src/*.c` vào `bin/producer-consumer`. Chưa có lệnh chạy thí nghiệm chương trình chính. Khi tích hợp PR #1 phải giữ cả `thread-demo` của Châu và `buffer-test`/`test-buffer` của Huy; nhánh này chưa có target thread-demo. Cách chạy ví dụ đúng nguồn từ thư mục tạm ở [minh chứng môi trường](results/week1/huy-environment.log).

Kiểm tra khung quản lý với Python 3 nếu có:

```sh
python3 scripts/check_scaffold.py
```

Kiểm tra này chỉ xác nhận cấu trúc, liên kết và quy tắc Git, không nghiệm thu thuật toán.

## Cấu trúc

| Đường dẫn | Vai trò |
| --- | --- |
| `README.md` | Giới thiệu, môi trường và quy trình Git chung |
| `AGENTS.md` (chỉ local) | Hướng dẫn Codex; không commit, push hoặc đưa vào gói nộp |
| [.gitignore](.gitignore) | Bỏ file build, cache, file tạm, secret và mọi `AGENTS.md` |
| [.gitattributes](.gitattributes) | Giữ dòng LF để Makefile và script dùng được khi clone từ Windows sang WSL |
| [Makefile](Makefile) | Build/test bộ đệm tuần tự, chuẩn bị `-pthread` và dọn build |
| [src/](src/README.md), [include/](include/README.md) | API bộ đệm tuần tự và item DATA/STOP |
| [tests/](tests/README.md) | Kiểm thử FIFO, biên, vòng đời và lỗi cấp phát |
| [scripts/](scripts/README.md) | Kiểm tra khung, script kiểm thử và demo |
| [docs/plan.md](docs/plan.md) | Kế hoạch 4 tuần và phân công |
| [docs/design.md](docs/design.md) | Mẫu thiết kế bộ đệm, đồng bộ và cách dừng |
| [docs/test-plan.md](docs/test-plan.md) | Ca kiểm thử dự kiến và tiêu chí |
| [docs/progress.md](docs/progress.md) | Nhiệm vụ, người phụ trách, trạng thái và minh chứng |
| [docs/reports/](docs/reports/README.md) | Hướng dẫn và báo cáo chung tuần 1–4 |
| [docs/final-report/](docs/final-report/README.md) | Báo cáo cuối kỳ khi có |
| [docs/slides/](docs/slides/README.md) | Slide khi có |
| [results/week1/](results/week1/README.md) – [week4/](results/week4/README.md) | Kết quả, log và ảnh thực tế cần lưu làm minh chứng |
| [.github/PULL_REQUEST_TEMPLATE.md](.github/PULL_REQUEST_TEMPLATE.md) | Mẫu PR và bằng chứng kiểm tra |

