/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-10-27 13:05:03
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-11-06 09:44:29
 * @FilePath: \undefinedc:\Users\Administrator\Desktop\Finite State Machine.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

// 状态类型枚举
typedef enum{
    SETTING, // 设置状态
    TINMING  // 定时状态
}STATE_TYPE;

// 事件类型枚举
typedef enum{
    UP_EVT,   // 上事件
    DOWN_EVT, // 下事件
    ARM_EVT,  // 武器事件
    TICK_EVT  // 时钟事件
}EVENT_TYPE;

// 炸弹结构体
struct bomb
{
    uint8_t state;       // 状态
    uint8_t timeout;     // 超时时间
    uint8_t code;        // 密码
    uint8_t defuse_code; // 拆弹密码
}bomb1;

// 炸弹初始化函数
void bomb1_init(void)
{
    bomb1.state = SETTING; // 初始状态为设置状态
    bomb1.defuse_code = 6; // 拆弹密码为6
}

// 有限状态机调度函数
void bomb1_fsm_dispatch(EVENT_TYPE evt,void*param)
{
    switch (bomb1.state)
    {
    case SETTING: // 设置状态
        switch (evt)
        {
        case UP_EVT: // 上事件
            if(bomb1.timeout <60)  ++bomb1.timeout; // 超时时间加1
            bsp_display(bomb1.timeout); // 显示超时时间
            break;
        case DOWN_EVT: // 下事件
            if(bomb1.timeout <0)  --bomb1.timeout; // 超时时间减1
            bsp_display(bomb1.timeout); // 显示超时时间
            break;
        case ARM_EVT: // 武器事件
            bomb1.code = 0; // 密码清零
            bomb1.state = TINMING; // 状态转换为定时状态
            break;
        default:
            break;
        }
        break;
    case TINMING: // 定时状态
        switch (evt)
        {
        case UP_EVT: // 上事件
            bomb1.code = (bomb1.code <<1) |0x01; // 密码左移一位并加1
            break;
        case DOWN_EVT: // 下事件
            bomb1.code = (bomb1.code <<1) ; // 密码左移一位
            break;
        case TICK_EVT: // 时钟事件
            if(bomb1.timeout ) // 如果超时时间不为0
            {
                --bomb1.timeout; // 超时时间减1
                bsp_display(bomb1.timeout); // 显示超时时间
            }
            if (bomb1.timeout == 0) // 如果超时时间为0
            {
                bsp_display("bomb!") // 显示"bomb!"
            }
            break;
        case ARM_EVT: // 武器事件
            if (bomb1.code == bomb1.defuse_code) // 如果密码等于拆弹密码
            {
                bomb1.state = SETTING; // 状态转换为设置状态
            }
            else
            {
                bsp_display("bomb!") // 显示"bomb!"
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}





   /*1.列出所有的状态*/
      enum
      {
          SETTING,
          TIMING,
          MAX_STATE
      };
      /*2.列出所有的事件*/
      enum
      {
          UP_EVT,
          DOWN_EVT,
          ARM_EVT,
          TICK_EVT,
          MAX_EVT
      };
      
      /*3.定义状态表*/
      typedef void (*fp_state)(EVT_TYPE evt , void* param);
      static  const fp_state  bomb2_table[MAX_STATE][MAX_EVENT] =
      {
          {setting_UP , setting_DOWN , setting_ARM , null},
          {setting_UP , setting_DOWN , setting_ARM , timing_TICK}
      };
      
      struct bomb_t
      {
          const fp_state const *state_table; /* the State-Table */
          uint8_t state; /* the current active state */
          
          uint8_t timeout;
          uint8_t code;
          uint8_t defuse_code;
      };
      struct bomb bomb2=
      {
          .state_table = bomb2_table;
      }
      void bomb2_init(void)
      {
          bomb2.defuse_code = 6; // 0110
          bomb2.state = SETTING;
      }
      
      void bomb2_dispatch(EVT_TYPE evt , void* param)
      {
          fp_state  s = NULL;
          if(evt > MAX_EVT)
          {
              LOG("EVT type error!");
              return;
          }
          s = bomb2.state_table[bomb2.state * MAX_EVT + evt];
          if(s != NULL)
          {
              s(evt , param);
          }
      }
      /*列出所有的状态对应的事件处理函数*/
      void setting_UP(EVT_TYPE evt, void* param)
      {
          if(bomb1.timeout< 60)  ++bomb1.timeout;
          bsp_display(bomb1.timeout);
      }   
      //对应上事件 处理机制二维状态表 
      //各个状态面向用户相对独立，增加事件和状态不需要去修改先前已存在的状态事件函数。

      //可将状态机进行封装，有较好的移植性 函数指针的安全转换 
      //利用下面的特性，用户可以扩展带有私有属性的状态机和事件而使用统一的基础状态机接口
      //缺点：函数粒度太小是最明显的一个缺点，一个状态和一个事件就会产生一个函数，当状态和事件较多时，处理函数将增加很快，在阅读代码时，逻辑分散。

      //没有实现进入退出动作，状态机的状态转换是瞬间完成的，没有实现状态的过渡，这样在状态转换时，可能会出现一些问题，比如在状态转换时，状态机可能处于一个不稳定的状态，这样可能会导致一些问题。


/* 按键去抖动状态机中的三个状态 */
#define KEY_STATE_RELEASE    // 按键未按下
#define KEY_STATE_WAITING    // 等待（消抖）
#define KEY_STATE_PRESSED    // 按键按下（等待释放）

/* 等待状态持续时间
 * 需要根据单片机速度和按键消抖程序被调用的速度来进行调整
 */
#define DURIATION_TIME 40

/* 按键检测函数的返回值，按下为 1，未按下为 0 */
#define PRESSED 1
#define NOT_PRESSED 0

/* 按键扫描程序所处的状态
 * 初始状态为：按键按下（KEY_STATE_RELEASE）
 */
uint8_t keyState = KEY_STATE_RELEASE;

/* 按键按下的时间 */
uint16_t keyDownTime = 0;

/* 按键检测函数，通过有限状态机实现
 * 函数在从等待状态转换到按键按下状态时返回 PRESSED，代表按键已被触发
 * 其他情况返回 NOT_PRESSED
 */
uint8_t keyDetect(void)
{
    static uint8_t duriation;  // 用于在等待状态中计数
    static uint8_t clickCount; // 用于记录按键点击次数
    static uint16_t lastClickTime; // 用于记录上一次按键点击的时间
    switch(keyState)
    {
    case KEY_STATE_RELEASE:
        if(readKey() == 1)     // 如果按键按下
        {
            keyState = KEY_STATE_WAITING;  // 转换至下一个状态
            keyDownTime = 0; // 重置按键按下的时间
        }
        return NOT_PRESSED;    // 返回：按键未按下
        break;
    case KEY_STATE_WAITING:
        if(readKey() == 1)     // 如果按键按下
        {
            duriation++;
            keyDownTime++;
            if(duriation >= DURIATION_TIME)    // 如果经过多次检测，按键仍然按下
            {   // 说明没有抖动了，可以确定按键已按下
                duriation = 0;
                keyState = KEY_STATE_PRESSED;  // 转换至下一个状态
                return PRESSED;
            }
        }
        else  // 如果此时按键松开
        {   // 可能存在抖动或干扰
            duriation = 0;  // 清零的目的是便于下次重新计数
            keyState = KEY_STATE_RELEASE;  // 重新返回按键松开的状态
            if (keyDownTime >= 1000) { // 如果按键按下的时间超过1秒，触发长按事件
                return 2;
            }
            return NOT_PRESSED;
        }
        break;
    case KEY_STATE_PRESSED:
        if(readKey() == 0)       // 如果按键松开
        {
            keyState = KEY_STATE_RELEASE;  // 回到按键松开的状态
            if (keyDownTime < 1000) { // 如果按键按下的时间不超过1秒
                if (clickCount == 0) { // 如果是第一次按下
                    clickCount++;
                    lastClickTime = keyDownTime;
                } else if (clickCount == 1) { // 如果是第二次按下
                    if (keyDownTime - lastClickTime < 500) { // 如果两次按下的时间间隔小于500ms，触发双击事件
                        clickCount = 0;
                        return 3;
                    } else { // 如果两次按下的时间间隔大于等于500ms，重新开始计数
                        clickCount = 1;
                        lastClickTime = keyDownTime;
                    }
                }
            }
        }
        return NOT_PRESSED;
        break;
    default:
        keyState = KEY_STATE_RELEASE;
        return NOT_PRESSED;
    }
}

//基于stm32f103的按键状态机 
#include "stm32f1xx_hal.h"

enum ButtonState {
    NO_PRESS,
    DEBOUNCE,
    SHORT_PRESS,
    LONG_PRESS
};

enum Event {
    BUTTON_DOWN,
    BUTTON_UP
};

volatile enum ButtonState current_state = NO_PRESS;
volatile int press_count = 0;
volatile bool button_was_pressed = false;

TIM_HandleTypeDef htim2;

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_TIM2_Init(void);

void handle_single_click(void);
void handle_double_click(void);
void handle_long_press(void);
bool is_button_pressed(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    
    HAL_TIM_Base_Start_IT(&htim2);  // Start Timer interrupt
    
    while (1) {
        enum Event event = is_button_pressed() ? BUTTON_DOWN : BUTTON_UP;
        
        if (event == BUTTON_DOWN && current_state == NO_PRESS) {
            current_state = DEBOUNCE;
        }
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        switch (current_state) {
            case DEBOUNCE:
                if (is_button_pressed()) {
                    current_state = SHORT_PRESS;
                    press_count = 1;
                    button_was_pressed = true;
                } else {
                    current_state = NO_PRESS;
                }
                break;
                
            case SHORT_PRESS:
                if (is_button_pressed()) {
                    current_state = LONG_PRESS;
                } else {
                    if (button_was_pressed && press_count == 1) {
                        handle_single_click();
                    } else if (button_was_pressed && press_count == 2) {
                        handle_double_click();
                    }
                    current_state = NO_PRESS;
                    button_was_pressed = false;
                    press_count = 0;
                }
                break;
                
            case LONG_PRESS:
                if (!is_button_pressed()) {
                    handle_long_press();
                    current_state = NO_PRESS;
                }
                break;
        }
    }
}

bool is_button_pressed(void) {
    return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET;
}

void handle_single_click(void) {
    // Handle single click event
}

void handle_double_click(void) {
    // Handle double click event
}

void handle_long_press(void) {
    // Handle long press event
}

void SystemClock_Config(void) {
    // ... (generated by STM32CubeMX)
}

void MX_GPIO_Init(void) {
    // ... (generated by STM32CubeMX)
}

void MX_TIM2_Init(void) {
    // ... (generated by STM32CubeMX)//基于stm32f103的按键状态机
}
//按照LSB格式把一个Word转化为两个字节
#define LSB(x) ((uint8_t)((x)&0xFF))

#define FLOPW( ray, val ) \
(ray)[0] = ((val) / 256); \
(ray)[1] = ((val) & 0xFF)  //把两个字节转化为一个Word


#define FLOPR( ray ) \
((ray)[0] * 256 + (ray)[1]) //把两个字节转化为一个Word



#define FLG_UART 0x01
#define FLG_TMR 0x02
#define FLG_EXI 0x04
#define FLG_KEY 0x08
volatile INT8U g_u8EvntFlgGrp = 0; /*事件标志组*/
INT8U read_envt_flg_grp(void);
/***************************************
*FuncName : main
*Description : 主函数
*Arguments : void
*Return : void
*****************************************/
void main(void)
{
 INT8U u8FlgTmp = 0;
 sys_init();
 while(1)
 {
  u8FlgTmp = read_envt_flg_grp(); /*读取事件标志组*/
  if(u8FlgTmp ) /*是否有事件发生？ */
  {
   if(u8FlgTmp & FLG_UART)
   {
    action_uart(); /*处理串口事件*/
   }
   if(u8FlgTmp & FLG_TMR)
   {
    action_tmr(); /*处理定时中断事件*/
   }
   if(u8FlgTmp & FLG_EXI)
   {
    action_exi(); /*处理外部中断事件*/
   }
   if(u8FlgTmp & FLG_KEY)
   {
    action_key(); /*处理击键事件*/
   }
  }
  else
  {
   ;/*idle code*/
  }
 }
}
/*********************************************
*FuncName : read_envt_flg_grp
*Description : 读取事件标志组 g_u8EvntFlgGrp ，
* 读取完毕后将其清零。
*Arguments : void
*Return : void
*********************************************/
INT8U read_envt_flg_grp(void)
{
 INT8U u8FlgTmp = 0;
 gbl_int_disable();
 u8FlgTmp = g_u8EvntFlgGrp; /*读取标志组*/
 g_u8EvntFlgGrp = 0; /*清零标志组*/
 gbl_int_enable();
 return u8FlgTmp;
}
/*********************************************
*FuncName : uart0_isr
*Description : uart0 中断服务函数
*Arguments : void
*Return : void
*********************************************/
void uart0_isr(void)
{
 ......
 push_uart_rcv_buf(new_rcvd_byte); /*新接收的字节存入缓冲区*/
 gbl_int_disable();
 g_u8EvntFlgGrp |= FLG_UART; /*设置 UART 事件标志*/
 gbl_int_enable();
 ......
}
/*********************************************
*FuncName : tmr0_isr
*Description : timer0 中断服务函数
*Arguments : void
*Return : void
*********************************************/
void tmr0_isr(void)
{
 INT8U u8KeyCode = 0;
 ......
 gbl_int_disable();
 g_u8EvntFlgGrp |= FLG_TMR; /*设置 TMR 事件标志*/
 gbl_int_enable();
 ......
 u8KeyCode = read_key(); /*读键盘*/
 if(u8KeyCode) /*有击键操作？ */
 {
  push_key_buf(u8KeyCode); /*新键值存入缓冲区*/
  gbl_int_disable();
  g_u8EvntFlgGrp |= FLG_KEY; /*设置 TMR 事件标志*/
  gbl_int_enable();
 }
 ......
}
/*********************************************
*FuncName : exit0_isr
*Description : exit0 中断服务函数
*Arguments : void
*Return : void
*********************************************/
void exit0_isr(void)
{
 ......
 gbl_int_disable();
 g_u8EvntFlgGrp |= FLG_EXI; /*设置 EXI 事件标志*/
 gbl_int_enable();
 ......
}