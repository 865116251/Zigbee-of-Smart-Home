/**************************************************************************************************
  Filename:       SampleApp.h
  Revised:        $Date: 2007-10-27 17:22:23 -0700 (Sat, 27 Oct 2007) $
  Revision:       $Revision: 15795 $

  Description:    This file contains the Sample Application definitions.


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

#ifndef SAMPLEAPP_H
#define SAMPLEAPP_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "ZDApp.h"
#include "ZComDef.h"
#include "OSAL_Tasks.h"
#include "aps_groups.h"

/*********************************************************************
 * CONSTANTS
 */

enum {
    DevTemp = 1,
    DevHumm,
    DevILLum,
    DevRain,
    DevIRDist,
    DevGas,
    DevSmoke,
    DevFire,
    DevIRPers,
    DevVoice,
    DevExecuteB,
    DevExecuteA,
    DevRemoter,
    DevRouter = 240,
    DevMaxNum
};
  
// These constants are only for example and should be changed to the
// device's needs
#define CONTROL_ENDPOINT             0xF0

#define SAMPLEAPP_PROFID             0x0F08
#define SAMPLEAPP_DEVICEID           0x0001
#define SAMPLEAPP_DEVICE_VERSION     0
#define SAMPLEAPP_FLAGS              0

#define SAMPLEAPP_MAX_CLUSTERS       2
#define SAMPLEAPP_PERIODIC_CLUSTERID 1
#define SAMPLEAPP_FLASH_CLUSTERID     2

// Application Events (OSAL) - These are bit weighted definitions.
#define SAMPLEAPP_SEND_PERIODIC_MSG_EVT       0x0001

#if !defined( ZDO_COORDINATOR ) && !defined( RTR_NWK )
//节点宏定义列表
//#define SAFETY
#define LIGHT
//#define TEMP_HUM
#define GAS
//#define SMOKE
//#define FIRE
//#define VOICE
//#define RAIN
//#define DIST
#define CONTROL
#endif

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */
typedef struct topo_info_t {
    uint8 type;
    uint8 IEEE[8];
    uint16 PAddr;
} TOPOINFO;

typedef struct func_info_t {
    uint8 type;
    uint8 id;
    uint8 cycle;
} FUNCINFO;
typedef union {
    uint8 ft_data[2];
    struct {
        uint8 ft_type;
        uint8 ft_count;
        FUNCINFO ft_list[0];
    } ft_field;
} FUNCTABLE;
#define ft_type ft_field.ft_type
#define ft_count ft_field.ft_count
#define ft_list ft_field.ft_list
#define createFuncTable(count)  (FUNCTABLE *)osal_mem_alloc(2 + count * sizeof(FUNCINFO))
#define destroyFuncTable(ft)    osal_mem_free(ft)

struct ep_info_t {
    // 网络状态发生变化时会调用该函数
    void (*nwk_stat_change)(const struct ep_info_t *ep);
    // 接收到数据时会调用该函数
    void (*incoming_data)(const struct ep_info_t *ep, uint16 addr, uint8 endPoint, afMSGCommandFormat_t *msg);
    // when time out, this function will be called
    void (*time_out)(const struct ep_info_t *ep);
    // NOTE: cycle成员会被用来计数,并周期性调用time_out函数
    struct func_info_t function;
  
    // 当前端点号
    uint8 ep;
    // 与此端点绑定的任务ID
    uint8 task_id;
    // 递减计数,为0时调用time_out函数,并重载初值=cycle
    uint8 timerTick;
    endPointDesc_t SampleApp_epDesc;
    SimpleDescriptionFormat_t simpleDesc;
};
#if !defined(ZDO_COORDINATOR)
extern struct ep_info_t funcList[];
extern const uint8 funcCount;
extern uint8 ctrlBuffer[];
extern TOPOINFO *topoBuffer;
extern FUNCTABLE *funcTableBuffer;
#endif
/*
 * Task Initialization for the Generic Application
 */
//extern void SampleApp_Init( uint8 ep, uint8 task_id );

/*
 * Task Event Processor for the Generic Application
 */
extern void taskInitProcess(void);
extern uint16 controlEpProcess(uint8 task_id, uint16 events);
extern UINT16 functionEpProcess( uint8 task_id, uint16 events );

extern void createEndPoint(struct ep_info_t *epInfo, uint8 *task_id, uint8 ep);
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SAMPLEAPP_H */
