#ifndef ETHERCAT_OD_H
#define ETHERCAT_OD_H

#include <stdint.h>

/* OD = Object Dictionary，对象字典。
 * 本课先支持两个 32 位有符号位置对象，暂不加入控制字或其他数据类型。 */
#define EC_OD_ENTRY_COUNT 2u

typedef enum {
    EC_OD_INTEGER32 /* 本课所有对象的数据类型，对应 C 的 int32_t。 */
} EC_OD_DataType;

typedef enum {
    EC_OD_RO, /* Read Only：通过对象字典接口只能读。 */
    EC_OD_RW  /* Read/Write：通过对象字典接口可以读、写。 */
} EC_OD_Access;

/* 一行对象描述：对象身份、类型、权限，以及实际变量在哪里。
 * value 是指针，指向已有位置变量，而不是在字典里复制一份数值。 */
typedef struct {
    uint16_t index;           /* 索引，例如 0x607A。不是物理内存地址。 */
    uint8_t subindex;         /* 子索引，本课两个简单变量都用 0。 */
    EC_OD_DataType data_type; /* 变量的类型，本课只有 INTEGER32。 */
    EC_OD_Access access;     /* 字典读写接口是否允许修改这个变量。 */
    int32_t *value;          /* 真正存储位置数值的变量地址。 */
} EC_OD_Entry;

typedef struct {
    EC_OD_Entry entries[EC_OD_ENTRY_COUNT]; /* 两行描述组成一张表。 */
} EC_ObjectDictionary;

/* 本地函数的返回结果，尚不是 SDO 协议中的错误码。 */
typedef enum {
    EC_OD_OK,
    EC_OD_NOT_FOUND,
    EC_OD_READ_ONLY,
    EC_OD_INVALID_ARGUMENT
} EC_OD_Result;

/* 初始化时关联从站的目标位置和实际位置；变量必须在字典使用期间有效。
 * 不改变这两个变量已有的数值。 */
EC_OD_Result EC_OD_Init(EC_ObjectDictionary *od,
                        int32_t *target_position, int32_t *actual_position);

/* 按「索引 + 子索引」读写一个 int32_t 数值。
 * 调用前需要成功初始化字典；本课不实现通用的多类型读写接口。 */
EC_OD_Result EC_OD_ReadI32(const EC_ObjectDictionary *od,
                          uint16_t index, uint8_t subindex, int32_t *value);
EC_OD_Result EC_OD_WriteI32(const EC_ObjectDictionary *od,
                           uint16_t index, uint8_t subindex, int32_t value);
const char *EC_OD_ResultName(EC_OD_Result result);

#endif
