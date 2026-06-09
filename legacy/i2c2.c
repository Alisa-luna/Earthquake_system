#include "stm32f10x.h"
#include "Delay.h"

// 引脚定义（用PB6/PB7，和你的OLED一致）
#define I2C_SCL_PIN   GPIO_Pin_8
#define I2C_SDA_PIN   GPIO_Pin_9
#define I2C_PORT      GPIOB

// 宏定义简化操作
#define SCL_HIGH()   GPIO_SetBits(I2C_PORT, I2C_SCL_PIN)
#define SCL_LOW()    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN)
#define SDA_HIGH()   GPIO_SetBits(I2C_PORT, I2C_SDA_PIN)
#define SDA_LOW()    GPIO_ResetBits(I2C_PORT, I2C_SDA_PIN)
#define SDA_READ()   GPIO_ReadInputDataBit(I2C_PORT, I2C_SDA_PIN)

// 延时函数（微秒级）
void I2C_Delay(void)
{
     Delay_us(2);
}

void i2c_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_PORT, &GPIO_InitStructure);
    
    // 初始状态：SCL和SDA都拉高（总线空闲）
    SCL_HIGH();
    SDA_HIGH();
}

void i2c_Start(void)
{
    SDA_HIGH();
    SCL_HIGH();
    I2C_Delay();
    SDA_LOW();      // SDA先拉低
    I2C_Delay();
    SCL_LOW();      // SCL再拉低
}

void i2c_Stop(void)
{
    SDA_LOW();
    I2C_Delay();
    SCL_HIGH();     // SCL先拉高
    I2C_Delay();
    SDA_HIGH();     // SDA再拉高
    I2C_Delay();
}

// ===== 修改1：发送字节带超时 =====
uint8_t i2c_SendByte(uint8_t Byte)
{
    uint8_t i;
    uint32_t timeout;
    
    // 发送8位数据（高位在前）
    for(i = 0; i < 8; i++)
    {
        if(Byte & 0x80)  // 最高位
            SDA_HIGH();
        else
            SDA_LOW();
        
        I2C_Delay();
        SCL_HIGH();
        I2C_Delay();
        SCL_LOW();
        
        Byte <<= 1;
    }
    
    // 释放SDA，准备接收应答
    SDA_HIGH();
    I2C_Delay();
    SCL_HIGH();
    I2C_Delay();
    
    // ===== 关键修改：等待应答时加超时 =====
    timeout = 10000;
    while(SDA_READ()) {  // 等待从机拉低SDA（应答）
        if(--timeout == 0) {
            SCL_LOW();
            return 1;  // 超时，返回非应答
        }
    }
    
    SCL_LOW();
    return 0;  // 收到应答
}

uint8_t i2c_ReceiveByte(uint8_t ack)
{
    uint8_t i, Byte = 0;
    
    // 释放SDA，由从机控制
    SDA_HIGH();
    
    for(i = 0; i < 8; i++)
    {
        Byte <<= 1;  // 左移，为新位腾出空间
        
        SCL_HIGH();
        I2C_Delay();
        
        if(SDA_READ())
            Byte |= 1;  // 读取到1
        
        SCL_LOW();
        I2C_Delay();
    }
    
    // 发送应答/非应答
    if(ack)
        SDA_HIGH();  // 非应答
    else
        SDA_LOW();   // 应答
    
    I2C_Delay();
    SCL_HIGH();
    I2C_Delay();
    SCL_LOW();
    
    // 释放SDA
    SDA_HIGH();
    
    return Byte;
}

uint8_t MPU6050_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    i2c_Start();
    
    // 发送设备地址（写）
    if(i2c_SendByte(dev_addr & 0xFE))
    {
        i2c_Stop();
        return 1;
    }
    
    // 发送寄存器地址
    if(i2c_SendByte(reg_addr))
    {
        i2c_Stop();
        return 1;
    }
    
    // 发送数据
    if(i2c_SendByte(data))
    {
        i2c_Stop();
        return 1;
    }
    
    i2c_Stop();
    return 0;
}

// ===== 修改2：读寄存器失败时返回0 =====
uint8_t MPU6050_ReadReg(uint8_t dev_addr, uint8_t reg_addr)
{
    uint8_t data = 0x00;  // 改为0
    
    i2c_Start();
    
    if(i2c_SendByte(dev_addr & 0xFE))
    {
        i2c_Stop();
        return 0x00;  // 返回0
    }
    
    if(i2c_SendByte(reg_addr))
    {
        i2c_Stop();
        return 0x00;
    }
    
    i2c_Start();
    
    if(i2c_SendByte(dev_addr | 0x01))
    {
        i2c_Stop();
        return 0x00;
    }
    
    data = i2c_ReceiveByte(1);
    i2c_Stop();
    
    return data;
}

void MPU6050_Init(void)
{
    i2c_Init();
    Delay_ms(100);
    
    // 唤醒MPU6050（退出睡眠模式）
    MPU6050_WriteReg(0xD0, 0x6B, 0x00);
    Delay_ms(100);
    
    // 设置陀螺仪量程 ±2000°/s
    MPU6050_WriteReg(0xD0, 0x1B, 0x18);
    
    // 设置加速度计量程 ±8g
    MPU6050_WriteReg(0xD0, 0x1C, 0x10);
    
    // 设置采样率分频器
    MPU6050_WriteReg(0xD0, 0x19, 0x07);
    
    // 设置DLPF（数字低通滤波器）
    MPU6050_WriteReg(0xD0, 0x1A, 0x06);
}

uint8_t MPU6050_Test(void)
{
    return MPU6050_ReadReg(0xD0, 0x75);
}
