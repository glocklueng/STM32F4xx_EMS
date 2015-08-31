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
char sockClosed = -1;
//extern bool IsCreateSocketResponsed;
extern int32u timeout;
//extern bool IsWIFIJoinResponsed ;
extern int32u selfIP;

bool IsVideoOn = false;
bool IsAudioOn = false;
bool IsSensorOn = false;

//timeout, destIP, destPort, sockConnected, sockClosed , TEST_BUFFERSIZE
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
				//video();
			}
			if(IsAudioOn)
			{
				//audio();
			}
			if(IsSensorOn)
			{
				char teststr[128] = "2";
				//int8u teststr[128];
		    int32u sock = 5;
	      int len;
				/*
				float tmp = 37.7;
				teststr[0] = 0;
				LCD_DisplayStringLine(LINE(2),"Hello");
				memcpy(teststr + 1, (int8u*)&tmp, 4);
				*/
		    len = (int)strlen((char*)&teststr);
				
        sendFromSock(5, (int8u*)teststr, len, 2, seqNo++);
				//sendFromSock(sock, teststr, len, 2, seqNo++);
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

/* WifiDisconn(seqNo++);
   WifiJoin(seqNo++);
   SnicInit(seqNo++);
   SnicIPConfig(seqNo++); */
/* //udp send 
   int i;
   udpCreateSocket(0, 0, 0, seqNo++);
   if (mysock != -1) {
      if (getUDPinfo() == CMD_ERROR) {
          printf("Invalid Server\n\r");
          break;
            }
            printf("Send %d\n\r", UDP_NUM_PKT);
            for (i=0; i<UDP_NUM_PKT; i++) {
                int si = i % TEST_BUFFERSIZE + 1;
                SendSNIC(TxBuf, si);
                printf("%d %d\n\r", i, si);
            }

            closeSocket(mysock,seqNo++);
        }*/
/*  //udp recv
    int16u port = 43211;
    int32u ip = 0xAC1F0001; // 172.31.0.1
    udpCreateSocket(1, ip, port, seqNo++);
    udpStartRecv(mysock, 2048, seqNo++);
    break;*/


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
