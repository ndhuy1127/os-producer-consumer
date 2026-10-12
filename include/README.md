# include

[item.h](item.h), [buffer.h](buffer.h) và [pc.h](pc.h) giữ nguyên API DATA/STOP, buffer và lõi Châu. [cli.h](cli.h) công bố cli_parse/cli_print_help và cli_options (config/quiet/help); [runtime_log.h](runtime_log.h) mượn result sau pc_run để in record/tổng kết, không sở hữu bộ nhớ. [Đặc tả thực](../docs/config-log-week1-huy.md), [minh chứng](../results/week2/huy-validation.md).
