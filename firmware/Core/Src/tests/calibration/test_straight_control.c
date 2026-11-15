#include "main.h"
#include "tests/calibration/test_straight_control.h"
#include "control/straight_control.h"
#include "control/straight_tuning.h"
#include "drivers/robot_hw.h"

#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart3;

/* =========================================================
 * CHANGE THIS ONE VALUE FOR EACH STRAIGHT TEST
 * Recommended floor checks: 10, 20, 30, 40, 50, 80, 100, 120 cm.
 * ========================================================= */
#define TEST_STRAIGHT_DISTANCE_CM 100

static void UART_Print(const char *text)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)text,
                      (uint16_t)strlen(text), HAL_MAX_DELAY);
}

static void FormatSignedX100(float value, char *out, size_t n)
{
    int32_t x100 = (int32_t)(value * 100.0f + ((value >= 0.0f) ? 0.5f : -0.5f));
    int32_t a = (x100 < 0) ? -x100 : x100;
    snprintf(out, n, "%c%ld.%02ld",
             (x100 < 0) ? '-' : '+',
             (long)(a / 100), (long)(a % 100));
}

void Test_StraightControl(void)
{
    char msg[220];
    char l_s[24], r_s[24], yaw_s[24], diff_s[24];
    uint32_t elapsed_ms = 0U;
    int32_t target_cm = 0;
    float l_cm = 0.0f, r_cm = 0.0f, yaw_deg = 0.0f, diff_cm = 0.0f;

    RobotHW_StartMotionPeripherals();
    Straight_Init();

    UART_Print("\r\n========================================\r\n");
    UART_Print(" STRAIGHT CONTROL FLOOR CALIBRATION\r\n");
    UART_Print("========================================\r\n");
    snprintf(msg, sizeof(msg),
             "Requested distance: %d cm\r\n"
             "Profile rule: 10-20 cm SHORT, >20 cm LONG\r\n"
             "Keep robot STILL during gyro calibration.\r\n"
             "Starting in 3 seconds...\r\n",
             TEST_STRAIGHT_DISTANCE_CM);
    UART_Print(msg);
    HAL_Delay(3000U);

    int rc = Straight_StartCm(TEST_STRAIGHT_DISTANCE_CM);
    if (rc != 0)
    {
        snprintf(msg, sizeof(msg), "ERROR: Straight_StartCm rc=%d\r\n", rc);
        UART_Print(msg);
        RobotHW_MotorStop();
        RobotHW_ServoCenter();
        while (1) HAL_Delay(1000U);
    }

    while (Straight_State() == STRAIGHT_RUNNING)
    {
        Straight_Step();
        HAL_Delay(1U);
    }

    Straight_Status(&elapsed_ms, &target_cm, &l_cm, &r_cm, &yaw_deg, &diff_cm);
    FormatSignedX100(l_cm, l_s, sizeof(l_s));
    FormatSignedX100(r_cm, r_s, sizeof(r_s));
    FormatSignedX100(yaw_deg, yaw_s, sizeof(yaw_s));
    FormatSignedX100(diff_cm, diff_s, sizeof(diff_s));

    UART_Print("\r\n--- RESULT ---\r\n");
    snprintf(msg, sizeof(msg),
             "State=%d  time=%lums  target=%ldcm\r\n"
             "Encoder travel L=%s cm  R=%s cm\r\n"
             "L-R difference=%s cm  final yaw=%s deg\r\n",
             (int)Straight_State(), (unsigned long)elapsed_ms, (long)target_cm,
             l_s, r_s, diff_s, yaw_s);
    UART_Print(msg);
    UART_Print("Measure the REAL floor distance and sideways deviation with a ruler/tape.\r\n");
    UART_Print("Tune values in Core/Inc/control/straight_tuning.h, then repeat.\r\n");

    RobotHW_MotorStop();
    RobotHW_ServoCenter();
    while (1) HAL_Delay(1000U);
}
