/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stdlib.h"
#include "stdio.h"
#include "String.h"

#define write(x, y) HAL_GPIO_WritePin(x##_GPIO_Port, x##_Pin, y)
#define read(x) HAL_GPIO_ReadPin(x##_GPIO_Port, x##_Pin)

#define NULL 0

//TELAS

uint8_t set, task, dbc;

uint8_t buff[32];

//geral e display

void delay_us (uint16_t us) {
	TIM2->CNT = 0;  // set the counter value a 0
	while (TIM2->CNT < us);  // wait for the counter to reach the us input in the parameter
}

void lcd_write(uint8_t data, uint8_t cmd) {
	
	write(rs, cmd);
	
	write(d7, data & 0x80);
	write(d6, data & 0x40);
	write(d5, data & 0x20);
	write(d4, data & 0x10);
	
	write(e, 1);
	delay_us(250);
	write(e, 0);
	
	write(d7, data & 0x8);
	write(d6, data & 0x4);
	write(d5, data & 0x2);
	write(d4, data & 0x1);
	
	write(e, 1);
	delay_us(250);
	write(e, 0);
}

void lcd_init() {
	
	lcd_write(0x33, 0);
	HAL_Delay(20);
	lcd_write(0x32, 0);
	HAL_Delay(20);
	lcd_write(0x28, 0);
	HAL_Delay(20);
	lcd_write(0x06, 0);
	HAL_Delay(20);
	lcd_write(0x0F, 0);
	HAL_Delay(20);
	lcd_write(0x01, 0);
	HAL_Delay(20);
	
}

void lcd_writestr(uint8_t *str) {
	
	while(*str) lcd_write(*str++, 1);
	
}

// codigos
// 1 peao em desacordo com tabuleiro na funcao mover


void err(uint8_t cod) {
	
	uint8_t buff[32];
	
	sprintf(buff, "CODIGO DE ERRO %d", cod);
	
	lcd_writestr(buff);	
	
}

struct {
	uint16_t ch1, ch2;
} mcp;

void readmcp() {
	
	uint8_t mosibuff[3], misobuff[3];
	
	memset(misobuff, 0, 3);
	
	mosibuff[0] = 0x01;
	mosibuff[1] = 0xA0;
	write(nss, 0);
	HAL_SPI_TransmitReceive(&hspi2, mosibuff, misobuff, 3, 1000);
	write(nss, 1);
	mcp.ch1 = ((misobuff[1] & 0x0F) << 8) | misobuff[2];
	
	mosibuff[0] = 0x01;
	mosibuff[1] = 0xE0;
	
	memset(misobuff, 0, 3);
	
	write(nss, 0);
	HAL_SPI_TransmitReceive(&hspi2, mosibuff, misobuff, 3, 1000);
	write(nss, 1);
	mcp.ch2 = ((misobuff[1] & 0x0F) << 8) | misobuff[2];
	
}

//matriz

uint8_t red;
uint8_t green;

uint8_t spi[6];

#define col spi[3]
#define vm spi[5]
#define vd spi[4]

//tabuleiro

typedef struct {
	
	uint8_t x, y, type, id, team, on, first;
	
} pc;

pc *team1[12], *team2[12];
pc *alive1[12], *alive2[12];

pc *tabuleiro[12][12];

pc nula;

uint16_t tim = 0;
pc *blink;

pc intent;

void start_game() {
	
	blink = NULL;
	intent = nula;
	
	for(uint8_t i = 0; i < 8; i++) {
		free(team1[i]);
		free(team2[i]);
	}
	
	
	memset(tabuleiro, 0, sizeof(&nula)*12*12);
	
	for(uint8_t i = 0; i < 8; i++) {
		
		team1[i] = tabuleiro[0][i] = (pc *)malloc(sizeof(pc));
		(*team1[i]).team = 2;
		(*team1[i]).id = i;
		(*team1[i]).x = i;
		(*team1[i]).y = 0;
		(*team1[i]).type = 0;
		(*team1[i]).on = 1;
		(*team1[i]).first = 1;
		
		team2[i] = tabuleiro[7][i] = (pc *)malloc(sizeof(pc));
		(*team2[i]).team = 1;
		(*team2[i]).id = i;
		(*team2[i]).x = i;
		(*team2[i]).y = 7;
		(*team2[i]).type = 0;
		(*team2[i]).on = 1;
		(*team2[i]).first = 1;
	}
	
}

void end_game() {
	
	for(uint8_t i = 0; i < 8; i++) {
		for(uint8_t j = 0; j < 8; j++) free(tabuleiro[i][j]);
	};
	
	set = 0;
	
}

void move(pc *peao, uint8_t x, uint8_t y) {
	
	if(tabuleiro[(*peao).y][(*peao).x] != peao) err(1);
	
	if(tabuleiro[y][x] != peao) (*tabuleiro[y][x]).on = 0;
	tabuleiro[(*peao).y][(*peao).x] = NULL;
	
	tabuleiro[y][x] = peao;
	
	(*peao).x = x;
	(*peao).y = y;	
	
}




//MENU

void set0() {
	
	memset(alive1, 0, sizeof(&nula)*12);
	memset(alive2, 0, sizeof(&nula)*12);
	
	task = 0;
	set = 0xFF;
	lcd_write(0x01, 0);
	
}

void task0() {
	
	lcd_write(0x80, 0);
	lcd_writestr("  XADREZ  RUPESTRE  ");
	
	lcd_write(0x94, 0);
	lcd_writestr("PRESSIONE O JOYSTICK");
	
	lcd_write(0xD4, 0);
	lcd_writestr("PARA COMECAR O JOGO!");
	
	if(!read(sw) && dbc) {
		set = 1;
	}
	if(read(sw)) dbc = 1;
	
}

//task1

uint8_t turno, sel, opt, step, mov;
uint8_t count1, count2, lim;
void set1() {
	
	turno = sel = opt = step = dbc = mov = count1 = count2 = lim = 0;
	
	task = 1;
	set = 0xFF;
	start_game();
	
}

int task1() {
	
	count1 = 0;
	count2 = 0;
	
	for(uint8_t i = 0; i < 8; i++) {
		
		if((*team1[i]).on)  {
			
			alive1[count1] = team1[i];
			count1++;
			
		}
		
		if((*team2[i]).on)  {
			
			alive2[count2] = team2[i];
			count2++;
			
		}
	}
	
	if(!count1) {
		lcd_write(0x80, 0);
		lcd_writestr("   FIM DE JOGO!    ");
		lcd_write(0xC0, 0);
		lcd_writestr("  VITORIA DO TIME  ");
		lcd_write(0x94, 0);
		lcd_writestr("      VERDE!       ");
		lcd_write(0xD4, 0);
		lcd_writestr("   PRIMA O BOTAO   ");
		HAL_Delay(1000);
		while(read(sw));
		set0();
		return 0;
	}
	
	if(!count2) {
		lcd_write(0x80, 0);
		lcd_writestr("   FIM DE JOGO!    ");
		lcd_write(0xC0, 0);
		lcd_writestr("  VITORIA DO TIME  ");
		lcd_write(0x94, 0);
		lcd_writestr("      VERM.!       ");
		lcd_write(0xD4, 0);
		lcd_writestr("   PRIMA O BOTAO   ");
		HAL_Delay(1000);
		while(read(sw));
		set0();
		return 0;
		
	}
	
	lcd_write(0x94, 0);
	lcd_writestr("TURNO DO TIME ");
	
	if(turno) {
		
		lim = count1-1;
		
		lcd_writestr("VERM. ");
		blink = alive1[sel];
		
	} else {
		
		lim = count2-1;
		
		lcd_writestr("VERDE ");
		blink = alive2[sel];
	}
	
	
	switch(step) {
		
		case 0:
		
				lcd_write(0xD4, 0);
				sprintf(buff, "Selec. peao no. %d   ", (*blink).id);
				lcd_writestr(buff);
		
			if(mcp.ch1 > 3900 && dbc) {

				if(sel < lim) sel++;
				else sel = 0;
				
			}

			if(mcp.ch1 < 100 && dbc) {

				if(sel > 0) sel--;
				else sel = lim;
				
			}	
			
			if(!read(sw) && dbc) {
				uint8_t can = 3;
				if((*blink).type == 0) {
					if(!turno) {
						if((*blink).x == 0 || (*tabuleiro[(*blink).y-1][(*blink).x-1]).team != 2) can--;
						if((*blink).y == 0 || tabuleiro[(*blink).y-1][(*blink).x]) can--;
						if((*blink).x == 7 || (*tabuleiro[(*blink).y-1][(*blink).x+1]).team != 2) can--;	
						
					}
					
					if(turno) {
						if((*blink).x == 0 || (*tabuleiro[(*blink).y+1][(*blink).x-1]).team != 1) can--;
						if((*blink).y == 7 || tabuleiro[(*blink).y+1][(*blink).x]) can--;
						if((*blink).x == 7 || (*tabuleiro[(*blink).y+1][(*blink).x+1]).team != 1) can--;
						
					}
						
					if(can) {
						step = 1;
						intent.x = (*blink).x;
						intent.y = (*blink).y;
						if(turno) {
							if(intent.y < 7 && tabuleiro[intent.y+1][intent.x] == NULL) intent.y++;
							else {
								
								if((*blink).x < 7 && (*blink).y < 7) {
								
									if((*tabuleiro[(*blink).y+1][(*blink).x+1]).team == 1) {
										
										intent.y = (*blink).y+1;
										intent.x = (*blink).x+1;
										
										
									}
									
								}
								
								if((*blink).x > 0 && (*blink).y < 7) {
								
									if((*tabuleiro[(*blink).y+1][(*blink).x-1]).team == 1) {
										
										intent.y = (*blink).y+1;
										intent.x = (*blink).x-1;
										
										
									}
									
								}
								
							}
						}
						if(!turno) {
							if(intent.y > 0 && tabuleiro[intent.y-1][intent.x] == NULL) intent.y--;
							else {
								
								if((*blink).x < 7 && (*blink).y > 0) {
								
									if((*tabuleiro[(*blink).y-1][(*blink).x+1]).team == 2) {
										
										intent.y = (*blink).y-1;
										intent.x = (*blink).x+1;
										
										
									}
									
								}
								
								if(!turno) {
							
								if((*blink).x > 0 && (*blink).y > 0) {
									
									if((*tabuleiro[(*blink).y-1][(*blink).x-1]).team == 2) {
										
										intent.y = (*blink).y-1;
										intent.x = (*blink).x-1;
										
										
									}
									
								}
								
							}
								
							}
						}
						intent.type = 1;
					
					}
			}
		}
			
		
			
			if(mcp.ch1 > 3900 || mcp.ch1 < 100 || !read(sw)) tim = dbc = 0;
			else dbc = 1;
		
		break;
		
		case 1:
			
				lcd_write(0xD4, 0);
				sprintf(buff, "Movendo peao no. %d  ", sel);
				lcd_writestr(buff);
				int8_t limx, limy;
				
				if((*blink).first) {
					limy = 2;
				} else limy = 1;
				
				
				if(mcp.ch2 < 100 && dbc) {
					if((!turno && tabuleiro[(*blink).y-1][(*blink).x] == NULL) || (turno && tabuleiro[(*blink).y+1][(*blink).x]) == NULL) {
					 if(tabuleiro[intent.y][intent.x] != blink)
						if(intent.y < 7 && tabuleiro[intent.y+1][intent.x] == NULL) {
							intent.x = (*blink).x;
						if(turno) if(intent.y >= 0 && intent.y < (*blink).y+limy) intent.y++;
						if(!turno) if(intent.y > 0 && intent.y <= (*blink).y+limy) intent.y++;
					}
				}
			}

				if(mcp.ch2 > 3900 && dbc) {
				 if((!turno && tabuleiro[(*blink).y-1][(*blink).x] == NULL) || (turno && tabuleiro[(*blink).y+1][(*blink).x]) == NULL) {
					if(tabuleiro[intent.y][intent.x] != blink)
						if(intent.y > 0 && tabuleiro[intent.y-1][intent.x] == NULL) {
							intent.x = (*blink).x;
						if(!turno) if(intent.y > (*blink).y-limy && intent.y <= 7) intent.y--;
						if(turno)	 if(intent.y >= (*blink).y-limy && intent.y < 7) intent.y--;
					}
				}
					
				}

				if(mcp.ch1 > 3900 && dbc) {

					if(turno) {
						
						if((*blink).x < 7 && (*blink).y < 7) {
							
							if((*tabuleiro[(*blink).y+1][(*blink).x+1]).team == 1) {
								
								intent.y = (*blink).y+1;
								intent.x = (*blink).x+1;
								
								
							}
							
						}
						
					}
					
					if(!turno) {
						
						if((*blink).x < 7 && (*blink).y > 0) {
							
							if((*tabuleiro[(*blink).y-1][(*blink).x+1]).team == 2) {
								
								intent.y = (*blink).y-1;
								intent.x = (*blink).x+1;
								
								
							}
							
						}
						
					}
					
				}
				
				if(mcp.ch1 < 100 && dbc) {

					if(turno) {
						
						if((*blink).x > 0 && (*blink).y < 7) {
							
							if((*tabuleiro[(*blink).y+1][(*blink).x-1]).team == 1) {
								
								intent.y = (*blink).y+1;
								intent.x = (*blink).x-1;
								
								
							}
							
						}
						
					}
					
					if(!turno) {
						
						if((*blink).x > 0 && (*blink).y > 0) {
							
							if((*tabuleiro[(*blink).y-1][(*blink).x-1]).team == 2) {
								
								intent.y = (*blink).y-1;
								intent.x = (*blink).x-1;
								
								
							}
							
						}
						
					}
					
				}	
				
				if(!read(sw) && dbc) {
					(*blink).first = 0;
					step = 0;
					turno = !turno;
					dbc = 0;
					sel = 0;
					intent.type = 0;
					move(blink, intent.x, intent.y);
					mov++;
				}
				
				if(mcp.ch1 > 3900 || mcp.ch1 < 100 || mcp.ch2 > 3900 || mcp.ch2 < 100 || !read(sw)) dbc = 0;
				else dbc = 1;
		
		break;
		
		case 2:
			
		
		break;
	
				
	}
	
}

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_TIM6_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
	
	HAL_TIM_Base_Start(&htim2);
	
	lcd_init();
	
	HAL_TIM_Base_Start_IT(&htim6);
	

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {		
		
		switch(task) {
			
			case 0:
				task0();
			break;
			
			case 1:
				task1();
			break;			
			
		}			
		
		switch(set) {

			case 0:
				set0();
			break;
			
			case 1:
				set1();
			break;
			
			
		}
		
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	
	if(tim < 500) tim++;
	else tim = 0;
	
	static uint8_t i = 0;
	
	if(i < 8) i++;
	else i = 0;
	col = 0x01<<i;

	red = 0;
	green = 0;
	for(uint8_t j = 0; j < 8; j++) {
	
		if ((i == intent.y && j == intent.x) && intent.type) {
			red |= (0x01<<j);
			green |= (0x01<<j);
		}
		
		if(tabuleiro[i][j] != NULL) {	
			
			if((*tabuleiro[i][j]).team == 2) {
				if(blink == tabuleiro[i][j]) {
					if(tim > 250) red |= (0x01<<j);
				} else if((*tabuleiro[i][j]).type == 1) {
					if(tim > 70) red |= (0x01<<j);
				} else red |= (0x01<<j);
			}
			else 
				if(blink == tabuleiro[i][j]) {
					if(tim > 250) green |= (0x01<<j);
				} else if((*tabuleiro[i][j]).type == 1) {
					if(tim > 70) green |= (0x01<<j);
				}else green |= (0x01<<j);
			
		}
	
	}
	
	vm = ~red;
	vd = ~green;
	HAL_SPI_Transmit(&hspi2, spi, 6, 999);
	
	write(rck, 1);
	write(rck, 0);
	
	readmcp();
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
