#include "device_driver.h"

void SysTick_Run(unsigned int msec)
{
	SysTick->CTRL = (0<<2)+(0<<1)+(0<<0);
	SysTick->LOAD = (unsigned int)((HCLK/(8.*1000.))*msec+0.5);
	SysTick->VAL = 0;
	Macro_Set_Bit(SysTick->CTRL, 0);
}

int SysTick_Check_Timeout(void)
{
	return ((SysTick->CTRL >> 16) & 0x1);
}

unsigned int SysTick_Get_Time(void)
{
	return SysTick->VAL;
}

unsigned int SysTick_Get_Load_Time(void)
{
	return SysTick->LOAD;
}

void SysTick_Stop(void)
{
	SysTick->CTRL = 0<<0;
}

void Delay_ms(unsigned int msec)
{
    // 한 번에 처리 가능한 최대 시간이 약 1.3초 500ms씩 나누어서 반복 실행.
    while(msec > 500)
    {
        SysTick_Run(500);
        while(!SysTick_Check_Timeout());
        msec -= 500;
    }
    
    // 남은 시간 처리
    if(msec > 0)
    {
        SysTick_Run(msec);
        while(!SysTick_Check_Timeout());
    }
    
    SysTick_Stop();
}