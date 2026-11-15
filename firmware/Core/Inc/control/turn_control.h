#ifndef TURN_CONTROL_H
#define TURN_CONTROL_H

#include <stdint.h>

typedef enum
{
    TURN_IDLE = 0,
    TURN_RUNNING,
    TURN_DONE,
    TURN_ABORTED,
    TURN_TIMEOUT,
    TURN_WRONG_SIGN,
    TURN_NO_TURN,
    TURN_BAD_CONFIG,
    TURN_NO_IMU
}
turn_state_t;

void Turn_RunMinRadiusTestBlocking(void);

void Turn_Init(void);
int Turn_StartRadiusAngle(int32_t radius_cm, int32_t angle_deg, int8_t turn_dir);
void Turn_Step(void);
void Turn_Stop(void);
/* Valid after a normal completed turn; signed yaw and positive turn rate. */
void Turn_StopMetrics(float *trigger_yaw_deg, float *trigger_rate_dps);
turn_state_t Turn_State(void);
void Turn_Status(uint32_t *elapsed_ms,
                 int32_t *radius_cm,
                 int32_t *target_deg,
                 float *yaw_deg,
                 int8_t *turn_dir);

#endif /* TURN_CONTROL_H */
