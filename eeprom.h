/* 
 * File:   eeprom.h
 * Author: DELL
 *
 * Created on 30 maja 2022, 20:03
 */

#ifndef EEPROM_H
#define	EEPROM_H

#ifdef	__cplusplus
extern "C" {
#endif

void EEPROM_Write (int address, char data);
char EEPROM_Read(int address);


#ifdef	__cplusplus
}
#endif

#endif	/* EEPROM_H */

