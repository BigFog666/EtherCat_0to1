#include "ethercat_fmmu.h"

bool EC_FMMU_Translate(const EC_FMMU *fmmu, EC_FMMU_Access access,
                       uint32_t logical_address, size_t count,
                       uint16_t *physical_address)
{
    if (fmmu == NULL || physical_address == NULL || !fmmu->enabled ||
        fmmu->length == 0u || count == 0u) {
        return false;
    }
    if ((access != EC_FMMU_READ && access != EC_FMMU_WRITE) ||
        (access == EC_FMMU_READ && !fmmu->read_enabled) ||
        (access == EC_FMMU_WRITE && !fmmu->write_enabled)) {
        return false;
    }

    /* 先保证整条映射不会跨越地址类型的上限。
     * 用“剩余空间”比较，避免直接求末地址时整数溢出。 */
    uint32_t last_offset = (uint32_t)fmmu->length - 1u;
    if (last_offset > UINT32_MAX - fmmu->logical_start ||
        last_offset > (uint32_t)UINT16_MAX - fmmu->physical_start ||
        logical_address < fmmu->logical_start) {
        return false;
    }

    /* 确认请求的每个字节都位于映射范围，不能只看第一个字节。 */
    uint32_t offset = logical_address - fmmu->logical_start;
    if (offset >= fmmu->length || count > (size_t)(fmmu->length - offset)) {
        return false;
    }
    *physical_address = (uint16_t)(fmmu->physical_start + offset);
    return true;
}
