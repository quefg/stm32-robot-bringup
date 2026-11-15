# Motion Control Tuning

My tuning work covered straight-line PID and filtering, steering PWM sweeps, and turning experiments within the team robot project. The selected source makes the separate control loops and test procedures inspectable.

## Straight motion: separate wheel balance from heading

The two correction loops act on different outputs:

| Loop | Feedback | Tuned output | Implementation |
|---|---|---|---|
| Wheel-speed PI | Calibrated left/right encoder travel over the measured control interval | Differential left/right motor PWM | `Straight_Step()` in [`straight_control.c`](../firmware/Core/Src/control/straight_control.c#L673) |
| Heading PID | Integrated gyro yaw, its integral, and filtered gyro angular rate | Steering-servo pulse around the calibrated centre | `ComputeSteeringUs()` in [`straight_control.c`](../firmware/Core/Src/control/straight_control.c#L187) |

Wheel mismatch is normalized to the nominal control interval, filtered, and fed into PI. The heading controller uses gyro rate as its derivative term. It limits the integral and steering output, and reverses the steering correction relationship when driving backward. Ultrasonic approach mode additionally applies a proportional correction to accumulated left/right path difference.

The included [`straight_tuning.h`](../firmware/Core/Inc/control/straight_tuning.h#L53) records these **configured examples**:

| Setting | Selected value |
|---|---:|
| Nominal control period | 20 ms |
| Wheel PI gains | KP 2.5; KI 10.0 |
| Encoder-mismatch IIR coefficient | α 0.30 |
| Gyro IIR coefficient for straight motion | α 0.10 |
| Steering PID gains | KP 9.0 μs/°; KI 0.6 μs/(°·s); KD 2.0 μs/(°/s) |
| Yaw / gyro-rate deadbands | 0.15° / 0.08°/s |

These are the selected source settings, not optimal gains or a quantified improvement. A filter coefficient changes smoothing and response delay; it is distinct from a PID gain. The source also limits integrators and correction outputs, which matters when interpreting saturation and oscillation during tuning.

## Gyro calibration and filtering

[`RobotHW_GyroCalibrate()`](../firmware/Core/Src/drivers/robot_hw.c#L348) averages valid stationary gyro-Z readings to obtain a bias. Shared defaults specify 500 samples, 10 ms spacing and 500 ms warmup. [`RobotHW_GyroUpdateYaw()`](../firmware/Core/Src/drivers/robot_hw.c#L419) applies a first-order IIR filter, subtracts the bias, converts to angular rate, applies a rate deadband and integrates using the measured time interval. Failed I²C reads hold the yaw estimate rather than introducing a false rate sample.

The straight and turn controllers use different gyro IIR coefficients: 0.10 and 0.35 respectively. Both use first-order IIR filtering with bias correction. The separate [`gyro calibration utility`](../firmware/Core/Src/tests/calibration/test_gyro_calibration.c) exposes raw and corrected readings for inspection.

## Steering sweeps and turn calibration

I used steering pulse sweeps to inspect the useful servo response and tune steering parameters. The project notes record gradual pulse increases: response continued to 2600 μs; at 2700 μs the servo stopped responding while the STM32/OLED continued operating. The notes associate that setup with a 1712 μs centre and an approximately 41 cm right-turn minimum radius at PWM 3000. These are historical observations from that setup, not the current universal centre or a validated radius model.

The selected [`minimum-radius routine`](../firmware/Core/Src/control/turn_control.c#L73) exposes a manually selected steering pulse and a 180-degree test. [`Test_MinTurnRadius()`](../firmware/Core/Src/tests/calibration/test_min_turn_radius.c) runs that routine, which reports gyro/encoder information and asks for physical endpoint and heading measurements. The steering pulse selection supports manual sweep experiments; it is not an automatic sweep result table.

The current [`Test_TurnAngle()` cases](../firmware/Core/Src/tests/calibration/test_turn_angle.c#L33) are:

| Case | Outbound | Return |
|---|---|---|
| Left, requested radius 20 cm | Forward +90° | Same steering side, reverse −90° |
| Right, requested radius 35 cm | Forward +90° | Same steering side, reverse −90° |

The test prompts the operator to mark the rear-axle midpoint, measure physical heading and endpoint chord, then inspect return-to-start error. UART output includes requested angle, scaled controller target, gyro progress and elapsed time. A gyro error relative to that controller target is not an independent physical-angle measurement.

[`turn_tuning.h`](../firmware/Core/Inc/control/turn_tuning.h) separates steering/radius scale, angle-target scale, PWM stages and stopping lead. The controller predicts coast using measured gyro rate. Its configured 0.7-degree stop tolerance is an algorithm threshold, not an achieved accuracy specification. The source labels some current higher-power settings as initial heavier-chassis trials requiring floor validation. No completed automatic 90–360-degree sweep or multi-trial accuracy result is inferred from those settings.

## What the recorded observations establish

The supplied configuration note is headed 21 August 2026 and includes later undated working sections. One historical straight-control report records encoder-derived wheel travel of 121.076 cm left and 121.556 cm right, a −0.480 cm difference. The same notes describe about 2.5 cm of physical left drift. This illustrates why matching encoder travel alone does not establish chassis path accuracy. The note also records a stationary gyro bias near 179 raw counts and corrected readings around −20 to +20 raw counts; these are observations from that calibration stage.

Servo centre candidates, gains, filter settings and physical results must be compared within their recorded configuration. Suggestions headed “Need to try” and recommended future test sequences remain plans. A before/after performance comparison will require matched trials and their measured results.

For subsequent comparison runs, the useful record is the parameter version together with actual ruler/tape distance, sideways deviation, physical angle/chord, encoder travel, gyro yaw, PWM, floor and battery conditions. [`Test_StraightControl()`](../firmware/Core/Src/tests/calibration/test_straight_control.c) currently requests 100 cm and reports encoder distances and yaw; it explicitly asks for real floor-distance and sideways-deviation measurements afterward.
