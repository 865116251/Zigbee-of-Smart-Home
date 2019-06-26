#ifndef _HELLO_
#define _HELLO_

#include "hal_uart.h"
#include "hal_types.h"
#include "OSAL_Timers.h"
#include "comdef.h"

void Hello_Init(uint8 task_id);
uint16 Hello_ProcessEvent(uint8 task_id, uint16 events);

#endif

