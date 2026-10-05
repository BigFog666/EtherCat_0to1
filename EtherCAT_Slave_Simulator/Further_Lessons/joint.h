#ifndef JOINT_H
#define JOINT_H
#include "mode_pdo.h"

/* 三个标准模式编号；本课只实现简化运动模型，不实现真实伺服闭环。 */
#define JOINT_CSP 8
#define JOINT_CSV 9
#define JOINT_CST 10
typedef struct {
    int64_t position_milli; /* 千分之一计数，保留速度积分的小数余量。 */
    int32_t velocity;       /* 模拟计数/秒。 */
    int16_t torque;         /* 模拟扭矩整数，无物理单位换算。 */
    int8_t mode;
    int32_t max_velocity;   /* 本地模拟速度限幅，必须为正。 */
} JointModel;

bool Joint_ModeSupported(int8_t mode);
bool Joint_Init(JointModel *joint, int32_t position, int32_t max_velocity);
/* dt_ms 是逻辑时间步长，范围 1～1000；不能证明实际执行周期。
 * 未使能时仍可接受有效模式，但速度和扭矩为零，位置保持。
 * 非法输入/位置越界返回 false，原模型不变。 */
bool Joint_Step(JointModel *joint, const ModeCommand *command, bool enabled, uint32_t dt_ms);
void Joint_Stop(JointModel *joint); /* 本模型立即清速度和扭矩，不模拟机械惯性。 */
bool Joint_ReadFeedback(const JointModel *joint, uint16_t statusword, ModeFeedback *feedback);
#endif
