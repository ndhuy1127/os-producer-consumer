# Nghiệm thu W2-H — Nguyễn Đức Huy

**W2-H đạt 100% công việc Tuần 2, phạm vi chương trình cơ bản 1 producer/1 consumer.** Buffer được rà soát/tích hợp và giữ nguyên; CLI, kiểm tra đầu vào, log production, quiet và kiểm tra N=1/đầy/trống đã hoàn thành bằng chương trình chính. Bản ghép Châu–Huy đạt mốc kỹ thuật Tuần 2 trên dev/huy, chưa merge/bàn giao main.

Ngày kiểm chứng **12/10/2026**, thời gian Asia/Saigon (UTC+07:00) trong từng log. Codex thực hiện theo yêu cầu trên máy **ChouPro**, Ubuntu 24.04.5 LTS/WSL2, kernel 6.18.33.2-microsoft-standard-WSL2, GCC 13.3.0, Make 4.3, Git 2.43.0, Python 3.12.3. Root `/mnt/d/hust/os-producer-consumer`. Không ghi Huy trực tiếp chạy/phê duyệt hoặc hai thành viên đã đọc chéo.

## Git, phần đã đọc và nguồn thực chạy

Ban đầu working tree sạch trên dev/chau ở bd00780; fetch origin thành công, dev/huy và origin/dev/huy cùng nền 9cf345c, origin/dev/chau có thêm một commit W2-C. Chuyển dev/huy rồi **fast-forward** tới `bd00780f115aaf752043735dac417f01054cc11d`; không tạo nhánh/cherry-pick/copy repo, không sửa dev/chau/main, không reset/clean Git/stash/force-push. Không có AGENTS.md local được tìm thấy qua các thư mục repo/cha đã kiểm tra.

Đã đọc README/Makefile/ignore, kế hoạch/tiến độ/báo cáo Tuần 1–2, hợp đồng buffer/cấu hình/log, design/test-plan, bàn giao Châu, nguồn buffer/pc/main, test buffer/lõi/wrapper, hai script Châu và minh chứng Tuần 1/W2-C trước khi sửa. Tuần 1 có buffer FIFO tuần tự 12 ca và thiết kế; W2-C có semaphore, STOP, record và summary sau join. Phần thiếu là parser/log production và nghiệm thu qua CLI. Không tìm thấy lỗi buffer/lõi cần sửa, giữ nguyên pc.c/pc.h và API của Huy; main được thay để ghép parser/logger đúng vòng đời.

Nguồn kiểm tra là **HEAD nền bd00780 cộng working tree W2-H trước commit**, không gán CLI/log mới cho commit nền. [huy-source.sha256](huy-source.sha256) ghi 22 file nguồn/header/build/test/script thực dùng, digest manifest `8f9d91afab4d444a5eea871511c16edfd9395bc06550b1264be5427ae3b6d4b9`. Header mọi log Huy ghi HEAD nền/branch/môi trường/hash; kiểm tra sau test xác nhận nguồn không đổi. Commit/push xác nhận từ Git sau thao tác, trong phần trả kết quả để tránh SHA tự tham chiếu.

Minh chứng results/week1 và mọi chau-* giữ nguyên byte. Manifest Châu là snapshot cũ (Makefile/main đã thay khi ghép Huy), không dùng checksum Châu để nghiệm thu toàn bộ bản mới. Dùng manifest Huy và diff bảo toàn lõi/historical evidence. **Không chạy scripts/validate_week2.py** trên CLI mới vì nó ghi đè chau-* và kỳ vọng đối số bị từ chối.

## Hoàn thành theo phân công

| Công việc Huy | Kết quả thực |
| --- | --- |
| Hoàn thiện buffer | Rà hợp đồng zero-init/sở hữu/copy/STOP, giữ API buffer_init/push/pop/destroy và đồng bộ ngoài buffer; 12/12 ca chạy lại đạt |
| Đọc tham số/kiểm tra sai | cli.c/cli.h, defaults (8,1,1,20,0,0), strtoumax/end/errno/ERANGE/kiểu; kiểm tra tràn DATA+STOP/records/kích thước và semaphore trước lõi |
| Log mã luồng/item/count | runtime_log.c/runtime_log.h dùng records sau join, đầy đủ schema và summary; PRIu64/%zu; STOP ID=-; quiet vẫn kiểm chứng |
| N=1/producer nhanh/consumer nhanh | T03/T04/T05 qua binary production ở cả thường và quiet, có đầy/trống và thiếu quyền thực |
| Minh chứng và bàn giao | Script riêng chỉ huy-*, lệnh/đầu ra/exit/env/time/hash, hồi quy, sanitizer, README/design/test-plan/progress/báo cáo sáu phần |

Main parse → help riêng hoặc result zero-init → pc_run → kiểm tra status/verified → log → pc_result_destroy → return. Không sao chép result sở hữu; không I/O trong guard/khóa thêm; lỗi có phục hồi vẫn destroy records. PASS chỉ khi status==PC_OK và verified, kể cả quiet.

## Build và hồi quy

| Lệnh thực | Kết quả | Exit |
| --- | --- | --- |
| make clean | Dọn build/bin, không git clean | 0 |
| make all thread-demo buffer-test pc-test pc-error-test cli-tests | C11/-Wall/-Wextra/-Wpedantic, -pthread compile/link, không cảnh báo, main test/ví dụ riêng | 0 |
| make help | Hướng dẫn đúng CLI/test/validation mới | 0 |
| timeout 5s ./bin/thread-demo | 55/210/465, ba create/join và completed | 0 |
| make test-buffer | Buffer tuần tự 12/12 | 0 |
| make test-pc | Tám preset lõi Châu PASS, timeout 15s | 0 |
| make test-errors | 26/26 ca lỗi/API Châu, timeout mỗi child 5s | 0 |
| make test (target tổng bản ghép) | Hồi quy và suite CLI/log production PASS, không cảnh báo | 0 |

[huy-build.log](huy-build.log), [huy-regression-tests.log](huy-regression-tests.log). Build thử ban đầu gặp future-mtime trên DrvFS; script chuẩn hóa timestamp nguồn/file build trước và sau clean/build, giữ bytes, ghi từng thao tác. Build/help/target tổng cuối không cảnh báo. Không sửa log Châu/Tuần 1 để che kết quả cũ.

## Runtime qua CLI thật

Mỗi ca dùng **bin/producer-consumer**. Ca default không đối số; ca còn lại truyền đủ sáu cờ số, chạy thêm --quiet. Python subprocess đặt timeout `5+ceil(2*K*(Dp_ms+Dc_ms)/1000)` giây. [huy-runtime-tests.log](huy-runtime-tests.log) ghi toàn bộ lệnh/stdout/stderr/exit, [huy-test-summary.json](huy-test-summary.json) ghi số quan sát thực.

| Ca | N/P/C/K/Dp_ms/Dc_ms | Thường / quiet | Quan sát thường | Timeout | Exit thường/quiet |
| --- | --- | --- | --- | --- | --- |
| default | 8/1/1/20/0/0 | PASS / PASS | 20 DATA, STOP 1/1 | 5s | 0/0 |
| source-N5 | 5/1/1/20/0/0 | PASS / PASS | Ca nguồn, 20 DATA | 5s | 0/0 |
| T02 | 8/1/1/100/0/0 | PASS / PASS | 100 ID đúng một lần | 5s | 0/0 |
| T03 | 1/1/1/100/0/0 | PASS / PASS | 101 snapshot đầy và 101 rỗng, gồm STOP | 5s | 0/0 |
| T04 | 2/1/1/40/0/20 | PASS / PASS | 40 snapshot đầy; 39 WAIT_SLOT, gồm 38 P0 và 1 MAIN | 7s | 0/0 |
| T05 | 2/1/1/40/20/0 | PASS / PASS | 41 snapshot rỗng; 40 WAIT_ITEM | 7s | 0/0 |
| K0 | 1/1/1/0/0/0 | PASS / PASS | 0 DATA; STOP 1/1, consumer kết thúc | 5s | 0/0 |
| queued-stop | 8/1/1/20/0/20 | PASS / PASS | Còn 8 DATA sau producer join, tiêu thụ đủ trước STOP | 6s | 0/0 |

Ca thường dùng **oracle queue tuyến tính từ stdout**, không mô phỏng modulo/head/tail của buffer: kiểm tra đủ schema/op_seq/actor/kind/ID/value/enqueue_seq, đối chiếu từng enqueue/dequeue theo thao tác thực, tần suất đúng một lần, count=số enqueue trừ số dequeue trong [0,N], cuối 0. STOP đúng 1/1, ID=-, không vào Counter DATA; DATA processed=K và create/join 1P/1C. Summary được đối chiếu với record, không chỉ tổng số/ID tăng.

Quiet không in record nên không có oracle FIFO độc lập từ stdout ở các lần quiet. Kiểm tra summary/verified/status, đủ DATA/STOP/joins, lỗi bằng 0 và record_count=2*(K+1) được giữ; lõi kiểm chứng vẫn chạy nguyên mã. Các ca tiêm lỗi main/log ở quiet xác nhận không biến lỗi thành PASS. Không đòi số chờ giống thường vì lịch chạy khác.

WAIT_SLOT/WAIT_ITEM chỉ chứng minh EAGAIN tại lần probe; snapshot count=N/0 trong guard chứng minh đầy/trống riêng, **không khẳng định thời gian block kernel**. Số quan sát trong bảng thuộc lần chạy lưu, không phải hằng số mọi lịch chạy. T04 đòi có WAIT_SLOT của P0; T05 có WAIT_ITEM C0, không suy đoán từ delay.

## CLI sai, help, biên và lỗi log/main

[huy-cli-tests.log](huy-cli-tests.log): **116 đầu vào sai**, mỗi đầu vào chạy trên production và binary cùng main có wrapper chặn pc_run/pthread_create (232 child), đều exit **2**, stdout rỗng/stderr cụ thể; không gọi lõi/worker. Positive control đầu vào hợp lệ bị trap pc_run và exit **99**, chứng minh hook hoạt động; không chỉ suy ra từ thiếu log. Bao gồm cờ lạ/lặp/thiếu/vị trí, N/P/C=0, P/C>1, âm/+1/whitespace/rỗng/float/hậu tố/Unicode/tràn, N=65537/K=1000001/delay=60001, help kết hợp cờ/quiet lặp/thừa giá trị và dạng = không hỗ trợ.

Help dùng riêng trên production và hook exit **0**, có đủ cờ/giới hạn/codes và không chạy lõi. Năm ca parser-only: defaults, lower bounds, upper bounds (65536/1/1/1000000/60000/60000), leading zeros và help, exit **0**. Không chạy worker/khẳng định runtime ở biên lớn có thể kéo dài nhiều giờ.

[huy-log-tests.log](huy-log-tests.log): formatter **7/7 PASS, exit 0**, thử uint64_t cực lớn/STOP ID=-/quiet/status=1+verified=1 vẫn FAIL/verified=0 vẫn FAIL/output error. Fixture formatter là dữ liệu dựng để thử định dạng, không phải minh chứng FIFO hay DATA thật. Sáu ca cùng main production với wrapper (init-error/cleanup-error/unverified × thường/quiet) exit **1** đúng dự kiến, không PASS và result_destroy records_before=1/after=0. Cleanup-error báo sem_destroy guard lỗi sau join; core verified=1 nhưng status=1, logger vẫn FAIL. Đây là lỗi tiêm có chủ đích, không tuyên bố OS thật lỗi hoặc Huy trực tiếp chạy.

## Sanitizer, rà soát và phạm vi

[huy-sanitizers.log](huy-sanitizers.log): ngoài sandbox, ASan/UBSan với leak detection giữ bật (`detect_leaks=1:halt_on_error=1`, `halt_on_error=1:print_stacktrace=1`), C11/cảnh báo/-pthread. Sáu runtime đại diện default/T03 quiet/T04/T05/K0 quiet/default quiet, help, CLI sai, biên parser lớn, formatter 7 ca và hai đường init/cleanup lỗi main quiet. Các ca bình thường/build exit 0; parse sai exit 2 và hook lỗi exit 1 đúng dự kiến, không diagnostics sanitizer/leak. Chạy ngoài sandbox vì lần W2-C đã xác nhận LSan không hoạt động dưới ptrace; không tắt leak detection để tuyên bố PASS.

[huy-review.log](huy-review.log) ghi scaffold/liên kết/diff-check, bảo toàn byte Tuần 1/chau-*/lõi/buffer/tests/writer Châu. Báo cáo đủ sáu phần, phần Châu và dòng W2-C giữ nguyên. Chỉ code/tài liệu/minh chứng cần thiết được bàn giao, không AGENTS/PDF/Word/binary/object/cache/secret/temp/meeting.

T02–T05 đã đạt cả lõi và CLI production. T08 chỉ phạm vi CLI Tuần 2 đang hỗ trợ 1P/1C, không nghiệm thu giới hạn runtime nhiều luồng. T06/T07 chưa làm; K0/queued-stop không đánh dấu toàn T09 nhiều luồng PASS. Mốc kỹ thuật bản cơ bản đạt trên dev/huy; merge/main và nghiệm thu nhiều luồng là công việc tiếp theo.

Tái lập từ Git root trên dev/huy: `make test`, hoặc `python3 scripts/validate_week2_huy.py` để ghi lại huy-* và sanitizer; `sha256sum -c results/week2/huy-source.sha256` để đối chiếu nguồn đã chạy. Writer Huy ghi đè huy-* khi chạy lại, giữ nguyên Tuần 1/chau-*.
