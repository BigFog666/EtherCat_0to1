#include "mode_pdo.h"
#include <string.h>

/* 复用已验证的 32 位小端编解码，暂不要求独立重写负数处理。 */
static void PackI32(uint8_t *bytes, int32_t value)
{
    uint8_t temporary[EC_RXPDO_SIZE];
    EC_RxPDO item = {.controlword = 0, .target_position = value};
    EC_PackRxPDO(temporary, &item);
    memcpy(bytes, temporary + 2, 4);
}
static int32_t UnpackI32(const uint8_t *bytes)
{
    uint8_t temporary[EC_RXPDO_SIZE] = {0};
    EC_RxPDO item;
    memcpy(temporary + 2, bytes, 4);
    EC_UnpackRxPDO(&item, temporary);
    return item.target_position;
}
static void PackTail(uint8_t *bytes, int8_t mode, int32_t velocity, int16_t torque)
{
    uint16_t raw_torque = (uint16_t)torque;
    bytes[6] = (uint8_t)mode;
    PackI32(bytes + 7, velocity);
    bytes[11] = (uint8_t)raw_torque;
    bytes[12] = (uint8_t)(raw_torque >> 8);
}
static int8_t ReadMode(uint8_t byte)
{
    /* 先在更宽类型里还原负值，再转到范围内的 int8_t。 */
    return (int8_t)(byte <= 127u ? (int16_t)byte : (int16_t)byte - 256);
}
static int16_t ReadTorque(const uint8_t *bytes)
{
    uint16_t raw = (uint16_t)((uint16_t)bytes[11] | ((uint16_t)bytes[12] << 8));
    return (int16_t)(raw <= 32767u ? (int32_t)raw : (int32_t)raw - 65536);
}
bool ModePDO_PackRx(uint8_t *bytes, size_t size, const ModeCommand *command)
{
    if (bytes == NULL || command == NULL || size != MODE_PDO_SIZE) return false;
    EC_PackRxPDO(bytes, &command->position);
    PackTail(bytes, command->mode, command->target_velocity, command->target_torque);
    return true;
}
bool ModePDO_UnpackRx(ModeCommand *command, const uint8_t *bytes, size_t size)
{
    if (bytes == NULL || command == NULL || size != MODE_PDO_SIZE) return false;
    ModeCommand decoded;
    EC_UnpackRxPDO(&decoded.position, bytes);
    decoded.mode = ReadMode(bytes[6]);
    decoded.target_velocity = UnpackI32(bytes + 7);
    decoded.target_torque = ReadTorque(bytes);
    *command = decoded;
    return true;
}
bool ModePDO_PackTx(uint8_t *bytes, size_t size, const ModeFeedback *feedback)
{
    if (bytes == NULL || feedback == NULL || size != MODE_PDO_SIZE) return false;
    EC_PackTxPDO(bytes, &feedback->position);
    PackTail(bytes, feedback->mode_display, feedback->actual_velocity, feedback->actual_torque);
    return true;
}
bool ModePDO_UnpackTx(ModeFeedback *feedback, const uint8_t *bytes, size_t size)
{
    if (bytes == NULL || feedback == NULL || size != MODE_PDO_SIZE) return false;
    ModeFeedback decoded;
    EC_UnpackTxPDO(&decoded.position, bytes);
    decoded.mode_display = ReadMode(bytes[6]);
    decoded.actual_velocity = UnpackI32(bytes + 7);
    decoded.actual_torque = ReadTorque(bytes);
    *feedback = decoded;
    return true;
}
