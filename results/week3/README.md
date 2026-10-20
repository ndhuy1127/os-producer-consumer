# Minh chứng Tuần 3

**Bản nhiều producer/consumer đạt toàn bộ kiểm thử đã chạy.** Chạy ngày 20/10/2026 lúc 23:17 (Asia/Saigon) trên máy của Huy (WSL2 Ubuntu 24.04, GCC 13.3.0, Make 4.3, Python 3.12.3), mã nguồn đúng commit `d2329b6` trên `dev/huy`, cây nguồn sạch. Claude Code thực hiện theo yêu cầu của Huy, bằng lệnh:

```sh
python3 scripts/validate_week3.py
```

**Về SHA trong log:** các log ghi `HEAD=3bda04b4091784c48898ce23589c4af9d5eb3b11`. Đó là SHA của commit W3-H trước khi message của nó được viết lại ngày 21/10/2026 (bỏ dòng ghi công AI, chưa từng push). Nội dung file không đổi: commit hiện tại `d2329b695fbc7ecbda68727ec5d87d0a81c6a328` có cùng tree `6d99a2d847f411adb2b296114f4dcc68b5c1ffd2`, kiểm tra bằng `git rev-parse d2329b6^{tree}`; manifest bên dưới vẫn khớp. Log được giữ nguyên như lúc chạy, không sửa SHA.

Script chỉ ghi vào thư mục này, không sửa minh chứng Tuần 1–2. Manifest [source.sha256](source.sha256) (23 file) có SHA256 `8bb98e5f…c30a`; đối chiếu bằng `sha256sum -c results/week3/source.sha256`. Phần W3-C (lõi nhiều luồng) **chưa được Châu review/xác nhận**.

## Kết quả

| Nhóm | Kết quả | Log |
| --- | --- | --- |
| Build sạch: chương trình, ví dụ, mọi test | Không cảnh báo với `-std=c11 -Wall -Wextra -Wpedantic -pthread` | [build.log](build.log) |
| Ví dụ luồng, bộ đệm, lõi, tiêm lỗi | thread-demo đạt; buffer 12/12; 16/16 preset lõi; 32/32 ca lỗi | [core-tests.log](core-tests.log) |
| Chương trình chính, 15 cấu hình × thường/quiet | 30/30 exit 0, `verification=PASS`; oracle từ log đạt ở mọi lần chạy thường | [runtime-tests.log](runtime-tests.log) |
| Tham số sai, help, biên parser | 114 đầu vào sai × 2 binary exit 2, không chạy lõi; help 2; positive control exit 99; 6 biên | [cli-tests.log](cli-tests.log) |
| Formatter và lỗi ở main | Formatter 8/8; 6 ca lỗi init/cleanup/unverified exit 1, không PASS, vẫn giải phóng kết quả | [log-tests.log](log-tests.log) |
| Chạy lặp nhiều luồng | 7 cấu hình × 20 = 140/140 lần đạt oracle đầy đủ | [stress-tests.log](stress-tests.log) |
| ASan + UBSan (bật phát hiện rò rỉ) và ThreadSanitizer | 9 cấu hình chương trình chính + 16 preset lõi cho mỗi công cụ, không cảnh báo | [sanitizers.log](sanitizers.log) |
| Rà soát | Kiểm tra khung/liên kết, `git diff --check`, Tuần 1–2 không đổi, manifest không đổi trong lúc chạy | [review.log](review.log) |

Bảng số liệu chi tiết (snapshot đầy/rỗng, số lần chờ, DATA từng consumer, khoảng giá trị khi chạy lặp): [results-table.md](results-table.md). Dữ liệu máy đọc được: [test-summary.json](test-summary.json).

## Cách đọc và giới hạn

- Ở chế độ thường, script dựng lại một hàng đợi tuyến tính từ từng dòng log và kiểm tra: mỗi ID 0..P·K−1 được thêm và lấy đúng một lần; dãy lấy ra trùng dãy đưa vào (FIFO); ID p·K+j do đúng producer p đưa vào theo thứ tự j; count sau mỗi thao tác khớp số thêm trừ số lấy và nằm trong [0, N]; mỗi consumer nhận đúng một STOP; tổng kết theo luồng khớp log. Ở chế độ quiet không có dòng log, nên chỉ kiểm tra tổng kết và phần tự kiểm chứng của chương trình.
- Chạy lặp chỉ lưu dòng tổng kết của từng lần để log gọn; toàn bộ dòng log vẫn được kiểm tra trong bộ nhớ trước khi ghi PASS.
- WAIT_SLOT/WAIT_ITEM nghĩa là `sem_trywait` trả `EAGAIN` (luồng thiếu quyền và phải gọi `sem_wait`), không phải số đo thời gian bị chặn. Snapshot count = N hoặc 0 trong `guard` mới là bằng chứng bộ đệm đầy/rỗng.
- ThreadSanitizer chạy với `setarch x86_64 -R` (tắt ASLR cho riêng tiến trình đó), vì kernel WSL2 6.18 làm TSan dừng với lỗi "unexpected memory mapping" khi để ASLR mặc định. TSan và các lần chạy lặp chỉ quan sát những lịch chạy đã thực sự xảy ra, không chứng minh mọi lịch chạy.
- Số lần chờ, số snapshot và cách chia DATA giữa các consumer phụ thuộc bộ lập lịch; số trong bảng là của lần chạy này.
- `review.log` có một dòng cảnh báo CRLF của bản sao làm việc `results/week1/huy-buffer-test.log` trên máy này; Git vẫn coi file sạch và nội dung đã commit không đổi.
