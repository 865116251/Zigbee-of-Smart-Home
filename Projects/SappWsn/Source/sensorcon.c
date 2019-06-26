
#include "hal_board.h"
#include "hal_defs.h"
#include "hal_mcu.h"
#include "hal_types.h"
#include "sensorcon.h"
#include  "hal_adc.h"

#define		SHT1DATA_HIGH	P1 |= 0x40
#define		SHT1DATA_LOW	P1 &= 0xBF

#define		SHT1SCK_HIGH	P1 |= 0x80
#define		SHT1SCK_LOW	P1 &= 0x7F

#define         SHT1READY       (P1>>6)&0X1
/*****************************************************************************
 函数声明
*****************************************************************************/
void Sensor_PIN_INT(uint8);
uint16 ReadAdcValue(uint8 ChannelNum,uint8 DecimationRate,uint8 RefVoltage);
void SHT1_Reset(void);
void SHT1_Start(void);
void SHT1_SendAck(void);
void SHT1_WriteCommandData(uint8);
uint8 SHT1_ReadData(void);
uint8 SHT1_Ready(void);
void SHT1_WriteReg(uint8);
uint16 SHT1_ReadReg(void);
void SHT1_INT(void);
uint32 ReadSHT1(uint8 Addr);
uint8 ReadTc77(void);
void SET_ADC_IO_SLEEP_MODE(void);
void SET_ADC_IO_ADC_MODE(void);
extern void UartTX_Send_String(uint8 *Data,int len);
uint8 CRC8(uint8 crc, uint8 data);
/*****************************************************************************
 void SET_ADC_IO_SLEEP_MODE(void)

  设置ADC I/O口为低功耗模式.
*****************************************************************************/
void SET_ADC_IO_SLEEP_MODE(void)
{
  P0SEL &= ~(1<<0);    // 
  P0    &= ~(1<<0);
  P0DIR &= ~(1<<0);
  P0INP |= (1<<0);
           
  P0SEL &= ~(1<<1);    // adc关闭
  P0    &= ~(1<<1);
  P0DIR &= ~(1<<1);
  P0INP |= (1<<1);
  
  P0SEL &= ~(1<<6);    // 
  P0    &= ~(1<<6);
  P0DIR &= ~(1<<6);
  P0INP |= (1<<6);
           
  P0SEL &= ~(1<<7);    // 
  P0    &= ~(1<<7);
  P0DIR &= ~(1<<7);
  P0INP |= (1<<7);
           
  APCFG &= ~0xC3;
}

/*函数功能:读出AD口的数据
输入参数:ChannelNum:采集的通道号  0-0xF
                    1000: AIN0–AIN1
                    1001: AIN2–AIN3
                    1010: AIN4–AIN5
                    1011: AIN6–AIN7
                    1100: GND
                    1101: Reserved
                    1110: Temperature sensor
                    1111: VDD/3
         DecimationRate:分辩率  00: 64 decimation rate (7 bits ENOB)
                    01: 128 decimation rate (9 bits ENOB)
                    10: 256 decimation rate (10 bits ENOB)
                    11: 512 decimation rate (12 bits ENOB)
RefVoltage:参考电压:00: Internal reference
                    01: External reference on AIN7 pin
                    10: AVDD5 pin
                    11: External reference on AIN6–AIN7 differential input
返回值:16bit的采集数据
*/
uint16 ReadAdcValue(uint8 ChannelNum,uint8 DecimationRate,uint8 RefVoltage)
{ 
  uint16 AdValue;
  if(ChannelNum == 0xe){//片内温度到ADC_SOC
    TR0 = 1;
    ATEST = 1;
  }
  else{
    TR0 = 0;
    ATEST = 0;
  } 
  ADCCON3 = ChannelNum&0xf;
  ADCCON3 = ADCCON3 | ((DecimationRate&0x3)<<4);
  ADCCON3 = ADCCON3 | ((RefVoltage&0x3)<<6);   
  ADCCON1 = ADCCON1 | (0x3<<4);//ADCCON1.ST = 1时启动
  AdValue = ADCL; //清除EOC 
  AdValue = ADCH; 
  ADCCON1 = ADCCON1 | (0x1<<6);//启动转换
  while(!(ADCCON1&0x80));
  AdValue = ADCH;
  AdValue = (AdValue<<6) + (ADCL>>2);
  ADCCON1 =  ADCCON1 & 0x7f;
  return AdValue;
}
/*****************************************************************************
  void Sensor_PIN_INT(void)

  传感器及ADC I/O口初始化.
*****************************************************************************/
void Sensor_PIN_INT(uint8 SensorType)
{	
  if(SensorType == 1)
    APCFG = (0x1<<1)|(0x1<<4)|(0x1<<5)|(0x1<<6)|(0x1<<7);//P04,P05,P06,P07为ADC口            
  else if(SensorType == 2){
    APCFG = (0x1<<1)|(0x1<<4)|(0x1<<5)|(0x1<<6)|(0x1<<7);//P04,P05,P06,P07为ADC口            
    P1INP &= (~(0x1 | (0x1<<1) | (0x1<<2) | (0x1<<6)));//P1.0,P1.1,P1.2,P1.6上拉 
    P1SEL &= ~((1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7));//P13,P14,P15,P16,P17为GPIO        
    P1DIR |= ((1<<3)|(1<<4)|(1<<5)|(1<<6));//P13,P14,P15,P16为OUTPUT  
    P1DIR &= ~(1<<7);//P17为INPUT              
  }
  else if(SensorType == 3){
    APCFG = ((0x1<<5)|(0x1<<6));//P05,P06为ADC口            
    P1INP &= (~(0x1 | (0x1<<1) | (0x1<<2) | (0x1<<6)));//P1.0,P1.1,P1.2,P1.6上拉 
    P1SEL &= ~((1<<3)|(1<<4)|(1<<5)|(1<<7));//P13,P14,P15,P16,P17为GPIO        
    P1DIR |= ((1<<3)|(1<<4)|(1<<5));//P13,P14,P15为OUTPUT  
    P1DIR &= ~(1<<7);//P17为INPUT              
    
    P1SEL |= (1<<6);//P16为外设功能       
    PICTL &= ~(1<<2); //P14~P17上升沿触发
    P1IEN |= (1<<6);//P16中断使能
    IEN2 |= (1<<4);//P1口中断使能
    P1 &= ~((1<<3)|(1<<4));//00:正,负管均不通  
  }
  else if(SensorType == 5){
    APCFG = ((0x1<<5)|(0x1<<6)|(0x1<<7));//P05,P06,P07为ADC口            
  }
  else if(SensorType == 6){//Motor,Step Motor    
    P1SEL = 0;   //P10,P11,P12,P13,P14,P15,P16,P17为GPIO        
    P1DIR = 0xff;//P10,P11,P12,P13,P14,P15,P16,P17为OUTPUT      
    P0SEL &= ~((1<<4)|(1<<5)|(1<<6));//P04,P05,P06为GPIO    
    P0DIR |= ((1<<4)|(1<<5)|(1<<6)); //P04,P05,P06为OUTPUT
    P0 &= ~((1<<4)|(1<<5)|(1<<6));
    APCFG = (0x1<<1);//P01为ADC口    
  }
  else if(SensorType == 7){//SEG显示,MATRIX LEDs    
    P1SEL = 0;   //P10,P11,P12,P13,P14,P15,P16,P17为GPIO        
    P1DIR = 0xff;//P10,P11,P12,P13,P14,P15,P16,P17为OUTPUT  
    P0SEL &= ~((1<<4)|(1<<5)|(1<<6)|(1<<7));//P04,P05,P06,P07为GPIO    
    P0DIR |= ((1<<4)|(1<<5)|(1<<6)|(1<<7));//P04,P05,P06,P07为OUTPUT        
    P0 &= ~((1<<4)|(1<<5)|(1<<6)|(1<<7));
    APCFG = (0x1<<1);//P01为ADC口    
  }
  else if(SensorType == 8){//传感器类型8  心电P0.5,模拟量；脉搏P0.6,数字量
    P1SEL = 0;   //P10,P11,P12,P13,P14,P15,P16,P17为GPIO        
    P1DIR = 0xff;//P10,P11,P12,P13,P14,P15,P16,P17为OUTPUT  
    P1DIR &= ~(1<<4);//P14为INPUT              
    P0SEL &= ~((1<<4)|(1<<5)|(1<<6)|(1<<7));//P04,P05,P06,P07为GPIO        
    P0DIR &= ~(1<<6);//P0.6为INPUT              
    APCFG = (0x1<<5);//P0.5为ADC口    
  }
  else if(SensorType == 9){//传感器类型9 紫外线P0.4,模拟量；红外线P0.5,数字量；门磁P1.4,数字量
    P1SEL = 0;   //P10,P11,P12,P13,P14,P15,P16,P17为GPIO        
    P1DIR = 0xff;//P10,P11,P12,P13,P14,P15,P16,P17为OUTPUT  
    P1DIR &= ~(1<<4);//P14为INPUT              
    P0SEL &= ~((1<<4)|(1<<5)|(1<<6)|(1<<7));//P04,P05,P06,P07为GPIO        
    P0DIR &= ~(1<<5);//P0.5为INPUT              
    APCFG = (0x1<<4);//P0.4为ADC口    
  }
  else{//SensorType为0等其它情况
    //用于传感器类型的读取
    P2INP &= (~(1<<6)); //P1口上拉使能        
    P1INP &= (~(0x1 | (0x1<<1) | (0x1<<2) | (0x1<<6) | (0x1<<7)));//P1.0,P1.1,P1.2,P1.6,P1.7上拉 
    
    //用于人体感应的读取
    P0SEL &= ~(1<<5);//P05为GPIO    
    P0DIR &= ~(1<<5);//P05为INPUT    
    APCFG = (0x1<1)|(0x1<<4)|(0x1<<6);//P01,P04,P06为ADC口       
   
    //用于温湿度测量
    P1SEL &= ~((1<<6)|(1<<7));//P16,P17为GPIO        
    P1DIR |= (1<<6)|(1<<7);//P16,P17为OUTPUT    
    
    P1SEL |= (1<<3);//P13为外设功能       
    PICTL &= ~(1<<1); //P10~P13上升沿触发
    P1IEN |= (1<<3);//P13中断使能
    IEN2 |= (1<<4);//P1口中断使能
  }  
}

//当uC和SHT10通信中断时,复位通信口
void SHT1_Reset(void)
{
  uint8 i;
  
  SHT1DATA_HIGH;
  for(i=0;i<10;i++){
    SHT1SCK_LOW;
    SHT1SCK_HIGH;    
  }  
}

//传输启始信号
void SHT1_Start(void)
{
  SHT1DATA_HIGH;
  SHT1SCK_HIGH;
  SHT1DATA_LOW;
  SHT1SCK_LOW;
  SHT1SCK_HIGH;
  SHT1DATA_HIGH;  
}

//为0时,写命令正确;为1时错误
uint8 SHT1_Ready(void)
{  
  //读应答信号
  SHT1DATA_HIGH;
  return(SHT1READY);
}

void SHT1_SendAck(void)
{
  SHT1SCK_HIGH;
  SHT1SCK_LOW;    
}
//为0时,写命令正确;为1时错误
void SHT1_WriteCommandData(uint8 bCommand)
{
  uint8 i;  
  SHT1SCK_LOW;
  for(i=0;i<8;i++){
    if(bCommand&(0x1<<(7-i)))
      SHT1DATA_HIGH;
    else
      SHT1DATA_LOW;
    SHT1SCK_HIGH;
    SHT1SCK_LOW;     
  }
}

//读一个字节的数据
uint8 SHT1_ReadData(void)
{
  uint8 i,bResult;
  bResult = 0;
  SHT1DATA_HIGH;
  for(i=0;i<8;i++){      
    if(SHT1_Ready() != 0)
      bResult |= (0x1<<(7-i));     
    SHT1SCK_HIGH;
    SHT1SCK_LOW; 
  }
  return bResult;
}

//写状态寄存器
void SHT1_WriteReg(uint8 Value)
{
  while(1){    
    SHT1_Start();  
    SHT1_WriteCommandData(6);
    if(SHT1_Ready() != 0){  //无应答
      SHT1_Reset();  
      continue;
    }
    else
      SHT1_SendAck();    
    SHT1_WriteCommandData(Value);
    if(SHT1_Ready() != 0){  //无应答
      SHT1_Reset(); 
      continue;
    }
    else
      SHT1_SendAck();   
    break;
  }   
}

uint16 SHT1_ReadReg(void)
{
  uint16 lResult;
  while(1){     
    SHT1_Start();  
    SHT1_WriteCommandData(7);
    if(SHT1_Ready() != 0){  //无应答
      SHT1_Reset();        
      continue;      
    }
    else{
      SHT1_SendAck();
      break;
    }    
  } 
  
  lResult = (SHT1_ReadData()<<8);  
  SHT1DATA_LOW;
  SHT1_SendAck();
      
  lResult |= SHT1_ReadData(); 
  SHT1DATA_HIGH;
  SHT1_SendAck();
    
  return lResult;
}

/*
void SHT1_INT(void)
{
    
}*/

uint32 ReadSHT1(uint8 Addr)
{
  uint32 lResult;
  while(1){     
    SHT1_Start();  
    SHT1_WriteCommandData(Addr);
    if(SHT1_Ready() != 0){  //无应答
      SHT1_Reset();  
      continue;      
    }
    else{
      SHT1_SendAck();
      break;
    }    
  }
  
  while(SHT1_Ready() == 1);
  lResult = SHT1_ReadData();  
  lResult = lResult<<16;
  SHT1DATA_LOW;
  SHT1_SendAck();
      
  lResult |= ((uint16)SHT1_ReadData()<<8); 
  SHT1DATA_LOW;
  SHT1_SendAck();
    
  lResult |= SHT1_ReadData(); 
  SHT1DATA_HIGH;
  SHT1_SendAck();
  
  return lResult;
}

uint8 CRC8(uint8 crc, uint8 data)
{
  uint8 i;
  crc = crc ^data;
  for (i = 0; i < 8; i++)
  {
     if ((crc & 0x01) != 0) crc = (crc >> 1) ^ 0x8c;
     else crc = crc >> 1;
  }
  return crc;
}
/*
unsigned CHAR code CCITT_CRC8[256] = 
{
    0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83,
    0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
    0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e,
    0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
    0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0,
    0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
    0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d,
    0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
    0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5,
    0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
    0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58,
    0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
    0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6,
    0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
    0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b,
    0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
    0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f,
    0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
    0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92,
    0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
    0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c,
    0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
    0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1,
    0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
    0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49,
    0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
    0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4,
    0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
    0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a,
    0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
    0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7,
    0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35
};
*/
/*interrupt
OFFSET   EQU    0x800

                COMMON  INTVEC(1)
                ;
                ; the interrupt vectors in the boot code simply jump to the user's LJMP
                ; instruction based at the known offset of 0x800 (flash page 1).
                ;
                ; the boot code does not use interrupts.

   ORG     0x0003      ; RF Tx FIFO underflow and overflow
   LJMP  $ + OFFSET
   ORG     0x000B      ; ADC end of conversion
   LJMP  $ + OFFSET
   ORG     0x0013      ; USART0 Rx complete
   LJMP  $ + OFFSET
   ORG     0x001B      ; USART1 Rx complete
   LJMP  $ + OFFSET
   ORG     0x0023      ; AES encryption/decryption complete
   LJMP  $ + OFFSET
   ORG     0x002B      ; Sleep timer compare
   LJMP  $ + OFFSET
   ORG     0x0033      ; Port 2 inputs
   LJMP  $ + OFFSET
   ORG     0x003B      ; USART0 Tx complete
   LJMP  $ + OFFSET
   ORG     0x0043      ; DMA transfer complete
   LJMP  $ + OFFSET
   ORG     0x004B      ; Timer 1 (16-bit) capture/compare/overflow
   LJMP  $ + OFFSET
   ORG     0x0053      ; Timer 2 (MAC timer)
   LJMP  $ + OFFSET
   ORG     0x005B      ; Timer 3 (8-bit) capture/compare/overflow
   LJMP  $ + OFFSET
   ORG     0x0063      ; Timer 4 (8-bit) capture/compare/overflow
   LJMP  $ + OFFSET
   ORG     0x006B      ; Port 0 inputs
   LJMP  $ + OFFSET
   ORG     0x0073      ; USART1 Tx complete
   LJMP  $ + OFFSET
   ORG     0x007B      ; Port 1 inputs
   LJMP  $ + OFFSET
   ORG     0x0083      ; RF general interrupts
   LJMP  $ + OFFSET
   ORG     0x008B      ; Watchdog overflow in timer mode
   LJMP  $ + OFFSET
*/