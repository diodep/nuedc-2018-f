/********************************** (C) COPYRIGHT *******************************
* File Name		: I2C.C
* Author		: Kongou Hikari
* License		: MIT
* Version		: V1.0
* Date			: 2018/03/17
* Description		: 8051 I2C
*******************************************************************************/
#include <8051.h>
#include <stdint.h>
#include "i2c.h"

#define I2C_SDAT	P3_6
#define I2C_SCLK	P3_7

#define I2C1_SDAT	P3_4
#define I2C1_SCLK	P3_5


void i2c_init()
{ /* GPIO port initial */
	I2C_SDAT = 1;
	I2C_SCLK = 1;
}

void i2c_delay()
{
	volatile char i = 1;
	while(i--);
}

void i2c_start()
{
	I2C_SDAT = 1;
	I2C_SCLK = 1;
	i2c_delay();

	I2C_SDAT = 0;
	i2c_delay();

	I2C_SCLK = 0;
	i2c_delay();
}

void i2c_stop()
{
	I2C_SDAT = 0;
	I2C_SCLK = 1;
	i2c_delay();

	I2C_SDAT = 1;
	i2c_delay();
}

void i2c_write(uint8_t data)
{
	int i;

	for(i = 0; i < 8; i++)
	{
		data <<= 1;
		I2C_SDAT = CY;

		I2C_SCLK = 1;
		i2c_delay();

		I2C_SCLK = 0;
		i2c_delay();
	}
}

uint8_t i2c_read()
{
	int i;
	uint8_t ret = 0;

	I2C_SDAT = 1;
	for(i = 0; i < 8; i++)
	{
		ret <<= 1;
		I2C_SCLK = 1;
		i2c_delay();

		if(I2C_SDAT)
			ret |= 0x01;

		I2C_SCLK = 0;
		i2c_delay();

	}
	return ret;
}

bool i2c_read_ack()
{
	bool status;

	I2C_SDAT = 1;

	I2C_SCLK = 1;
	i2c_delay();

	status = I2C_SDAT;

	I2C_SCLK = 0;
	i2c_delay();

	return !status;
}

bool i2c_read_nak()
{
	return !i2c_read_ack();
}

void i2c_send_ack()
{
	I2C_SDAT = 0;

	I2C_SCLK = 1;
	i2c_delay();

	I2C_SCLK = 0;
	i2c_delay();

}


void i2c1_init()
{ /* GPIO port initial */
	I2C1_SDAT = 1;
	I2C1_SCLK = 1;
}

void i2c1_start()
{
	I2C1_SDAT = 1;
	I2C1_SCLK = 1;
	i2c_delay();

	I2C1_SDAT = 0;
	i2c_delay();

	I2C1_SCLK = 0;
	i2c_delay();
}

void i2c1_stop()
{
	I2C1_SDAT = 0;
	I2C1_SCLK = 1;
	i2c_delay();

	I2C1_SDAT = 1;
	i2c_delay();
}

void i2c1_write(uint8_t data)
{
	int i;

	for(i = 0; i < 8; i++)
	{
		data <<= 1;
		I2C1_SDAT = CY;

		I2C1_SCLK = 1;
		i2c_delay();

		I2C1_SCLK = 0;
		i2c_delay();
	}
}

uint8_t i2c1_read()
{
	int i;
	uint8_t ret = 0;

	I2C1_SDAT = 1;
	for(i = 0; i < 8; i++)
	{
		ret <<= 1;
		I2C1_SCLK = 1;
		i2c_delay();

		if(I2C1_SDAT)
			ret |= 0x01;

		I2C1_SCLK = 0;
		i2c_delay();

	}
	return ret;
}

bool i2c1_read_ack()
{
	bool status;

	I2C1_SDAT = 1;

	I2C1_SCLK = 1;
	i2c_delay();

	status = I2C1_SDAT;

	I2C1_SCLK = 0;
	i2c_delay();

	return !status;
}

bool i2c1_read_nak()
{
	return !i2c1_read_ack();
}

void i2c1_send_ack()
{
	I2C1_SDAT = 0;

	I2C1_SCLK = 1;
	i2c_delay();

	I2C1_SCLK = 0;
	i2c_delay();

}

