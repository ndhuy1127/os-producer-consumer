# scripts

| Script | Lệnh | Vai trò |
| --- | --- | --- |
| [check_cli_log.py](check_cli_log.py) | `make test-cli` | Chạy chương trình chính 15 cấu hình × thường/quiet và kiểm tra bằng hàng đợi tuyến tính dựng từ log (ID, FIFO, chủ sở hữu và thứ tự j, count, STOP từng consumer, tổng kết theo luồng); 114 tham số sai trên binary thật và binary chặn lõi; help, 6 biên parser, formatter, lỗi ở main |
| [check_stress.py](check_stress.py) | `make test-stress` hoặc `python3 scripts/check_stress.py --runs N` | Chạy lặp 7 cấu hình nhiều luồng (mặc định 20 lần), mỗi lần kiểm tra bằng oracle đầy đủ, in bảng kết quả |
| [check_pc_errors.py](check_pc_errors.py) | `make test-errors` | 32 ca tiêm lỗi, mỗi ca một tiến trình, timeout 5 s, kiểm tra mã thoát và thông báo |
| [validate_week3.py](validate_week3.py) | `python3 scripts/validate_week3.py` | Ghi minh chứng Tuần 3: build sạch, toàn bộ test, chạy lặp, ASan/UBSan và TSan, rà soát; **ghi đè** `results/week3/` |
| [check_scaffold.py](check_scaffold.py) | `python3 scripts/check_scaffold.py` | Kiểm tra cấu trúc, liên kết tương đối, quy tắc ignore; không kiểm tra thuật toán |

Script ghi minh chứng lịch sử, **không chạy lại** trên mã hiện tại:

- [validate_week2.py](validate_week2.py): writer W2-C của Châu, giữ nguyên để đối chiếu bản `bd00780`; chạy lại sẽ ghi đè `results/week2/chau-*` và kỳ vọng CLI chưa tồn tại.
- [validate_week2_huy.py](validate_week2_huy.py): writer W2-H; chạy lại sẽ ghi đè `results/week2/huy-*` bằng kết quả của mã Tuần 3, làm sai lệch minh chứng Tuần 2.

Minh chứng Tuần 2 ([Châu](../results/week2/chau-validation.md), [Huy](../results/week2/huy-validation.md)) và results/week1 được giữ nguyên.
