# scripts

[check_scaffold.py](check_scaffold.py) kiểm tra khung/liên kết/ignore, không nghiệm thu thuật toán. [check_pc_errors.py](check_pc_errors.py) giữ 26 ca lỗi/API Châu.

[check_cli_log.py](check_cli_log.py) kiểm tra CLI/log production, 8 cấu hình thường + quiet, 116 đầu vào sai trên production + wrapper chặn lõi, help/biên/formatter/main errors. Chạy `make test-cli`; có timeout mỗi tiến trình, không ghi đè minh chứng.

[validate_week2_huy.py](validate_week2_huy.py) clean/build, hồi quy Tuần 1/W2-C, suite CLI/log, ASan/UBSan/leak detection; chỉ ghi huy-* kèm môi trường/time/hash vào results/week2. Chạy trên dev/huy; bản thực đã chạy ngoài sandbox để LSan hoạt động. [Đánh giá Huy](../results/week2/huy-validation.md).

[validate_week2.py](validate_week2.py) là writer lịch sử Châu, giữ nguyên byte để tái lập bản bd00780. **Không chạy trên CLI mới:** nó ghi đè chau-* và kỳ vọng đối số bị từ chối. Các [minh chứng Châu](../results/week2/chau-validation.md) và results/week1 được bảo toàn.
