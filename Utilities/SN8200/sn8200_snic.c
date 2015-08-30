#include "sn8200_core.h"
#include "sn8200_api.h"
#include "sn8200_snic.h"
#include "delay.h"

#include <stdio.h>
#include <string.h>

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_lcd.h"

bool IsSNICIPConfigResponsed = false;
bool IsSNICGetDHCPInfoResponsed = false;
bool IsSNICTCPConnectToServerResponsed = false;
bool IsSNICSendFromSocketResponsed = false;
bool IsSNICUDPSendFromSocketResponsed = false;
bool IsCreateSocketResponsed = false;

extern bool IsVideoOn;
extern bool IsAudioOn;
extern bool IsSensorOn;

extern int32u timeout;

int32u pktcnt = 0;
int32u selfIP = 0;
int ipok = 0;

extern int8_t mysock;
extern int8s sockClosed;
extern int8s sockConnected;

volatile int8u sendUDPDone = 0;

int srcIP = INADDR_NONE;
long int srcPort = PORT_NONE;

int udpSrcIP = INADDR_NONE;
long int udpSrcPort = PORT_NONE;

void setTCPinfo(void)
{
    srcIP = selfIP;
    srcPort = 2222;
    srcPort = swap16(srcPort);
}

void setUDPinfo(void)
{
    udpSrcIP = selfIP;
    udpSrcPort = 2222;
    udpSrcPort = swap16(udpSrcPort);
}

//initialize the SNIC framework on SN8200
void SnicInit(int8u seq)
{
    int8u buf[4];
    int tmp;
    tmp = 0x00;			//The Default receive buffer size
    buf[0] = SNIC_INIT_REQ;
    buf[1] = seq;
    memcpy(buf+2, (uint8_t*)&tmp, 2);
    serial_transmit(CMD_ID_SNIC, buf, 4, ACK_NOT_REQUIRED);
}

//close the SNIC framework on SN8200
void SnicCleanup(int8u seq)
{
    int8u buf[2];
    buf[0] = SNIC_CLEANUP_REQ;
    buf[1] = seq;
    serial_transmit(CMD_ID_SNIC, buf, 2, ACK_NOT_REQUIRED);
}


void SnicIPConfig(int8u seq)
{
    int8u buf[16];

    buf[0] = SNIC_IP_CONFIG_REQ;
    buf[1] = seq;
    buf[2] = 0; //STA
    buf[3] = 1; //DHCP
    serial_transmit(CMD_ID_SNIC, buf, 4, ACK_NOT_REQUIRED);

    timeout = 10000;
    while (timeout--) {
        if(SN8200_API_HasInput()) {
            ProcessSN8200Input();
        }
        if(IsSNICIPConfigResponsed) {
            IsSNICIPConfigResponsed = false;
            break;
        }
        mdelay(1);
    }
}

//Query the DHCP information for a particular interface
void SnicGetDhcp(int8u seq)
{
    int8u buf[3];

    buf[0] = SNIC_GET_DHCP_INFO_REQ;
    buf[1] = seq;
    buf[2] = 1;    // 0: STA  1: AP

    serial_transmit(CMD_ID_SNIC, buf, 3, ACK_NOT_REQUIRED);

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
int tcpCreateSocket(int8u bindOption, int32u localIp, int16u port, int8u seq, int8u ssl)
{
    int8u buf[9];

    buf[0] = ssl;
    buf[1] = seq;
    buf[2] = bindOption;

    if (bindOption) {
			//Bind option set as 1 means the socket will be bound to Local IP address and Local port specified;
       memcpy(buf+3, (uint8_t*)&localIp, 4);
       memcpy(buf+7, (uint8_t*)&port, 2);
       serial_transmit(CMD_ID_SNIC, buf, 9, ACK_NOT_REQUIRED);
    } else
		   //Bind option set as 0 means the socket will not be bound
       serial_transmit(CMD_ID_SNIC, buf, 3, ACK_NOT_REQUIRED);

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


int closeSocket(int8u shortSocket, int8u seq)
{
    int8u buf[3];
    buf[0] = SNIC_CLOSE_SOCKET_REQ;
    buf[1] = seq;
    buf[2] = shortSocket;
    serial_transmit(CMD_ID_SNIC, buf, 3, ACK_NOT_REQUIRED);
    return 0;
}

//Instruct the SN8200 to use the TCP server socket to listen for incoming connection
int tcpCreateConnection(int8u shortSock, int16u size, int8u maxClient, int8u seq)
{
    int8u buf[6];
    if (size == 0 || size > MAX_BUFFER_SIZE) {
        size = MAX_BUFFER_SIZE;
    }

    if (maxClient == 0 || maxClient > MAX_CONNECTION_PER_SOCK)
        maxClient = MAX_CONNECTION_PER_SOCK;

    buf[0] = SNIC_TCP_CREATE_CONNECTION_REQ;
    buf[1] = seq;
    buf[2] = shortSock;
    size = swap16(size);
    memcpy(buf+3, (int8u*)&size, 2);

    buf[5] = maxClient;

    serial_transmit(CMD_ID_SNIC, buf, 6, ACK_NOT_REQUIRED);

    return 0;
}

//Instruct the SN8200 to send a packet to the remote peer via a connected socket
int sendFromSock(int8u shortSocket, int8u * sendBuf, int16u len, int8u timeout, int8u seq)
{
    int8u buf[MAX_BUFFER_SIZE+6];
    int16u mybufsize;
    if (len == 0 || len > MAX_BUFFER_SIZE) {
        len = MAX_BUFFER_SIZE;
    }
    buf[0] = SNIC_SEND_FROM_SOCKET_REQ;
    buf[1] = seq;
    buf[2] = shortSocket;
    buf[3] = 0;

    mybufsize = swap16(len);
    memcpy(buf+4, (int8u*)&mybufsize, 2);
    memcpy(buf+6, sendBuf, len);

    serial_transmit(CMD_ID_SNIC, buf, 6+len, ACK_NOT_REQUIRED);

    while (1) {
        if(SN8200_API_HasInput()) {
            ProcessSN8200Input();
        }
        if(IsSNICSendFromSocketResponsed) {
            IsSNICSendFromSocketResponsed = false;
            break;
        }
        mdelay(1);
    }
    return 0;
}

void handleRxSNIC(uint8_t* buf, int len)
{
    uint8_t subCmdId = buf[0];
    static int times = 0;

    switch (subCmdId) {

    case SNIC_CLOSE_SOCKET_RSP: {
        if (SNIC_SUCCESS != buf[2]) 
				    LCD_DisplayStringLine(LINE(9), "Close socket failed");
        else 
					  LCD_DisplayStringLine(LINE(9), "Socket closed");
    }
    break;

    case SNIC_IP_CONFIG_RSP: {
        IsSNICIPConfigResponsed = true;
        ipok = 0;
        if (SNIC_SUCCESS == buf[2]) {
					  LCD_DisplayStringLine(LINE(9), "IPConfig OK");
            ipok = 1;
        } else
				    LCD_DisplayStringLine(LINE(9), "IPConfig fail");
    }
    break;

    case SNIC_GET_DHCP_INFO_RSP: {
        IsSNICGetDHCPInfoResponsed = true;
        if (SNIC_SUCCESS == buf[2]) {
					  char strtmp[20];
					sprintf(strtmp, "IP:%i.%i.%i.%i", buf[9],buf[10],buf[11],buf[12]);
					LCD_DisplayStringLine(LINE(4), (uint8_t*)&strtmp);
            //save IP
            memcpy(&selfIP, buf+9, 4);
        } else
				    LCD_DisplayStringLine(LINE(4), "IP not assigned");
    }
    break;

    case SNIC_TCP_CREATE_SOCKET_RSP:
    case SNIC_UDP_CREATE_SOCKET_RSP: {
        IsCreateSocketResponsed = true;
        if (SNIC_SUCCESS == buf[2]) {
					char strtmp[20];
					mysock = buf[3];
					sprintf(strtmp, "Socket %d opened", mysock);
					LCD_DisplayStringLine(LINE(5), (uint8_t*)&strtmp);
        } else
				  LCD_DisplayStringLine(LINE(5), "Socket creation failed");
    }
    break;

    case SNIC_TCP_CONNECT_TO_SERVER_RSP: {
        IsSNICTCPConnectToServerResponsed = true;
        if (SNIC_CONNECT_TO_SERVER_PENDING == buf[2] || SNIC_SUCCESS == buf[2])
            ;
        else
            printf("Unable to connect server\n\r");
    }
    break;
    
    case SNIC_TCP_CREATE_CONNECTION_RSP:
            {
                if (SNIC_SUCCESS != buf[2])
                    printf("Unable to create TCP server\n\r");
            }
    break;

    case SNIC_TCP_CONNECTION_STATUS_IND: {
        if (SNIC_CONNECTION_UP == buf[2]) {
            printf("Socket connection UP\n\r");
            sockConnected = buf[3];
        }
        else if (SNIC_CONNECTION_CLOSED == buf[2]) {
            printf("Socket %i closed\n\r", buf[3]);
            sockClosed = buf[3];
        }
    }
    break;

    case SNIC_SEND_RSP: {
        int32u sentsize;
        IsSNICSendFromSocketResponsed = true;
        if (SNIC_SUCCESS == buf[2]) {
					  char strtmp[20];
            pktcnt ++;
            sentsize = ((int32u)(buf[3] << 8) | (int32u)buf[4]);
					  sprintf(strtmp, "pkt %d, %d bytes sent", pktcnt, sentsize);
            //printf("pkt %d, %d bytes sent \n\r", pktcnt, sentsize);
					  LCD_DisplayStringLine(LINE(7), (uint8_t*)&strtmp);
        }
    }
    break;

    case SNIC_CONNECTION_RECV_IND: {
        //int32u sentsize = ((int32u)(buf[3] << 8) | (int32u)buf[4]);
        int32u sock = (int32u)buf[2];
			  char strtmp[20];
			  sprintf(strtmp, "socket %d rvd %d", sock, buf[5]);
			  LCD_DisplayStringLine(LINE(6), (uint8_t*)&strtmp);
			  if(buf[5] == 0x00){
					IsSensorOn = false;
				}
				if(buf[5] == 0x01){
				  IsSensorOn = true;
				}
				if(buf[5] == 0x02){
				  IsAudioOn = false;
				}
				if(buf[5] == 0x03){
				  IsAudioOn = true;
				}
				if(buf[5] == 0x04){
				  IsVideoOn = false;
				}
				if(buf[5] == 0x05){
				  IsVideoOn = true;
				}
    }
    break;

    case SNIC_TCP_CLIENT_SOCKET_IND: {
        //int8u listen_sock = buf[2];
			  char strtmp[20];
			  sprintf(strtmp, "%d on %i.%i.%i.%i", buf[3], buf[4], buf[5], buf[6], buf[7]);
			  LCD_DisplayStringLine(LINE(4), (uint8_t*)&strtmp);

    }
    break;
    case SNIC_UDP_RECV_IND: {
        printf("%d %d\n\r", times++, htons(*((int16u*)&buf[9])));
    }
    break;
		
    case SNIC_UDP_SEND_FROM_SOCKET_RSP: {
        IsSNICUDPSendFromSocketResponsed = true;
        if (SNIC_SUCCESS != buf[2]) {
            printf("UDP Send bad\n\r");
        }
        sendUDPDone = 1;
        break;
    }

    default:
        break;

    }
}
