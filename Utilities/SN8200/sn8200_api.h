#ifndef __SN8200_API_H
#define __SN8200_API_H
#include <stdbool.h>
#include <rtl.h>
#include <stdint.h>

#define PORT_NONE 0
#define CMD_ERROR -1
#define SUB_CMD_RESP_MASK 0x80 // Bit 7: 0 for original command, 1 for response 
#define MAX_CONNECTION_PER_SOCK 4   // max connection per listening socket
#define MAX_BUFFER_SIZE 0x800

#define ACK_REQUIRED 1
#define ACK_NOT_REQUIRED 0

typedef  unsigned char   u8_t;
typedef  signed char     s8_t;
typedef  unsigned short  u16_t;
typedef  signed short    s16_t;
typedef  unsigned long   u32_t;
typedef  signed long     s32_t;


#define int8u unsigned char
#define int8s char
#define int16u unsigned short
#define int16s short
#define int32u unsigned int
#define int32s int

void SN8200_API_Init(uint32_t baudrate);
bool SN8200_API_HasInput(void);
void ProcessSN8200Input(void);
#endif
