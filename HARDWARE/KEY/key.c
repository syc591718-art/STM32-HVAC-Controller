#include "key.h"
#include "delay.h"

//按键初始化函数
void KEY_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE); //使能GPIOE时钟

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4; //KEY0 KEY1 KEY2
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;      //普通输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;      //上拉
    GPIO_Init(GPIOE, &GPIO_InitStructure);            //初始化GPIOE
}

//按键处理函数
//mode:0,不支持连续按; 1,支持连续按;
//返回值：0，没有任何按键按下; 1，KEY0按下; 2，KEY1按下; 3，KEY2按下
uint8_t KEY_Scan(uint8_t mode)
{	 
	static uint8_t key_up=1;//按键按松开标志
	if(mode)key_up=1;  //支持连按		  
	if(key_up&&(KEY0==0||KEY1==0||KEY2==0))
	{
		delay_ms(10); //去抖动 
		key_up=0;
		if(KEY0==0)return KEY0_PRES;
		else if(KEY1==0)return KEY1_PRES;
		else if(KEY2==0)return KEY2_PRES;
	}else if(KEY0==1&&KEY1==1&&KEY2==1)key_up=1; 	    
 	return 0; //无按键按下
}