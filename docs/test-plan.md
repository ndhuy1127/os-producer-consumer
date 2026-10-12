# Kế hoạch kiểm thử

Trạng thái: **T01 đạt; T02–T05 đạt trên lõi 1P/1C Tuần 2; T06–T08 chưa chạy; T09 mới nghiệm thu kết thúc cơ bản 1P/1C**. Đã có chương trình mặc định và semaphore; parser/log production chờ Huy. Cấu hình theo [đặc tả Tuần 1](config-log-week1-huy.md), K là số DATA mỗi producer. [Minh chứng W2-C](../results/week2/chau-validation.md) phân biệt test runner gọi API với đường CLI chưa có; các ca API sai không thay T08.

| ID | Tình huống dự kiến | Tiêu chí cần quan sát | Trạng thái |
| --- | --- | --- | --- |
| T01 | Bộ đệm chạy tuần tự: enqueue/dequeue, wrap-around | Dữ liệu đúng, FIFO, occupancy không vượt N | Đã hoàn thành — 12/12 PASS; [log máy Huy](../results/week1/huy-buffer-test.log), [bản chung máy Châu](../results/week1/group-integration.log) |
| T02 | 1 producer / 1 consumer | Mọi ID được tiêu thụ đúng một lần; tự kết thúc | Đã hoàn thành trên lõi — K=100, PASS, exit 0 |
| T03 | N = 1 | Không mất/lặp; occupancy chỉ 0/1; không deadlock | Đã hoàn thành trên lõi — 1P/1C/K=100, PASS, exit 0 |
| T04 | Producer nhanh, consumer chậm để bộ đệm đầy | Producer thiếu quyền, không ghi vượt N; tiếp tục sau dequeue | Đã hoàn thành trên lõi — snapshot đầy và WAIT_SLOT, exit 0 |
| T05 | Consumer nhanh, producer chậm để bộ đệm trống | Consumer thiếu quyền, không đọc rỗng; tiếp tục sau enqueue | Đã hoàn thành trên lõi — snapshot rỗng và WAIT_ITEM, exit 0 |
| T06 | Nhiều producer/consumer, số lượng khác nhau | ID duy nhất; tập dữ liệu khớp; FIFO theo enqueue thực tế; mọi luồng kết thúc | Chưa bắt đầu |
| T07 | Nhiều dữ liệu và chạy lặp lại | Không mất/lặp, không treo; lưu cấu hình và số lần lặp thực tế | Chưa bắt đầu |
| T08 | CLI thiếu/sai, số âm, 0, quá lớn hoặc tràn số | Từ chối theo quy tắc đã chốt; báo lỗi và mã thoát phù hợp | Chưa bắt đầu |
| T09 | Kết thúc khi consumer đang chờ; còn dữ liệu cần tiêu thụ | Tiêu thụ hết dữ liệu; mọi luồng join; tín hiệu dừng không làm sai thống kê | Đang thực hiện — phần cơ bản 1P/1C đạt; cấu hình nhiều luồng chưa chạy, chưa PASS toàn T09 |

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

## Cấu hình và quan sát dự kiến cho T02–T09

T02–T05 trong bảng dưới đã chạy qua test runner cùng lõi, [log W2-C](../results/week2/chau-runtime-tests.log). Các cấu hình T06–T09 nhiều luồng và đường CLI chưa chạy; parser chưa triển khai. Mỗi cấu hình là `(N,P,C,K,Dp_ms,Dc_ms)`; quiet cần kiểm tra sau ghép parser/log. Độ trễ không tự chứng minh trạng thái đầy/trống hoặc loại trừ deadlock. Ca K0=(1,1,1,0,0,0) và queued-stop=(8,1,1,20,0,20) hiện chỉ kiểm tra kết thúc cơ bản, không thay cấu hình T09 dưới đây.

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

Timeout đã chốt cho test script tương lai: `5 + ceil(2*P*K*(Dp_ms+Dc_ms)/1000)` giây, với kiểm tra tràn. Đây là ngân sách bảo thủ chưa đo trên Producer–Consumer, không bảo đảm toán học về scheduler. Mặc định `(8,1,1,20,0,0)`; N<=65536, P/C<=64, tổng P*K<=1000000, độ trễ ms<=60000 theo [đặc tả](config-log-week1-huy.md). Với quiet vẫn giữ oracle/tổng kết; thiếu quan sát thì ghi Chưa kiểm chứng, không PASS.

## Cách đánh giá khi đã triển khai

- Dữ liệu: đối chiếu tập ID sản xuất/tiêu thụ và tần suất; chỉ tổng số bằng nhau chưa đủ chứng minh không mất/lặp.
- FIFO: đối chiếu thứ tự enqueue/dequeue trong vùng bảo vệ; không dùng thứ tự in log hoặc thứ tự sinh ID như thứ tự FIFO toàn cục.
- Giới hạn: quan sát occupancy sau mỗi thao tác; luôn 0 ≤ count ≤ N.
- Kết thúc: đặt timeout có lý do theo cấu hình và kiểm tra mọi luồng kết thúc; ngân sách theo công thức đã chốt, sẽ đối chiếu thực nghiệm khi có chương trình.
- Full/empty: có bằng chứng thực sự đạt trạng thái cần kiểm tra, không chỉ thay đổi độ trễ rồi suy đoán.
- Lưu lệnh/cấu hình, môi trường, commit được kiểm tra, đầu vào, kết quả quan sát và mã thoát vào results/weekN/. Không bịa log hoặc đánh dấu PASS khi chưa chạy.

Kiểm tra khung bằng `python3 scripts/check_scaffold.py` và thông báo chưa triển khai của Makefile được ghi riêng ở [progress.md](progress.md); không thay các ca thuật toán trên.
