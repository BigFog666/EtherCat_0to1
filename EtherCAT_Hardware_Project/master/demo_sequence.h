#ifndef PROJECT_DEMO_SEQUENCE_H
#define PROJECT_DEMO_SEQUENCE_H
#include "joint.h"
typedef struct { unsigned phase, ticks, reached; } DemoSequence;
/* 返回 1 继续，0 完成，-1 非预期状态或阶段超时。 */
int demo_next(DemoSequence *demo, const JointFeedback *feedback, JointCommand *command);
/* 可视演示保持各状态数秒，并缓慢递增 CSP 目标，让板载灯可观察。 */
int demo_next_visual(DemoSequence *demo, const JointFeedback *feedback, JointCommand *command);
#endif
