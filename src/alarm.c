#include <stdio.h>
#include "device_driver.h"
#include <string.h>

void RTC_Init_And_Alarm_Set(int hour, int min, int sec)
{
    // 전원(PWR) 제어 클럭 활성화
    Macro_Set_Bit(RCC->APB1ENR, 28);
    
    // RTC 레지스터 쓰기 접근 허용, DBP 비트 세팅
    PWR->CR |= (1 << 8);

    RCC->BDCR |= (1 << 16);  // 리셋 스위치 ON
    RCC->BDCR &= ~(1 << 16); // 리셋 스위치 OFF
    
    // 외부 정밀 클럭(LSE) 켜기 (PC14, PC15 핀 자동 점유)
    RCC->BDCR |= (1 << 0);   // LSEON
    
    // LSE가 안정적으로 뛸 때까지 대기 (LSERDY)
    while(!(RCC->BDCR & (1 << 1))); 
    
    // RTC에서 LSE를 선택 (RTCSEL = 01)
    RCC->BDCR |= (0x1 << 8); 
    
    // RTC 시계 켜기 (RTCEN)
    Macro_Set_Bit(RCC->BDCR, 15);
    
    // 레지스터 쓰기 잠금 해제
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;
    
    // 초기화 모드 진입 (INIT = 1)
    Macro_Set_Bit(RTC->ISR, 7);      
    while(!Macro_Check_Bit_Set(RTC->ISR, 6)); // INITF 대기
    
    // 32,768Hz를 1초로 (128 * 256)
    RTC->PRER = (127 << 16) | 255;   

    // 현재 시간 설정 (처음 켰을 때 00:00:00)
    RTC->TR = 0; 

    // 초기화 모드 종료
    Macro_Clear_Bit(RTC->ISR, 7);    

    // 알람 A 설정
    Macro_Clear_Bit(RTC->CR, 8);     // ALRAE Disable
    while(!Macro_Check_Bit_Set(RTC->ISR, 0)); // ALRAWF 대기

    RTC->ALRMAR = (1 << 31) | 
                  (TO_BCD(hour) << 16) | 
                  (TO_BCD(min) << 8) | 
                  (TO_BCD(sec));

    Macro_Set_Bit(RTC->CR, 8);       // ALRAE Enable
    Macro_Set_Bit(RTC->CR, 12);      // ALRAIE Interrupt Enable

    // EXTI 및 NVIC 설정
    Macro_Set_Bit(EXTI->IMR, 17);    
    Macro_Set_Bit(EXTI->RTSR, 17);   
    NVIC->ISER[1] |= (1 << (41 - 32)); 
}

// 외부에서 받은 문자열을 분석해 알람 설정 함수
void Set_Alarm_From_String(char *str) {
    int h, m, s;
    
    // 형식 검사 (예: "08:30:05")
    if (sscanf(str, "%d:%d:%d", &h, &m, &s) == 3) {
        printf("[SETTING] New Alarm Time -> %02d:%02d:%02d\r\n", h, m, s);

        // 1. 알람 설정을 위해 잠시 비활성화
        RTC->WPR = 0xCA;
        RTC->WPR = 0x53;
        RTC->CR &= ~(1 << 8); // ALRAE OFF
        while(!(RTC->ISR & (1 << 0))); // ALRAWF 대기

        // 2. 입력받은 시간을 BCD로 변환하여 알람 레지스터에 저장
        // 매일 해당 시간에 울리도록 Mask(1<<31, 23, 15)는 끄고 시간값만 넣음
        RTC->ALRMAR = (TO_BCD(h) << 16) | (TO_BCD(m) << 8) | TO_BCD(s);
        
        // 날짜 마스크(1<<31)를 켜면 '매일' 이 시간에 울림
        RTC->ALRMAR |= (1 << 31); 

        // 3. 알람 다시 활성화
        RTC->CR |= (1 << 8); // ALRAE ON
        RTC->WPR = 0xFF;     // 다시 잠금
        
        printf("[SUCCESS] Alarm has been updated!\r\n");
    } else {
        printf("[ERROR] Invalid Format! Use HH:MM:SS\r\n");
    }
}

// 현재 시간을 덮어씌우는 함수
void Set_Current_Time(int h, int m, int s) {
    // 1. RTC 레지스터 쓰기 잠금 해제
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    // 2. 시간 설정을 위해 초기화(INIT) 모드 진입
    Macro_Set_Bit(RTC->ISR, 7);      
    while(!Macro_Check_Bit_Set(RTC->ISR, 6)); // INITF 대기

    // 3. 현재 시간 덮어쓰기 (BCD 변환)
    RTC->TR = (TO_BCD(h) << 16) | (TO_BCD(m) << 8) | TO_BCD(s);

    // 4. 초기화 모드 종료 및 레지스터 다시 잠금
    Macro_Clear_Bit(RTC->ISR, 7);    
    RTC->WPR = 0xFF;
}