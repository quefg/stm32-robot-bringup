# Selected STM32 Source Snapshot

This directory contains **41 selected source/configuration files** from the existing MDP21_8 STM32CubeIDE working tree, plus four original component license texts. Files were copied byte-for-byte; sizes and SHA-256 hashes for all 45 files are recorded in [`source-manifest.json`](source-manifest.json). Snapshot prepared on 15 November 2026.

This is source evidence for the portfolio. It is **not a complete, independently buildable firmware tree**: startup/linker files, HAL/CMSIS/RTOS implementation dependencies, the full application and several supporting modules are omitted. No firmware edits, clean build, flashing or hardware retest were performed as part of packaging it.

## Review paths

| Area | Files |
|---|---|
| Pin, clock and peripheral configuration | [`mdp21_8.ioc`](mdp21_8.ioc), [`Core/Src/main.c`](Core/Src/main.c), [`stm32f4xx_hal_msp.c`](Core/Src/stm32f4xx_hal_msp.c) |
| Interrupt dispatch and HAL time base | [`stm32f4xx_it.c`](Core/Src/stm32f4xx_it.c), [`stm32f4xx_hal_timebase_tim.c`](Core/Src/stm32f4xx_hal_timebase_tim.c) |
| Sensor acquisition and conversion | [`Core/Src/sensors/`](Core/Src/sensors/), matching `Core/Inc/sensors` headers |
| Servo, PWM and shared parameters | [`robot_hw.c`](Core/Src/drivers/robot_hw.c), [`robot_tuning.h`](Core/Inc/drivers/robot_tuning.h), `Core/Inc/control/*_tuning.h` |
| Straight-line wheel PI, heading PID and filtering | [`straight_control.c`](Core/Src/control/straight_control.c), [`straight_control.h`](Core/Inc/control/straight_control.h), [`straight_tuning.h`](Core/Inc/control/straight_tuning.h) |
| Radius/angle turning and stopping | [`turn_control.c`](Core/Src/control/turn_control.c), [`turn_control.h`](Core/Inc/control/turn_control.h), [`turn_tuning.h`](Core/Inc/control/turn_tuning.h) |
| Component checks | [`Core/Src/tests/component/`](Core/Src/tests/component/) |
| Known-distance IR capture | [`test_ir_calibration.c`](Core/Src/tests/calibration/test_ir_calibration.c) |
| Motion and gyro calibration checks | [`test_straight_control.c`](Core/Src/tests/calibration/test_straight_control.c), [`test_turn_angle.c`](Core/Src/tests/calibration/test_turn_angle.c), [`test_min_turn_radius.c`](Core/Src/tests/calibration/test_min_turn_radius.c), [`test_gyro_calibration.c`](Core/Src/tests/calibration/test_gyro_calibration.c), and their matching headers |

The [motion-control tuning record](../docs/motion-control-tuning.md) explains the separate wheel PI, steering PID and IIR filter settings, the manual steering sweeps, and the current 90-degree outbound/return calibration cases. Source constants and printed test fields describe implementation; they do not establish measured accuracy.

## Configuration context

The selected `test_config.h` uses `ASSESSMENT_COMMAND_INTERFACE`. That application's full command implementation is outside this curated set. The original `main.c` invokes the selected application before CMSIS kernel initialization; RTOS configuration files are included as configuration context.

The selected clock configuration is HSI 16 MHz without PLL. The current ultrasonic module uses PB15 trigger and PC8/TIM8_CH3 capture. Earlier project notes use a different PC7/PC6 mapping. Historical servo and PWM settings likewise differ between experiments; the [portfolio documentation](../docs/peripheral-map.md) identifies the stages.

Some retained test text still describes the earlier IR conversion range. The actual selected conversion tables in `Core/Src/sensors/ir_sensor.c` define left 10–50 cm and right 10–60 cm. Original source is preserved for traceability rather than silently corrected in this snapshot.

## Provenance

My work covered MCU configuration, sensor calibration, resistor/wire soldering, servo/PWM tuning, straight-line PID/filter tuning and turning sweeps within the team robot project. The snapshot also includes team modules and generated STM32 configuration. Original STMicroelectronics and component notices remain in their source files and [`licenses/`](licenses/).
