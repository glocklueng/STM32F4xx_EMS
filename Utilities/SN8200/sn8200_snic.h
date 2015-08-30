#ifndef __SN8200_SNIC_H
#define __SN8200_SNIC_H


void setTCPinfo(void);
void setUDPinfo(void);

void SnicInit(int8u seq);
void SnicCleanup(int8u seq);
void SnicIPConfig(int8u seq);
void SnicGetDhcp(int8u seq);

int tcpCreateSocket(uint8_t bindOption, uint32_t localIp, uint16_t port, uint8_t seq, int8u ssl);
int closeSocket(uint8_t shortSocket, uint8_t seq);
int sendFromSock(int8u shortSocket, int8u * sendBuf, int16u len, int8u timeout, int8u seq);
int tcpCreateConnection(int8u shortSock, int16u size, int8u maxClient, int8u seq);

void handleRxSNIC(uint8_t* buf, int len);

#endif
