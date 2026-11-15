#ifndef TURN_TUNING_H
#define TURN_TUNING_H

#include "drivers/robot_tuning.h"

/*
 * TURN CONTROL TUNING
 * ===================
 * Re-check these values when floor friction / tyre grip changes.
 * Use tests/calibration/test_turn_angle.c to validate actual turn angles.
 */

/* Initial heavier-chassis trial, not yet floor-validated. Raise drive effort
 * at every stage while keeping KICK > CRUISE > MEDIUM > SLOW. The longer
 * kick applies to all angles; short turns limit it by time and progress.
 * Stop checks always take precedence.
 * Recheck left/right 90/180 deg and small turns with the actual payload. */
#define TURN_MOTOR_PWM                 3600U
#define TURN_MEDIUM_PWM                3300U
#define TURN_SLOW_PWM                  3100U
#define TURN_KICKSTART_PWM             4000U
#define TURN_KICKSTART_DURATION_MS      120U
#define TURN_SHORT_KICK_DURATION_MS      60U
#define TURN_SHORT_KICK_PROGRESS_DEG      2.0f
/* Normal command-turn brake hold, sampled non-blockingly. */
#define TURN_BRAKE_HOLD_MS              150U

/* Reduce speed as the remaining yaw becomes small.  The final stop predicts
 * coast from the measured gyro rate instead of assuming coast is a fixed
 * percentage of every requested angle. */
#define TURN_SLOWDOWN_REMAINING_DEG     30.0f
#define TURN_FINAL_REMAINING_DEG         9.0f
#define TURN_SMALL_ANGLE_DEG            15.0f
#define TURN_LEFT_COAST_TIME_S           0.22f
#define TURN_RIGHT_COAST_TIME_S          0.17f
#define TURN_STOP_TOLERANCE_DEG          0.7f

/* RIGHT commands with |angle| below 90 degrees, forward and reverse. Initial trial values:
 * bounded high-power kick, earlier final speed, independent stopping lead.
 * Keep lead at the existing value until measured floor errors are available.
 * Increase lead for overshoot; decrease for undershoot. Units are seconds,
 * not the duration of the motor brake pulse. */
#define TURN_RIGHT_SHORT_LIMIT_DEG      90
#define TURN_RIGHT_SHORT_CRUISE_PWM     TURN_MOTOR_PWM
#define TURN_RIGHT_SHORT_SLOW_PWM       TURN_SLOW_PWM
#define TURN_RIGHT_SHORT_FINAL_DEG      20.0f
#define TURN_RIGHT_SHORT_LEAD_TIME_S     0.17f

/* Command-mode yaw target magnitude = |requested angle| * side scale.
 * Supports undershoot (>1) and overshoot (<1) compensation, forward/reverse.
 * Valid range: (0, TURN_TARGET_SCALE_MAX]. Steering scale is independent. */
#define TURN_TARGET_SCALE_MAX           2.000f
#define TURN_LEFT_TARGET_SCALE           1.040f
#define TURN_RIGHT_TARGET_SCALE          1.000f

/* Shared center/endpoints/geometry live in drivers/robot_tuning.h.
 * Pulse-span trim AFTER radius mapping. Smaller scale = gentler steering /
 * larger radius. These are initial field adjustments, not measured radii.
 * Selected by steering side for forward and reverse. Keep within (0, 1]. */
#define TURN_LEFT_STEER_SCALE           0.98f
#define TURN_RIGHT_STEER_SCALE          0.95f

/* Valid command angle range. */
#define CMD_MIN_ANGLE_DEG              5
#define CMD_MAX_ANGLE_DEG              360

/*
 * Legacy fixed motor cut-off ratios used by the minimum-radius calibration
 * routine.  Production command turns use measured yaw rate and the coast-time
 * constants above, so their stopping lead does not grow incorrectly with the
 * requested angle.
 * Select by steering side (LEFT/RIGHT), for both forward and reverse turns.
 * If turns overshoot on a slippery floor, DECREASE this ratio.
 * If turns stop short, INCREASE it slightly.
 */
#define TURN_LEFT_STOP_RATIO           0.80f
#define TURN_RIGHT_STOP_RATIO          0.92f

#endif /* TURN_TUNING_H */
