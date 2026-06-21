#include "device_driver.h"

// I2C LCD 기본 주소, LCD 남땜해서 바꿔야함
#define LCD1_ADDR 0x4E   // 메인 LCD (0x27 << 1)
#define LCD2_ADDR 0x4C   // 상태 LCD (0x26 << 1)

// I2C 통신 설정 (PB8: SCL, PB9: SDA)
void I2C1_Init(void)
{
	// 1. 클럭 활성화 (GPIOB, I2C1)
	Macro_Set_Bit(RCC->AHB1ENR, 1);
	Macro_Set_Bit(RCC->APB1ENR, 21);

	// 2. PB8, PB9 핀 설정, I2C할떄 Open-Drain 해야함
	Macro_Write_Block(GPIOB->MODER, 0b1111, 0b1010, 16);  	// AF 모드 (10|10)
	Macro_Write_Block(GPIOB->OTYPER, 0b11, 0b11, 8);  		// Open-Drain
	Macro_Write_Block(GPIOB->PUPDR, 0b1111, 0b0101, 16); 	 // Pull-Up (01|01)
	Macro_Write_Block(GPIOB->AFR[1], 0b11111111, 0b01000100, 0);  // AF4 (0100|0100)

	// 3. I2C1 소프트웨어 리셋
	Macro_Set_Bit(I2C1->CR1, 15);
	Macro_Clear_Bit(I2C1->CR1, 15);

	// 4. 통신 속도 이거 사용함 (APB1 42MHz, 100kHz)
	I2C1->CR2   = 42;
	I2C1->CCR   = 210;
	I2C1->TRISE = 43;

	// 5. I2C 활성화
	Macro_Set_Bit(I2C1->CR1, 0);
}


// 8비트를 상위/하위 4비트로 쪼개서 전송 (4bit 모드라 두 번 보냄)
// mode=0: 명령어(RS=0), mode=1: 글자(RS=1)
void LCD_Send(unsigned char addr, char data, int mode)
{
	while(Macro_Check_Bit_Set(I2C1->SR2, 1));				// BUSY 대기

	Macro_Set_Bit(I2C1->CR1, 8);							//S		// Start
	while(Macro_Check_Bit_Clear(I2C1->SR1, 0));				//EV5	// Check Start

	I2C1->DR = addr;										//ADDRESS
	while(Macro_Check_Bit_Clear(I2C1->SR1, 1));				//ACK	// Check Address
	(void)I2C1->SR1; (void)I2C1->SR2;						//EV6	// Clear ADDR flag
	while(Macro_Check_Bit_Clear(I2C1->SR1, 7));				//EV8_1	// Check TxE

	// 보내는 형식
	// p7 p6 p5 p4 p3 p2 p1 p0
	// d7 d6 d5 d4 bl en rw rs
	// 상위 4비트 전송
	I2C1->DR = (data & 0xf0) | 0b1100 | mode;	// BackLight:1, EN:1, RW:0, RS=mode
	while(Macro_Check_Bit_Clear(I2C1->SR1, 2));				//EV8	// Check Byte Transfer Finished
	while(Macro_Check_Bit_Clear(I2C1->SR1, 7));				//TxE

	I2C1->DR = (data & 0xf0) | 0b1000 | mode;	// BackLight:1, EN:0, RW:0, RS=mode  → Falling Edge로 LCD 래치
	while(Macro_Check_Bit_Clear(I2C1->SR1, 2));				//EV8
	while(Macro_Check_Bit_Clear(I2C1->SR1, 7));				//TxE

	// 하위 4비트 전송
	I2C1->DR = ((data << 4) & 0xf0) | 0b1100 | mode;	// BackLight:1, EN:1, RW:0, RS=mode
	while(Macro_Check_Bit_Clear(I2C1->SR1, 2));				//EV8
	while(Macro_Check_Bit_Clear(I2C1->SR1, 7));				//TxE

	I2C1->DR = ((data << 4) & 0xf0) | 0b1000 | mode;	// BackLight:1, EN:0, RW:0, RS=mode  → Falling Edge로 LCD 래치
	while(Macro_Check_Bit_Clear(I2C1->SR1, 2));				//EV8
	while(Macro_Check_Bit_Clear(I2C1->SR1, 7));				//TxE

	Macro_Set_Bit(I2C1->CR1, 9);							//P		// Stop
	while(Macro_Check_Bit_Set(I2C1->CR1, 9));				// Check Stop (Auto Cleared)
}

// 명령어 전송(RS=0) / 글자 전송(RS=1)
void LCD_Cmd(unsigned char addr, char cmd)   
{ 
	LCD_Send(addr, cmd, 0); 
}
void LCD_Data(unsigned char addr, char data) 
{ 
	LCD_Send(addr, data, 1); 
}

// 니블 1개만 전송 (초기화할떄만 씀, 8bit 모드 상태에서 상위 4비트만 보냄)
static void LCD_Init_Nibble(unsigned char addr, char nibble)
{
	while(Macro_Check_Bit_Set(I2C1->SR2, 1));

	Macro_Set_Bit(I2C1->CR1, 8);
	while(Macro_Check_Bit_Clear(I2C1->SR1, 0));

	I2C1->DR = addr;
	while(Macro_Check_Bit_Clear(I2C1->SR1, 1));
	(void)I2C1->SR1; (void)I2C1->SR2;
	while(Macro_Check_Bit_Clear(I2C1->SR1, 7));

	I2C1->DR = (nibble & 0xf0) | 0b1100;	// BackLight:1, EN:1, RW:0, RS:0
	while(Macro_Check_Bit_Clear(I2C1->SR1, 2));
	while(Macro_Check_Bit_Clear(I2C1->SR1, 7));

	I2C1->DR = (nibble & 0xf0) | 0b1000;	// BackLight:1, EN:0 → Falling Edge로 LCD 래치
	while(Macro_Check_Bit_Clear(I2C1->SR1, 2));
	while(Macro_Check_Bit_Clear(I2C1->SR1, 7));

	Macro_Set_Bit(I2C1->CR1, 9);
	while(Macro_Check_Bit_Set(I2C1->CR1, 9));
}

// LCD 패널 초기화 (I2C1_Init은 먼저 불러와야함!!!!)
void LCD_Init(unsigned char addr)
{
	TIM2_Delay(20);							// 전원 켜지고 15ms 이상 대기

	// 처음엔 8bit 모드라 상위니블만 3번 보내서 상태 맞춤
	LCD_Init_Nibble(addr, 0b00110000);	// 8bit 모드
	TIM2_Delay(5);
	LCD_Init_Nibble(addr, 0b00110000);	// 8bit 모드
	TIM2_Delay(1);
	LCD_Init_Nibble(addr, 0b00110000);	// 8bit 모드
	TIM2_Delay(1);

	LCD_Init_Nibble(addr, 0b00100000);	// 4bit 모드로 전환
	TIM2_Delay(1);

	LCD_Cmd(addr, 0b00101000);	// function set : 4bit, 2줄, 5x8 폰트
	LCD_Cmd(addr, 0b00001000);	// display OFF
	LCD_Cmd(addr, 0b00000001);	// display clear
	TIM2_Delay(2);				// clear 대기
	LCD_Cmd(addr, 0b00000110);	// entryMode set, 글자 쓸 때 커서 우측 이동
	LCD_Cmd(addr, 0b00001100);	// display on, 커서 OFF
}

// 커서 위치 이동 (x: 가로칸 0~15, y: 세로줄 0~1)
void LCD_Set_Cursor(unsigned char addr, int y, int x)
{
	if (y == 0) LCD_Cmd(addr, 0b10000000 + x);		// 첫 번째 줄
	else        LCD_Cmd(addr, 0b11000000 + x);		// 두 번째 줄
}

// 문자열 끝(\0)까지 한 글자씩 전송
void LCD_String(unsigned char addr, char *str)
{
	while(*str) LCD_Data(addr, *str++);
}
