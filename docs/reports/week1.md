# Báo cáo chung Tuần 1 — Chuẩn bị và thiết kế

**Phần việc và tiêu chí kỹ thuật Tuần 1 của Châu và Huy đã đạt; bản chung đã kiểm tra.** Ngày kiểm tra 08/10/2026 theo Asia/Saigon, không tự đặt ngày bắt đầu/hạn nộp. Tích hợp/rà soát do Codex thực hiện theo yêu cầu Châu trên máy Châu; trạng thái bàn giao Git xem PR #2 ở phần 4.

## 1. Mục tiêu tuần

- Châu: lý thuyết luồng/semaphore, giả mã đồng bộ, thiết kế kết thúc hữu hạn, ví dụ tạo/join.
- Huy: môi trường C, Makefile, bộ đệm vòng tuần tự, đặc tả tham số/log và kế hoạch kiểm thử.
- Mốc chung: chốt giao diện theo mã thực tế, tích hợp và kiểm tra, cập nhật báo cáo/minh chứng, bàn giao hai nhánh qua main.

## 2. Công việc Châu đã hoàn thành

Hoàn thành W1-C theo phạm vi tài liệu, thiết kế và ví dụ vòng đời luồng. Thiết kế đã ghép với API thực tế qua Codex theo yêu cầu Châu; chưa triển khai Producer–Consumer.

| Nhiệm vụ | Việc thực tế đã làm | Trạng thái | Minh chứng |
| --- | --- | --- | --- |
| Lý thuyết luồng/semaphore | Viết tiếng Việt về tiến trình/luồng, hàng đợi N chỗ và FIFO, vùng găng/data race, deadlock, empty/full/guard, sleep và create/join; đối chiếu Linux man-pages, ghi rõ chưa đọc giáo trình | Đã hoàn thành | [Lý thuyết](../theory-week1-chau.md) |
| Thiết kế và giả mã | Chọn ba semaphore pshared = 0; viết main/producer/consumer/enqueue STOP; nêu bất biến, FIFO, quyền sở hữu và lỗi; đề xuất API bộ đệm tuần tự để ghép với Huy | Đã hoàn thành; đã rà soát kỹ thuật khi tích hợp | [Thiết kế](../design.md) |
| Ví dụ C và build | Tạo 3 worker có struct đối số và ô kết quả riêng, tính tổng hữu hạn, join rồi kiểm tra; xử lý mã lỗi pthread và create lỗi giữa chừng; thêm target thread-demo, ở bản độc lập giữ target mặc định/test chưa triển khai; bản chung hiện chạy test tuần tự | Đã hoàn thành | [Mã C](../../examples/thread_lifecycle.c), [Makefile](../../Makefile) |
| Chạy kiểm tra và lưu minh chứng | Biên dịch với cảnh báo và -pthread; chạy 5 lần, clean/build/chạy lại, kiểm tra kết quả; thử create lỗi ở lần 1/3 bằng wrapper tạm ngoài repo; cập nhật tài liệu và log thực tế | Đã hoàn thành | [Môi trường](../../results/week1/chau-environment.log), [log](../../results/week1/chau-thread-demo.log), [đánh giá](../../results/week1/chau-validation.md) |

## 3. Công việc Huy đã hoàn thành

Nguyễn Đức Huy — 20233448; thực hiện trên `dev/huy`, Codex hỗ trợ theo yêu cầu. Phần độc lập W1-H đã hoàn thành; API/cấu hình đã được Codex rà soát và chốt theo yêu cầu Châu trong lần tích hợp này, không ghi Huy phê duyệt hoặc hai người đã đọc chéo trực tiếp.

| Nhiệm vụ | Việc thực tế đã làm | Trạng thái | Minh chứng |
| --- | --- | --- | --- |
| Môi trường C trên máy Huy | Kiểm tra Ubuntu 24.04.5/WSL2, GCC 13.3.0, Make 4.3, Git 2.43.0, Python 3.12.3; build/chạy ví dụ Châu từ đúng SHA ngoài repo, kết quả 55/210/465 và đủ join | Đã hoàn thành | [Log môi trường](../../results/week1/huy-environment.log) |
| API bộ đệm tuần tự | DATA/STOP; mảng, head/tail/count/capacity; bốn hàm tương thích chữ ký Châu; lỗi và quyền sở hữu rõ ràng | Đã hoàn thành | [Hợp đồng](../buffer-week1-huy.md), [nguồn](../../src/buffer.c) |
| Makefile và T01 | Build riêng, test tuần tự, không tạo main giả; 12/12 ca qua hai build sạch; sanitizer 11/11 ca thường | Đã hoàn thành | [Log test](../../results/week1/huy-buffer-test.log), [đánh giá](../../results/week1/huy-validation.md) |
| Đề xuất tham số/log/test Tuần 2 | K DATA mỗi producer, mặc định/giới hạn/lỗi CLI, schema log trong guard, quan sát chờ bằng semaphore; T02–T09 có cấu hình nhưng chưa chạy | Đã hoàn thành | [Cấu hình/log](../config-log-week1-huy.md), [test-plan](../test-plan.md) |

## 4. Kết quả tích hợp và kiểm thử của nhóm

PR #1 đã merge ở `ac5ff4de4f765f70dfd56ed843db936b3020d909`. Đã ghép origin/main vào dev/huy có nền `b12b825a53119e44d7bf063ef957eff1d280f660`, xử lý 6 file xung đột: Makefile, README, design, progress, báo cáo này và README minh chứng. Bảo toàn mã/commit và log độc lập của hai người; không chọn toàn bộ ours/theirs. Trạng thái merge GitHub xem [PR #2](https://github.com/ndhuy1127/os-producer-consumer/pull/2), không ghi sự kiện merge trước khi xảy ra.

Giao diện đã chốt theo code: zero-init buffer; lỗi trực tiếp; push/pop sao chép item; out riêng không alias; buffer sở hữu mảng, destroy reset; không sao chép buffer đang sở hữu để dùng như đối tượng thứ hai; lớp Châu khóa ngoài API. Thiết kế ba semaphore, STOP, CLI/log và test-plan đã thống nhất về mặt kỹ thuật. Chốt này do Codex theo yêu cầu Châu, không đồng nghĩa Huy phê duyệt hoặc cả hai đã đọc chéo trực tiếp.

**Bản chung đã kiểm tra đạt** trên Ubuntu 24.04.5/WSL2 máy Châu, GCC 13.3.0; nguồn là hai SHA nền ở trên cộng nội dung giải quyết xung đột trước commit. Log ghi SHA256 Makefile và năm file C/header/test, không gán SHA nền cho nội dung mới.

| Lệnh kiểm tra bản chung | Kết quả thực tế | Mã thoát |
| --- | --- | --- |
| make clean | Dọn build/bin | 0 |
| make thread-demo buffer-test | Build sạch, không cảnh báo compiler; -pthread ở compile/link, wrapper chỉ trong test | 0 |
| timeout 5s ./bin/thread-demo | 3 worker, 3 join, 55/210/465 đúng; completed cuối, thứ tự worker tự do | 0 |
| make test | Đủ 12/12 ca T01: FIFO, wrap, N=1, đầy/rỗng, STOP, lỗi/vòng đời/malloc | 0 |
| make help | Đủ target thread-demo/buffer-test/test-buffer/test/clean | 0 |
| make | Chưa có src/main.c, không link chương trình giả | 2 đúng dự kiến |
| python3 scripts/check_scaffold.py; git diff --check | Khung, liên kết, ignore và whitespace đạt | 0 |
| git ls-files -u; rà conflict marker/tracked/staged | Không unmerged hoặc marker; không AGENTS/build/cache/secret/file tạm | 0; rg trả 1 do không có marker |

Kết quả mới lưu tại [group-integration.log](../../results/week1/group-integration.log), đánh giá ở [group-validation.md](../../results/week1/group-validation.md). Phạm vi là create/join và bộ đệm tuần tự T01. Năm file C/header/test không sửa và khớp hash minh chứng Huy, gồm sanitizer 11/11 ca thường đã chạy trên máy Huy; không ghi sanitizer mới trên máy Châu. Các log độc lập ở mục 2/3 giữ nguyên byte, không thay chúng bằng kết quả máy Châu. T02–T09 chưa chạy.

## 5. Việc còn thiếu hoặc đang vướng

Không còn vướng kỹ thuật trong tiêu chí Tuần 1 đã kiểm tra; trạng thái merge/bàn giao Git theo PR #2, không suy ra từ bản tài liệu trước merge. Producer/consumer, semaphore runtime, CLI parser và log runtime chưa triển khai; T02–T09 chưa chạy, thuộc tuần 2 trở đi và không phải tiêu chí thiếu Tuần 1. Không có bằng chứng Huy phê duyệt trực tiếp; báo cáo chỉ xác nhận rà soát kỹ thuật theo yêu cầu Châu.

## 6. Công việc tiếp theo

Sau bàn giao, dùng đặc tả chung để Châu triển khai producer/consumer, semaphore và STOP; Huy triển khai CLI/log theo API đã có. Kiểm tra T02–T05/T09 trước, sau đó nhiều luồng T06–T08 theo [kế hoạch](../plan.md) và [test-plan](../test-plan.md). Giữ dev/chau/dev/huy sau merge, cập nhật main trước việc mới. Quy tắc báo cáo ở [README báo cáo](README.md).
