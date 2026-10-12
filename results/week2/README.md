# Minh chứng Tuần 2

**W2-C đã nghiệm thu ngày 12/10/2026 (Asia/Saigon) trên máy Châu, phạm vi 1P/1C.** Không xác nhận phần Huy hoặc toàn nhóm hoàn thành Tuần 2. [Đánh giá đầy đủ](chau-validation.md), [báo cáo](../../docs/reports/week2.md), [API bàn giao](../../docs/handoff-week2-chau.md).

| Tệp | Nội dung |
| --- | --- |
| [chau-build.log](chau-build.log) | Môi trường/Git, chuẩn hóa mtime, clean/build không cảnh báo, ví dụ Tuần 1 và buffer 12/12 |
| [chau-runtime-tests.log](chau-runtime-tests.log) | Main mặc định và tám preset; record N5/đầy/trống/K0/còn hàng trước STOP, exit từng lệnh |
| [chau-error-tests.log](chau-error-tests.log) | 26 ca wrapper/API, timeout mỗi ca, mã lỗi dự kiến và lifecycle/cấp phát |
| [chau-sanitizers.log](chau-sanitizers.log) | ASan/UBSan/leak detection ngoài sandbox: lõi 8 ca, production mặc định, buffer 11 ca |
| [chau-sanitizer-sandbox.log](chau-sanitizer-sandbox.log) | Lần LSan lỗi dưới ptrace trong sandbox, giữ để giải thích giới hạn môi trường |
| [chau-sandbox-source.sha256](chau-sandbox-source.sha256) | Manifest cho lần sandbox lỗi; script minh chứng chưa có bước chuẩn hóa mtime sau build, nguồn C/header/test giống bản cuối |
| [chau-review.log](chau-review.log) | Scaffold, diff/check, bảo toàn Tuần 1, hash nguồn và rà soát cuối |
| [chau-source.sha256](chau-source.sha256) | Hash 13 file build/nguồn/test/script thực chạy |
| [chau-test-summary.json](chau-test-summary.json) | Tóm tắt máy đọc được của lần kiểm chứng cuối thành công |

Mỗi log ghi HEAD nền `9cf345c…` và hash nguồn working tree trước commit; không gán code mới cho commit nền. Chạy lại bằng `python3 scripts/validate_week2.py` (ghi đè minh chứng Tuần 2), đối chiếu hash bằng `sha256sum -c results/week2/chau-source.sha256`. Binary/object/cache chỉ ở build/bin hoặc /tmp, không đưa vào Git. Log Tuần 1 giữ nguyên.
