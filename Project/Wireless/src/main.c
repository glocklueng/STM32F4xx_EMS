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
#include "delay.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_lcd.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define TEST_BUFFERSIZE 128
#define UDP_NUM_PKT 10

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

//bool quit_flag = false;

//uint8_t key;
uint8_t seqNo = 0;

int8_t mysock = -1;
int8u TxBuf[TEST_BUFFERSIZE];

extern int ipok, joinok;
extern int destIP, srcIP;
extern long int destPort, srcPort;
//extern int32u pktcnt;
extern int8u APOnOff;

extern char Portstr[8];
char sockConnected = -1;
char sockClosed = -1;
extern bool IsCreateSocketResponsed ;
extern int32u timeout;
extern bool IsWIFIJoinResponsed ;
extern int32u selfIP;

bool IsVideoOn = false;
bool IsAudioOn = false;
bool IsSensorOn = false;

//timeout, Portstr, destIP, destPort, sockConnected, sockClosed , TEST_BUFFERSIZE
/* Private function prototypes -----------------------------------------------*/


/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
	STM32f4_Discovery_LCD_Init();
  LCD_Clear(LCD_COLOR_WHITE);           // Clear the LCD
  LCD_SetBackColor(LCD_COLOR_BLUE);     // Set the LCD Back Color
  LCD_SetTextColor(LCD_COLOR_WHITE);    // Set the LCD Text Color
	
  SysTick_Configuration();
  SN8200_API_Init(921600);

	//GetStatus(seqNo++);
    WifiOn(seqNo++);
	  ApOnOff(1, seqNo++);
	  if(SN8200_API_HasInput()) {
			ProcessSN8200Input();
		}
		SnicInit(seqNo++);
    SnicGetDhcp(seqNo++);
		if(SN8200_API_HasInput()) {
			ProcessSN8200Input();
		}
		/* TCPClient 
    mysock = -1;
    tcpCreateSocket(0, 0xFF, 0xFF, seqNo++, SNIC_TCP_CREATE_SOCKET_REQ);
    if (mysock != -1) {
			if (getTCPinfo() == CMD_ERROR) {
				printf("Invalid Server\n\r");
				return;
			}
      // This connection can receive data upto 0x0400=1K bytes at a time.
      tcpConnectToServer(mysock, destIP, (unsigned short)destPort, 0x0400, 0x5, seqNo++);
		}*/
		
		/* TCPServer */
		setTCPinfo();
		mysock = -1;
		tcpCreateSocket(1, srcIP, (unsigned short)srcPort, seqNo++, SNIC_TCP_CREATE_SOCKET_REQ);
		if (mysock != -1) {
			// This connection can receive data upto TEST_BUFFERSIZE at a time.
			tcpCreateConnection(mysock, TEST_BUFFERSIZE, 0x5, seqNo++);
		}
		
		/* loop */
		while(1){
			char teststr[128] = "2";
		  int32u sock = 5;
	    int len;
		  len = (int)strlen(teststr);
			if(IsVideoOn)
			{
				
			}
			if(IsAudioOn)
			{
				
			}
			if(IsSensorOn)
			{
        //sendFromSock(sock, (int8u*)teststr, len, 2, seqNo++);
			}
			if(SN8200_API_HasInput()) {
				ProcessSN8200Input();
			}
		}
		
		
		
		/* Send From Socket */
    /*
    char tempstr[2] = {0};
    int8u datamode;
    char sockstr[8];
    int32u sock;
    char teststr[128];
    int len;

    printf("Enter socket number to send from: \n\r");
    scanf("%s", sockstr);
    sock = strtol(sockstr, NULL, 0);

    printf("Content Option? (0: Default  1: User specific) \n\r");
    scanf("%s", tempstr);
    datamode = atoi(tempstr);

        if (datamode) {
            printf("Enter payload to send (up to 128 bytes): \n\r");
            scanf("%s", teststr);
            len = (int)strlen(teststr);
            sendFromSock(sock, (int8u*)teststr, len, 2, seqNo++);
        } else {
            sendFromSock(sock, TxBuf, TEST_BUFFERSIZE, 2, seqNo++);
            pktcnt = 0;
        }
				*/
}


/* WifiScan(seqNo++); */
/* WifiDisconn(seqNo++);
   WifiJoin(seqNo++);
   SnicInit(seqNo++);
   SnicIPConfig(seqNo++); */
/* SnicCleanup(seqNo++);
   WifiDisconn(seqNo++); */
/* SnicCleanup(seqNo++);
   WifiOff(seqNo++);     */
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
