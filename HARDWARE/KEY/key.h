#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h" 

/* 按键引脚定义 */
#define KEY0 		GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_2) //PE2
#define KEY1 		GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_3) //PE3
#define KEY2 		GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_4) //PE4

/* 按键按下标志位 */
#define KEY0_PRES 	1
#define KEY1_PRES	2
#define KEY2_PRES	3

/* 函数声明 */
void KEY_Init(void);	//IO初始化
uint8_t KEY_Scan(uint8_t mode);  	//按键扫描函数					    
#endif