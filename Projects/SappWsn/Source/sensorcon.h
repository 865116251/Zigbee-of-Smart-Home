#ifndef SENSORCON_H
#define SENSORCON_H

#define 	SensorPowerOn()		P1 &= 0xf7
#define 	SensorPowerOff()	P1 |= 0x08

#define		ACC_NOMAL()		P1 |= 0x04
#define		ACC_SLEEP()		P1 &= 0xFB


#define		ADC_GM		0
#define		ACC_X		1
#define		ACC_Y		6
#define		ACC_Z		7

extern void Sensor_PIN_INT(uint8);
extern uint8 ReadTc77(void);
extern void SET_ADC_IO_SLEEP_MODE(void);
extern void SET_ADC_IO_ADC_MODE(void);

#endif