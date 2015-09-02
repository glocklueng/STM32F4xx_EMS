#include "sn8200_api.h"
#include "sn8200_core.h"
#include "sn8200_wifi.h"
#include "delay.h"

#include <stdio.h>
#include <string.h>

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_lcd.h"

int8u APOnOff = 1;

extern int32u timeout;

bool IsWIFIGetStatusResponsed = false;
bool IsWIFIApCtrlResponsed = false;

//Get the WiFi status from SN8200
void GetStatus(int8u seq)
{
    int8u payload[4];
    payload[0] = WIFI_GET_STATUS_REQ;
    payload[1] = seq;
    payload[2] = 1; // 0 STA ; 1 AP
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

    case WIFI_AP_CTRL_RSP: {
			IsWIFIApCtrlResponsed = true;
			if (WIFI_SUCCESS == buf[2]) {
				if (APOnOff)
					LCD_DisplayStringLine(LINE(2), "AP is ON");
				else
					LCD_DisplayStringLine(LINE(2), "AP is OFF");
			} 
			else
			  LCD_DisplayStringLine(LINE(2), "AP control fail");
			break;
		}

    default:
        break;
	}
}
