#include "sn8200_api.h"
#include "sn8200_core.h"
#include "sn8200_wifi.h"
#include "delay.h"

#include <stdio.h>
#include <string.h>

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_lcd.h"

int8u APOnOff = 1;

int joinok = 0;

extern int32u timeout;

bool IsWIFIGetStatusResponsed = false;
bool IsWIFIJoinResponsed = false;
bool IsWIFIApCtrlResponsed = false;

//Get the WiFi status from SN8200
void GetStatus(int8u seq)
{
    int8u payload[4];
    payload[0] = WIFI_GET_STATUS_REQ;
    payload[1] = seq;
    payload[2] = 0;
    serial_transmit(CMD_ID_WIFI, payload, 3, ACK_NOT_REQUIRED);

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

//Turn on Wifi on SN8200
void WifiOn(int8u seq)
{
    int8u payload[4];
    payload[0] = WIFI_ON_REQ;
    payload[1] = seq;
    payload[2] = (char)'U';   //Country code
    payload[3] = (char)'S';   //Country code
    serial_transmit(CMD_ID_WIFI, payload, 4, ACK_NOT_REQUIRED);
}

//Turn off Wifi on SN8200
void WifiOff(int8u seq)
{
    int8u payload[2];
    payload[0] = WIFI_OFF_REQ;
    payload[1] = seq;
    serial_transmit(CMD_ID_WIFI, payload, 2, ACK_NOT_REQUIRED);
}

//Turn on or off the soft AP
void ApOnOff(int8u OnOff, int8u seq)
{
    int8u payload[4];
    APOnOff = OnOff;
    payload[0] = WIFI_AP_CTRL_REQ;
    payload[1] = seq;
    payload[2] = APOnOff;
    payload[3] = 0; //persistency hardcode set as 0 means NOT save to NVM
    serial_transmit(CMD_ID_WIFI, payload, 4, ACK_NOT_REQUIRED);

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

/*
void WifiJoin(char* SSID, int8u secMode, char* secKey, int8u seq)
{
	int8u payload[128];
  int8u *p = payload;
	
	char SSID[32];
	int SSIDlen = 0;
	int8u secMode = 0;  //0 for open, 2 for WPA TKIP, 4 for WPA2 AES, 6 for WPA2 MIXED
	char secKey[64];
	int8u Keylen = 0;
  

    *p++ = WIFI_JOIN_REQ;
    *p++ = seq;

    memcpy(p, SSID, strlen(SSID));

    p += strlen(SSID);
    *p++ = 0x00;

    if (secMode) {
        Keylen = (unsigned char)strlen(secKey);
    }

    *p++ = secMode;
    *p++ = Keylen;

    if (Keylen) {
        memcpy(p, secKey, Keylen);
        p += Keylen;
    }
    serial_transmit(CMD_ID_WIFI, payload, (int)(p - buf), ACK_NOT_REQUIRED);
		
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
*/

void WifiDisconn(int8u seq)
{
    int8u payload[2];
    payload[0] = WIFI_DISCONNECT_REQ;
    payload[1] = seq;
    serial_transmit(CMD_ID_WIFI, payload, 2, ACK_NOT_REQUIRED);
}

void handleRxWiFi(int8u* buf, int len)
{
	int8u subCmdId = buf[0];
	
	switch (subCmdId) {
		
		case WIFI_GET_STATUS_RSP: {
			IsWIFIGetStatusResponsed = true;
			if (MODE_WIFI_OFF == buf[2]) {
				LCD_DisplayStringLine(LINE(0), "WiFi Off");
			} else {
				//Display MAC address
				char val[20] = {0};
        int i=0;
				for(i=0; i<6; i++) {
					sprintf(val+3*i, "%02X:", buf[3+i]);
				}
        val[strlen(val)-1] = 0;
				LCD_DisplayStringLine(LINE(1), (uint8_t*)&val);
        if ( MODE_NO_NETWORK == buf[2] )
					LCD_DisplayStringLine(LINE(0), "NO NETWORK");
				else if ( MODE_STA_JOINED == buf[2] ) {    
          char strtmp[20];
					sprintf(strtmp, "SSID:%s", buf+9);  
					LCD_DisplayStringLine(LINE(0), (uint8_t*)&strtmp);
				} else  // MODE_AP_STARTED
				  LCD_DisplayStringLine(LINE(0), "AP STARTED");
			}
			break;
		}

    case WIFI_JOIN_RSP: {
			IsWIFIJoinResponsed = true;
			if (WIFI_SUCCESS == buf[2])
				LCD_DisplayStringLine(LINE(2), "Join success   ");
			else
				LCD_DisplayStringLine(LINE(2), "Join fail      ");
			break;
		}

    case WIFI_AP_CTRL_RSP: {
			IsWIFIApCtrlResponsed = true;
			if (WIFI_SUCCESS == buf[2]) {
				if (APOnOff)
					LCD_DisplayStringLine(LINE(3), "AP is ON");
				else
					LCD_DisplayStringLine(LINE(3), "AP is OFF");
			} 
			else
			  LCD_DisplayStringLine(LINE(3), "AP control fail");
			break;
		}

    case WIFI_NETWORK_STATUS_IND: {
			if (WIFI_NETWORK_UP == buf[3]) {
				LCD_DisplayStringLine(LINE(2), "Network UP     ");
        joinok = 1;
			} else {
				LCD_DisplayStringLine(LINE(2), "Network Down   ");
			}
    }
    break;

    default:
        break;
	}
}
