#ifndef ETHERCAT_SDO_WIRE_H
#define ETHERCAT_SDO_WIRE_H

#include "ethercat_sdo.h"

/* 第八课第二步：只实现 4 字节对象的 expedited（快速）SDO 内容。
 * 这 8 字节还需要外层 CoE 和 Mailbox 头，才能成为真实邮箱消息。
 * 不支持分段传输、Complete Access、超时重试或实际网卡通信。 */
#define EC_SDO_FRAME_SIZE 8u
#define EC_SDO_CMD_UPLOAD_REQUEST 0x40u
#define EC_SDO_CMD_DOWNLOAD_I32 0x23u
#define EC_SDO_CMD_UPLOAD_I32 0x43u
#define EC_SDO_CMD_DOWNLOAD_ACK 0x60u
#define EC_SDO_CMD_ABORT 0x80u

/* 标准 SDO Abort Code；它们不同于上一小节的本地 EC_OD_Result。 */
#define EC_SDO_ABORT_COMMAND UINT32_C(0x05040001)
#define EC_SDO_ABORT_READ_ONLY UINT32_C(0x06010002)
#define EC_SDO_ABORT_NO_OBJECT UINT32_C(0x06020000)
#define EC_SDO_ABORT_TYPE UINT32_C(0x06070010)
#define EC_SDO_ABORT_NO_SUBINDEX UINT32_C(0x06090011)
#define EC_SDO_ABORT_GENERAL UINT32_C(0x08000000)

typedef struct {
    /* 偏移 0：命令；1～2：索引；3：子索引；4～7：数值或错误码。 */
    uint8_t bytes[EC_SDO_FRAME_SIZE];
} EC_SDO_Frame;

/* 主站侧：按协议布局构造请求，其他保留字节自动置零。 */
bool EC_SDO_BuildUpload(EC_SDO_Frame *frame, uint16_t index, uint8_t subindex);
bool EC_SDO_BuildDownloadI32(EC_SDO_Frame *frame, uint16_t index,
                            uint8_t subindex, int32_t value);

/* 从站侧：解析字节 -> 复用原来的邮箱/字典 -> 编码响应。
 * true 表示生成响应，响应可能是成功，也可能是 Abort。
 * false 表示参数无效、模拟邮箱忙，或收到无需回复的客户端 Abort。
 * 调用者提供有效的完整 EC_SDO_Frame 对象；这里不解析外层网络包。
 * 客户端 Abort 不回复；本模型没有分段会话可取消。 */
bool EC_SDO_ProcessFrame(EC_Mailbox *mailbox, const EC_ObjectDictionary *od,
                         const EC_SDO_Frame *request, EC_SDO_Frame *response);

/* 主站侧：确认读取响应的命令及对象身份，再取出位置。
 * 失败时不改变调用者的 value；只支持显式标明 4 字节的响应 0x43。 */
bool EC_SDO_GetUploadI32(const EC_SDO_Frame *frame, uint16_t expected_index,
                        uint8_t expected_subindex, int32_t *value);
bool EC_SDO_GetAbortCode(const EC_SDO_Frame *frame, uint32_t *abort_code);

#endif
