/********************************** (C) COPYRIGHT *******************************
* File Name		: MAIN.C
* Author		: Kongou Hikari
* License		: MIT
* Version		: V1.0
* Date			: 2018/07/21
* Description		: 2018电赛F题发射机程序
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

uint16_t tx_freq;
uint8_t state = 0;
uint8_t store_item = 0;

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
	i2c_bus = i2c_bus;
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
	return 0;
}
uint8_t rda_read_err;
uint16_t RDA5820_Read(uint8_t i2c_bus, uint8_t reg_addr)
{
	uint8_t hi8, lo8;
	rda_read_err = 0;
	i2c_bus = i2c_bus;
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
	return (hi8 << 8) | lo8;
}

uint8_t RDA5820_Init(uint8_t i2c_bus)
{

	SendString("Trying initializing RDA5820\r\n");
	Delayms(1);
	while(RDA5820_Write(i2c_bus, 0x02, 0x0002));
	Delayms(50);
	while(RDA5820_Write(i2c_bus, 0x02, 0xC001));
	Delayms(400);
	return 0;
}

void CheckRDA5820(uint8_t i2c_bus)
{
	uint16_t chipid;
reread:
	chipid = RDA5820_Read(i2c_bus, 0x00);
	if(rda_read_err) goto reread;

	if(chipid == 0x5820)
	{
		//SendString("This chip is RDA5820NS\r\n");
	}
	else if(chipid == 0x5805)
	{
		//SendString("This chip is RDA5820\r\n");
	}
	else
	{
		//while(RDA5820_Write(i2c_bus, 0x02, 0xC001));
		//Delayms(200);
		SendString("Unknown chip!\r\n");
	}
}

void SetRDA5820Freq(uint8_t i2c_bus, uint16_t freq) //870~1080
{
    uint16_t freq_set = (freq - 870) / 2;
    uint16_t reg_read;
    uint16_t reg_set = 0x11 | (freq_set << 6); /* channel spacing = 200khz, tune = 1, 87-108M*/
reset:
	//while(RDA5820_Write(i2c_bus, 0x0a, 0x0000));
    while(RDA5820_Write(i2c_bus, 0x03, reg_set));

    Delayms(80);
reread:
    reg_read = RDA5820_Read(i2c_bus, 0x0a);
    if(rda_read_err) goto reread;

    //RDA5820_Read(i2c_bus, 0x0b);
    if(reg_read & ( 1<<14 ))
		dbg("Set freq successful\r\n");
	else
	{
		dbg("Wait set\r\n");
		goto reread;
	}
}

void SetRDA5820Tx(uint8_t i2c_bus)
{
	//uint16_t audio_pga;
	while(RDA5820_Write(i2c_bus, 0x40, 0x0001)); //发射模式
    while(RDA5820_Write(i2c_bus, 0x41, 0x3F | (0x7 << 6))); //PA_GAIN=0x3F, BIAS=0x03
    //audio_pga = RDA5820_Read(i2c_bus, 0x68);
    //printf("Audio PGA = %04x\r\n", audio_pga);
    while(RDA5820_Write(i2c_bus, 0x68, 0xf0 | (0x02 << 10) | (0x01 << 8))); //设定音量增益
    dbg("Set RDA5820 to TX mode\r\n");
    Delayms(20);
}

void SetRDA5820Rx(uint8_t i2c_bus)
{
	while(RDA5820_Write(i2c_bus, 0x40, 0x0000)); //接收模式
	while(RDA5820_Write(i2c_bus, 0x05, (0x03 << 6)));
	Delayms(20);
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

void DisplayMain()
{
	if(state == 1)
		LCD_OnCur();
	else
		LCD_OffCur();
	LCD_Cls();
	LCD_PutString(0, 0, "Tx Freq:");
	if(tx_freq >= 1000)
		LCD_WriteData(tx_freq / 1000 + '0');
	LCD_WriteData(tx_freq % 1000 / 100 + '0');
	LCD_WriteData(tx_freq % 100 / 10 + '0');
	LCD_WriteData('.');
	LCD_WriteData(tx_freq % 10 + '0');
	LCD_Puts("MHz");
	if(tx_freq >= 1000)
		LCD_SetCur(12, 0);
	else
		LCD_SetCur(11, 0); //光标到点
}

uint8_t CheckStation(uint8_t i2c_bus, uint16_t freq)
{
	uint16_t status_reg;
	uint8_t rssi;
	SetRDA5820Rx(i2c_bus); //将RDA5820切换到接收模式
	SetRDA5820Freq(i2c_bus, freq); //设置频率
	Delayms(20);
reread:
	status_reg = RDA5820_Read(i2c_bus, 0x0b); //读状态寄存器
	if(rda_read_err) goto reread;

	rssi = status_reg >> 9;
	printf("RSSI value = %d\r\n", rssi);
    if(status_reg & (1<<8) || rssi > 50)
	{//是个台
		dbg("Is Station!\r\n");
		return 1;
	}
	dbg("No station!\r\n");
	return 0;
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

void ReadLastFrequency()
{ //存储格式:0x5A 0xA5 Hi Low
	uint16_t i;
	tx_freq = 1012; //101.2MHz
	store_item = 0;
	for(i = 0; i < 512; i += 4)
	{
		if(IapReadByte(i) == 0x5A && IapReadByte(i + 1) == 0xA5)
		{//有效记录
			store_item ++;
			tx_freq = IapReadByte(i + 2) << 8 | IapReadByte(i + 3);
		}
		else
			break;
	}
	if(tx_freq > 1080 || tx_freq < 870)
		tx_freq = 1012;
	printf("SI%d\r\n", store_item);
}


void SaveFrequency()
{//保存频率
	if(store_item >= 127 || IapReadByte(store_item * 4) != 0xff)
	{
		IapEraseSector(0); //已满,擦除扇区
		store_item = 0;
		IapProgramByte(store_item * 4, 0x5A);
		IapProgramByte(store_item * 4 + 1, 0xA5);
		IapProgramByte(store_item * 4 + 2, tx_freq >> 8);
		IapProgramByte(store_item * 4 + 3, tx_freq & 0xff);
	}
	else
	{
		IapProgramByte(store_item * 4, 0x5A);
		IapProgramByte(store_item * 4 + 1, 0xA5);
		IapProgramByte(store_item * 4 + 2, tx_freq >> 8);
		IapProgramByte(store_item * 4 + 3, tx_freq & 0xff);
	}
	dbg("SFOK\r\n");
}

void JumpToFreeFreq()
{
	uint16_t oldfreq = tx_freq;
	LCD_PutString(0, 1, "Seeking conflict");
	while(1)
	{
		if(CheckStation(0, tx_freq))
		{
			tx_freq += 2; //+200kHz
			if(tx_freq > 1080)
				tx_freq = 880; //88M
			//dbg("Freq is not free yet\r\n");
		}
        else
			break;
	}
	if(oldfreq != tx_freq)
	{
		SaveFrequency();
		LCD_Cls();
		LCD_PutString(0, 1, "Conflict solved");
	}
}

void main()
{
	uint16_t counterp = 0;
	uint16_t counterm = 0;
	uint16_t oldfreq = 0;
	uint8_t keyp_status = 0;
	uint8_t keym_status = 0;

	UartInit();
	Key_Init();
	LCD_Init();//初始化液晶
	LCD_PutString(0, 0, "Initializing");
	//LCD_PutString(0, 1, "      System");
	i2c_init();
	ReadLastFrequency();
	RDA5820_Init(0);

	CheckRDA5820(0);
	JumpToFreeFreq();

	SetRDA5820Tx(0);
	SetRDA5820Freq(0, tx_freq);

	DisplayMain();

    while(1)
	{
        if(!KEY_SEL)
		{
			i2c_delay();
			if(!KEY_SEL)
			{
				oldfreq = tx_freq;
				state = 1;//设定频率模式
				DisplayMain();
			}
		}
		if(state == 1)
		{
//回车按钮处理
			if(!KEY_ENTER)
			{
				i2c_delay();
				if(!KEY_ENTER)
				{
					if(CheckStation(0, tx_freq))
					{//有台
						tx_freq = oldfreq;
						DisplayMain();
						LCD_PutString(0, 1, "Conflict Freq!");
					}
					else
					{
						SetRDA5820Tx(0); //重新设回发射模式
						SetRDA5820Freq(0, tx_freq);
						SaveFrequency();
						state = 0;
						DisplayMain();
					}
				}
			}
//加按键处理
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
				{//短按
					tx_freq += 2;
					if(tx_freq > 1080)
						tx_freq = 1080;
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
				{
					counterp = 0;
					tx_freq += 2;
					if(tx_freq > 1080)
						tx_freq = 1080; // 长按
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
				{//短按
					tx_freq -= 2;
					if(tx_freq < 870)
						tx_freq = 870;
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
			{
				counterm ++;
				if(counterm > 30000)
				{
					counterm = 0;
					tx_freq -= 2;
					if(tx_freq < 870)
						tx_freq = 870; // 长按
					DisplayMain();
				}
			}
		}
	}

}

