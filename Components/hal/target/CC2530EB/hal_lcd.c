/**************************************************************************************************
  Filename:       hal_lcd.c
  Revised:        $Date: 2009-03-12 16:25:22 -0700 (Thu, 12 Mar 2009) $
  Revision:       $Revision: 19404 $

  Description:    This file contains the interface to the HAL LCD Service.


  Copyright 2007 - 2009 Texas Instruments Incorporated. All rights reserved.

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

/**************************************************************************************************
 *                                           INCLUDES
 **************************************************************************************************/
#include "hal_types.h"
#include "hal_lcd.h"
#include "OSAL.h"
#include "OnBoard.h"
#include "hal_assert.h"
#include "lcd_dis.h"
#include "hal_led.h"
#define HAL_LCD TRUE
#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
  #include "DebugTrace.h"
#endif

/**************************************************************************************************
 *                                          CONSTANTS
 **************************************************************************************************/
/*
  LCD pins

  //control
  P0.0 - LCD_MODE
  P1.1 - LCD_FLASH_RESET
  P1.2 - LCD_CS

  //spi
  P1.5 - CLK
  P1.6 - MOSI
  P1.7 - MISO
*/

/* LCD Control lines */
#define HAL_LCD_MODE_PORT 0
#define HAL_LCD_MODE_PIN  0

#define HAL_LCD_RESET_PORT 1
#define HAL_LCD_RESET_PIN  1

#define HAL_LCD_CS_PORT 1
#define HAL_LCD_CS_PIN  2

/* LCD SPI lines */
#define HAL_LCD_CLK_PORT 1
#define HAL_LCD_CLK_PIN  5

#define HAL_LCD_MOSI_PORT 1
#define HAL_LCD_MOSI_PIN  6

#define HAL_LCD_MISO_PORT 1
#define HAL_LCD_MISO_PIN  7

/* SPI settings */
#define HAL_SPI_CLOCK_POL_LO       0x00
#define HAL_SPI_CLOCK_PHA_0        0x00
#define HAL_SPI_TRANSFER_MSB_FIRST 0x20

/* LCD lines */
#define LCD_MAX_LINE_COUNT              3
#define LCD_MAX_LINE_LENGTH             16
#define LCD_MAX_BUF                     25

/* Defines for HW LCD */

/* Set power save mode */
#define OSC_OFF                         0x00
#define OSC_ON                          0x01
#define POWER_SAVE_OFF                  0x00
#define POWER_SAVE_ON                   0x02
#define SET_POWER_SAVE_MODE(options)    HalLcd_HW_Control(0x0C | (options))

/* Function Set */
#define CGROM                           0x00
#define CGRAM                           0x01
#define COM_FORWARD                     0x00
#define COM_BACKWARD                    0x02
#define TWO_LINE                        0x00
#define THREE_LINE                      0x04
#define FUNCTION_SET(options)           HalLcd_HW_Control(0x10 | (options))

/* Set Display Start Line */
#define SET_DISPLAY_START_LINE(line)    HalLcd_HW_Control(0x18 | (line))

/* Bias control */
#define BIAS_1_5                        0x00
#define BIAS_1_4                        0x01
#define SET_BIAS_CTRL(bias)             HalLcd_HW_Control(0x1C | (bias))

/* Power control */
#define VOLTAGE_DIVIDER_OFF             0x00
#define VOLTAGE_DIVIDER_ON              0x01
#define CONVERTER_AND_REG_OFF           0x00
#define CONVERTER_AND_REG_ON            0x04
#define SET_POWER_CTRL(options)         HalLcd_HW_Control(0x20 | (options))

// Set display control
#define DISPLAY_CTRL_ON                 0x01
#define DISPLAY_CTRL_OFF                0x00
#define DISPLAY_CTRL_BLINK_ON           0x02
#define DISPLAY_CTRL_BLINK_OFF          0x00
#define DISPLAY_CTRL_CURSOR_ON          0x04
#define DISPLAY_CTRL_CURSOR_OFF         0x00
#define SET_DISPLAY_CTRL(options)       HalLcd_HW_Control(0x28 | (options))

/* Set DD/ CGRAM address */
#define SET_DDRAM_ADDR(charIndex)       HalLcd_HW_Control(0x80 | (charIndex))
#define SET_GCRAM_CHAR(specIndex)       HalLcd_HW_Control(0xC0 | (specIndex))

/* Set ICONRAM address */
#define CONTRAST_CTRL_REGISTER          0x10
#define SET_ICONRAM_ADDR(addr)          HalLcd_HW_Control(0x40 | (addr))

/* Set double height */
#define LINE_1_AND_2                    0x01
#define LINE_2_AND_3                    0x02
#define NORMAL_DISPLAY                  0x00
#define SET_DOUBLE_HEIGHT(options)      HalLcd_HW_Control(0x08 | (options))

/**************************************************************************************************
 *                                           MACROS
 **************************************************************************************************/

#define HAL_IO_SET(port, pin, val)        HAL_IO_SET_PREP(port, pin, val)
#define HAL_IO_SET_PREP(port, pin, val)   st( P##port##_##pin## = val; )

#define HAL_CONFIG_IO_OUTPUT(port, pin, val)      HAL_CONFIG_IO_OUTPUT_PREP(port, pin, val)
#define HAL_CONFIG_IO_OUTPUT_PREP(port, pin, val) st( P##port##SEL &= ~BV(pin); \
                                                      P##port##_##pin## = val; \
                                                      P##port##DIR |= BV(pin); )

#define HAL_CONFIG_IO_PERIPHERAL(port, pin)      HAL_CONFIG_IO_PERIPHERAL_PREP(port, pin)
#define HAL_CONFIG_IO_PERIPHERAL_PREP(port, pin) st( P##port##SEL |= BV(pin); )



/* SPI interface control */
#define LCD_SPI_BEGIN()     HAL_IO_SET(HAL_LCD_CS_PORT,  HAL_LCD_CS_PIN,  0); /* chip select */
#define LCD_SPI_END()                                                         \
{                                                                             \
  asm("NOP");                                                                 \
  asm("NOP");                                                                 \
  asm("NOP");                                                                 \
  asm("NOP");                                                                 \
  HAL_IO_SET(HAL_LCD_CS_PORT,  HAL_LCD_CS_PIN,  1); /* chip select */         \
}
/* clear the received and transmit byte status, write tx data to buffer, wait till transmit done */
#define LCD_SPI_TX(x)                   { U1CSR &= ~(BV(2) | BV(1)); U1DBUF = x; while( !(U1CSR & BV(1)) ); }
#define LCD_SPI_WAIT_RXRDY()            { while(!(U1CSR & BV(1))); }


/* Control macros */
#define LCD_DO_WRITE()        HAL_IO_SET(HAL_LCD_MODE_PORT,  HAL_LCD_MODE_PIN,  1);
#define LCD_DO_CONTROL()      HAL_IO_SET(HAL_LCD_MODE_PORT,  HAL_LCD_MODE_PIN,  0);

#define LCD_ACTIVATE_RESET()  HAL_IO_SET(HAL_LCD_RESET_PORT, HAL_LCD_RESET_PIN, 0);
#define LCD_RELEASE_RESET()   HAL_IO_SET(HAL_LCD_RESET_PORT, HAL_LCD_RESET_PIN, 1);


/**************************************************************************************************
 *                                       GLOBAL VARIABLES
 **************************************************************************************************/
uint8 *Lcd_Line1;

/**************************************************************************************************
 *                                       FUNCTIONS - API
 **************************************************************************************************/
#if (HAL_LCD == TRUE)
void HalLcd_HW_Init(void);
void HalLcd_HW_WaitUs(uint16 i);
void HalLcd_HW_Clear(void);
void HalLcd_HW_ClearAllSpecChars(void);
void HalLcd_HW_Control(uint8 cmd);
void HalLcd_HW_Write(uint8 data);
void HalLcd_HW_SetContrast(uint8 value);
void HalLcd_HW_WriteChar(uint8 line, uint8 col, char text);
void HalLcd_HW_WriteLine(uint8 line, const char *pText);
#endif //LCD

/**************************************************************************************************
 * @fn      HalLcdInit
 *
 * @brief   Initilize LCD Service
 *
 * @param   init - pointer to void that contains the initialized value
 *
 * @return  None
 **************************************************************************************************/
void HalLcdInit(void)
{
  Lcd_Line1 = NULL;
#if (HAL_LCD == TRUE)
  HalLcd_HW_Init();
#endif
}

/*************************************************************************************************
 *                    LCD EMULATION FUNCTIONS
 *
 * Some evaluation boards are equipped with Liquid Crystal Displays
 * (LCD) which may be used to display diagnostic information. These
 * functions provide LCD emulation, sending the diagnostic strings
 * to Z-Tool via the RS232 serial port. These functions are enabled
 * when the "LCD_SUPPORTED" compiler flag is placed in the makefile.
 *
 * Most applications update both lines (1 and 2) of the LCD whenever
 * text is posted to the device. This emulator assumes that line 1 is
 * updated first (saved locally) and the formatting and send operation
 * is triggered by receipt of line 2. Nothing will be transmitted if
 * only line 1 is updated.
 *
 *************************************************************************************************/


/**************************************************************************************************
 * @fn      HalLcdWriteString
 *
 * @brief   Write a string to the LCD
 *
 * @param   str    - pointer to the string that will be displayed
 *          option - display options
 *
 * @return  None
 **************************************************************************************************/
void HalLcdWriteString ( char *str, uint8 option)
{
#if (HAL_LCD == TRUE)

  uint8 strLen = 0;
  uint8 totalLen = 0;
  uint8 *buf;
  uint8 tmpLen;
  
  strLen = (uint8)osal_strlen( (char*)str );

  /* Check boundries */
  if ( strLen > HAL_LCD_MAX_CHARS )
    strLen = HAL_LCD_MAX_CHARS;

  if ( option == HAL_LCD_LINE_1 )
  {
    /* Line 1 gets saved for later */
    osal_memcpy( Lcd_Line1, str, strLen );
    Lcd_Line1[strLen] = '\0';
  }
  else
  {
    /* Line 2 triggers action */
    tmpLen = (uint8)osal_strlen( (char*)Lcd_Line1 );
    totalLen =  tmpLen + 1 + strLen + 1;
    buf = osal_mem_alloc( totalLen );
    if ( buf != NULL )
    {
      /* Concatenate strings */
      osal_memcpy( buf, Lcd_Line1, tmpLen );
      buf[tmpLen++] = ' ';
      osal_memcpy( &buf[tmpLen], str, strLen );
      buf[tmpLen+strLen] = '\0';

      /* Send it out */
#if defined (ZTOOL_P1) || defined (ZTOOL_P2)

#if defined(SERIAL_DEBUG_SUPPORTED)
      debug_str( (uint8*)buf );
#endif //LCD_SUPPORTED

#endif //ZTOOL_P1

      /* Free mem */
      osal_mem_free( buf );
    }
  }

  /* Display the string */
  HalLcd_HW_WriteLine (option, str);

#endif //HAL_LCD

}

/**************************************************************************************************
 * @fn      HalLcdWriteValue
 *
 * @brief   Write a value to the LCD
 *
 * @param   value  - value that will be displayed
 *          radix  - 8, 10, 16
 *          option - display options
 *
 * @return  None
 **************************************************************************************************/
void HalLcdWriteValue ( uint32 value, const uint8 radix, uint8 option)
{
#if (HAL_LCD == TRUE)
  uint8 buf[LCD_MAX_BUF];

  _ltoa( value, &buf[0], radix );
  HalLcdWriteString( (char*)buf, option );
#endif
}

/**************************************************************************************************
 * @fn      HalLcdWriteScreen
 *
 * @brief   Write a value to the LCD
 *
 * @param   line1  - string that will be displayed on line 1
 *          line2  - string that will be displayed on line 2
 *
 * @return  None
 **************************************************************************************************/
void HalLcdWriteScreen( char *line1, char *line2 )
{
#if (HAL_LCD == TRUE)
  HalLcdWriteString( line1, 1 );
  HalLcdWriteString( line2, 2 );
#endif
}

/**************************************************************************************************
 * @fn      HalLcdWriteStringValue
 *
 * @brief   Write a string followed by a value to the LCD
 *
 * @param   title  - Title that will be displayed before the value
 *          value  - value
 *          format - redix
 *          line   - line number
 *
 * @return  None
 **************************************************************************************************/
void HalLcdWriteStringValue( char *title, uint16 value, uint8 format, uint8 line )
{
#if (HAL_LCD == TRUE)
  uint8 tmpLen;
  uint8 buf[LCD_MAX_BUF];
  uint32 err;

  tmpLen = (uint8)osal_strlen( (char*)title );
  osal_memcpy( buf, title, tmpLen );
  buf[tmpLen] = ' ';
  err = (uint32)(value);
  _ltoa( err, &buf[tmpLen+1], format );
  HalLcdWriteString( (char*)buf, line );		
#endif
}

/**************************************************************************************************
 * @fn      HalLcdWriteStringValue
 *
 * @brief   Write a string followed by a value to the LCD
 *
 * @param   title   - Title that will be displayed before the value
 *          value1  - value #1
 *          format1 - redix of value #1
 *          value2  - value #2
 *          format2 - redix of value #2
 *          line    - line number
 *
 * @return  None
 **************************************************************************************************/
void HalLcdWriteStringValueValue( char *title, uint16 value1, uint8 format1,
                                  uint16 value2, uint8 format2, uint8 line )
{

#if (HAL_LCD == TRUE)

  uint8 tmpLen;
  uint8 buf[LCD_MAX_BUF];
  uint32 err;

  tmpLen = (uint8)osal_strlen( (char*)title );
  if ( tmpLen )
  {
    osal_memcpy( buf, title, tmpLen );
    buf[tmpLen++] = ' ';
  }

  err = (uint32)(value1);
  _ltoa( err, &buf[tmpLen], format1 );
  tmpLen = (uint8)osal_strlen( (char*)buf );

  buf[tmpLen++] = ',';
  buf[tmpLen++] = ' ';
  err = (uint32)(value2);
  _ltoa( err, &buf[tmpLen], format2 );

  HalLcdWriteString( (char *)buf, line );		

#endif
}

/**************************************************************************************************
 * @fn      HalLcdDisplayPercentBar
 *
 * @brief   Display percentage bar on the LCD
 *
 * @param   title   -
 *          value   -
 *
 * @return  None
 **************************************************************************************************/
void HalLcdDisplayPercentBar( char *title, uint8 value )
{
#if (HAL_LCD == TRUE)

  uint8 percent;
  uint8 leftOver;
  uint8 buf[17];
  uint32 err;
  uint8 x;

  /* Write the title: */
  HalLcdWriteString( title, HAL_LCD_LINE_1 );

  if ( value > 100 )
    value = 100;

  /* convert to blocks */
  percent = (uint8)(value / 10);
  leftOver = (uint8)(value % 10);

  /* Make window */
  osal_memcpy( buf, "[          ]  ", 15 );

  for ( x = 0; x < percent; x ++ )
  {
    buf[1+x] = '>';
  }

  if ( leftOver >= 5 )
    buf[1+x] = '+';

  err = (uint32)value;
  _ltoa( err, (uint8*)&buf[13], 10 );

  HalLcdWriteString( (char*)buf, HAL_LCD_LINE_2 );

#endif

}


#if (HAL_LCD == TRUE)
/**************************************************************************************************
 *                                    HARDWARE LCD
 **************************************************************************************************/

/**************************************************************************************************

 * @fn      HalLcd_HW_Init
 *
 * @brief   Initilize HW LCD Driver.
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalLcd_HW_Init(void)
{ 
        unsigned int i;
        LCD_Init();
        //物联网教学系统
        //CVT-WSN-II
        //武汉创维特公司
        //cvtech.com.cn
	SetBackLight(127);		//设置背光的亮度等级为最亮127,0为最暗
	ClrScreen();                
        FontSet_cn(1,1);                //16x16	
	PutString_cn(8,0,"物联网教学系统");	
        FontSet(1,1); 			  //选择ASCII字符1(8X16),字符色为1(黑色)
        //PutString(24,16,"CVT-WSN-II");  //在指定位置开始显示ASCII字符串
        //PutString(24,16,"CVT-WSN-III"); //在指定位置开始显示ASCII字符串
        //PutString(24,16,"CVT-WSN-IV");  //在指定位置开始显示ASCII字符串        
        //PutString(24,16,"CVT-WSN-V");	  //在指定位置开始显示ASCII字符串
        PutString(24,16,"CVT-IOT-V");	  //在指定位置开始显示ASCII字符串
        FontSet_cn(1,1);                  //16x16
        PutString_cn(8,32,"武汉创维特公司");	
        FontSet(1,1); 			//选择ASCII字符1(8X16),字符色为1(黑色)
        PutString(12,48,"cvtech.com.cn");//在指定位置开始显示ASCII字符串
        for(i=0;i<30;i++){
          TimeDelay(60000);
        }
        ClrScreen();
}

/**************************************************************************************************
 * @fn          HalLcd_HW_SetContrast
 *
 * @brief       Set display contrast
 *
 * @param       uint8 value - contrast value
 *
 * @return      none
 **************************************************************************************************/
/*void HalLcd_HW_SetContrast(uint8 value)
{
  SET_ICONRAM_ADDR(CONTRAST_CTRL_REGISTER);
  HalLcd_HW_Write(value);
}*/

/**************************************************************************************************
 * @fn      HalLcd_HW_Clear
 *
 * @brief   Clear the HW LCD
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
/*void HalLcd_HW_Clear(void)
{
  uint8 n;

  SET_DDRAM_ADDR(0x00);
  for (n = 0; n < (LCD_MAX_LINE_COUNT * LCD_MAX_LINE_LENGTH); n++)
  {
    HalLcd_HW_Write(' ');
  }
}*/

/**************************************************************************************************
 * @fn      HalLcd_HW_ClearAllSpecChars
 *
 * @brief   Clear all special chars
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
/*void HalLcd_HW_ClearAllSpecChars(void)
{
  uint8 n = 0;

  SET_GCRAM_CHAR(0);
  for (n = 0; n < (8 * 8); n++)
  {
    HalLcd_HW_Write(0x00);
  }
}*/

/**************************************************************************************************
 * @fn      HalLcd_HW_WriteChar
 *
 * @brief   Write one char to the display
 *
 * @param   uint8 line - line number that the char will be displayed
 *          uint8 col - colum where the char will be displayed
 *
 * @return  None
 **************************************************************************************************/
void HalLcd_HW_WriteChar(uint8 line, uint8 col, char text)
{
  if (col < LCD_MAX_LINE_LENGTH)
  {
    PutChar(col*8,(line - 1)*16,text) ;     
  }
  else
  {
    return;
  }
}

/**************************************************************************************************
 * @fn          halLcdWriteLine
 *
 * @brief       Write one line on display
 *
 * @param       uint8 line - display line
 *              char *pText - text buffer to write
 *
 * @return      none
 **************************************************************************************************/
void HalLcd_HW_WriteLine(uint8 line, const char *pText)
{
  uint8 count;
  uint8 totalLength = (uint8)osal_strlen( (char *)pText );

  /* Write the content first */
  for (count=0; count<totalLength; count++)
  {
    HalLcd_HW_WriteChar(line, count, (*(pText++)));
  }

  // Write blank spaces to rest of the line 
  for(count=totalLength; count<LCD_MAX_LINE_LENGTH;count++)
  {
    HalLcd_HW_WriteChar(line, count, ' ');
  }
}

/**************************************************************************************************
 * @fn      HalLcd_HW_WaitUs
 *
 * @brief   wait for x us. @ 32MHz MCU clock it takes 32 "nop"s for 1 us delay.
 *
 * @param   x us. range[0-65536]
 *
 * @return  None
 **************************************************************************************************/
/*void HalLcd_HW_WaitUs(uint16 microSecs)
{
  while(microSecs--)
  {
    // 32 NOPs == 1 usecs
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop");
  }
}*/
#endif


/**************************************************************************************************
**************************************************************************************************/



