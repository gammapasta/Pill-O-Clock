#include "device_driver.h"

// PB6 핀을 TIM4_CH1 으로 사용하여 패시브 부저 작동
void Buzzer_Init(void)
{
    // 1. GPIOB 및 TIM4 클럭 활성화
    Macro_Set_Bit(RCC->AHB1ENR, 1);    // GPIOB ON
    Macro_Set_Bit(RCC->APB1ENR, 2);    // TIM4 ON

    // 2. PB6 핀 설정 (Alternate Function 모드)
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x2, 12);  // PB6 => AF 모드
    Macro_Write_Block(GPIOB->AFR[0], 0xF, 0x2, 24); // PB6 => AF2 (TIM4) 연결

    // 3. TIM4 기본 설정 (1MHz 카운터)
    TIM4->PSC = 84 - 1;   // 84MHz 기준 1us 마다 카운트
    TIM4->ARR = 1000 - 1; // 기본 주파수
    TIM4->CCR1 = 0;       // 처음엔 소리 끄기 (Duty 0%)

    // 4. TIM4_CH1 PWM 모드 1 설정
    TIM4->CCMR1 &= ~(0xFF);
    TIM4->CCMR1 |= (0x6 << 4); // PWM 모드 1
    TIM4->CCER |= (1 << 0);    // CH1 출력 활성화

    // 5. 타이머 시작
    TIM4->CR1 |= (1 << 0);
}

void Buzzer_On(void)
{
    // 2000Hz 주파수 생성 (1,000,000 / 2000 = 500)
    TIM4->ARR = 500 - 1;
    TIM4->CCR1 = 250; // 50% 듀티비
}

void Buzzer_Off(void)
{
    // 듀티비를 0으로 만들어서 소리 차단
    TIM4->CCR1 = 0;
}