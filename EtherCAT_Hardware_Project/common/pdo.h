#ifndef PROJECT_PDO_H
#define PROJECT_PDO_H
#include <stddef.h>
#include "joint.h"
#define PROJECT_PDO_BYTES 12u
int project_decode_command(const uint8_t *data, size_t size, JointCommand *command);
int project_decode_feedback(const uint8_t *data, size_t size, JointFeedback *feedback);
void project_encode_command(uint8_t *data, const JointCommand *command);
void project_encode_feedback(uint8_t *data, const JointFeedback *feedback);
#endif
