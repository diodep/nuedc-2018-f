/********************************** (C) COPYRIGHT *******************************
* File Name		: LCD1602.H
* Author		: Kongou Hikari
* License		: MIT
* Version		: V1.0
* Date			: 2018/07/21
* Description		: 8051 Çý¶¯1602
*******************************************************************************/

extern void LCD_WriteCommand(uint8_t cmd);
extern void LCD_WriteData(uint8_t data);
extern void LCD_Init();
extern void LCD_PutString(uint8_t x, uint8_t y, char *s);
extern void LCD_Puts(char *s);

#define LCD_Cls() LCD_WriteCommand(0x01)
#define LCD_OffCur() LCD_WriteCommand(0x0C)
#define LCD_OnCur() LCD_WriteCommand(0x0F)
#define LCD_SetCur(x, y) LCD_WriteCommand(0x80 | y * 0x40 | x)
