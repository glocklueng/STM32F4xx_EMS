#include "sn8200_core.h"
#include "sn8200_api.h"
#include "delay.h"

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_lcd.h"

/*
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
*/

int32u timeout = 10000;

/*
extern int buf_pos;
extern unsigned char RxBuffer[];
extern int ack_recv_status;
*/

/*
int destIP = INADDR_NONE;
int srcIP = INADDR_NONE;
long int destPort = PORT_NONE;
long int srcPort = PORT_NONE;
int udpDestIP = INADDR_NONE;
int udpSrcIP = INADDR_NONE;
long int udpDestPort = PORT_NONE;
long int udpSrcPort = PORT_NONE;
*/

//extern int8u seqNo;

void SN8200_API_Init(uint32_t baudrate)
{
    SN8200_Init(baudrate);
}

bool SN8200_API_HasInput(void)
{
    return SN8200_HasInput();
}

void ProcessSN8200Input(void)
{
    rx_thread_proc();
}
