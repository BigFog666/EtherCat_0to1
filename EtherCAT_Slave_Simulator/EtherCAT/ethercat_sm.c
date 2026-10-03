#include "ethercat_sm.h"

#include <string.h> /* memmove 将一整份字节复制到目标数组。 */

/* 先验证配置和访问范围，保证失败时原数据不变。
 * 真实 SM 支持更复杂的缓冲切换；这里先只接收一份完整 PDO。 */
static EC_SM_Result CheckAccess(const EC_SyncManager *sm, EC_SM_Side side,
                                uint16_t address, size_t count, bool writing)
{
    if (sm == NULL ||
        (side != EC_SM_ETHERCAT_SIDE && side != EC_SM_PDI_SIDE) ||
        (sm->direction != EC_SM_RXPDO && sm->direction != EC_SM_TXPDO)) {
        return EC_SM_INVALID_ARGUMENT;
    }
    if (!sm->enabled) {
        return EC_SM_DISABLED;
    }
    if (sm->physical_start > UINT16_MAX - (EC_SM_PROCESS_SIZE - 1u) ||
        address != sm->physical_start || count != EC_SM_PROCESS_SIZE) {
        return EC_SM_OUT_OF_RANGE;
    }

    /* Rx 通道的生产者是 EtherCAT 侧，Tx 通道的生产者是 PDI 侧。
     * 生产者只写、消费者只读；这里的 bool 保存本次是否来自生产者。 */
    bool producer = (sm->direction == EC_SM_RXPDO)
                        ? (side == EC_SM_ETHERCAT_SIDE)
                        : (side == EC_SM_PDI_SIDE);
    if (writing != producer) {
        return EC_SM_WRONG_DIRECTION;
    }
    return EC_SM_OK;
}

EC_SM_Result EC_SM_Init(EC_SyncManager *sm, uint16_t physical_start,
                        EC_SM_Direction direction)
{
    if (sm == NULL ||
        physical_start > UINT16_MAX - (EC_SM_PROCESS_SIZE - 1u) ||
        (direction != EC_SM_RXPDO && direction != EC_SM_TXPDO)) {
        return EC_SM_INVALID_ARGUMENT;
    }
    *sm = (EC_SyncManager){
        .physical_start = physical_start,
        .direction = direction,
        .enabled = true,
        .has_data = false,
        .bytes = {0}
    };
    return EC_SM_OK;
}

EC_SM_Result EC_SM_Write(EC_SyncManager *sm, EC_SM_Side side,
                         uint16_t physical_address, const uint8_t *source,
                         size_t count)
{
    if (source == NULL) {
        return EC_SM_INVALID_ARGUMENT;
    }
    EC_SM_Result result = CheckAccess(sm, side, physical_address, count, true);
    if (result != EC_SM_OK) {
        return result; /* 校验失败，不复制任何字节，也不发布数据。 */
    }
    memmove(sm->bytes, source, count);
    sm->has_data = true; /* 复制完整份数据之后，才标记有有效数据。 */
    return EC_SM_OK;
}

EC_SM_Result EC_SM_Read(const EC_SyncManager *sm, EC_SM_Side side,
                        uint16_t physical_address, uint8_t *destination,
                        size_t count)
{
    if (destination == NULL) {
        return EC_SM_INVALID_ARGUMENT;
    }
    EC_SM_Result result = CheckAccess(sm, side, physical_address, count, false);
    if (result != EC_SM_OK) {
        return result;
    }
    if (!sm->has_data) {
        return EC_SM_NO_DATA; /* 失败时不修改调用者的接收缓冲区。 */
    }
    memmove(destination, sm->bytes, count);
    return EC_SM_OK;
}

const char *EC_SM_ResultName(EC_SM_Result result)
{
    switch (result) {
    case EC_SM_OK: return "OK";
    case EC_SM_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
    case EC_SM_DISABLED: return "DISABLED";
    case EC_SM_OUT_OF_RANGE: return "OUT_OF_RANGE";
    case EC_SM_WRONG_DIRECTION: return "WRONG_DIRECTION";
    case EC_SM_NO_DATA: return "NO_DATA";
    default: return "UNKNOWN";
    }
}
