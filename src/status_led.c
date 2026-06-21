#include "device_driver.h"

// 외부 상태 표시용 LED 6개 초기화 (PB12~15, PA7~8)
void Status_LED_Init(void)
{
    // GPIOA, GPIOB 클럭 켜기 (0번 비트, 1번 비트)
    Macro_Set_Bit(RCC->AHB1ENR, 0);
    Macro_Set_Bit(RCC->AHB1ENR, 1);

    // PB12 Output 모드 설정  제외PB13, PB14, PB15
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, 12 * 2);
    Macro_Clear_Bit(GPIOB->OTYPER, 12);                  // Push-Pull
    Macro_Write_Block(GPIOB->OSPEEDR, 0x3, 0x2, 12 * 2); // Fast speed
    Macro_Set_Bit(GPIOB->ODR, 12);                     // 초기 상태: 끄기

    // PA7 Output 모드 설정
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 7 * 2);
    Macro_Clear_Bit(GPIOA->OTYPER, 7);                  // Push-Pull
    Macro_Write_Block(GPIOA->OSPEEDR, 0x3, 0x2, 7 * 2); // Fast speed
    Macro_Set_Bit(GPIOA->ODR, 7);                     // 초기 상태: 끄기
}

// 모든 LED 끄기
void Status_LED_All_Off(void)
{
    // PB12  끄기
    Macro_Set_Bit(GPIOB->ODR, 12);

    // PA7, 끄기
    Macro_Set_Bit(GPIOA->ODR, 7);
}

// 빨간색 2개 켜기 (PB12, PB13)
void Status_LED_Red(void)
{
    Status_LED_All_Off();
    Macro_Clear_Bit(GPIOB->ODR, 12);
    // Macro_Set_Bit(GPIOB->ODR, 13);
}


// 초록색 4개 켜기 (PA7, PA8)
void Status_LED_Green(void)
{
    Status_LED_All_Off();
    Macro_Clear_Bit(GPIOA->ODR, 7);
}