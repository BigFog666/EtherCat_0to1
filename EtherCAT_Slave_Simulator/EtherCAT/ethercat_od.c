#include "ethercat_od.h"

EC_OD_Result EC_OD_Init(EC_ObjectDictionary *od,
                        int32_t *target_position, int32_t *actual_position)
{
    if (od == 0 || target_position == 0 || actual_position == 0) {
        return EC_OD_INVALID_ARGUMENT;
    }

    /* 建立第一行：0x607A:00 对应从站目标位置，可以读写。
     * 括号中的 EC_OD_Entry 表示构造一行描述，再赋给数组元素。 */
    od->entries[0] = (EC_OD_Entry){
        .index = 0x607A,
        .subindex = 0,
        .data_type = EC_OD_INTEGER32,
        .access = EC_OD_RW,
        .value = target_position
    };

    /* 第二行：0x6064:00 对应从站实际位置，对外只读。
     * 只读限制作用于下面的 WriteI32 接口，不禁止编码器代码更新变量。 */
    od->entries[1] = (EC_OD_Entry){
        .index = 0x6064,
        .subindex = 0,
        .data_type = EC_OD_INTEGER32,
        .access = EC_OD_RO,
        .value = actual_position
    };
    return EC_OD_OK;
}

/* 查表：索引和子索引必须同时匹配。
 * 返回的是「这一行描述的地址」，找不到则返回空指针。 */
static const EC_OD_Entry *FindEntry(const EC_ObjectDictionary *od,
                                  uint16_t index, uint8_t subindex)
{
    for (unsigned int i = 0; i < EC_OD_ENTRY_COUNT; ++i) {
        const EC_OD_Entry *entry = &od->entries[i];
        if (entry->index == index && entry->subindex == subindex) {
            return entry;
        }
    }
    return 0;
}

EC_OD_Result EC_OD_ReadI32(const EC_ObjectDictionary *od,
                          uint16_t index, uint8_t subindex, int32_t *value)
{
    if (od == 0 || value == 0) {
        return EC_OD_INVALID_ARGUMENT;
    }
    const EC_OD_Entry *entry = FindEntry(od, index, subindex);
    if (entry == 0) {
        return EC_OD_NOT_FOUND;
    }
    if (entry->value == 0) {
        return EC_OD_INVALID_ARGUMENT;
    }
    /* entry->value 指向从站变量；*entry->value 取出变量当前的数值。
     * *value 把读到的数值写到调用者提供的接收变量里。 */
    *value = *entry->value;
    return EC_OD_OK;
}

EC_OD_Result EC_OD_WriteI32(const EC_ObjectDictionary *od,
                           uint16_t index, uint8_t subindex, int32_t value)
{
    if (od == 0) {
        return EC_OD_INVALID_ARGUMENT;
    }
    const EC_OD_Entry *entry = FindEntry(od, index, subindex);
    if (entry == 0) {
        return EC_OD_NOT_FOUND;
    }
    /* 先检查权限，避免把只读的反馈量当成命令改掉。 */
    if (entry->access != EC_OD_RW) {
        return EC_OD_READ_ONLY;
    }
    if (entry->value == 0) {
        return EC_OD_INVALID_ARGUMENT;
    }
    /* 沿指针修改原来的从站变量，所以字典与 PDO 使用同一份数据。 */
    *entry->value = value;
    return EC_OD_OK;
}

/* 仅用于打印操作结果，本课可以先跳过。 */
const char *EC_OD_ResultName(EC_OD_Result result)
{
    switch (result) {
    case EC_OD_OK: return "OK";
    case EC_OD_NOT_FOUND: return "NOT_FOUND";
    case EC_OD_READ_ONLY: return "READ_ONLY";
    case EC_OD_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
    default: return "UNKNOWN";
    }
}
