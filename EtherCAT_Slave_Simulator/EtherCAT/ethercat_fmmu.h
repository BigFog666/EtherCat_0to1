#ifndef ETHERCAT_FMMU_H
#define ETHERCAT_FMMU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* READ/WRITE 以 EtherCAT 侧为参照：读反馈、写命令。 */
typedef enum {
    EC_FMMU_READ,
    EC_FMMU_WRITE
} EC_FMMU_Access;

/* 一条按整字节配置的映射规则，不是实际 ESC 寄存器布局。
 * 一个连续逻辑区间，映射到一个连续本地区间。
 * 位级映射、多个从站和真正的报文解析留到后续。 */
typedef struct {
    uint32_t logical_start;
    uint16_t physical_start;
    uint16_t length;
    bool enabled;
    bool read_enabled;
    bool write_enabled;
} EC_FMMU;

/* 验证映射、方向和访问区间，再计算本地地址。
 * physical = physical_start + (logical_address - logical_start)。
 * 本函数只算地址，不搬数据，也不理解 0x607A 等对象索引。
 * 成功才填写 physical_address，失败保留调用者原来的数值。 */
bool EC_FMMU_Translate(const EC_FMMU *fmmu, EC_FMMU_Access access,
                       uint32_t logical_address, size_t count,
                       uint16_t *physical_address);

#endif
