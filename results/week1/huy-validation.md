# Đánh giá W1-H — Nguyễn Đức Huy

Ngày thực chạy **08/10/2026**, theo Asia/Saigon; không gán ngày bắt đầu/hạn nộp. Ubuntu 24.04.5 LTS/WSL2, GCC 13.3.0, GNU Make 4.3, Git 2.43.0, Python 3.12.3 trên máy Huy. [huy-environment.log](huy-environment.log) và [huy-buffer-test.log](huy-buffer-test.log) lưu lệnh, cấu hình, đầu ra, exit code và SHA nguồn thật.

Working tree ban đầu sạch. Git root Windows `D:/os-producer-consumer`, trong WSL `/mnt/d/os-producer-consumer`. Fetch origin rồi chuyển nhánh đã tồn tại `dev/huy`, đồng bộ origin/dev/huy, fast-forward origin/main đến `a004c2ddf90999486f87126260c70087026b8972`. Không tạo nhánh mới, không sửa dev/chau/main hoặc reset/force-push. PR #1 được kiểm tra bằng GitHub connector: open, merged=false, head `2c8d7f61415c1dc730c8e4dbde4c991a37bfdfa6`.

Đã đọc từ origin/dev/chau: docs/design.md, docs/theory-week1-chau.md, examples/thread_lifecycle.c, Makefile; đây là đối chiếu nội dung repo, không tuyên bố đã đọc lại các nguồn lý thuyết bên ngoài. Dùng chữ ký API, enum item và K DATA mỗi producer như Châu đề xuất. Không merge PR #1, không copy lý thuyết/giả mã/ví dụ của Châu vào repo nhánh Huy.

## Kết quả thực chạy

| Lệnh/cấu hình | Kết quả | Mã thoát |
| --- | --- | --- |
| uname, /etc/os-release, gcc/make/git/python3 --version | Kiểm tra thực tế các công cụ Ubuntu/WSL nêu trên, không thiếu công cụ | 0 mỗi lệnh |
| gcc C11, -Wall -Wextra -Wpedantic -pthread trên ví dụ Châu trong /tmp | Compile/link không diagnostics; đúng nguồn origin/dev/chau/SHA và hash trong log | 0 |
| timeout 5s ./thread-demo ngoài repo | 55/210/465; mỗi worker và join đúng một lần; completed cuối cùng | 0 |
| make help | Hướng dẫn buffer-test/test-buffer và phạm vi tuần tự | 0 |
| make clean; make buffer-test; make test-buffer — hai chu kỳ | Cả hai build sạch không cảnh báo, -pthread compile/link; 12/12 PASS mỗi lần | 0 mỗi lệnh |
| make test | Gọi test-buffer, ghi rõ chỉ kiểm thử tuần tự; 12/12 PASS | 0 |
| make mặc định | Báo chưa có src/main.c/chưa triển khai; không link buffer.c thành chương trình giả | 2 đúng dự kiến |
| gcc -fsanitize=address,undefined rồi timeout 10s binary tạm | 11/11 ca thường PASS, không diagnostics; wrapper cấp phát không bật ở bản này | 0 cả hai lệnh |

Ca wrapper tiêm malloc=NULL có chủ đích để kiểm tra BUFFER_NO_MEMORY, zero-state, destroy an toàn và phục hồi lần init tiếp theo; không tuyên bố hệ thống thật hết bộ nhớ. Đếm malloc/free từ code linked, yêu cầu không còn cấp phát mảng sau từng ca. Sanitizer kiểm tra thêm lỗi truy cập/UB ở đường chạy bình thường. Không coi những kiểm tra hữu hạn này là chứng minh mọi đầu vào.

Oracle FIFO đối chiếu kind/id/value: dãy ID không tăng 90,2,77; wrap-around 100 lượt dùng queue tuyến tính độc lập; N=1 qua 50 chu kỳ; từ chối đầy/rỗng không đổi state, DATA cũ hay out. STOP có ID/value khác 0 vẫn đi qua API bình thường. Kiểm tra invalid kind/NULL/N=0/trước init/sau destroy, overflow, init lại khi sống và destroy/lặp/tái sử dụng.

Nguồn bộ đệm thực chạy: HEAD nền `a004c2d` cộng các file W1-H chưa commit. Log ghi SHA256 `include/item.h`, `include/buffer.h`, `src/buffer.c`, `tests/test_buffer.c`, `Makefile` trước/sau kiểm thử; hash không đổi. SHA commit bàn giao sẽ được xác nhận sau commit trong PR/phần trả kết quả, không dùng SHA nền để gán cho code mới.

WSL/Windows có chênh mtime: trước đọc lại dependency, đã chuẩn hóa mtime file build phát sinh về đồng hồ WSL, không đổi nội dung. Thao tác được ghi trong log; không che cảnh báo compiler hay sửa thuật toán để làm test xanh. Hai build sạch đều không cảnh báo compiler/make.

## Kiểm tra tài liệu và Git

Lệnh `python3 scripts/check_scaffold.py` kiểm tra cấu trúc, toàn bộ liên kết Markdown tương đối, ignore và AGENTS.md tracked/staged; `git diff --check`, `git diff --cached --check`, `git ls-files`, danh sách staged và staged diff được rà soát trước commit. Đầu ra thực tế tại cuối [huy-buffer-test.log](huy-buffer-test.log). Các lệnh khung không kiểm chứng thuật toán.

Kết quả: scaffold exit 0, 23 file bắt buộc và 105 liên kết tương đối; cả hai diff-check exit 0. Kiểm tra tự động xác nhận phần Châu/dòng W1-C nguyên vẹn, đủ sáu phần báo cáo, không có ví dụ trùng và cả năm hash nguồn/Makefile vẫn khớp. Danh sách staged chỉ có 19 file W1-H, không AGENTS.md hoặc sản phẩm build.

Giữ sáu phần báo cáo; phần Châu và dòng W1-C ở nhánh nền nguyên vẹn. Tài liệu ghi rõ PR #1 còn mở và cần giữ target thread-demo khi tích hợp Makefile, thay vì thêm bản sao ví dụ. Không có meeting, secret, PDF, AGENTS.md, binary, object, cache hoặc script tạm trong phần bàn giao. Chỉ stage phần W1-H, reviewer quyết định merge; giữ cả hai nhánh thành viên.

## Giới hạn kết luận và việc cần xác nhận

W1-H **Đã hoàn thành phần độc lập**: môi trường, buffer tuần tự, Makefile/test, đề xuất cấu hình/log và test-plan. Chờ Châu xác nhận [hợp đồng API](../../docs/buffer-week1-huy.md) (zero-init, lỗi trực tiếp, sở hữu mảng, out không alias), [CLI/log](../../docs/config-log-week1-huy.md) (K mỗi producer, mặc định/giới hạn, metadata/thứ tự trong guard và quan sát chờ semaphore), đọc chéo và tích hợp hai PR. Chưa có bằng chứng nhóm nghiệm thu mốc chung.

T02–T09 chưa chạy; chưa có producer/consumer, lớp semaphore, CLI parser hoặc runtime log. STOP chỉ đã được kiểm thử lưu/lấy tuần tự, không chứng minh consumer dừng. Ví dụ luồng chỉ chứng minh create/join/kết quả và môi trường máy Huy. Chưa tiêm lỗi create/join trong lần kiểm tra của Huy.

## Chạy lại

Từ Git root trong Ubuntu/WSL:

```sh
make clean
make buffer-test
make test-buffer
make test
python3 scripts/check_scaffold.py
git diff --check
```

Kỳ vọng test 12/12 PASS, exit 0; make mặc định vẫn exit 2. Sanitizer độc lập, không bật wrapper:

```sh
tmpdir=$(mktemp -d)
gcc -Iinclude -std=c11 -Wall -Wextra -Wpedantic -g -O1 \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    src/buffer.c tests/test_buffer.c -o "$tmpdir/buffer-test"
timeout 10s "$tmpdir/buffer-test"
```

Kỳ vọng 11/11 PASS, exit 0. Binary nằm ngoài repo; dọn đúng thư mục tạm đã tạo sau khi kiểm tra.

Nếu PR #1 chưa tích hợp, chạy lại đúng ví dụ nguồn đã kiểm tra từ thư mục tạm, không copy vào repo:

```sh
source_sha=2c8d7f61415c1dc730c8e4dbde4c991a37bfdfa6
tmpdir=$(mktemp -d)
git show "$source_sha:examples/thread_lifecycle.c" > "$tmpdir/thread_lifecycle.c"
gcc -std=c11 -Wall -Wextra -Wpedantic -pthread \
    "$tmpdir/thread_lifecycle.c" -o "$tmpdir/thread-demo"
timeout 5s "$tmpdir/thread-demo"
```

Sau tích hợp dùng `make thread-demo` và `timeout 5s ./bin/thread-demo`; vẫn cần ghi đúng SHA được kiểm tra khi nguồn thay đổi.
