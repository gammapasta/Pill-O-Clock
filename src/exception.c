#include "device_driver.h"
#include <stdio.h>

void _Invalid_ISR(void)
{
	unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
	Uart2_Printf("\nInvalid_Exception: %d!\n", r);
	Uart2_Printf("Invalid_ISR: %d!\n", r - 16);
	for(;;);
}

extern volatile int system_mode;

void EXTI15_10_IRQHandler(void)
{
	system_mode = (system_mode + 1) % 4;

	EXTI->PR = 0x1<<13;
	NVIC_ClearPendingIRQ((IRQn_Type)40);
}

extern volatile int pill_alarm_flag;

void RTC_Alarm_IRQHandler(void)
{
	// 알람 인터럽트 플래그가 세팅되었는지 확인
    if (EXTI->PR & (1 << 17)) // RTC Alarm은 보통 EXTI 라인 17에 연결
    {
        pill_alarm_flag = 1;  // 메인 루프에 알림
        EXTI->PR |= (1 << 17); // EXTI 플래그 초기화
        RTC->ISR &= ~(1 << 8); // RTC 알람 플래그 초기화
    }
}



// [디버그] 테라텀으로 제어
extern volatile unsigned char Uart_Data;

extern volatile char uart2_buffer[64];
extern volatile int uart2_rx_index;
extern volatile int uart2_rx_exist;
void USART2_IRQHandler(void)
{
	// 수신된 데이터는 Uart_Data에 저장
	Uart_Data = USART2->DR & (0xff<<0);

	// 이미 존재하면 클리어
	if (uart2_rx_exist == 1)
    {
        NVIC_ClearPendingIRQ(38);
        return; 
    }
	
	if (Uart_Data == '\n' || Uart_Data == ' ')  // 엔터 or 공백으로 문자열 완성
	{
		uart2_buffer[uart2_rx_index] = '\0';  // 문자열 종료
		uart2_rx_index = 0;  // 인덱스 초기화
		uart2_rx_exist = 1;   // 문자열 완성해서 있다는 플래그
	}
	// 문자열 만들기
	else
	{
		if (uart2_rx_index < 64 - 1)
		{
			uart2_buffer[uart2_rx_index] = Uart_Data;
			uart2_rx_index++;
		}
	}
	// NVIC Pending Clear
	NVIC_ClearPendingIRQ(38);

}