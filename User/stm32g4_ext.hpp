#pragma once

#include "flash.hpp"
#include "libxr.hpp"
#include "main.h"
#include "stm32_power.hpp"

class STM32G4Flash : public LibXR::Flash {
 public:
  STM32G4Flash(size_t start_offset, size_t min_erase_size,
               size_t min_write_size)
      : Flash(
            min_erase_size, min_write_size,
            LibXR::RawData(reinterpret_cast<void *>(FLASH_BASE + start_offset),
                           2 * min_erase_size)),
        start_offset_(start_offset),
        min_erase_size_(min_erase_size) {}

  ErrorCode Erase(size_t offset, size_t size) override {
    if ((offset + size) > 2 * min_erase_size_) {
      return ErrorCode::OUT_OF_RANGE;
    }

    offset += start_offset_;

    HAL_FLASH_Unlock();

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    for (size_t addr = offset; addr < offset + size; addr += FLASH_PAGE_SIZE) {
      FLASH_EraseInitTypeDef erase_init = {};
      uint32_t sector_error = 0;

      erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
      erase_init.Page = (addr - FLASH_BASE) / FLASH_PAGE_SIZE;
      erase_init.NbPages = 1;
      erase_init.Banks = FLASH_BANK_1;

      if (HAL_FLASHEx_Erase(&erase_init, &sector_error) != HAL_OK) {
        HAL_FLASH_Lock();
        return ErrorCode::FAILED;
      }
    }

    HAL_FLASH_Lock();
    return ErrorCode::OK;
  }

  ErrorCode Write(size_t offset, LibXR::ConstRawData data) override {
    if ((offset + data.size_) > 2 * min_erase_size_) {
      return ErrorCode::OUT_OF_RANGE;
    }

    offset += start_offset_;

    HAL_FLASH_Unlock();

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    uint32_t addr = FLASH_BASE + offset;
    const uint8_t *src = static_cast<const uint8_t *>(data.addr_);

    // G4 requires 64-bit aligned writes (double word)
    while (addr < (FLASH_BASE + offset + data.size_)) {
      uint64_t word = 0xFFFFFFFFFFFFFFFF;
      memcpy(&word, src,
             std::min<size_t>(8, (FLASH_BASE + offset + data.size_) - addr));

      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, word) !=
          HAL_OK) {
        HAL_FLASH_Lock();
        return ErrorCode::FAILED;
      }

      addr += 8;
      src += 8;
    }

    HAL_FLASH_Lock();
    return ErrorCode::OK;
  }

 private:
  size_t start_offset_;
  size_t min_erase_size_;
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
