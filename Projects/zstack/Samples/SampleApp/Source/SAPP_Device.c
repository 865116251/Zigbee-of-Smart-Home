#include "SAPP_Device.h"
#include "Sensor.h"
#include <string.h>

/**************************************************************/
/* 传感器列表                                                 */
/**************************************************************/
/********************************/
/* 燃气传感器                   */
/********************************/
#if defined(HAS_GAS)
void sensorGasResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void sensorGasResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        GasInit();
    }
}
void sensorGasTimeout(struct ep_info_t *ep);
void sensorGasTimeout(struct ep_info_t *ep)
{
    uint8 value = GasInfo();
    SendData(ep->ep, &value, 0x0000, TRANSFER_ENDPOINT, sizeof(value));
}
#endif
/********************************/
/* 温度传感器                   */
/********************************/
#if defined(HAS_TEMP) || defined(HAS_HUMM)
#include "sht10.h"
#define TEMPERATURE 1
#define HUMIDITY 2
static uint16 lastTemp = 0;
#endif
#if defined(HAS_TEMP)
void sensorTempResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void sensorTempResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        {
            SHT10_init(0x01);
        }
    }
}
void sensorTempTimeout(struct ep_info_t *ep);
void sensorTempTimeout(struct ep_info_t *ep)
{
    unsigned int value = 0;
    unsigned char checksum = 0;
    SHT10_Measure(&value,&checksum, TEMPERATURE);
    lastTemp = (value << 2) - 3960;
    SendData(ep->ep, &lastTemp, 0x0000, TRANSFER_ENDPOINT, sizeof(lastTemp));
}
#endif
/********************************/
/* 湿度传感器                   */
/********************************/
#if defined(HAS_HUMM)
void sensorHummResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void sensorHummResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        {
            SHT10_init(0x01);
        }
    }
}
void sensorHummTimeout(struct ep_info_t *ep);
void sensorHummTimeout(struct ep_info_t *ep)
{
    const float C1 = -4.0f;              // for 8 Bit
    const float C2 = +0.648f;            // for 8 Bit
    const float C3 = -0.0000072f;        // for 8 Bit
    const float T1 = 0.01f;              // for 8 bit
    const float T2 = 0.00128f;           // for 8 bit
    float rh_lin    =   0.0f;                     // rh_lin: Humidity linear
    float rh_true   =   0.0f;                    // rh_true: Temperature compensated humidity
    float t_C   = 0.0f;                        // t_C   : Temperature []

    unsigned int value = 0;
    unsigned char checksum = 0;
    SHT10_Measure(&value,&checksum, HUMIDITY);
    rh_lin=C3*value*value + C2*value + C1;     //calc. humidity from ticks to [%RH]
    rh_true=(t_C-25)*(T1+T2*value)+rh_lin;   //calc. temperature compensated humidity [%RH]
    if(rh_true>100)
        rh_true=100;       //cut if the value is outside of
    if(rh_true<0.1)
        rh_true=0.1f;       //the physical possible range
    value = (unsigned int)(rh_true * 100);
    SendData(ep->ep, &value, 0x0000, TRANSFER_ENDPOINT, sizeof(value));
}
#endif
/********************************/
/* 雨滴传感器                   */
/********************************/
#if defined(HAS_RAIN)
void sensorRainTimeout(struct ep_info_t *ep);
void sensorRainTimeout(struct ep_info_t *ep)
{
    uint8 value = GetADValue();
    // TODO: 这里增加算法,将AD值变换为有雨或没雨(1或0)
    value = (value > 0x0A);
    SendData(ep->ep, &value, 0x0000, TRANSFER_ENDPOINT, sizeof(value));
}
#endif
/********************************/
/* 火焰传感器                   */
/********************************/
#if defined(HAS_FIRE)
void sensorFireTimeout(struct ep_info_t *ep);
void sensorFireTimeout(struct ep_info_t *ep)
{
    uint8 value = GetADValue();
    // TODO: 这里增加算法,将AD值变换为有雨或没雨(1或0)
    value = (value > 0x0A);
    SendData(ep->ep, &value, 0x0000, TRANSFER_ENDPOINT, sizeof(value));
}
#endif
/********************************/
/* 烟雾传感器                   */
/********************************/
#if defined(HAS_SMOKE)
void sensorSmokeResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void sensorSmokeResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
        SmokeInit();
}
void sensorSmokeTimeout(struct ep_info_t *ep);
void sensorSmokeTimeout(struct ep_info_t *ep)
{
    uint8 value = SmokeInfo();
    SendData(ep->ep, &value, 0x0000, TRANSFER_ENDPOINT, sizeof(value));
}
#endif
/********************************/
/* 光照度传感器                 */
/********************************/
#if defined(HAS_ILLUM)
void sensorILLumTimeout(struct ep_info_t *ep);
void sensorILLumTimeout(struct ep_info_t *ep)
{
    uint16 value = 256 - GetADValue();
    // TODO: 这里增加算法,将AD值变换为光照度的100倍
    value = value * 39;// * 10000 / 256;
    SendData(ep->ep, &value, 0x0000, TRANSFER_ENDPOINT, sizeof(value));
}
#endif
/********************************/
/* 安防传感器                   */
/********************************/
#if defined(HAS_IRPERS)
void sensorIRPersResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void sensorIRPersResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
        SafetyInit();
}
void sensorIRPersTimeout(struct ep_info_t *ep);
void sensorIRPersTimeout(struct ep_info_t *ep)
{
    uint8 value = SafetyInfo();
    SendData(ep->ep, &value, 0x0000, TRANSFER_ENDPOINT, sizeof(value));
}
#endif
/********************************/
/* 红外测距传感器               */
/********************************/
#if defined(HAS_IRDIST)
void sensorIRDistTimeout(struct ep_info_t *ep);
void sensorIRDistTimeout(struct ep_info_t *ep)
{
    uint16 value = 0;
    // TODO: 这里增加算法,计算距离值value的单位为mm
    
    SendData(ep->ep, &value, 0x0000, TRANSFER_ENDPOINT, sizeof(value));
}
#endif
/********************************/
/* 语音传感器                   */
/********************************/
#if defined(HAS_VOICE)
#include "hal_uart.h"
static struct ep_info_t *voiceEndPoint = NULL;
static uint8 lastVoiceData = 0;
static void sensorVoiceUartProcess( uint8 port, uint8 event );
static void sensorVoiceUartProcess( uint8 port, uint8 event )
{
    (void)event;  // Intentionally unreferenced parameter
    while (Hal_UART_RxBufLen(port))
    {
        HalUARTRead(port, &lastVoiceData, 1);
        if(lastVoiceData == 0xAA)
            lastVoiceData = 1;
        else if(lastVoiceData == 0x55)
            lastVoiceData = 0;
        else
            lastVoiceData = -1;
        if(voiceEndPoint != NULL)
            SendData(voiceEndPoint->ep, &lastVoiceData, 0x0000, TRANSFER_ENDPOINT, 1);
    }
}
void sensorVoiceNwkStateChange(struct ep_info_t *ep);
void sensorVoiceNwkStateChange(struct ep_info_t *ep)
{
    voiceEndPoint = ep;
}
void sensorVoiceResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void sensorVoiceResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        halUARTCfg_t uartConfig;
        
        voiceEndPoint = ep;
        /* UART Configuration */
        uartConfig.configured           = TRUE;
        uartConfig.baudRate             = HAL_UART_BR_9600;
        uartConfig.flowControl          = FALSE;
        uartConfig.flowControlThreshold = MT_UART_DEFAULT_THRESHOLD;
        uartConfig.rx.maxBufSize        = MT_UART_DEFAULT_MAX_RX_BUFF;
        uartConfig.tx.maxBufSize        = MT_UART_DEFAULT_MAX_TX_BUFF;
        uartConfig.idleTimeout          = MT_UART_DEFAULT_IDLE_TIMEOUT;
        uartConfig.intEnable            = TRUE;
        uartConfig.callBackFunc         = sensorVoiceUartProcess;
        HalUARTOpen(HAL_UART_PORT_1, &uartConfig);
    }
}
void sensorVoiceTimeout(struct ep_info_t *ep);
void sensorVoiceTimeout(struct ep_info_t *ep)
{
    uint8 nulData = 0;
    SendData(ep->ep, &nulData, 0x0000, TRANSFER_ENDPOINT, 1);
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
/********************************/
/* IC卡读卡器                   */
/********************************/
#if defined(HAS_125KREADER)
#include "hal_uart.h"
#define CARDID_SIZE     5
static uint8 lastCardId[CARDID_SIZE];
static uint8 cardRecvIdx;
static uint32 lastTick;
static struct ep_info_t *cardEndPoint;
static void sensor125kReaderUartProcess( uint8 port, uint8 event );
static void sensor125kReaderUartProcess( uint8 port, uint8 event )
{
    (void)event;  // Intentionally unreferenced parameter
    if((lastTick + 100) <= osal_GetSystemClock())
    {
        cardRecvIdx = 0;
    }
    lastTick = osal_GetSystemClock();
    while (Hal_UART_RxBufLen(port))
    {
        uint16 restLen = Hal_UART_RxBufLen(port);
        if(restLen > (CARDID_SIZE - cardRecvIdx))
            restLen = CARDID_SIZE - cardRecvIdx;
        HalUARTRead(port, &lastCardId[cardRecvIdx], restLen);
        cardRecvIdx += restLen;
        if(cardRecvIdx >= CARDID_SIZE)
        {
            SendData(cardEndPoint->ep, lastCardId, 0x0000, TRANSFER_ENDPOINT, CARDID_SIZE);
        }
    }
}
void sensor125kReaderResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res);
void sensor125kReaderResAvailable(struct ep_info_t *ep, RES_TYPE type, void *res)
{
    if(type == ResInit)
    {
        halUARTCfg_t uartConfig;
        
        memset(lastCardId, 0, sizeof(lastCardId));
        cardRecvIdx = 0;
        cardEndPoint = ep;
        /* UART Configuration */
        uartConfig.configured           = TRUE;
        uartConfig.baudRate             = HAL_UART_BR_19200;
        uartConfig.flowControl          = FALSE;
        uartConfig.flowControlThreshold = MT_UART_DEFAULT_THRESHOLD;
        uartConfig.rx.maxBufSize        = MT_UART_DEFAULT_MAX_RX_BUFF;
        uartConfig.tx.maxBufSize        = MT_UART_DEFAULT_MAX_TX_BUFF;
        uartConfig.idleTimeout          = MT_UART_DEFAULT_IDLE_TIMEOUT;
        uartConfig.intEnable            = TRUE;
        uartConfig.callBackFunc         = sensor125kReaderUartProcess;
        HalUARTOpen(HAL_UART_PORT_0, &uartConfig);
    }
}
void sensor125kReaderTimeout(struct ep_info_t *ep);
void sensor125kReaderTimeout(struct ep_info_t *ep)
{
  uint8 nullId[CARDID_SIZE] = { 0x00 };
    SendData(cardEndPoint->ep, nullId, 0x0000, TRANSFER_ENDPOINT, CARDID_SIZE);
}
#endif
/***************************************************/
/* 下面这一段针对路由器, 不需要修改                */
/***************************************************/
struct ep_info_t funcList[] = {
#if defined(HAS_GAS)
    {
        //stat,income,timeout,resource
        NULL, NULL, sensorGasTimeout, sensorGasResAvailable,
        { DevGas, 0, 3 },                   // type, id, refresh cycle
    },
#endif
#if defined(HAS_TEMP)
    {
        NULL, NULL, sensorTempTimeout, sensorTempResAvailable,
        { DevTemp, 1, 5 },                 // type, id, refresh cycle
    },
#endif
#if defined(HAS_HUMM)
    {
        NULL, NULL, sensorHummTimeout, sensorHummResAvailable,
        { DevHumm, 0, 5 },                 // type, id, refresh cycle
    },
#endif
#if defined(HAS_ILLUM)
    {
        NULL, NULL, sensorILLumTimeout, NULL,
        { DevILLum, 0, 3 },                // type, id, refresh cycle
    },
#endif
#if defined(HAS_RAIN)
    {
        NULL, NULL, sensorRainTimeout, NULL,
        { DevRain, 0, 5 },                 // type, id, refresh cycle
    },
#endif
#if defined(HAS_IRDIST)
    {
        NULL, NULL, sensorIRDistTimeout, NULL,
        { DevIRDist, 0, 3 },               // type, id, refresh cycle
    },
#endif
#if defined(HAS_SMOKE)
    {
        NULL, NULL, sensorSmokeTimeout, sensorSmokeResAvailable,
        { DevSmoke, 0, 3 },                 // type, id, refresh cycle
    },
#endif
#if defined(HAS_FIRE)
    {
        NULL, NULL, sensorFireTimeout, NULL,
        { DevFire, 0, 3 },                  // type, id, refresh cycle
    },
#endif
#if defined(HAS_IRPERS)
    {
        NULL, NULL, sensorIRPersTimeout, sensorIRPersResAvailable,
        { DevIRPers, 0, 3 },                // type, id, refresh cycle
    },
#endif
#if defined(HAS_VOICE)
    {
        sensorVoiceNwkStateChange, NULL, sensorVoiceTimeout, sensorVoiceResAvailable,
        { DevVoice, 0, 5 },                // type, id, refresh cycle
    },
#endif
#if defined(HAS_EXECUTEB)
    {
        NULL, outputExecuteB, outputExecuteBTimeout, OutputExecuteBResAvailable,
        { DevExecuteB, 0, 10 },              // type, id, refresh cycle
    },
#endif
#if defined(HAS_EXECUTEA)
    {
        NULL, outputExecuteA, NULL, NULL,
        { DevExecuteA, 0, 3 },              // type, id, refresh cycle
    },
#endif
#if defined(HAS_REMOTER)
    {
        NULL, outputRemoter, NULL, NULL,
        { DevRemoter, 0, 10 },              // type, id, refresh cycle
    },
#endif
#if defined(HAS_TESTFUNCTION)
    {
        testFunc_NwkStateChanged,
        testFunc_inComeData,
        testFunc_TimeOut,
        testFunc_ResAvailable,
        { 1, 0, 5 },
    },
#endif
#if defined(HAS_125KREADER)
    {
        NULL, NULL, sensor125kReaderTimeout, sensor125kReaderResAvailable,
        { Dev125kReader, 0, 10 },
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
#elif defined(RTR_NWK)
    {   // 路由器
        RouterNwkStateChangeRoutine,
        RouterIncomingRoutine,
        RouterTimeoutRoutine,
        RouterResAvailableRoutine,
        { DevRouter, 0, 5 },
    },
#endif
};

// 不能修改下面的内容!!!
const uint8 funcCount = sizeof(funcList) / sizeof(funcList[0]);
