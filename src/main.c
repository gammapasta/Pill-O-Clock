#include "device_driver.h"
#include <stdio.h>
#include <string.h>

#define REST 0

// LCD 어드레스
#define LCD1_ADDR 0x4E
#define LCD2_ADDR 0x4C

#define STATE_IDLE      0
#define STATE_FORWARD   1
#define STATE_WAIT      2
#define STATE_BACKWARD  3
#define STATE_FINISHED  4

volatile int system_mode = 0;
volatile int pill_alarm_flag = 0;
int current_state = STATE_IDLE;


// [디버그] 부저 상태
volatile int buzzer_state = 0;

// [디버그] 테라텀으로 제어하기 
volatile unsigned char Uart_Data = 0;
volatile char uart2_buffer[64];
volatile int uart2_rx_index = 0;
volatile int uart2_rx_exist = 0;


// 자동 채우기 기능용 전역 변수
volatile int auto_load_flag = 0;
char auto_load_data[8] = "0000000"; 

static void Sys_Init(void)
{
    SCB->CPACR |= (0x3 << 10*2) | (0x3 << 11*2);
    Clock_Init();
    Uart2_Init(115200); // PC 테라텀용
    Uart1_Init(9600);   // 블루투스 모듈용
    setvbuf(stdout, NULL, _IONBF, 0);

    LED_Init();
    Status_LED_Init(); // 외부 상태 표시 LED 6개 초기화
    Motor_Init();
    I2C1_Init();
    LCD_Init(LCD1_ADDR);      // 메인 LCD(0x4E) 초기화
    LCD_Init(LCD2_ADDR);      // 두번째 LCD(0x4C) 초기화
    Ultrasonic_Init();
    Key_Poll_Init();

    // 패시브 부저용: TIM4
    Buzzer_Init();

    Uart2_RX_Interrupt_Enable();

    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    NVIC_SetPriority(RTC_Alarm_IRQn, 1);              // RTC Alarm
    NVIC_SetPriority(EXTI15_10_IRQn, 2);              // EXTI15_10
    NVIC_SetPriority(USART2_IRQn, 3);              // USART2
}

// 7일 치 약통 자동 분배 함수
void Auto_Load_Sequence(const char* week_data) 
{
    printf("\r\n[LOADING] Start Auto-Loading Sequence for 7 days...\r\n");
    
    // 상태 표시 LCD 업데이트
    LCD_Set_Cursor(LCD2_ADDR, 0, 0);
    LCD_String(LCD2_ADDR, "State: LOADING ");

    // 7칸(월~일)을 순서대로 돌면서 확인
    for(int i = 0; i < 7; i++) {
        char lcd_buf[20];
        
        // 현재 칸이 채우는 칸인지 건너뛰는 칸인지 화면에 표시
        sprintf(lcd_buf, "Slot %d: %s      ", i+1, (week_data[i] == '1') ? "LOADING" : "SKIP");
        LCD_Set_Cursor(LCD2_ADDR, 1, 0);
        LCD_String(LCD2_ADDR, lcd_buf);
        printf("[LOADING] Slot %d -> %s\r\n", i+1, (week_data[i] == '1') ? "LOADING" : "SKIP");

        // 앱에서 '1'로 선택한 요일이면 약 공급
        if(week_data[i] == '1') {
            Status_LED_Red(); // 채울 때 노란색 불빛 켜기
            Supply_Pill();       // 두 번째 스텝 모터(호퍼) 작동
            TIM2_Delay(500);     // 캡슐이 안정적으로 떨어지도록 0.5초 대기
        }
                    
        // 다음 요일 칸으로 메인 약통 회전
        Status_LED_All_Off();
        Stepper2_One_Day();
        TIM2_Delay(500); 
    }
    
    printf("[LOADING] Sequence Complete!\r\n");
    LCD_Set_Cursor(LCD2_ADDR, 0, 0);
    LCD_String(LCD2_ADDR, "State: LOAD DONE");

}


static void LCD2_Show_State(int state, int dist)
{
    LCD_Set_Cursor(LCD2_ADDR, 0, 0);

    switch(state)
    {
        case STATE_IDLE:
            LCD_String(LCD2_ADDR, "State: IDLE    ");
            LCD_Set_Cursor(LCD2_ADDR, 1, 0);
            LCD_String(LCD2_ADDR, "Waiting Alarm  ");
            break;
        case STATE_FORWARD:
            LCD_String(LCD2_ADDR, "State: FORWARD ");
            LCD_Set_Cursor(LCD2_ADDR, 1, 0);
            LCD_String(LCD2_ADDR, "Pill Moving... ");
            break;
        case STATE_WAIT:
            LCD_String(LCD2_ADDR, "State: WAIT BTN");
            LCD_Set_Cursor(LCD2_ADDR, 1, 0);
            LCD_String(LCD2_ADDR, "Press Button   ");
            break;
        case STATE_BACKWARD:
            LCD_String(LCD2_ADDR, "State: BACKWARD");
            LCD_Set_Cursor(LCD2_ADDR, 1, 0);
            LCD_String(LCD2_ADDR, "Returning...   ");
            break;
        case STATE_FINISHED:
            LCD_String(LCD2_ADDR, "State: DONE    ");
            LCD_Set_Cursor(LCD2_ADDR, 1, 0);
            LCD_String(LCD2_ADDR, "Sequence End   ");
            break;
        default:
            LCD_String(LCD2_ADDR, "State: UNKNOWN ");
            LCD_Set_Cursor(LCD2_ADDR, 1, 0);
            LCD_String(LCD2_ADDR, "Check System   ");
            break;
    }
}

void Main(void)
{
    Sys_Init();
    RTC_Init_And_Alarm_Set(0, 0, 0);

    static int last_sec = -1;
    char key;
    int dist;

    printf("=== Integrated Pill Dispenser System Ready ===\r\n");
    printf("Waiting for Alarm or Press 'f' for manual test.\r\n");
    printf("Passive Buzzer : PB6 (TIM4_CH1)\r\n");
    
    // 시작할 때 모든 외부 LED 끄기
    Status_LED_All_Off();

    while(1)
    {
        // 0. 스마트폰(UART1)에서 날아오는 명령 처리
        Process_UART_Input();

        // 1. 알약 자동 채우기
        if (auto_load_flag == 1) {
            Auto_Load_Sequence(auto_load_data); // 자동 채우기 시퀀스 실행
            auto_load_flag = 0;                 // 플래그 초기화
            current_state = STATE_IDLE;         // 완료 후 대기 상태로 복귀
        }

        // 2. 실시간 시계 출력 로직
        unsigned int tr = RTC->TR;

        int s = BCD_TO_DEC(tr & 0x7F);
        int m = BCD_TO_DEC((tr >> 8) & 0x7F);
        int h = BCD_TO_DEC((tr >> 16) & 0x3F);

        // 시간이 1초 흘렀을 때만 화면 갱신
        if (s != last_sec) {
            unsigned int alr = RTC->ALRMAR;
            int as = BCD_TO_DEC(alr & 0x7F);
            int am = BCD_TO_DEC((alr >> 8) & 0x7F);
            int ah = BCD_TO_DEC((alr >> 16) & 0x3F);

            printf("\r[PC] Time %02d:%02d:%02d | Alarm %02d:%02d:%02d | Buzzer %d  ",
                   h, m, s, ah, am, as, buzzer_state);

            char lcd_buffer[20];

            sprintf(lcd_buffer, "Clock : %02d:%02d:%02d", h, m, s);
            LCD_Set_Cursor(LCD1_ADDR, 0, 0);
            LCD_String(LCD1_ADDR, lcd_buffer);

            sprintf(lcd_buffer, "Alarm : %02d:%02d:%02d", ah, am, as);
            LCD_Set_Cursor(LCD1_ADDR, 1, 0);
            LCD_String(LCD1_ADDR, lcd_buffer);

            last_sec = s;
        }

        // 3. 알람 발생 시 동작
        if (pill_alarm_flag == 1) {
            printf("\r\n\r\n[ACTION] It's pill time! Rotating...\r\n");
            // 약이 배출될 때: 빨간색 LED ON
            Status_LED_Red();

            Rotate_Next_Slot();
            Servo_Open_Close();
            TIM2_Delay(200);

            
            
            pill_alarm_flag = 0;
            printf("\r\n");

            if (current_state == STATE_IDLE || current_state == STATE_FINISHED) {
                printf("[1/4] Pill dropped. Conveyor Auto-Start...\r\n");

                // 컨베이어 이동 시작: 노란색 LED ON!
                Status_LED_Red();                
                Motor_Set_Percent(50);
                Move_CW();
                current_state = STATE_FORWARD;
            }

        }

        // 4. 컨베이어 벨트 상태 머신 및 거리 측정
        dist = Ultrasonic_Get_Distance();

        switch (current_state)
        {
            case STATE_IDLE:
            case STATE_FINISHED:
                key = Uart2_Get_Pressed();
                if (key == 'f' || key == 'F') {
                    printf("\n[ACTION] Manual pill drop...\n");
                    // 수동 약 배출: 빨간색 LED ON
                    Status_LED_Green();
                    Rotate_Next_Slot();
                    Servo_Open_Close();

                    printf("[1/4] Manual Start. Moving Forward...\n");
                    // 컨베이어 전진 시작: 노란색 LED ON
                    Status_LED_Green();
                    Motor_Set_Percent(50);
                    Move_CW();
                    current_state = STATE_FORWARD;
                }
                break;

            case STATE_FORWARD:
                // 필요하면 <=1 을 <=3 으로 완화해서 테스트
                if (dist > 0 && dist <= 1) {
                    Stop();

                    printf("\r\n[DEBUG] dist = %d -> Stop()\r\n", dist);
                    Buzzer_On();

                    // 도착해서 멈추면 일단 LED off
                    Status_LED_All_Off();

                    printf("[2/4] 1cm detected. Motor stop, passive buzzer ON.\r\n");
                    printf("Press external switch(PB5) button to stop buzzer and reverse.\r\n");

                    current_state = STATE_WAIT;
                }
                break;

            case STATE_WAIT:
                if (Key_Get_Pressed()) {
                    printf("[DEBUG] External switch pressed\r\n");
                    Buzzer_Off();

                    printf("[3/4] Button pressed. Buzzer OFF. Reversing...\r\n");
                    // 역회전 복귀 시작: 초록색 LED ON
                    Status_LED_Red();

                    Motor_Set_Percent(50);
                    Move_CCW();

                    Key_Wait_Key_Released();
                    TIM2_Delay(50);
                    current_state = STATE_BACKWARD;
                }
                break;

            case STATE_BACKWARD:
                if (dist >= 15) {
                    Stop();
                    Buzzer_Off();

                    // 복귀 완료 후 LED 모두 끄기
                    Status_LED_All_Off();


                    printf("\r\n[4/4] Reached 15cm. Sequence Finished.\r\n");
                    current_state = STATE_FINISHED;

                    Rotate_Next_Slot();
                    TIM2_Delay(200);
                    Supply_Pill();
                }
                break;
        }

        LCD2_Show_State(current_state, dist); // [수정] 상태값을 인자로 넘겨서 LCD2 표시
        Delay_ms(100);



        //================================
        // uart 로 디버그
        //==================================
        if (uart2_rx_exist) 
        {
            
            // 1. 시간 동기화 (T)
            if (strcmp((const char *)uart2_buffer, "T") == 0  || strcmp((const char *)uart2_buffer, "t") == 0) 
            {
                int h = 0;
                int m = 0;
                int s = 0;

                Set_Current_Time(h, m, s);
                printf("\r\n[SYNC] Current Time Synced -> %02d:%02d:%02d\r\n", h, m, s);
                Uart1_Printf("T:%02d:%02d:%02d\n", h, m, s); 
                
            }

            // 2. 알람 설정 (A)
            else if (strcmp((const char *)uart2_buffer, "A") == 0  || strcmp((const char *)uart2_buffer, "a") == 0) 
            {
                int h = 0;
                int m = 0;
                int s = 3;
                

            RTC->WPR = 0xCA; RTC->WPR = 0x53;
            RTC->CR &= ~(1 << 8); 
            while(!(RTC->ISR & (1 << 0))); 
            
            RTC->ALRMAR = (1 << 31) | (TO_BCD(h) << 16) | (TO_BCD(m) << 8) | TO_BCD(s);
            
            RTC->CR |= (1 << 8); 
            RTC->WPR = 0xFF;     
            
            printf("\r\n[SET] Alarm Updated -> %02d:%02d:%02d\r\n", h, m, s);
            Uart1_Printf("A:%02d:%02d:%02d\n", h, m, s); 
                
            }

            //스테퍼모터 설정 (s)
            else if(strcmp((const char *)uart2_buffer, "s") == 0)
            {
                printf("\r\nstepper start\r\n");
                int temp = 0;
                for (int k = 0; k < 1144; k++)
                {
                    Stepper2_Step(temp);
                    temp++;
                    TIM2_Delay(5); 
                }
                
                
                
            }

            // 알약 집어 넣기 임시 (store)
            else if(strcmp((const char *)uart2_buffer, "store") == 0)
            {
                printf("\nstore\n");
                for (int i = 0; i < 7; i++)
                {
                    Stepper2_One_Day();
                    Supply_Pill();

                    TIM2_Delay(5);

                }
                Rotate_Next_Slot();
                
                
            }

            // 스텝모터1 살짝 움직이기 (step)
            else if (strcmp((const char *)uart2_buffer, "step") == 0)
            {
                    static int current_step = 0; 

                    for(int i = 0; i < 300; i++) {
                        Stepper_Step(current_step);
                        current_step++;

                        // 한 스텝마다 2ms 대기
                        TIM2_Delay(5); 
                    }
                    
                    // 회전 완료 후 대기 상태일 때 모터 발열 방지 (전류 차단)
                    GPIOC->ODR &= ~(0xF << 0); 
            }
            

            // 서보모터 움직이기 (servo)
            else if (strcmp((const char *)uart2_buffer, "servo") == 0)
            {
                Servo_Open_Close();
            }

            else if (uart2_buffer[0] == 'L' || uart2_buffer[0] == 'l') {
                if (strlen((const char * restrict)uart2_buffer) >= 8) { // L + 7자리 데이터(월~일)
                        printf("uart2_buffer 받음\n");
                        // 7자리 요일 데이터를 복사 (예: "1010100" 월수금)
                        strncpy(auto_load_data, (const char *)(uart2_buffer + 1), 7); // L 제외 7자리
                        auto_load_data[7] = '\0'; 
                        auto_load_flag = 1; 
                        printf("\r\n[BLE] Auto-Load Command Received: %s\r\n", auto_load_data);

                    }
                }
            
            uart2_buffer[0] = '\0'; 
            uart2_rx_exist = 0;
        }



    }
}
