# Nghiệm thu W2-C — Lê Hải Châu

**W2-C đạt 100% phạm vi Tuần 2: 1 producer/1 consumer, ba semaphore, tích hợp và kết thúc hữu hạn.** Đã chạy chương trình thật và kiểm tra dữ liệu, FIFO, occupancy, STOP, vòng đời và đường lỗi. Codex thực hiện theo yêu cầu Châu trên máy Châu; không xác nhận Huy phê duyệt hoặc toàn nhóm hoàn thành Tuần 2.

Ngày thực chạy **12/10/2026**, timestamp Asia/Saigon (UTC+07:00) trong log. Môi trường thực: Ubuntu 24.04.5 LTS trên WSL2, kernel `6.18.33.2-microsoft-standard-WSL2`, GCC 13.3.0, GNU Make 4.3, Git 2.43.0, Python 3.12.3. Root `/mnt/d/HUST/os-producer-consumer`, nhánh `dev/chau`. Đã fetch origin; dev/chau, origin/dev/chau và origin/main đều ở `9cf345c12ca59fb1a6bf476147551cb89f7e043b`, không cần fast-forward. Working tree ban đầu sạch, không reset/clean Git/stash, không đổi dev/huy/main.

Đã đọc lại các tài liệu/code Tuần 1 được yêu cầu trước khi sửa. Tái sử dụng nguyên byte item/buffer/test-buffer/thread-demo và minh chứng Tuần 1; không thêm đồng bộ bên trong API buffer. Không tìm thấy kế hoạch Word hoặc Modern Operating Systems liên quan trong repo, attachments và danh sách tài liệu dưới /mnt/d/HUST đã kiểm tra; không tuyên bố đọc giáo trình/Hình 2-28. Phân công nguồn trong yêu cầu đủ để thực hiện W2-C.

## Nguồn thực chạy và tái lập

Đây là **working tree mới trước commit trên HEAD nền `9cf345c…`**, không phải chương trình ở commit nền đó. [Manifest SHA256](chau-source.sha256) lưu hash 13 file build/C/header/test/script thực dùng, digest manifest `b4dde74335815fdc922975efe8c293576c3169d26b1bc85d0d4b352a9445730a`. Header các log lần cuối ghi cùng HEAD nền, branch, môi trường và digest; kiểm tra cuối xác nhận source manifest không đổi trong quá trình chạy. Riêng lần LSan lỗi ban đầu dùng [manifest lưu riêng](chau-sandbox-source.sha256), khác script ghi minh chứng trước khi sửa chuẩn hóa mtime, nguồn C/header/test giống bản cuối. Commit bàn giao và push được xác nhận từ Git sau khi commit tồn tại, trong phần trả kết quả, tránh SHA tự tham chiếu.

```sh
python3 scripts/validate_week2.py
```

Lệnh trên clean/build, kiểm tra Tuần 1, chạy riêng tám preset với timeout, 26 ca lỗi/API và sanitizer. Exit **0** ở lần cuối ngoài sandbox. Script sẽ ghi đè minh chứng Tuần 2 khi chạy lại; không sửa Tuần 1. Muốn đối chiếu đúng nguồn đã kiểm tra: `sha256sum -c results/week2/chau-source.sha256`.

## Kết quả build và Tuần 1

| Lệnh thực chạy | Kết quả | Exit |
| --- | --- | --- |
| make clean | Dọn build/bin trước build sạch | 0 |
| make all thread-demo buffer-test pc-test pc-error-test | C11, -Wall -Wextra -Wpedantic; -pthread compile/link; không cảnh báo; main của test/ví dụ tách production | 0 |
| make help | Đủ hướng dẫn chương trình, test và clean | 0 |
| timeout 5s ./bin/thread-demo | Ba worker/join, 55/210/465; completed cuối cùng | 0 |
| make test-buffer | Bộ đệm tuần tự 12/12 PASS, wrapper cấp phát chỉ trong test | 0 |
| make test (target tổng sau rà soát) | 12 ca buffer + 8 ca lõi + 26 ca lỗi PASS, không cảnh báo | 0 |
| python3 scripts/check_scaffold.py; git diff --check; git diff --cached --check | Khung/liên kết/ignore/whitespace hợp lệ | 0 |
| git diff --exit-code HEAD -- [buffer/header/test/ví dụ/Minh chứng Tuần 1/báo cáo Tuần 1] | Các file cũ giữ nguyên | 0 |

Chi tiết lệnh/đầu ra ở [chau-build.log](chau-build.log), [chau-review.log](chau-review.log). Build thử ban đầu có lỗi cú pháp comma cho cờ linker wrapper; đã sửa trước build sạch. WSL/mnt/d từng báo future-mtime/clock skew cả ở nguồn lẫn file build mới sinh; script chuẩn hóa timestamp trước build và sau build trước khi make đọc lại dependencies, **không đổi bytes**. Lần cuối clean/build/help/test-buffer không cảnh báo. Python host thiếu tzdata nên script dùng timezone UTC+07:00 chuẩn thư viện thay cho tra database; không cần cài thêm gói.

## Tám cấu hình runtime

Mỗi dòng chạy `./bin/pc-test <preset> [--records]` qua `timeout` với ngân sách `5+ceil(2*K*(Dp_ms+Dc_ms)/1000)` giây. Main production chạy thêm không đối số; CLI chưa ghép. Đầu ra/record/lệnh/exit ở [chau-runtime-tests.log](chau-runtime-tests.log).

| Preset | N/P/C/K/Dp_ms/Dc_ms | DATA enqueue/dequeue | Quan sát cuối | Timeout | Exit |
| --- | --- | --- | --- | --- | --- |
| default | 8/1/1/20/0/0 | 20/20 | PASS | 5s | 0 |
| source-N5 | 5/1/1/20/0/0 | 20/20 | PASS — ca theo kế hoạch nguồn | 5s | 0 |
| T02 | 8/1/1/100/0/0 | 100/100 | PASS | 5s | 0 |
| T03 | 1/1/1/100/0/0 | 100/100 | PASS, 101 snapshot đầy/101 rỗng (gồm STOP) | 5s | 0 |
| T04 | 2/1/1/40/0/20 | 40/40 | PASS, 40 snapshot đầy, 39 WAIT_SLOT | 7s | 0 |
| T05 | 2/1/1/40/20/0 | 40/40 | PASS, 41 snapshot rỗng, 41 WAIT_ITEM | 7s | 0 |
| K0 | 1/1/1/0/0/0 | 0/0 | PASS, STOP 1/1, WAIT_ITEM=1 | 5s | 0 |
| queued-stop | 8/1/1/20/0/20 | 20/20 | PASS, 8 DATA còn đợi sau producer join, đủ DATA trước STOP | 6s | 0 |

**Mỗi ca** đã kiểm tra bằng oracle độc lập: mọi ID 0..K-1 được enqueue/dequeue đúng một lần; không thiếu/lặp/ngoài phạm vi; kind/id/value và FIFO khớp dãy enqueue thực trong guard; enqueue_seq cùng item; count sau từng thao tác khớp số thêm trừ số lấy và trong [0,N]; cuối 0; STOP enqueue/dequeue đúng một, không tính DATA; hai worker create/join thành công; đủ DATA processed và kết thúc trong timeout. Oracle dùng queue tuyến tính, không sao chép công thức modulo/head/tail của buffer. Lõi cũng xác minh tần suất, metadata FIFO và hai dãy item sau join.

T04/T05/queued-stop có record thao tác thực đầy đủ. EAGAIN được gắn vào record sau khi lấy guard; **không coi EAGAIN là bằng chứng sem_wait đã block hay count ngay lúc đó là N/0**. Snapshot count dưới guard chứng minh đầy/trống riêng. Số lần chờ/snapshot phụ thuộc lịch chạy, số ở bảng là lần thực lưu trong log, không phải hằng số bắt buộc mọi lần chạy.

`timeout 5s ./bin/producer-consumer` mặc định PASS, exit 0. Gọi `--capacity 5` hiện bị từ chối rõ CLI còn chờ, exit 2; đây là xác nhận giới hạn chương trình hiện tại, không nghiệm thu parser/T08.

## Kiểm tra lỗi và vòng đời

`python3 scripts/check_pc_errors.py`: **26/26 PASS, exit 0**, mỗi tiến trình con timeout 5s; [lệnh/đầu ra](chau-error-tests.log). Wrapper GNU ld chỉ liên kết `bin/pc-error-test`, production không có test hook.

| Nhóm/ca | Kết quả thực tế | Exit của tiến trình test / mã pc_run |
| --- | --- | --- |
| wait-eintr | EINTR một lần ở sem_wait quyền và guard, thử lại thành công, DATA/STOP/joins đúng | 0 / 0 |
| trywait-eintr | EINTR ở sem_trywait rồi tiếp tục thành công | 0 / 0 |
| producer-create | Consumer đã vào đường sem_wait(full) sau EAGAIN; producer create tiêm EAGAIN; STOP/join consumer, final=0, không rò cấp phát | 0 / 1 đúng dự kiến |
| consumer-create | Không tạo/join producer khi consumer create lỗi; 0 worker, dọn đủ | 0 / 1 đúng dự kiến |
| init-empty/full/guard | Chỉ destroy 0/1/2 semaphore init thành công; không worker, free đủ | 0 mỗi ca / 1 |
| malloc và 4 ca calloc | Init/cấp phát lỗi trước create; free đủ mọi mảng đã cấp phát | 0 mỗi ca / 1 |
| destroy | Lỗi sem_destroy báo rõ sau cả hai join, phiên chạy trả lỗi | 0 / 1 |
| 6 ca config API sai | N=0/65537, P=2, C=2, K=SIZE_MAX, delay=60001: trả 2 trước init/create | 0 mỗi ca / 2 |
| 7 ca fatal guard/wait/trywait/post/join/push/pop | Báo tên lời gọi; kết thúc tiến trình, không treo; buffer dùng tên enum riêng | 1 mỗi ca, đúng dự kiến |

Các ca có phục hồi kiểm tra số create==join, init thành công==destroy, không destroy trước join, live_allocations=0 sau destroy result. Ca producer-create dùng semaphore riêng trong wrapper để báo consumer đã vào đường chờ, không dùng sleep/busy-wait; không khẳng định đo được khoảng thời gian block kernel. Các lỗi đều được tiêm có chủ đích, không tuyên bố OS thật hết tài nguyên. Ca fatal chỉ chứng minh tiến trình thoát lỗi kịp thời, không tuyên bố cleanup có phối hợp. Khả năng hoàn trả token trước thay đổi buffer được rà code; không tuyên bố mọi nhánh fatal đã kiểm chứng chính xác số token.

## Sanitizer và giới hạn kết luận

[chau-sanitizers.log](chau-sanitizers.log): GCC với `-fsanitize=address,undefined -fno-omit-frame-pointer`, `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`, `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`, C11/cảnh báo/-pthread. Tám preset lõi, main mặc định và 11 ca buffer thường đều exit 0, không diagnostics. Wrapper không bật ở bản sanitizer. Lần đầu trong sandbox LeakSanitizer báo không hoạt động dưới ptrace, exit 1; giữ nguyên [log đó](chau-sanitizer-sandbox.log), lần kiểm tra cuối chạy ngoài sandbox thành công, không tắt leak detection để ghi PASS.

T06–T08 chưa triển khai/kiểm thử. K0 và queued-stop chỉ nghiệm thu kết thúc cơ bản Tuần 2 1P/1C, **chưa PASS toàn bộ T09** có nhiều consumer/producer. Chưa ghép parser/help/quiet và trình bày runtime log của Huy; record và API đã sẵn ở [bàn giao](../../docs/handoff-week2-chau.md). W2-H và mốc chung Tuần 2 chưa xác nhận hoàn thành. Test hữu hạn không chứng minh mọi lịch chạy hoặc đầu vào; phạm vi W2-C yêu cầu đã có code, chạy thực và minh chứng đầy đủ.
