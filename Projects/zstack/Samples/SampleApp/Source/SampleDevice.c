/**************************************************************************************************
  Filename:       SampleApp.c
  Revised:        $Date: 2009-03-18 15:56:27 -0700 (Wed, 18 Mar 2009) $
  Revision:       $Revision: 19453 $

  Description:    Sample Application (no Profile).


  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED 揂S IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
  This application isn't intended to do anything useful, it is
  intended to be a simple example of an application's structure.

  This application sends it's messages either as broadcast or
  broadcast filtered group messages.  The other (more normal)
  message addressing is unicast.  Most of the other sample
  applications are written to support the unicast message model.

  Key control:
    SW1:  Sends a flash command to all devices in Group 1.
    SW2:  Adds/Removes (toggles) this device in and out
          of Group 1.  This will enable and disable the
          reception of the flash command.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"

#include "MT.h"
#include "MT_UART.h"
#if defined( MT_TASK )
#include "MT_TASK.h"
#endif
#if defined ( ZIGBEE_FREQ_AGILITY ) || defined ( ZIGBEE_PANID_CONFLICT )
  #include "ZDNwkMgr.h"
#endif
#include "SampleDevice.h"
#include "SampleAppHw.h"
#include <string.h>
#include "OnBoard.h"

/* HAL */
#include "hal_led.h"

#include "Sensor.h"

uint8 SendData(uint8 srcEP, uint8 *buf, uint16 addr, uint8 dsrEP, uint8 Leng);
#if defined(ZDO_COORDINATOR)
uint8 uartMsgProcesser(uint8 *msg);
#endif

#include "hal_drivers.h"
#if defined ( ZIGBEE_FRAGMENTATION )
#include "aps_frag.h"
#endif

const pTaskEventHandlerFn tasksArr[] = {
    macEventLoop,
    nwk_event_loop,
    Hal_ProcessEvent,
#if defined( MT_TASK )
    MT_ProcessEvent,
#endif
    APS_event_loop,
#if defined ( ZIGBEE_FRAGMENTATION )
    APSF_ProcessEvent,
#endif
    ZDApp_event_loop,
#if defined ( ZIGBEE_FREQ_AGILITY ) || defined ( ZIGBEE_PANID_CONFLICT )
    ZDNwkMgr_event_loop,
#endif
    controlEpProcess,
#if !defined(ZDO_COORDINATOR)
    functionEpProcess,
#endif
};
const uint8 tasksCnt = sizeof(tasksArr)/sizeof(tasksArr[0]);

static uint8 controlTaskId, functionTaskId;
static struct ep_info_t controlEndPointInfo;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void taskInitProcess(void)
{
 #if defined ( BUILD_ALL_DEVICES )
    // The "Demo" target is setup to have BUILD_ALL_DEVICES and HOLD_AUTO_START
    // We are looking at a jumper (defined in SampleAppHw.c) to be jumpered
    // together - if they are - we will start up a coordinator. Otherwise,
    // the device will start as a router.
    if ( readCoordinatorJumper() )
        zgDeviceLogicalType = ZG_DEVICETYPE_COORDINATOR;
    else
        zgDeviceLogicalType = ZG_DEVICETYPE_ROUTER;
#endif // BUILD_ALL_DEVICES

#if defined ( HOLD_AUTO_START )
    // HOLD_AUTO_START is a compile option that will surpress ZDApp
    //  from starting the device and wait for the application to
    //  start the device.
    ZDOInitDevice(0);
#endif

#if !defined(ZDO_COORDINATOR)
  // 构造功能列表
    funcTableBuffer = createFuncTable(funcCount);
    funcTableBuffer->ft_type = 0x01;
    funcTableBuffer->ft_count = funcCount;
    int i;
    for(i = 0; i < funcCount; i++)
    {
        funcTableBuffer->ft_list[i].type = funcList[i].function.type;
        funcTableBuffer->ft_list[i].id = funcList[i].function.id;
        funcTableBuffer->ft_list[i].cycle = funcList[i].function.cycle;
    }
    controlTaskId = tasksCnt - 2;
    functionTaskId = tasksCnt - 1;
    createEndPoint(&controlEndPointInfo, &controlTaskId, CONTROL_ENDPOINT);
    for(i = 0; i < funcCount; i++)
    {
        createEndPoint(&funcList[i], &functionTaskId, i + 1);
    }
#else
    controlTaskId = tasksCnt - 1;
    createEndPoint(&controlEndPointInfo, &controlTaskId, CONTROL_ENDPOINT);
#endif
#if defined(ZDO_COORDINATOR)// || defined(RTR_NWK)
//    RegisterForKeys( SampleApp_TaskID );
    MT_UartRegisterTaskID(controlTaskId);
#endif
}

void createEndPoint(struct ep_info_t *epInfo, uint8 *task_id, uint8 ep)
{
    static cId_t commonClusterId = SAMPLEAPP_PERIODIC_CLUSTERID;
    // Fill out the endpoint description.
    epInfo->task_id = *task_id;
    epInfo->ep = ep;
    epInfo->timerTick = epInfo->function.cycle;

    epInfo->simpleDesc.EndPoint = ep;
    epInfo->simpleDesc.AppProfId = SAMPLEAPP_PROFID;
    epInfo->simpleDesc.AppDeviceId = SAMPLEAPP_DEVICEID;
    epInfo->simpleDesc.AppDevVer = SAMPLEAPP_DEVICE_VERSION;
    epInfo->simpleDesc.Reserved = 0;
    epInfo->simpleDesc.AppNumInClusters = 1;
    epInfo->simpleDesc.pAppInClusterList = &commonClusterId;
    epInfo->simpleDesc.AppNumOutClusters = 1;
    epInfo->simpleDesc.pAppOutClusterList = &commonClusterId;
    
    epInfo->SampleApp_epDesc.endPoint = ep;
    epInfo->SampleApp_epDesc.task_id = task_id;
    epInfo->SampleApp_epDesc.simpleDesc = &epInfo->simpleDesc;
    epInfo->SampleApp_epDesc.latencyReq = noLatencyReqs;

    // Register the endpoint description with the AF
    afRegister(&epInfo->SampleApp_epDesc);
}

uint16 controlEpProcess(uint8 task_id, uint16 events)
{
    afIncomingMSGPacket_t *MSGpkt;

    if ( events & SYS_EVENT_MSG )
    {
        MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(task_id);
        while ( MSGpkt )
        {
            switch ( MSGpkt->hdr.event )
            {
#if defined(ZDO_COORDINATOR)
            case CMD_SERIAL_MSG:
//                SampleApp_UartMessage((uint8 *)MSGpkt);
                uartMsgProcesser((uint8 *)MSGpkt);
                HalLedBlink( HAL_LED_1, 2, 50, 90 );
                break;
#endif
                // Received when a messages is received (OTA) for this endpoint
            case AF_INCOMING_MSG_CMD:
            {
                // TODO: QueryProfile or QueryTopo
                switch(MSGpkt->clusterId)
                {
                case SAMPLEAPP_PERIODIC_CLUSTERID:
#if defined(ZDO_COORDINATOR)
                    // 转发数据到串口
                    if(MSGpkt->cmd.DataLength > 0)
                    {
                        mtUserSerialMsg_t *pMsg = osal_mem_alloc(sizeof(mtUserSerialMsg_t) + MSGpkt->cmd.DataLength - 1);
                        pMsg->sop = MT_UART_SOF;
                        pMsg->len = MSGpkt->cmd.DataLength + 6;
                        pMsg->cmd = 0x0018;
                        pMsg->cmdEndPoint = 0xF1;
                        pMsg->addr = MSGpkt->srcAddr.addr.shortAddr;
                        pMsg->endPoint = MSGpkt->srcAddr.endPoint;
                        memcpy(pMsg->data, MSGpkt->cmd.Data, MSGpkt->cmd.DataLength);
                        pMsg->fsc = MT_UartCalcFCS(0, &pMsg->len, 1);
                        pMsg->fsc = MT_UartCalcFCS(pMsg->fsc, pMsg->dataBody, pMsg->len);
                        HalUARTWrite(HAL_UART_PORT_0, &pMsg->sop, sizeof(mtUserSerialMsg_t) - 2 + MSGpkt->cmd.DataLength);
                        HalUARTWrite(HAL_UART_PORT_0, &pMsg->fsc, 1);
                    }
#else
                    switch(MSGpkt->cmd.Data[0])
                    {
                    case 0x01:
                        // CtrlQueryProfile
                        // TODO: 应当获取到数据包的来源地址来当做发送数据的目标
                        SendData(CONTROL_ENDPOINT, funcTableBuffer->ft_data, 0x0000, CONTROL_ENDPOINT, sizeof(FUNCTABLE) + funcCount * sizeof(FUNCINFO));
                        break;
                    case 0x02:
                        // CtrlQueryTopo
                        // TODO: 应当获取到数据包的来源地址来当做发送数据的目标
                        SendData(CONTROL_ENDPOINT, (unsigned char *)topoBuffer, 0x0000, CONTROL_ENDPOINT, sizeof(TOPOINFO)); //节点向协调器发送采集数据
                        break;
                    }
#endif
                    HalLedBlink( HAL_LED_2, 1, 50, 250 );
                    break;
                }
            }
            break;
            // Received whenever the device changes state in the network
            case ZDO_STATE_CHANGE:
            {
                devStates_t st = (devStates_t)(MSGpkt->hdr.status);
                if ( (st == DEV_ZB_COORD)
                        || (st == DEV_ROUTER)
                        || (st == DEV_END_DEVICE) )
                {
#if !defined(ZDO_COORDINATOR)
                    topoBuffer->type = 0x02;
                    memcpy(topoBuffer->IEEE, NLME_GetExtAddr(), 8);
                    topoBuffer->PAddr = NLME_GetCoordShortAddr();
#endif
                    HalLedBlink( HAL_LED_2, 4, 50, 250 );
                }
                else
                {
                    // Device is no longer in the network
                    // TODO:
                    switch(st)
                    {
                    }
                }
            }
            break;
            default:
                break;
            }
            // Release the memory
            osal_msg_deallocate( (uint8 *)MSGpkt );
            // Next - if one is available
            MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( task_id );
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    // Discard unknown events
    return 0;
}

#if !defined(ZDO_COORDINATOR)
uint16 functionEpProcess(uint8 task_id, uint16 events)
{
    afIncomingMSGPacket_t *MSGpkt;
    if(events & SYS_EVENT_MSG)
    {
        MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( task_id );
        while ( MSGpkt )
        {
            switch ( MSGpkt->hdr.event )
            {
            // 接收到数据包
            case AF_INCOMING_MSG_CMD:
                {
                    switch ( MSGpkt->clusterId )
                    {
                    case SAMPLEAPP_PERIODIC_CLUSTERID:
                        if(MSGpkt->endPoint <= funcCount)
                        {
                            struct ep_info_t *ep = &funcList[MSGpkt->endPoint - 1];
                            if(ep->incoming_data)
                                (*ep->incoming_data)(ep, MSGpkt->srcAddr.addr.shortAddr, MSGpkt->endPoint, &MSGpkt->cmd);
                        }
                        HalLedBlink( HAL_LED_2, 1, 50, 250 );
                        break;
                    }
                }
                break;

            // 普通的传感器需要这段代码
            // Received whenever the device changes state in the network
            case ZDO_STATE_CHANGE:
                // TODO: 确认这个消息是否会被派发到此任务
                {
                    devStates_t st = (devStates_t)(MSGpkt->hdr.status);
                    if ( (st == DEV_ZB_COORD)
                            || (st == DEV_ROUTER)
                            || (st == DEV_END_DEVICE) )
                    {
                        int i;
                        int hasTimeOut = 0;
                        for(i = 0; i < funcCount; i++)
                        {
                            struct ep_info_t *ep = &funcList[i];
                            if(ep->nwk_stat_change)
                                (*ep->nwk_stat_change)(ep);
                            // 重置端点计数器
                            if(ep->time_out && ep->function.cycle)
                            {
                                ep->timerTick = ep->function.cycle;
                                hasTimeOut = 1;
                            }
                        }
                        if(hasTimeOut)
                        {
                            // 加入网络成功,启动定时器,为各个端点提供定时
                            osal_start_timerEx(task_id,
                                               SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                                               1000);
                        }
                    }
                }
                break;
            default:
                break;
            }
            // Release the memory
            osal_msg_deallocate( (uint8 *)MSGpkt );
            // Next - if one is available
            MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( task_id );
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    // 定时器时间到, 遍历所有端点看是否有需要调用time_out
    if(events & SAMPLEAPP_SEND_PERIODIC_MSG_EVT)
    {
        int i;
        for(i = 0; i < funcCount; i++)
        {
            struct ep_info_t *ep = &funcList[i];
            if(ep->time_out && ep->function.cycle)
            {
                // 端点需要周期执行
                ep->timerTick = ep->timerTick - 1;
                if(ep->timerTick == 0)
                {
                  // 定时时间到,执行time_out函数 
                  (*ep->time_out)(ep);
                  ep->timerTick = ep->function.cycle;
                }
            }
        }
        // 重新启动定时器
        osal_start_timerEx(task_id, SAMPLEAPP_SEND_PERIODIC_MSG_EVT, 1000);
        // return unprocessed events
        return (events ^ SAMPLEAPP_SEND_PERIODIC_MSG_EVT);
    }
    // Discard unknown events
    return 0;
}
#endif

#if defined(ZDO_COORDINATOR)
uint8 uartMsgProcesser(uint8 *msg)
{
    mtOSALSerialData_t *pMsg = (mtOSALSerialData_t *)msg;
    mtUserSerialMsg_t *pMsgBody = (mtUserSerialMsg_t *)pMsg->msg;
    switch(pMsgBody->cmd)
    {
    case 0x0018:
        {
            switch(pMsgBody->cmdEndPoint)
            {
            case 0xF1:
                {
                    // 转发数据
                    SendData(CONTROL_ENDPOINT, pMsgBody->data,
                             pMsgBody->addr, pMsgBody->endPoint,
                             pMsgBody->len - 6);
                }
                break;
            }
        }
        break;
    }
    return 1;
}
#endif
//**********************************************************************
//**以短地址方式发送数据
//buf ::发送的数据
//addr::目的地址
//Leng::数据长度
//********************************************************************
uint8 SendData(uint8 srcEP, uint8 *buf, uint16 addr, uint8 dstEP, uint8 Leng)
{
    static uint8 transID = 0;
    afAddrType_t SendDataAddr;
    struct ep_info_t *epInfo;

#if !defined(ZDO_COORDINATOR)
    if(srcEP <= funcCount)
        epInfo = &funcList[srcEP - 1];
    else
#endif
        epInfo = &controlEndPointInfo;
    
    SendDataAddr.addrMode = (afAddrMode_t)Addr16Bit;         //短地址发送
    SendDataAddr.endPoint = dstEP;
    SendDataAddr.addr.shortAddr = addr;
    if ( AF_DataRequest( &SendDataAddr, //发送的地址和模式
                         // TODO:
                         &epInfo->SampleApp_epDesc,   //终端（比如操作系统中任务ID等）
                         SAMPLEAPP_PERIODIC_CLUSTERID,//发送串ID
                         Leng,
                         buf,
                         &transID,  //信息ID（操作系统参数）
                         AF_DISCV_ROUTE,
                         AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
    {
        HalLedBlink( HAL_LED_1, 1, 50, 250 );
        return 1;
    }
    else
    {
        return 0;
    }
}
/*********************************************************************
*********************************************************************/


#if !defined(ZDO_COORDINATOR)
/**************************************************************/
/* 传感器列表                                                 */
/**************************************************************/
void testFunc_NwkStateChanged(const struct ep_info_t *ep);
void testFunc_NwkStateChanged(const struct ep_info_t *ep)
{
}
void testFunc_inComeData(const struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg);
void testFunc_inComeData(const struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg)
{
    //msg->Data[], msg->DataLength, msg->TransSeqNumber
}
void testFunc_TimeOut(const struct ep_info_t *ep);
void testFunc_TimeOut(const struct ep_info_t *ep)
{
    // send sensor data to coordinator
    //    SampleApp_SendPeriodicMessage(task_id);
    uint8 sendBuf[] = "(1 1)\r\n";
    sendBuf[1] = ep->ep + '0';
    sendBuf[3] = ep->function.type + '0';
    SendData(ep->ep, sendBuf, 0x0000, CONTROL_ENDPOINT, 7);
}

#if defined(RTR_NWK)
void router_nwkStateChanged(const struct ep_info_t *ep);
void router_nwkStateChanged(const struct ep_info_t *ep)
{
}
void router_TimeOut(const struct ep_info_t *ep);
void router_TimeOut(const struct ep_info_t *ep)
{
    // send sensor data to coordinator
    //    SampleApp_SendPeriodicMessage(task_id);
//    uint8 sendBuf[] = "(1 1)\r\n";
//    sendBuf[1] = ep->ep + '0';
//    sendBuf[3] = ep->function.type + '0';
//    SendData(ep->ep, sendBuf, 0x0000, CONTROL_ENDPOINT, 7);
}

#endif
struct ep_info_t funcList[] = {
#if defined(RTR_NWK)
    {   // 路由器
        router_nwkStateChanged,
        NULL,
        router_TimeOut,
        { DevRouter, 0, 30 },
    },
#endif
    {
        testFunc_NwkStateChanged,
        testFunc_inComeData,
        testFunc_TimeOut,
        { 1, 0, 5 },
    },
};
const uint8 funcCount = sizeof(funcList) / sizeof(funcList[0]);
#define FUNC_NUM        (sizeof(funcList) / sizeof(funcList[0]))
#endif

#if !defined(ZDO_COORDINATOR)
uint8 ctrlBuffer[sizeof(TOPOINFO) + sizeof(FUNCTABLE) + FUNC_NUM * sizeof(FUNCINFO)];
TOPOINFO *topoBuffer = (TOPOINFO *)ctrlBuffer;
FUNCTABLE *funcTableBuffer = (FUNCTABLE *)(&ctrlBuffer[sizeof(TOPOINFO)]);
#endif
