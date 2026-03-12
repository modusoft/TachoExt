/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define DIN_LOW_BEAM_Pin GPIO_PIN_0
#define DIN_LOW_BEAM_GPIO_Port GPIOA
#define DIN_HI_BEAM_Pin GPIO_PIN_1
#define DIN_HI_BEAM_GPIO_Port GPIOA
#define DIN_IND_LEFT_Pin GPIO_PIN_2
#define DIN_IND_LEFT_GPIO_Port GPIOA
#define DIN_IND_RIGHT_Pin GPIO_PIN_3
#define DIN_IND_RIGHT_GPIO_Port GPIOA
#define DIN_WARN_ENG_Pin GPIO_PIN_4
#define DIN_WARN_ENG_GPIO_Port GPIOA
#define DIN_WARN_OIL_Pin GPIO_PIN_5
#define DIN_WARN_OIL_GPIO_Port GPIOA
#define DIN_NEUTRAL_Pin GPIO_PIN_6
#define DIN_NEUTRAL_GPIO_Port GPIOA
#define DIN_BTN_SCREEN_Pin GPIO_PIN_7
#define DIN_BTN_SCREEN_GPIO_Port GPIOA
#define TACHO_IN_Pin GPIO_PIN_15
#define TACHO_IN_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
