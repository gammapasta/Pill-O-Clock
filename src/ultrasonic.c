#include "device_driver.h"

void Ultrasonic_Init(void)
{
    // GPIOC 클럭 활성화
    Macro_Set_Bit(RCC->AHB1ENR, 2);

    // PC4 (Trig) 출력 모드
    Macro_Write_Block(GPIOC->MODER, 0x3, 0x1, 8);
    Macro_Clear_Bit(GPIOC->OTYPER, 4); // Push-pull
    Macro_Write_Block(GPIOC->OSPEEDR, 0x3, 0x3, 8); // High speed

    // PC5 (Echo) 입력 모드
    Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, 10);
    // Pull-down
    Macro_Write_Block(GPIOC->PUPDR, 0x3, 0x2, 10); 

    Macro_Clear_Bit(GPIOC->ODR, 4);
}

int Ultrasonic_Get_Distance(void)
{
    volatile int timeout;
    int time = 0;

    Macro_Set_Bit(GPIOC->ODR, 4);
    for(volatile int i = 0; i < 5000; i++); 
    Macro_Clear_Bit(GPIOC->ODR, 4);

    // echo 핀 high가 될 때까지 대기
    timeout = 2000000;
    while(Macro_Check_Bit_Clear(GPIOC->IDR, 5))
    {
        if(--timeout == 0) return -1; // 센서 무응답 에러
    }

    // echo 핀이 high로 유지되는 시간 측정
    timeout = 2000000;
    while(Macro_Check_Bit_Set(GPIOC->IDR, 5))
    {
        time++;
        for(volatile int i = 0; i < 15; i++); 
        if(--timeout == 0) return -2;
    }

    // 거리 계산 
    return time / 100; 
}