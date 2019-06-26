#include "Sensor.h"
uint8 GetADValue(void)
{
    uint8 v = 0;
    ADCCFG = 0x01;
    ADCCON1 = 0x33;
    ADCCON2 = 0xB0;
    ADCCON1 |= 0x40;
    while(!(ADCCON1 & 0x80));
    v = ADCL;
    v = ADCH;
    return(v);
}
// pull: 0 - disable, 1 - pulldown, 2 - pullup
void SetIOInput(uint8 group, uint8 bit, uint8 pull)
{
   switch(group)
   {
   case 0: P0DIR &= ~(1 << bit); P0SEL &= ~(1 << bit); break;
   case 1: P1DIR &= ~(1 << bit); P1SEL &= ~(1 << bit); break;
   case 2: P2DIR &= ~(1 << bit); P2SEL &= ~(1 << bit); break;
   }
}
void SetIOOutput(uint8 group, uint8 bit)
{
   switch(group)
   {
   case 0: P0DIR |= (1 << bit); P0SEL &= ~(1 << bit); break;
   case 1: P1DIR |= (1 << bit); P1SEL &= ~(1 << bit); break;
   case 2: P2DIR |= (1 << bit); P2SEL &= ~(1 << bit); break;
   }
}
uint8 GetIOLevel(uint8 group, uint8 bit)
{
    switch(group)
    {
    case 0: return !!(P0 & (1 << bit));
    case 1: return !!(P1 & (1 << bit));
    case 2: return !!(P2 & (1 << bit));
    }
    return 0;
}
void SetIOLevel(uint8 group, uint8 bit, uint8 value)
{
    switch(group)
    {
    case 0:
        if(value)
          P0 |= (1 << bit);
        else
          P0 &=~(1 << bit);
        break;
    case 1:
        if(value)
          P1 |= (1 << bit);
        else
          P1 &=~(1 << bit);
        break;
    case 2:
        if(value)
          P2 |= (1 << bit);
        else
          P2 &=~(1 << bit);
        break;
    }
}
/****************************************************************
*函数功能 ：初始化串口1										
*入口参数 ：无												
*返 回 值 ：无							
*说    明 ：57600-8-n-1						
****************************************************************/
void initUART_1(void)
{
    CLKCONCMD &= ~0x40;              //晶振
    while(!(SLEEPSTA & 0x40));      //等待晶振稳定
    CLKCONCMD &= ~0x47;             //TICHSPD128分频，CLKSPD不分频
    SLEEPCMD |= 0x04; 		 //关闭不用的RC振荡器
    PERCFG |= 0x02;				//位置1 P0口
    P1SEL |= 0xF0;				//P0用作串口
    P2DIR |= 0X80;                             //P0优先作为串口1
    U1CSR |= 0x80;				//UART方式
    //*********************************************************
    //	CC2530 UART1  BAUD设置
    //*********************************************************
    //设置博特率为 9600
    U1GCR |= 8;				//baud_e
    U1BAUD |= 59;				//波特率设为 9600
    //设置博特率为 38400
    //U1GCR |= 10;				//baud_e
    //U1BAUD |= 59;				//波特率设为 38400
    //设置博特率为 115200
    //U1GCR |= 11;				//baud_e
    //U1BAUD |= 216;				//波特率设为 115200
    UTX1IF = 0;
    U1CSR |= 0X40;				//允许接收
    IEN0 |= 0x88;				//开总中断，接收中断
}

void Uart1TxByte(unsigned char v)
{
    U1DBUF = v;
    while(UTX1IF == 0);
    UTX1IF = 0;
}

/****************************************************************
*函数功能 ：串口发送字符串函数					
*入口参数 : data:数据									
*			len :数据长度							
*返 回 值 ：无											
*说    明 ：				
****************************************************************/
void Uart1TX(unsigned char *Data,unsigned int len)
{
  unsigned int j;
  for(j=0;j<len;j++)
  {
    U1DBUF = *Data++;
    while(UTX1IF == 0);
    UTX1IF = 0;
  }
}

char Uart1RX(void)
{
   char c;
   unsigned char status;
   status = U1CSR;
   U1CSR |= 0x40;
   while (!URX1IF);
   c = U1DBUF;
   URX1IF = 0;
   U1CSR = status;
   return c;
}
