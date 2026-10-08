# os-producer-consumer

Bài tập lớn môn Hệ điều hành: **Xây dựng chương trình giải quyết bài toán Người sản xuất – Người tiêu thụ với bộ đệm giới hạn bằng luồng và semaphore trên Linux**.

Giảng viên: **Nguyễn Quang Minh**.

| Thành viên | Mã sinh viên | Nhánh làm việc |
| --- | --- | --- |
| Lê Hải Châu | 20233283 | `dev/chau` |
| Nguyễn Đức Huy | 20233448 | `dev/huy` |

## Mục tiêu và trạng thái

Sản phẩm dự kiến là chương trình dòng lệnh bằng C, dùng POSIX Threads và POSIX semaphore: bộ đệm vòng FIFO, một hoặc nhiều producer/consumer, cấu hình sức chứa/số luồng/số phần tử/độ trễ, log hoạt động, tổng kết kiểm tra dữ liệu và kết thúc hữu hạn.

Đã tích hợp phần Tuần 1 của Châu và Huy: [lý thuyết](docs/theory-week1-chau.md), [thiết kế/giả mã chung](docs/design.md), [ví dụ tạo/join 3 luồng](examples/thread_lifecycle.c), [bộ đệm vòng tuần tự](docs/buffer-week1-huy.md), 12 ca kiểm thử và [đặc tả cấu hình/log](docs/config-log-week1-huy.md). Codex rà soát và chốt giao diện theo yêu cầu Châu; không ghi nhận Huy đã phê duyệt hoặc hai thành viên đã đọc chéo trực tiếp. **Bản chung đã kiểm tra đạt trên máy Châu**, đủ tiêu chí kỹ thuật Tuần 1; [minh chứng tích hợp](results/week1/group-validation.md) ghi kết quả/phạm vi, trạng thái bàn giao Git xem [PR #2](https://github.com/ndhuy1127/os-producer-consumer/pull/2). Chưa triển khai producer/consumer, lớp semaphore, CLI parser hoặc log runtime; các phần đó thuộc Tuần 2. Kế hoạch kéo dài 4 tuần; chưa có ngày bắt đầu/hạn nộp. Xem [tiến độ](docs/progress.md) và [báo cáo tuần](docs/reports/week1.md).

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
make clean
make thread-demo buffer-test
timeout 5s ./bin/thread-demo
make test
make help
```

`make buffer-test` build `bin/buffer-test`; `make test-buffer` và `make test` chạy kiểm thử bộ đệm **tuần tự**, kỳ vọng `Sequential buffer tests: 12/12 PASS`, exit 0. Test dùng wrapper malloc/free để thử lỗi cấp phát và theo dõi thu hồi mảng; wrapper chỉ liên kết vào binary test. Không dùng kết quả này để nghiệm thu Producer–Consumer/semaphore.

`make` mặc định vẫn trả 2 khi chưa có entry point thật `src/main.c`, kể cả khi `src/buffer.c` tồn tại. Khi có main, target chính chỉ dùng `src/*.c`, không ghép main của ví dụ hoặc test. `make clean` dọn build/bin của cả hai target.

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
| [Makefile](Makefile) | Build thread-demo/buffer-test, chạy test-buffer/test tuần tự và dọn build |
| [examples/thread_lifecycle.c](examples/thread_lifecycle.c) | Ví dụ Tuần 1 của Châu: tạo/join luồng, đối số và kết quả riêng |
| [src/](src/README.md), [include/](include/README.md) | Bộ đệm tuần tự, kiểu item DATA/STOP; chưa có main |
| [tests/](tests/README.md) | 12 ca kiểm thử bộ đệm tuần tự |
| [scripts/](scripts/README.md) | Kiểm tra khung, script kiểm thử và demo |
| [docs/plan.md](docs/plan.md) | Kế hoạch 4 tuần và phân công |
| [docs/theory-week1-chau.md](docs/theory-week1-chau.md) | Lý thuyết luồng, vùng găng, semaphore, deadlock và nguồn đã đối chiếu |
| [docs/design.md](docs/design.md) | Thiết kế tích hợp Tuần 1: API thực tế, semaphore và STOP cho Tuần 2 |
| [docs/buffer-week1-huy.md](docs/buffer-week1-huy.md) | Hợp đồng bộ đệm, lỗi và vòng đời |
| [docs/config-log-week1-huy.md](docs/config-log-week1-huy.md) | Mặc định/giới hạn CLI và schema log đã chốt cho Tuần 2 |
| [docs/test-plan.md](docs/test-plan.md) | Ca kiểm thử dự kiến và tiêu chí |
| [docs/progress.md](docs/progress.md) | Nhiệm vụ, người phụ trách, trạng thái và minh chứng |
| [docs/reports/](docs/reports/README.md) | Hướng dẫn và báo cáo chung tuần 1–4 |
| [docs/final-report/](docs/final-report/README.md) | Báo cáo cuối kỳ khi có |
| [docs/slides/](docs/slides/README.md) | Slide khi có |
| [results/week1/](results/week1/README.md) – [week4/](results/week4/README.md) | Kết quả, log và ảnh thực tế cần lưu làm minh chứng |
| [.github/PULL_REQUEST_TEMPLATE.md](.github/PULL_REQUEST_TEMPLATE.md) | Mẫu PR và bằng chứng kiểm tra |
