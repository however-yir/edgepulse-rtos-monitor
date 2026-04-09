# Third-Party Notices

This repository includes third-party components. Use, redistribution, and
modification must comply with the original license terms for each component.

## 1. STMicroelectronics STM32Cube Components

- Typical locations:
  - `Core/Inc/*.h` and `Core/Src/*.c` files with ST copyright headers
  - `Drivers/STM32F7xx_HAL_Driver`
  - `Drivers/BSP/STM32F7xx_Nucleo_144`
  - `Drivers/CMSIS`
- License: per-file license headers provided by STMicroelectronics.

## 2. FreeRTOS

- Typical location:
  - `Middlewares/Third_Party/FreeRTOS`
- License: see the FreeRTOS source distribution in this repository.

## 3. LwIP

- Typical location:
  - `Middlewares/Third_Party/LwIP`
- License: see `Middlewares/Third_Party/LwIP/COPYING` and related upstream
  notices in the LwIP source tree.

## 4. Local Integration Layer

- Files added specifically for this repository-level integration (for example
  repository docs and custom glue code without a different file header) follow
  the root `LICENSE`.

If you redistribute this project, keep all original third-party notices and
license headers intact.
