#include "sys.h"
#include "usart.h"

#if SYSTEM_SUPPORT_OS
#include "includes.h" 
#endif
  
#if 1
#pragma import(__use_no_semihosting)            

// 房间状态结构体
typedef struct {
    uint8_t temp;
    uint8_t humi;
    uint8_t ac_state;
    uint8_t fan_state; 
} RoomStatus;

extern RoomStatus rooms[8]; 
extern void Update_Temp_Bound(uint8_t cmd, uint8_t value);

//支持 printf 重定向
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
 
void _sys_exit(int x) 
{ 
	x = x; 
} 

// 重定向 fputc 函数，使得使用 printf 时数据从 USART1 输出
int fputc(int ch, FILE *f)
{ 	
	while((USART1->SR&0X40)==0); 
	USART1->DR = (u8) ch;     
	return ch;
}
#endif
 
#if EN_USART1_RX 


u8 USART_RX_BUF[USART_REC_LEN];

u16 USART_RX_STA=0; 


void uart_init(u32 bound){
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);

	// 设置引脚复用映射：PA9 映射为 USART1_TX，PA10 映射为 USART1_RX
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

    // USART1 协议参数配置
	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1; 
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
	
    USART_Cmd(USART1, ENABLE);

	
#if EN_USART1_RX	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); 

    // 串口1中断优先级配置（NVIC）
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);	

#endif
	
}

// 接收逻辑状态机枚举定义
typedef enum STATE{
	DETECT_HEAD_STATE=0, // 寻找帧头状态
	NOMAL_RECDATA_STATE, // 接收核心数据状态
	DETECT_TAIL_STATE    // 寻找帧尾状态
}RecState;

u8 USART_RX_BUF[USART_REC_LEN]; // 再次定义缓冲区（全局）

// 串口1中断服务函数：负责所有下位机的接收逻辑
void USART1_IRQHandler(void) {
    u8 data;
    static u8 rx_buf[10]; // 静态局部数组：临时存放剥离帧头帧尾后的核心数据
    static u8 index = 0;  // 静态局部变量：用于核心数据存储计数
    static u8 state = 0;  // 静态局部变量：维护状态机当前所处位置

#if SYSTEM_SUPPORT_OS
    OSIntEnter(); // 进入中断前通知操作系统（如UCOS）    
#endif

    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        data = USART_ReceiveData(USART1); 
        //状态机跳转逻辑
        switch(state) {
            case 0: 
                if(data == 0x3C) 
					state = 1;
                break;
            case 1:
                if(data == 0x3C) 
				{
                    state = 2;   
                    index = 0;  
                } else state = 0; //出错
                break;
            case 2: 
                rx_buf[index++] = data;
                if(index >= 5) state = 3; 
                break;
            case 3: 
                if(data == 0x3E) state = 4;
                else state = 0; 
                break;
            case 4: 
                if(data == 0x3E) { 
                    // --- 开始执行协议指令解析 ---
                    u8 rid = rx_buf[0]; 
                    u8 cmd = rx_buf[1]; 
                    u8 dH  = rx_buf[2]; 
                    u8 dL  = rx_buf[3]; 
                    
                    if(rid >= 1 && rid <= 7) {
                        if(cmd == 0x12) { //修改空调风机状态
                            rooms[rid].ac_state = dH; 
                            rooms[rid].fan_state = dL;
                        } else if(cmd == 0x13) { //修改温度
                            rooms[rid].temp = dH; 
                        }
						else if(cmd == 0x20 || cmd == 0x21) { //更新温度上下限
                            Update_Temp_Bound(cmd, dH);
                        }
                    }
                }
                state = 0; 
                break;
            default: 
                state = 0;
                break;
        }
    }

#if SYSTEM_SUPPORT_OS
    OSIntExit(); // 中断处理结束，通知操作系统进行可能的任务调度                                               
#endif
}
#endif