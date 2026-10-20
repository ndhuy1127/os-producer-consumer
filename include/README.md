# include

| Header | Nội dung |
| --- | --- |
| [item.h](item.h) | `struct item` gồm kind DATA/STOP, id, value |
| [buffer.h](buffer.h) | API bộ đệm tuần tự và mã lỗi |
| [pc.h](pc.h) | Giới hạn chung (`PC_MAX_*`), cấu hình, bản ghi thao tác (có `actor_index`), tổng kết (có số liệu theo từng luồng), `pc_run`, `pc_result_destroy` |
| [cli.h](cli.h) | `cli_parse`, `cli_print_help`, `struct cli_options` |
| [runtime_log.h](runtime_log.h) | `runtime_log_result`: mượn kết quả sau `pc_run` để in, không sở hữu bộ nhớ |

Đặc tả tham số/log: [config-log-week1-huy.md](../docs/config-log-week1-huy.md); thay đổi Tuần 3: [design.md §11](../docs/design.md#11-tuần-3--nhiều-producerconsumer).
