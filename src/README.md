# src

| File | Vai trò |
| --- | --- |
| [buffer.c](buffer.c) | Bộ đệm vòng FIFO tuần tự, không tự khóa ([hợp đồng](../docs/buffer-week1-huy.md)) |
| [pc.c](pc.c) | Lõi: P producer, C consumer, semaphore empty/full/guard, C tín hiệu STOP, bản ghi trong guard, tự kiểm chứng sau join ([thiết kế](../docs/design.md)) |
| [cli.c](cli.c) | Đọc và kiểm tra tham số trước khi chạy lõi |
| [runtime_log.c](runtime_log.c) | In bản ghi và tổng kết sau khi mọi luồng đã join; quiet vẫn in tổng kết |
| [main.c](main.c) | parse → `pc_run` → in log → giải phóng kết quả; mã thoát 0/1/2 |

`make` tạo `bin/producer-consumer`; test, ví dụ và wrapper có binary riêng. Cách dùng: [run-guide.md](../docs/run-guide.md).
