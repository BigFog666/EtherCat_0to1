#include "pdo.h"
#include <limits.h>
/* 显式小端处理，不通过未对齐的结构体指针读写网络数据。 */
static uint16_t read16(const uint8_t *p)
{ return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8)); }
static int32_t read32(const uint8_t *p)
{
    uint32_t v = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                 ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    return v <= INT32_MAX ? (int32_t)v : -1 - (int32_t)(UINT32_MAX - v);
}
static void write16(uint8_t *p, uint16_t v)
{ p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void write32(uint8_t *p, int32_t v)
{
    uint32_t u = (uint32_t)v;
    p[0] = (uint8_t)u; p[1] = (uint8_t)(u >> 8);
    p[2] = (uint8_t)(u >> 16); p[3] = (uint8_t)(u >> 24);
}
int project_decode_command(const uint8_t *p, size_t n, JointCommand *c)
{
    if (!p || !c || n != PROJECT_PDO_BYTES) return 0;
    c->controlword = read16(p); c->target_position = read32(p+2);
    c->target_velocity = read32(p+6);
    c->mode = p[10] <= 127 ? (int8_t)p[10] : (int8_t)(-1-(255-p[10]));
    return 1;
}
int project_decode_feedback(const uint8_t *p, size_t n, JointFeedback *f)
{
    if (!p || !f || n != PROJECT_PDO_BYTES) return 0;
    f->statusword = read16(p); f->position = read32(p+2); f->velocity = read32(p+6);
    f->mode = p[10] <= 127 ? (int8_t)p[10] : (int8_t)(-1-(255-p[10]));
    return 1;
}
void project_encode_command(uint8_t *p, const JointCommand *c)
{
    write16(p,c->controlword); write32(p+2,c->target_position);
    write32(p+6,c->target_velocity); p[10] = (uint8_t)c->mode; p[11] = 0;
}
void project_encode_feedback(uint8_t *p, const JointFeedback *f)
{
    write16(p,f->statusword); write32(p+2,f->position);
    write32(p+6,f->velocity); p[10] = (uint8_t)f->mode; p[11] = 0;
}
