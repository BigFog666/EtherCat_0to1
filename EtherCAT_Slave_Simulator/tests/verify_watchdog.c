#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "../Further_Lessons/watchdog.h"

int main(void)
{
    ProcessWatchdog wd;
    bool expired = true;
    assert(!Watchdog_Init(NULL, 0, 3));
    assert(!Watchdog_Init(&wd, 0, 0));
    assert(!Watchdog_Init(&wd, 0, 0x80000000u));
    assert(Watchdog_Init(&wd, 0, 3));
    assert(Watchdog_Update(&wd, 2, false, &expired) && !expired);
    assert(Watchdog_Update(&wd, 3, false, &expired) && expired); /* 等号边界。 */
    assert(Watchdog_Update(&wd, 4, true, &expired) && !expired); /* 数据健康可恢复。 */
    assert(wd.last_valid_ms == 4);
    assert(Watchdog_Update(&wd, 6, false, &expired) && !expired);
    assert(Watchdog_Update(&wd, 7, false, &expired) && expired);
    assert(wd.last_valid_ms == 4); /* 缺数据不能刷新。 */
    assert(Watchdog_Update(&wd, 7, true, &expired) && !expired); /* 本轮先到数据再检查。 */
    assert(Watchdog_Init(&wd, UINT32_MAX - 1, 3));
    assert(Watchdog_Update(&wd, 0, false, &expired) && !expired);
    assert(Watchdog_Update(&wd, 1, false, &expired) && expired); /* 跨计数回绕。 */
    uint32_t before = wd.last_valid_ms;
    assert(!Watchdog_Update(&wd, 2, true, NULL));
    assert(wd.last_valid_ms == before && wd.expired);
    wd.timeout_ms = 0;
    expired = true;
    assert(!Watchdog_Update(&wd, 2, true, &expired));
    assert(expired && wd.last_valid_ms == before);
    puts("Watchdog checks passed: startup grace, equality boundary, fresh-only feed, receive-before-check, wraparound, rejected inputs");
    return 0;
}
