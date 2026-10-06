#include <stddef.h>
#include <stdint.h>
#include <errno.h>
extern unsigned char _heap_start, _heap_end;
/* SSC 启动时分配轴对象字典；为 newlib 提供有界堆，禁止越过保留区。 */
void *_sbrk(ptrdiff_t increment)
{
    static uintptr_t current;
    uintptr_t old, begin=(uintptr_t)&_heap_start, end=(uintptr_t)&_heap_end;
    if(!current) current=begin;
    old=current;
    if((increment>=0 && (uintptr_t)increment>end-current) ||
       (increment<0 && (uintptr_t)(-(increment+1))+1>current-begin)) {
        errno=ENOMEM; return (void *)-1;
    }
    current=(uintptr_t)((intptr_t)current+increment);
    return (void *)old;
}
