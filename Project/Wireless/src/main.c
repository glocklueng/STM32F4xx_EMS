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
#define DBGU_RX_BUFFER_SIZE 256
#define TEST_BUFFERSIZE 128
#define UDP_NUM_PKT 10

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t DBGU_RxBuffer[DBGU_RX_BUFFER_SIZE];
uint32_t DBGU_RxBufferTail = 0;
uint32_t DBGU_RxBufferHead = 0;
bool DBGU_InputReady = false;
bool quit_flag = false;

//uint8_t key;
uint8_t seqNo = 0;

int8_t mysock = -1;
int8u TxBuf[TEST_BUFFERSIZE];

extern int ipok, joinok;
extern int destIP, srcIP;
extern long int destPort, srcPort;
extern int32u pktcnt;

//extern char domain[100];
extern char Portstr[8];
//char uri[100]={0};
char sockConnected = -1;
char sockClosed = -1;
int timeout1 = 5;
extern bool IsCreateSocketResponsed ;
extern int32u timeout;
extern bool IsWIFIJoinResponsed ;


#define GET_REQUEST \
    "GET / HTTP/1.1\r\n" \
    "Host: 192.168.2.125\r\n" \
    "Accept: text/html\r\n" \
    "\r\n"

/* Private function prototypes -----------------------------------------------*/
#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

void DBGU_Init(void);
bool DBGU_RxBufferEmpty(void);
uint8_t DBGU_GetChar(void);
void ProcessUserInput(uint8_t key);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
	  STM32f4_Discovery_LCD_Init();
    /* Display message on stm32f4_discovery LCD **********************************/
    /* Clear the LCD */ 
    LCD_Clear(LCD_COLOR_WHITE);
    /* Set the LCD Back Color */
    LCD_SetBackColor(LCD_COLOR_BLUE);
    /* Set the LCD Text Color */
    LCD_SetTextColor(LCD_COLOR_WHITE);
	
    SysTick_Configuration();
    DBGU_Init();
    SN8200_API_Init(921600);
    //strcpy(domain, "www.murata-ws.com");
    //strcpy(uri, "/index.html");

    WifiOn(seqNo++);
	  ApOff(seqNo++);
    ProcessUserInput('d');
    
	  /* Infinite loop */
    /*while (1) {
        if(DBGU_InputReady) {
            //ProcessUserInput();
					  ProcessUserInput('d');
        }

        if(SN8200_API_HasInput()) {
            ProcessSN8200Input();
        }

        if(quit_flag)
            break;
    }*/
}

void DBGU_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable GPIO clock */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    /* Enable UART clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);


    /* Connect PXx to USARTx_Tx*/
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_USART3);

    /* Connect PXx to USARTx_Rx*/
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_USART3);

    /* Configure USART Tx as alternate function  */
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Configure USART Rx as alternate function  */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

    /* Enable the USART3 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* USART configuration */
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART3, &USART_InitStructure);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    /* Enable USART */
    USART_Cmd(USART3, ENABLE);
}


bool DBGU_RxBufferEmpty(void)
{
    return (DBGU_RxBufferHead == DBGU_RxBufferTail);
}


void USART3_IRQHandler(void)
{
    uint8_t ch;
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        ch = USART_ReceiveData(USART3);
        switch (ch) {
        case 0x7F:
            if(DBGU_RxBufferHead != DBGU_RxBufferTail) {
                DBGU_RxBufferHead = (DBGU_RxBufferHead - 1) % DBGU_RX_BUFFER_SIZE;
                USART_SendData(USART3, 0x7F);
            }
            break;
        case 0x0D:
            DBGU_RxBuffer[DBGU_RxBufferHead] = ch;
            USART_SendData(USART3, 0x0D);
            DBGU_RxBufferHead = (DBGU_RxBufferHead + 1) % DBGU_RX_BUFFER_SIZE;
            DBGU_InputReady = true;
            break;
        default:
            DBGU_RxBuffer[DBGU_RxBufferHead] = ch;
            USART_SendData(USART3, ch);
            DBGU_RxBufferHead = (DBGU_RxBufferHead + 1) % DBGU_RX_BUFFER_SIZE;
            break;
        }
    }
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE {
    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART */
    USART_SendData(USART3, (uint8_t) ch);

    /* Loop until the end of transmission */
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET) {
    }

    return ch;
}

uint8_t DBGU_GetChar(void)
{
    uint8_t ch = 0;

    if(DBGU_RxBufferHead != DBGU_RxBufferTail) {
        ch = DBGU_RxBuffer[DBGU_RxBufferTail];
        DBGU_RxBufferTail = (DBGU_RxBufferTail + 1) % DBGU_RX_BUFFER_SIZE;
    } else {
        DBGU_InputReady = false;
    }

    return ch;
}


int fgetc(FILE *f)
{
    uint8_t ch = 0;

    while (!DBGU_InputReady);
    while(DBGU_RxBufferHead == DBGU_RxBufferTail);
    ch = DBGU_RxBuffer[DBGU_RxBufferTail];
    DBGU_RxBufferTail = (DBGU_RxBufferTail + 1) % DBGU_RX_BUFFER_SIZE;
    if (DBGU_RxBufferHead == DBGU_RxBufferTail)
        DBGU_InputReady = false;

    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);

    return ch;
}

void ProcessUserInput(uint8_t key)
{
    //char tmp[100];
    //key = DBGU_GetChar();
    switch(key) {
		//d 
		case 'd':
				LCD_DisplayStringLine(LINE(9), "   Success In   ");
			  break;
	  //0 Get WiFi status
    case '0':
        GetStatus(seqNo++);
        break;
    //1 Wifi Scan
    case '1':
        WifiScan(seqNo++);
        break;
    //2 Wifi Join
    case '2':
        WifiDisconn(seqNo++);
        WifiJoin(seqNo++);
        SnicInit(seqNo++);
        SnicIPConfig(seqNo++);
        break;
    //3 Get IP
    case '3':
        SnicInit(seqNo++);
        SnicGetDhcp(seqNo++);
        break;
    //4 TCP client
    case '4':
        mysock = -1;
        tcpCreateSocket(0, 0xFF, 0xFF, seqNo++, SNIC_TCP_CREATE_SOCKET_REQ);
        if (mysock != -1) {
            if (getTCPinfo() == CMD_ERROR) {
                printf("Invalid Server\n\r");
                break;
            }
            // This connection can receive data upto 0x0400=1K bytes at a time.
            tcpConnectToServer(mysock, destIP, (unsigned short)destPort, 0x0400, 0x5, seqNo++);
        }
        break;
    //5 TCP sever
    case '5':
        if (setTCPinfo() == CMD_ERROR) {
            printf("Invalid Server to create\n\r");
            break;
        }
        mysock = -1;
        tcpCreateSocket(1, srcIP, (unsigned short)srcPort, seqNo++, SNIC_TCP_CREATE_SOCKET_REQ);
        if (mysock != -1) {
            // This connection can receive data upto TEST_BUFFERSIZE at a time.
            tcpCreateConnection(mysock, TEST_BUFFERSIZE, 0x5, seqNo++);
        }
        break;
    //6 Send from sock
    case '6': {
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
        break;
    }
    //7 WiFi Leave
    case '7':
        SnicCleanup(seqNo++);
        WifiDisconn(seqNo++);
        break;
    //8 AP On/Off
    case '8':
        ApOnOff(seqNo++);
        break;
    //9 UDP client
    case '9': {//udp send
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
        }
        break;
    }
    //a UDP server
    case 'a': {//udp recv
        int16u port = 43211;
        int32u ip = 0xAC1F0001; // 172.31.0.1
        udpCreateSocket(1, ip, port, seqNo++);
        udpStartRecv(mysock, 2048, seqNo++);
        break;
    }
    //b Wifi Off
    case 'b':
        SnicCleanup(seqNo++);
        WifiOff(seqNo++);
        break;
    //c Wifi On
    case 'c':
        WifiOn(seqNo++);
        break;
    default:
        break;
    }
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
