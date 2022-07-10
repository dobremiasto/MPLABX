
#include <xc.h>
#include <stdlib.h>


#include "config.h"
#include "i2c.h"
#include "RDA5807.h"
#include "can.h"
#include "eeprom.h"




#define LCD_BACKLIGHT          0x08
#define LCD_NOBACKLIGHT        0x00
#define LCD_FIRST_ROW          0x80
#define LCD_SECOND_ROW         0xC0
#define LCD_THIRD_ROW          0x94
#define LCD_FOURTH_ROW         0xD4
#define LCD_CLEAR              0x01
#define LCD_RETURN_HOME        0x02
#define LCD_ENTRY_MODE_SET     0x04
#define LCD_CURSOR_OFF         0x0C
#define LCD_UNDERLINE_ON       0x0E
#define LCD_BLINK_CURSOR_ON    0x0F
#define LCD_MOVE_CURSOR_LEFT   0x10
#define LCD_MOVE_CURSOR_RIGHT  0x14
#define LCD_TURN_ON            0x0C
#define LCD_TURN_OFF           0x08
#define LCD_SHIFT_LEFT         0x18
#define LCD_SHIFT_RIGHT        0x1E

#ifndef LCD_TYPE
#define LCD_TYPE 2           // 0=5x7, 1=5x10, 2=2 lines
#endif

#define FREQ	873 //947 = 94.7MHz
#define frq		((FREQ-870)<<6)
#define frqh	(frq>>8)
#define frql	((frq & 0x00FF) | 0b00010000)






#pragma config WDT = OFF

volatile unsigned int i2c_addr, cpm = 0, backlight_val = LCD_BACKLIGHT;
volatile unsigned char pomiary[60], lcd_update = 0, znaki = 0, frequency, frequency_2, frequency_3;
volatile unsigned int uSv1, uSv2;
volatile unsigned int enc_dir, enc_cnt, enc_last = 2;

int RS;

struct {
    int status;
    unsigned int buff[16];
    char signal;
    char stereo;
    char rds;
    char mute;
} radio;

void delay(void) {
    unsigned int i;

    for (i = 0; i < 20000; i++)
        ;
}

void LED_R_ON(void) {
    TRISCbits.TRISC5 = 0; // make RC5 output
    LATCbits.LATC5 = 0;
    //__del
}

void LED_R_OFF(void) {
    TRISCbits.TRISC5 = 0; // make RC5 output
    LATCbits.LATC5 = 1;
}

void LED_G_ON(void) {
    TRISCbits.TRISC1 = 0; // make RC1 output
    LATCbits.LATC1 = 0;
}

void LED_G_OFF(void) {
    TRISCbits.TRISC1 = 0; // make RC1 output
    LATCbits.LATC1 = 1;
}

void BUZZER_ON(void) {
    TRISAbits.TRISA6 = 0; // make RA6 output
    LATAbits.LATA6 = 0;
}

void BUZZER_OFF(void) {
    TRISAbits.TRISA6 = 0; // make RA6 output
    LATAbits.LATA6 = 1;
}

void Backlight() {
    backlight_val = LCD_BACKLIGHT;
    PCF8574_WRITE(0);
}

void noBacklight() {
    backlight_val = LCD_NOBACKLIGHT;
    PCF8574_WRITE(0);
}

void LCD_Write_Nibble(unsigned int n) {
    n |= RS;
    PCF8574_WRITE(n);
    PCF8574_WRITE(n | 0x04);
    __delay_us(1);
    PCF8574_WRITE(n & 0xFB);
    __delay_us(50);
}

void LCD_Cmd(unsigned int Command) {
    RS = 0;
    LCD_Write_Nibble(Command & 0xF0);
    LCD_Write_Nibble((Command << 4) & 0xF0);
}

void LCD_Begin(unsigned int _i2c_addr) {
    i2c_addr = _i2c_addr;
    PCF8574_WRITE(0);
    __delay_ms(40);
    //Delay10KTCYx(16);
    LCD_Cmd(3);
    __delay_ms(5);
    //Delay10KTCYx(2);
    LCD_Cmd(3);
    __delay_ms(5);
    //Delay10KTCYx(2);
    LCD_Cmd(3);
    __delay_ms(5);
    //Delay10KTCYx(2);
    LCD_Cmd(LCD_RETURN_HOME);
    __delay_ms(5);
    //Delay10KTCYx(2);
    LCD_Cmd(0x20 | (LCD_TYPE << 2));
    __delay_ms(50);
    //Delay10KTCYx(20);
    LCD_Cmd(LCD_TURN_ON);
    __delay_ms(50);
    //Delay10KTCYx(20);
    LCD_Cmd(LCD_CLEAR);
    __delay_ms(50);
    //Delay10KTCYx(20);  
    LCD_Cmd(LCD_ENTRY_MODE_SET | LCD_RETURN_HOME);
    __delay_ms(50);
    //Delay10KTCYx(20);
}

void LCD_Out(unsigned char LCD_Char) {
    RS = 1;
    LCD_Write_Nibble(LCD_Char & 0xF0);
    LCD_Write_Nibble((LCD_Char << 4) & 0xF0);
}

void LCD_Goto(unsigned int col, unsigned int row) {
    switch (row) {
        case 2:
            LCD_Cmd(0xC0 + col - 1);
            break;
        case 3:
            LCD_Cmd(0x94 + col - 1);
            break;
        case 4:
            LCD_Cmd(0xD4 + col - 1);
            break;
        default: // case 1:
            LCD_Cmd(0x80 + col - 1);
    }
}

void LCD_WriteText(unsigned char * text) {
    while (*text)
        LCD_Out((*text++));
}


// this routine found online somewhere, then tweaked
// returns pointer to ASCII string in a static buffer

char *itoa(int value) {
    static char buffer[12]; // 12 bytes is big enough for an INT32
    int original = value; // save original value

    int c = sizeof (buffer) - 1;

    buffer[c] = 0; // write trailing null in last byte of buffer    

    if (value < 0) // if it's negative, note that and take the absolute value
        value = -value;

    do // write least significant digit of value that's left
    {
        buffer[--c] = (value % 10) + '0';
        value /= 10;
    } while (value);

    if (original < 0)
        buffer[--c] = '-';

    return &buffer[c];
}

unsigned char g2b(unsigned char gray) {
    unsigned char temp1;
    unsigned char temp2;
    unsigned char temp3;

    temp1 = (gray ^= (gray >> 4));
    temp2 = (temp1 ^= (temp1 >> 2));
    temp3 = (temp2 ^= (temp2 >> 1));

    return temp3;
}

void main(void) {

    //   unsigned char buforLCD_1[17] = "NOWY Timer 29s  ";
    //unsigned char buforLCD_2[17] = "BogArt PROTOTYPE";
    unsigned int licznik = 1, licznik_2 = 0;
    //OSCCONbits.IRCF0=3;			//internal 8mhz
    OSCCON = 0b01110000;
    //OSCTUNEbits.PLLEN = 1; // Turn on PLL 8*4=32MHz

    // InitECAN();

    //EEPROM_Write(0x0001,83);



    I2C_Master_Init(100000);



    __delay_ms(1000);
    RDA_WRITE(0x02, 0b1100001010001101);
    __delay_ms(100);

    //frg=870+eeprom_read
    //((FREQ-870)<<6)
    frequency = EEPROM_Read(0x0f);
    frequency_2 = frequency;
    enc_cnt = 870 + frequency;
    // = (frequency + 870);

    RDA_WRITE(0x03, ((enc_cnt - 870) << 6) | 0b00010000);
    __delay_ms(100);
    RDA_WRITE(0x04, 0b0000100000000000);
    __delay_ms(100);
    RDA_WRITE(0x05, 0b1000001011110010); //
    //while(1);

    Backlight();
    LCD_Begin(0x7E);
    LCD_Goto(1, 1);
    LCD_WriteText("Radio FM");
    __delaywdt_ms(1000);
    //LCD_CHAR_u_micro
    LCD_Cmd(0x40);
    LCD_Out(0x00);
    LCD_Out(0x11);
    LCD_Out(0x11);
    LCD_Out(0x11);
    LCD_Out(0x13);
    LCD_Out(0x1d);
    LCD_Out(0x10);
    LCD_Out(0x10);
    //    LCD_Goto(1, 1); // Go to column 2 row 1
    //  buforLCD_1="BOGARTBOGART    ";
    //  LCD_WriteText(buforLCD_1);
    //LCD_Goto(1, 2);
    //LCD_WriteText(buforLCD_2);
    //LCD_Out('Q');

    /* Make all bits on the Port B (LEDs) output bits.
     * If bit is cleared, then the bit is an output bit.
     */
    // TRISC = 0;
    //  TRISA = 0;

    //USART init

    //baud 9600
    RCSTAbits.SPEN = 1;
    TXSTAbits.TXEN = 1;
    TXSTAbits.BRGH = 1;
    SPBRG = 51;
    TRISCbits.TRISC7 = 1;
    TRISCbits.TRISC6 = 0;

    TRISAbits.TRISA4 = 1; // make RA4 input

    //T0CKI-RA4-TIMER0
    //T0CON = 0b10111000;
    T0CON = 0b10000100; //T0 ON, /32
    //TMR0L = 0x00;
    //TMR0H = 0xFF;

    //TIMER1
    //////   T1CON = 0b10110101;
    //T1CON = 0b00000011;
    //T1CONbits.TMR1CS=1;//external input clock
    //T1CONbits.TMR1ON=1;//timer on
    //T1CON
    TMR1L = 0x00;
    TMR1H = 0x00;
    //PIR1bits.TMR1IF = 0;
    //T2CON=

    //enkoder
    //TRISB4_bit=1;//
    ADCON1bits.PCFG = 0b1111;
    TRISBbits.RB4 = 1; //
    TRISBbits.RB5 = 1;

    INTCONbits.RBIE = 1;
    INTCON2bits.RBIP = 1; //high priority
    //   PIE1bits.TMR1IE = 1;
    INTCONbits.TMR0IE = 1;
    //   IPR1bits.TMR1IP=0;
    INTCONbits.GIE = 1;
    //INTCONbits.PEIE=1;
    //   RCONbits.IPEN=1;


    //if (PORTAbits.RA4); // test pin if is 1

    LCD_Goto(1, 1);
    LCD_Out(0x00);
    LCD_WriteText("Sv:        ");
    LCD_Goto(1, 2);
    LCD_WriteText("CPM:");



    RB3 = 1;

    // InitECAN();
    lcd_update == 1;
    while (1) {
        //LED_R_ON();


        TXREG = 0x55;
        //delay();
        __delaywdt_ms(5);
        BUZZER_ON();
        //LED_R_OFF();
        //LED_G_ON();
        //BUZZER_OFF();
        //TXREG = 0xAA;
        //delay();
        // __delaywdt_ms(10);
        /*if (licznik == 512) {
            LCD_Goto(1, 1);
            LCD_WriteText("   ");
            licznik = 0;
        }*/
        //LCD_Goto(1, 1);
        //LCD_WriteInt(buffer,2);
        //LCD_WriteText(itoa(TMR0L));
        licznik = TMR1H;
        licznik = licznik << 8;
        licznik = licznik + TMR1L;
        if (licznik != licznik_2) {
            //   LCD_WriteText(itoa(licznik));
            //licznik=TMR0L;
            licznik_2 = licznik;
            if (licznik_2 > 0) {
                BUZZER_OFF();
                //    LED_R_ON();
            }

            //LED_G_OFF();

        }

        if (lcd_update == 1) {



            //LCD_Goto(5, 1);
            //LCD_WriteText("     ");
            //cpm=cpm*2;
            uSv1 = (cpm * 0.00664); ///1000000;
            uSv2 = (cpm * 0.6664); ///1000)%100;
            uSv2 = uSv2 - (uSv1 * 100);
            //LCD_Goto(5, 1);
            // LCD_WriteText(itoa(uSv1));
            // LCD_WriteText(",");
            if (uSv2 < 10) {
                //    LCD_WriteText("0");


            }

            //LCD_WriteText(itoa(znaki));

            /*
            LCD_WriteText(itoa(uSv2));
            LCD_WriteText(" ");
            //LCD_Goto(5, 2);
            //LCD_WriteText("     ");
            LCD_Goto(5, 2);
            LCD_WriteText(itoa(cpm));
            LCD_WriteText("  ");
            //LCD_Out(znaki);
             */

            // LCD_Goto(1,1);
            //LCD_WriteText("step:");
            //LCD_WriteText(itoa(enc_dir));

            // RDA_WRITE(0x05, 0b1000001011110000+enc_cnt); //
            frequency = (enc_cnt - 870);

            RDA_WRITE(0x03, ((enc_cnt - 870) << 6) | 0b00010000);
            LCD_Goto(1, 2); //x,y
            LCD_WriteText(("cnt: "));
            LCD_WriteText(itoa(enc_cnt));
            LCD_WriteText(("   "));
            //frequency


            lcd_update = 0;
        }
    }
}

void __interrupt(high_priority) przerwanie(void) {
    // if (PIR1bits.TMR1IF) {

    if (INTCONbits.RBIF == 1) {

        enc_dir = PORTB;

        enc_dir = PORTB >> 4;
        enc_dir &= 0b00000011;
        enc_dir = g2b(enc_dir);


        if (enc_dir == 2) {
            if (enc_last <= 1)enc_cnt++;
            if (enc_last > 2)enc_cnt--;
            lcd_update = 1;
        }
        enc_last = enc_dir;
        //enc_cnt;

        //LED_G_ON();

        //__delay_ms(5);

        // LED_G_OFF();

        __delay_ms(2);
        INTCONbits.RBIF = 0;
    }

    if (INTCONbits.TMR0IF == 1) {
        TMR0L = 0xDB;
        TMR0H = 0x0B;
        INTCONbits.TMR0IF = 0;
        //static 
        static unsigned int numer_pomiaru;
        //licznik_1s++;
        //if (licznik_1s > 4) {
        //   licznik_1s = 0;
        if (numer_pomiaru > 59) {
            numer_pomiaru = 0;
        }
        pomiary[numer_pomiaru] = TMR1L;
        numer_pomiaru++;
        cpm = 0;
        for (int i = 0; i < 60; i++) {
            cpm = cpm + pomiary[i];

        }
        // cpm=cpm*6;

        TMR1L = 0;
        //lcd_update = 1;
        znaki++;

        //ECAN_Transmit();

        //    LED_G_ON;
        LED_R_ON();
        __delay_ms(10);
        LED_R_OFF();


        // }


        //PIR1bits.TMR1IF = 0;
        //EEPROM_Read(0x0001);
        if (frequency_3 == frequency) {
            static char i2;
            i2++;
            if ((frequency != frequency_2)&&(i2 > 3)) {
                static char i;
                
                i++;
                if (i >= 5) {
                    EEPROM_Write(0x0f, (frequency));
                    LED_G_ON();
                    BUZZER_OFF();
                    __delay_ms(50);
                    BUZZER_ON();
                    LED_G_OFF();
                    i = 0;
                    i2 = 0;
                    frequency_2 = frequency;
                }
            }

        } else {
            frequency_3 = frequency;
            //i2 = 0;
        }




    }


}


