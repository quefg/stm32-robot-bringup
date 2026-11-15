# Source Provenance and Notices

This portfolio combines the author's photographs and contribution record with selected files from the MDP21_8 team robot project. The project record includes the configuration note dated 21 August 2026 and supplied calibration/tuning records. The original note's document link and local machine paths are not included.

## Selected source

The files under `firmware/Core` and `firmware/mdp21_8.ioc` are preserved byte-for-byte from the selected working tree. They include generated STM32 setup, project modules and component/calibration utilities. My confirmed contribution areas are pin/NVIC/prescaler configuration, IR/ultrasonic calibration, ultrasonic-resistor and IR-wire soldering, and servo/PWM tuning. The wider team's firmware and library infrastructure provide context for that work.

The included generated STM32 files retain their STMicroelectronics copyright and license-reference headers. No new blanket license has been applied to the selected project source. The snapshot is incomplete; bundled notices do not imply that all corresponding libraries are included.

## Original dependency notice texts

These are original texts from the source project's dependencies, retained for provenance:

| Original component | Preserved notice |
|---|---|
| STM32F4 HAL Driver | [`STM32F4xx_HAL_Driver-LICENSE.txt`](firmware/licenses/STM32F4xx_HAL_Driver-LICENSE.txt) |
| ARM CMSIS | [`CMSIS-LICENSE.txt`](firmware/licenses/CMSIS-LICENSE.txt) |
| ST STM32F4 CMSIS device component | [`STM32F4xx_CMSIS_Device-LICENSE.txt`](firmware/licenses/STM32F4xx_CMSIS_Device-LICENSE.txt) |
| FreeRTOS | [`FreeRTOS-LICENSE.txt`](firmware/licenses/FreeRTOS-LICENSE.txt) |

Per-file copyright and license notices remain relevant. File hashes and original relative locations are available in [`firmware/source-manifest.json`](firmware/source-manifest.json).

## Images and measurements

Original build photographs are supplied by the project author. The AI-assisted annotated overview and build-process panels are presentation aids derived from those photos; original images remain the authoritative photographic record and are retained for inspection. Callouts are not physical measurements. The IR analysis identifies the supplied measurement log and keeps raw records distinct from summaries and the selected firmware tables. Any photographed third-party instructional wiring card is a reference, rather than an original circuit design by the author.
