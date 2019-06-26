#ifndef _LIGHT_
#define _LIGHT_
#ifndef _IOCC2530_
#define _IOCC2530_
#include "ioCC2530.h"
#endif

#define uint8 unsigned char
#define uint16 unsigned short int
#define uint32 unsigned long int

uint8 GetADValue(void);
// pull: 0 - disable, 1 - pulldown, 2 - pullup
#define PULL_NONE 0
#define PULL_DOWN 1
#define PULL_UP   2
void SetIOInput(uint8 group, uint8 bit, uint8 pull);
void SetIOOutput(uint8 group, uint8 bit);
uint8 GetIOLevel(uint8 group, uint8 bit);
void SetIOLevel(uint8 group, uint8 bit, uint8 value);
#define SafetyInit()    SetIOInput(1, 0, PULL_DOWN)
#define SafetyInfo()    GetIOLevel(1, 0)
#define SmokeInit()     SetIOInput(0, 0, PULL_DOWN)
#define SmokeInfo()     GetIOLevel(0, 0)
#define GasInit()       SetIOInput(0, 0, PULL_DOWN)
#define GasInfo()       GetIOLevel(0, 0)
#define ControlInit()   do { SetIOOutput(1,4);SetIOOutput(1,5);SetIOOutput(1,6);SetIOOutput(1,7);Control(0); } while(0)
#define Control(mask)   do { SetIOLevel(1,4,mask&0x01);SetIOLevel(1,5,mask&0x02);SetIOLevel(1,6,mask&0x04);SetIOLevel(1,7,mask&0x08); } while(0)
void initUART_1(void);
void Uart1TxByte(unsigned char v);
void Uart1TX(unsigned char *Data, unsigned int len);
char Uart1RX(void);
char VoiceInfo(void);
extern uint16 ReadAdcValue(uint8 ChannelNum,uint8 DecimationRate,uint8 RefVoltage);
extern void Sensor_PIN_INT(uint8);
extern uint32 ReadSHT1(uint8 Addr);
extern uint32 macMcuPrecisionCount(void);
#endif
