/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan;

/* USER CODE BEGIN PV */
CAN_RxHeaderTypeDef RxHeader;
uint8_t RxData[8];
volatile uint8_t can_open = 0;
uint8_t slcan_rx_buffer[32];
uint8_t slcan_rx_len = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void CAN_to_SLCAN_and_send(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
  char slcan_msg[32];
  int len = 0;

  // Standard frame
  if (header->IDE == CAN_ID_STD)
  {
    len = sprintf(slcan_msg, "t%03X%u", (unsigned int)header->StdId, (unsigned int)header->DLC);
  }
  else
  { // Extended frame
    len = sprintf(slcan_msg, "T%08X%u", (unsigned int)header->ExtId, (unsigned int)header->DLC);
  }

  for (uint8_t i = 0; i < header->DLC; i++)
  {
    len += sprintf(slcan_msg + len, "%02X", data[i]);
  }
  slcan_msg[len++] = '\r'; // SLCAN frame end

  CDC_Transmit_FS((uint8_t *)slcan_msg, len);
}

void slcan_set_channel(uint8_t channel) {
  if (channel == 0) {
    HAL_GPIO_WritePin(CAN_SELECT_GPIO_Port, CAN_SELECT_Pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(CAN_SELECT_GPIO_Port, CAN_SELECT_Pin, GPIO_PIN_RESET);
  }
}

void slcan_send_ok(void) {
  CDC_Transmit_FS((uint8_t*)"\r", 1);
}

void slcan_send_error(void) {
  CDC_Transmit_FS((uint8_t*)"\a", 1);
}

void slcan_send_version(void){
  CDC_Transmit_FS((uint8_t*)"V0101\r", 6);
}

void slcan_send_serial_number(void){
  CDC_Transmit_FS((uint8_t*)"N6969\r", 6);
}

void slcan_return_status_flags(void){
  uint8_t status = 0;

  // Bit 0: CAN RX FIFO is full
  // Bit 1: CAN TX FIFO is full
  // Bit 2: 0
  // Bit 3: 0
  // Bit 4: 0
  // Bit 5: 0
  // Bit 6: CAN arbitration lost
  // Bit 7: CAN bus error

  uint8_t bit0 = HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) >= 3 ? 1 : 0; // Check if FIFO is full (size is 3)
  uint8_t bit1 = HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0 ? 1 : 0;
  uint8_t bit2 = 0;
  uint8_t bit3 = 0;
  uint8_t bit4 = 0;
  uint8_t bit5 = 0;
  uint8_t bit6 = 0;
  uint8_t bit7 = (hcan.Instance->ESR & CAN_ESR_EWGF) ? 1 : 0;

  status |= (bit0 << 0);
  status |= (bit1 << 1);
  status |= (bit2 << 2);
  status |= (bit3 << 3);
  status |= (bit4 << 4);
  status |= (bit5 << 5);
  status |= (bit6 << 6);
  status |= (bit7 << 7);

  CDC_Transmit_FS(&status, 1);
}

void slcan_set_bitrate(uint8_t bitrate) {
  // Set the CAN bitrate based on the provided value
  uint8_t ok = 1;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = DISABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = DISABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  switch (bitrate)
  {
    case 0:
    { // 100k
      hcan.Init.TimeSeg1 = CAN_BS1_14TQ;
      hcan.Init.TimeSeg2 = CAN_BS2_3TQ;
      hcan.Init.Prescaler = 200;
      break;
    }
    case 1:
    { // 20k
      hcan.Init.TimeSeg1 = CAN_BS1_14TQ;
      hcan.Init.TimeSeg2 = CAN_BS2_3TQ;
      hcan.Init.Prescaler = 100;
      break;
    }
    case 2:
    { // 50k
      hcan.Init.TimeSeg1 = CAN_BS1_14TQ;
      hcan.Init.TimeSeg2 = CAN_BS2_3TQ;
      hcan.Init.Prescaler = 40;
      break;
    }
    case 3:
    { // 100k
      hcan.Init.TimeSeg1 = CAN_BS1_14TQ;
      hcan.Init.TimeSeg2 = CAN_BS2_3TQ;
      hcan.Init.Prescaler = 20;
      break;
    }
    case 4:
    { // 125k
      hcan.Init.TimeSeg1 = CAN_BS1_12TQ;
      hcan.Init.TimeSeg2 = CAN_BS2_5TQ;
      hcan.Init.Prescaler = 16;
      break;
    }
    case 5:
    { // 250k
      hcan.Init.TimeSeg1 = CAN_BS1_12TQ;
      hcan.Init.TimeSeg2 = CAN_BS2_5TQ;
      hcan.Init.Prescaler = 8;
      break;
    }
    case 6:
    { // 500k
      hcan.Init.TimeSeg1 = CAN_BS1_12TQ;
      hcan.Init.TimeSeg2 = CAN_BS2_5TQ;
      hcan.Init.Prescaler = 4;
      break;
    }
    case 7:
    { // 750k
      hcan.Init.TimeSeg1 = CAN_BS1_16TQ;
      hcan.Init.TimeSeg2 = CAN_BS2_7TQ;
      hcan.Init.Prescaler = 2;
      break;
    }
    case 8:
    { // 1M
      hcan.Init.TimeSeg1 = CAN_BS1_12TQ;
      hcan.Init.TimeSeg2 = CAN_BS2_5TQ;
      hcan.Init.Prescaler = 2;
      break;
    }
    default:
      ok = 0;
      break;
  }
  if (ok && HAL_CAN_Init(&hcan) == HAL_OK)
  {
    slcan_send_ok();
  }
  else
  {
    slcan_send_error();
  }
}

void slcan_send_std_can_msg(uint32_t id, uint8_t dlc, uint8_t *data)
{
  // Send standard can message to CAN
  CAN_TxHeaderTypeDef txHeader;
  uint8_t txData[8] = {0};
  uint32_t txMailbox;

  txHeader.StdId = id;
  txHeader.IDE = CAN_ID_STD;
  txHeader.RTR = CAN_RTR_DATA;
  txHeader.DLC = dlc;

  memcpy(txData, data, dlc);

  if (HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &txMailbox) == HAL_OK)
  {
    slcan_send_ok();
  }
  else
  {
    CDC_Transmit_FS((uint8_t *)"z\r", 2);
  }
}

void slcan_send_ext_can_msg(uint32_t id, uint8_t dlc, uint8_t *data)
{
  // Send extended CAN message to CAN
  CAN_TxHeaderTypeDef txHeader;
  uint8_t txData[8] = {0};
  uint32_t txMailbox;

  txHeader.ExtId = id;
  txHeader.IDE = CAN_ID_EXT;
  txHeader.RTR = CAN_RTR_DATA;
  txHeader.DLC = dlc;

  memcpy(txData, data, dlc);

  if (HAL_CAN_AddTxMessage(&hcan, &txHeader, txData, &txMailbox) == HAL_OK)
  {
    CDC_Transmit_FS((uint8_t *)"Z\r", 2);
  }
  else
  {
    slcan_send_error();
  }
}

void slcan_send_std_rtr_msg(uint32_t id, uint8_t dlc)
{
  // Send standard RTR message to CAN
  CAN_TxHeaderTypeDef txHeader;
  uint32_t txMailbox;

  txHeader.StdId = id;
  txHeader.IDE = CAN_ID_STD;
  txHeader.RTR = CAN_RTR_REMOTE;
  txHeader.DLC = dlc;

  if (HAL_CAN_AddTxMessage(&hcan, &txHeader, NULL, &txMailbox) == HAL_OK)
  {
    CDC_Transmit_FS((uint8_t *)"z\r", 2);
  }
  else
  {
    slcan_send_error();
  }
}

void slcan_send_ext_rtr_msg(uint32_t id, uint8_t dlc)
{
  // Send extended RTR message to CAN
  CAN_TxHeaderTypeDef txHeader;
  uint32_t txMailbox;

  txHeader.ExtId = id;
  txHeader.IDE = CAN_ID_EXT;
  txHeader.RTR = CAN_RTR_REMOTE;
  txHeader.DLC = dlc;

  if (HAL_CAN_AddTxMessage(&hcan, &txHeader, NULL, &txMailbox) == HAL_OK)
  {
    CDC_Transmit_FS((uint8_t *)"Z\r", 2);
  }
  else
  {
    slcan_send_error();
  }
}

void slcan_process_command(uint8_t *cmd, uint8_t len) {
  if (len < 1) return;
  switch (cmd[0]) {
    case 'S': // Set bitrate
      if (can_open || len < 2)
      {
        slcan_send_error();
        break;
      }
      slcan_set_bitrate(cmd[1] - '0');
      break;
    case 'c': // Set CAN channel
      if (len < 2) {
        slcan_send_error();
        break;
      }
      if(can_open){
        if (HAL_CAN_Stop(&hcan) == HAL_OK)
        {
          can_open = 0;
        }
      }
      slcan_set_channel(cmd[1] - '0');
      slcan_send_ok();
      break;
    case 'N': // Get serial number
      slcan_send_serial_number();
      break;
    case 'O': // Open CAN channel
      if (!can_open)
      {
        if (HAL_CAN_Start(&hcan) == HAL_OK)
        {
          HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
          can_open = 1;
          slcan_send_ok();
        }
        else
        {
          slcan_send_error();
        }
      }
      else
      {
        slcan_send_error();
      }
      break;
    case 'C': // Close CAN channel
      if (can_open)
      {
        if (HAL_CAN_Stop(&hcan) == HAL_OK)
        {
          can_open = 0;
          slcan_send_ok();
        }
        else
        {
          slcan_send_error();
        }
      }
      else
      {
        slcan_send_error();
      }
      break;
    case 't': // Send standard CAN frame
      if (!can_open)
      {
        slcan_send_error();
        break;
      }
      {
        // SLCAN: tiiiLddd... (iii=ID, L=DLC, d=DATA)
        uint8_t txData[8] = {0};
        if (len < 5)
        {
          slcan_send_error();
          break;
        }
        // Parse 11-bit ID
        char idStr[4] = {0};
        memcpy(idStr, &cmd[1], 3);
        uint32_t ID = (uint32_t)strtol(idStr, NULL, 16);
        uint32_t DLC = cmd[4] - '0';
        // Data bytes
        for (uint8_t i = 0; i < DLC && (5 + 2 * i + 1) <= len; i++)
        {
          char byteStr[3] = {cmd[5 + 2 * i], cmd[5 + 2 * i + 1], 0};
          txData[i] = (uint8_t)strtol(byteStr, NULL, 16);
        }
        slcan_send_std_can_msg(ID, DLC, txData);
      }
      break;
    case 'r': // Send standard RTR
      if (!can_open)
      {
        slcan_send_error();
        break;
      }
      {
        // SLCAN: riiiLddd... (iii=ID, L=DLC, d=DATA)
        if (len < 5)
        {
          slcan_send_error();
          break;
        }
        // Parse 11-bit ID
        char idStr[4] = {0};
        memcpy(idStr, &cmd[1], 3);
        uint32_t ID = (uint32_t)strtol(idStr, NULL, 16);
        uint32_t DLC = cmd[4] - '0';
        slcan_send_std_rtr_msg(ID, DLC);
      }
      break;
    case 'T': // Send extended CAN frame
      if (!can_open)
      {
        slcan_send_error();
        break;
      }
      {
        // SLCAN: TiiiiiiiNd... (iiiiiiii=ID, N=DLC, d=DATA)
        char idStr[9] = {0};
        memcpy(idStr, &cmd[1], 8);
        uint32_t ID = (uint32_t)strtol(idStr, NULL, 16);
        uint32_t DLC = cmd[9] - '0';
        uint8_t txData[8] = {0};
        for (uint8_t i = 0; i < DLC && (10 + 2 * i + 1) <= len; i++)
        {
          char byteStr[3] = {cmd[10 + 2 * i], cmd[10 + 2 * i + 1], 0};
          txData[i] = (uint8_t)strtol(byteStr, NULL, 16);
        }
        slcan_send_ext_can_msg(ID, DLC, txData);
      }
      break;
    case 'R': // Send extended RTR
      if (!can_open)
      {
        slcan_send_error();
        break;
      }
      {
        // SLCAN: TiiiiiiiNd... (iiiiiiii=ID, N=DLC, d=DATA)
        
        if (len < 10)
        {
          slcan_send_error();
          break;
        }
        // Parse 29-bit ID
        char idStr[9] = {0};
        memcpy(idStr, &cmd[1], 8);
        uint32_t ID = (uint32_t)strtol(idStr, NULL, 16);
        uint32_t DLC = cmd[9] - '0';
        slcan_send_ext_rtr_msg(ID, DLC);
      }
      break;
    case 'V': // Get version
      CDC_Transmit_FS((uint8_t*)"V0101\r", 6);
      break;
    case 'F':
      slcan_return_status_flags();
      break;
    default : slcan_send_error();
      break;
  }
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
  MX_CAN_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL3;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN;
  hcan.Init.Prescaler = 2;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_12TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_5TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = DISABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = DISABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */
  CAN_FilterTypeDef sFilterConfig;
  sFilterConfig.FilterBank = 0;
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  sFilterConfig.FilterIdHigh = 0x0000;
  sFilterConfig.FilterIdLow = 0x0000;
  sFilterConfig.FilterMaskIdHigh = 0x0000;
  sFilterConfig.FilterMaskIdLow = 0x0000;
  sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  sFilterConfig.FilterActivation = ENABLE;
  sFilterConfig.SlaveStartFilterBank = 14;
  HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);
  /* USER CODE END CAN_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CAN_SELECT_GPIO_Port, CAN_SELECT_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : CAN_SELECT_Pin */
  GPIO_InitStruct.Pin = CAN_SELECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CAN_SELECT_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
  {
    CAN_to_SLCAN_and_send(&RxHeader, RxData);
  }
}

// Funkcija za sprejem podatkov iz USB CDC (klice se iz usbd_cdc_if.c)
void slcan_usb_rx(uint8_t* Buf, uint32_t Len) {
  for (uint32_t i = 0; i < Len; i++) {
    if (Buf[i] == '\r') {
      slcan_process_command(slcan_rx_buffer, slcan_rx_len);
      slcan_rx_len = 0;
    } else if (slcan_rx_len < sizeof(slcan_rx_buffer)-1) {
      slcan_rx_buffer[slcan_rx_len++] = Buf[i];
    }
  }
}
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
