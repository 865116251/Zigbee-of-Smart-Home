#include "SAPP_FrameWork.h"
#include <string.h>
#include <Lcd_Dis.h>
#include "hal_lcd.h"
/*********************************************************************
 * FUNCTIONS
 *********************************************************************/
static void createEndPoint(struct ep_info_t *epInfo, uint8 *task_id, uint8 ep);
static void taskInitProcess(void);
static uint16 controlEpProcess(uint8 task_id, uint16 events);
static UINT16 functionEpProcess( uint8 task_id, uint16 events );
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_MessageMSGCB2( afIncomingMSGPacket_t *pckt );
uint16 SrcShortAddr = 0xFFFF;//PeerToPeer时使用
void Delay100us(uint16);
#if defined(ZDO_COORDINATOR) || defined(PEER_ROUTER) || defined(PEER_COORD)
static uint8 uartMsgProcesser(uint8 *msg);
#endif



//uint8 ctrlBuffer[sizeof(TOPOINFO) + sizeof(FUNCTABLE) + FUNC_NUM * sizeof(FUNCINFO)];
static TOPOINFO topoBuffer = { 0x02 };
FUNCTABLE *funcTableBuffer;// = (FUNCTABLE *)(&ctrlBuffer[sizeof(TOPOINFO)]);

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
    functionEpProcess,
};
const uint8 tasksCnt = sizeof(tasksArr)/sizeof(tasksArr[0]);
/*********************************************************************
*********************************************************************/

/*********************************************************************
 * @fn      osalInitTasks
 *
 * @brief   This function invokes the initialization function for each task.
 *
 * @param   void
 *
 * @return  none
 */
void osalInitTasks( void )
{
  P0DIR = (0x01<<0)|(0x01<<4)|(0x01<<5)|(0x01<<6)|(0x01<<7);  // 设置P0_0,P0_4,P0_5,P06,P0_7 为输出方式 
  P1DIR = 0xff;  // 设置 P1 为输出方式 
  P2DIR |= 0x01;  // 设置 P2.0 为输出方式 
  P0_7 = 0;//关闭蜂鸣器
  //全灭     
  P0 |= (0x1 << 6); 
  P1 = 0x0; 
                    P0 &= ~(0x1 << 6); 
 /* P0DIR = 0xf1;  // 设置 P0.0,P0.4,P0.5,P0.6,P0.7 为输出方式 
  P1DIR = 0xf1;  // 设置 P1.0,P1.4,P1.5,P1.6,P1.7 为输出方式 
  P2DIR |= 0x01;  // 设置 P2.0 为输出方式 */
 // P0DIR = 0x40;  // 设置 P0.6 为输出方式 

    uint8 taskID = 0;

    macTaskInit( taskID++ );
    nwk_init( taskID++ );
    Hal_Init( taskID++ );
#if defined( MT_TASK )
    MT_TaskInit( taskID++ );
#endif
    APS_Init( taskID++ );
#if defined ( ZIGBEE_FRAGMENTATION )
    APSF_Init( taskID++ );
#endif
    ZDApp_Init( taskID++ );
#if defined ( ZIGBEE_FREQ_AGILITY ) || defined ( ZIGBEE_PANID_CONFLICT )
    ZDNwkMgr_Init( taskID++ );
#endif
#if defined(ZDO_COORDINATOR)// || defined(RTR_NWK)    
    ClrScreen();        
    FontSet_cn(1,1);                //16x16	
    PutString_cn(24,0,"网络协调器");
    PutString_cn(0,16,"---------------");
    PutString_cn(8,32,"启动新的网络");	
    PutString_cn(8,48,"正在启动...");	
#endif  
    taskInitProcess();  
    //Hello_Init(taskID);
    
//P0_6=0;   //停止直流电机
}

static devStates_t curNwkState;
static uint8 controlTaskId;
static uint8 functionTaskId;
static struct ep_info_t controlEndPointInfo;
static uint8 isUserTimerRunning = 0;
/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void taskInitProcess(void)
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
    controlTaskId = tasksCnt - 2; //排在 tasksArr[] 的最后两个。
    functionTaskId = tasksCnt - 1;
    createEndPoint(&controlEndPointInfo, &controlTaskId, CONTROL_ENDPOINT);
    for(i = 0; i < funcCount; i++)
    {
        struct ep_info_t *ep = &funcList[i];
        createEndPoint(ep, &functionTaskId, i + 1);
        if(ep->res_available)
          (*ep->res_available)(ep, ResInit, NULL);
    }
#if defined(ZDO_COORDINATOR) || defined(PEER_ROUTER) || defined(PEER_COORD)   
//  RegisterForKeys( SampleApp_TaskID );
    MT_UartRegisterTaskID(controlTaskId);     
#endif  
}

static void createEndPoint(struct ep_info_t *epInfo, uint8 *task_id, uint8 ep)
{
    static cId_t commonClusterId = SAPP_PERIODIC_CLUSTERID;
    // Fill out the endpoint description.
    epInfo->task_id = *task_id;
    epInfo->ep = ep;
    epInfo->timerTick = epInfo->function.cycle;
    epInfo->userTimer = 0;

    epInfo->simpleDesc.EndPoint = ep;
    epInfo->simpleDesc.AppProfId = SAPP_PROFID;
    epInfo->simpleDesc.AppDeviceId = SAPP_DEVICEID;
    epInfo->simpleDesc.AppDevVer = SAPP_DEVICE_VERSION;
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

static uint16 controlEpProcess(uint8 task_id, uint16 events)
{
    afIncomingMSGPacket_t *MSGpkt;
    //系统消息事件。
    if ( events & SYS_EVENT_MSG )
    {
        MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(task_id);
        while ( MSGpkt )
        {                
            switch ( MSGpkt->hdr.event )
            {         
//#if defined(ZDO_COORDINATOR) 
#if defined(ZDO_COORDINATOR) || defined(PEER_ROUTER) || defined(PEER_COORD)              
            case CMD_SERIAL_MSG:
//              SampleApp_UartMessage((uint8 *)MSGpkt);                                          
                uartMsgProcesser((uint8 *)MSGpkt);
                HalLedBlink( HAL_LED_1, 2, 50, 90 );
                break;
#endif
            // Received when a messages is received (OTA) for this endpoint
            case AF_INCOMING_MSG_CMD:
            {           
                #if defined(PEER_ROUTER) || defined(PEER_COORD)
                    HalUARTWrite(HAL_UART_PORT_0, &(MSGpkt->cmd.Data[0]), 2);
                    break;
                #endif
                // TODO: QueryProfile or QueryTopo
                switch(MSGpkt->clusterId)
                {
                case SAPP_PERIODIC_CLUSTERID:
                    switch(MSGpkt->cmd.Data[0])
                    {
                    case 0x01:
                        // CtrlQueryProfile
                        // 获取到数据包的来源地址来当做发送数据的目标
                        SendData(CONTROL_ENDPOINT, funcTableBuffer->ft_data, MSGpkt->srcAddr.addr.shortAddr, MSGpkt->srcAddr.endPoint, sizeof(FUNCTABLE) + funcCount * sizeof(FUNCINFO));
                        break;
                    case 0x02:
                        // CtrlQueryTopo
                        // 获取到数据包的来源地址来当做发送数据的目标
                        SendData(CONTROL_ENDPOINT, (unsigned char *)&topoBuffer, MSGpkt->srcAddr.addr.shortAddr, MSGpkt->srcAddr.endPoint, sizeof(TOPOINFO));                        
                        break;
                    case 0x03:
                        // CtrlQuerySpecialFunction
                        // cmd.Data[0] = 3, cmd.Data[1] = funcCode, cmd.Data[2] = funcID
                        {
                            uint8 i;
                            for(i = 0; i < funcTableBuffer->ft_count; i++)
                            {
                                if((funcTableBuffer->ft_list[i].type == MSGpkt->cmd.Data[1])
                                   && (funcTableBuffer->ft_list[i].id == MSGpkt->cmd.Data[2]))
                                {
                                    // 0x03, EndPoint, rCycle
                                    uint8 specialFunc[3] = { 0x03, i + 1, funcTableBuffer->ft_list[i].cycle };
                                    SendData(CONTROL_ENDPOINT, specialFunc, MSGpkt->srcAddr.addr.shortAddr, MSGpkt->srcAddr.endPoint, sizeof(specialFunc));
                                    break;
                                }
                            }
                        }
                        break;
                    default:
                        {
                            int i;
                            for(i = 0; i < funcCount; i++)
                            {
                                struct ep_info_t *ep = &funcList[i];
                                if(ep->res_available)   (*ep->res_available)(ep, ResControlPkg, MSGpkt);
                            }
                        }
                        break;
                    }               
                    HalLedBlink( HAL_LED_1, 1, 50, 250 );
                    break;
                }
                break;
            }
            // Received whenever the device changes state in the network
            case ZDO_STATE_CHANGE:
            {
                devStates_t st = (devStates_t)(MSGpkt->hdr.status);
                if ( (st == DEV_ZB_COORD)
                        || (st == DEV_ROUTER)
                        || (st == DEV_END_DEVICE) )
                {
//                  topoBuffer->type = 0x02;
                    memcpy(topoBuffer.IEEE, NLME_GetExtAddr(), 8);
#if !defined(ZDO_COORDINATOR)
                    topoBuffer.PAddr = NLME_GetCoordShortAddr();
#else                                                             
                    topoBuffer.PAddr = 0xFFFF;
                    uint8 i,TxPower;
                    uint16 SrcSaddr;
                    uint8 *LongAddr;//物理地址
                    //显示协调器的配置信息
                    ClrScreen();                                               
                    PutString_cn(24,0,"网络协调器");
                    TxPower = TXPOWER;
                    HalLcdWriteStringValue( "Send Power:",TxPower, 16, HAL_LCD_LINE_2);                                           
                    SrcSaddr = NLME_GetShortAddr();
                    HalLcdWriteStringValue( "Short Addr:",SrcSaddr, 16, HAL_LCD_LINE_3);
                    LongAddr = NLME_GetExtAddr();  
                    for(i=0;i<8;i++)
                        PutAbyte(i*16,48,LongAddr[i]);                                                                                                                                                                                                
                    for(i=0;i<30;i++){
                        TimeDelay(60000);
                    }
                    ClrScreen();                                                                                               
                    PutString_cn(24,0,"网络协调器");	
                    PutString_cn(0,16,"---------------");	                                                
                    PutString_cn(4,32,"网络已成功建立");	
                    PutString_cn(4,48,"可接收节点加入");	
                    
#endif
                    //向协调器发送拓扑信息
                    SendData(CONTROL_ENDPOINT, (unsigned char *)&topoBuffer, 0x0000, TRANSFER_ENDPOINT, sizeof(TOPOINFO));
                    HalLedBlink( HAL_LED_1, 1, 50, 250 );
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
        return (events ^ SYS_EVENT_MSG);  //返回当前任务待处理事件标志。
    }
    // 定时器时间到, 遍历所有端点看是否有userTimer
    if(events & SAPP_SEND_PERIODIC_MSG_EVT)
    {
        int i;
        uint8 hasUserTimer = 0;
        for(i = 0; i < funcCount; i++)
        {
            struct ep_info_t *ep = &funcList[i];
            if(ep->userTimer && ep->res_available)
            {
                hasUserTimer = 1;
                ep->userTimer = ep->userTimer - 1;
                if(ep->userTimer <= 1)
                {
                    ep->userTimer = 0;
                    (*ep->res_available)(ep, ResUserTimer, NULL);
                }
            }
        }
        if(hasUserTimer)
        {
            // 重新启动定时器
            osal_start_timerEx(task_id, SAPP_SEND_PERIODIC_MSG_EVT, 1000);
        }
        else
        {
            isUserTimerRunning = 0;
            osal_stop_timerEx(task_id, SAPP_SEND_PERIODIC_MSG_EVT);
        }
        // return unprocessed events
        return (events ^ SAPP_SEND_PERIODIC_MSG_EVT);
    }
    // Discard unknown events
    return 0;
}

static uint16 functionEpProcess(uint8 task_id, uint16 events)  //节点接收数据处理
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
            HalLedBlink( HAL_LED_1, 1, 50, 250 );
            //点对点及广播实验                
       /*     {
              switch ( MSGpkt->clusterId )
              {
              case SAPP_PERIODIC_CLUSTERID:
                if(MSGpkt->endPoint <= funcCount)
                {
                  struct ep_info_t *ep = &funcList[MSGpkt->endPoint - 1];
                  if(ep->incoming_data)
                    (*ep->incoming_data)(ep, MSGpkt->srcAddr.addr.shortAddr, MSGpkt->srcAddr.endPoint, &MSGpkt->cmd);
                }
                break;
              }*/
#if defined(ZDO_COORDINATOR)
             //HalUARTWrite(HAL_UART_PORT_0, "get3", 4); 
             SampleApp_MessageMSGCB( MSGpkt );
              break;
#else 
              SampleApp_MessageMSGCB2( MSGpkt );
              break;
#endif

            case ZDO_STATE_CHANGE:
                {
                    curNwkState = (devStates_t)(MSGpkt->hdr.status);
                    if ( (curNwkState == DEV_ZB_COORD)
                            || (curNwkState == DEV_ROUTER)
                            || (curNwkState == DEV_END_DEVICE) )
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
                                               SAPP_SEND_PERIODIC_MSG_EVT,
                                               1000);
                        }
                    }
                    else
                        osal_stop_timerEx(task_id, SAPP_SEND_PERIODIC_MSG_EVT);
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
    if(events & SAPP_SEND_PERIODIC_MSG_EVT)
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
#if 0
            if(ep->userTimer && ep->res_available)
            {
                ep->userTimer = ep->userTimer - 1;
                if(ep->userTimer <= 1)
                {
                    (*ep->res_available)(ep, ResUserTimer, NULL);
                    ep->userTimer = 0;
                }
            }
#endif
        }
        // 重新启动定时器
        osal_start_timerEx(task_id, SAPP_SEND_PERIODIC_MSG_EVT, 1000);
        // return unprocessed events
        return (events ^ SAPP_SEND_PERIODIC_MSG_EVT);
    }
    // Discard unknown events
    return 0;
}

//#if defined(ZDO_COORDINATOR)
#if defined(ZDO_COORDINATOR) || defined(PEER_ROUTER) || defined(PEER_COORD)
#if defined(ZDO_COORDINATOR)
    uint8 SendBuf[10];
#endif
static uint8 uartMsgProcesser(uint8 *msg)
{
    mtOSALSerialData_t *pMsg = (mtOSALSerialData_t *)msg;
    mtUserSerialMsg_t *pMsgBody = (mtUserSerialMsg_t *)pMsg->msg;
    if ( (curNwkState != DEV_ZB_COORD)
            && (curNwkState != DEV_ROUTER)
            && (curNwkState != DEV_END_DEVICE) )
        return 1;   
#if defined(PEER_COORD)
    SendData(CONTROL_ENDPOINT, &msg[4],SrcShortAddr, TRANSFER_ENDPOINT,8);    
    return 1;
#endif

#if defined(PEER_ROUTER)
    SendData(CONTROL_ENDPOINT, &msg[4],0x0000, TRANSFER_ENDPOINT,8);
    return 1;
#endif        
    switch(pMsgBody->cmd)
    {
        case 0x46B9:
        {                  
            switch(pMsgBody->cmdEndPoint)
            {
            case 0xF1:
                {                                 
                    // 转发数据                    
                    /*SendData(TRANSFER_ENDPOINT, pMsgBody->data,
                             pMsgBody->addr, pMsgBody->endPoint,
                             pMsgBody->len - 6);
                    */
                    SendData(TRANSFER_ENDPOINT, pMsgBody->data,
                             0XFFFF, pMsgBody->endPoint,
                             pMsgBody->len - 6);
                }
                break;
#if defined(ZDO_COORDINATOR)                
            case 0xF0:
                HalLedSet ( HAL_LED_1, HAL_LED_MODE_ON );
                HalLedSet ( HAL_LED_2, HAL_LED_MODE_ON );
                Delay100us(5000);                
                HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );
                HalLedSet ( HAL_LED_2, HAL_LED_MODE_OFF );
                Delay100us(5000);                
                HalLedSet ( HAL_LED_1, HAL_LED_MODE_ON );
                HalLedSet ( HAL_LED_2, HAL_LED_MODE_ON );                
                SystemReset();  
                break;
            case 0xEF:   
                SendBuf[0]=0x2;
                SendBuf[1]=0x7;
                SendBuf[2]=0xB9;
                SendBuf[3]=0x46;
                SendBuf[4]=0xEF;
                SendBuf[5]=0x0;
                SendBuf[6]=0x0;
                SendBuf[7]=0x0;
                SendBuf[8]=0x0;
                SendBuf[9]=0x17;
                HalUARTWrite(HAL_UART_PORT_0, &SendBuf[0], 10);    
                break;
#endif                
            }            
        }
        break;
    }
    return 1;
}
#endif

uint8 SendData(uint8 srcEP, const void *buf, uint16 addr, uint8 dstEP, uint8 Len)
{
    static uint8 transID = 0;
    afAddrType_t SendDataAddr;
    struct ep_info_t *epInfo;

    if(srcEP <= funcCount)
        epInfo = &funcList[srcEP - 1];
    else
        epInfo = &controlEndPointInfo;

    SendDataAddr.addrMode = (afAddrMode_t)Addr16Bit;         //短地址发送
    SendDataAddr.endPoint = dstEP;
    SendDataAddr.addr.shortAddr = addr;
    if ( AF_DataRequest( &SendDataAddr, //发送的地址和模式
                         // TODO:
                         &epInfo->SampleApp_epDesc,   //终端（比如操作系统中任务ID等）
                         SAPP_PERIODIC_CLUSTERID,//发送串ID
                         Len,
                         (uint8*)buf,
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

void CreateUserTimer(struct ep_info_t *ep, uint8 seconds)
{
    if(ep == NULL)
        return;
    if(ep->res_available == NULL)
        return;
    ep->userTimer = seconds;
    if(isUserTimerRunning == 0)
    {
        osal_start_timerEx(controlTaskId,
                           SAPP_SEND_PERIODIC_MSG_EVT,
                           1000);
        isUserTimerRunning = 1;
    }
}

void DeleteUserTimer(struct ep_info_t *ep)
{
    if(ep == NULL)
        return;
    ep->userTimer = 0;
}

void ModifyRefreshCycle(struct ep_info_t *ep, uint8 seconds)
{
    if(ep == NULL)
        return;
    if(ep->time_out == NULL)
        return;
    ep->function.cycle = seconds;
    if(ep->timerTick > seconds)
        ep->timerTick = seconds;
}

#if ! defined(ZDO_COORDINATOR) && defined(RTR_NWK) && !defined(PEER_ROUTER)
void RouterTimeoutRoutine(struct ep_info_t *ep)
{    
    SendData(CONTROL_ENDPOINT, (unsigned char *)&topoBuffer, 0x0000, TRANSFER_ENDPOINT, sizeof(TOPOINFO)); //路由节点向协调器发送拓扑信息      
}
#elif defined(PEER_ROUTER)
void RouterTimeoutRoutine(struct ep_info_t *ep)
{
}  
#endif

#if defined(ZDO_COORDINATOR)
void CoordinatorIncomingRoutine(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg)
{
    //msg->Data[], msg->DataLength, msg->TransSeqNumber
    //转发数据到串口  
    if(msg->DataLength > 0)
    {
        mtUserSerialMsg_t *pMsg = osal_mem_alloc(sizeof(mtUserSerialMsg_t) + msg->DataLength - 1);
        pMsg->sop = MT_UART_SOF;
        pMsg->len = msg->DataLength + 6;
        pMsg->cmd = 0x46B9;
        pMsg->cmdEndPoint = 0xF1;
        pMsg->addr = addr;
        SrcShortAddr = addr;//PeerToPeer时使用
        pMsg->endPoint = endPoint;
        if((endPoint == 0xF0)&&(addr !=0)){//为路由包且不为网关路由包
            ClrScreen();                        
            PutString_cn(24,0,"网络协调器");
            PutString_cn(0,16,"---------------");
            PutString_cn(4,32,"新节点加入");
            HalLcdWriteStringValue( "Short Addr:",addr, 16, HAL_LCD_LINE_4);                                 
        }                  
        memcpy(pMsg->data, msg->Data, msg->DataLength);
        pMsg->fsc = MT_UartCalcFCS(0, &pMsg->len, 1);
        pMsg->fsc = MT_UartCalcFCS(pMsg->fsc, pMsg->dataBody, pMsg->len);
        #if (defined(PEER_COORD)||(PEER_ROUTER))//简单无线收发实验
        HalUARTWrite(HAL_UART_PORT_0, &pMsg->data[0], 1);        
        #else
        HalUARTWrite(HAL_UART_PORT_0, &pMsg->sop, sizeof(mtUserSerialMsg_t) - 2 + msg->DataLength);    
        Delay100us(20);  
        HalUARTWrite(HAL_UART_PORT_0, &pMsg->fsc, 1);
        #endif
        osal_mem_free(pMsg);
    }
}
#elif defined(RTR_NWK) || defined(PEER_ROUTER) || defined(LIGHT)
#ifdef LIGHT
uint8 SegValue = 0;
bool MotorValue = 0;
extern const unsigned char seg7table[16];
#endif
void RouterIncomingRoutine(struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg)
{
    //msg->Data[], msg->DataLength, msg->TransSeqNumber
    // 转发数据到串口       
    if(msg->DataLength > 0)
    {                       
        mtUserSerialMsg_t *pMsg = osal_mem_alloc(sizeof(mtUserSerialMsg_t) + msg->DataLength - 1);
        pMsg->sop = MT_UART_SOF;
        pMsg->len = msg->DataLength + 6;
        pMsg->cmd = 0x46B9;
        pMsg->cmdEndPoint = 0xF1;
        pMsg->addr = addr;
        SrcShortAddr = addr;//PeerToPeer时使用
        pMsg->endPoint = endPoint;                
        memcpy(pMsg->data, msg->Data, msg->DataLength);
        pMsg->fsc = MT_UartCalcFCS(0, &pMsg->len, 1);
        pMsg->fsc = MT_UartCalcFCS(pMsg->fsc, pMsg->dataBody, pMsg->len);
        #if defined(PEER_ROUTER)//简单无线收发实验
        HalUARTWrite(HAL_UART_PORT_0, &pMsg->data[0], 1);        
        #elif  defined(LIGHT)//点对点或广播通信实验      
        HalUARTWrite(HAL_UART_PORT_0, &pMsg->data[0], 1);
        //数码管控制
        Sensor_PIN_INT(7);
        P0 |= (0x1<<4);  
        SegValue++;
        if(SegValue>0xf) SegValue=0;      
        P1 =  seg7table[SegValue];
        P0 &= ~(0x1<<4); 
        
        Sensor_PIN_INT(6);
       MotorValue = !MotorValue;
        if(MotorValue == 0)//模拟电机停止
            P0 &= (~(0x1 << 6));

        if(MotorValue == 1)//模拟电机启动
            P0 |= (0x1 << 6);            
        #else
        HalUARTWrite(HAL_UART_PORT_0, &pMsg->sop, sizeof(mtUserSerialMsg_t) - 2 + msg->DataLength);       
        Delay100us(20);
        HalUARTWrite(HAL_UART_PORT_0, &pMsg->fsc, 1);
        #endif
        osal_mem_free(pMsg);
    }
}
#endif

void Delay100us(uint16 Num)
{
  uint16 i;
  uint8 j;
  for(i=0;i<Num;i++){  
    for(j=0;j<120;j++){
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
  }    
}


void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt )
{
 // unsigned char buf[3]; 
  uint8 RCbf[3];
  osal_memset(RCbf, 0 , 3);
  osal_memcpy(RCbf, pckt->cmd.Data, 3);
  
  switch ( pckt->clusterId )
  {
  case SAPP_PERIODIC_CLUSTERID:
    if(pckt->endPoint <= funcCount)
    {
      struct ep_info_t *ep = &funcList[pckt->endPoint - 1];
      if(ep->incoming_data)
        (*ep->incoming_data)(ep, pckt->srcAddr.addr.shortAddr, pckt->srcAddr.endPoint, &pckt->cmd);
    }
    break;
  }
}


void SampleApp_MessageMSGCB2( afIncomingMSGPacket_t *pckt )
{
 // unsigned char buf[3]; 
  uint8 RCbf[3];
  osal_memset(RCbf, 0 , 3);
  osal_memcpy(RCbf, pckt->cmd.Data, 3);
  
  switch ( pckt->clusterId )
  {
       case SAPP_PERIODIC_CLUSTERID:
        /*LED灯 & 风扇*/
        switch(pckt->cmd.Data[0])
        {  
           // case 'S':                                         //风扇
                /*if(pckt->endPoint <= funcCount)
                        {
                            struct ep_info_t *ep = &funcList[pckt->endPoint - 1];
                            if(ep->incoming_data)
                                (*ep->incoming_data)(ep, pckt->srcAddr.addr.shortAddr, pckt->srcAddr.endPoint, &pckt->cmd);
                        }*/
             // P0_6 = ~P0_6;//停止直流电机
             //   break;
                
                case'L':
                  P0DIR = (0x01<<0)|(0x01<<4)|(0x01<<5)|(0x01<<6|(0x01<<7));  // 设置P0_0,P0_4,P0_5,P06,P0_7 为输出方式 
                  P1DIR = 0xff;  // 设置 P1 为输出方式 
                  P2DIR |= 0x01;  // 设置 P2.0 为输出方式 
                  //P0_7 = 0;//关闭蜂鸣器
                  //全亮     
                  P0 |= (0x1 << 6); 
                  P1 = 0xff; 
                  P0 &= ~(0x1 << 6); 
                  
                  P0 |= (0x1 << 5);         
                  P1 = 0x0; 
                  P0 &= ~(0x1 << 5);   
                  break;
                  case'B':
                    P0DIR = (0x01<<0)|(0x01<<4)|(0x01<<5)|(0x01<<6)|(0x01<<7);  // 设置P0_0,P0_4,P0_5,P06,P0_7 为输出方式 
                    P1DIR = 0xff;  // 设置 P1 为输出方式 
                    P2DIR |= 0x01;  // 设置 P2.0 为输出方式 
                   // P0_7 = 0;//关闭蜂鸣器
                    //全灭     
                    P0 |= (0x1 << 6); 
                    P1 = 0x0; 
                    P0 &= ~(0x1 << 6); 
                    break;
                   case'W':
                     P0DIR = (0x01<<0)|(0x01<<4)|(0x01<<5)|(0x01<<6)|(0x01<<7);  // 设置P0_0,P0_4,P0_5,P06,P0_7 为输出方式 
                     P0_7 = 1;//打开蜂鸣器
                    break;
                    case'G':
                      P0DIR = (0x01<<0)|(0x01<<4)|(0x01<<5)|(0x01<<6)|(0x01<<7);  // 设置P0_0,P0_4,P0_5,P06,P0_7 为输出方式 
                      P0_7=0;
                      break;
                     
                     
        }
      break;
  }
}

