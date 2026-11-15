#ifndef STRAIGHT_CONTROL_H
#define STRAIGHT_CONTROL_H

#include <stdint.h>

typedef enum
{
    STRAIGHT_IDLE = 0,
    STRAIGHT_RUNNING,
    STRAIGHT_DONE,
    STRAIGHT_ABORTED,
    STRAIGHT_TIMEOUT,
    STRAIGHT_SENSOR_ERROR,
    STRAIGHT_DISTANCE_LIMIT,
    STRAIGHT_STALL
} straight_state_t;

/* Production non-blocking controller used by the command interface. */
void Straight_Init(void);
int Straight_StartCm(int32_t target_cm);
int Straight_StartUntilUltrasonicCm(int32_t threshold_cm);
typedef enum { STRAIGHT_IR_GT = 0, STRAIGHT_IR_LT } straight_ir_comparison_t;
int Straight_StartUntilIRCm(int32_t threshold_cm, uint8_t side,
                            straight_ir_comparison_t comparison);
straight_ir_comparison_t Straight_IRComparison(void);
/* Forward until selected IR > threshold. side: 0=LEFT, 1=RIGHT.
 * FAR or ADC failure completes normally, per command policy. */
int Straight_StartUntilIRGreaterCm(int32_t threshold_cm, uint8_t side);
/* Returns 0=not IR, 1=LEFTIR, 2=RIGHTIR. UINT32_MAX distance means FAR. */
int Straight_IRStatus(int32_t *threshold_cm, uint32_t *cm_x10);
/* Returns 1 for UNTILUS mode. Unavailable clearance = UINT32_MAX (infinity).
 * Nonzero status means invalid/stale; missing echoes do not stop motion. */
int Straight_UltrasonicStatus(int32_t *threshold_cm, uint32_t *distance_mm,
                              int *status, uint32_t *age_ms);
void Straight_Step(void);
void Straight_Stop(void);
straight_state_t Straight_State(void);
void Straight_Status(uint32_t *elapsed_ms,
                     int32_t *target_cm,
                     float *l_cm,
                     float *r_cm,
                     float *yaw_deg,
                     float *diff_cm);


#endif /* STRAIGHT_CONTROL_H */
