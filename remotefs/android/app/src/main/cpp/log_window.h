#ifndef LOG_WINDOW_H
#define LOG_WINDOW_H

#include <stdint.h>

struct log_window_shared_data_s {
    uint64_t lw_epoch;
    uint32_t lw_current;
    uint32_t lw_rows;
    uint32_t lw_columns;
    uint8_t  lw_data[];
};

/* Set by nativeInitLogWindow, used by log_fmt() in remotefs.c */
extern struct log_window_shared_data_s *volatile log_window;

/* Called from remotefs.c run_service() to start/stop Java polling */
extern void notify_java_enable_polling (int enable);

#endif
