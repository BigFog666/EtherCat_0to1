#include "joint.h"
#include <string.h>

void joint_init(Joint *j, uint32_t now)
{
    memset(j, 0, sizeof(*j));
    j->state = JD_DISABLED;
    j->last_update_us = now;
    j->feedback.statusword = 0x0040;
}

void joint_receive(Joint *j, const JointCommand *c, uint32_t now)
{
    /* 每次有效 PDO 接收事件都刷新时间；相同目标值也是新帧。 */
    j->command = *c;
    j->last_receive_us = now;
    j->have_command = 1;
    j->receive_count++;
}

static int command_cause(const Joint *j)
{
    const JointCommand *c = &j->command;
    if ((c->controlword & 0x8000u) != 0) return 0xFF01; /* 演示故障注入 */
    if (c->mode != 8 && c->mode != 9) return 0xFF02;
    if (c->mode == 8 && (c->target_position > JOINT_LIMIT ||
                        c->target_position < -JOINT_LIMIT)) return 0x8611;
    if (c->mode == 9 && (c->target_velocity > JOINT_MAX_VELOCITY ||
                        c->target_velocity < -JOINT_MAX_VELOCITY)) return 0xFF03;
    return 0;
}

void joint_update(Joint *j, uint32_t now, int op)
{
    uint32_t dt = now - j->last_update_us;
    uint16_t cw = j->command.controlword;
    int reset = (cw & 0x0080u) != 0;
    int rising = reset && !j->previous_reset;
    int stale = j->have_command &&
                (uint32_t)(now - j->last_receive_us) >= JOINT_TIMEOUT_US;
    int cause = j->have_command ? command_cause(j) : 0;
    int32_t velocity = 0;
    int64_t next, target;
    j->last_update_us = now;
    j->previous_reset = (uint8_t)reset;
    if (stale) cause = 0xFF04;
    if (stale && !j->timeout_active) j->timeout_count++;
    j->timeout_active = (uint8_t)stale;

    /* 通信许可与驱动状态分开；故障锁存不被恢复 OP 自动清除。 */
    if (cause) {
        j->error_code = (uint16_t)cause;
        if (j->state != JD_FAULT && j->state != JD_FAULT_REACTION)
            j->state = JD_FAULT_REACTION;
        else if (j->state == JD_FAULT_REACTION) j->state = JD_FAULT;
    } else if (j->state == JD_FAULT_REACTION) {
        j->state = JD_FAULT;
    } else if (j->state == JD_FAULT) {
        if (rising && op && j->have_command) {
            j->state = JD_DISABLED;
            j->error_code = 0;
        }
    } else if (!op || !j->have_command || (cw & 0x0082u) != 0x0002u) {
        j->state = JD_DISABLED;
    } else if ((cw & 0x0004u) == 0) {
        /* 模拟器立即停止；经过 QUICK_STOP 报告后回禁止接通。 */
        if (j->state == JD_ENABLED) j->state = JD_QUICK_STOP;
        else if (j->state == JD_QUICK_STOP) j->state = JD_DISABLED;
    } else if ((cw & 0x0087u) == 0x0006u) {
        j->state = JD_READY;
    } else if ((cw & 0x008Fu) == 0x0007u &&
               (j->state == JD_READY || j->state == JD_ENABLED || j->state == JD_SWITCHED)) {
        j->state = JD_SWITCHED;
    } else if ((cw & 0x008Fu) == 0x000Fu) {
        if (j->state == JD_READY) j->state = JD_SWITCHED;
        else if (j->state == JD_SWITCHED) j->state = JD_ENABLED;
    }

    if (j->state == JD_ENABLED && op && !stale && !cause) {
        /* 不把一次异常长停顿补算成运动，避免模型跳变。 */
        if (dt > JOINT_TIMEOUT_US) dt = 0;
        if (j->command.mode == 8) {
            target = (int64_t)j->command.target_position * 1000000;
            if (target > j->position_micro) velocity = JOINT_MAX_VELOCITY;
            else if (target < j->position_micro) velocity = -JOINT_MAX_VELOCITY;
            next = j->position_micro + (int64_t)velocity * dt;
            if ((velocity > 0 && next >= target) || (velocity < 0 && next <= target)) {
                next = target;
                velocity = 0;
            }
        } else {
            velocity = j->command.target_velocity;
            next = j->position_micro + (int64_t)velocity * dt;
            if (next > (int64_t)JOINT_LIMIT * 1000000 ||
                next < -(int64_t)JOINT_LIMIT * 1000000) {
                next = j->position_micro;
                velocity = 0;
                j->error_code = 0x8611;
                j->state = JD_FAULT_REACTION;
            }
        }
        j->position_micro = next;
    }
    j->feedback.position = (int32_t)(j->position_micro / 1000000);
    j->feedback.velocity = velocity;
    j->feedback.mode = (j->command.mode == 8 || j->command.mode == 9) ? j->command.mode : 0;
    switch (j->state) {
    case JD_DISABLED: j->feedback.statusword = 0x0040; break;
    case JD_READY: j->feedback.statusword = 0x0021; break;
    case JD_SWITCHED: j->feedback.statusword = 0x0023; break;
    case JD_ENABLED: j->feedback.statusword = 0x0027; break;
    case JD_QUICK_STOP: j->feedback.statusword = 0x0007; break;
    case JD_FAULT_REACTION: j->feedback.statusword = 0x000F; break;
    default: j->feedback.statusword = 0x0008; break;
    }
    if (j->state == JD_ENABLED && j->command.mode == 8 &&
        j->feedback.position == j->command.target_position)
        j->feedback.statusword |= 0x0400; /* 教学目标到达标志 */
}
