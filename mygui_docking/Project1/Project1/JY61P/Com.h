
#ifndef __UART_NET_H
#define __UART_NET_H
//#include <tchar.h>
#include <stdint.h>
//#include "JY61P/REG.h"
signed char	SendUARTMessageLength(const char chrMessage[],const unsigned short usLen);
signed char SetBaundrate(const unsigned long ulBaundrate);
signed char OpenCOMDevice(const unsigned long ulPortNo,const unsigned long ulBaundrate);
void CloseCOMDevice(void);
void CopeSensorData(uint32_t uiReg, uint32_t uiRegNum);
void AutoScanSensor(void);
void DelayMs(uint16_t ms);
void ComRxCallBack(char* p_data, UINT32 uiSize);
#endif
