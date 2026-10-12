# src

[buffer.c](buffer.c) giữ nguyên API tuần tự của Huy. [pc.c](pc.c) bao quanh bằng empty/full/guard, chạy 1P/1C hữu hạn và trả record/kiểm chứng sau join. [main.c](main.c) là entry point thật, chạy mặc định N=8,K=20; CLI parser chờ Huy. `make` tạo bin/producer-consumer; test/ví dụ được build riêng. Xem [hợp đồng buffer](../docs/buffer-week1-huy.md) và [API bàn giao](../docs/handoff-week2-chau.md).
