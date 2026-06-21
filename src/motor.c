#include <device_driver.h>
#include <stdio.h>

// ==================
// 모터 및 통신 핀 초기화
// ==================
void Motor_Init(void)
{
    // 1. DC 모터 제어 핀 설정 (PA0, PA1 -> TIM5 PWM)
    Macro_Set_Bit(RCC->APB1ENR, 3); // TIM5 클럭 ON
    
    Macro_Write_Block(GPIOA->MODER, 0xF, 0xA, 0);       // PA0, PA1 => Alternate Function
    Macro_Write_Block(GPIOA->AFR[0], 0xFF, 0x22, 0);    // PA0, PA1 => AF2 (TIM5) 연결
    
    Macro_Clear_Bit(GPIOA->OTYPER, 0);
    Macro_Clear_Bit(GPIOA->OTYPER, 1);
    
    TIM5->PSC = 11;
    TIM5->ARR = 79999;     
    
    // TIM5 CH1, CH2 PWM 모드 설정
    TIM5->CCMR1 = (0x6 << 4) | (1 << 3) | (0x6 << 12) | (1 << 11);
    TIM5->CCER = (1 << 0) | (1 << 4); 
    
    TIM5->CCR1 = 0;
    TIM5->CCR2 = 0;
    
    TIM5->EGR |= 1;
    TIM5->CR1 |= 1; // TIM5 시작


    // 2. 서보 모터 설정 (PA6 -> TIM3_CH1)
    Macro_Set_Bit(RCC->APB1ENR, 1);    // TIM3 클럭 활성화
    
    // PA6 핀을 Alternate Function(10) 모드로 설정 (12번째 비트부터 2칸)
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x2, 12); 
    
    // PA6를 AF2(TIM3)로 연결 (AFR[0]의 24번째 비트부터 4칸)
    Macro_Write_Block(GPIOA->AFR[0], 0xF, 0x2, 24);

    // TIM3 설정 (서보 모터용 50Hz PWM 생성)
    // 84MHz / 84 = 1,000,000Hz (1us 단위)
    TIM3->PSC = 84 - 1; 
    // 1,000,000 / 20,000 = 50Hz (20ms 주기)
    TIM3->ARR = 20000 - 1; 

    // PWM 모드 1 설정 (CH1)
    TIM3->CCMR1 |= (6 << 4); 
    // 출력 활성화
    TIM3->CCER |= (1 << 0);
    // 타이머 시작
    TIM3->CR1 |= (1 << 0);
    // 처음 시작시 서보 끄기
    TIM3->CCR1 = 0;

    // 3. 스텝 모터 설정 (PC0~PC3)
    RCC->AHB1ENR |= (1 << 2); 
    GPIOC->MODER &= ~(0xFF << 0);
    GPIOC->MODER |=  (0x55 << 0);

    // PC6~PC9 핀(12~19번 비트)의 모드를 초기화(00)
    GPIOC->MODER &= ~(0xFF << 12); 
    // PC6~PC9 핀을 일반 출력(01) 모드로 설정
    GPIOC->MODER |=  (0x55 << 12);
}

// ====================
// 서보 모터
// ====================
void Servo_Open_Close(void)
{
    // 1. 90도로 이동 (보통 1.5ms 펄스 = CCR값 1500)
    // 1500~2000 사이에서 조절, 직접 해봐야함
    TIM3->CCR1 = 1900; 
    TIM2_Delay(200);
    TIM3->CCR1 = 1800; 
    TIM2_Delay(200);
    TIM3->CCR1 = 1700; 
    TIM2_Delay(200);
    TIM3->CCR1 = 1600; 
    TIM2_Delay(200);
    TIM3->CCR1 = 1400; 
    printf("[SERVO] Lid Opening (90 deg)...\r\n");

    // 2. 2초간 상태 유지
    TIM2_Delay(2000);

    // 3. 다시 원래 자리(0도)로 복귀 (보통 0.5ms 펄스 = CCR값 500)
    TIM3->CCR1 = 2000; 
    printf("[SERVO] Lid Closing (0 deg)...\r\n");
    
    // 복귀할 시간 딜레이
    TIM2_Delay(500);

    // 4. 뚜껑이 다 닫혔으면 서보 모터 신호를 끊기
    TIM3->CCR1 = 0; 
    printf("[SERVO] Signal OFF (Resting)...\r\n");
}

// ====================
// 스텝 모터
// ====================
/*
*  28BYJ-48 모터: 1바퀴 32스텝
*  내부 1:64 기어
*  총 360도 도는데 32스텝 × 64 = 2048스텝
*
*  대형 기어가 1:78 비율, 7일이라 1일당 78/7 * 2048 스탭 돌아야함
*/

// 첫번째 스텝 모터 1스텝
void Stepper_Step(int step_num) {
    unsigned int temp = GPIOC->ODR;
    
    // PC0~PC3 비트 0으로 설정
    temp &= ~(0xF << 0); 
    
    // 스텝에 맞게 핀(전자석) 켜기
    switch(step_num % 4) {
        case 0: temp |= 0x09; break; // 1001
        case 1: temp |= 0x03; break; // 0011
        case 2: temp |= 0x06; break; // 0110
        case 3: temp |= 0x0C; break; // 1100
    }
    
    // 레지스터에 적용
    GPIOC->ODR = temp;
}

// 12도 회전 함수 (68 스텝 구동)
void Rotate_Next_Slot(void) {
    static int current_step = 0; 

    for(int i = 0; i < 1141; i++) {
        Stepper_Step(current_step);
        current_step++;

        // 스텝마다 2ms 대기
        TIM2_Delay(5); 
    }
    
    // 회전 완료 후 끄기
    GPIOC->ODR &= ~(0xF << 0); 
}

// 두번쨰 스텝 모터
void Stepper2_One_Day(void)
{
    static int current_step = 0; 

    for(int i = 0; i < 2290; i++) // 1일 돌기 처음 돌때 스탭이 무시되서 조금 더 스탭 높혔음
    {
        Stepper_Step(current_step); 
        current_step++;

        // 스텝마다 2ms 대기
        TIM2_Delay(5); 
    }

    GPIOC->ODR &= ~(0xF << 0); 
}

// 두 번째 스텝 모터 1스텝
void Stepper2_Step(int step_num) {
    unsigned int temp = GPIOC->ODR;
    
    // PC6~PC9 비트 0으로 설정
    temp &= ~(0xF << 6); 
    
    int step_val = 0;
    switch(step_num % 4) {
        case 0: step_val = 0x09; break; // 1001
        case 1: step_val = 0x03; break; // 0011
        case 2: step_val = 0x06; break; // 0110
        case 3: step_val = 0x0C; break; // 1100
    }
    
    // 계산된 스텝 값 PC6~PC9 위치에 맞게 6칸 왼쪽으로 밀기
    temp |= (step_val << 6); 
    
    // 레지스터 적용
    GPIOC->ODR = temp;
}

// 두 번째 스텝 모터 알약 자동 넣기
void Supply_Pill(void) { 
    static int current_step = 0; 

    for(int i = 0; i < 1141; i++) {
        Stepper2_Step(current_step);
        current_step++;

        TIM2_Delay(5); 
    }
    
    GPIOC->ODR &= ~(0xF << 6); 
}

// =======
// DC 모터 
// =======
#define TIM5_FREQ_LOCAL (8000000U)

#define MOTOR_STOP   0
#define MOTOR_CW     1
#define MOTOR_CCW   -1
static unsigned int Motor_Percent = 37;   // 50 ~ 100%
static int Motor_Dir = MOTOR_STOP;  

void Motor_Set_Percent(unsigned int percent)
{
   if(percent < 20) percent = 20;
   if(percent > 100) percent = 100;

   Motor_Percent = percent;
}

unsigned int Motor_Get_Percent(void)
{
   return Motor_Percent;
}

int Motor_Get_Dir(void)
{
   return Motor_Dir;
}

void Motor_Apply_Duty(void)
{
   unsigned int duty;

   duty = (unsigned int)(((unsigned long)(TIM5->ARR + 1) * Motor_Percent) / 100);

   if(Motor_Dir == MOTOR_CW)
   {
      TIM5->CCR2 = 0;
      TIM5->CCR1 = duty;
   }
   else if(Motor_Dir == MOTOR_CCW)
   {
      TIM5->CCR1 = 0;
      TIM5->CCR2 = duty;
   }
   else
   {
      TIM5->CCR1 = 0;
      TIM5->CCR2 = 0;
   }
}

void Stop(void)
{
   TIM5->CCR1 = 0;
   TIM5->CCR2 = 0;
   Motor_Dir = MOTOR_STOP;
}

void Move_CW(void)
{
   TIM5->CCR2 = 0;

   for(volatile int i = 0; i < 100; i++);

   Motor_Dir = MOTOR_CW;
   Motor_Apply_Duty();
}

void Move_CCW(void)
{
   TIM5->CCR1 = 0;

   for(volatile int i = 0; i < 100; i++);

   Motor_Dir = MOTOR_CCW;
   Motor_Apply_Duty();
}

