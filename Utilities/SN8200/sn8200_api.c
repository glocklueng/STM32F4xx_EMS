#include "sn8200_core.h"
#include "sn8200_api.h"
#include "delay.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_lcd.h"

#define ACK_REQUIRED 1
#define ACK_NOT_REQUIRED 0

typedef struct {
    int ch;
    int rssi;
    int sectype;
    char SSIDname[33];
} scanlist_t ;

extern int buf_pos;
extern unsigned char RxBuffer[];
extern int ack_recv_status;
extern int8_t mysock;
extern int8s sockConnected;
extern int8s sockClosed;

int32u timeout = 10000;

int8u totalscan = 0;
scanlist_t sl[30];
char SSID[32];
int SSIDlen = 0;
int8u secMode = 0;
int8u APOnOff = 1;
char secKey[64];
int8u Keylen = 0;

int joinok = 0;
int ipok = 0;
int32u selfIP = 0;
int32u pktcnt = 0;

char domain[100];
char Portstr[8]={0};
char IPstr[32]={0};
char out[256];
#define HTTP_RSP_STR "HTTP/1.1 200 OK\r\n" \
    "Content-Type: text/html\r\n" \
    "Connection: close\r\n" \
    "Content-Length: 47\r\n\r\n" \
    "<html><body>Hello from SN8200 #%2d</body></html>\r\n\r\n"

bool IsWIFIGetStatusResponsed = false;
bool IsWIFIJoinResponsed = false;
bool IsWIFIApCtrlResponsed = false;

bool IsSNICIPConfigResponsed = false;
bool IsSNICGetDHCPInfoResponsed = false;
bool IsSNICTCPConnectToServerResponsed = false;
bool IsSNICSendFromSocketResponsed = false;
bool IsSNICUDPSendFromSocketResponsed = false;
bool IsCreateSocketResponsed = false;

int destIP = INADDR_NONE;
int srcIP = INADDR_NONE;
long int destPort = PORT_NONE;
long int srcPort = PORT_NONE;

int udpDestIP = INADDR_NONE;
int udpSrcIP = INADDR_NONE;
long int udpDestPort = PORT_NONE;
long int udpSrcPort = PORT_NONE;

volatile int8u sendUDPDone = 0;
extern int8u seqNo;

extern bool IsVideoOn;
extern bool IsAudioOn;
extern bool IsSensorOn;

//encodedLen

u32_t
inet_addr(const char *cp)
{
    struct in_addr val;

    if (inet_aton(cp, &val)) {
        return (val.s_addr);
    }
    return (INADDR_NONE);
}

/**
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 *
 * @param cp IP address in ascii represenation (e.g. "127.0.0.1")
 * @param addr pointer to which to save the ip address in network order
 * @return 1 if cp could be converted to addr, 0 on failure
 */
int
inet_aton(const char *cp, struct in_addr *addr)
{
    u32_t val;
    u8_t base;
    char c;
    u32_t parts[4];
    u32_t *pp = parts;

    c = *cp;
    for (;;) {
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, 1-9=decimal.
         */
        if (!isdigit(c))
            return (0);
        val = 0;
        base = 10;
        if (c == '0') {
            c = *++cp;
            if (c == 'x' || c == 'X') {
                base = 16;
                c = *++cp;
            } else
                base = 8;
        }
        for (;;) {
            if (isdigit(c)) {
                val = (val * base) + (int)(c - '0');
                c = *++cp;
            } else if (base == 16 && isxdigit(c)) {
                val = (val << 4) | (int)(c + 10 - (islower(c) ? 'a' : 'A'));
                c = *++cp;
            } else
                break;
        }
        if (c == '.') {
            /*
             * Internet format:
             *  a.b.c.d
             *  a.b.c   (with c treated as 16 bits)
             *  a.b (with b treated as 24 bits)
             */
            if (pp >= parts + 3)
                return (0);
            *pp++ = val;
            c = *++cp;
        } else
            break;
    }
    /*
     * Check for trailing characters.
     */
    if (c != '\0' && !isspace(c))
        return (0);
    /*
     * Concoct the address according to
     * the number of parts specified.
     */
    switch (pp - parts + 1) {

    case 0:
        return (0);       /* initial nondigit */

    case 1:             /* a -- 32 bits */
        break;

    case 2:             /* a.b -- 8.24 bits */
        if (val > 0xffffffUL)
            return (0);
        val |= parts[0] << 24;
        break;

    case 3:             /* a.b.c -- 8.8.16 bits */
        if (val > 0xffff)
            return (0);
        val |= (parts[0] << 24) | (parts[1] << 16);
        break;

    case 4:             /* a.b.c.d -- 8.8.8.8 bits */
        if (val > 0xff)
            return (0);
        val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
        break;
    }
    if (addr)
        addr->s_addr = htonl(val);
    return (1);
}


int getTCPinfo(void)
{
    char tempIPstr[32] = "172.31.0.4";
    char teststr[8] = "2222";
    
    if (strlen(IPstr)==0)
        strcpy(IPstr, "192.168.10.101");

    if (strlen(Portstr)==0)
        strcpy(Portstr, "0x6990");
		
    if (strlen(tempIPstr))
        strcpy(IPstr, tempIPstr);
    destIP = inet_addr(IPstr);
    if (destIP == INADDR_NONE || destIP == INADDR_ANY) {
        return CMD_ERROR;
    }
		
    if (strlen(teststr))
        strcpy(Portstr, teststr);
    destPort = strtol(Portstr, NULL, 0);
    destPort = swap16(destPort);
    if (destPort > 0xFFFF) {
        printf("Invalid port, max limit 0xFFFF \n\r");
        return CMD_ERROR;
    }

    return 0;

}

int setTCPinfo(void)
{
    char teststr[8] = "2222";
    if (selfIP == 0) {
        printf("IP address has not been obtained.\n\r");
        return CMD_ERROR;
    }

    srcIP = selfIP;

    srcPort = strtol(teststr, NULL, 0);
    srcPort = swap16(srcPort);

    if (srcPort > 0xFFFF) {
        printf("Invalid port, max limit 0xFFFF \n\r");
        return CMD_ERROR;
    }

    return 0;

}


int getUDPinfo(void)
{
    char tempIPstr[32];
    char teststr[8];
    printf("Enter server IP to connect: \n\r");
    scanf("%s", tempIPstr);
    printf("\n\r");
    udpDestIP = inet_addr(tempIPstr);
    if (udpDestIP == INADDR_NONE) {
        return CMD_ERROR;
    }

    printf("Enter server port number: \n\r");
    scanf("%s", teststr);
    printf("\n\r");
    udpDestPort = strtol(teststr, NULL, 0);
    udpDestPort = swap16(udpDestPort);
    if (udpDestPort > 0xFFFF) {
        printf("Invalid port, max limit 0xFFFF \n\r");
        return CMD_ERROR;
    }

    return 0;

}


int setUDPinfo(void)
{
    char teststr[8];
    if (selfIP == 0) {
        printf("IP address has not been obtained.\n\r");
        return CMD_ERROR;
    }

    udpSrcIP = selfIP;

    printf("Enter server port number to set: \n\r");
    scanf("%s", teststr);
    printf("\n\r");
    udpSrcPort = strtol(teststr, NULL, 0);
    udpSrcPort = swap16(udpSrcPort);

    if (udpSrcPort > 0xFFFF) {
        printf("Invalid port, max limit 0xFFFF \n\r");
        return CMD_ERROR;
    }

    return 0;

}

void SN8200_API_Init(uint32_t baudrate)
{
    SN8200_Init(baudrate);
}

bool SN8200_API_HasInput(void)
{
    return SN8200_HasInput();
}

void GetStatus(int8u seq)
{
    int8u buf[4];
    buf[0] = WIFI_GET_STATUS_REQ;
    buf[1] = seq;
    buf[2] = 0;
    serial_transmit(CMD_ID_WIFI, buf, 3, ACK_NOT_REQUIRED);

    timeout = 10000;
    while (timeout--) {
        if(SN8200_API_HasInput()) {
            ProcessSN8200Input();
        }
        if(IsWIFIGetStatusResponsed) {
            IsWIFIGetStatusResponsed = false;
            break;
        }
        mdelay(1);
    }
}

void WifiOn(int8u seq)
{
    int8u buf[4];
    buf[0] = WIFI_ON_REQ;
    buf[1] = seq;
    buf[2] = (char)'U';
    buf[3] = (char)'S';
    serial_transmit(CMD_ID_WIFI, buf, 4, ACK_NOT_REQUIRED);
}

void WifiOff(int8u seq)
{
    int8u buf[2];
    buf[0] = WIFI_OFF_REQ;
    buf[1] = seq;
    serial_transmit(CMD_ID_WIFI, buf, 2, ACK_NOT_REQUIRED);
}

void ApOnOff(int8u OnOff, int8u seq)
{
    int8u buf[4];

    APOnOff = OnOff;
    buf[0] = WIFI_AP_CTRL_REQ;
    buf[1] = seq;
    buf[2] = APOnOff;
    buf[3] = 0; //persistency hardcode set as 0 means NOT save to NVM
    serial_transmit(CMD_ID_WIFI, buf, 4, ACK_NOT_REQUIRED);

    timeout = 10000;
    while (timeout--) {
        if(SN8200_API_HasInput()) {
            ProcessSN8200Input();
        }
        if(IsWIFIApCtrlResponsed) {
            IsWIFIApCtrlResponsed = false;
            break;
        }
        mdelay(1);
    }
}

void WifiScan(int8u seq)
{
    int8u buf[12];
    buf[0] = WIFI_SCAN_REQ;
    buf[1] = seq;
    memset(&buf[2], 0, 10);
    buf[3] = 2; // bss type = any

    serial_transmit(CMD_ID_WIFI, buf, 12, ACK_NOT_REQUIRED);
}

void WifiJoin(int8u seq)
{
    int8u buf[128];
    char tempstr[2] = {0};
    int8u *p = buf;

    *p++ = WIFI_JOIN_REQ;
    *p++ = seq;

    printf("Enter SSID:\n\r");
    scanf("%s", SSID);
    printf("\n\r");
    while(!strlen(SSID)) {
        printf("SSID can't be empty. Enter SSID:\n\r");
        scanf("%s", SSID);
        printf("\n\r");
    }
    memcpy(p, SSID, strlen(SSID));

    p += strlen(SSID);
    *p++ = 0x00;

    printf("Enter Security Mode (e.g., 0 for open, 2 for WPA TKIP, 4 for WPA2 AES, 6 for WPA2 MIXED):\n\r");
    scanf("%s", tempstr);
    printf("\n\r");
    secMode = atoi(tempstr);

    if (secMode) {
        printf("Enter Security Key:\n\r");
        scanf("%s", secKey);
        printf("\n\r");
        Keylen = (unsigned char)strlen(secKey);

        if (Keylen <= 0) {
            printf("Invalid Key\n\r");
            return;
        }
    }

    *p++ = secMode;
    *p++ = Keylen;

    if (Keylen) {
        memcpy(p, secKey, Keylen);
        p += Keylen;
    }

    serial_transmit(CMD_ID_WIFI, buf, (int)(p - buf), ACK_NOT_REQUIRED);
		
    timeout = 10000;
    while (timeout--) {
        if(SN8200_API_HasInput()) {
            ProcessSN8200Input();
        }
        if(IsWIFIJoinResponsed) {
            IsWIFIJoinResponsed = false;
            break;
        }
        mdelay(1);
    }
    joinok = 0;
}

void WifiDisconn(int8u seq)
{
    int8u buf[2];
    buf[0] = WIFI_DISCONNECT_REQ;
    buf[1] = seq;
    serial_transmit(CMD_ID_WIFI, buf, 2, ACK_NOT_REQUIRED);
}

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

void SnicGetDhcp(int8u seq)
{
    int8u buf[3];
    char tempstr[2] = {0};

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


int tcpCreateSocket(int8u bindOption, int32u localIp, int16u port, int8u seq, int8u ssl)
{
    int8u buf[9];

    buf[0] = ssl;
    buf[1] = seq;
    buf[2] = bindOption;

    if (bindOption) {
        memcpy(buf+3, (uint8_t*)&localIp, 4);
        memcpy(buf+7, (uint8_t*)&port, 2);
        serial_transmit(CMD_ID_SNIC, buf, 9, ACK_NOT_REQUIRED);
    } else
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

    printf("-closeSocket\n\r");
    printf("Socket %d closed\n\r", shortSocket);

    return 0;

}

int tcpConnectToServer(int8u shortSock, int32u ip, int16u port, int16u bufsize, int8u timeout, int8u seq)
{
    int8u buf[12];
    if (bufsize == 0 || bufsize > MAX_BUFFER_SIZE) {
        bufsize = MAX_BUFFER_SIZE;
    }

    buf[0] = SNIC_TCP_CONNECT_TO_SERVER_REQ;
    buf[1] = seq;
    buf[2] = shortSock;

    memcpy(buf+3, (int8u*)&ip, 4);
    memcpy(buf+7, (int8u*)&port, 2);
    bufsize = swap16(bufsize);
    memcpy(buf+9, (int8u*)&bufsize, 2);

    buf[11] = timeout;

    serial_transmit(CMD_ID_SNIC, buf, 12, ACK_NOT_REQUIRED);

    mdelay(1000);        //Wait module return value
    while (1) {
        if(SN8200_API_HasInput()) {
            ProcessSN8200Input();
        }
        if(IsSNICTCPConnectToServerResponsed) {
            IsSNICTCPConnectToServerResponsed = false;
            break;
        }
        mdelay(1);
    }
    return 0;
}

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


int udpCreateSocket(int8u bindOption, int32u ip, int16u port, int8u seq)
{
    int8u buf[9];

    buf[0] = SNIC_UDP_CREATE_SOCKET_REQ;
    buf[1] = seq;
    buf[2] = bindOption;

    if (bindOption) {
        int32u myip = swap32(ip);
        int16u myport = swap16(port);
        memcpy(buf+3, (int8u*)&myip, 4);
        memcpy(buf+7, (int8u*)&myport, 2);
        serial_transmit(CMD_ID_SNIC, buf, 9, ACK_NOT_REQUIRED);
    } else
        serial_transmit(CMD_ID_SNIC, buf, 3, ACK_NOT_REQUIRED);

    while (1) {
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

int udpSendFromSock(int32u ip, int16u iPort, int8u shortsock, int8u conMode, int8u *sendbuf, int16u len, int8u seq)
{
    int8u buf[2048+12];
    int16u mybufsize;
    if (len == 0 || len > MAX_BUFFER_SIZE) {
        len = MAX_BUFFER_SIZE;
    }
    buf[0] = SNIC_UDP_SEND_FROM_SOCKET_REQ;
    buf[1] = seq;
    memcpy(buf+2, (int8u*)&ip, 4);
    memcpy(buf+6, (int8u*)&iPort, 2);
    buf[8] = shortsock;
    buf[9] = conMode;
    mybufsize = swap16(len);
    memcpy(buf+10, (int8u*)&mybufsize, 2);
    memcpy(buf+12, sendbuf, len);
    serial_transmit(CMD_ID_SNIC, buf, 12+len, ACK_NOT_REQUIRED);

    timeout = 10000;
    while (timeout--) {
        if(SN8200_API_HasInput()) {
            ProcessSN8200Input();
        }
        if(sendUDPDone) {
            sendUDPDone = 0;
            break;
        }
        mdelay(1);
    }

    return 0;
}

int udpStartRecv(int32u sock, int16u bufsize, int8u seq)
{
    int8u buf[5];
    int tmp;
    if (bufsize ==0 || bufsize > MAX_BUFFER_SIZE) {
        bufsize = MAX_BUFFER_SIZE;
    }
    tmp = swap16(bufsize);
    buf[0] = SNIC_UDP_START_RECV_REQ;
    buf[1] = seq;
    buf[2] = sock;
    memcpy(buf+3, (int8u*)&tmp, 2);
    serial_transmit(CMD_ID_SNIC, buf, 5, ACK_NOT_REQUIRED);

    return 0;
}

void ProcessSN8200Input(void)
{
    rx_thread_proc();
}


void handleRxWiFi(int8u* buf, int len)
{
    int8u subCmdId = buf[0];

    switch (subCmdId) {
    case WIFI_GET_STATUS_RSP: {
        IsWIFIGetStatusResponsed = true;
        if (buf[2] == MODE_WIFI_OFF) {
            printf("WiFi Off.\n\r");
        } else {
            char val[20] = {0};
            int i=0;
            for(i=0; i<6; i++) {
                sprintf(val+3*i, "%02X:", buf[3+i]);
            }
            val[strlen(val)-1] = 0;
            printf("WiFi On.  Mac: %s.  ", val);

            if (buf[2] == MODE_NO_NETWORK) {
                printf("Not joined any network.\n\r");
            } else {
                printf("Joined SSID: %s\n\r", buf+9);
            }
        }
    }
    break;

    case WIFI_JOIN_RSP: {
        IsWIFIJoinResponsed = true;
        if (WIFI_SUCCESS == buf[2])
            printf("Join success\n\r");
        else
            printf("Join fail\n\r");
    }
    break;

    case WIFI_AP_CTRL_RSP: {
        IsWIFIApCtrlResponsed = true;
        if (WIFI_SUCCESS == buf[2]) {
            if (APOnOff)
						    LCD_DisplayStringLine(LINE(1), "AP is ON");
            else
						    LCD_DisplayStringLine(LINE(1), "AP is OFF");
        } else
				    LCD_DisplayStringLine(LINE(1), "AP control fail");
    }
    break;

    case WIFI_NETWORK_STATUS_IND: {
        if (WIFI_NETWORK_UP == buf[3]) {
            printf("Network UP\n\r");
            joinok = 1;
        } else {
            printf("Network Down\n\r");
        }
    }
    break;
    
		/*
    case WIFI_SCAN_RESULT_IND: {
        int cnt = buf[2];
        int i=3;
        int j;
        int8u ch, sec_tmp;
        int8s rssi;
        int8u len=32;

        if(cnt == 0) {
            for (j = 0; j < totalscan; j++) {
                printf("SSID: %20s CH: %2d RSSI: %3d Sec: %d\n\r",sl[j].SSIDname, sl[j].ch,sl[j].rssi,sl[j].sectype);
            }
            memset(sl, 0, totalscan * sizeof(scanlist_t));
            totalscan = 0; //reset current scan result
        } else {
            while (cnt--) {
                ch = buf[i++];
                rssi = (int8s)buf[i++];
                sec_tmp = buf[i++];
                i += 6;
                i++;
                i += 2;
                len = (int8u)strlen((char*)buf+i);
                if (len>32) {
                    break;
                }

                strcpy((char*)sl[totalscan].SSIDname,(char*)buf+i);

                sl[totalscan].ch = ch;
                sl[totalscan].rssi = rssi;
                sl[totalscan].sectype = sec_tmp;

                if (len == 0) {
                    while (buf[i] == 0) i++;
                } else
                    i += len+1;

                totalscan++;
            }
        }
    }
    break;
		*/

    default:
        break;
    }
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
					LCD_DisplayStringLine(LINE(2), strtmp);
            //save IP
            memcpy(&selfIP, buf+9, 4);
        } else
				    LCD_DisplayStringLine(LINE(2), "IP not assigned");
    }
    break;

    case SNIC_TCP_CREATE_SOCKET_RSP:
    case SNIC_UDP_CREATE_SOCKET_RSP: {
        IsCreateSocketResponsed = true;
        if (SNIC_SUCCESS == buf[2]) {
					char strtmp[20];
					mysock = buf[3];
					sprintf(strtmp, "Socket %d opened", mysock);
					LCD_DisplayStringLine(LINE(3), strtmp);
        } else
				  LCD_DisplayStringLine(LINE(3), "Socket creation failed");
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
					  LCD_DisplayStringLine(LINE(7), strtmp);
        }
    }
    break;

    case SNIC_CONNECTION_RECV_IND: {
        int i=0;
        int32u sentsize = ((int32u)(buf[3] << 8) | (int32u)buf[4]);
        int32u sock = (int32u)buf[2];
			  char strtmp[20];
			  sprintf(strtmp, "socket %d rvd %d", sock, buf[5]);
			  LCD_DisplayStringLine(LINE(5), strtmp);
			  if(buf[5] = 0x00){
					IsSensorOn = false;
				}
				if(buf[5] = 0x01){
				  IsSensorOn = true;
				}
				if(buf[5] = 0x02){
				  IsAudioOn = false;
				}
				if(buf[5] = 0x03){
				  IsAudioOn = true;
				}
				if(buf[5] = 0x04){
				  IsVideoOn = false;
				}
				if(buf[5] = 0x05){
				  IsVideoOn = true;
				}
				
				/*
        mdelay(10);
        if (strncmp((char*)buf+5, "GET /", 5) == 0 || strncmp((char*)buf+5, "POST /", 6) == 0) { // Receives a HTTP(S) get/post request.
            static int i=0;
            for (i=0; i<sentsize; i++) {
                printf("%c", buf[5+i]);
            }
            i = 0;
            sprintf(out, HTTP_RSP_STR, i++);
            // Send back something
            sendFromSock(sock, (int8u*)out, strlen(out), 2, seqNo++);
        
            // If it is a TLS server, it only accepts one connection, so
            // it is better for the host app to close it after some idle time for new connections.
            //	Sleep(100);
            //	closeSocket((int8u)sock,seqNo++);
        }
        else if (strncmp((char*)buf+5, "HTTP/1.", 7) == 0) { // Receives HTTP response, close socket. If the socket is not closed,
             for (i=0; i<sentsize; i++) {
                printf("%c", buf[5+i]);
            }                                              // it can be used for more data communication (using send from socket).
            closeSocket((int8u)sock,seqNo++); }*/
    }
    break;

    case SNIC_TCP_CLIENT_SOCKET_IND: {
        int8u listen_sock = buf[2];
			  char strtmp[20];
			  sprintf(strtmp, "%d on %i.%i.%i.%i", buf[3], buf[4], buf[5], buf[6], buf[7]);
			  LCD_DisplayStringLine(LINE(4), strtmp);

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

void SendSNIC(unsigned char *buf, int size)
{
    const unsigned int IP = udpDestIP;
    int16u port = udpDestPort;
    sendUDPDone = 0;
    udpSendFromSock(IP, port, mysock, 0, buf, size, seqNo++);
}

