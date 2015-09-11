#include "sn8200_core.h"
#include "sn8200_api.h"
#include "sn8200_snic.h"
#include "delay.h"

#include <stdio.h>
#include <string.h>

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_lcd.h"

extern bool IsSensorOn;
extern bool IsAudioOn;
extern bool IsVideoOn;
extern bool IsSystemExit;
extern int32u timeout;

int8_t mysock = -1;
int8s sockConnected  = -1;

int srcIP = INADDR_NONE;
long int srcPort = PORT_NONE;

int32u pktcnt = 0;
int32u selfIP = 0;

bool IsSNICGetDHCPInfoResponsed = false;
bool IsCreateSocketResponsed = false;

//Set IP address and Port Number;
void setTCPinfo(void)
{
    srcIP = selfIP;
    srcPort = 2222;
    srcPort = swap16(srcPort);
}

//initialize the SNIC framework on SN8200
void SnicInit(int8u seq)
{
    int8u payload[4];
    int tmp;
    tmp = 0x00;			//The Default receive buffer size
    payload[0] = SNIC_INIT_REQ;
    payload[1] = seq;
    memcpy(payload + 2, (uint8_t*)&tmp, 2);
    serial_transmit(CMD_ID_SNIC, payload, 4, ACK_NOT_REQUIRED);
}

//close the SNIC framework on SN8200
void SnicCleanup(int8u seq)
{
    int8u payload[2];
    payload[0] = SNIC_CLEANUP_REQ;
    payload[1] = seq;
    serial_transmit(CMD_ID_SNIC, payload, 2, ACK_NOT_REQUIRED);
}

//Query the DHCP information for a particular interface
void SnicGetDhcp(int8u seq)
{
    int8u payload[3];
    payload[0] = SNIC_GET_DHCP_INFO_REQ;
    payload[1] = seq;
    payload[2] = 1;    // 0: STA  1: AP

    serial_transmit(CMD_ID_SNIC, payload, 3, ACK_NOT_REQUIRED);

    timeout = 10000;
    while (timeout--) {
        if(SN8200_API_HasInput()) {
            ProcessSN8200Input();
        }
        if(IsSNICGetDHCPInfoResponsed) {
            IsSNICGetDHCPInfoResponsed = false;
            break;
        }
        mdelay(1);
    }
}

//Create a TCP socket with bind option
int tcpCreateSocket(int8u bindOption, int32u localIp, int16u port, int8u seq)
{
    int8u payload[9];
    payload[0] = SNIC_TCP_CREATE_SOCKET_REQ;
    payload[1] = seq;
    payload[2] = bindOption;

    if (bindOption) {
			//Bind option set as 1 means the socket will be bound to Local IP address and Local port specified;
       memcpy(payload + 3, (uint8_t*)&localIp, 4);
       memcpy(payload + 7, (uint8_t*)&port, 2);
       serial_transmit(CMD_ID_SNIC, payload, 9, ACK_NOT_REQUIRED);
    } else
		   //Bind option set as 0 means the socket will not be bound
       serial_transmit(CMD_ID_SNIC, payload, 3, ACK_NOT_REQUIRED);

    timeout = 10000;
    while (timeout--) {
        if(SN8200_API_HasInput()) {
            ProcessSN8200Input();
        }
        if(IsCreateSocketResponsed) {
            IsCreateSocketResponsed = false;
            break;
        }
        mdelay(1);
    }
    return 0;
}

//Close the socket
int closeSocket(int8u sock, int8u seq)
{
    int8u payload[3];
    payload[0] = SNIC_CLOSE_SOCKET_REQ;
    payload[1] = seq;
    payload[2] = sock;
    serial_transmit(CMD_ID_SNIC, payload, 3, ACK_NOT_REQUIRED);
    return 0;
}

//Instruct the SN8200 to use the TCP server socket to listen for incoming connection
int tcpCreateConnection(int8u shortSock, int16u size, int8u maxClient, int8u seq)
{
    int8u payload[6];
    if (size == 0 || size > MAX_BUFFER_SIZE) {
        size = MAX_BUFFER_SIZE;
    }

    if (maxClient == 0 || maxClient > MAX_CONNECTION_PER_SOCK)
        maxClient = MAX_CONNECTION_PER_SOCK;

    payload[0] = SNIC_TCP_CREATE_CONNECTION_REQ;
    payload[1] = seq;
    payload[2] = shortSock;
    size = swap16(size);
    memcpy(payload + 3, (int8u*)&size, 2);

    payload[5] = maxClient;

    serial_transmit(CMD_ID_SNIC, payload, 6, ACK_NOT_REQUIRED);

    return 0;
}

//Instruct the SN8200 to send a packet to the remote peer via a connected socket
int sendFromSock(int8u shortSocket, int8u * sendBuf, int16u len, int8u timeout, int8u seq)
{
    int8u payload[MAX_BUFFER_SIZE+6];
    int16u mybufsize;
    if (len == 0 || len > MAX_BUFFER_SIZE) {
        len = MAX_BUFFER_SIZE;
    }
    payload[0] = SNIC_SEND_FROM_SOCKET_REQ;
    payload[1] = seq;
    payload[2] = shortSocket;
    payload[3] = 0;  //on action

    mybufsize = swap16(len);
    memcpy(payload + 4, (int8u*)&mybufsize, 2);
    memcpy(payload + 6, sendBuf, len);

    serial_transmit(CMD_ID_SNIC, payload, 6 + len, ACK_NOT_REQUIRED);
		
    return 0;
}

void handleRxSNIC(uint8_t* buf, int len)
{
	uint8_t subCmdId = buf[0];
	char strtmp[20];
	
	switch (subCmdId) {
		case SNIC_CLOSE_SOCKET_RSP: {
			if (SNIC_SUCCESS != buf[2]) 
				LCD_DisplayStringLine(LINE(9), "Close socket failed      ");
			else 
				LCD_DisplayStringLine(LINE(9), "Socket closed            ");
			break;
		}

    case SNIC_GET_DHCP_INFO_RSP: {
			IsSNICGetDHCPInfoResponsed = true;
			if (SNIC_SUCCESS == buf[2]) {
				sprintf(strtmp, "IP:%i.%i.%i.%i", buf[9],buf[10],buf[11],buf[12]);
				LCD_DisplayStringLine(LINE(3), (uint8_t*)strtmp);
				//save IP
				memcpy(&selfIP, buf+9, 4);
			} else
				LCD_DisplayStringLine(LINE(3), "IP not assigned");
			break;
    }

    case SNIC_TCP_CREATE_SOCKET_RSP: {
			IsCreateSocketResponsed = true;
			if (SNIC_SUCCESS == buf[2]) {
				mysock = buf[3];
				sprintf(strtmp, "Socket %d opened", mysock);
				LCD_DisplayStringLine(LINE(4), (uint8_t*)strtmp);
			} else
			  LCD_DisplayStringLine(LINE(4), "Socket creation failed");
			break;
    }
		
		//Connection established 
		case SNIC_TCP_CLIENT_SOCKET_IND: {
			sockConnected = buf[3];
			sprintf(strtmp, "Conn from %i.%i.%i.%i", buf[4], buf[5], buf[6], buf[7]);
			LCD_DisplayStringLine(LINE(4), (uint8_t*)strtmp);
			break;
    }
		
		//Receive the instrutions
		case SNIC_CONNECTION_RECV_IND: {
			if(buf[5] == 0x00){
				IsSensorOn = false;
				LCD_DisplayStringLine(LINE(7), "Sensor Off");
			}
			if(buf[5] == 0x01){
				IsSensorOn = true;
				LCD_DisplayStringLine(LINE(7), "Sensor On ");
			}
			if(buf[5] == 0x02){
				IsAudioOn = false;
				LCD_DisplayStringLine(LINE(8), "Audio  Off");
			}
			if(buf[5] == 0x03){
				IsAudioOn = true;
				LCD_DisplayStringLine(LINE(8), "Audio  On ");
			}
			if(buf[5] == 0x04){
				IsVideoOn = false;
				LCD_DisplayStringLine(LINE(9), "Video  Off");
			}
			if(buf[5] == 0x05){
				IsVideoOn = true;
				LCD_DisplayStringLine(LINE(9), "Video  On ");
			}
			if(buf[5] == 0x06){
				IsSystemExit = true; 
			}
			break;
    }

		//Send Data
    case SNIC_SEND_RSP: {
			int32u sentsize;
			if (SNIC_SUCCESS == buf[2]) {
				pktcnt ++;
				sentsize = ((int32u)(buf[3] << 8) | (int32u)buf[4]);
				sprintf(strtmp, "pkt %d:%d bytes", pktcnt, sentsize);
				LCD_DisplayStringLine(LINE(6), (uint8_t*)strtmp);
			} else if (SNIC_PACKET_TOO_LARGE == buf[2] ){
			  LCD_DisplayStringLine(LINE(6), "Send Packet Too Large ");
			}
			break;
    }

    default:
        break;

    }
}
