# Servo and PWM Experiments

I recorded steering-servo parameters and tuned PWM settings during robot bring-up. The useful result was a set of testable settings tied to the actual steering mechanism, rather than assuming that a nominal centre pulse would make the chassis travel straight.

## Steering centre evolved with the setup

| Stage / record | Pulse width | Meaning |
|---|---:|---|
| Generated peripheral initialization | 1500 μs | Initial TIM12 compare value; nominal starting point |
| Initial physical test note | 1730 μs | Centre candidate; the note describes approximately straight motion with slight right drift |
| Later straight-control/minimum-radius notes | 1712 μs | Working centre in that test configuration |
| Selected source snapshot | 1726 μs | Shared centre trim; source comment calls for floor validation |

These values belong to different stages. They are not interchangeable results from one controlled comparison. The shared source also mentions a previous 1716 μs straight centre. Changes in mounting, steering alignment, load and test conditions need to be considered when selecting a centre value.

The recorded endpoint experiment increased steering pulse width gradually. The notes report continued motion up to 2600 μs and no further servo response at 2700 μs while STM32/OLED operation continued. That is an observation from this assembly, not a universal servo specification. The selected shared constants use left/centre/right values of 1000/1726/2600 μs.

Review: [shared robot parameters](../firmware/Core/Inc/drivers/robot_tuning.h) · [servo setter](../firmware/Core/Src/drivers/robot_hw.c) · [component experiment](../firmware/Core/Src/tests/component/test_servo.c).

## Timer setup makes the experiment measurable

TIM12_CH1 on PB14 uses prescaler 15 and period 19999. At the recorded 16 MHz timer clock:

```text
16,000,000 / (15 + 1) = 1,000,000 counts/s
1 count = 1 μs
20,000 counts = 20 ms = 50 Hz
```

The servo utility prints the pulse width over UART and cycles through centre and ±100 μs positions with a 1500 ms hold. This provides an inspectable connection between a timer compare value and observed wheel position.

## Motor PWM and steering pulse width

| Quantity | Drive motor | Steering servo |
|---|---|---|
| Outputs | TIM4_CH3/4 and TIM9_CH1/2 | TIM12_CH1 |
| Period in this snapshot | Approximately 450 μs / 2.22 kHz | 20 ms / 50 Hz |
| Tuned value | PWM compare magnitude and left/right balance | Pulse width in μs, centre and endpoints |
| Physical observation | Start, direction, wheel travel and stopping | Steering position, residual drift and useful range |

Each motor direction is produced by choosing which driver input receives PWM. The component test checks individual left/right forward and reverse actions before paired movement. The shared driver applies a configured global PWM scale and clamps values to the timer limits.

The selected snapshot sets `MOTOR_SPEED_SCALE_PERCENT` to 120. This scales requested PWM; it does not establish a 20% vehicle-speed increase. Motor load, battery voltage and mechanical behavior affect the relationship between PWM and travel.

Review: [motor component test](../firmware/Core/Src/tests/component/test_motor.c) · [driver output mapping](../firmware/Core/Src/drivers/robot_hw.c) · [straight parameters](../firmware/Core/Inc/control/straight_tuning.h) · [turn parameters](../firmware/Core/Inc/control/turn_tuning.h).

## Reading tuning observations carefully

The supplied notes distinguish wheel-distance agreement from chassis drift: matching encoder distances can coexist with a steering-centre error. Their stored report is therefore useful diagnostic context, without proving overall path accuracy. Likewise, “Need to try” control changes and recommended test sequences remain proposed experiments until backed by a run record.

For future comparison runs, record the centre pulse, motor PWM, commanded motion, physical distance/angle, left/right encoder distances and floor/battery conditions together. Change one parameter at a time so the observation can be connected to a specific adjustment.
