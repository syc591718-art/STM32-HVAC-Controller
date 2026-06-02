#include "led.h" 


void LED_Init(void)
{    	 
  GPIO_InitTypeDef  GPIO_InitStructure;

  // 1. 使能 GPIOF(LED0), GPIOC(绿灯), GPIOE(红蓝灯) 的时钟
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOE, ENABLE);

  // 2. 配置 LED0 (PF9)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOF, &GPIO_InitStructure);

  // 3. 配置 绿灯 (PC0)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  // 4. 配置 红灯和蓝灯 (PE5, PE6)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  // 5. 初始状态全部熄灭 (LED设为1熄灭，RGB设为0亮)
  LED0=1; 
  LED_RED=1; LED_GREEN=1; LED_BLUE=1;
}






