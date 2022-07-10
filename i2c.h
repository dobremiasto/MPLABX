/* 
 * File:   i2c.h
 * Author: DELL
 *
 * Created on 20 kwietnia 2022, 23:41
 */

#ifndef I2C_H
#define	I2C_H

#ifdef	__cplusplus
extern "C" {
#endif

void I2C_Master_Init(unsigned long c);
void PCF8574_WRITE(unsigned int value);
void RDA_WRITE(char reg, unsigned int value);
void I2C_WriteInt(char addr,char reg, unsigned int value);


#ifdef	__cplusplus
}
#endif

#endif	/* I2C_H */

