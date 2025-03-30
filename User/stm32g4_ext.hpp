#pragma once

#include "flash.hpp"
#include "libxr.hpp"
#include "main.h"
#include "stm32_power.hpp"

class STM32G4Flash : public LibXR::Flash {
 public:
  STM32G4Flash(size_t start_offset, size_t min_erase_size, size_t min_write_size)
      : Flash(min_erase_size, min_write_size,
              RawData(reinterpret_cast<void *>(FLASH_BASE + start_offset),
                      2 * min_erase_size)),
        start_offset_(start_offset) {}

  ErrorCode Erase(size_t offset, size_t size) override {
    UNUSED(size);

    FLASH_EraseInitTypeDef flash_erase;

    uint32_t sector_error = 0;

    HAL_FLASH_Unlock();

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    flash_erase.NbPages = 1;
    flash_erase.Banks = FLASH_BANK_1;
    flash_erase.Page = (offset + start_offset_) / FLASH_PAGE_SIZE;
    flash_erase.TypeErase = FLASH_TYPEERASE_PAGES;

    HAL_FLASHEx_Erase(&flash_erase, &sector_error);

    HAL_FLASH_Lock();

    return ErrorCode::OK;
  }

  ErrorCode Write(size_t offset, ConstRawData data) override {
    uint8_t *data_u8 =
        const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(data.addr_));

    if (HAL_FLASH_Unlock() != HAL_OK) {
      return ErrorCode::BUSY;
    }
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    uint32_t startaddr = offset + start_offset_ + FLASH_BASE;
    uint32_t endaddr = startaddr + data.size_;

    while ((startaddr < endaddr)) {
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, startaddr,
                            *(reinterpret_cast<uint64_t *>(data_u8))) ==
          HAL_OK) {
        data_u8 = data_u8 + 8;
        startaddr = startaddr + 8;
      } else {
        break;
      }
    }

    HAL_FLASH_Lock();

    return ErrorCode::OK;
  }

 private:
  size_t start_offset_;
};

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
