/* 第九课的验证代码，可暂时跳过；课堂入口仍是 main.c。
 * 使用独立数据测试边界和失败保护，不改动学习者的练习变量。 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../EtherCAT/ethercat_fmmu.h"
#include "../EtherCAT/ethercat_sm.h"

static void VerifyMapping(void)
{
    EC_FMMU map = {
        .logical_start = 0x20, .physical_start = 0x1000, .length = 6,
        .enabled = true, .read_enabled = false, .write_enabled = true
    };
    uint16_t address = 0;
    assert(EC_FMMU_Translate(&map, EC_FMMU_WRITE, 0x20, 6, &address));
    assert(address == 0x1000);
    /* 目标位置在 PDO 内偏移 2；FMMU 只做地址换算，不解读目标数值。 */
    assert(EC_FMMU_Translate(&map, EC_FMMU_WRITE, 0x22, 4, &address));
    assert(address == 0x1002);
    assert(EC_FMMU_Translate(&map, EC_FMMU_WRITE, 0x25, 1, &address));
    assert(address == 0x1005);
    address = 0xAAAA;
    assert(!EC_FMMU_Translate(&map, EC_FMMU_WRITE, 0x1F, 1, &address));
    assert(!EC_FMMU_Translate(&map, EC_FMMU_WRITE, 0x26, 1, &address));
    assert(!EC_FMMU_Translate(&map, EC_FMMU_WRITE, 0x22, 5, &address));
    assert(!EC_FMMU_Translate(&map, EC_FMMU_WRITE, 0x20, SIZE_MAX, &address));
    assert(!EC_FMMU_Translate(&map, EC_FMMU_READ, 0x20, 6, &address));
    assert(!EC_FMMU_Translate(&map, (EC_FMMU_Access)99, 0x20, 6, &address));
    assert(!EC_FMMU_Translate(&map, EC_FMMU_WRITE, 0x20, 0, &address));
    assert(!EC_FMMU_Translate(NULL, EC_FMMU_WRITE, 0x20, 6, &address));
    assert(!EC_FMMU_Translate(&map, EC_FMMU_WRITE, 0x20, 6, NULL));
    assert(address == 0xAAAA); /* 失败不能把旧输出地址改成半成品。 */
    map.enabled = false;
    assert(!EC_FMMU_Translate(&map, EC_FMMU_WRITE, 0x20, 6, &address));
    map.enabled = true;
    map.length = 0;
    assert(!EC_FMMU_Translate(&map, EC_FMMU_WRITE, 0x20, 1, &address));
    map.length = 6;
    map.logical_start = UINT32_MAX - 4u; /* 整条映射跨越 32 位上限。 */
    assert(!EC_FMMU_Translate(&map, EC_FMMU_WRITE, map.logical_start, 1, &address));
    map.logical_start = UINT32_MAX - 5u;
    map.physical_start = UINT16_MAX - 4u; /* 本地映射跨越 16 位上限。 */
    assert(!EC_FMMU_Translate(&map, EC_FMMU_WRITE, map.logical_start, 1, &address));
    map.physical_start = UINT16_MAX - 5u;
    assert(EC_FMMU_Translate(&map, EC_FMMU_WRITE, UINT32_MAX, 1, &address));
    assert(address == UINT16_MAX); /* 恰好触及上限仍合法。 */
    puts("FMMU checks passed: offsets, permissions, bounds, overflow, failure output");
}

static void VerifyProcessData(void)
{
    const uint8_t first[6] = {0, 0, 0x28, 0x23, 0, 0}; /* 9000 的完整 PDO。 */
    const uint8_t latest[6] = {0x34, 0x12, 0x10, 0x27, 0, 0}; /* 新控制字和 10000。 */
    uint8_t received[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    const uint8_t untouched[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    EC_SyncManager rx = {0};
    EC_SyncManager tx = {0};
    assert(EC_SM_Init(&rx, 0x1000, EC_SM_RXPDO) == EC_SM_OK);
    assert(EC_SM_Init(&tx, 0x1010, EC_SM_TXPDO) == EC_SM_OK);
    assert(EC_SM_Read(&rx, EC_SM_PDI_SIDE, 0x1000, received, 6) == EC_SM_NO_DATA);
    assert(memcmp(received, untouched, 6) == 0);
    assert(EC_SM_Write(&rx, EC_SM_ETHERCAT_SIDE, 0x1000, first, 6) == EC_SM_OK);
    /* 过程数据可连续更新，不需要等消费者读取上一份。 */
    assert(EC_SM_Write(&rx, EC_SM_ETHERCAT_SIDE, 0x1000, latest, 6) == EC_SM_OK);
    assert(EC_SM_Read(&rx, EC_SM_PDI_SIDE, 0x1000, received, 6) == EC_SM_OK);
    assert(memcmp(received, latest, 6) == 0);
    assert(EC_SM_Read(&rx, EC_SM_PDI_SIDE, 0x1000, received, 6) == EC_SM_OK);
    assert(memcmp(received, latest, 6) == 0); /* 读取不消耗最新快照。 */

    assert(EC_SM_Write(&rx, EC_SM_PDI_SIDE, 0x1000, first, 6) == EC_SM_WRONG_DIRECTION);
    assert(EC_SM_Read(&rx, EC_SM_ETHERCAT_SIDE, 0x1000, received, 6) == EC_SM_WRONG_DIRECTION);
    assert(EC_SM_Write(&rx, EC_SM_ETHERCAT_SIDE, 0x1001, first, 6) == EC_SM_OUT_OF_RANGE);
    assert(EC_SM_Write(&rx, EC_SM_ETHERCAT_SIDE, 0x1000, first, 5) == EC_SM_OUT_OF_RANGE);
    assert(EC_SM_Write(&rx, EC_SM_ETHERCAT_SIDE, 0x1000, NULL, 6) == EC_SM_INVALID_ARGUMENT);
    assert(memcmp(rx.bytes, latest, 6) == 0 && rx.has_data);
    assert(EC_SM_Write(&tx, EC_SM_ETHERCAT_SIDE, 0x1010, first, 6) == EC_SM_WRONG_DIRECTION);
    assert(EC_SM_Write(&tx, EC_SM_PDI_SIDE, 0x1010, first, 6) == EC_SM_OK);
    assert(EC_SM_Read(&tx, EC_SM_ETHERCAT_SIDE, 0x1010, received, 6) == EC_SM_OK);
    assert(memcmp(received, first, 6) == 0);
    rx.enabled = false;
    assert(EC_SM_Write(&rx, EC_SM_ETHERCAT_SIDE, 0x1000, first, 6) == EC_SM_DISABLED);
    rx.enabled = true;
    assert(EC_SM_Write(NULL, EC_SM_ETHERCAT_SIDE, 0x1000, first, 6) == EC_SM_INVALID_ARGUMENT);
    assert(EC_SM_Read(&rx, EC_SM_PDI_SIDE, 0x1000, NULL, 6) == EC_SM_INVALID_ARGUMENT);
    assert(EC_SM_Write(&rx, (EC_SM_Side)99, 0x1000, first, 6) == EC_SM_INVALID_ARGUMENT);
    assert(EC_SM_Init(&rx, UINT16_MAX, EC_SM_RXPDO) == EC_SM_INVALID_ARGUMENT);
    assert(EC_SM_Init(&rx, 0x1000, (EC_SM_Direction)99) == EC_SM_INVALID_ARGUMENT);
    assert(rx.physical_start == 0x1000 && memcmp(rx.bytes, latest, 6) == 0);

    /* FMMU 算出合法地址，也可能与 SM 的配置不一致，不能据此写别的区。 */
    EC_FMMU wrong_map = {
        .logical_start = 0, .physical_start = 0x1020, .length = 6,
        .enabled = true, .write_enabled = true
    };
    uint16_t physical = 0;
    assert(EC_FMMU_Translate(&wrong_map, EC_FMMU_WRITE, 0, 6, &physical));
    assert(EC_SM_Write(&rx, EC_SM_ETHERCAT_SIDE, physical, first, 6) == EC_SM_OUT_OF_RANGE);
    assert(memcmp(rx.bytes, latest, 6) == 0);
    puts("SM checks passed: first publish, latest snapshot, both directions, rejected writes");
}

int main(void)
{
    VerifyMapping();
    VerifyProcessData();
    return 0;
}
