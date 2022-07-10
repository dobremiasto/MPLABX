/**********************************************************************
* 2010 Microchip Technology Inc.
*
* FileName:        ECAN.c
* Dependencies:    ECAN (.h) & other files if applicable, see below
* Processor:       PIC18F66K80 family
* Linker:          MPLINK 4.37+
* Compiler:        C18 3.36+
*
* 
* REVISION HISTORY:
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Author        Date      	Comments on this revision
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Manning C.    12/1/2010	First release of source file
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Titus, Jon    5/17/2016	Comments added and text cleaned up
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* ADDITIONAL NOTES:
* Code Tested on: PIC18F26K80 (PIC18F66K80 family) controller (05/17/2016) 
* 
* DESCRIPTION:
* In this example, CPU is starts to run from external secondary oscillator
*  and then clock switching lets it run from internal FRC.
*********************************************************************/

/*********************************************************************
*
*                            Includes 
*
*********************************************************************/
#include <p18cxxx.h>
#include "can.h"

/*********************************************************************
*
*                             Definitions 
*
*********************************************************************/
// ECAN bitrate define, choose only ONE rate
#define F_ECAN_100    0       // 1 sets ECAN module for 100kbps
#define F_ECAN_125    0       // 1 sets ECAN module for 125kbps
#define F_ECAN_500    0       // 1 sets ECAN module for 500kbps
#define F_ECAN_1000   1       // 1 sets ECAN module for 1000kbps

/*********************************************************************
*
*                            Global Variables 
*
*********************************************************************/
unsigned char temp_EIDH;    //Extended Identifier, high byte
unsigned char temp_EIDL;    //Extended Identifier, low byte
unsigned char temp_SIDH;    //Standard Identifier, high byte
unsigned char temp_SIDL;    //Standard Identifier, low byte
unsigned char temp_DLC;     //Data Length Control value, 0 to 8
unsigned char temp_D0;      //Data byte 0 through...
unsigned char temp_D1;
unsigned char temp_D2;
unsigned char temp_D3;
unsigned char temp_D4;
unsigned char temp_D5;
unsigned char temp_D6;
unsigned char temp_D7;      //Data byte 7



/*********************************************************************
*
*     Function: Initialize the CAN Module
*
*********************************************************************/
void InitECAN(void)
{
    // Place CAN module in configuration mode, see CANCON register data
    CANCON = 0x80;    //REQOP bits <2:0> = 0b100
    while(!(CANSTATbits.OPMODE2 ==1));    //Wait for op-mode bits in the
                                            //CANSTAT register to = 0b100
                                            //to indicate config mode OK

    // Enter CAN module into Mode 0, standard legacy mode; see ECANCON register
    
    ECANCON = 0x00;
    
    // See Microchip application note AN754, Understanding Microchip's
    // CAN Module Bit Timing."  See also: Microchip Controller Area Network
    //(CAN) Bit Timing Calculator, available at Intrepid Control Systems:
    //www.intrepidcs.com/support/mbtime.htm.
    
    // Initialize CAN Bus bit rate timing. Assumes only four standard rates.  
    if (F_ECAN_100==1)  //  100 kbps @ 64 MHz 
    {
        BRGCON1 = 0x93; //0001 1111     //SJW=3TQ     BRP  19
        BRGCON2 = 0xB8; //1011 1000     //SEG2PHTS 1    sampled once  PS1=8TQ  PropagationT 1TQ  
        BRGCON3 = 0x05; //0000 0101     //PS2  6TQ
    } 
    else if (F_ECAN_125==1) //  125 kbps @ 64 MHz
    {
        BRGCON1 = 0x8F; //0000 0111     //SJW=3TQ     BRP  15
        BRGCON2 = 0xB8; //1011 1000     //SEG2PHTS 1    sampled once  PS1=8TQ  PropagationT 1TQ  
        BRGCON3 = 0x05; //0000 0101     //PS2  6TQ 
    }
    else if (F_ECAN_500==1) //  500 kbps @ 64 MHz
    {
        BRGCON1 = 0x83; //0000 0111     //SJW=3TQ     BRP  3
        BRGCON2 = 0xB8; //1011 1000     //SEG2PHTS 1    sampled once  PS1=8TQ  PropagationT 1TQ  
        BRGCON3 = 0x05; //0000 0101     //PS2  6TQ
    }
    else if (F_ECAN_1000==1)  //  1 Mbps (1000 kbps) @ 64 MHz
    {
          
        BRGCON1 = 0x81; //0000 0011     //SJW=3TQ     BRP  1
        BRGCON2 = 0xB8; //1011 1000     //SEG2PHTS 1    sampled once  PS1=8TQ  PropagationT 1TQ  
        BRGCON3 = 0x05; //0000 0101     //PS2  6TQ
    } 
    else                //default to 100 kbps if necessary
    {
        //  100 Kbps @ 64MHz  
        BRGCON1 = 0x93; //0001 1111     //SJW=3TQ     BRP  31
        BRGCON2 = 0xB8; //1010 0000     //SEG2PHTS 1    sampled once  PS1=8TQ  PropagationT 1TQ  
        BRGCON3 = 0x05; //0000 0010     //PS2  6TQ
    }

    // Initialize Receive Masks, see registers RXMxEIDH, RXMxEIDL, etc...
    // Mask 0 (M0) will accept NO extended addresses, but any standard address
    RXM0EIDH = 0x00;    // Extended Address receive acceptance mask, high byte 
    RXM0EIDL = 0x00;    // Extended Address receive acceptance mask, low byte
    RXM0SIDH = 0xFF;    // Standard Address receive acceptance mask, high byte
    RXM0SIDL = 0xE0;    // Standard Address receive acceptance mask, low byte
    
    // Mask 1 (M1) will accept NO extended addresses, but any standard address
    RXM1EIDH = 0x00;    // Extended Address receive acceptance mask, high byte    
    RXM1EIDL = 0x00;    // Extended Address receive acceptance mask, low byte
    RXM1SIDH = 0xFF;    // Standard Address receive acceptance mask, high byte
    RXM1SIDL = 0xE0;    // Standard Address receive acceptance mask, low byte
    
    // Mode 0 allows use of receiver filters RXF0 through RXF5. Enable filters
    // RXF0 and RXF1, all others disabled. See register RXFCONn.
    //  Only using two filters
    RXFCON0 = 0x03;     //Enable Filter-0 and Filter-1; disable others 
    RXFCON1 = 0x00;     //Disable Filters 8 through 15
    
    // Initialize Receive Filters
    //  Filter 0 = 0x32C0
    //  Filter 1 = 0x33C0
   
    RXF0EIDH = 0x00;    //Extended Address Filter-0 unused, set high byte to 0
    RXF0EIDL = 0x00;    //Extended Address Filter-0 unused, set low byte to 0
    RXF0SIDH = 0x32;    //Standard Address Filter-0 high byte set to 0x32
    RXF0SIDL = 0xC0;    //Standard Address Filter-0 low byte set to 0xC0

    RXF2EIDH = 0x00;    //Extended Address Filter-0 unused, set high byte to 0
    RXF2EIDL = 0x00;    //Extended Address Filter-0 unused, set low byte to 0
    RXF2SIDH = 0x33;    //Standard Address Filter-0 high byte set to 0x33
    RXF2SIDL = 0xC0;    //Standard Address Filter-0 low byte set to 0xC0
    
    // After configuring CAN module with above settings, return it
    // to Normal mode
    CANCON = 0x00;
    while(CANSTATbits.OPMODE0==0x00);        //Wait for op-mode bits in the
                                            //CANSTAT register to = 0b000
                                            //to indicate Normal mode OK
    
    // Set Receiving Modes for receiver buffers 0 and 1
    RXB0CON = 0x00;     // See register RXB0CON
    RXB1CON = 0x00;     // See register RXB1CON    
}

/*********************************************************************
*
*     Function: Check the buffers to determine if they have messages
*               if so, transfer the info to the temporary-storage
*               variables. Note: Messages to receiver 0 or 1 get saved in
*               the same variables.  This id done for simplicity in
*               this example. You could save messages to separate
*               variables, or in separate arrays, if you wish. 
*
*********************************************************************/
/*unsigned char ECAN_Receive(void)
{
    unsigned char RXMsgFlag;    // Temporary storage for message flag
    RXMsgFlag = 0x00;           // Set message flag to zero to start
    
    if (RXB0CONbits.RXFUL)      // Check RXB0CON bit RXFUL to see if RX Buffer 0
                                // has received a message, if so, get the
                                // associated data from the buffer and save it.
    {
        temp_EIDH = RXB0EIDH;
        temp_EIDL = RXB0EIDL;
        temp_SIDH = RXB0SIDH;
        temp_SIDL = RXB0SIDL;
        temp_DLC =  RXB0DLC;
        temp_D0 =   RXB0D0;
        temp_D1 =   RXB0D1;
        temp_D2 =   RXB0D2;
        temp_D3 =   RXB0D3;
        temp_D4 =   RXB0D4;
        temp_D5 =   RXB0D5;
        temp_D6 =   RXB0D6;
        temp_D7 =   RXB0D7;
        RXB0CONbits.RXFUL = 0;      // Reset buffer-0-full bit to show "empty"
        RXMsgFlag = 0x01;           // Set message flag to 1
    }
    else if (RXB1CONbits.RXFUL) // Check RXB1CON bit RXFUL to see if RX Buffer 1
                                // has received a message, if so, get the
                                // associated data from the buffer and save it.
    {
        temp_EIDH = RXB1EIDH;
        temp_EIDL = RXB1EIDL;
        temp_SIDH = RXB1SIDH;
        temp_SIDL = RXB1SIDL;
        temp_DLC =  RXB1DLC;
        temp_D0 =   RXB1D0;
        temp_D1 =   RXB1D1;
        temp_D2 =   RXB1D2;
        temp_D3 =   RXB1D3;
        temp_D4 =   RXB1D4;
        temp_D5 =   RXB1D5;
        temp_D6 =   RXB1D6;
        temp_D7 =   RXB1D7;
        RXB1CONbits.RXFUL = 0;      //Reset buffer-1-full bit to show "empty"
        RXMsgFlag = 0x01;           // Set message flag to 1
    }
    else if (B0CONbits.RXFUL) //CheckB0
    {
        temp_EIDH = B0EIDH;
        temp_EIDL = B0EIDL;
        temp_SIDH = B0SIDH;
        temp_SIDL = B0SIDL;
        temp_DLC = B0DLC;
        temp_D0 = B0D0;
        temp_D1 = B0D1;
        temp_D2 = B0D2;
        temp_D3 = B0D3;
        temp_D4 = B0D4;
        temp_D5 = B0D5;
        temp_D6 = B0D6;
        temp_D7 = B0D7;
        
        B0CONbits.RXFUL = 0;
        RXMsgFlag = 0x01;
    }
    
    if  (RXMsgFlag == 0x01)     // Test message flag.
                                // if message flag is a 1...
    {
        RXMsgFlag = 0x00;       // Clear the message flag
        PIR5bits.RXB1IF = 0;    // Clear the Buffer-1 interrupt bit (if used) 
        return TRUE;            // Return a "true" condition to the code that
                                // called the ECAN_Receive function.
    }
    else
    {
        return FALSE;           // Otherwise, return a false condition.
    }    
}
*/


/*********************************************************************
*
*                      Transmit Sample Mesaage
*
*********************************************************************/
void ECAN_Transmit(void)
{
    TXB0EIDH = 0x00;
    TXB0EIDL = 0x00;
    
    //0x123    0010 0100 011
    //0x35E    0110 1011 110
    TXB0SIDH = 0x24;
    TXB0SIDL = 0x70;

    TXB0DLC = 0x04;
    TXB0D0 = 0x55;
    TXB0D1 = 0xff;
    TXB0D2 = 0x00;
    TXB0D3 = 0xaa;
    TXB0D4 = 0xcc;
    TXB0D5 = 0xdd;
    TXB0D6 = 0xee;
    TXB0D7 = 0xff;
    
    TXB0CONbits.TXREQ = 1; //Set the buffer to transmit

    
}

