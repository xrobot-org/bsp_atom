#include "app_main.h"
#include "libxr.hpp"
#include "main.h"
#include "stm32_adc.hpp"
#include "stm32_can.hpp"
#include "stm32_canfd.hpp"
#include "stm32_gpio.hpp"
#include "stm32_i2c.hpp"
#include "stm32_power.hpp"
#include "stm32_pwm.hpp"
#include "stm32_spi.hpp"
#include "stm32_timebase.hpp"
#include "stm32_uart.hpp"
#include "stm32_usb.hpp"
#include "app_framework.hpp"
#include "xrobot_main.hpp"

using namespace LibXR;

/* User Code Begin 1 */
/* User Code End 1 */
/* External HAL Declarations */
extern FDCAN_HandleTypeDef hfdcan1;
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi3;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim8;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* DMA Resources */
static uint8_t spi1_tx_buf[32];
static uint8_t spi1_rx_buf[32];
static uint8_t spi3_tx_buf[32];
static uint8_t spi3_rx_buf[32];
static uint8_t usart1_tx_buf[128];
static uint8_t usart1_rx_buf[128];
static uint8_t usart2_tx_buf[128];
static uint8_t usart2_rx_buf[128];

extern "C" void app_main(void) {
  /* User Code Begin 2 */
  
  /* User Code End 2 */
  STM32Timebase timebase;
  PlatformInit(2, 512);
  STM32PowerManager power_manager;

  /* GPIO Configuration */
  STM32GPIO BMI088_CS_2(BMI088_CS_2_GPIO_Port, BMI088_CS_2_Pin);
  STM32GPIO BMI088_INT_1(BMI088_INT_1_GPIO_Port, BMI088_INT_1_Pin, EXTI15_10_IRQn);
  STM32GPIO IMU_CS(IMU_CS_GPIO_Port, IMU_CS_Pin);
  STM32GPIO IMU_INT1(IMU_INT1_GPIO_Port, IMU_INT1_Pin, EXTI0_IRQn);
  STM32GPIO BMI088_CS_1(BMI088_CS_1_GPIO_Port, BMI088_CS_1_Pin);
  STM32GPIO BMI088_INT_2(BMI088_INT_2_GPIO_Port, BMI088_INT_2_Pin, EXTI9_5_IRQn);
  STM32GPIO LED(LED_GPIO_Port, LED_Pin);


  STM32PWM pwm_tim1_ch1(&htim1, TIM_CHANNEL_1);

  STM32PWM pwm_tim2_ch2(&htim2, TIM_CHANNEL_2);

  STM32SPI spi1(&hspi1, spi1_rx_buf, spi1_tx_buf, 3);

  STM32SPI spi3(&hspi3, spi3_rx_buf, spi3_tx_buf, 3);

  STM32UART usart1(&huart1,
              usart1_rx_buf, usart1_tx_buf, 5, 5);

  STM32UART usart2(&huart2,
              usart2_rx_buf, usart2_tx_buf, 5, 5);

  STM32CANFD fdcan1(&hfdcan1, "fdcan1", 5);

  STDIO::read_ = &usart2.read_port_;
  STDIO::write_ = &usart2.write_port_;
  RamFS ramfs("XRobot");
  Terminal<32, 32, 5, 5> terminal(ramfs);
  auto terminal_task = Timer::CreateTask(terminal.TaskFun, &terminal, 10);
  Timer::Add(terminal_task);
  Timer::Start(terminal_task);


  LibXR::HardwareContainer<
    LibXR::Entry<LibXR::PowerManager>,
    LibXR::Entry<LibXR::GPIO>,
    LibXR::Entry<LibXR::GPIO>,
    LibXR::Entry<LibXR::GPIO>,
    LibXR::Entry<LibXR::GPIO>,
    LibXR::Entry<LibXR::GPIO>,
    LibXR::Entry<LibXR::GPIO>,
    LibXR::Entry<LibXR::GPIO>,
    LibXR::Entry<LibXR::PWM>,
    LibXR::Entry<LibXR::PWM>,
    LibXR::Entry<LibXR::SPI>,
    LibXR::Entry<LibXR::SPI>,
    LibXR::Entry<LibXR::UART>,
    LibXR::Entry<LibXR::UART>,
    LibXR::Entry<LibXR::FDCAN>,
    LibXR::Entry<LibXR::RamFS>,
    LibXR::Entry<LibXR::Terminal<32, 32, 5, 5>>
  > peripherals{
    {power_manager, {"power_manager"}},
    {BMI088_CS_2, {"BMI088_CS_2"}},
    {BMI088_INT_1, {"BMI088_INT_1"}},
    {IMU_CS, {"IMU_CS"}},
    {IMU_INT1, {"IMU_INT1"}},
    {BMI088_CS_1, {"BMI088_CS_1"}},
    {BMI088_INT_2, {"BMI088_INT_2"}},
    {LED, {"LED"}},
    {pwm_tim1_ch1, {"pwm_tim1_ch1"}},
    {pwm_tim2_ch2, {"pwm_tim2_ch2"}},
    {spi1, {"spi1"}},
    {spi3, {"spi3"}},
    {usart1, {"usart1"}},
    {usart2, {"usart2"}},
    {fdcan1, {"fdcan1"}},
    {ramfs, {"ramfs"}},
    {terminal, {"terminal"}}
  };

  /* User Code Begin 3 */
  auto power_cmd_file = LibXR::RamFS::CreateFile<STM32PowerManager *>(
      "power",
      [](LibXR::STM32PowerManager *&power, int argc, char **argv) {
        if (argc == 2) {
          if (strcmp(argv[1], "reset") == 0) {
            power->Reset();
          } else if (strcmp(argv[1], "shutdown") == 0) {
            power->Shutdown();
          } else if (strcmp(argv[1], "bl") == 0) {
            HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
            HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);

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
      },
      &power_manager);

  ramfs.Add(power_cmd_file);
  XRobotMain(peripherals);
  /* User Code End 3 */
}