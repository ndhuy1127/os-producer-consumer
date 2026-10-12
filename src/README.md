# src

[buffer.c](buffer.c) giữ API tuần tự; [pc.c](pc.c) giữ lõi Châu với empty/full/guard và 1P/1C hữu hạn. [cli.c](cli.c) parse/kiểm tra trước lõi; [runtime_log.c](runtime_log.c) đọc/in snapshot sau join, quiet giữ tổng kết. [main.c](main.c) ghép parse → pc_run → log → destroy result. `make` tạo bin/producer-consumer, mặc định N=8/K=20; test/ví dụ và wrapper có binary riêng. [Hợp đồng buffer](../docs/buffer-week1-huy.md), [bàn giao Châu](../docs/handoff-week2-chau.md), [minh chứng bản ghép](../results/week2/huy-validation.md).
