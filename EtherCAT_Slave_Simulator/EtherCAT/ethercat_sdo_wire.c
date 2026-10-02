#include "ethercat_sdo_wire.h"
#include <string.h>

/* 拆字节、拼字节的方式与第六课相同，先理解偏移，负数细节可稍后看。 */
static void WriteU32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static uint32_t ReadU32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static uint16_t ReadIndex(const EC_SDO_Frame *frame)
{
    return (uint16_t)((uint16_t)frame->bytes[1] |
                      ((uint16_t)frame->bytes[2] << 8));
}

static int32_t ReadI32(const uint8_t *bytes)
{
    uint32_t raw = ReadU32(bytes);
    if (raw <= (uint32_t)INT32_MAX) {
        return (int32_t)raw;
    }
    return INT32_MIN + (int32_t)(raw - UINT32_C(0x80000000));
}

/* 所有请求与响应都采用相同的前四字节布局。
 * memset 先清零 8 字节，避免旧数值留在保留字段中。 */
static void InitFrame(EC_SDO_Frame *frame, uint8_t command,
                      uint16_t index, uint8_t subindex)
{
    memset(frame->bytes, 0, sizeof frame->bytes);
    frame->bytes[0] = command;
    frame->bytes[1] = (uint8_t)index;        /* 索引低字节，例如 0x7A。 */
    frame->bytes[2] = (uint8_t)(index >> 8); /* 索引高字节，例如 0x60。 */
    frame->bytes[3] = subindex;
}

bool EC_SDO_BuildUpload(EC_SDO_Frame *frame, uint16_t index, uint8_t subindex)
{
    if (frame == 0) {
        return false;
    }
    /* 读请求不带数值，最后四字节保持为零。 */
    InitFrame(frame, EC_SDO_CMD_UPLOAD_REQUEST, index, subindex);
    return true;
}

bool EC_SDO_BuildDownloadI32(EC_SDO_Frame *frame, uint16_t index,
                            uint8_t subindex, int32_t value)
{
    if (frame == 0) {
        return false;
    }
    InitFrame(frame, EC_SDO_CMD_DOWNLOAD_I32, index, subindex);
    WriteU32(frame->bytes + 4, (uint32_t)value); /* 偏移 4 开始放写入值。 */
    return true;
}

/* 字典结果变成标准 Abort Code。先学会只读错误，其他项用于排查问题。 */
static uint32_t ToAbortCode(EC_OD_Result result,
                            const EC_ObjectDictionary *od, uint16_t index)
{
    if (result == EC_OD_READ_ONLY) {
        return EC_SDO_ABORT_READ_ONLY;
    }
    if (result == EC_OD_NOT_FOUND) {
        /* 原来的 OD_NOT_FOUND 没细分原因，这里再检查索引是否存在。 */
        for (unsigned int i = 0; i < EC_OD_ENTRY_COUNT; ++i) {
            if (od->entries[i].index == index) {
                return EC_SDO_ABORT_NO_SUBINDEX;
            }
        }
        return EC_SDO_ABORT_NO_OBJECT;
    }
    return EC_SDO_ABORT_GENERAL;
}

static void BuildAbort(EC_SDO_Frame *frame, uint16_t index,
                       uint8_t subindex, uint32_t abort_code)
{
    InitFrame(frame, EC_SDO_CMD_ABORT, index, subindex);
    WriteU32(frame->bytes + 4, abort_code);
}

bool EC_SDO_ProcessFrame(EC_Mailbox *mailbox, const EC_ObjectDictionary *od,
                         const EC_SDO_Frame *request, EC_SDO_Frame *response)
{
    if (mailbox == 0 || od == 0 || request == 0 || response == 0) {
        return false;
    }
    if (mailbox->state != EC_MAILBOX_EMPTY ||
        request->bytes[0] == EC_SDO_CMD_ABORT) {
        return false;
    }

    /* 第一步：字节解码，还原成上一小节已经看懂的请求结构体。 */
    EC_SDO_Request decoded = {
        .index = ReadIndex(request),
        .subindex = request->bytes[3],
        .value = 0
    };
    if (request->bytes[0] == EC_SDO_CMD_UPLOAD_REQUEST) {
        decoded.service = EC_SDO_UPLOAD;
    } else if (request->bytes[0] == EC_SDO_CMD_DOWNLOAD_I32) {
        decoded.service = EC_SDO_DOWNLOAD;
        decoded.value = ReadI32(request->bytes + 4);
    } else {
        /* 1～3 字节快速写请求不符合当前两个 INTEGER32 对象的宽度。
         * 其他命令也不在本课支持范围内，不继续访问字典。 */
        uint32_t code = EC_SDO_ABORT_COMMAND;
        if (request->bytes[0] == 0x27 || request->bytes[0] == 0x2B ||
            request->bytes[0] == 0x2F) {
            code = EC_SDO_ABORT_TYPE;
        }
        BuildAbort(response, decoded.index, decoded.subindex, code);
        return true;
    }

    /* 第二步：复用原有邮箱事务，仍由字典负责读取/修改原变量。 */
    EC_SDO_Response result = {0};
    if (!EC_SDO_Transfer(mailbox, od, &decoded, &result)) {
        return false;
    }

    /* 第三步：将成功或失败结果编码成 8 字节响应。 */
    if (result.result != EC_OD_OK) {
        BuildAbort(response, decoded.index, decoded.subindex,
                   ToAbortCode(result.result, od, decoded.index));
    } else if (decoded.service == EC_SDO_UPLOAD) {
        InitFrame(response, EC_SDO_CMD_UPLOAD_I32, decoded.index, decoded.subindex);
        WriteU32(response->bytes + 4, (uint32_t)result.value);
    } else {
        /* 写入成功只返回确认：0x60 + 索引 + 子索引 + 四个零。 */
        InitFrame(response, EC_SDO_CMD_DOWNLOAD_ACK, decoded.index, decoded.subindex);
    }
    return true;
}

bool EC_SDO_GetUploadI32(const EC_SDO_Frame *frame, uint16_t expected_index,
                        uint8_t expected_subindex, int32_t *value)
{
    if (frame == 0 || value == 0) {
        return false;
    }
    if (frame->bytes[0] != EC_SDO_CMD_UPLOAD_I32 ||
        ReadIndex(frame) != expected_index || frame->bytes[3] != expected_subindex) {
        return false;
    }
    *value = ReadI32(frame->bytes + 4);
    return true;
}

bool EC_SDO_GetAbortCode(const EC_SDO_Frame *frame, uint32_t *abort_code)
{
    if (frame == 0 || abort_code == 0 || frame->bytes[0] != EC_SDO_CMD_ABORT) {
        return false;
    }
    *abort_code = ReadU32(frame->bytes + 4);
    return true;
}
