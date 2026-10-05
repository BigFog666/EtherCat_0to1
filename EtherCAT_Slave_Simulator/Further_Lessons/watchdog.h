#ifndef WATCHDOG_H
#define WATCHDOG_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t last_valid_ms;
    uint32_t timeout_ms;
    bool expired;
} ProcessWatchdog;

/* 启动时给予一次 timeout 的等待窗口；timeout 必须处于 1～2^31-1。 */
bool Watchdog_Init(ProcessWatchdog *watchdog, uint32_t now_ms, uint32_t timeout_ms);
/* now_ms 来自单调毫秒计数（可 uint32_t 回绕），不是系统日历时间。
 * valid_pdo 表示本轮确实接受了新过程数据；旧缓存不能喂狗。
 * 新数据在本次检查前到达会刷新时间；elapsed >= timeout 即超时。
 * 超时健康标志可随新数据恢复，但应用的停用锁存由应用独立保持。
 * 错误参数返回 false，保留原 watchdog 和输出。 */
bool Watchdog_Update(ProcessWatchdog *watchdog, uint32_t now_ms, bool valid_pdo, bool *expired);
#endif
