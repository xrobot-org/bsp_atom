#include "app_main.h"

#include "app_framework.hpp"
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
#include "xrobot_main.hpp"

using namespace LibXR;

/* User Code Begin 1 */
#include "stm32g4_ext.hpp"
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
  STM32GPIO BMI088_INT_1(BMI088_INT_1_GPIO_Port, BMI088_INT_1_Pin,
                         EXTI15_10_IRQn);
  STM32GPIO IMU_CS(IMU_CS_GPIO_Port, IMU_CS_Pin);
  STM32GPIO IMU_INT1(IMU_INT1_GPIO_Port, IMU_INT1_Pin, EXTI0_IRQn);
  STM32GPIO BMI088_CS_1(BMI088_CS_1_GPIO_Port, BMI088_CS_1_Pin);
  STM32GPIO BMI088_INT_2(BMI088_INT_2_GPIO_Port, BMI088_INT_2_Pin,
                         EXTI9_5_IRQn);
  STM32GPIO LED(LED_GPIO_Port, LED_Pin);

  STM32PWM pwm_tim1_ch1(&htim1, TIM_CHANNEL_1);

  STM32PWM pwm_tim2_ch2(&htim2, TIM_CHANNEL_2);

  STM32SPI spi1(&hspi1, spi1_rx_buf, spi1_tx_buf, 3);

  STM32SPI spi3(&hspi3, spi3_rx_buf, spi3_tx_buf, 3);

  STM32UART usart1(&huart1, usart1_rx_buf, usart1_tx_buf, 5, 5);

  STM32UART usart2(&huart2, usart2_rx_buf, usart2_tx_buf, 5, 5);

  STM32CANFD fdcan1(&hfdcan1, "fdcan1", 5);

  STDIO::read_ = &usart2.read_port_;
  STDIO::write_ = &usart2.write_port_;
  RamFS ramfs("XRobot");
  Terminal<32, 32, 5, 5> terminal(ramfs);
  auto terminal_task = Timer::CreateTask(terminal.TaskFun, &terminal, 10);
  Timer::Add(terminal_task);
  Timer::Start(terminal_task);

  LibXR::HardwareContainer<
      LibXR::Entry<LibXR::PowerManager>, LibXR::Entry<LibXR::GPIO>,
      LibXR::Entry<LibXR::GPIO>, LibXR::Entry<LibXR::GPIO>,
      LibXR::Entry<LibXR::GPIO>, LibXR::Entry<LibXR::GPIO>,
      LibXR::Entry<LibXR::GPIO>, LibXR::Entry<LibXR::GPIO>,
      LibXR::Entry<LibXR::PWM>, LibXR::Entry<LibXR::PWM>,
      LibXR::Entry<LibXR::SPI>, LibXR::Entry<LibXR::SPI>,
      LibXR::Entry<LibXR::UART>, LibXR::Entry<LibXR::UART>,
      LibXR::Entry<LibXR::FDCAN>, LibXR::Entry<LibXR::RamFS>,
      LibXR::Entry<LibXR::Terminal<32, 32, 5, 5>>>
      peripherals{{power_manager, {"power_manager"}},
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
                  {terminal, {"terminal"}}};

  /* User Code Begin 3 */
  auto power_cmd_file = LibXR::RamFS::CreateFile<STM32PowerManager *>(
      "power", STM32G4PowerFun, &power_manager);

  ramfs.Add(power_cmd_file);

  STM32G4Flash flash(0x1F000,  // start_offset，对应 0x0801F000
                     2048,     // min_erase_size，一页大小
                     8         // min_write_size，双字（64位）
  );

  LibXR::DatabaseRawSequential database(flash, 256);

  peripherals.Register(LibXR::Entry<LibXR::Database>{database, {"database"}});

  XRobotMain(peripherals);
  /* User Code End 3 */
}
