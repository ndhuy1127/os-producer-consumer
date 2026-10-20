# os-producer-consumer

Bài tập lớn môn Hệ điều hành: **Xây dựng chương trình giải quyết bài toán Người sản xuất – Người tiêu thụ với bộ đệm giới hạn bằng luồng và semaphore trên Linux**.

Giảng viên: **Nguyễn Quang Minh**.

| Thành viên | Mã sinh viên | Nhánh làm việc |
| --- | --- | --- |
| Lê Hải Châu | 20233283 | `dev/chau` |
| Nguyễn Đức Huy | 20233448 | `dev/huy` |

## Sản phẩm

Chương trình dòng lệnh C11 dùng POSIX Threads và POSIX semaphore: P producer và C consumer (1..64 mỗi loại) trao đổi dữ liệu qua bộ đệm vòng FIFO N chỗ, đồng bộ bằng ba semaphore `empty`/`full`/`guard`; mỗi producer sinh K phần tử có ID riêng p·K+j; main gửi một tín hiệu STOP cho mỗi consumer để mọi luồng tự kết thúc. Chương trình ghi lại mọi thao tác trong vùng găng và tự kiểm chứng sau khi chạy: không mất/lặp dữ liệu, FIFO, bộ đệm không tràn/không đọc rỗng, mọi luồng được join.

## Trạng thái

Kế hoạch 4 tuần, chưa có ngày bắt đầu/hạn nộp. Chi tiết ở [kế hoạch](docs/plan.md), [tiến độ](docs/progress.md) và [báo cáo tuần](docs/reports/README.md).

| Tuần | Nội dung | Trạng thái |
| --- | --- | --- |
| 1 | Lý thuyết, thiết kế, ví dụ luồng (Châu); môi trường, bộ đệm tuần tự, đặc tả CLI/log (Huy) | Hoàn thành, PR #1 và #2 |
| 2 | Lõi 1 producer/1 consumer với semaphore và STOP (Châu); CLI, log, kiểm thử đầy/rỗng/N=1 (Huy) | Hoàn thành, PR #3 merge ở `d125734` |
| 3 | Nhiều producer/consumer, ID riêng, rà soát đồng bộ, lý thuyết/thuật toán (phần Châu); script kiểm thử, xác thực FIFO, bảng kết quả, hướng dẫn chạy (phần Huy) | Hoàn thành trên `dev/huy`, **chưa PR**; do Claude Code thực hiện theo yêu cầu Huy, **Châu chưa review** |
| 4 | Hoàn thiện báo cáo, slide, demo, đọc chéo, gói nộp | Chưa bắt đầu |

Kết quả Tuần 3 (20/10/2026): 16/16 preset lõi, 32/32 ca tiêm lỗi, 30/30 lần chạy chương trình chính, 140/140 lần chạy lặp nhiều luồng; ASan/UBSan và ThreadSanitizer không cảnh báo. Xem [minh chứng](results/week3/README.md), [bảng kết quả](results/week3/results-table.md) và [báo cáo nháp](docs/final-report/bao-cao-nhap.md).

## Chạy nhanh

Trong Ubuntu hoặc WSL2, từ thư mục gốc:

```sh
sudo apt install build-essential python3
make
./bin/producer-consumer --capacity 3 --producers 3 --consumers 2 --items-per-producer 100
make test          # bộ đệm, lõi, tiêm lỗi, CLI/log (~16 s)
make test-stress   # chạy lặp các cấu hình nhiều luồng (~18 s)
```

| Tham số | Mặc định | Giới hạn |
| --- | --- | --- |
| `--capacity N` | 8 | 1..65536 |
| `--producers P` / `--consumers C` | 1 / 1 | 1..64 |
| `--items-per-producer K` | 20 | 0..1000000, P·K ≤ 1000000 |
| `--producer-delay-ms` / `--consumer-delay-ms` | 0 / 0 | 0..60000 ms |
| `--quiet` | tắt | Không in từng thao tác, vẫn kiểm chứng và in tổng kết |

Mã thoát: 0 thành công, 1 lỗi runtime/kiểm chứng, 2 tham số sai. Cách đọc log, toàn bộ lệnh kiểm thử và sự cố thường gặp: **[hướng dẫn chạy và kiểm thử](docs/run-guide.md)**. Ví dụ tạo/join luồng Tuần 1: `make thread-demo && ./bin/thread-demo`.

## Cấu trúc

| Đường dẫn | Vai trò |
| --- | --- |
| [src/](src/README.md), [include/](include/README.md) | Bộ đệm, lõi đồng bộ, CLI, log, main |
| [tests/](tests/README.md) | Test bộ đệm, lõi, tiêm lỗi, formatter, main |
| [scripts/](scripts/README.md) | Script kiểm thử, chạy lặp, ghi minh chứng, kiểm tra khung |
| [examples/thread_lifecycle.c](examples/thread_lifecycle.c) | Ví dụ tạo/join 3 luồng (Tuần 1) |
| [Makefile](Makefile) | Build chương trình, ví dụ, test; `make help` |
| [docs/design.md](docs/design.md) | Thiết kế Tuần 1–3 |
| [docs/theory-week1-chau.md](docs/theory-week1-chau.md) | Lý thuyết luồng, vùng găng, semaphore, bế tắc |
| [docs/run-guide.md](docs/run-guide.md) | Hướng dẫn chạy và kiểm thử |
| [docs/test-plan.md](docs/test-plan.md) | Ca kiểm thử T01–T09 và trạng thái |
| [docs/buffer-week1-huy.md](docs/buffer-week1-huy.md), [docs/config-log-week1-huy.md](docs/config-log-week1-huy.md), [docs/handoff-week2-chau.md](docs/handoff-week2-chau.md) | Hợp đồng bộ đệm, đặc tả CLI/log, bàn giao API Tuần 2 |
| [docs/plan.md](docs/plan.md), [docs/progress.md](docs/progress.md), [docs/reports/](docs/reports/README.md) | Kế hoạch, tiến độ, báo cáo tuần |
| [docs/final-report/](docs/final-report/README.md), [docs/slides/](docs/slides/README.md) | Báo cáo cuối kỳ (đang có bản nháp), slide |
| [results/week1/](results/week1/README.md) – [week4/](results/week4/README.md) | Log và kết quả thực chạy làm minh chứng |
| [.gitignore](.gitignore), [.gitattributes](.gitattributes), [.github/PULL_REQUEST_TEMPLATE.md](.github/PULL_REQUEST_TEMPLATE.md) | Bỏ qua build/secret/`AGENTS.md`, ép dòng LF, mẫu PR |

`AGENTS.md` là hướng dẫn cho trợ lý AI, chỉ để local; không commit và không đưa vào gói nộp.
