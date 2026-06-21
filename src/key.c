#include "device_driver.h"

void Key_Poll_Init(void)
{
    // GPIOB 클럭 활성화
    Macro_Set_Bit(RCC->AHB1ENR, 1);

    // PB5를 입력 모드(00)로 설정
    // PB5의 MODER 비트 위치는 5 * 2 = 10
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x0, 10);

    // PB5 내부 Pull-up 설정(01)
    // 평소에는 1, 스위치를 누르면 GND로 떨어져 0이 됨
    Macro_Write_Block(GPIOB->PUPDR, 0x3, 0x1, 10);
}

int Key_Get_Pressed(void)
{
    // 외부 스위치는 active-low 방식
    // 눌렀을 때 PB5 = 0 이므로 bit clear를 검사
    return Macro_Check_Bit_Clear(GPIOB->IDR, 5);
}

void Key_Wait_Key_Pressed(void)
{
    while(!Macro_Check_Bit_Clear(GPIOB->IDR, 5));
}

void Key_Wait_Key_Released(void)
{
    while(!Macro_Check_Bit_Set(GPIOB->IDR, 5));
}

void Key_ISR_Enable(int en)
{
    (void)en;
}
