# Kế hoạch kiểm thử

Trạng thái: **T01–T09 đều đã chạy và đạt** (ngày 20/10/2026, commit `d2329b6`, [minh chứng Tuần 3](../results/week3/README.md)). T01–T05 đạt từ Tuần 1–2 và chạy lại đạt ở Tuần 3; T06, T07, T09 nhiều luồng và T08 với giới hạn P/C = 1..64 được kiểm chứng ở Tuần 3. [Minh chứng Châu](../results/week2/chau-validation.md) giữ snapshot trước CLI; [minh chứng Huy](../results/week2/huy-validation.md) ghi bản ghép Tuần 2, [review chung](../results/week2/group-validation.md) kiểm tra lại đạt; PR #3 đã merge ở `d125734`. K là số DATA mỗi producer.

| ID | Tình huống dự kiến | Tiêu chí cần quan sát | Trạng thái |
| --- | --- | --- | --- |
| T01 | Bộ đệm chạy tuần tự: enqueue/dequeue, wrap-around | Dữ liệu đúng, FIFO, occupancy không vượt N | Đã hoàn thành — 12/12 PASS; [log máy Huy](../results/week1/huy-buffer-test.log), [bản chung máy Châu](../results/week1/group-integration.log) |
| T02 | 1 producer / 1 consumer | Mọi ID được tiêu thụ đúng một lần; tự kết thúc | Đã hoàn thành qua CLI — K=100, thường/quiet PASS, exit 0 |
| T03 | N = 1 | Không mất/lặp; occupancy chỉ 0/1; không deadlock | Đã hoàn thành qua CLI — 1P/1C/K=100, thường/quiet PASS, exit 0 |
| T04 | Producer nhanh, consumer chậm để bộ đệm đầy | Producer thiếu quyền, không ghi vượt N; tiếp tục sau dequeue | Đã hoàn thành qua CLI — 40 snapshot đầy, 39 WAIT_SLOT (38 P0); quiet PASS, exit 0 |
| T05 | Consumer nhanh, producer chậm để bộ đệm trống | Consumer thiếu quyền, không đọc rỗng; tiếp tục sau enqueue | Đã hoàn thành qua CLI — 41 snapshot rỗng, 40 WAIT_ITEM; quiet PASS, exit 0 |
| T06 | Nhiều producer/consumer, số lượng khác nhau | ID duy nhất; tập dữ liệu khớp; FIFO theo enqueue thực tế; mọi luồng kết thúc | Đã hoàn thành — T06a 3/3/2/100 và T06b 2/2/5/100 (C > N) qua lõi, CLI thường/quiet, 20 lần lặp mỗi cấu hình, ASan/UBSan, TSan |
| T07 | Nhiều dữ liệu và chạy lặp lại | Không mất/lặp, không treo; lưu cấu hình và số lần lặp thực tế | Đã hoàn thành — 16/4/3/10000, 20/20 lần, mỗi lần 40000 DATA kiểm tra đủ; thêm 8P8C N=1 và 64P64C, mỗi cấu hình 20/20 |
| T08 | CLI thiếu/sai, số âm, 0, quá lớn hoặc tràn số | Từ chối theo quy tắc đã chốt; báo lỗi và mã thoát phù hợp | Đã hoàn thành — 114 đầu vào sai × 2 binary exit 2, không chạy lõi; P/C = 0, 65, 2^64−1; P·K > 1000000; 6 biên parser gồm P=C=64, P·K = 1000000 |
| T09 | Kết thúc khi consumer đang chờ; còn dữ liệu cần tiêu thụ | Tiêu thụ hết dữ liệu; mọi luồng join; tín hiệu dừng không làm sai thống kê | Đã hoàn thành — T09a 1/1/4/0: WAIT_ITEM = 4 ở 20/20 lần (cả 4 consumer đang chờ); T09b 2/2/3/30/0/10 còn 0–2 DATA khi gửi STOP qua 20 lần, queued-stop-multi 8/2/3/40/0/20 còn 8 DATA; mọi DATA được lấy trước STOP |

## T01 — minh chứng độc lập máy Huy và kiểm tra bản chung

Minh chứng độc lập trên máy Huy ngày 08/10/2026 theo Asia/Saigon; log lịch sử giữ nguyên. Kiểm tra bản tích hợp trên máy Châu được ghi riêng ở [group-validation](../results/week1/group-validation.md). Lệnh `make clean`, `make buffer-test`, `make test-buffer` chạy hai chu kỳ build sạch; `make test` cũng chạy lại, exit 0. GCC 13.3.0, C11, -Wall -Wextra -Wpedantic, không cảnh báo. [Nguồn kiểm thử](../tests/test_buffer.c) và [minh chứng](../results/week1/huy-validation.md) ghi SHA256 của bản thực chạy; không dùng commit nền để tuyên bố code mới đã nằm trong đó.

| Ca trong T01 | Cấu hình/đầu vào thực chạy | Kết quả quan sát |
| --- | --- | --- |
| Khởi tạo | N=3, đối tượng zero | PASS: head=tail=count=0; capacity=3 |
| Một item/sao chép | N=2, DATA id=42/value=-17; đổi biến đầu vào sau push | PASS: pop đúng bản gốc |
| FIFO và đầy | N=3, ID 90,2,77; push ID999 khi đầy | PASS: FULL; state/mảng cũ nguyên vẹn; pop theo 90,2,77 |
| Rỗng | N=2, pop lúc đầu và sau khi rút hết | PASS: EMPTY; state và out không đổi |
| Wrap-around | N=3, 100 lượt, để lại một item giữa lượt | PASS: mọi pop khớp oracle tuyến tính kind/id/value; biên hợp lệ |
| N=1 | 50 chu kỳ thêm/lấy | PASS: chỉ số 0; đầy/trống từ chối đúng; dữ liệu đúng |
| STOP | DATA0, STOP(id=UINT64_MAX/value=-9), DATA1 | PASS: giữ thứ tự/các trường, STOP chiếm chỗ và không làm API dừng |
| Đối số/lifecycle | NULL, N=0, out NULL, kind=99, trước init | PASS: INVALID_ARGUMENT; giữ state/out và DATA đang đợi |
| Tràn kích thước | N=SIZE_MAX/sizeof(item)+1 | PASS: SIZE_OVERFLOW; zero object nguyên vẹn |
| Init lại khi sống | N=2 có DATA, init N=4 | PASS: ALREADY_INITIALIZED; dữ liệu/mảng cũ giữ nguyên |
| Destroy/reuse | NULL, zero, còn DATA, destroy hai lần, init lại N=1 | PASS: reset zero; thao tác sau destroy bị từ chối; tái dùng được |
| malloc thất bại | Wrapper tiêm một lần, sau đó init N=3 lại | PASS: NO_MEMORY, state zero; phục hồi và free đủ |

Wrapper malloc/free kiểm tra không còn cấp phát mảng sau **mỗi** ca. Bản AddressSanitizer/UndefinedBehaviorSanitizer chạy 11 ca thường, exit 0, không diagnostics; ca tiêm malloc chỉ chạy ở binary wrapper. Phạm vi kết luận: API tuần tự, không phải nghiệm thu T02–T09.

## Cấu hình T02–T09

T02–T05 chạy ở Tuần 2 qua test runner [Châu](../results/week2/chau-runtime-tests.log) và qua CLI thật [Huy](../results/week2/huy-runtime-tests.log); Tuần 3 chạy lại cùng các ca và thêm cấu hình nhiều luồng (phần dưới). Độ trễ không tự chứng minh đầy/trống: ca thường kiểm tra snapshot count = N/0 trong guard và số lần thiếu quyền riêng; quiet kiểm tra tổng kết và phần tự kiểm chứng của lõi, không có dòng log để dựng oracle.

| ID | Cấu hình/đầu vào dự kiến | Mục tiêu và kết quả cần kiểm tra |
| --- | --- | --- |
| T02 | (8,1,1,100,0,0) | 100 ID đúng một lần, hai dãy enqueue/dequeue khớp, count trong [0,8], 1 STOP, tất cả join, cuối count=0 |
| T03 | (1,1,1,100,0,0) | DATA không mất/lặp, occupancy 0/1, wait/post tiến được, STOP trả chỗ và cả hai worker kết thúc |
| T04 | (2,1,1,40,0,20) | Snapshot đầy trong guard và thiếu quyền empty qua semaphore; producer tiếp tục sau dequeue, không ghi vượt N |
| T05 | (2,1,1,40,20,0) | Snapshot rỗng trong guard và thiếu quyền full qua semaphore; consumer tiếp tục khi có enqueue, không đọc rỗng |
| T06 | (3,3,2,100,0,0) và (2,2,5,100,0,0) | 300/200 DATA, ID p*K+j đúng tần suất; FIFO theo enqueue thực; C STOP, cả trường hợp C>N; mọi worker join |
| T07 | (16,4,3,10000,0,0), 20 lần | Mỗi lần 40000 DATA; không mất/lặp/treo, FIFO/biên/STOP/join; lưu số lần thực tế, không suy rộng vô hạn |
| T08 | Cờ lạ/lặp/thiếu; N/P/C=0; K=0 hợp lệ; -1,+1,1x,1.5; chuỗi số vượt uint64_t; N=65537/P=65/D=60001; P=64,K=1000000 vượt tổng | Parser từ chối sai/tràn/out-of-range trước tạo worker, stderr cụ thể, exit 2; mặc định/help/K=0 được xử lý theo hợp đồng; runtime lỗi exit 1 |
| T09 | (1,1,4,0,0,0) và (2,2,3,30,0,10) | K=0 đánh thức C consumer qua STOP; trường hợp 60 DATA phải dequeue hết trước STOP, join xác nhận xử lý xong, STOP không tính DATA, count cuối 0 |

Timeout mỗi lần chạy: 5+ceil(2·P·K·(Dp_ms+Dc_ms)/1000) giây (Tuần 2 dùng cùng công thức với P=1: 5 s ca không delay, 7 s T04/T05, 6 s queued-stop). Đây là ngân sách kiểm thử, không bảo đảm lịch chạy của hệ điều hành. Mặc định (8,1,1,20,0,0); N ≤ 65536, P và C ≤ 64, K ≤ 1000000 và P·K ≤ 1000000, độ trễ ≤ 60000 ms. Thiếu bằng chứng cần thiết thì không PASS.

## Tuần 3 — cấu hình nhiều luồng và kết quả thực chạy

Chạy ngày 20/10/2026 (Asia/Saigon) trên máy Huy, commit `d2329b6`, bằng `python3 scripts/validate_week3.py`; Claude Code thực hiện theo yêu cầu của Huy. Mọi cấu hình dưới đây chạy ở cả test runner lõi (`make test-pc`) và chương trình chính (`make test-cli`, thường + quiet), đều đạt. Cột "Bắt buộc" là điều kiện test sẽ FAIL nếu không quan sát được.

| Tên | N/P/C/K/Dp/Dc | Mục đích | Bắt buộc | Quan sát lần chạy thường qua CLI |
| --- | --- | --- | --- | --- |
| T06a | 3/3/2/100/0/0 | Nhiều producer và consumer | Oracle đầy đủ | 300 DATA; consumer 154/146 |
| T06b | 2/2/5/100/0/0 | C > N | Oracle đầy đủ | 200 DATA; 5 STOP qua bộ đệm 2 chỗ |
| T09a | 1/1/4/0/0/0 | Chỉ STOP, consumer đang chờ, C > N | Oracle đầy đủ | WAIT_ITEM = 4 |
| T09b | 2/2/3/30/0/10 | Consumer chậm, kết thúc khi còn dữ liệu | Oracle đầy đủ | Còn 1 DATA khi gửi STOP |
| T04-multi | 2/3/2/40/0/20 | Bộ đệm đầy với nhiều producer | Có snapshot đầy và WAIT_SLOT | 74 snapshot đầy; 71 WAIT_SLOT của producer |
| T05-multi | 2/2/3/40/20/0 | Bộ đệm rỗng với nhiều consumer | Có snapshot rỗng và WAIT_ITEM | 63 snapshot rỗng; 66 WAIT_ITEM |
| queued-stop-multi | 8/2/3/40/0/20 | Còn dữ liệu khi gửi STOP | DATA còn đợi > 0 | Còn 8 DATA |
| T07 (chỉ lõi + chạy lặp) | 16/4/3/10000/0/0 | Nhiều dữ liệu | Oracle đầy đủ | 20/20 lần, 40000 DATA mỗi lần |

Chạy lặp (`make test-stress`, 20 lần mỗi cấu hình, mỗi lần kiểm tra bằng oracle đầy đủ từ log): T06a, T06b, T07, T09a, T09b, 8P8C với N=1 (1/8/8/500/0/0) và 64P64C (64/64/64/100/0/0), tổng 140/140 lần đạt. ASan+UBSan và TSan chạy 9 cấu hình chương trình chính và 16 preset lõi, không có cảnh báo. Tiêm lỗi 32/32: thêm `EINTR` khi 3P/3C, tạo consumer thứ 2/3 lỗi, tạo producer thứ 2/3 lỗi, P/C = 0 hoặc 65, P·K > 1000000. Số liệu đầy đủ: [results-table.md](../results/week3/results-table.md).

Số lần chờ, snapshot và cách chia DATA giữa consumer phụ thuộc bộ lập lịch; các con số trên là của lần chạy được lưu, không phải hằng số. Kiểm thử chỉ quan sát những lịch chạy đã xảy ra.

## Cách đánh giá khi đã triển khai

- Dữ liệu: đối chiếu tập ID sản xuất/tiêu thụ và tần suất; chỉ tổng số bằng nhau chưa đủ chứng minh không mất/lặp.
- FIFO: đối chiếu thứ tự enqueue/dequeue trong vùng bảo vệ; không dùng thứ tự in log hoặc thứ tự sinh ID như thứ tự FIFO toàn cục.
- Giới hạn: quan sát occupancy sau mỗi thao tác; luôn 0 ≤ count ≤ N.
- Kết thúc: đặt timeout có lý do theo cấu hình và kiểm tra mọi luồng kết thúc; ngân sách theo công thức đã chốt. Thực tế T07 (40000 DATA) chạy 0,12–0,21 s so với ngân sách 5 s.
- Nhiều luồng: ID p·K+j phải do producer p đưa vào theo thứ tự j; mỗi consumer nhận đúng một STOP; số DATA từng luồng trong tổng kết phải khớp log.
- Full/empty: có bằng chứng thực sự đạt trạng thái cần kiểm tra, không chỉ thay đổi độ trễ rồi suy đoán.
- Lưu lệnh/cấu hình, môi trường, commit được kiểm tra, đầu vào, kết quả quan sát và mã thoát vào results/weekN/. Không bịa log hoặc đánh dấu PASS khi chưa chạy.

Kiểm tra khung bằng python3 scripts/check_scaffold.py được ghi riêng, không thay nghiệm thu thuật toán/CLI. Makefile build production và chạy các nhóm bằng `make test`; `make test-stress` chạy lặp. `scripts/validate_week3.py` chỉ ghi results/week3; các writer Tuần 2 là lịch sử, không chạy lại. Hướng dẫn chạy ở [run-guide.md](run-guide.md).
