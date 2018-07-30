/********************************** (C) COPYRIGHT *******************************
* File Name		: I2C.H
* Author		: Kongou Hikari
* License		: MIT
* Version		: V1.0
* Date			: 2018/03/17
* Description		: 8051 I2C
*******************************************************************************/
#ifndef _I2C_H_

#define _I2C_H_

typedef __bit bool;

extern void i2c_init();

extern void i2c_start();

extern void i2c_stop();

extern void i2c_write(uint8_t data);

extern bool i2c_read_ack();

extern bool i2c_read_nak();

extern uint8_t i2c_read();

extern void i2c_send_ack();


extern void i2c1_init();

extern void i2c1_start();

extern void i2c1_stop();

extern void i2c1_write(uint8_t data);

extern bool i2c1_read_ack();

extern bool i2c1_read_nak();

extern uint8_t i2c1_read();

extern void i2c1_send_ack();


extern void i2c_delay();

#define TW_READ		0x01

#endif
