#include "stm32f4xx.h"
#include "option.h"
#include "macro.h"
#include "malloc.h"

// Uart.c

extern void Uart2_Init(int baud);
extern void Uart2_Send_Byte(char data);

extern void Uart1_Init(int baud);
extern void Uart1_Send_Byte(char data);
extern void Uart1_Send_String(char *pt);
extern void Uart1_Printf(char *fmt,...);
extern char Uart2_Get_Char(void);
extern char Uart1_Get_Pressed(void);
extern void Uart2_Send_String(char *pt);
extern void Uart2_Printf(char *fmt,...);
extern void Process_UART_Input(void);
extern char Uart2_Get_Pressed(void);

extern void Uart2_RX_Interrupt_Enable(void);
// SysTick.c

extern void SysTick_Run(unsigned int msec);
extern int SysTick_Check_Timeout(void);
extern unsigned int SysTick_Get_Time(void);
extern unsigned int SysTick_Get_Load_Time(void);
extern void SysTick_Stop(void);
extern void Delay_ms(unsigned int msec);

// Led.c

extern void LED_Init(void);
extern void LED_On(void);
extern void LED_Off(void);

// Clock.c

extern void Clock_Init(void);

// Key.c

extern void Key_Poll_Init(void);
extern int Key_Get_Pressed(void);
extern void Key_Wait_Key_Released(void);
extern void Key_Wait_Key_Pressed(void);
extern void Key_ISR_Enable(int);

// Timer.c

extern void TIM2_Delay(int time);
extern void TIM2_Stopwatch_Start(void);
extern unsigned int TIM2_Stopwatch_Stop(void);
extern void TIM4_Repeat(int time);
extern int TIM4_Check_Timeout(void);
extern void TIM4_Stop(void);
extern void TIM4_Change_Value(int time);
extern void TIM3_Out_Init(void);
extern void TIM3_Out_Freq_Generation(unsigned short freq);
extern void TIM3_Out_Stop(void);
extern void TIM2_Init(void);

// motor.c
extern void Motor_Init(void);
extern void Stop(void);
extern void Move_CW(void);
extern void Move_CCW(void);
extern void Rotate_Next_Slot(void);
extern void Stepper_Step(int);
extern void Servo_Open_Close(void);
extern void Motor_Set_Percent(unsigned int percent);
extern unsigned int Motor_Get_Percent(void);
extern int Motor_Get_Dir(void);
extern void Stepper2_Step(int);
extern void Supply_Pill(void);
extern void Stepper2_One_Day(void);
// adc.c
extern void ADC1_IN6_Init(void);
extern void ADC1_Start(void);
extern void ADC1_Stop(void);
extern int ADC1_Get_Status(void);
extern int ADC1_Get_Data(void);

// alarm.c
extern void RTC_Init_And_Alarm_Set(int, int, int);
extern void Set_Alarm_From_String(char*);
extern void Set_Current_Time(int, int, int);

// lcd.c
extern void I2C1_Init(void);
extern void I2C1_Write_Byte(unsigned char addr, char data);
extern void LCD_Send(unsigned char addr, char data, int mode);
extern void LCD_Cmd(unsigned char addr, char cmd);
extern void LCD_Data(unsigned char addr, char data);
extern void LCD_Init(unsigned char addr);
extern void LCD_Set_Cursor(unsigned char addr, int y, int x);
extern void LCD_String(unsigned char addr, char *str);

// buzzer.c
extern void Buzzer_Init(void);
extern void Buzzer_On(void);
extern void Buzzer_Off(void);

// utrasonic.c
extern void Ultrasonic_Init(void);
extern int Ultrasonic_Get_Distance(void);

// status_led.c
extern void Status_LED_Init(void);
extern void Status_LED_All_Off(void);
extern void Status_LED_Red(void);
extern void Status_LED_Yellow(void);
extern void Status_LED_Green(void);
