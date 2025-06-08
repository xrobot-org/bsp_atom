#pragma once

#include "flash.hpp"
#include "libxr.hpp"
#include "main.h"
#include "stm32_power.hpp"

__attribute__((unused)) static int STM32G4PowerFun(
    LibXR::STM32PowerManager *power, int argc, char **argv) {
  {
    if (argc == 2) {
      if (strcmp(argv[1], "reset") == 0) {
        power->Reset();
      } else if (strcmp(argv[1], "shutdown") == 0) {
        power->Shutdown();
      } else if (strcmp(argv[1], "bl") == 0) {
        __set_PRIMASK(1);

        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL = 0;

        HAL_RCC_DeInit();

        for (int i = 0; i < 8; i++) {
          NVIC->ICER[i] = 0xFFFFFFFF;
          NVIC->ICPR[i] = 0xFFFFFFFF;
        }

        __set_PRIMASK(0);

        __set_MSP(*(__IO uint32_t *)0x1FFF0000);
        void (*sys_mem_boot_jump)(void) =
            (void (*)(void))(*((uint32_t *)0x1FFF0004));
        sys_mem_boot_jump();
      }
    }
    return 0;
  }
}
