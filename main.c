/* 
 * File:   main.c
 * Author: user
 *
 * Created on 2014. március 21., 9:37
 */

#include <stdio.h>
#include <stdlib.h>
#include <pic16f873.h>

// CONFIG
#pragma config FOSC = EXTRC     // Oscillator Selection bits (RC oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = ON      // Power-up Timer Enable bit (PWRT disabled)
#pragma config CP = OFF         // FLASH Program Memory Code Protection bits (Code protection off)
#pragma config BOREN = ON       // Brown-out Reset Enable bit (BOR enabled)
#pragma config LVP = OFF         // Low Voltage In-Circuit Serial Programming Enable bit (RB3/PGM pin has PGM function; low-voltage programming enabled)
#pragma config CPD = OFF        // Data EE Memory Code Protection (Code Protection off)
#pragma config WRT = ON         // FLASH Program Memory Write Enable (Unprotected program memory may be written to by EECON control)


#define TMR0_DATA 20
#define BT_START
#define BT_PAUSE

#define BT_STOP   0x0001    //
#define BT_B0     0x0002    //
#define BT_DP     0x0004    //
#define BT_ENT    0x0008

#define BT_HOLD   0x0010    //
#define BT_B1     0x0020    //
#define BT_B2     0x0040    //
#define BT_B3     0x0080    //

#define BT_START  0x0100    //
#define BT_B4     0x0200    //
#define BT_B5     0x0400    //
#define BT_B6     0x0800    //

//#define BT_START  0x0100    //
#define BT_B7     0x2000    //
#define BT_B8     0x4000    //
#define BT_B9     0x8000    //


#define SLACK_B_DOWN  PORTAbits.RA5   // Slack band switch downwards term 26
#define SLACK_B_UP    PORTCbits.RC4   // Slack band switch upwards term 24

#define INITIATOR_END PORTCbits.RC7   // Initiator end stop term 21
#define INITIATOR_COUNT PORTCbits.RC6 // Initiator count term 22
#define START         PORTCbits.RC5   // Start (remote) term 4

/******************************************************************************/
/* General interrupt handler for treats the timer, and counter interrupts     */
/******************************************************************************/

int TMR0_COUNTER;
/* @TEST_BT_STATES: button states datas, contains the new button states datas. */
unsigned int TEST_BT_STATES;
/* @BT_STATES: button states. */
unsigned int BT_STATES;
unsigned int BT_FALL_EDGES;
unsigned int BT_RISE_EDGES;
/* row selector for button reading. */
unsigned char row_selector = 0b00000001;
int row_counter = 0;

void interrupt isr(void)
{
  if (INTCONbits.T0IF)/* */
  {
  row_selector = 0x01;
  TEST_BT_STATES = 0;

  for (row_counter = 0; row_counter < 4; row_counter++)
  {
    PORTC &= 0xF0;
    // select the next row.
//    PORTC |= (~row_selector) & 0x0F;
    PORTC |= (~row_selector) & 0x0F;
    // make the BT_STATES from PORTB inputs.
    TEST_BT_STATES |= ((~PORTB) & 0x0F) << (row_counter * 4);
    // select the next row.
    row_selector <<= 1;
  }

  // RISE, and FALL edges words.
  BT_RISE_EDGES = (TEST_BT_STATES ^ BT_STATES) & TEST_BT_STATES;
  BT_FALL_EDGES = (TEST_BT_STATES ^ BT_STATES) & BT_STATES;

  BT_STATES = TEST_BT_STATES;

    if (PORTAbits.RA5)
    {
//      PORTAbits.RA5 = 0;
    }
    else 
    {
//      PORTAbits.RA5 = 1;
    }
    TMR0 = TMR0_DATA;
    INTCONbits.T0IF = 0;  // Reset interrupt flag.
  }
}

void wait()
{ int i;
  for (i = 0; i< 8000; i++)
  {
  }
}

/*
 * 
 */
int main(int, char**) {

  /* Initialize time base. */

  OPTION_REGbits.T0CS = 0b00; /* TMR0 Clock Source Select bit
                               0 = Internal instruction cycle clock (CLKOUT) */
  OPTION_REGbits.PSA = 0;   //Prescaler is assigned to the Timer0 module

  OPTION_REGbits.PS = 0b111;  /*
                               *                   */

  ADCON1bits.PCFG = 0b0101; // D D D D VREF+ D A A RA3 VSS 2/1

  TRISAbits.TRISA5 = 0;     //

  TRISC = 0b00000000;     // RC 0-7 all outputs
  OPTION_REGbits.nRBPU = 0; //

  TMR0 = TMR0_DATA;
  
  INTCONbits.T0IF = 0;

  INTCONbits.T0IE = 1;        // Enable T0 interrupt.
  INTCONbits.GIE = 1;         // Enable global interrupt

  while (1)
  {
    if (BT_FALL_EDGES & BT_START)
    {
      if (PORTCbits.RC4)
      PORTCbits.RC4 = 0;
      else
      PORTCbits.RC4 = 1;
      /* Clear the button event. */
      BT_FALL_EDGES &= ~BT_START;
    };

    if (BT_FALL_EDGES & BT_B7)
    {
      if (PORTCbits.RC5)
      PORTCbits.RC5 = 0;
      else
      PORTCbits.RC5 = 1;
      /* Clear the button event. */
      BT_FALL_EDGES &= ~BT_B7;
    };

    if (BT_FALL_EDGES & BT_B8)
    {
      if (PORTCbits.RC6)
      PORTCbits.RC6 = 0;
      else
      PORTCbits.RC6 = 1;
      /* Clear the button event. */
      BT_FALL_EDGES &= ~BT_B8;
    };
    if (BT_FALL_EDGES & BT_B9)
    {
      if (PORTCbits.RC7)
      PORTCbits.RC7 = 0;
      else
      PORTCbits.RC7 = 1;
      /* Clear the button event. */
      BT_FALL_EDGES &= ~BT_B9;
    };

  }
  return (EXIT_SUCCESS);
}

