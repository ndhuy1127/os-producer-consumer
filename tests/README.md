# tests

[test_buffer.c](test_buffer.c) gồm 12 ca tuần tự, chạy bằng `make test-buffer` hoặc `make test`. Kiểm tra từng kind/id/value với oracle FIFO độc lập, lỗi giữ nguyên dữ liệu, biên N=1, STOP, vòng đời, tràn kích thước và malloc thất bại được tiêm bằng GNU ld wrapper; kiểm tra mảng được free sau mỗi ca. Exit 0 chỉ khi tất cả ca PASS; lỗi trả EXIT_FAILURE.

Bản sanitizer không bật wrapper chạy 11 ca thường. [Minh chứng](../results/week1/huy-validation.md) ghi lệnh thực chạy. Các test Producer–Consumer, semaphore và CLI vẫn [dự kiến/chưa chạy](../docs/test-plan.md).
