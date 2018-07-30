/********************************** (C) COPYRIGHT *******************************
* File Name		: LCD1602.C
* Author		: Kongou Hikari
* License		: MIT
* Version		: V1.0
* Date			: 2018/07/21
* Description		: 8051 驱动1602
*******************************************************************************/
#include <8051.h>
#include <stdint.h>
#include "i2c.h"

#define LCD_DATA P0
#define LCD_RS P2_0
#define LCD_RW P2_1
#define LCD_EN P2_2
__sfr __at 0x94 P0M0; //                                                                         0000,0000
__sfr __at 0x93 P0M1; //
void LCD_Delay(uint8_t i)
{
	uint8_t j = 10;
	while(i--)
		while(j--);
}
void LCD_WriteCommand(uint8_t cmd)
{
	LCD_RS = 0;
	LCD_RW = 0;
	P0 = cmd;
	LCD_Delay(1);
	LCD_EN = 1;
	LCD_Delay(1);
	LCD_EN = 0;
	LCD_Delay(5);
}
void LCD_WriteData(uint8_t data)
{
	LCD_RS = 1;
	LCD_RW = 0;
	P0 = data;
	LCD_Delay(1);
	LCD_EN = 1;
	LCD_EN = 0;
	LCD_Delay(3);
}

void LCD_Init()
{
	P0M0 = 0x00;
	P0M1 = 0x00; //P0口准双向
	LCD_RS = 1;
	LCD_RW = 1;
	LCD_EN = 1;
	LCD_WriteCommand(0x38);
	LCD_Delay(30);
	LCD_WriteCommand(0x0C);
	LCD_Delay(4);
	LCD_WriteCommand(0x06);
	LCD_Delay(4);
	LCD_WriteCommand(0x01);

}

void LCD_PutString(uint8_t x, uint8_t y, char *s)
{
	LCD_WriteCommand(0x80 + y * 0x40 + x); //写入字符位置
	while(*s)
		LCD_WriteData(*s++);
}

void LCD_Puts(char *s)
{
	while(*s)
		LCD_WriteData(*s++);
}

