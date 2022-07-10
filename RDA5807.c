#include <xc.h>
#include <stdlib.h>

#include "config.h"
#include "i2c.h"

void RDA_WRITE(char reg, unsigned int value) {
    I2C_WriteInt(0x22,reg,value);
    //I2C_Start(I2C_LCD);
    //I2C_Write(I2C_LCD, i2c_addr);
    //I2C_Write(I2C_LCD, value | backlight_val);
    //I2C_Stop(I2C_LCD);
}


