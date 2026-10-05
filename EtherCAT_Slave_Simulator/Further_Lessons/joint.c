#include "joint.h"
#include <limits.h>

bool Joint_ModeSupported(int8_t mode)
{
    return mode == JOINT_CSP || mode == JOINT_CSV || mode == JOINT_CST;
}
bool Joint_Init(JointModel *joint, int32_t position, int32_t max_velocity)
{
    if (joint == NULL || max_velocity <= 0) return false;
    *joint = (JointModel){.position_milli = (int64_t)position * 1000,
                          .mode = JOINT_CSP, .max_velocity = max_velocity};
    return true;
}
static int32_t LimitVelocity(int64_t value, int32_t limit)
{
    if (value > limit) return limit;
    if (value < -(int64_t)limit) return -limit;
    return (int32_t)value;
}
void Joint_Stop(JointModel *joint)
{
    if (joint != NULL) { joint->velocity = 0; joint->torque = 0; }
}
bool Joint_Step(JointModel *joint, const ModeCommand *command, bool enabled, uint32_t dt_ms)
{
    if (joint == NULL || command == NULL || !Joint_ModeSupported(command->mode) ||
        joint->max_velocity <= 0 || dt_ms == 0 || dt_ms > 1000u ||
        joint->position_milli < (int64_t)INT32_MIN * 1000 ||
        joint->position_milli > (int64_t)INT32_MAX * 1000) return false;
    JointModel next = *joint; /* 先计算候选结果，失败时不改原模型。 */
    next.mode = command->mode;
    next.torque = 0;
    if (!enabled) {
        Joint_Stop(&next);
    } else if (next.mode == JOINT_CSP) {
        /* 位置模式：按速度上限朝目标走，最后一步不能越过目标。 */
        int64_t remaining = (int64_t)command->position.target_position * 1000 - next.position_milli;
        int64_t limit = (int64_t)next.max_velocity * dt_ms;
        int64_t step = remaining > limit ? limit : (remaining < -limit ? -limit : remaining);
        next.position_milli += step;
        next.velocity = (int32_t)(step / dt_ms);
    } else if (next.mode == JOINT_CSV) {
        /* 速度模式：模拟驱动立即跟随限幅后的目标速度，再积分位置。
         * counts/s × ms = 千分之一计数，避免 0.5 计数被每轮丢掉。 */
        next.velocity = LimitVelocity(command->target_velocity, next.max_velocity);
        next.position_milli += (int64_t)next.velocity * dt_ms;
    } else {
        /* 扭矩模式：教学假设加速度 = 输入扭矩 × 1000 计数/秒²。
         * 不是电流环、FOC 或真实力矩控制；先更新速度，再积分位置。 */
        next.torque = command->target_torque;
        next.velocity = LimitVelocity((int64_t)next.velocity +
                                       (int64_t)next.torque * dt_ms, next.max_velocity);
        next.position_milli += (int64_t)next.velocity * dt_ms;
    }
    if (next.position_milli < (int64_t)INT32_MIN * 1000 ||
        next.position_milli > (int64_t)INT32_MAX * 1000) return false;
    *joint = next;
    return true;
}
bool Joint_ReadFeedback(const JointModel *joint, uint16_t statusword, ModeFeedback *feedback)
{
    if (joint == NULL || feedback == NULL ||
        joint->position_milli < (int64_t)INT32_MIN * 1000 ||
        joint->position_milli > (int64_t)INT32_MAX * 1000) return false;
    *feedback = (ModeFeedback){.position = {.statusword = statusword,
                  .actual_position = (int32_t)(joint->position_milli / 1000)},
                  .mode_display = joint->mode, .actual_velocity = joint->velocity,
                  .actual_torque = joint->torque};
    return true;
}
