#include "sn8200_core.h"
#include "sn8200_api.h"

int32u timeout = 10000;

/*
extern int buf_pos;
extern unsigned char RxBuffer[];
extern int ack_recv_status;
*/

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
