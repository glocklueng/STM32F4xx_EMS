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
#define UDP_NUM_PKT 10

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t seqNo = 0;

int8_t mysock = -1;
int8u TxBuf[TEST_BUFFERSIZE];

extern int destIP, srcIP;
extern long int destPort, srcPort;
extern int8u APOnOff;

char sockConnected = -1;
//char sockClosed = -1;
extern int32u timeout;
extern int32u selfIP;

bool IsVideoOn = false;
bool IsAudioOn = false;
bool IsSensorOn = false;
int8u teststr[128];

/* Private function prototypes -----------------------------------------------*/
void LCD_Init(void);

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
	
	LCD_DisplayStringLine(LINE(7), "Sensor Off");
	LCD_DisplayStringLine(LINE(8), "Audio  Off");
	LCD_DisplayStringLine(LINE(9), "Video  Off");
	//WiFi Command
  WifiOn(seqNo++);
	ApOnOff(1, seqNo++);
	GetStatus(seqNo++);
	
	//SNIC Command
  SnicInit(seqNo++);
  SnicGetDhcp(seqNo++);
	
	// TCPServer
  setTCPinfo();
	mysock = -1;
	tcpCreateSocket(1, srcIP, (unsigned short)srcPort, seqNo++, SNIC_TCP_CREATE_SOCKET_REQ);
		if (mysock != -1) {
			// This connection can receive data upto TEST_BUFFERSIZE at a time.
			tcpCreateConnection(mysock, TEST_BUFFERSIZE, 0x5, seqNo++);
		}
		
		/* loop */
		while(1){
			if(IsVideoOn)
			{
				teststr[0] = 0x02;
				//video();
				
			}
			if(IsAudioOn)
			{
				teststr[0] = 0x01;
				//audio();
			}
			if(IsSensorOn)
			{
				int32u tmp[3]; 
				char strtmp[20];
				tmp[0] = 23;
				tmp[1] = 34;
				tmp[2] = 14;
				teststr[0] = 0x00;
				
				sprintf(strtmp, "Send:%d %d %d", tmp[0], tmp[1], tmp[2]);
				LCD_DisplayStringLine(LINE(5), (uint8_t*)strtmp );
				tmp[0] = swap32(tmp[0]);
				tmp[1] = swap32(tmp[1]);
				tmp[2] = swap32(tmp[2]);
				memcpy(teststr + 1, (int8u*)&tmp[0], 4);
				memcpy(teststr + 5, (int8u*)&tmp[1], 4);
				memcpy(teststr + 9, (int8u*)&tmp[2], 4);
				/*
				char teststr[128] = "23";
				int len = (int)strlen((char*)&teststr);
				sendFromSock(sockConnected, (int8u*)teststr, len, 2, seqNo++);
				*/
				sendFromSock(sockConnected, teststr, 13, 2, seqNo++);
			}
			if(SN8200_API_HasInput()) {
				ProcessSN8200Input();
			}
		}
		
		/* // Shut down all
		SnicCleanup(seqNo++);
		WifiDisconn(seqNo++);
		WifiOff(seqNo++); 
		*/
		
}

//Initial LCD Display
void LCD_Init(void){
	STM32f4_Discovery_LCD_Init();         // Initial LCD
  LCD_Clear(LCD_COLOR_WHITE);           // Clear the LCD
  LCD_SetBackColor(LCD_COLOR_BLUE);     // Set the LCD Back Color
  LCD_SetTextColor(LCD_COLOR_WHITE);    // Set the LCD Text Color
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
