# Peripheral Map

This map describes the source snapshot in [`firmware/`](../firmware/README.md). The project uses a WHEELTEC STM32F407VET6 board. The configuration notes identify the onboard AT8236 motor drivers and ICM20948 IMU. My work included initial pin configuration, NVIC settings and timer-prescaler setup in STM32CubeIDE.

## Pins and interfaces

| Function | MCU pins | Peripheral / setting |
|---|---|---|
| Programming/debug | PA13 / PA14 | SWDIO / SWCLK; Serial Wire mode |
| Left drive motor (MOTORA) | PB8 / PB9 | TIM4_CH3 / CH4 PWM; Motor A IN2 / IN1 |
| Right drive motor (MOTORB) | PE5 / PE6 | TIM9_CH1 / CH2 PWM; Motor B IN1 / IN2 |
| Left encoder | PA15 / PB3 | TIM2_CH1 / CH2, quadrature TI12 |
| Right encoder | PB4 / PB5 | TIM3_CH1 / CH2, quadrature TI12 |
| Steering servo | PB14 | TIM12_CH1 PWM |
| Left / right IR signal | PC0 / PC1 | ADC1_IN10 / IN11, two-channel scan |
| Ultrasonic trigger | PB15 | GPIO output |
| Ultrasonic echo | PC8 | TIM8_CH3, AF3, both-edge input capture through a voltage divider |
| IMU SCL / SDA | PB10 / PB11 | I2C2, 100 kHz, 7-bit addressing |
| Serial TX / RX | PD8 / PD9 | USART3, 115200 baud, 8 data bits, no parity, 1 stop bit |
| OLED control/data | PD11 / PD12 / PD13 / PD14 | GPIO outputs for DC / RES / SDA / SCL |
| HAL millisecond tick | Internal | TIM6 |

Serial Wire mode makes PA15/PB3/PB4 available for the encoder interfaces. Two drive motors each use a pair of driver inputs; four PWM channels do not mean four independently driven wheels.

## Timer calculations

Both the supplied initial configuration record and this selected source use HSI directly at **16 MHz**, with PLL disabled and AHB/APB dividers set to 1. The table below is calculated from those settings.

```text
timer counter frequency = timer input clock / (PSC + 1)
PWM frequency           = timer counter frequency / (ARR + 1)
```

| Use | Timer | PSC | ARR | Derived behavior |
|---|---|---:|---:|---|
| Left motor PWM | TIM4 | 0 | 7199 | 16 MHz / 7200 ≈ 2.22 kHz |
| Right motor PWM | TIM9 | 0 | 7199 | 16 MHz / 7200 ≈ 2.22 kHz |
| Servo PWM | TIM12 | 15 | 19999 | 1 MHz counter; 1 μs/count; 20 ms period / 50 Hz |
| Ultrasonic capture | TIM8 | 15 | 65535 | 1 MHz counter; echo width in μs, with wrap handling |
| Encoders | TIM2 / TIM3 | 0 | 65535 | Quadrature TI12; input filter 10 on both channels |
| HAL tick | TIM6 | Calculated from PCLK1 | 999 | 1 MHz counter; 1 ms update |

ADC1 uses a PCLK divider of 4, 12-bit resolution and 144-cycle sampling. The two scan ranks correspond to the left and right IR sensors. Timing values here are configuration-derived, without a claim of oscilloscope validation. A different clock tree requires recalculating these values.

## NVIC and initialization

The selected configuration enables TIM8 capture/compare and USART3 interrupts at priority `(5, 0)`, with TIM6 as the HAL tick at priority 15. The priority group is `NVIC_PRIORITYGROUP_4`.

The relevant review path is:

```text
mdp21_8.ioc
  → main.c: clock, peripheral initialization, timer parameters
  → stm32f4xx_hal_msp.c: clocks, GPIO alternate functions, NVIC
  → stm32f4xx_it.c: interrupt dispatch
  → sensors/ultrasonic_sensor.c: input-capture callback and measurement
```

See the [CubeMX settings](../firmware/mdp21_8.ioc), [initialization](../firmware/Core/Src/main.c), [MSP setup](../firmware/Core/Src/stm32f4xx_hal_msp.c) and [interrupt handlers](../firmware/Core/Src/stm32f4xx_it.c).

## Configuration stages

| Record | Ultrasonic mapping | Clock |
|---|---|---|
| Earlier wiring table in the project notes | PC7 trigger; PC6 echo input | HSI 16 MHz, PLL disabled |
| Selected firmware snapshot | PB15 trigger; PC8 TIM8_CH3 echo capture | HSI 16 MHz, PLL disabled |

These are different wiring/measurement versions. The earlier PC6 input is still present in generated GPIO configuration; its existence does not change the active driver's PB15/PC8 assignments. Match the installed wiring to the driver version before interpreting readings.

The source contains CMSIS-V2/FreeRTOS configuration, but its current `main.c` calls the selected application before kernel initialization. This portfolio therefore describes peripheral setup and component work without treating generated RTOS scaffolding as evidence of a completed multitasking design.
