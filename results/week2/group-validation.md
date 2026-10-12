# Review và nghiệm thu bản ghép Tuần 2

**Bản ghép W2-C + W2-H đạt tiêu chí kỹ thuật Tuần 2 trong phạm vi 1 producer/1 consumer.** Codex review code/diff/minh chứng và thực chạy kiểm tra ngày 12/10/2026 (Asia/Saigon), theo yêu cầu người dùng trên checkout ChouPro WSL. Không ghi Châu/Huy trực tiếp review hoặc phê duyệt. Trạng thái merge/bàn giao main được xác nhận trên GitHub tại [PR #3](https://github.com/ndhuy1127/os-producer-consumer/pull/3), không suy ra từ tài liệu trước merge.

Nguồn thực chạy là commit `7c356e9908ae126579526a79804a112b33275c0f`, chứa W2-C `bd00780f115aaf752043735dac417f01054cc11d` và W2-H. [group-source.sha256](group-source.sha256) ghi 22 file build/C/header/test/script; digest `8f9d91afab4d444a5eea871511c16edfd9395bc06550b1264be5427ae3b6d4b9`, khớp manifest Huy. Thay đổi sau kiểm tra chỉ là tài liệu/minh chứng review; không gán nội dung tài liệu mới cho commit nguồn cũ. [group-review.log](group-review.log) lưu lệnh, môi trường, cấu hình, đầu ra và mã thoát thực tế.

Môi trường: Ubuntu 24.04.5 LTS/WSL2, kernel `6.18.33.2-microsoft-standard-WSL2`, GCC 13.3.0, Make 4.3, Git 2.43.0, Python 3.12.3. Git root `/mnt/d/HUST/os-producer-consumer`, nhánh review `dev/huy`, working tree ban đầu sạch. Fetch xác nhận main/origin/main ở 9cf345c, dev/chau/origin/dev/chau ở bd00780, dev/huy/origin/dev/huy ở 7c356e9; không có công việc mới hoặc PR mở. Không tìm thấy AGENTS.md local/cha. GitHub cho phép merge commit, giữ nhánh sau merge; main không có branch protection hoặc ruleset bắt buộc tại thời điểm kiểm tra.

## Kết quả review

- `empty=N`, `full=0`, `guard=1`, pshared=0; lấy quyền empty/full trước guard. Mọi truy cập buffer/metadata/oracle/record dùng chung khi worker còn chạy đều trong guard; tạo item, xử lý và delay ở ngoài.
- sem_wait/sem_trywait/nanosleep thử lại EINTR; kiểm tra lỗi cấp phát, buffer, semaphore và pthread. Lỗi có phục hồi dọn đúng tài nguyên; lỗi đồng bộ/join nghiêm trọng kết thúc toàn tiến trình với 1, tránh treo hoặc giải phóng storage có worker sống.
- Main điều phối qua pc_run: join producer → chụp DATA pending trong guard → enqueue STOP bằng cơ chế thường → join consumer → xác minh → cleanup. Context sở hữu buffer/metadata/oracle/semaphore, result sở hữu records, logger chỉ mượn sau join, main destroy result kể cả lỗi có phục hồi.
- CLI kiểm tra ký tự số, ERANGE/end pointer, kiểu/giới hạn/tràn, cờ lạ/lặp/thiếu trước pc_run/worker. K=0 hợp lệ; P/C>1 bị từ chối rõ trong Tuần 2.
- Logger lấy record trong guard, in sau join; quiet giữ oracle/records/tổng kết. PASS cần đồng thời status=0 và verified. FIFO đối chiếu hai dãy item và enqueue_seq theo thao tác thực, không chỉ ID hoặc tổng số.

Không phát hiện lỗi runtime cần sửa trong phạm vi được yêu cầu. Đã sửa mô tả tài liệu còn nói chưa review/chưa bàn giao, tham chiếu PR thật làm nguồn trạng thái merge; ghi rõ tài liệu bàn giao Châu là snapshot trước CLI. Giữ đóng góp W2-C/W2-H và mọi minh chứng lịch sử riêng.

## Kiểm tra thực chạy

| Lệnh/nhóm | Kết quả | Exit |
| --- | --- | --- |
| make clean | Dọn build/bin trước build sạch | 0 |
| make all thread-demo buffer-test pc-test pc-error-test cli-tests | Đủ production/ví dụ/test; C11/-Wall/-Wextra/-Wpedantic/-pthread, không cảnh báo | 0 |
| make help | Hướng dẫn CLI/test hiện tại | 0 |
| timeout 5s ./bin/thread-demo | 3 worker/join; 55/210/465; completed | 0 |
| make test — buffer | 12/12 PASS, lỗi/lifecycle/FIFO/wrap/N1/STOP/cấp phát | 0 tổng |
| make test — lõi | 8/8 preset PASS, oracle tuyến tính độc lập | 0 tổng |
| make test — lỗi/API | 26/26 PASS, EINTR/init/create/cleanup/config và fatal có deadline | 0 tổng |
| make test — CLI/log | 16 production; 116 đầu vào sai × production/hook; help 2; 5 biên parser; formatter 7; main errors 6 | 0 tổng |
| python3 scripts/check_scaffold.py | Cấu trúc/liên kết/ignore và không tracked/staged AGENTS.md đạt | 0 |
| git diff --check; git diff --cached --check | Whitespace hợp lệ | 0 mỗi lệnh |
| sha256sum -c huy-source.sha256; group-source.sha256 | 22/22 khớp nguồn thực chạy | 0 mỗi lệnh |
| Diff và hash minh chứng cũ | 31 file results/week1, chau-*, huy-* và báo cáo Tuần 1 nguyên byte | 0 / assertions đạt |

Chỉ chạy một lần toàn bộ make test trên nguồn đã review. Timeout ngoài cho suite 90s; test lõi 15s; từng ca lỗi/API 5s. Ca tiêm fatal và main lỗi exit 1 đúng dự kiến; đầu vào CLI sai exit 2; positive control trap lõi exit 99. Không coi các exit này là suite thất bại. DrvFS tạo build output với timestamp đi trước WSL; đã chuẩn hóa mtime file build, không đổi bytes, trước khi make đọc lại dependencies; chi tiết trong log.

Tám cấu hình production đều được suite chạy ở thường và quiet, lệnh/cờ được liệt kê trong log. Mỗi lần đúng DATA/STOP/count/vòng đời và status=0/verified; ca thường có oracle FIFO từ stdout, quiet kiểm tra summary/oracle nội bộ và record_count giữ nguyên.

| Ca | N/P/C/K/Dp_ms/Dc_ms | Điều kiện được kiểm chứng | Timeout mỗi lần | Exit thường/quiet |
| --- | --- | --- | --- | --- |
| Mặc định, không đối số | 8/1/1/20/0/0 | 20 DATA, STOP 1/1 | 5s | 0/0 |
| Ca nguồn N5 | 5/1/1/20/0/0 | Cấu hình kế hoạch nguồn; FIFO/data/count đúng | 5s | 0/0 |
| T02 | 8/1/1/100/0/0 | 100 ID đúng một lần | 5s | 0/0 |
| T03 — N1 | 1/1/1/100/0/0 | Occupancy chỉ 0/1; tự kết thúc | 5s | 0/0 |
| T04 — producer nhanh | 2/1/1/40/0/20 | Snapshot đầy, P0 có WAIT_SLOT; tiếp tục sau dequeue | 7s | 0/0 |
| T05 — consumer nhanh | 2/1/1/40/20/0 | Snapshot rỗng và WAIT_ITEM; tiếp tục sau enqueue | 7s | 0/0 |
| K0 | 1/1/1/0/0/0 | Không DATA; STOP đánh thức/dừng consumer; join đủ | 5s | 0/0 |
| Còn DATA trước STOP | 8/1/1/20/0/20 | DATA pending sau producer join >0; tiêu thụ đủ trước STOP | 6s | 0/0 |

EAGAIN chứng minh thiếu quyền ở lần probe; snapshot count dưới guard chứng minh đầy/rỗng riêng. Không khẳng định thời gian block kernel hoặc số lần chờ cố định cho mọi scheduler. Parser biên lớn chỉ kiểm tra parse, không chạy worker ở cấu hình kéo dài nhiều giờ.

## Sanitizer và bảo toàn minh chứng

Sử dụng minh chứng ASan/UBSan/leak detection **đã bàn giao**, không chạy sanitizer mới trong lượt review vì không sửa nguồn và hash đều khớp. [Sanitizer Huy](huy-sanitizers.log) có header khớp manifest 22 file; [sanitizer Châu](chau-sanitizers.log) khớp digest manifest lịch sử và các nguồn chung hiện tại. Makefile/main Châu thuộc snapshot trước CLI, không áp checksum đó cho toàn bản mới. Cả hai log thành công ngoài sandbox, detect_leaks=1 giữ bật và không diagnostics. [Lần Châu lỗi ptrace](chau-sanitizer-sandbox.log) được giữ riêng, không chuyển thành PASS.

Không chạy scripts/validate_week2.py hoặc scripts/validate_week2_huy.py vì chúng ghi đè minh chứng lịch sử. make test/checker không ghi các file đó. [Đánh giá Châu](chau-validation.md) và [đánh giá Huy](huy-validation.md) giữ nguyên trạng thái tại thời điểm bàn giao riêng, có thể còn nói chưa merge; trạng thái hiện tại xem PR #3 và [báo cáo chung](../../docs/reports/week2.md).

## Bàn giao và phạm vi tiếp theo

[PR #3](https://github.com/ndhuy1127/os-producer-consumer/pull/3) là nguồn xác nhận HEAD/base/merge và merge SHA trên GitHub. Merge dùng merge commit giữ lịch sử gốc Châu/Huy, giữ dev/chau/dev/huy; sau merge đồng bộ local/remote ba nhánh, kết thúc checkout dev/chau và smoke test. SHA/ref thực sau thao tác được ghi trong kết quả bàn giao, không điền SHA hoặc xác nhận merge trước khi GitHub thành công.

W2-C/W2-H hoàn thành 1P/1C; T08 chỉ CLI hiện hỗ trợ. T06/T07 và phần nhiều luồng T09 vẫn thuộc Tuần 3, chưa triển khai/chạy trong lượt này. Không suy ra máy khác đã pull từ việc cập nhật checkout hiện tại.
