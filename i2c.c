#include <xc.h>
#include <stdlib.h>

#include "config.h"


void I2C_Master_Init(unsigned long c) {
    TRISCbits.TRISC3 = 1;
    TRISCbits.TRISC4 = 1;
    SSPSTAT = 0x80;
    SSPCON1 = 0x28; //SSP Module as Master
    SSPCON2 = 0;
    SSPADD = (_XTAL_FREQ / (4 * c)) - 1; //Setting Clock Speed
    PIE1bits.SSPIE = 1;
    PIR1bits.SSPIF = 0;
}

void I2C_Ready() {
    while (PIR2bits.BCLIF); /* Wait if bit collision interrupt flag is set*/

    /* Wait for Buffer full and read write flag*/
    while (SSPSTATbits.BF || (SSPSTATbits.R_NOT_W));
    PIR1bits.SSPIF = 0; /* Clear SSPIF interrupt flag*/
}

char I2C_Write(unsigned char data) {
    SSPBUF = data; /* Write data to SSPBUF*/
    I2C_Ready();
    if (SSPCON2bits.ACKSTAT) /* Check for acknowledge bit*/
        return 1;
    else
        return 2;
}

char I2C_Start(char slave_write_address) {
    SSPCON2bits.SEN = 1; /* Send start pulse */
    while (SSPCON2bits.SEN); /* Wait for completion of start pulse */
    PIR1bits.SSPIF = 0;
    if (!SSPSTATbits.S) /* Check whether START detected last */
        return 0; /* Return 0 to indicate start failed */
    return (I2C_Write(slave_write_address)); /* Write slave device address
						with write to communicate */
}

char I2C_Stop() {
    I2C_Ready();
    SSPCON2bits.PEN = 1; /* Stop communication*/
    while (SSPCON2bits.PEN == 1); /* Wait for end of stop pulse*/
    PIR1bits.SSPIF = 0;
    if (!SSPSTATbits.P) /* Check whether STOP is detected last */
        return 0; /* If not return 0 to indicate start failed*/
}

void I2C_Ack() {
    SSPCON2bits.ACKDT = 0; /* Acknowledge data 1:NACK,0:ACK */
    SSPCON2bits.ACKEN = 1; /* Enable ACK to send */
    while (SSPCON2bits.ACKEN);
}

void I2C_Nack() {
    SSPCON2bits.ACKDT = 1; /* Acknowledge data 1:NACK,0:ACK */
    SSPCON2bits.ACKEN = 1; /* Enable ACK to send */
    while (SSPCON2bits.ACKEN);
}

char I2C_Read(char flag) {
    int buffer = 0;
    SSPCON2bits.RCEN = 1; /* Enable receive */

    /* Wait for buffer full flag which when complete byte received */
    while (!SSPSTATbits.BF);
    buffer = SSPBUF; /* Copy SSPBUF to buffer */

    /* Send acknowledgment or negative acknowledgment after read to 
    continue or stop reading */
    if (flag == 0)
        I2C_Ack();
    else
        I2C_Nack();
    I2C_Ready();
    return (buffer);
}

void PCF8574_WRITE(unsigned int value) {
    I2C_Start(0x7E);
    I2C_Write(value);// | backlight_val);
    I2C_Stop();
    //I2C_Start(I2C_LCD);
    //I2C_Write(I2C_LCD, i2c_addr);
    //I2C_Write(I2C_LCD, value | backlight_val);
    //I2C_Stop(I2C_LCD);
}

void I2C_WriteInt(char addr,char reg, unsigned int value) {
    I2C_Start(addr);
    I2C_Write(reg);
    I2C_Write(value>>8);
    I2C_Write(value&0xff);
    I2C_Stop();
    //I2C_Start(I2C_LCD);
    //I2C_Write(I2C_LCD, i2c_addr);
    //I2C_Write(I2C_LCD, value | backlight_val);
    //I2C_Stop(I2C_LCD);
}