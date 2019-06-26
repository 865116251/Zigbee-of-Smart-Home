#ifndef _SAPP_DEVICE_H_
#define _SAPP_DEVICE_H_
#include "SAPP_FrameWork.h"
#ifdef __cplusplus
extern "C"
{
#endif

// 传感器类型值定义
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
    Dev125kReader,
    DevMaxNum
};

#if !defined( ZDO_COORDINATOR ) && !defined( RTR_NWK )
// 节点功能定义
//#define HAS_GAS
//#define HAS_TEMP
//#define HAS_HUMM
//#define HAS_RAIN
//#define HAS_FIRE
//#define HAS_SMOKE
//#define HAS_ILLUM
//#define HAS_IRPERS
//#define HAS_IRDIST
//#define HAS_VOICE           // 修改HAL_UART_DMA的定义为2
//#define HAS_EXECUTEB
//#define HAS_EXECUTEA
//#define HAS_REMOTER
//#define HAS_TESTFUNCTION
#define HAS_125KREADER
#endif

#ifdef __cplusplus
}
#endif
#endif//_SAPP_DEVICE_H_
