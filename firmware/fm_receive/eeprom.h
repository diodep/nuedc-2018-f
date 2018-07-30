/*Declare SFR associated with the IAP */
__sfr __at 0xc2 IAP_DATA  ;           //Flash data register
__sfr __at 0xc3 IAP_ADDRH ;           //Flash address HIGH
__sfr __at 0xc4 IAP_ADDRL ;           //Flash address LOW
__sfr __at 0xc5 IAP_CMD   ;           //Flash command register
__sfr __at 0xc6 IAP_TRIG  ;           //Flash command trigger
__sfr __at 0xc7 IAP_CONTR ;           //Flash control register

/*Define ISP/IAP/EEPROM command*/
#define CMD_IDLE    0               //Stand-By
#define CMD_READ    1               //Byte-Read
#define CMD_PROGRAM 2               //Byte-Program
#define CMD_ERASE   3               //Sector-Erase

/*Define ISP/IAP/EEPROM operation const for IAP_CONTR*/
//#define ENABLE_IAP 0x80           //if SYSCLK<30MHz
//#define ENABLE_IAP 0x81           //if SYSCLK<24MHz
//#define ENABLE_IAP  0x82            //if SYSCLK<20MHz
#define ENABLE_IAP 0x83           //if SYSCLK<12MHz
//#define ENABLE_IAP 0x84           //if SYSCLK<6MHz
//#define ENABLE_IAP 0x85           //if SYSCLK<3MHz
//#define ENABLE_IAP 0x86           //if SYSCLK<2MHz
//#define ENABLE_IAP 0x87           //if SYSCLK<1MHz

//Start address for STC12C5A60S2 EEPROM
#define IAP_ADDRESS 0x0000


extern void Delay(uint8_t n);
extern void IapIdle();
extern uint8_t IapReadByte(uint16_t addr);
extern void IapProgramByte(uint16_t addr, uint8_t dat);
extern void IapEraseSector(uint16_t addr);

#define _nop_() __asm nop __endasm
