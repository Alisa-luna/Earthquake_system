#include "stm32f10x.h"                  // Device header
#ifndef __I2C2_H
#define __I2C2_H

#include "stm32f10x.h"

// I2C初始化
void i2c_Init(void);
void i2c_Start(void);
void i2c_Stop(void);

// 发送/接收字节
uint8_t i2c_SendByte(uint8_t Byte);
uint8_t i2c_ReceiveByte(uint8_t ack);

// MPU6050操作函数
uint8_t MPU6050_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
uint8_t MPU6050_ReadReg(uint8_t dev_addr, uint8_t reg_addr);
void MPU6050_Init(void);
uint8_t MPU6050_Test(void);

#endif
