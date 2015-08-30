#ifndef __SN8200_WIFI_H
#define __SN8200_WIFI_H

void GetStatus(int8u seq);
void WifiOn(int8u seq);
void WifiOff(int8u seq);
void ApOnOff(int8u OnOff, int8u seq);
//void WifiJoin(int8u seq);
void WifiDisconn(int8u seq);

void handleRxWiFi(int8u* buf, int len);

#endif
