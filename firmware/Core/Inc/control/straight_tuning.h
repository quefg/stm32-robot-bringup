#ifndef STRAIGHT_TUNING_H
#define STRAIGHT_TUNING_H

#include "drivers/robot_tuning.h"

/*
 * STRAIGHT CONTROL TUNING
 * =======================
 * These are the main values to re-check when floor friction, tyre grip,
 * payload, battery condition, or steering behaviour changes.
 *
 * Use tests/calibration/test_straight_control.c to validate changes.
 */

/* Shared encoder/servo calibration lives in drivers/robot_tuning.h. */
/* Straight-only feed-forward trim relative to the shared mechanical centre.
 * Positive pulse offset steers RIGHT in either gear (not a yaw correction).
 * Forward and reverse share the same +6 us trim. Keep reverse linked to
 * forward so later adjustments apply to both; validate on the floor.
 * This also applies to forward UNTILUS, which shares this controller. */
#define STRAIGHT_FORWARD_TRIM_US        6
#define STRAIGHT_REVERSE_TRIM_US        STRAIGHT_FORWARD_TRIM_US

#if (SERVO_CENTER_US + STRAIGHT_FORWARD_TRIM_US <= CMD_SERVO_LEFT_US) || \
    (SERVO_CENTER_US + STRAIGHT_FORWARD_TRIM_US >= CMD_SERVO_RIGHT_US) || \
    (SERVO_CENTER_US + STRAIGHT_REVERSE_TRIM_US <= CMD_SERVO_LEFT_US) || \
    (SERVO_CENTER_US + STRAIGHT_REVERSE_TRIM_US >= CMD_SERVO_RIGHT_US)
#error "Straight trim must remain inside servo endpoints"
#endif

/* Profile selection: 10..20 cm = SHORT, 21..200 cm = LONG. */
#define SHORT_PROFILE_MAX_CM            20
#define MIN_COMMAND_DISTANCE_CM         10
#define MAX_COMMAND_DISTANCE_CM         200

/* SHORT profile: most floor-sensitive values for 10..20 cm. */
#define SHORT_CALIBRATION_OFFSET_CM     (-2.5f)
#define SHORT_OVERSHOOT_COMP_CM         0.5f
#define SHORT_RAMP_START_PWM            2950U
#define SHORT_RAMP_PEAK_PWM             2980U
#define SHORT_RAMP_END_PWM              2350U
#define SHORT_KICKSTART_PWM             3200U
#define SHORT_KICKSTART_DURATION_MS     100U

/* LONG profile: intended for assessment distances such as 80/100/120 cm. */
/* Re-tuned after active braking: 40 cm commanded measured about 35 cm with
 * the old 0.37 rev coast compensation, so reduce the early-stop amount. */
#define LONG_REV_ERROR_COMPENSATION     0.14f
#define LONG_BASE_PWM                   3300U
#define LONG_SLOW_PWM                   2400U
#define LONG_SLOWDOWN_START_REV         0.80f

/* Left/right speed synchronization. Error is left-equivalent encoder counts
 * per CONTROL_PERIOD_MS, normalized by calibrated cm and measured dt.
 * SYNC_KP units: PWM per count per nominal interval (no extra x1000). */
#define SYNC_KP                         2.5f
#define SYNC_KI                         10.0f
#define SYNC_ERROR_FILTER_ALPHA         0.30f
#define SYNC_INT_LIMIT_COUNTS_S         25.0f
#define MAX_SYNC_CORRECTION             350

/* Gyro / control loop. */
#define CONTROL_PERIOD_MS               20U
#define GYRO_IIR_ALPHA                  0.10f

/* Gyro yaw -> steering servo PID (gyro rate is the derivative term). */
#define KP_STEER_US_PER_DEG             9.0f
#define KI_STEER_US_PER_DEG_S           0.6f
#define KD_STEER_US_PER_DPS             2.0f
#define YAW_INT_LIMIT_DEG_S             15.0f
#define YAW_DEADBAND_DEG                0.15f
#define GYRO_RATE_DEADBAND_DPS           0.08f
/* Minimum nonzero correction, NOT a zero-output deadband. A 12 us floor
 * made even tiny PID commands jump between opposite steering offsets. */
#define STEER_DEADBAND_US               4.0f
#define MAX_STEER_CORRECTION_US         250.0f
#define MIN_STEER_US                    CMD_SERVO_LEFT_US
#define MAX_STEER_US                    CMD_SERVO_RIGHT_US

/* UNTILUS: use the already-proven long-straight cruise PWM.  The stronger,
 * encoder-confirmed kick gets both wheels out of their static-friction region
 * before the normal wheel synchronizer takes over.
 * Threshold is sensor-to-object clearance, NOT commanded travel distance. */
#define US_MIN_THRESHOLD_CM              5
#define US_MAX_THRESHOLD_CM              200
#define US_CRUISE_PWM                    3500U
#define US_APPROACH_PWM                  2600U
#define US_KICKSTART_PWM                 4000U
#define US_KICKSTART_MIN_MS                120U
#define US_KICKSTART_MAX_MS               400U
#define US_KICKSTART_MIN_COUNTS             6
#define US_START_CONFIRM_SAMPLES            2U
#define US_START_UNKNOWN_SAMPLES            3U
#define US_SLOW_MARGIN_MM                250U
#define US_SPEED_HYSTERESIS_MM            50U
#define US_PWM_SLEW_PER_SECOND           4200U

/* Close the accumulated left/right path error as well as the instantaneous
 * wheel-speed error.  This is only added in ultrasonic approach mode, where
 * the lower final speed makes a small motor imbalance more visible. */
#define US_PATH_SYNC_KP_PWM_PER_CM       80.0f

/* Reject a single short ultrasonic echo as noise.  At the 60 ms sensor
 * cadence, two consecutive readings add little latency at approach speed. */
#define US_STOP_CONFIRM_SAMPLES             2U
/* Older readings become infinity; this is not a motion timeout. */
#define US_SAMPLE_MAX_AGE_MS             200U
#define US_MOTION_TIMEOUT_MS             30000U
#define US_MAX_TRAVEL_CM                 200U

/* Side IR commands: stop on strictly greater distance (FAR/failure = infinity). */
#define IR_COMMAND_SAMPLE_MS             40U
#define IR_COMMAND_CRUISE_PWM            3300U
#define IR_COMMAND_KICK_PWM              4000U
#define IR_COMMAND_KICK_MS               120U
#define IR_COMMAND_TIMEOUT_MS           30000U
#define IR_COMMAND_MAX_TRAVEL_CM         200U

#endif /* STRAIGHT_TUNING_H */
