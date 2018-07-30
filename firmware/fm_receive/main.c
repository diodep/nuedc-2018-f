/********************************** (C) COPYRIGHT *******************************
* File Name		: MAIN.C
* Author		: Kongou Hikari
* License		: MIT
* Version		: V1.0
* Date			: 2018/07/21
* Description		: 2018电赛F题接收机程序
*******************************************************************************/
#include <8052.h>
#include <stdint.h>
#include <stdio.h>
#include "i2c.h"
#include "lcd1602.h"
#include "eeprom.h"

__sfr  __at 0x8E  AUXR; //Auxiliary Register  T0x12 T1x12 UART_M0x6 BRTR S2SMOD BRTx12 EXTRAM S1BRS  0000,0000
//-----------------------------------
__sfr  __at 0xA2 AUXR1; //Auxiliary Register 1  -  PCA_P4  SPI_P4  S2_P4  GF2    ADRJ   -    DPS  0000,0000
__sfr  __at 0x9C BRT; //S2 Baud-Rate Timer

#define FOSC 8000000UL      //System frequency
#define BAUD 9600UL           //UART baudrate

/*Define UART parity mode*/
#define NONE_PARITY     0   //None parity
#define ODD_PARITY      1   //Odd parity
#define EVEN_PARITY     2   //Even parity
#define MARK_PARITY     3   //Mark parity
#define SPACE_PARITY    4   //Space parity

#define PARITYBIT NONE_PARITY   //Testing even parity


void putchar(uint8_t dat);
void SendString(char *s);

#define dbg(s) SendString(s)

uint16_t rx_freq1;
uint16_t rx_freq2;

uint8_t rx_vol1;
uint8_t rx_vol2;

uint8_t rssi1 = 0;
uint8_t rssi2 = 0;

uint8_t state = 0;

#define POS_THRESHOLD 54
#define NEG_THRESHOLD 47

void UartInit(void)		//9600bps@8MHz
{
	PCON &= 0x7F;		//波特率不倍速
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x40;		//定时器1时钟为Fosc,即1T
	AUXR &= 0xFE;		//串口1选择定时器1为波特率发生器
	TMOD &= 0x0F;		//清除定时器1模式位
	TMOD |= 0x20;		//设定定时器1为8位自动重装方式
	TL1 = 0xE6;		//设定定时初值
	TH1 = 0xE6;		//设定定时器重装值
	ET1 = 0;		//禁止定时器1中断
	TR1 = 1;		//启动定时器1
}



void Delayms(uint16_t ms)		//@8MHz
{
	unsigned char i, j;
	while(ms--)
	{
		__asm
		nop
		nop
		__endasm;
		i = 8;
		j = 196;
		do
		{
			while (--j);
		} while (--i);
	}
}


uint8_t RDA5820_Write(uint8_t i2c_bus, uint8_t reg_addr, uint16_t write_data)
{
	if(i2c_bus == 0)
	{
		i2c_start();

		i2c_write(0x22); //RDA5820地址
		if(i2c_read_nak())
		{
			i2c_stop();
			dbg("Fetch address failed!\r\n");
			return -1;
		}
		i2c_write(reg_addr); //寄存器地址
		if(i2c_read_nak())
		{
			i2c_stop();
			dbg("Set register failed\r\n");
			return -2;
		}
		i2c_write(write_data >> 8); //寄存器地址
		if(i2c_read_nak())
		{
			i2c_stop();
			dbg("Write register failed\r\n");
			return -2;
		}
		i2c_write(write_data & 0xff); //寄存器地址
		i2c_read_nak();

		i2c_stop();
	}
	else
	{
		i2c1_start();

		i2c1_write(0x22); //RDA5820地址
		if(i2c1_read_nak())
		{
			i2c1_stop();
			dbg("Fetch address failed!\r\n");
			return -1;
		}
		i2c1_write(reg_addr); //寄存器地址
		if(i2c1_read_nak())
		{
			i2c1_stop();
			dbg("Set register failed\r\n");
			return -2;
		}
		i2c1_write(write_data >> 8); //寄存器地址
		if(i2c1_read_nak())
		{
			i2c1_stop();
			dbg("Write register failed\r\n");
			return -2;
		}
		i2c1_write(write_data & 0xff); //寄存器地址
		i2c1_read_nak();

		i2c1_stop();

	}
	return 0;
}
uint8_t rda_read_err;

uint16_t RDA5820_Read(uint8_t i2c_bus, uint8_t reg_addr)
{
	uint8_t hi8, lo8;
	rda_read_err = 0;
	if(i2c_bus == 0)
	{
		i2c_start();

		i2c_write(0x22); //RDA5820地址
		if(i2c_read_nak())
		{
			i2c_stop();
			dbg("Fetch address failed!\r\n");
			rda_read_err = 3;
			return 0;
		}
		i2c_write(reg_addr); //寄存器地址
		if(i2c_read_nak())
		{
			i2c_stop();
			dbg("Set register failed\r\n");
			rda_read_err = 2;
			return 0;
		}

		i2c_start();
		i2c_write(0x23); //RDA5820地址
		if(i2c_read_nak())
		{
			i2c_stop();
			dbg("Fetch read address failed!\r\n");
			rda_read_err = 1;
			return 0;
		}

		hi8 = i2c_read();
		i2c_send_ack();
		lo8 = i2c_read();
		i2c_read_nak();

		i2c_stop();
	}
	else
	{
		i2c1_start();

		i2c1_write(0x22); //RDA5820地址
		if(i2c1_read_nak())
		{
			i2c1_stop();
			dbg("Fetch address failed!\r\n");
			rda_read_err = 3;
			return 0;
		}
		i2c1_write(reg_addr); //寄存器地址
		if(i2c1_read_nak())
		{
			i2c1_stop();
			dbg("Set register failed\r\n");
			rda_read_err = 2;
			return 0;
		}

		i2c1_start();
		i2c1_write(0x23); //RDA5820地址
		if(i2c1_read_nak())
		{
			i2c1_stop();
			dbg("Fetch read address failed!\r\n");
			rda_read_err = 1;
			return 0;
		}

		hi8 = i2c1_read();
		i2c1_send_ack();
		lo8 = i2c1_read();
		i2c1_read_nak();

		i2c1_stop();
	}
	return (hi8 << 8) | lo8;
}


void CheckRDA5820(uint8_t i2c_bus)
{
	uint16_t chipid;
reread:
	chipid = RDA5820_Read(i2c_bus, 0x00);
	if(rda_read_err) goto reread;

	if(chipid == 0x5820)
		SendString("This chip is RDA5820NS\r\n");
	else if(chipid == 0x5805)
		SendString("This chip is RDA5820\r\n");
	else
	{
		while(RDA5820_Write(i2c_bus, 0x02, 0x8001));
		SendString("Unknown chip!\r\n");
	}
}

void SetRDA5820Freq(uint8_t i2c_bus, uint16_t freq) //870~1080
{
    uint16_t freq_set = (freq - 870) / 2;
    uint16_t reg_read;
    uint16_t reg_set = 0x11 | (freq_set << 6); /* channel spacing = 200khz, tune = 1, 87-108M*/
    while(RDA5820_Write(i2c_bus, 0x03, reg_set));
    Delayms(80);
reread:
    reg_read = RDA5820_Read(i2c_bus, 0x0a);
    if(rda_read_err) goto reread;
    //RDA5820_Read(i2c_bus, 0x0b);
    if(reg_read & ( 1<<14 ))
		dbg("Set freq successful\r\n");
	else
		dbg("Set freq unsuccessful\r\n");
}

void SetRDA5820Tx(uint8_t i2c_bus)
{
	while(RDA5820_Write(i2c_bus, 0x40, 0x0001)); //发射模式
    while(RDA5820_Write(i2c_bus, 0x41, 0x3F | 0x1C0)); //PA_GAIN=0x3F, BIAS=0x03
    dbg("Set RDA5820 to TX mode\r\n");
}

void SetRDA5820Rx(uint8_t i2c_bus)
{
	while(RDA5820_Write(i2c_bus, 0x40, 0x0000)); //接收模式
	dbg("Set RDA5820 to RX mode\r\n");
}

void putchar(uint8_t dat)
{
	TI = 0;
    SBUF = dat;
    while(!TI);
}

void SendString(char *s)
{
    while (*s)              //Check the end of the string
    {
        putchar(*s++);     //Send current char and increment string ptr
    }
}

uint8_t CheckStation(uint8_t i2c_bus, uint16_t freq)
{
	uint16_t status_reg;
	uint8_t rssi;
	SetRDA5820Rx(i2c_bus); //将RDA5820切换到接收模式
	SetRDA5820Freq(i2c_bus, freq); //设置频率
	Delayms(10);
reread:
	status_reg = RDA5820_Read(i2c_bus, 0x0b); //读状态寄存器
    if(rda_read_err) goto reread;
	rssi = status_reg >> 9;
	printf("RSSI value = %d\r\n", rssi);
    if(status_reg & (1<<8))
	{//是个台
		dbg("Is Station!\r\n");
		return 1;
	}
	dbg("No station!\r\n");
	return 0;
}

void RDA5820_SetVol(uint8_t i2c_bus, uint8_t vol)
{
	vol &= 0x0f;
	while(RDA5820_Write(i2c_bus, 0x05, (0x03 << 6) | vol));
}

#define KEY_SEL P2_3
#define KEY_ENTER P2_4
#define KEY_PLUS P2_5
#define KEY_MINUS P2_6

void Key_Init()
{
	KEY_SEL = 1;
	KEY_ENTER = 1;
	KEY_PLUS = 1;
	KEY_MINUS = 1;
}


void DisplayMain()
{
	if(state > 0)
		LCD_OnCur();
	else
		LCD_OffCur(); //开关光标

	LCD_Cls();
	LCD_PutString(0, 0, "1:");
	if(rx_freq1 >= 1000)
		LCD_WriteData(rx_freq1 / 1000 + '0');
	LCD_WriteData(rx_freq1 % 1000 / 100 + '0');
	LCD_WriteData(rx_freq1 % 100 / 10 + '0');
	LCD_WriteData('.');
	LCD_WriteData(rx_freq1 % 10 + '0');
	LCD_Puts("M V");
	LCD_WriteData(rx_vol1 / 10 + '0');
	LCD_WriteData(rx_vol1 % 10 + '0');
	LCD_Puts(" S");
	LCD_WriteData(rssi1 / 10 + '0');
	LCD_WriteData(rssi1 % 10 + '0');

	LCD_PutString(0, 1, "2:");
	if(rx_freq2 >= 1000)
		LCD_WriteData(rx_freq2 / 1000 + '0');
	LCD_WriteData(rx_freq2 % 1000 / 100 + '0');
	LCD_WriteData(rx_freq2 % 100 / 10 + '0');
	LCD_WriteData('.');
	LCD_WriteData(rx_freq2 % 10 + '0');
	LCD_Puts("M V");
	LCD_WriteData(rx_vol2 / 10 + '0');
	LCD_WriteData(rx_vol2 % 10 + '0');
	LCD_Puts(" S");
	LCD_WriteData(rssi2 / 10 + '0');
	LCD_WriteData(rssi2 % 10 + '0');

	if(state == 1)
	{
		if(rx_freq1 >= 1000)
			LCD_SetCur(6, 0);
		else
			LCD_SetCur(5, 0);
	}

	if(state == 3)
	{
		if(rx_freq1 >= 1000)
			LCD_SetCur(6, 1);
		else
			LCD_SetCur(5, 1);
	}

	if(state == 2)
	{
		LCD_SetCur(11, 0);
	}
	if(state == 4)
	{
		LCD_SetCur(11, 1);
	}
}

uint8_t RDA5820_GetRSSI(uint8_t i2c_bus)
{
	uint16_t status_reg;
	uint8_t rssi;
reread:
	status_reg = RDA5820_Read(i2c_bus, 0x0b); //读状态寄存器
    if(rda_read_err) goto reread;

	rssi = status_reg >> 9;
	//printf("rssi %d = %d\r\n", i2c_bus, rssi);
	return rssi;
}

uint16_t store_item = 0;
void ReadLastParamenter()
{//0x5A 0xA5 rx_freq1 rx_freq2 rx_vol1 rx_vol2, 8bytes
	uint16_t i;
	rx_freq1 = 1012;
	rx_freq2 = 1014;

	rx_vol1 = 15;
	rx_vol2 = 0;

	store_item = 0;
	for(i = 0; i < 512; i += 8)
	{
		if(IapReadByte(i) == 0x5A && IapReadByte(i + 1) == 0xA5)
		{//有效记录
			store_item ++;
			rx_freq1 = IapReadByte(i + 2) << 8 | IapReadByte(i + 3);
			rx_freq2 = IapReadByte(i + 4) << 8 | IapReadByte(i + 5);
			rx_vol1 = IapReadByte(i + 6);
			rx_vol2 = IapReadByte(i + 7);
		}
		else
			break;
	}
	if(rx_freq1 > 1080 || rx_freq1 < 870)
		rx_freq1 = 1012;
	if(rx_freq2 > 1080 || rx_freq2 < 870)
		rx_freq2 = 1012;
	rx_vol1 &= 0x0F;
	rx_vol2 &= 0x0F;
	printf("Store_Item %d\r\n", store_item);
}

void SaveParamenter()
{
	if(store_item >= 63 || IapReadByte(store_item * 8) != 0xff)
	{
		IapEraseSector(0); //已满,擦除扇区
		store_item = 0;
		IapProgramByte(store_item * 8, 0x5A);
		IapProgramByte(store_item * 8 + 1, 0xA5);
		IapProgramByte(store_item * 8 + 2, rx_freq1 >> 8);
		IapProgramByte(store_item * 8 + 3, rx_freq1 & 0xff);
		IapProgramByte(store_item * 8 + 4, rx_freq2 >> 8);
		IapProgramByte(store_item * 8 + 5, rx_freq2 & 0xff);
		IapProgramByte(store_item * 8 + 6, rx_vol1);
		IapProgramByte(store_item * 8 + 7, rx_vol2);
	}
	else
	{
		IapProgramByte(store_item * 8, 0x5A);
		IapProgramByte(store_item * 8 + 1, 0xA5);
		IapProgramByte(store_item * 8 + 2, rx_freq1 >> 8);
		IapProgramByte(store_item * 8 + 3, rx_freq1 & 0xff);
		IapProgramByte(store_item * 8 + 4, rx_freq2 >> 8);
		IapProgramByte(store_item * 8 + 5, rx_freq2 & 0xff);
		IapProgramByte(store_item * 8 + 6, rx_vol1);
		IapProgramByte(store_item * 8 + 7, rx_vol2);
	}
	printf("Store Item %d OK\r\n", store_item);
}

uint8_t RDA5820_Init()
{
	SendString("Trying initializing RDA5820s\r\n");
	while(RDA5820_Write(0, 0x02, 0x0002));
	while(RDA5820_Write(1, 0x02, 0x0002));
	Delayms(100);
	while(RDA5820_Write(0, 0x02, 0x8001));
	while(RDA5820_Write(1, 0x02, 0x8001));

	Delayms(100);
	return 0;
}


void main()
{
	uint16_t counterp = 0;
	uint16_t counterm = 0;
	uint16_t oldfreq = 0;
	uint8_t keyp_status = 0;
	uint8_t keym_status = 0;
	int8_t tmpvol;
	uint16_t counter_rssi = 0;

	UartInit();
	Key_Init();
	LCD_Init();//初始化液晶
	ReadLastParamenter();
	LCD_PutString(0, 0, "Initializing");
	LCD_PutString(0, 1, "By Hikari");
	i2c_init();

	RDA5820_Init();
	/* 关闭输出 */

	CheckRDA5820(0);
	SetRDA5820Rx(0);
	SetRDA5820Freq(0, rx_freq1);
	RDA5820_SetVol(0, rx_vol1);


	CheckRDA5820(1);
	SetRDA5820Rx(1);
	SetRDA5820Freq(1, rx_freq2);
	RDA5820_SetVol(1, rx_vol2);

	while(RDA5820_Write(0, 0x02, 0xC001));
	while(RDA5820_Write(1, 0x02, 0xC001));

	DisplayMain();
	while(1)
	{
		counter_rssi ++;
		if(counter_rssi >= 60000)
		{ // 禁噪控制
			counter_rssi = 0;
			rssi1 = RDA5820_GetRSSI(0);
			rssi2 = RDA5820_GetRSSI(1);
			if(rssi1 > POS_THRESHOLD)
			{
				RDA5820_SetVol(0, rx_vol1);
			}
			else if(rssi1 < NEG_THRESHOLD)
			{
				RDA5820_SetVol(0, 0);
			}

			if(rssi2 > POS_THRESHOLD)
			{
				RDA5820_SetVol(1, rx_vol2);
			}
			else if(rssi2 < NEG_THRESHOLD)
			{
				RDA5820_SetVol(1, 0);
			}
			DisplayMain();
		}
		if(!KEY_SEL)
		{
			i2c_delay();
			if(!KEY_SEL)
			{
				while(KEY_SEL == 0);
				state ++;
				if(state > 4)
					state = 0;
				DisplayMain();
			}
		}
		if(state > 0)
		{
			if(!KEY_ENTER) /* 回车键按下 */
			{
				i2c_delay();
				if(!KEY_ENTER)
				{
					if(state == 1) //CH1 freq
					{
						SetRDA5820Freq(0, rx_freq1);
					}
					else if(state == 2)
					{
						RDA5820_SetVol(0, rx_vol1);
					}
					else if(state == 3)
					{
						SetRDA5820Freq(1, rx_freq2);
					}
					else if(state == 4)
					{
						RDA5820_SetVol(1, rx_vol2);
					}
					SaveParamenter();
					state = 0;
				}
			}
#if 1
			//加减按键处理
			if(KEY_PLUS == 0 && keyp_status == 0)
			{
				i2c_delay();
				if(KEY_PLUS == 0)
				{
					keyp_status = 1;/* 转换状态 */
					dbg("Key status 1\r\n");
				}
				else
				{
					keyp_status = 0;
				}
			}
			if(KEY_PLUS == 1)
			{
				if(keyp_status == 1)
				{//短按+
					if(state == 1) //CH1 freq
					{
						rx_freq1 += 2;
						if(rx_freq1 > 1080)
							rx_freq1 = 1080;
					}
					else if(state == 2)
					{
						rx_vol1 += 1;
						if(rx_vol1 > 15)
							rx_vol1 = 15;
					}
					else if(state == 3)
					{
						rx_freq2 += 2;
						if(rx_freq2 > 1080)
							rx_freq2 = 1080;
					}
					else if(state == 4)
					{
						rx_vol2 += 1;
						if(rx_vol2 > 15)
							rx_vol2 = 15;
					}
					DisplayMain();
				}
				keyp_status = 0;
				counterp = 0;
			}
			if(KEY_PLUS == 0 && keyp_status == 1)
			{
				counterp ++;
			}
			if(counterp > 60000 && keyp_status == 1)
			{
				keyp_status = 2;
				dbg("Key status 2\r\n");
				counterp = 0;
			}
			if(keyp_status == 2 && KEY_PLUS == 0)
			{
				counterp ++;
				if(counterp > 30000)
				{//长按+
					counterp = 0;
					if(state == 1) //CH1 freq
					{
						rx_freq1 += 2;
						if(rx_freq1 > 1080)
							rx_freq1 = 1080;
					}
					else if(state == 2)
					{
						rx_vol1 += 1;
						if(rx_vol1 > 15)
							rx_vol1 = 15;
					}
					else if(state == 3)
					{
						rx_freq2 += 2;
						if(rx_freq2 > 1080)
							rx_freq2 = 1080;
					}
					else if(state == 4)
					{
						rx_vol2 += 1;
						if(rx_vol2 > 15)
							rx_vol2 = 15;
					}
					DisplayMain();
				}
			}
//--------------减按键处理
			if(KEY_MINUS == 0 && keym_status == 0)
			{
				i2c_delay();
				if(KEY_MINUS == 0)
				{
					keym_status = 1;/* 转换状态 */
					dbg("Key status 1\r\n");
				}
				else
				{
					keym_status = 0;
				}
			}
			if(KEY_MINUS == 1)
			{
				if(keym_status == 1)
				{//短按-
					if(state == 1) //CH1 freq
					{
						rx_freq1 -= 2;
						if(rx_freq1 < 870)
							rx_freq1 = 870;
					}
					else if(state == 2)
					{
						if(rx_vol1 != 0)
							rx_vol1 -= 1;
					}
					else if(state == 3)
					{
						rx_freq2 -= 2;
						if(rx_freq2 < 870)
							rx_freq2 = 870;
					}
					else if(state == 4)
					{
						if(rx_vol2 != 0)
							rx_vol2 --;
					}
					DisplayMain();
				}
				keym_status = 0;
				counterm = 0;
			}
			if(KEY_MINUS == 0)
			{
				counterm ++;
			}
			if(counterm > 60000)
			{
				keym_status = 2;
				dbg("Key status 2\r\n");
				counterm = 0;
			}
			if(keym_status == 2 && KEY_MINUS == 0)
			{// 长按 -
				counterm ++;
				if(counterm > 30000)
				{
					counterm = 0;
					if(state == 1) //CH1 freq
					{
						rx_freq1 -= 2;
						if(rx_freq1 < 870)
							rx_freq1 = 870;
					}
					else if(state == 2)
					{
						if(rx_vol1 != 0)
							rx_vol1 -= 1;
					}
					else if(state == 3)
					{
						rx_freq2 -= 2;
						if(rx_freq2 < 870)
							rx_freq2 = 870;
					}
					else if(state == 4)
					{
						if(rx_vol2 != 0)
							rx_vol2 --;
					}
					DisplayMain();
				}
			}
#endif
		}
	}
}

