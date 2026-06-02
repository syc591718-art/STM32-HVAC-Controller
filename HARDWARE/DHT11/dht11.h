#ifndef __DHT11_H
#define __DHT11_H 
#include "sys.h"   

/* DHT11 引脚定义：PG9 */
#define DHT11_DQ_OUT PGout(9) // 数据输出
#define DHT11_DQ_IN  PGin(9)  // 数据输入

/* * STM32F407 引脚模式快速切换寄存器操作 
 * 00:输入, 01:输出 (针对PG9，操作MODER寄存器的第18,19位)
 */
#define DHT11_IO_IN()  {GPIOG->MODER&=~(3<<(9*2));GPIOG->MODER|=0<<9*2;}	//脱离输出模式，设为输入
#define DHT11_IO_OUT() {GPIOG->MODER&=~(3<<(9*2));GPIOG->MODER|=1<<9*2;} 	//设为通用输出模式

/* 函数声明 */
uint8_t DHT11_Init(void);			//初始化DHT11
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi); //读取温湿度数据
uint8_t DHT11_Read_Byte(void);		//读一个字节
uint8_t DHT11_Read_Bit(void);		//读一个位
uint8_t DHT11_Check(void);			//检测DHT11
void DHT11_Rst(void);				//复位DHT11    
#endif
