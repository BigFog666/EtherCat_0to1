#include "watchdog.h"
#include <stddef.h>

bool Watchdog_Init(ProcessWatchdog *watchdog, uint32_t now_ms, uint32_t timeout_ms)
{
    if (watchdog == NULL || timeout_ms == 0u || timeout_ms >= 0x80000000u) return false;
    *watchdog = (ProcessWatchdog){.last_valid_ms = now_ms, .timeout_ms = timeout_ms};
    return true;
}
bool Watchdog_Update(ProcessWatchdog *watchdog, uint32_t now_ms, bool valid_pdo, bool *expired)
{
    if (watchdog == NULL || expired == NULL || watchdog->timeout_ms == 0u ||
        watchdog->timeout_ms >= 0x80000000u) return false;
    if (valid_pdo) watchdog->last_valid_ms = now_ms;
    /* 无符号减法按模 2^32 计算，可跨一次计数回绕。
     * 前提是调用时间单调、跨度足够短，不能拿日历跳变当时间源。 */
    uint32_t elapsed = now_ms - watchdog->last_valid_ms;
    watchdog->expired = elapsed >= watchdog->timeout_ms;
    *expired = watchdog->expired;
    return true;
}
