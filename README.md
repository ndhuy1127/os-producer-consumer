# os-producer-consumer

Bài tập lớn môn Hệ điều hành: **Xây dựng chương trình giải quyết bài toán Người sản xuất – Người tiêu thụ với bộ đệm giới hạn bằng luồng và semaphore trên Linux**.

Giảng viên: **Nguyễn Quang Minh**.

| Thành viên | Mã sinh viên | Nhánh làm việc |
| --- | --- | --- |
| Lê Hải Châu | 20233283 | `dev/chau` |
| Nguyễn Đức Huy | 20233448 | `dev/huy` |

## Mục tiêu và trạng thái

Sản phẩm dự kiến là chương trình dòng lệnh bằng C, dùng POSIX Threads và POSIX semaphore: bộ đệm vòng FIFO, một hoặc nhiều producer/consumer, cấu hình sức chứa/số luồng/số phần tử/độ trễ, log hoạt động, tổng kết kiểm tra dữ liệu và kết thúc hữu hạn.

Hiện có khung quản lý dự án, Makefile và mẫu tài liệu. **Chưa có chương trình C, bộ kiểm thử thuật toán hoặc kết quả demo.** Các nhiệm vụ học thuật chưa được xác nhận hoàn thành. Kế hoạch kéo dài 4 tuần; chưa có ngày bắt đầu hoặc hạn nộp. Xem [kế hoạch](docs/plan.md), [tiến độ](docs/progress.md) và [báo cáo tuần](docs/reports/README.md).

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

Ở khung hiện tại, `make` và `make test` trả mã lỗi và thông báo chưa triển khai. Khi có nguồn trong `src/*.c`, Makefile biên dịch với `-pthread` và tạo `bin/producer-consumer`. Khi đó có thể chạy `./bin/producer-consumer`; cú pháp tham số sẽ được cập nhật sau khi triển khai. Chưa có lệnh chạy thí nghiệm hoặc kiểm thử chương trình chính thức.

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
| [Makefile](Makefile) | Chuẩn bị build C với `-pthread` và dọn file build |
| [src/](src/README.md), [include/](include/README.md) | Nguồn C và header khi triển khai |
| [tests/](tests/README.md) | Kiểm thử bộ đệm và chương trình khi triển khai |
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

