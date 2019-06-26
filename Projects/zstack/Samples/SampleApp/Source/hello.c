#include "hello.h"

uint8 HelloID;

void Hello_Init(uint8 task_id){
  HelloID = task_id;
  osal_start_timerEx( task_id, SYS_EVENT_MSG, 1000 );
}

uint16 Hello_ProcessEvent(uint8 task_id, uint16 events){
  if(events & SYS_EVENT_MSG){
    HalUARTWrite ( HAL_UART_PORT_0, "Hello world\r\n", 13);
    osal_start_timerEx( task_id, SYS_EVENT_MSG, 1000 );
  }
  return(events ^ SYS_EVENT_MSG);
}
