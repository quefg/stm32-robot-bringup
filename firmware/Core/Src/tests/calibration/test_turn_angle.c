#include "main.h"
#include "tests/calibration/test_turn_angle.h"
#include "control/turn_control.h"
#include "control/turn_tuning.h"
#include "drivers/oled.h"
#include "drivers/robot_hw.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart3;

#define TURN_TEST_LEFT              (-1)
#define TURN_TEST_RIGHT              (1)
#define TURN_TEST_START_DELAY_MS   1500U
#define TURN_TEST_STATUS_PERIOD_MS  250U

typedef struct
{
    int8_t direction;
    int32_t radius_cm;
    int32_t angle_deg;
} turn_test_case_t;

typedef enum
{
    TEST_READY_OUTBOUND = 0,
    TEST_OUTBOUND_DONE,
    TEST_RETURN_DONE
} test_phase_t;

/* Each case first drives forward through +90 degrees, then uses the same
 * steering side and -90 degrees to reverse along the same nominal arc. */
static const turn_test_case_t turn_cases[] =
{
    { TURN_TEST_LEFT,  20,  90 },
    { TURN_TEST_RIGHT, 35,  90 }
};

#define TURN_TEST_CASE_COUNT \
    ((uint32_t)(sizeof(turn_cases) / sizeof(turn_cases[0])))

static void UART_Print(const char *text)
{
    if (text == NULL) return;
    (void)HAL_UART_Transmit(&huart3, (uint8_t *)text,
                            (uint16_t)strlen(text), HAL_MAX_DELAY);
}

static const char *DirectionText(int8_t direction)
{
    return (direction == TURN_TEST_RIGHT) ? "RIGHT" : "LEFT";
}

static char DirectionLetter(int8_t direction)
{
    return (direction == TURN_TEST_RIGHT) ? 'R' : 'L';
}

static const char *TurnStateText(turn_state_t state)
{
    switch (state)
    {
        case TURN_IDLE:       return "IDLE";
        case TURN_RUNNING:    return "RUNNING";
        case TURN_DONE:       return "DONE";
        case TURN_ABORTED:    return "ABORTED";
        case TURN_TIMEOUT:    return "TIMEOUT";
        case TURN_WRONG_SIGN: return "WRONG_SIGN";
        case TURN_NO_TURN:    return "NO_TURN";
        case TURN_BAD_CONFIG: return "BAD_CONFIG";
        case TURN_NO_IMU:     return "NO_IMU";
        default:              return "UNKNOWN";
    }
}

static int32_t RoundX10(float value)
{
    return (int32_t)(value * 10.0f + ((value >= 0.0f) ? 0.5f : -0.5f));
}

static int32_t RoundX100(float value)
{
    return (int32_t)(value * 100.0f + ((value >= 0.0f) ? 0.5f : -0.5f));
}

static void FormatSignedX100(float value, char *out, size_t n)
{
    int32_t x100 = RoundX100(value);
    int32_t magnitude = (x100 < 0) ? -x100 : x100;

    (void)snprintf(out, n, "%c%ld.%02ld",
                   (x100 < 0) ? '-' : '+',
                   (long)(magnitude / 100),
                   (long)(magnitude % 100));
}

static void FormatUnsignedX100(float value, char *out, size_t n)
{
    int32_t x100 = RoundX100(value);
    if (x100 < 0) x100 = -x100;
    (void)snprintf(out, n, "%ld.%02ld",
                   (long)(x100 / 100), (long)(x100 % 100));
}

static float ExpectedChordCm(const turn_test_case_t *test)
{
    return (float)test->radius_cm * 1.41421356f;
}

static float MotionProgressDeg(int8_t direction,
                               int32_t signed_angle_deg,
                               float yaw_deg)
{
    float expected_sign = (direction == TURN_TEST_RIGHT) ? -1.0f : 1.0f;
    if (signed_angle_deg < 0) expected_sign = -expected_sign;
    return expected_sign * yaw_deg;
}

static void OLED_FourLines(const char *line1,
                           const char *line2,
                           const char *line3,
                           const char *line4)
{
    OLED_Clear();
    if (line1 != NULL) OLED_ShowString(0, 0, (const u8 *)line1);
    if (line2 != NULL) OLED_ShowString(0, 16, (const u8 *)line2);
    if (line3 != NULL) OLED_ShowString(0, 32, (const u8 *)line3);
    if (line4 != NULL) OLED_ShowString(0, 48, (const u8 *)line4);
    OLED_Refresh_Gram();
}

static void PrintCaseList(void)
{
    char msg[112];

    UART_Print("\r\nTURN_RETURN_CALIBRATION_CASES\r\n");
    for (uint32_t i = 0U; i < TURN_TEST_CASE_COUNT; ++i)
    {
        (void)snprintf(msg, sizeof(msg),
                       "  %lu: %s R=%ldcm OUT=+%lddeg BACK=-%lddeg\r\n",
                       (unsigned long)(i + 1U),
                       DirectionText(turn_cases[i].direction),
                       (long)turn_cases[i].radius_cm,
                       (long)turn_cases[i].angle_deg,
                       (long)turn_cases[i].angle_deg);
        UART_Print(msg);
    }
}

static void PrintMeasurementInstructions(const turn_test_case_t *test)
{
    char chord_text[24];
    char msg[320];

    FormatUnsignedX100(ExpectedChordCm(test), chord_text, sizeof(chord_text));
    (void)snprintf(msg, sizeof(msg),
                   "MEASURE_OUTBOUND:\r\n"
                   "  1. Physical chassis angle target = %ld deg.\r\n"
                   "  2. Mark rear-axle midpoint E without moving the car.\r\n"
                   "  3. Expected start-to-end chord for R=%ld cm is %s cm.\r\n",
                   (long)test->angle_deg, (long)test->radius_cm, chord_text);
    UART_Print(msg);

    UART_Print("  4. If physical angle is 90 deg: actual radius = measured chord / 1.4142.\r\n");
    UART_Print("After measuring, send B without repositioning the car.\r\n");
}

static void ShowSelectedCase(uint32_t index, test_phase_t phase)
{
    const turn_test_case_t *test = &turn_cases[index];
    char line1[22];
    char line2[22];
    char line3[22];
    char msg[220];

    (void)snprintf(line1, sizeof(line1), "CAL CASE %lu/%lu",
                   (unsigned long)(index + 1U),
                   (unsigned long)TURN_TEST_CASE_COUNT);
    (void)snprintf(line2, sizeof(line2), "%s R%ld A%ld",
                   DirectionText(test->direction),
                   (long)test->radius_cm, (long)test->angle_deg);

    if (phase == TEST_READY_OUTBOUND)
    {
        (void)snprintf(line3, sizeof(line3), "G:GO OUTBOUND");
        OLED_FourLines(line1, line2, line3, "N/P CASE  Q STOP");
    }
    else if (phase == TEST_OUTBOUND_DONE)
    {
        (void)snprintf(line3, sizeof(line3), "MARK END + MEASURE");
        OLED_FourLines(line1, line2, line3, "B:BACK  R:RESET");
    }
    else
    {
        (void)snprintf(line3, sizeof(line3), "CHECK START ERROR");
        OLED_FourLines(line1, line2, line3, "R:RESET N:NEXT");
    }

    (void)snprintf(msg, sizeof(msg),
                   "TURN_CAL,SELECTED,case=%lu/%lu,dir=%s,r_cm=%ld,a_deg=%ld,phase=%s\r\n",
                   (unsigned long)(index + 1U),
                   (unsigned long)TURN_TEST_CASE_COUNT,
                   DirectionText(test->direction),
                   (long)test->radius_cm, (long)test->angle_deg,
                   (phase == TEST_READY_OUTBOUND) ? "READY_OUT" :
                   (phase == TEST_OUTBOUND_DONE) ? "OUT_DONE" : "RETURN_DONE");
    UART_Print(msg);
}

static char ReadControlKey(void)
{
    uint8_t key = 0U;

    if (HAL_UART_Receive(&huart3, &key, 1U, 50U) != HAL_OK) return '\0';
    if (key >= (uint8_t)'a' && key <= (uint8_t)'z')
        key = (uint8_t)(key - (uint8_t)'a' + (uint8_t)'A');

    if (key == (uint8_t)'G' || key == (uint8_t)'B' ||
        key == (uint8_t)'R' || key == (uint8_t)'N' ||
        key == (uint8_t)'P' || key == (uint8_t)'L' ||
        key == (uint8_t)'Q')
        return (char)key;
    return '\0';
}

static void ShowRunning(uint32_t index,
                        int32_t signed_angle_deg,
                        float yaw_deg)
{
    const turn_test_case_t *test = &turn_cases[index];
    int32_t progress_x10 = RoundX10(MotionProgressDeg(test->direction,
                                                       signed_angle_deg,
                                                       yaw_deg));
    char line1[22];
    char line2[22];
    char line3[22];

    if (progress_x10 < 0) progress_x10 = -progress_x10;
    (void)snprintf(line1, sizeof(line1), "%s CASE %lu",
                   (signed_angle_deg >= 0) ? "OUT" : "BACK",
                   (unsigned long)(index + 1U));
    (void)snprintf(line2, sizeof(line2), "%c R%ld TARGET%ld",
                   DirectionLetter(test->direction),
                   (long)test->radius_cm,
                   (long)((signed_angle_deg < 0)
                          ? -signed_angle_deg : signed_angle_deg));
    (void)snprintf(line3, sizeof(line3), "GYRO %ld.%ld DEG",
                   (long)(progress_x10 / 10),
                   (long)(progress_x10 % 10));
    OLED_FourLines(line1, line2, line3, "KEEP CLEAR");
}

static turn_state_t RunMotion(uint32_t index, int32_t signed_angle_deg)
{
    const turn_test_case_t *test = &turn_cases[index];
    uint32_t elapsed_ms = 0U;
    uint32_t last_status_tick = 0U;
    int32_t radius_cm = 0;
    int32_t requested_deg = 0;
    int8_t direction = 0;
    float yaw_deg = 0.0f;
    float progress_deg;
    float controller_target_deg;
    char progress_text[24];
    char target_error_text[24];
    char msg[320];
    int rc;

    (void)snprintf(msg, sizeof(msg),
                   "TURN_CAL,ARMED,phase=%s,case=%lu,dir=%s,r_cm=%ld,a_deg=%ld,start_in_ms=%lu\r\n",
                   (signed_angle_deg >= 0) ? "OUT" : "BACK",
                   (unsigned long)(index + 1U),
                   DirectionText(test->direction),
                   (long)test->radius_cm, (long)signed_angle_deg,
                   (unsigned long)TURN_TEST_START_DELAY_MS);
    UART_Print(msg);
    OLED_FourLines("TURN CAL ARMED", "HANDS CLEAR", NULL, "START IN 1.5S");
    HAL_Delay(TURN_TEST_START_DELAY_MS);

    rc = Turn_StartRadiusAngle(test->radius_cm,
                               signed_angle_deg,
                               test->direction);
    if (rc != 0)
    {
        (void)snprintf(msg, sizeof(msg),
                       "TURN_CAL,START_ERROR,case=%lu,rc=%d\r\n",
                       (unsigned long)(index + 1U), rc);
        UART_Print(msg);
        RobotHW_MotorStop();
        RobotHW_ServoCenter();
        OLED_FourLines("START ERROR", "CHECK UART", NULL, "R:RESET");
        return TURN_BAD_CONFIG;
    }

    while (Turn_State() == TURN_RUNNING)
    {
        uint32_t now = HAL_GetTick();
        Turn_Step();
        if ((now - last_status_tick) >= TURN_TEST_STATUS_PERIOD_MS)
        {
            Turn_Status(&elapsed_ms, &radius_cm, &requested_deg,
                        &yaw_deg, &direction);
            ShowRunning(index, signed_angle_deg, yaw_deg);
            last_status_tick = now;
        }
        HAL_Delay(1U);
    }

    Turn_Status(&elapsed_ms, &radius_cm, &requested_deg,
                &yaw_deg, &direction);
    progress_deg = MotionProgressDeg(direction, requested_deg, yaw_deg);
    controller_target_deg = (float)((requested_deg < 0)
                                    ? -requested_deg : requested_deg)
                          * ((direction == TURN_TEST_RIGHT)
                             ? TURN_RIGHT_TARGET_SCALE
                             : TURN_LEFT_TARGET_SCALE);
    FormatSignedX100(progress_deg, progress_text, sizeof(progress_text));
    FormatSignedX100(progress_deg - controller_target_deg,
                     target_error_text, sizeof(target_error_text));

    (void)snprintf(msg, sizeof(msg),
                   "TURN_CAL,RESULT,phase=%s,case=%lu,state=%s,dir=%s,r_cm=%ld,requested_deg=%ld,controller_target_x100=%ld,gyro_progress=%s,gyro_target_error=%s,elapsed_ms=%lu\r\n",
                   (signed_angle_deg >= 0) ? "OUT" : "BACK",
                   (unsigned long)(index + 1U), TurnStateText(Turn_State()),
                   DirectionText(direction), (long)radius_cm,
                   (long)requested_deg, (long)RoundX100(controller_target_deg),
                   progress_text, target_error_text,
                   (unsigned long)elapsed_ms);
    UART_Print(msg);
    return Turn_State();
}

static void ShowOutboundResult(uint32_t index)
{
    const turn_test_case_t *test = &turn_cases[index];
    char line1[22];
    char line2[22];

    (void)snprintf(line1, sizeof(line1), "OUT DONE CASE %lu",
                   (unsigned long)(index + 1U));
    (void)snprintf(line2, sizeof(line2), "%c R%ld A%ld",
                   DirectionLetter(test->direction),
                   (long)test->radius_cm, (long)test->angle_deg);
    OLED_FourLines(line1, line2, "MARK END + MEASURE", "B:BACK  R:RESET");
}

static void ShowReturnResult(uint32_t index)
{
    const turn_test_case_t *test = &turn_cases[index];
    char line1[22];
    char line2[22];

    (void)snprintf(line1, sizeof(line1), "RETURN DONE %lu",
                   (unsigned long)(index + 1U));
    (void)snprintf(line2, sizeof(line2), "%c R%ld A%ld",
                   DirectionLetter(test->direction),
                   (long)test->radius_cm, (long)test->angle_deg);
    OLED_FourLines(line1, line2, "MEASURE START ERROR", "R:RESET N:NEXT");
}

void Test_TurnAngle(void)
{
    uint32_t selected = 0U;
    test_phase_t phase = TEST_READY_OUTBOUND;

    RobotHW_StartMotionPeripherals();
    OLED_Init();
    OLED_Display_On();
    Turn_Init();

    UART_Print("\r\n========================================\r\n");
    UART_Print(" 90-DEG ANGLE + RADIUS + RETURN CALIBRATION\r\n");
    UART_Print("========================================\r\n");
    UART_Print("G = run forward/outbound arc\r\n");
    UART_Print("B = reverse the same arc back to the start\r\n");
    UART_Print("R = reset current case after manually repositioning\r\n");
    UART_Print("N/P = next/previous case, L = list, Q = halt\r\n");
    UART_Print("Before G: mark the rear-axle midpoint S and align the chassis with the start line.\r\n");
    PrintCaseList();
    ShowSelectedCase(selected, phase);

    while (1)
    {
        char key = ReadControlKey();

        if (key == 'G')
        {
            if (phase != TEST_READY_OUTBOUND)
            {
                UART_Print("TURN_CAL,REJECTED,reason=RESET_REQUIRED,use=R\r\n");
                continue;
            }
            if (RunMotion(selected, turn_cases[selected].angle_deg) == TURN_DONE)
            {
                phase = TEST_OUTBOUND_DONE;
                PrintMeasurementInstructions(&turn_cases[selected]);
                ShowOutboundResult(selected);
            }
        }
        else if (key == 'B')
        {
            if (phase != TEST_OUTBOUND_DONE)
            {
                UART_Print("TURN_CAL,REJECTED,reason=OUTBOUND_NOT_DONE,use=G\r\n");
                continue;
            }
            if (RunMotion(selected, -turn_cases[selected].angle_deg) == TURN_DONE)
            {
                phase = TEST_RETURN_DONE;
                UART_Print("MEASURE_RETURN: rear-axle midpoint distance from S and final chassis heading error from the start line.\r\n");
                UART_Print("Suggested pass limits: position error <= 2 cm and heading error <= 2 deg.\r\n");
                ShowReturnResult(selected);
            }
        }
        else if (key == 'R')
        {
            Turn_Stop();
            RobotHW_MotorStop();
            RobotHW_ServoCenter();
            phase = TEST_READY_OUTBOUND;
            UART_Print("TURN_CAL,RESET_CASE,reposition_robot_before_G\r\n");
            ShowSelectedCase(selected, phase);
        }
        else if (key == 'N')
        {
            selected = (selected + 1U) % TURN_TEST_CASE_COUNT;
            phase = TEST_READY_OUTBOUND;
            ShowSelectedCase(selected, phase);
        }
        else if (key == 'P')
        {
            selected = (selected == 0U)
                     ? (TURN_TEST_CASE_COUNT - 1U) : (selected - 1U);
            phase = TEST_READY_OUTBOUND;
            ShowSelectedCase(selected, phase);
        }
        else if (key == 'L')
        {
            PrintCaseList();
            ShowSelectedCase(selected, phase);
        }
        else if (key == 'Q')
        {
            Turn_Stop();
            RobotHW_MotorStop();
            RobotHW_ServoCenter();
            UART_Print("TURN_CAL,HALTED\r\n");
            OLED_FourLines("TURN CAL HALTED", NULL, NULL, "RESET TO RESTART");
            while (1) HAL_Delay(1000U);
        }
    }
}
