# Kế hoạch kiểm thử

Trạng thái: **T01 đã chạy cho bộ đệm tuần tự; T02–T09 dự kiến/chưa chạy**. Chưa có chương trình Producer–Consumer, semaphore hoặc parser CLI. Cấu hình dưới đây theo [đề xuất Huy](config-log-week1-huy.md), K là số DATA mỗi producer, còn cần Châu xác nhận. T03 nhiều luồng N=1 chưa được kiểm chứng bởi ca T01 N=1 tuần tự.

| ID | Tình huống dự kiến | Tiêu chí cần quan sát | Trạng thái |
| --- | --- | --- | --- |
| T01 | Bộ đệm chạy tuần tự: enqueue/dequeue, wrap-around | Dữ liệu đúng, FIFO, occupancy không vượt N | Đã hoàn thành — 12/12 PASS; [log](../results/week1/huy-buffer-test.log) |
| T02 | 1 producer / 1 consumer | Mọi ID được tiêu thụ đúng một lần; tự kết thúc | Chưa bắt đầu |
| T03 | N = 1 | Không mất/lặp; occupancy chỉ 0/1; không deadlock | Chưa bắt đầu |
| T04 | Producer nhanh, consumer chậm để bộ đệm đầy | Producer chờ khi đầy, không ghi vượt N; tiếp tục sau dequeue | Chưa bắt đầu |
| T05 | Consumer nhanh, producer chậm để bộ đệm trống | Consumer chờ khi trống, không đọc rỗng; tiếp tục sau enqueue | Chưa bắt đầu |
| T06 | Nhiều producer/consumer, số lượng khác nhau | ID duy nhất; tập dữ liệu khớp; FIFO theo enqueue thực tế; mọi luồng kết thúc | Chưa bắt đầu |
| T07 | Nhiều dữ liệu và chạy lặp lại | Không mất/lặp, không treo; lưu cấu hình và số lần lặp thực tế | Chưa bắt đầu |
| T08 | CLI thiếu/sai, số âm, 0, quá lớn hoặc tràn số | Từ chối theo quy tắc đã chốt; báo lỗi và mã thoát phù hợp | Chưa bắt đầu |
| T09 | Kết thúc khi consumer đang chờ; còn dữ liệu cần tiêu thụ | Tiêu thụ hết dữ liệu; mọi luồng join; tín hiệu dừng không làm sai thống kê | Chưa bắt đầu |

## T01 — kết quả thực chạy trên Ubuntu/WSL

Ngày kiểm tra 08/10/2026 theo Asia/Saigon. Lệnh `make clean`, `make buffer-test`, `make test-buffer` chạy hai chu kỳ build sạch; `make test` cũng chạy lại, exit 0. GCC 13.3.0, C11, -Wall -Wextra -Wpedantic, không cảnh báo. [Nguồn kiểm thử](../tests/test_buffer.c) và [minh chứng](../results/week1/huy-validation.md) ghi SHA256 của bản thực chạy; không dùng commit nền để tuyên bố code mới đã nằm trong đó.

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

Các ca dưới đây **chưa chạy**, tên cờ còn là đề xuất. Mỗi cấu hình là `(N,P,C,K,Dp_ms,Dc_ms)`; mặc định tắt quiet để có bằng chứng thao tác, cần chạy thêm quiet khi parser/log đã có. Độ trễ không tự chứng minh trạng thái đầy/trống hoặc loại trừ deadlock.

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

Timeout dự kiến cho test script: `5s + 2*(P*K*(Dp+Dc)/1000)` làm ngân sách bảo thủ cho chạy hữu hạn trên máy thử; kiểm tra phép tính không tràn. Chưa chốt/đo thời gian và không coi timeout này là chứng minh toán học về scheduler. Với quiet phải vẫn giữ dữ liệu oracle/tổng kết; nếu không quan sát đủ thì ghi Chưa kiểm chứng, không PASS.

## Cách đánh giá khi đã triển khai

- Dữ liệu: đối chiếu tập ID sản xuất/tiêu thụ và tần suất; chỉ tổng số bằng nhau chưa đủ chứng minh không mất/lặp.
- FIFO: đối chiếu thứ tự enqueue/dequeue trong vùng bảo vệ; không dùng thứ tự in log hoặc thứ tự sinh ID như thứ tự FIFO toàn cục.
- Giới hạn: quan sát occupancy sau mỗi thao tác; luôn 0 ≤ count ≤ N.
- Kết thúc: đặt timeout có lý do theo cấu hình và kiểm tra mọi luồng kết thúc; timeout chưa chốt.
- Full/empty: có bằng chứng thực sự đạt trạng thái cần kiểm tra, không chỉ thay đổi độ trễ rồi suy đoán.
- Lưu lệnh/cấu hình, môi trường, commit được kiểm tra, đầu vào, kết quả quan sát và mã thoát vào results/weekN/. Không bịa log hoặc đánh dấu PASS khi chưa chạy.

Kiểm tra khung bằng `python3 scripts/check_scaffold.py` và thông báo chưa triển khai của Makefile được ghi riêng ở [progress.md](progress.md); không thay các ca thuật toán trên.
