/******************************************************************************
* File Name          : main.c
* Author             : Xiaotian
* Date First Issued  : 11/07/2015
* Description        : Main program body
*******************************************************************************
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "sn8200_api.h"
#include "sn8200_core.h"
#include "sn8200_wifi.h"
#include "sn8200_snic.h"
#include "delay.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_lcd.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define TEST_BUFFERSIZE 128

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern int8u APOnOff;
extern int32u selfIP;
extern int srcIP;
extern long int srcPort;
extern int8_t mysock;
extern char sockConnected;
extern int32u timeout;

uint8_t seqNo = 0;

//int8u TxBuf[TEST_BUFFERSIZE];

bool IsVideoOn = false;
bool IsAudioOn = false;
bool IsSensorOn = false;
bool IsSystemExit = false;
int8u teststr[128];

/* Private function prototypes -----------------------------------------------*/
void LCD_Init(void);
float getRandFloat(float, float);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
	LCD_Init();
  SysTick_Configuration();
  SN8200_API_Init(921600);
	
	//WiFi Command
  WifiOn(seqNo++);
	ApOnOff(1, seqNo++);
	GetStatus(seqNo++);
	
	//SNIC Command
	SnicCleanup(seqNo++);
  SnicInit(seqNo++);
  SnicGetDhcp(seqNo++);
	
	// TCPServer
  setTCPinfo();
	//mysock = -1;
	tcpCreateSocket(1, srcIP, (unsigned short)srcPort, seqNo++);
	if (mysock != -1) {
		// This connection can receive data upto TEST_BUFFERSIZE at a time.
		tcpCreateConnection(mysock, TEST_BUFFERSIZE, 0x5, seqNo++);
	}
	/*
	while(1){
		char strtmp[20];
		sprintf(strtmp,"%f",getTmptr());
		LCD_DisplayStringLine(LINE(5), strtmp);
	}*/
	
	/* loop */
	while(1){
		if (IsVideoOn) {
			teststr[0] = 0x02;
				//video();
			}
		if (IsAudioOn) {
				teststr[0] = 0x01;
				//audio();
			}
		if (IsSensorOn) {
			float tmptr = getRandFloat(23.0f, 25.0f);
			float hmd = getRandFloat(44.0f, 47.0f);
			float ap = getRandFloat(1013.0f, 1024.0f);
			teststr[0] = 0x00;
			
			memcpy(teststr + 1, (int8u*)&tmptr, 4);
			memcpy(teststr + 5, (int8u*)&hmd, 4);
			memcpy(teststr + 9, (int8u*)&ap, 4);
			sendFromSock(sockConnected, teststr, 13, 2, seqNo++);
			mdelay(10); 
		}
		if(SN8200_API_HasInput()) {
			ProcessSN8200Input();
		}
		if(IsSystemExit) {
			LCD_Clear(LCD_COLOR_WHITE);
			break;
		}
	}
	// Shut down all
	SnicCleanup(seqNo++);
	WifiDisconn(seqNo++);
	//WifiOff(seqNo++);
	GetStatus(seqNo++);
	LCD_DisplayStringLine(LINE(1), "System Exit");
}

//Initial LCD Display
void LCD_Init(void){
	STM32f4_Discovery_LCD_Init();         // Initial LCD
  LCD_Clear(LCD_COLOR_WHITE);           // Clear the LCD
  LCD_SetBackColor(LCD_COLOR_BLUE);     // Set the LCD Back Color
  LCD_SetTextColor(LCD_COLOR_WHITE);    // Set the LCD Text Color
	
	LCD_DisplayStringLine(LINE(7), "Sensor Off");
	LCD_DisplayStringLine(LINE(8), "Audio  Off");
	LCD_DisplayStringLine(LINE(9), "Video  Off");
}

float getRandFloat(float min, float max){
	float randv;
	float range;
	char buf[10];
	randv = rand();
	range = max - min;
	randv = (randv / RAND_MAX) * range + min;
	sprintf(buf, "%.1f", randv);
	sscanf(buf, "%f", &randv);
	return randv;
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
        ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1) {
    }
}
#endif                                                                              
/******** (C) COPYRIGHT 2015 Xiaotian @ University of York *****END OF FILE****/
