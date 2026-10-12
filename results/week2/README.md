# Minh chứng Tuần 2

**W2-C và W2-H đã nghiệm thu ngày 12/10/2026 (Asia/Saigon), phạm vi 1P/1C; bản ghép đạt mốc kỹ thuật Tuần 2 trên dev/huy, chưa merge main.** Codex thực hiện theo yêu cầu trên máy ChouPro WSL; không ghi Huy trực tiếp chạy/phê duyệt. [Đánh giá Châu](chau-validation.md), [đánh giá Huy](huy-validation.md), [báo cáo](../../docs/reports/week2.md), [API bàn giao Châu](../../docs/handoff-week2-chau.md).

Các chau-* dưới đây giữ nguyên byte, là snapshot bàn giao trước CLI.

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

Log Châu ghi HEAD nền 9cf345c và hash nguồn W2-C trước commit; sau đó bàn giao ở bd00780. **Không chạy scripts/validate_week2.py trên bản CLI mới:** nó ghi đè chau-* và kỳ vọng đối số bị từ chối. Checksum Châu chỉ áp dụng snapshot cũ, không toàn bộ Makefile/main đã ghép Huy.

| Tệp Huy | Nội dung |
| --- | --- |
| [huy-build.log](huy-build.log) | Môi trường/Git/mtime, clean/build/help và target tổng make test |
| [huy-regression-tests.log](huy-regression-tests.log) | Thread-demo, buffer 12, lõi Châu 8 và lỗi/API Châu 26 |
| [huy-runtime-tests.log](huy-runtime-tests.log) | CLI production: tám cấu hình thường + tám quiet, toàn bộ command/record/summary/exit |
| [huy-cli-tests.log](huy-cli-tests.log) | 116 đầu vào sai × production/hook, help, positive control, năm biên parser và tóm tắt assertion |
| [huy-log-tests.log](huy-log-tests.log) | Formatter 7 ca và main errors 6 ca, gồm cleanup lỗi nhưng verified=1 |
| [huy-sanitizers.log](huy-sanitizers.log) | ASan/UBSan/leak detection cho production/parser/logger/main cleanup mới |
| [huy-review.log](huy-review.log) | Scaffold/diff/hash nguồn, bảo toàn Châu/Tuần 1, rà soát cuối |
| [huy-source.sha256](huy-source.sha256) | 22 file build/nguồn/header/test/script thực dùng |
| [huy-test-summary.json](huy-test-summary.json) | Kết quả máy đọc được, cấu hình và số snapshot/chờ thực |
| [huy-validation.md](huy-validation.md) | Nghiệm thu W2-H, phạm vi, lệnh và giới hạn |

Các huy-* ghi HEAD nền bd00780 cộng nguồn working tree mới/hash, không gán code mới cho commit nền. Tái lập trên dev/huy bằng `python3 scripts/validate_week2_huy.py` (ghi lại huy-*), đối chiếu bằng `sha256sum -c results/week2/huy-source.sha256`. Không chạy writer Châu. Binary/object/cache chỉ build/bin hoặc /tmp; Tuần 1 và chau-* được giữ nguyên.
