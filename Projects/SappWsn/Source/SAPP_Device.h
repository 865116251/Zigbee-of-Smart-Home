#ifndef _SAPP_DEVICE_H_
#define _SAPP_DEVICE_H_
#include "SAPP_FrameWork.h"
#ifdef __cplusplus
extern "C"
{
#endif

// 传感器类型值定义
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
// 节点功能定义
//#define SENSORBOARD0//传感器板0  温湿度传感器、光照、振动、人体感应    
//#define SENSORBOARD1//传感器板1  烟雾、酒精、压力、气压       
//#define SENSORBOARD2//传感器板2  超声波、三轴加速度     
//#define SENSORBOARD3//传感器板3  霍尔传感器、雨滴     
//#define SENSORBOARD4//传感器板4  RFID     
//#define SENSORBOARD5//传感器板5  CO、CO2、甲醛     
#define SENSORBOARD6//传感器板6  步进电机、模拟电机     
//#define SENSORBOARD7//传感器板7  7段数码管、蜂鸣器、矩阵LED   
//#define SENSORBOARD8//传感器板8  心电、脉搏     
//#define SENSORBOARD9//传感器板9  红外、紫外、门磁
#endif

#ifdef __cplusplus
}
#endif
#endif//_SAPP_DEVICE_H_
