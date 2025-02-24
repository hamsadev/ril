#ifdef __EXAMPLE_TELEPHONY__

#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include <string.h>
#include "ril.h"
#include "dbg.h"
#include "ril_telephony.h"

void SystemClock_Config(void);
void URCIndicationCallback(RIL_URCInfo* info);
void DBGLineReceivedCallback(char* line, uint16_t len);

int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();

  /* DEBUG Initialize*/
  DBG_Init(&huart2, DBGLineReceivedCallback);
  DBG("<-- APP STARTED CallExample -->\r\n");

  /* RIL Initialize*/
  RIL_initialize(&huart1, URCIndicationCallback);
  DBG("<-- RIL is ready -->\r\n");

  while (1)
  {
    DBG_Routine();
    RIL_ServiceRoutine();
    HAL_Delay(120);

  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
  RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
    | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

void URCIndicationCallback(RIL_URCInfo* info)
{
  switch (info->urcType)
  {
  case URC_CALLER_ID:
  {
    char* phoneNumber = info->params[CLIP_PARAM_NUMBER].strValue;
    int type = info->params[CLIP_PARAM_TYPE].intValue;
    Enum_CallState callState;
    RIL_ATSndError ret;

    DBG("<-- Coming call, number:%s, type:%d -->\r\n", phoneNumber, type);
    ret = RIL_Telephony_Hangup();
    if (ret == RIL_AT_SUCCESS)
    {
      DBG("<-- Hang up the coming call -->\r\n");
    }
    else
    {
      DBG("<-- Fail to hang up the coming call -->\r\n");
    }

    ret = RIL_Telephony_Dial(0, phoneNumber, &callState);
    if (ret == RIL_AT_SUCCESS && callState == CALL_STATE_OK)
    {
      DBG("<-- Calling back... -->\r\n");
    }
    else if (RIL_AT_SUCCESS == ret)
    {
      DBG("<-- Fail to call back, callState=%d-->\r\n", callState);
    }
    else {
      DBG("<--RIL_Telephony_Dial() failed , ret =%d  -->", ret);
    }
    break;
  }
  case URC_CALL_READY:
    DBG("\r\n<-- Call module is ready -->\r\n");
    break;
  case URC_SMS_READY:
    DBG("\r\n<-- SMS module is ready -->\r\n");
    break;
  case URC_SIM_STATUS:
    break;
  default:
    DBG("<-- Unknown URC code: %d:", info->urcType);
    for (size_t i = 0; i < info->paramCount; i++)
    {

      URC_Param* param = &info->params[i];
      switch (param->type)
      {
      case URC_PARAM_INT:
        DBG("%d,", param->intValue);
        break;
      case URC_PARAM_STRING:
        DBG("%s,", param->strValue);
        break;
      }
    }
    DBG("-->\r\n");
    break;
  }
}

void DBGLineReceivedCallback(char* line, uint16_t len)
{
  RIL_ATSndError ret = RIL_SendATCmd(line, len, NULL, NULL, 0);
  DBG("<-- DBGLineReceivedCallback: Sending AT CMD (%.*s,%d) -->\r\n", (int)strcspn(line, "\r\n"), line, ret);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#endif //__CUSTOMER_CODE__
