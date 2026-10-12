# os-producer-consumer

Bài tập lớn môn Hệ điều hành: **Xây dựng chương trình giải quyết bài toán Người sản xuất – Người tiêu thụ với bộ đệm giới hạn bằng luồng và semaphore trên Linux**.

Giảng viên: **Nguyễn Quang Minh**.

| Thành viên | Mã sinh viên | Nhánh làm việc |
| --- | --- | --- |
| Lê Hải Châu | 20233283 | `dev/chau` |
| Nguyễn Đức Huy | 20233448 | `dev/huy` |

## Mục tiêu và trạng thái

Sản phẩm dự kiến là chương trình dòng lệnh bằng C, dùng POSIX Threads và POSIX semaphore: bộ đệm vòng FIFO, một hoặc nhiều producer/consumer, cấu hình sức chứa/số luồng/số phần tử/độ trễ, log hoạt động, tổng kết kiểm tra dữ liệu và kết thúc hữu hạn.

Tuần 1 đã tích hợp [lý thuyết](docs/theory-week1-chau.md), [thiết kế chung](docs/design.md), [ví dụ tạo/join 3 luồng](examples/thread_lifecycle.c), [bộ đệm FIFO tuần tự](docs/buffer-week1-huy.md), 12 ca test và [đặc tả cấu hình/log](docs/config-log-week1-huy.md). [Minh chứng Tuần 1](results/week1/group-validation.md) và log lịch sử được giữ nguyên; PR #2 đã merge ở nền `9cf345c`.

**W2-C của Lê Hải Châu đạt 100% phạm vi 1 producer/1 consumer:** ba semaphore, STOP hữu hạn, kiểm chứng ID/FIFO/biên và xử lý lỗi; [minh chứng Châu](results/week2/chau-validation.md) giữ nguyên. `dev/huy` đã fast-forward nhận bàn giao `bd00780` từ origin/dev/chau trước khi ghép phần Huy.

**W2-H đạt 100% phạm vi Tuần 2:** giữ buffer đã kiểm chứng, hoàn thiện CLI/kiểm tra đầu vào, log production và kiểm tra qua chương trình chính ở thường/quiet. Codex thực hiện theo yêu cầu trên nhánh Huy, trên máy ChouPro Ubuntu/WSL ngày 12/10/2026; không ghi Huy trực tiếp chạy hoặc phê duyệt. [Minh chứng Huy](results/week2/huy-validation.md), [báo cáo chung](docs/reports/week2.md) và [tiến độ](docs/progress.md) xác nhận **mốc kỹ thuật bản cơ bản Tuần 2 đạt trên dev/huy**, chưa merge/bàn giao main. Nhiều producer/consumer thuộc Tuần 3. Kế hoạch kéo dài 4 tuần; chưa có ngày bắt đầu/hạn nộp.

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
make all thread-demo
timeout 5s ./bin/producer-consumer
timeout 5s ./bin/thread-demo
make test
make help
```

`make` build `bin/producer-consumer` với C11, `-Wall -Wextra -Wpedantic`, `-pthread` ở compile/link. Không đối số dùng đúng `(N,P,C,K,Dp_ms,Dc_ms)=(8,1,1,20,0,0)`. Tổng kết chỉ PASS khi `pc_run` trả 0 và kiểm chứng đạt: mỗi ID đúng một lần, FIFO, count cuối 0, một STOP và cả hai worker join. Mã thoát: **0** thành công/help; **1** tài nguyên/runtime/kiểm chứng/output lỗi; **2** tham số/cấu hình lỗi. Lỗi đồng bộ/join không thể phục hồi kết thúc toàn tiến trình với 1.

`make test` chạy 12 ca buffer, tám ca lõi, 26 ca lỗi Châu, và kiểm tra CLI/log production (16 lần thường/quiet, 116 đầu vào sai cùng wrapper, help, biên parser và lỗi main/log). `make test-cli` chỉ chạy nhóm CLI/log; `make test-buffer` chỉ chạy bộ đệm. Wrapper chỉ liên kết vào binary test; production không có hook. `make clean` dọn build/bin; main test/ví dụ tách chương trình chính.

## CLI Tuần 2 và demo

```sh
./bin/producer-consumer --help
./bin/producer-consumer --capacity 5 --producers 1 --consumers 1 --items-per-producer 20 --producer-delay-ms 0 --consumer-delay-ms 0
./bin/producer-consumer --capacity 1 --items-per-producer 100 --quiet
timeout 7s ./bin/producer-consumer --capacity 2 --items-per-producer 40 --producer-delay-ms 0 --consumer-delay-ms 20
timeout 7s ./bin/producer-consumer --capacity 2 --items-per-producer 40 --producer-delay-ms 20 --consumer-delay-ms 0
./bin/producer-consumer --capacity 1 --items-per-producer 0
```

| Cờ | Mặc định | Phạm vi hiện hỗ trợ |
| --- | --- | --- |
| --capacity | 8 | 1..65536, còn giới hạn semaphore/kích thước |
| --producers / --consumers | 1 / 1 | Chỉ 1P/1C Tuần 2; >1 báo chưa hỗ trợ, exit 2 |
| --items-per-producer | 20 | 0..1000000, K=0 hợp lệ |
| --producer-delay-ms / --consumer-delay-ms | 0 / 0 | 0..60000 millisecond |
| --quiet | tắt | Bỏ record chi tiết; giữ oracle, kiểm chứng và toàn bộ tổng kết |
| --help | — | Dùng riêng, exit 0 và không chạy worker |

Chỉ nhận dạng `--cờ giá_trị`, **không hỗ trợ `--cờ=giá_trị`**. Số chỉ gồm chữ số ASCII 0..9; từ chối âm/dấu +/whitespace/rỗng/thực/hậu tố/tràn, cờ lạ/lặp/thiếu và đối số vị trí. `--help` kết hợp cờ khác bị từ chối. N=5/K=20 là ca nguồn riêng, không đổi mặc định N=8. Nhiều luồng trong đặc tả dài hạn chưa được triển khai.

Chạy riêng các cấu hình nghiệm thu qua cùng lõi (tên preset của test runner, không phải CLI sản phẩm):

```sh
make pc-test pc-error-test
timeout 5s ./bin/pc-test source-N5 --records   # N=5, K=20 theo kế hoạch nguồn
timeout 7s ./bin/pc-test T04 --records        # N=2, K=40, consumer 20ms
timeout 7s ./bin/pc-test T05 --records        # N=2, K=40, producer 20ms
timeout 15s ./bin/pc-test all
python3 scripts/check_pc_errors.py
make test-cli
python3 scripts/validate_week2_huy.py         # minh chứng huy-*, gồm sanitizer
```

Record được lõi chụp trong guard; logger production in theo op_seq **sau khi chạy xong và join**, không phải log streaming theo thời gian thực. Mỗi record có ENQUEUE/DEQUEUE, P0/C0/MAIN, DATA/STOP, ID hoặc `-`, value, enqueue_seq, count_after/capacity và wait_reason. `WAIT_SLOT/WAIT_ITEM` chứng minh từng gặp EAGAIN; snapshot count=N/0 chứng minh đầy/trống riêng, không suy ra thời gian block kernel. Quiet giữ `records=2*(K+1)` và toàn bộ kiểm chứng/tổng kết.

Script W2-H ghi thời gian Asia/Saigon và hash nguồn vào huy-*, không sửa Tuần 1/chau-*. **Không chạy scripts/validate_week2.py trên CLI mới**: đó là writer lịch sử W2-C, ghi đè chau-* và kỳ vọng CLI chưa có. Sanitizer Huy đã đạt với leak detection ngoài sandbox; [log Châu từng lỗi ptrace](results/week2/chau-sanitizer-sandbox.log) giữ nguyên. Xem [đánh giá Huy](results/week2/huy-validation.md) để tái lập.

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
| [Makefile](Makefile) | Build chương trình, ví dụ, test lõi/buffer/wrapper và dọn build |
| [examples/thread_lifecycle.c](examples/thread_lifecycle.c) | Ví dụ Tuần 1 của Châu: tạo/join luồng, đối số và kết quả riêng |
| [src/](src/README.md), [include/](include/README.md) | main, parser, logger, lõi pc và bộ đệm tuần tự |
| [tests/](tests/README.md) | Buffer/lõi, CLI/log, biên parser và wrapper lỗi tách production |
| [scripts/](scripts/README.md) | Kiểm tra khung, script kiểm thử và demo |
| [docs/plan.md](docs/plan.md) | Kế hoạch 4 tuần và phân công |
| [docs/theory-week1-chau.md](docs/theory-week1-chau.md) | Lý thuyết luồng, vùng găng, semaphore, deadlock và nguồn đã đối chiếu |
| [docs/design.md](docs/design.md) | Thiết kế tích hợp Tuần 1 và triển khai semaphore/STOP Tuần 2 |
| [docs/handoff-week2-chau.md](docs/handoff-week2-chau.md) | API và cách Huy ghép CLI/log |
| [docs/buffer-week1-huy.md](docs/buffer-week1-huy.md) | Hợp đồng bộ đệm, lỗi và vòng đời |
| [docs/config-log-week1-huy.md](docs/config-log-week1-huy.md) | Mặc định/giới hạn CLI và schema log đã chốt cho Tuần 2 |
| [docs/test-plan.md](docs/test-plan.md) | Ca kiểm thử dự kiến và tiêu chí |
| [docs/progress.md](docs/progress.md) | Nhiệm vụ, người phụ trách, trạng thái và minh chứng |
| [docs/reports/](docs/reports/README.md) | Hướng dẫn và báo cáo chung tuần 1–4 |
| [docs/final-report/](docs/final-report/README.md) | Báo cáo cuối kỳ khi có |
| [docs/slides/](docs/slides/README.md) | Slide khi có |
| [results/week1/](results/week1/README.md) – [week4/](results/week4/README.md) | Kết quả, log và ảnh thực tế cần lưu làm minh chứng |
| [.github/PULL_REQUEST_TEMPLATE.md](.github/PULL_REQUEST_TEMPLATE.md) | Mẫu PR và bằng chứng kiểm tra |
