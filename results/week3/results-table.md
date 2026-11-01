# Bảng kết quả Tuần 3

Sinh tự động bởi `scripts/validate_week3.py` lúc 2026-10-20T23:17:48+07:00 (Asia/Saigon), mã nguồn tại commit `3bda04b4091784c48898ce23589c4af9d5eb3b11`, manifest SHA256 `8bb98e5f51266eaa885f73160a1ca6c1485fb59aa95445bae923de40fd42c30a`. Mọi số liệu là của lần chạy này; số lần chờ, số snapshot và cách chia DATA giữa các consumer phụ thuộc bộ lập lịch, không phải hằng số.

## Một lần chạy qua chương trình chính (thường và quiet)

Cả hai chế độ đều exit 0 và in `verification=PASS`. Ở chế độ thường, script dựng lại hàng đợi tuyến tính từ từng dòng log để kiểm tra mọi ID đúng một lần, FIFO, chủ sở hữu ID và thứ tự trong từng producer, count trong [0,N], mỗi consumer nhận đúng một STOP. Cột số liệu lấy từ lần chạy thường.

| Ca | N/P/C/K/Dp/Dc | Thường / quiet | Bản ghi | count tối đa | Snapshot đầy / rỗng | WAIT_SLOT / WAIT_ITEM | DATA còn đợi khi gửi STOP | DATA từng consumer |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| default | 8/1/1/20/0/0 | PASS / PASS | 42 | 8 | 2 / 4 | 2 / 4 | 0 | 20 |
| source-N5 | 5/1/1/20/0/0 | PASS / PASS | 42 | 5 | 4 / 5 | 3 / 5 | 0 | 20 |
| T02 | 8/1/1/100/0/0 | PASS / PASS | 202 | 8 | 12 / 14 | 12 / 14 | 0 | 100 |
| T03 | 1/1/1/100/0/0 | PASS / PASS | 202 | 1 | 101 / 101 | 99 / 101 | 0 | 100 |
| T04 | 2/1/1/40/0/20 | PASS / PASS | 82 | 2 | 40 / 1 | 39 / 1 | 2 | 40 |
| T05 | 2/1/1/40/20/0 | PASS / PASS | 82 | 1 | 0 / 41 | 0 / 41 | 0 | 40 |
| K0 | 1/1/1/0/0/0 | PASS / PASS | 2 | 1 | 1 / 1 | 0 / 1 | 0 | 0 |
| queued-stop | 8/1/1/20/0/20 | PASS / PASS | 42 | 8 | 14 / 1 | 13 / 1 | 8 | 20 |
| T06a | 3/3/2/100/0/0 | PASS / PASS | 604 | 3 | 122 / 95 | 156 / 89 | 0 | 154, 146 |
| T06b | 2/2/5/100/0/0 | PASS / PASS | 410 | 2 | 104 / 101 | 3 / 112 | 0 | 43, 37, 44, 41, 35 |
| T09a | 1/1/4/0/0/0 | PASS / PASS | 8 | 1 | 4 / 4 | 3 / 4 | 0 | 0, 0, 0, 0 |
| T09b | 2/2/3/30/0/10 | PASS / PASS | 126 | 2 | 44 / 19 | 45 / 7 | 1 | 20, 20, 20 |
| T04-multi | 2/3/2/40/0/20 | PASS / PASS | 244 | 2 | 74 / 48 | 72 / 2 | 2 | 60, 60 |
| T05-multi | 2/2/3/40/20/0 | PASS / PASS | 166 | 2 | 20 / 63 | 0 / 66 | 0 | 25, 27, 28 |
| queued-stop-multi | 8/2/3/40/0/20 | PASS / PASS | 166 | 8 | 42 / 1 | 42 / 3 | 8 | 27, 27, 26 |

## Chạy lặp, mỗi lần kiểm tra bằng oracle đầy đủ

Khoảng giá trị là nhỏ nhất–lớn nhất qua các lần chạy.

| Cấu hình | N/P/C/K/Dp/Dc | Lần chạy đạt | DATA mỗi lần | Thời gian (s) | count tối đa | Snapshot đầy | WAIT_SLOT | WAIT_ITEM | DATA còn đợi khi STOP | DATA ít/nhiều nhất của 1 consumer |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| T06a | 3/3/2/100/0/0 | 20/20 | 300 | 0.004–0.009 | 3 | 97–149 | 124–175 | 55–136 | 0 | 123–177 |
| T06b | 2/2/5/100/0/0 | 20/20 | 200 | 0.003–0.02 | 2 | 67–101 | 1–41 | 117–144 | 0 | 27–53 |
| T07 | 16/4/3/10000/0/0 | 20/20 | 40000 | 0.118–0.205 | 16 | 2536–3850 | 6710–7958 | 3282–4749 | 0 | 11779–14437 |
| T09a | 1/1/4/0/0/0 | 20/20 | 0 | 0.001–0.005 | 1 | 4 | 2–3 | 4 | 0 | 0 |
| T09b | 2/2/3/30/0/10 | 20/20 | 60 | 0.208–0.233 | 2 | 40–50 | 40–48 | 4–12 | 0–2 | 20 |
| N1-8P8C | 1/8/8/500/0/0 | 20/20 | 4000 | 0.038–0.08 | 1 | 4008 | 2059–3149 | 2166–3264 | 0 | 409–774 |
| max-workers | 64/64/64/100/0/0 | 20/20 | 6400 | 0.021–0.045 | 21–56 | 0 | 0–2043 | 2471–3621 | 0 | 18–216 |

## Sanitizer

| Công cụ | Chương trình | Cấu hình | Kết quả |
| --- | --- | --- | --- |
| ASan+UBSan | bin/producer-consumer | T06a, T06b, T09a, T09b, T04-multi, T05-multi, N1-8P8C, max-workers, T07 | PASS, không có cảnh báo |
| ASan+UBSan | pc-test all | 16 preset lõi | PASS, không có cảnh báo |
| TSan | bin/producer-consumer | T06a, T06b, T09a, T09b, T04-multi, T05-multi, N1-8P8C, max-workers, T07 | PASS, không có cảnh báo |
| TSan | pc-test all | 16 preset lõi | PASS, không có cảnh báo |
