#include "SAPP_Device.h"
#include "Sensor.h"
#include <string.h>
void SendASignal(void);
uint16 WaterFlowCount;
uint8 RfidStatus = 0;
uint8 rxBytePtr = 0;
uint8 rxByte[8];
const unsigned char seg7table[16] = {
    /* 0       1       2       3       4       5       6      7*/
    0xc0,   0xf9,   0xa4,   0xb0,   0x99,   0x92,   0x82,   0xf8,
    /* 8       9      A        B       C       D       E      F*/
    0x80,   0x90,   0x88,   0x83,   0xc6,   0xa1,   0x86,   0x8e };

/**************************************************************/
/* 传感器列表                                                 */
/**************************************************************/
/********************************/
/* 燃气传感器                   */
/********************************/
#if !defined(ZDO_COORDINATOR)    
    bool fLibrate = 0;//1:有振动;0:无振动
__interrupt void P1_ISR(void);    
#pragma vector = 0x007B 
__interrupt void P1_ISR(void)
{ 
#if defined(SENSORBOARD0)     
    if(P1IFG & (0x1<<3))
        fLibrate = 1;    
#endif    
#if defined(SENSORBOARD3)
    if(P1IFG & (0x1<<6))
        WaterFlowCount++;
#endif    
    P1IFG = 0;
    P1IF = 0;
}
#endif
//传感器板0处理
#if defined(SENSORBOARD0)
void SensorBd0ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void SensorBd0ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        Sensor_PIN_INT(0);
    }
}
void SensorBd0Timeout(struct ep_info_t *ep);
void SensorBd0Timeout(struct ep_info_t *ep)
{       
    uint8 SendBuf[12];
    uint16 AdValue;  
    float RHTValue;
    uint32 lTemp;
    uint16 ParentShortAddr;
    ParentShortAddr = NLME_GetCoordShortAddr();
    SendBuf[0] = (unsigned char)(ParentShortAddr);
    SendBuf[1] = (unsigned char)(ParentShortAddr>>8);
#if defined(RTR_NWK)
    SendBuf[2] = 0x40 | 0;
#else
    SendBuf[2] = 0x80 | 0;
#endif
    AdValue = ReadAdcValue(0x1,3,2);
    SendBuf[3] = (uint8)(AdValue>>6);//A/D采集                                                                                       
    AdValue = ReadAdcValue(0xe,3,2);   
    RHTValue = AdValue;
    RHTValue = RHTValue /1480 * 25;    
    SendBuf[4] = (uint8)(RHTValue);//主板温度   
    
    AdValue = ReadAdcValue(0x6,3,2);
    AdValue = AdValue>>6;
    RHTValue = AdValue;
    RHTValue = 330*RHTValue/128-50;
    SendBuf[5] = (uint8)RHTValue;//传感器板温度   
                  
    //SHT1_WriteReg(0x1);
    //Temp = SHT1_ReadReg();                            
    lTemp = ReadSHT1(3);//14bit温度
    lTemp = lTemp >> 8;
    RHTValue = lTemp;
    RHTValue = 0.01 * RHTValue - 39.64;                   
    SendBuf[6] = (uint8)RHTValue;//温湿度传感器温度  
                  
    lTemp = ReadSHT1(5);//12bit湿度                  
    lTemp = lTemp >> 8;
    RHTValue = lTemp;
    RHTValue = 0.0405 * RHTValue -4 - 2.8*RHTValue*RHTValue/1000000;                                                      
    SendBuf[7] = (uint8)RHTValue;//温湿度传感器湿度 
                  
    AdValue = ReadAdcValue(0x4,3,2);
    SendBuf[8] = (uint8)(AdValue>>6);//光照
                  
    SendBuf[9] = (uint8)fLibrate;//振动
    fLibrate = 0;
    SendBuf[10] = (P0>>5)&0x1;//人体感应                             	
    SendData(ep->ep, &SendBuf[0], 0x0000, TRANSFER_ENDPOINT, 11);
}   
#endif

//传感器板1处理
#if defined(SENSORBOARD1)
void SensorBd1ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void SensorBd1ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        Sensor_PIN_INT(1);
    }
}
void SensorBd1Timeout(struct ep_info_t *ep);
void SensorBd1Timeout(struct ep_info_t *ep)
{       
    uint8 SendBuf[12];
    uint16 AdValue;  
    float RHTValue;
    uint16 ParentShortAddr;
    ParentShortAddr = NLME_GetCoordShortAddr();
    SendBuf[0] = (unsigned char)(ParentShortAddr);
    SendBuf[1] = (unsigned char)(ParentShortAddr>>8);
#if defined(RTR_NWK)
    SendBuf[2] = 0x40 | 1;
#else
    SendBuf[2] = 0x80 | 1;
#endif
    AdValue = ReadAdcValue(0x1,3,2);
    SendBuf[3] = (uint8)(AdValue>>6);//A/D采集                                                                                       
    AdValue = ReadAdcValue(0xe,3,2);   
    RHTValue = AdValue;
    RHTValue = RHTValue /1480 * 25;    
    SendBuf[4] = (uint8)(RHTValue);//主板温度   
    
    AdValue = ReadAdcValue(0x4,3,2);
    AdValue = AdValue>>6;
    SendBuf[5] = (uint8)AdValue;//烟雾   
                  
    AdValue = ReadAdcValue(0x5,3,2);
    AdValue = AdValue>>6;
    SendBuf[6] = (uint8)AdValue;//酒精   
                  
    AdValue = ReadAdcValue(0x6,3,2);
    AdValue = AdValue>>6;
    SendBuf[7] = (uint8)AdValue;//压力   
               
    AdValue = ReadAdcValue(0x7,3,2);
    AdValue = AdValue>>6;
    SendBuf[8] = (uint8)AdValue;//气压     	
    SendData(ep->ep, &SendBuf[0], 0x0000, TRANSFER_ENDPOINT, 9);
}   
#endif

//传感器板2处理
#if defined(SENSORBOARD2)
uint32 C320us;
uint32 temp;
void SensorBd2ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void SensorBd2ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        Sensor_PIN_INT(2);
    }
}
void SensorBd2Timeout(struct ep_info_t *ep);
void SensorBd2Timeout(struct ep_info_t *ep)
{       
    uint8 SendBuf[12];
    uint16 AdValue;  
    float RHTValue;
    uint16 ParentShortAddr;
    ParentShortAddr = NLME_GetCoordShortAddr();
    SendBuf[0] = (unsigned char)(ParentShortAddr);
    SendBuf[1] = (unsigned char)(ParentShortAddr>>8);
#if defined(RTR_NWK)
    SendBuf[2] = 0x40 | 2;
#else
    SendBuf[2] = 0x80 | 2;
#endif
    AdValue = ReadAdcValue(0x1,3,2);
    SendBuf[3] = (uint8)(AdValue>>6);//A/D采集                                                                                       
    AdValue = ReadAdcValue(0xe,3,2);   
    RHTValue = AdValue;
    RHTValue = RHTValue /1480 * 25;    
    SendBuf[4] = (uint8)(RHTValue);//主板温度   
    
    SendASignal();                  
    while(!(P1 & 0x80));                                                                                 
    C320us = macMcuPrecisionCount();//高电平开始时间                 
    while(P1 & 0x80);                                                                                       
    temp = macMcuPrecisionCount() - C320us;//高电平宽度,单位为320us                                            
    SendBuf[5] = (uint8)(temp&0xff);    
                  
    AdValue = ReadAdcValue(0x4,3,2);//三轴加速度
    AdValue = AdValue>>6;
    SendBuf[6] = (uint8)AdValue;
                  
    AdValue = ReadAdcValue(0x5,3,2);
    AdValue = AdValue>>6;
    SendBuf[7] = (uint8)AdValue;
                 
    AdValue = ReadAdcValue(0x6,3,2);
    AdValue = AdValue>>6;
    SendBuf[8] = (uint8)AdValue;
            
    SendData(ep->ep, &SendBuf[0], 0x0000, TRANSFER_ENDPOINT, 9);
}   
#endif

//传感器板3处理
#if defined(SENSORBOARD3)
void SensorBd3ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void SensorBd3ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        Sensor_PIN_INT(3);
    }
}
void SensorBd3Timeout(struct ep_info_t *ep);
void SensorBd3Timeout(struct ep_info_t *ep)
{      
    uint8 SendBuf[12];
    uint16 AdValue;  
    float RHTValue;
    uint16 ParentShortAddr;
    ParentShortAddr = NLME_GetCoordShortAddr();
    SendBuf[0] = (unsigned char)(ParentShortAddr);
    SendBuf[1] = (unsigned char)(ParentShortAddr>>8);
#if defined(RTR_NWK)
    SendBuf[2] = 0x40 | 3;
#else
    SendBuf[2] = 0x80 | 3;
#endif
    AdValue = ReadAdcValue(0x1,3,2);
    SendBuf[3] = (uint8)(AdValue>>6);//A/D采集                                                                                       
    AdValue = ReadAdcValue(0xe,3,2);   
    RHTValue = AdValue;
    RHTValue = RHTValue /1480 * 25;    
    SendBuf[4] = (uint8)(RHTValue);//主板温度   
    
    if(P1 & 0x80)//霍尔传感器
        SendBuf[5] = 0;  
    else
        SendBuf[5] = 1;  
    AdValue = ReadAdcValue(0x6,3,2);//雨滴传感器
    AdValue = AdValue>>6;
    SendBuf[6] = (uint8)AdValue;
    RHTValue = WaterFlowCount/3/*(SAMPLEAPP_RUN_TIMEOUT/1000)*/;//水流量
    SendBuf[7] = (uint8)RHTValue;
    WaterFlowCount = 0;
    P1 &= ~((1<<3)|(1<<4));//00:正,负管均不通         	
    SendData(ep->ep, &SendBuf[0], 0x0000, TRANSFER_ENDPOINT, 8);   
}   
#endif

//传感器板4处理
#if defined(SENSORBOARD4)
void SensorBd4ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void SensorBd4ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        Sensor_PIN_INT(4);
    }
}
void SensorBd4Timeout(struct ep_info_t *ep);
void SensorBd4Timeout(struct ep_info_t *ep)
{      
    uint8 SendBuf[12];    
    SendBuf[0] = 0x2;
    SendBuf[1] = 0x2;
    SendBuf[2] = 0x26;
    HalUARTWrite(HAL_UART_PORT_0, &SendBuf[0], 3);//发送寻卡命令                   
    RfidStatus = 1;//等待寻卡应答状态
    rxBytePtr = 0;          
}
#endif

//传感器板5处理
#if defined(SENSORBOARD5)
void SensorBd5ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void SensorBd5ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        Sensor_PIN_INT(5);
    }
}
void SensorBd5Timeout(struct ep_info_t *ep);
void SensorBd5Timeout(struct ep_info_t *ep)
{      
    uint8 SendBuf[12];
    uint16 AdValue;  
    float RHTValue;
    uint16 ParentShortAddr;
    ParentShortAddr = NLME_GetCoordShortAddr();
    SendBuf[0] = (unsigned char)(ParentShortAddr);
    SendBuf[1] = (unsigned char)(ParentShortAddr>>8);
#if defined(RTR_NWK)
    SendBuf[2] = 0x40 | 5;
#else
    SendBuf[2] = 0x80 | 5;
#endif
    AdValue = ReadAdcValue(0x1,3,2);
    SendBuf[3] = (uint8)(AdValue>>6);//A/D采集                                                                                       
    AdValue = ReadAdcValue(0xe,3,2);   
    RHTValue = AdValue;
    RHTValue = RHTValue /1480 * 25;    
    SendBuf[4] = (uint8)(RHTValue);//主板温度 
    
    AdValue = ReadAdcValue(0x7,3,2);  //CO、CO2、甲醛  
    AdValue = AdValue>>4;            
    SendBuf[5] = (uint8)(AdValue&0xff);
                  
    AdValue = ReadAdcValue(0x6,3,2);
    AdValue = AdValue>>4;            
    SendBuf[6] = (uint8)(AdValue&0xff);
                              
    AdValue = ReadAdcValue(0x5,3,2);
    AdValue = AdValue>>4;            
    SendBuf[7] = (uint8)(AdValue&0xff);   
    	
    SendData(ep->ep, &SendBuf[0], 0x0000, TRANSFER_ENDPOINT, 8);  
}
#endif

//传感器板6处理
#if defined(SENSORBOARD6)
uint8 DataValid;
bool bStepEnable = 0;//步进电机启动或停止 0--停止；1--启动
bool bStepDirect = 0;//步进电机方向，0--正向；1--反向
uint8 bStepSpeed = 10;
bool fStepDelay;
uint8 cStepDelay;

void SensorBd6ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void SensorBd6ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        Sensor_PIN_INT(6);
    }
}
void SensorBd6Timeout(struct ep_info_t *ep);
void SensorBd6Timeout(struct ep_info_t *ep)
{      
    uint8 SendBuf[12];
    uint16 AdValue;  
    float RHTValue;
    uint16 ParentShortAddr;
    ParentShortAddr = NLME_GetCoordShortAddr();
    SendBuf[0] = (unsigned char)(ParentShortAddr);
    SendBuf[1] = (unsigned char)(ParentShortAddr>>8);
#if defined(RTR_NWK)
    SendBuf[2] = 0x40 | 6;
#else
    SendBuf[2] = 0x80 | 6;
#endif
    AdValue = ReadAdcValue(0x1,3,2);
    SendBuf[3] = (uint8)(AdValue>>6);//A/D采集                                                                                       
    AdValue = ReadAdcValue(0xe,3,2);   
    RHTValue = AdValue;
    RHTValue = RHTValue /1480 * 25;    
    SendBuf[4] = (uint8)(RHTValue);//主板温度         	
    SendData(ep->ep, &SendBuf[0], 0x0000, TRANSFER_ENDPOINT, 5);  
}
void outputSensorBd6(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg);
void outputSensorBd6(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg)
{
  HalUARTWrite(HAL_UART_PORT_0, &msg->Data[0], 3);   
  HalUARTWrite(HAL_UART_PORT_0, "get", 3);
    if(msg->Data[0] == 0x6){//传感器6 电机
        DataValid = msg->Data[1];
        if( (DataValid>=1) && (DataValid<=6) ){                                          
            if(DataValid == 1){//停止
                bStepEnable = 0;
            }
            else if(DataValid == 2){//启动
               	bStepEnable = 1;
               	cStepDelay = bStepSpeed*10;
               	fStepDelay = 0;               
            }
            else if(DataValid == 3)//正向
               	bStepDirect = 0;
            else if(DataValid == 4)//反向
               	bStepDirect = 1;
            else if(DataValid == 5){//加速
               	if(bStepSpeed >1){                  
            	    bStepSpeed--;
                    cStepDelay = bStepSpeed*10;
                    fStepDelay = 0;
                }
            }
            else if(DataValid == 6){//减速
                if(bStepSpeed <20){                  
                    bStepSpeed++;
                    cStepDelay = bStepSpeed*10;
                    fStepDelay = 0;
                }
            }              
        }
        DataValid = msg->Data[2];
        if(DataValid == 1)//模拟电机停止
            P0 &= (~(0x1 << 6));
        if(DataValid == 2)//模拟电机启动
            P0 |= (0x1 << 6);            
    }      
}
#endif

//传感器板7处理
#if defined(SENSORBOARD7)
uint8 DataValid;
uint8 DataValue;
uint16 cSound = 0;
uint8 MatrixLed[8];
uint8 i;
void SensorBd7ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void SensorBd7ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        Sensor_PIN_INT(7);
    }
}
void SensorBd7Timeout(struct ep_info_t *ep);
void SensorBd7Timeout(struct ep_info_t *ep)
{      
    uint8 SendBuf[12];
    uint16 AdValue;  
    float RHTValue;
    uint16 ParentShortAddr;
    ParentShortAddr = NLME_GetCoordShortAddr();
    SendBuf[0] = (unsigned char)(ParentShortAddr);
    SendBuf[1] = (unsigned char)(ParentShortAddr>>8);
#if defined(RTR_NWK)
    SendBuf[2] = 0x40 | 7;
#else
    SendBuf[2] = 0x80 | 7;
#endif
    AdValue = ReadAdcValue(0x1,3,2);
    SendBuf[3] = (uint8)(AdValue>>6);//A/D采集                                                                                       
    AdValue = ReadAdcValue(0xe,3,2);   
    RHTValue = AdValue;
    RHTValue = RHTValue /1480 * 25;    
    SendBuf[4] = (uint8)(RHTValue);//主板温度         	
    SendData(ep->ep, &SendBuf[0], 0x0000, TRANSFER_ENDPOINT, 5);  
}
void outputSensorBd7(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg);
void outputSensorBd7(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg)
{    
    HalUARTWrite(HAL_UART_PORT_0, &msg->Data[0], 12);   
    if(msg->Data[0] == 0x7){//传感器7 显示
        DataValid = msg->Data[1];
        if(DataValid&0x1){//SEG7 Display
            P0 |= (0x1<<4);  
            DataValue = (msg->Data[2])&0xf;
            P1 =  seg7table[DataValue];
            P0 &= ~(0x1<<4);  
        }            
        else if(DataValid&0x2){//蜂鸣器
            cSound = (msg->Data[3]);                  
            cSound = cSound * 50;
        }
        else if(DataValid&0x4){//矩阵LED
            for(i=0;i<8;i++)
                MatrixLed[i]= msg->Data[4+i];                
        }
    }           
}
#endif

//传感器板8处理
#if defined(SENSORBOARD8)
void SensorBd8ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void SensorBd8ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        Sensor_PIN_INT(8);
    }
}
void SensorBd8Timeout(struct ep_info_t *ep);
void SensorBd8Timeout(struct ep_info_t *ep)
{      
    uint8 SendBuf[12];
    uint16 AdValue;  
    float RHTValue;
    uint16 ParentShortAddr;
    ParentShortAddr = NLME_GetCoordShortAddr();
    SendBuf[0] = (unsigned char)(ParentShortAddr);
    SendBuf[1] = (unsigned char)(ParentShortAddr>>8);
#if defined(RTR_NWK)
    SendBuf[2] = 0x40 | 8;
#else
    SendBuf[2] = 0x80 | 8;
#endif
    AdValue = ReadAdcValue(0x1,3,2);
    SendBuf[3] = (uint8)(AdValue>>6);//A/D采集                                                                                       
    AdValue = ReadAdcValue(0xe,3,2);   
    RHTValue = AdValue;
    RHTValue = RHTValue /1480 * 25;    
    SendBuf[4] = (uint8)(RHTValue);//主板温度 
       
    SendData(ep->ep, &SendBuf[0], 0x0000, TRANSFER_ENDPOINT, 5);  
}
#endif
//传感器板9处理
#if defined(SENSORBOARD9)
void SensorBd9ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void SensorBd9ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        Sensor_PIN_INT(9);
    }
}
void SensorBd9Timeout(struct ep_info_t *ep);
void SensorBd9Timeout(struct ep_info_t *ep)
{      
    uint8 SendBuf[12];
    uint16 AdValue;  
    float RHTValue;
    uint16 ParentShortAddr;
    ParentShortAddr = NLME_GetCoordShortAddr();
    SendBuf[0] = (unsigned char)(ParentShortAddr);
    SendBuf[1] = (unsigned char)(ParentShortAddr>>8);
#if defined(RTR_NWK)
    SendBuf[2] = 0x40 | 9;
#else
    SendBuf[2] = 0x80 | 9;
#endif
    AdValue = ReadAdcValue(0x1,3,2);
    SendBuf[3] = (uint8)(AdValue>>6);//A/D采集                                                                                       
    AdValue = ReadAdcValue(0xe,3,2);   
    RHTValue = AdValue;
    RHTValue = RHTValue /1480 * 25;    
    SendBuf[4] = (uint8)(RHTValue);//主板温度 
    
    AdValue = ReadAdcValue(0x4,3,2);    
    SendBuf[5] = (uint8)(AdValue&0xff);
    AdValue = AdValue>>8;            
    SendBuf[6] = (uint8)(AdValue&0xff);
    SendBuf[7] = (uint8)P0_5;
    SendBuf[8] = (uint8)P1_4; 
    SendData(ep->ep, &SendBuf[0], 0x0000, TRANSFER_ENDPOINT, 9);  
}
#endif


/********************************/
/* 二进制执行器传感器           */
/********************************/
#if defined(HAS_EXECUTEB)
void OutputExecuteBResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void OutputExecuteBResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
        ControlInit();
}
void outputExecuteB(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg);
void outputExecuteB(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg)
{
    //msg->Data[], msg->DataLength, msg->TransSeqNumber
    Control(msg->Data[0]);
    SendData(ep->ep, &msg->Data[0], 0x0000, TRANSFER_ENDPOINT, 1);
}
void outputExecuteBTimeout(struct ep_info_t *ep);
void outputExecuteBTimeout(struct ep_info_t *ep)
{
    uint8 value = P1 >> 4;
    SendData(ep->ep, &value, 0x0000, TRANSFER_ENDPOINT, sizeof(value));
}
#endif
/********************************/
/* 模拟执行器传感器             */
/********************************/
#if defined(HAS_EXECUTEA)
void outputExecuteA(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg);
void outputExecuteA(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg)
{
}
#endif
/********************************/
/* 遥控器传感器                 */
/********************************/
#if defined(HAS_REMOTER)
void outputRemoter(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg);
void outputRemoter(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg)
{
}
#endif
/********************************/
/* 测试代码                     */
/********************************/
#if defined(HAS_TESTFUNCTION)
void testFunc_NwkStateChanged(struct ep_info_t *ep);
void testFunc_NwkStateChanged(struct ep_info_t *ep)
{
}
void testFunc_inComeData(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg);
void testFunc_inComeData(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg)
{
    //msg->Data[], msg->DataLength, msg->TransSeqNumber
}
void testFunc_TimeOut(struct ep_info_t *ep);
void testFunc_TimeOut(struct ep_info_t *ep)
{
    // send sensor data to coordinator
    //    SampleApp_SendPeriodicMessage(task_id);
    uint8 sendBuf[] = "(1 1)\r\n";
    sendBuf[1] = ep->ep + '0';
    sendBuf[3] = ep->function.type + '0';
    SendData(ep->ep, sendBuf, 0x0000, TRANSFER_ENDPOINT, 7);
}
void testFunc_ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void testFunc_ResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    switch(type)
    {
    case ResInit:
        // 在这里可以做需要在初始化阶段做的事情
        break;
    }
}
#endif

struct ep_info_t funcList[] = {
#if defined(SENSORBOARD0)
    {
        //stat,income,timeout,resource
        NULL, NULL, SensorBd0Timeout, SensorBd0ResAvailable,
        { DevSensorBd0, 0, 5 },                   // type, id, refresh cycle
    },
#endif
#if defined(SENSORBOARD1)
    {
        //stat,income,timeout,resource
        NULL, NULL, SensorBd1Timeout, SensorBd1ResAvailable,
        { DevSensorBd1, 0, 2 },                   // type, id, refresh cycle
    },
#endif
#if defined(SENSORBOARD2)
    {
        //stat,income,timeout,resource
        NULL, NULL, SensorBd2Timeout, SensorBd2ResAvailable,
        { DevSensorBd2, 0, 1 },                   // type, id, refresh cycle
    },
#endif
#if defined(SENSORBOARD3)
    {
        //stat,income,timeout,resource
        NULL, NULL, SensorBd3Timeout, SensorBd3ResAvailable,
        { DevSensorBd3, 0, 3 },                   // type, id, refresh cycle
    },
#endif
#if defined(SENSORBOARD4)
    {
        //stat,income,timeout,resource
        NULL, NULL, SensorBd4Timeout, SensorBd4ResAvailable,
        { DevSensorBd4, 0, 3 },                   // type, id, refresh cycle
    },
#endif
#if defined(SENSORBOARD5)
    {
        //stat,income,timeout,resource
        NULL, NULL, SensorBd5Timeout, SensorBd5ResAvailable,
        { DevSensorBd5, 0, 3 },                   // type, id, refresh cycle
    },
#endif
#if defined(SENSORBOARD6)
    {
        //stat,income,timeout,resource
        NULL, outputSensorBd6, SensorBd6Timeout, SensorBd6ResAvailable,
        { DevSensorBd6, 0, 5 },                   // type, id, refresh cycle
    },
#endif
#if defined(SENSORBOARD7)
    {
        //stat,income,timeout,resource
        NULL, outputSensorBd7, SensorBd7Timeout, SensorBd7ResAvailable,
        { DevSensorBd7, 0, 3 },                   // type, id, refresh cycle
    },
#endif
#if defined(SENSORBOARD8)
    {
        //stat,income,timeout,resource
        NULL, NULL, SensorBd8Timeout, SensorBd8ResAvailable,
        { DevSensorBd8, 0, 3 },                   // type, id, refresh cycle
    },
#endif
#if defined(SENSORBOARD9)
    {
        //stat,income,timeout,resource
        NULL, NULL, SensorBd9Timeout, SensorBd9ResAvailable,
        { DevSensorBd9, 0, 3 },                   // type, id, refresh cycle
    },
#endif
#if defined(ZDO_COORDINATOR)
    {   // 协调器
        CoordinatorNwkStateChangeRoutine,
        CoordinatorIncomingRoutine,
        CoordinatorTimeoutRoutine,
        CoordinatorResAvailableRoutine,
        { DevCoordinator, 0, 0 },
    },
/***************************************************/
/* 下面这一段针对路由器, 不需要修改                */
/***************************************************/
#elif defined(RTR_NWK) || defined(PEER_ROUTER) || defined(LIGHT)
    {   // 路由器
        RouterNwkStateChangeRoutine,
        RouterIncomingRoutine,
        RouterTimeoutRoutine,
        RouterResAvailableRoutine,
        { DevRouter, 0, 30 },
    },
#endif
};
void SendASignal(void)
{
    uint8 i;
    P1 &= ~(1<<6);                  
    P1 |= (1<<6);
    for(i=0;i<33;i++){
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
    }
    P1 &= ~(1<<6);
} 
//不能修改下面的内容!!!
const uint8 funcCount = sizeof(funcList) / sizeof(funcList[0]);
