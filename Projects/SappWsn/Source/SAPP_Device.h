#ifndef _SAPP_DEVICE_H_
#define _SAPP_DEVICE_H_
#include "SAPP_FrameWork.h"
#ifdef __cplusplus
extern "C"
{
#endif

// ����������ֵ����
enum {
    DevSensorBd0 = 1,
    DevSensorBd1,
    DevSensorBd2,
    DevSensorBd3,
    DevSensorBd4,
    DevSensorBd5,
    DevSensorBd6,
    DevSensorBd7,
    DevSensorBd8,
    DevSensorBd9,
    DevMaxNum
};

#if !defined( ZDO_COORDINATOR ) && !defined(PEER_ROUTER) && !defined(LIGHT) 
// �ڵ㹦�ܶ���
//#define SENSORBOARD0//��������0  ��ʪ�ȴ����������ա��񶯡������Ӧ    
//#define SENSORBOARD1//��������1  �����ƾ���ѹ������ѹ       
//#define SENSORBOARD2//��������2  ��������������ٶ�     
//#define SENSORBOARD3//��������3  ���������������     
//#define SENSORBOARD4//��������4  RFID     
//#define SENSORBOARD5//��������5  CO��CO2����ȩ     
#define SENSORBOARD6//��������6  ���������ģ����     
//#define SENSORBOARD7//��������7  7������ܡ�������������LED   
//#define SENSORBOARD8//��������8  �ĵ硢����     
//#define SENSORBOARD9//��������9  ���⡢���⡢�Ŵ�
#endif

#ifdef __cplusplus
}
#endif
#endif//_SAPP_DEVICE_H_
