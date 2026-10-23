#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H
#include <stdint.h>
void motor_set_speed(uint8_t channel, uint16_t rpm);
void motor_emergency_stop(void);
#endif
