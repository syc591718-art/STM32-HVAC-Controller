#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "key.h"
#include "dht11.h"

// 房间状态结构体定义
typedef struct {
    uint8_t temp;
    uint8_t humi;
    uint8_t ac_state;  // 空调状态：0-恒温, 1-制热, 2-制冷
    uint8_t fan_state; // 风机状态：0-关闭(OFF), 1-开启(ON)
} RoomStatus;

RoomStatus rooms[8];    //下标0不用，用1-7表示房间
uint8_t select_room = 1; 

// --- 函数声明 ---
void System_Init(void);
void Display_Home(void); 
void Update_LCD_Data(void);
void Send_Frame(uint8_t rid, uint8_t cmd, uint8_t dH, uint8_t dL);
void Refresh_Hardware_LED(void);
uint8_t Limit_Temp(uint8_t temp);
void Update_Temp_Bound(uint8_t cmd, uint8_t value);

uint8_t temp_min = 20; 
uint8_t temp_max = 30; 


int main(void) {
    uint8_t key_val = 0;
    uint32_t timer_cnt = 0;
    
    System_Init();
    Display_Home();
    
    while(1) {
        // 1. 按键处理逻辑 
        key_val = KEY_Scan(0);
        if(key_val != 0) {
            if(key_val == KEY0_PRES) { select_room++; if(select_room > 7) select_room = 1; } 
            else if(key_val == KEY1_PRES) { rooms[select_room].temp++; } 
            else if(key_val == KEY2_PRES) { rooms[select_room].temp--; }
            Refresh_Hardware_LED(); // 按键操作后立即更新硬件LED灯指示状态
        }

        // 2. 房间1：读取真实传感器数据 (每500ms采样一次)
        if(timer_cnt % 50 == 0) { 
            uint8_t t, h;
            if(DHT11_Read_Data(&t, &h) == 0) { // 如果DHT11传感器读取成功
                rooms[1].temp = t;
                rooms[1].humi = h;
            }
        }

        // 3. 自动调节与模拟反馈逻辑 (每1000ms执行一次)
        if(timer_cnt % 100 == 0) { 
            for(int i = 1; i <= 7; i++) { // 遍历所有7个房间
               if(rooms[i].temp > temp_max)      rooms[i].ac_state = 2; 
                else if(rooms[i].temp < temp_min) rooms[i].ac_state = 1; 
                else                         rooms[i].ac_state = 0; 

                if(rooms[i].humi > 65)      rooms[i].fan_state = 1; 
                else if(rooms[i].humi <= 55) rooms[i].fan_state = 0; 

                // B. 演示模拟逻辑：针对房间2-7进行数值自动变化演示
                if(i > 1) { 
                    if(rooms[i].ac_state == 1) rooms[i].temp++;  
                    if(rooms[i].ac_state == 2) rooms[i].temp--; 
                    if(rooms[i].fan_state == 1 && rooms[i].humi > 10) rooms[i].humi -= 2; 
                }
            }
        }

        // 4. LCD刷新与串口同步上传 (每300ms执行一次)
        if(timer_cnt % 30 == 0) { // 30 * 10ms = 300ms
            Update_LCD_Data(); 
            Refresh_Hardware_LED(); 
            for(int i = 1; i <= 7; i++) { // 循环发送所有房间的数据包给上位机Python
                uint8_t s_cmd = (rooms[i].ac_state << 4) | (rooms[i].fan_state & 0x0F); // 将空调和风机状态合并为一个字节
                Send_Frame(i, s_cmd, rooms[i].temp, rooms[i].humi); // 发送9字节标准协议帧
                delay_ms(1);
            }
        }
        delay_ms(10); 
        timer_cnt++; 
    }
}

// 系统初始化函数
void System_Init(void) 
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 
    delay_init(168);   // 初始化延时函数，时钟频率168MHz 
    uart_init(115200); // 初始化串口1，波特率为115200
    LED_Init();
    LCD_Init();
    KEY_Init();
    DHT11_Init();
    for(int i=1; i<=7; i++) 
	{
        rooms[i].temp = 25;
        rooms[i].humi = 55;
        rooms[i].ac_state = 0;
        rooms[i].fan_state = 0;
    }
    LCD_Clear(LIGHTBLUE);
}

// 串口数据包发送函数：遵循 << [ID] [CMD] [D1] [D2] [Sum] >>
void Send_Frame(uint8_t rid, uint8_t cmd, uint8_t dH, uint8_t dL) 
{
    uint8_t sum = (rid + cmd + dH + dL) & 0xFF; // 计算校验和（前四个字节累加后取低8位）
    USART_SendData(USART1, 0x3C); while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET); 
    USART_SendData(USART1, 0x3C); while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    USART_SendData(USART1, rid);  while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET); 
    USART_SendData(USART1, cmd);  while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    USART_SendData(USART1, dH);   while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET); 
    USART_SendData(USART1, dL);   while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    USART_SendData(USART1, sum);  while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    USART_SendData(USART1, 0x3E); while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    USART_SendData(USART1, 0x3E); while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}

// 硬件LED状态反馈函数
void Refresh_Hardware_LED(void)
{
    if (rooms[select_room].ac_state == 1) { LED_RED = 1; LED_BLUE = 0; LED_GREEN = 0; } 
    else if (rooms[select_room].ac_state == 2) { LED_RED = 0; LED_BLUE = 1; LED_GREEN = 0; }
    else { LED_RED = 0; LED_BLUE = 0; LED_GREEN = 1; }
    LED0 = (rooms[select_room].fan_state) ? 0 : 1;
}

// 局部刷新LCD内容函数：仅刷新变化的部分
void Update_LCD_Data(void) 
{
    static RoomStatus old[8]; static uint8_t old_sel = 0; // 定义静态变量保存上一次的状态
    for(int i=1; i<=7; i++) { // 遍历显示7个房间
        int x = (i <= 4) ? 15 : 245;  // 1-4房在左侧，5-7房在右侧
        int y = (i <= 4) ? (85 + (i-1)*170) : (85 + (i-5)*170); // 根据房间号计算卡片纵坐标
        
        // 只有当数值、状态或选中项发生改变时才执行重绘
        if(rooms[i].temp != old[i].temp || rooms[i].humi != old[i].humi || 
           rooms[i].ac_state != old[i].ac_state || rooms[i].fan_state != old[i].fan_state || 
           select_room != old_sel) {
            
            // 绘制/刷新 [ACTIVE] 选中标志
            if(i == select_room) {
                POINT_COLOR = RED; // 选中房间标志设为红色
                LCD_ShowString(x+115, y+12, 100, 16, 16, (u8*)"[ACTIVE]"); 
            } else {
                LCD_Fill(x+115, y+12, x+210, y+28, WHITE); // 未选中房间则用背景色抹除旧标志
            }

            // 局部刷新温湿度数字
            LCD_Fill(x+70, y+55, x+95, y+71, WHITE);  // 抹除旧温度数字
            LCD_Fill(x+70, y+85, x+95, y+101, WHITE); // 抹除旧湿度数字
            POINT_COLOR = BLACK; // 设置文字颜色为黑色
            LCD_ShowNum(x+70, y+55, rooms[i].temp, 2, 16);  // 显示新温度
            LCD_ShowNum(x+70, y+85, rooms[i].humi, 2, 16);  // 显示新湿度

            // 局部刷新空调模式文字
            LCD_Fill(x+10, y+120, x+210, y+140, WHITE); // 抹除旧状态文字行
            if(rooms[i].ac_state == 1) { POINT_COLOR = RED; LCD_ShowString(x+10, y+120, 100, 16, 16, (u8*)"AC:HEAT"); } // 制热显示红色
            else if(rooms[i].ac_state == 2) { POINT_COLOR = BLUE; LCD_ShowString(x+10, y+120, 100, 16, 16, (u8*)"AC:COOL"); } // 制冷显示蓝色
            else { POINT_COLOR = GRAY; LCD_ShowString(x+10, y+120, 100, 16, 16, (u8*)"AC:KEEP"); } // 恒温显示灰色

            // 局部刷新风机开关状态文字
            if(rooms[i].fan_state == 1) { POINT_COLOR = GREEN; LCD_ShowString(x+120, y+120, 90, 16, 16, (u8*)"FAN:ON "); } // 开启显示绿色
            else { POINT_COLOR = GRAY; LCD_ShowString(x+120, y+120, 90, 16, 16, (u8*)"FAN:OFF"); } // 关闭显示灰色
            
            old[i] = rooms[i]; // 将当前状态存入old，供下一次比对
        }
    }
    old_sel = select_room; // 更新选中的旧值
}

// 初始静态UI绘制函数
void Display_Home(void) 
{
    LCD_Clear(LIGHTBLUE); // 全屏背景色
    LCD_Fill(0, 0, 480, 70, BLUE); // 顶部蓝色标题栏
    POINT_COLOR = BLUE; // 设置标题文字颜色
    LCD_ShowString(110, 25, 300, 24, 24, (u8*)"SMART HVAC SYSTEM"); // 打印大标题
    for(int i=1; i<=7; i++) { // 循环绘制7个房间的UI框架
        int x = (i <= 4) ? 15 : 245; 
        int y = (i <= 4) ? (85 + (i-1)*170) : (85 + (i-5)*170);
        LCD_Fill(x+5, y+5, x+225, y+160, GRAY); // 卡片阴影
        LCD_Fill(x, y, x+220, y+155, WHITE); // 卡片主体
        
        POINT_COLOR = BLACK; // 房间名称文字颜色
        if(i <= 3) LCD_ShowString(x+5, y+12, 80, 16, 16, (u8*)"BEDRM"); // 1-3房设为卧室
        else if(i <= 5) LCD_ShowString(x+5, y+12, 80, 16, 16, (u8*)"LIVING"); // 4-5房设为客厅
        else LCD_ShowString(x+5, y+12, 80, 16, 16, (u8*)"BATHRM"); // 6-7房设为卫生间
        LCD_ShowNum(x+75, y+12, i, 1, 16); // 打印房间编号
        
        POINT_COLOR = GRAY; LCD_DrawLine(x+10, y+40, x+210, y+40); // 标题下方的分隔线
        POINT_COLOR = BLACK; 
        LCD_ShowString(x+20, y+55, 100, 16, 16, (u8*)"Temp: "); // 温度标签
        LCD_Draw_Circle(x+105, y+57, 2); // 绘制摄氏度的小圆圈符号
        LCD_ShowString(x+110, y+55, 10, 16, 16, (u8*)"C"); // 摄氏度单位C
        
        LCD_ShowString(x+20, y+85, 100, 16, 16, (u8*)"Humi: "); // 湿度标签
        LCD_ShowString(x+105, y+85, 20, 16, 16, (u8*)"%"); // 湿度单位%
    }
}

// 温度限幅函数：确保温度不会超出设定的上下限
uint8_t Limit_Temp(uint8_t temp)
{
    if(temp < temp_min) return temp_min; 
    if(temp > temp_max) return temp_max; 
    return temp;
}

// 更新系统温度上下限函数
void Update_Temp_Bound(uint8_t cmd, uint8_t value)
{
    if(cmd == 0x20) // 命令码是0x20，代表修改温度下限
    {
        if(value < temp_max) 
		{ 
            temp_min = value;  // 更新全局下限值
        }
    }
    else if(cmd == 0x21) // 命令码是0x21，代表修改温度上限
    {
        if(value > temp_min) 
		{ 
            temp_max = value;  // 更新全局上限值
        }
    }
}
