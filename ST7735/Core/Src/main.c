/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  *@author	:	Mateusz Berliński
  *
  *@title	:	Zegar
  *
  *@description:
  *		Tematem projektu jest zegar. W normalnej pracy będzie wyświetlał aktualną godzinę i datę.
  *		Będzie można go przełączyć na wyświetlanie temperatury. Sterowanie odbywa się przez pilot podczerwieni.
  *		Dane pokazywane są na wyswietlaczu tft przy pomocy SPI. Na zegarze można ustawiać date, godzinę i alarmy.
  *		Alarmy włączają brzęczyk, a na ekranie pojawia się komunikat.
  *
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "st7735.h"
#include "fonts.h"
#include <stdio.h>
#include "ir.h"
#include "ds18b20.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
HAL_StatusTypeDef HAL_RTC_GetTime(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *sTime, uint32_t Format);
HAL_StatusTypeDef HAL_RTC_GetDate(RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate, uint32_t Format);
HAL_StatusTypeDef HAL_RTC_SetTime(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *sTime, uint32_t Format);
HAL_StatusTypeDef HAL_RTC_GetAlarm(RTC_HandleTypeDef *hrtc, RTC_AlarmTypeDef *sAlarm, uint32_t Alarm, uint32_t Format);

typedef enum {
	PULSE_9MS,
	PULSE_4MS,
	PULSE_2MS,
	PULSE_LONG,
	PULSE_SHORT,
	PULSE_ERROR,
}pulse_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
RTC_TimeTypeDef time;
RTC_DateTypeDef date;
RTC_AlarmTypeDef alarm;
char strTime[20];
char strDate[20];
char strTemp[20];
char strColored[10];
uint8_t Select_write = 0;
bool Enable_change = false;
bool Enable_alarm = false;
bool Alarm_on = false;
RTC_TimeTypeDef new_time = {0};
RTC_DateTypeDef new_date = {0};
RTC_AlarmTypeDef new_alarm = {0};
uint8_t Select_time_date = 0;
uint8_t Select_alarm_time = 0;
uint32_t last_ms = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Active_alarm(uint8_t remote)
{
	uint32_t now = HAL_GetTick();
	if(now - last_ms >= 100) {
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		last_ms = now;
	}

	if(remote == IR_BACK){
		Alarm_on = false;
		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, RESET);
	}

}

void Turn_on_alarm(void)
{
	Alarm_on = true;
}

void Save_last_time(void)
{
	new_time.Hours = time.Hours;
	new_time.Minutes = time.Minutes;
	new_time.Seconds = time.Seconds;
	new_date.Date = date.Date;
	new_date.Month = date.Month;
	new_date.Year = date.Year;
}

void Save_last_alarm(void)
{
	HAL_RTC_GetAlarm(&hrtc, &alarm, RTC_ALARM_A, RTC_FORMAT_BIN);
	new_alarm.AlarmTime.Hours = alarm.AlarmTime.Hours;
	new_alarm.AlarmTime.Minutes = alarm.AlarmTime.Minutes;
}

void Color_Selected(void)
{
	if(Select_time_date == 0){
		sprintf(strColored,"%02d",new_time.Hours);
		ST7735_WriteString(15, 35, strColored, Font_16x26, ST7735_BLACK,ST7735_BLACK);
		ST7735_WriteString(15, 35, strColored, Font_16x26, ST7735_WHITE,ST7735_BLACK);
	}
	else if(Select_time_date == 1){
		sprintf(strColored,"%02d",new_time.Minutes);
		ST7735_WriteString(63, 35, strColored, Font_16x26, ST7735_BLACK,ST7735_BLACK);
		ST7735_WriteString(63, 35, strColored, Font_16x26, ST7735_WHITE,ST7735_BLACK);
	}
	else if(Select_time_date == 2){
		sprintf(strColored,"%02d",new_time.Seconds);
		ST7735_WriteString(111, 35, strColored, Font_16x26, ST7735_BLACK,ST7735_BLACK);
		ST7735_WriteString(111, 35, strColored, Font_16x26, ST7735_WHITE,ST7735_BLACK);
	}
	else if(Select_time_date == 3){
		sprintf(strColored,"%02d",new_date.Date);
		ST7735_WriteString(25, 67, strColored, Font_11x18, ST7735_BLACK,ST7735_BLACK);
		ST7735_WriteString(25, 67, strColored, Font_11x18, ST7735_WHITE,ST7735_BLACK);
	}
	else if(Select_time_date == 4){
		sprintf(strColored,"%02d",new_date.Month);
		ST7735_WriteString(58, 67, strColored, Font_11x18, ST7735_BLACK,ST7735_BLACK);
		ST7735_WriteString(58, 67, strColored, Font_11x18, ST7735_WHITE,ST7735_BLACK);
	}
	else if(Select_time_date == 5){
		sprintf(strColored,"20%02d",new_date.Year);
		ST7735_WriteString(91, 67, strColored, Font_11x18, ST7735_BLACK,ST7735_BLACK);
		ST7735_WriteString(91, 67, strColored, Font_11x18, ST7735_WHITE,ST7735_BLACK);
	}
}

void Plus_Time(void)
{
	if(Select_time_date == 0)
		new_time.Hours = (new_time.Hours +1) % 24;
	else if(Select_time_date == 1)
		new_time.Minutes = (new_time.Minutes +1) % 60;
	else if(Select_time_date == 2)
		new_time.Seconds = (new_time.Seconds +1) % 60;
	else if(Select_time_date == 3)
		new_date.Date = new_date.Date % 30 + 1;
	else if(Select_time_date == 4)
		new_date.Month = new_date.Month % 11 + 1;
	else if(Select_time_date == 5)
		new_date.Year = (new_date.Year +1) % 100;
}

void Minus_Time(void)
{
	if(Select_time_date == 0){
		if(new_time.Hours == 0) new_time.Hours = 24;
		new_time.Hours = (new_time.Hours -1);
	}
	else if(Select_time_date == 1){
		if(new_time.Minutes == 0) new_time.Minutes = 60;
		new_time.Minutes = (new_time.Minutes -1);
	}
	else if(Select_time_date == 2){
		if(new_time.Seconds == 0) new_time.Seconds = 60;
		new_time.Seconds = (new_time.Seconds -1);
	}
	else if(Select_time_date == 3){
		new_date.Date = (new_date.Date -1);
		if(new_date.Date == 0) new_date.Date = 31;
	}
	else if(Select_time_date == 4){
		new_date.Month = (new_date.Month -1);
		if(new_date.Month == 0) new_date.Month = 12;
	}
	else if(Select_time_date == 5){
		if(new_date.Year == 0) new_date.Year = 100;
		new_date.Year = (new_date.Year -1);
	}
}

void Change_Time(uint8_t remote)
{
	if(remote == IR_BACK)
		Enable_change = false;

	if(remote == IR_PLUS){
		Plus_Time();
	}

	if(remote == IR_MINUS){
		Minus_Time();
	}

	if(remote == IR_FORWARD)
		Select_time_date = (Select_time_date +1) % 6;

	if(remote == IR_REWIND){
		if(Select_time_date == 0) Select_time_date = 6;
		Select_time_date = (Select_time_date -1);
	}

	Color_Selected();

	if(remote == IR_PLAY){
		HAL_RTC_SetTime(&hrtc, &new_time, RTC_FORMAT_BIN);
		HAL_RTC_SetDate(&hrtc, &new_date, RTC_FORMAT_BIN);
		Select_time_date = 0;
		Enable_change = false;
	}
}

void Set_Alarm(uint8_t remote)
{
	if(remote == IR_BACK)
		Enable_alarm = false;

	if(remote == IR_PLUS){
		if(Select_alarm_time == 0)
			new_alarm.AlarmTime.Hours = (new_alarm.AlarmTime.Hours +1) % 24;
		else if(Select_alarm_time == 1)
			new_alarm.AlarmTime.Minutes = (new_alarm.AlarmTime.Minutes +1) % 60;
	}

	if(remote == IR_MINUS){
		if(Select_alarm_time == 0){
			if(new_alarm.AlarmTime.Hours == 0) new_alarm.AlarmTime.Hours = 24;
			new_alarm.AlarmTime.Hours = (new_alarm.AlarmTime.Hours -1);
		}
		else if(Select_alarm_time == 1){
			if(new_alarm.AlarmTime.Minutes == 0) new_alarm.AlarmTime.Minutes = 60;
			new_alarm.AlarmTime.Minutes = (new_alarm.AlarmTime.Minutes -1);
		}
	}

	if(remote == IR_FORWARD)
		Select_alarm_time = (Select_alarm_time +1) % 2;

	if(remote == IR_REWIND){
		if(Select_alarm_time == 0) Select_alarm_time = 2;
		Select_alarm_time = (Select_alarm_time -1);
	}

	if(remote == IR_CANCEL){
		HAL_RTC_SetAlarm(&hrtc, &new_alarm, RTC_FORMAT_BIN); //to nie działa - ta funkcja nie zapisuje nowego alarmu
		Select_alarm_time = 0;
		Enable_alarm = false;
	}

	sprintf(strTime,"%02d:%02d:%02d", new_alarm.AlarmTime.Hours, new_alarm.AlarmTime.Minutes, new_alarm.AlarmTime.Seconds);
	ST7735_WriteString(15, 35, strTime, Font_16x26, ST7735_WHITE,ST7735_BLACK);

	if(Select_alarm_time == 0){
		sprintf(strColored,"%02d",new_alarm.AlarmTime.Hours);
		ST7735_WriteString(15, 35, strColored, Font_16x26, ST7735_BLACK,ST7735_BLACK);
	}
	else if(Select_alarm_time == 1){
		sprintf(strColored,"%02d",new_alarm.AlarmTime.Minutes);
		ST7735_WriteString(63, 35, strColored, Font_16x26, ST7735_BLACK,ST7735_BLACK);
	}
}

void Hello(void)
{
	 ST7735_FillScreen(ST7735_BLACK);
	 ST7735_WriteString(35, 51, "Hello", Font_16x26, ST7735_WHITE,ST7735_BLACK);
	 HAL_Delay(500);
	 ST7735_FillScreen(ST7735_BLACK);
}

void Write_TimeDate(void)
{
	HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

	sprintf(strTime,"%02d:%02d:%02d", time.Hours, time.Minutes, time.Seconds);//to one string
	sprintf(strDate,"%02d %02d 20%02d", date.Date, date.Month, date.Year);

	ST7735_WriteString(15, 35, strTime, Font_16x26, ST7735_WHITE,ST7735_BLACK);
	ST7735_WriteString(25, 67, strDate, Font_11x18, ST7735_WHITE,ST7735_BLACK);
}

void Write_Temp(void)
{
	  ds18b20_start_measure(NULL);

	  float temp = ds18b20_get_temp(NULL);
	  if (temp >= 80.0f)
	    sprintf(strTemp, "wait...");
	  else
	    sprintf(strTemp, " %.1f`C", temp);

	  ST7735_WriteString(20, 51, strTemp, Font_16x26, ST7735_WHITE,ST7735_BLACK);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &htim3)
  {
    switch (HAL_TIM_GetActiveChannel(&htim3))
    {
      case HAL_TIM_ACTIVE_CHANNEL_1:
        ir_tim_interrupt();
        break;
      default:
        break;
    }
  }
}

void Write_selected(uint8_t select)
{
	if(select == 0)
	{
		Write_TimeDate();
	}
	else if(select == 1)
	{
		Write_Temp();
	}
}


void Write_screen(uint8_t remote)
{
	if(remote == IR_ONOFF){
		HAL_GPIO_TogglePin(BL_GPIO_Port, BL_Pin);
	}
	else if(remote == IR_MENU){
		Select_write = (Select_write + 1) % 2;
		ST7735_FillScreen(ST7735_BLACK);
	}
	else if((remote == IR_PLAY) & (Select_write == 0)){
		Enable_change = true;
		Save_last_time();
	}
	else if((remote == IR_CANCEL) & (Select_write == 0)){
		Enable_alarm = true;
		Save_last_alarm();
		ST7735_FillScreen(ST7735_BLACK);
	}

	if(!Enable_change & !Enable_alarm)
		Write_selected(Select_write);
}




/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_RTC_Init();
  MX_TIM3_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  ir_init();
  ST7735_Init();
  Hello();

  HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2048, RTC_WAKEUPCLOCK_RTCCLK_DIV16);//przerwanie RTC co 1s


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(Alarm_on)
		  Active_alarm(ir_read());
	  else if(Enable_change)
		  Change_Time(ir_read());
	  else if(Enable_alarm)
		  Set_Alarm(ir_read());
	  else
		  __WFI();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
