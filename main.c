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

/* @BUTTON symbols */

#define BT_STOP   0x0001    //
#define BT_ESC    0x0002    //
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

#define BT_B7     0x2000    //
#define BT_B8     0x4000    //
#define BT_B9     0x8000    //


#define SLACK_B_DOWN  PORTAbits.RA5   // Slack band switch downwards term 26
#define SLACK_B_UP    PORTCbits.RC4   // Slack band switch upwards term 24

#define INITIATOR_END PORTCbits.RC7   // Initiator end stop term 21
#define INITIATOR_COUNT PORTCbits.RC6 // Initiator count term 22
#define START         PORTCbits.RC5   // Start (remote) term 4

#define RED_SIGNAL    PORTAbits.RA4   // red signal LED on the button board.
#define GREEN_SIGNAL  PORTAbits.RA3   // green signal LED on the button board.
#define YEL_SIGNAL    PORTAbits.RA2   // yellow signal LED on the button PCB.
#define TIMER_2_PRESET 100

#define ASCDESC_WT_TIME 20
#define ST_WT_TIME 40
#define ST_HOLD_TIME 80
#define ST_IE_TIME 120
#define MAX_BASE_COUNT  129
#define TEST_COUNT 75

typedef union  {
  struct {
    unsigned PR_START: 1;
    unsigned COUNT_STATE: 1;
    unsigned END_TEST: 1;
  };
} U_TEST_STATE;

/******************************************************************************/
/* General interrupt handler for treats the timer, and counter interrupts     */
/******************************************************************************/

int TMR0_COUNTER;
/* @TEST_BT_STATES: button states datas, contains the new button states datas. */
unsigned int TEST_BT_STATES;
/* @BT_STATES: button states. */
unsigned int BT_STATES;
/* @BT_FALL_EDGES: falling edges indicating of buttons. */
unsigned int BT_FALL_EDGES;
/* @BT_RISE_EDGES: rising edges indicating of buttons. */
unsigned int BT_RISE_EDGES;
/* row selector for button reading. */
unsigned char row_selector = 0b00000001;
/* Row counter for for cycle. */
int row_counter = 0;

unsigned char PRESET_DATA = TIMER_2_PRESET;

int desired_count = (MAX_BASE_COUNT + 1);
int meas_count = (TEST_COUNT);


int actual_count;

int time_counter;
int time_post_counter;

unsigned char PROGRAM_STATE = 1;  // Program state

U_TEST_STATE TEST_STATE;

/* void ClearOuts() clear all outputs for program state change.*/

void ClearOuts()
{
  SLACK_B_DOWN = SLACK_B_UP = INITIATOR_END = INITIATOR_COUNT = START = 0;
}

void ResetProgram()
{
  ClearOuts();
  time_counter = 0;
  actual_count = 0;
}

void ClearEvents()
{
  BT_FALL_EDGES = 0;
};

void SetProgramState(int newState)
{
  ClearOuts();
  switch (newState)
  {
    /* Normal test state. */
    case 1:
      RED_SIGNAL = 0;
      GREEN_SIGNAL = 1;
      YEL_SIGNAL = 1;
      break;
   /* Setup state: */
    case 2:
      RED_SIGNAL = 1;
      GREEN_SIGNAL = 0;
      YEL_SIGNAL = 1;
      break;
    /* Setup timer state. */
    case 3:
      RED_SIGNAL = 1;
      GREEN_SIGNAL = 1;
      YEL_SIGNAL = 0;
      break;
  }
  PROGRAM_STATE = newState;
}

void interrupt isr(void)
{
  if (INTCONbits.T0IF)/* T0 timer interrupt. */
  {
  row_selector = 0x01;
  TEST_BT_STATES = 0;

  for (row_counter = 0; row_counter < 4; row_counter++)
  {
    PORTC &= 0xF0;
    /* Select the next row to pull down the C port row selector pin. */
    PORTC |= (~row_selector) & 0x0F;
    /* Get the B port button status, then make the TEST_BT_STATES integer. */
    TEST_BT_STATES |= ((~PORTB) & 0x0F) << (row_counter * 4);
    /* Select the next row with shifting the row_selector. */
    row_selector <<= 1;
  }

  // RISE, and FALL edges words.
  BT_RISE_EDGES = (TEST_BT_STATES ^ BT_STATES) & TEST_BT_STATES;
  BT_FALL_EDGES = (TEST_BT_STATES ^ BT_STATES) & BT_STATES;

  BT_STATES = TEST_BT_STATES;
  
    TMR0 = TMR0_DATA;
    INTCONbits.T0IF = 0;  // Reset interrupt flag.
  }else
    if (PIR1bits.TMR2IF)
    {

      if (TEST_STATE.PR_START)
      {
        /* Indicator blinking always. */
        if (!RED_SIGNAL)
        {
          RED_SIGNAL = 1;
//        INITIATOR_COUNT = 1;
        } else
        {
          RED_SIGNAL = 0;
//        INITIATOR_COUNT = 0;
        }
        time_counter++;

        /* SLACK BAND SWITCH ON TIME. */

        if (time_counter == ASCDESC_WT_TIME)
        {
          SLACK_B_DOWN = 1;
        };

        /* START SIGNAL WAITING ON TIME. */
        if (time_counter == ST_WT_TIME)
        {
          START = 1;

        };
        /* Initiator end stop must on within certainly time. */
        if (time_counter == ST_IE_TIME)
        {
          INITIATOR_END = 1;
          TEST_STATE.COUNT_STATE = 1;
        }

        /* Start OK, the may going the counting.*/

        if ((time_counter > ST_HOLD_TIME) && (TEST_STATE.COUNT_STATE))
        {
          if (time_counter & 0x02)
          {
            INITIATOR_COUNT = 1;
          } else
          {
            actual_count++;
            if (actual_count == desired_count)
            {
              /* end of counting. */
//              TEST_STATE.COUNT_STATE = 0;
//              TEST_STATE.END_TEST = 1;
            }
            if (actual_count == meas_count)
            {
//              INITIATOR_END = 1;
//              SLACK_B_DOWN = 0;
//              SLACK_B_UP = 1;
            }
            if (actual_count == meas_count + 5)
            {
//              SLACK_B_DOWN = 1;
//              SLACK_B_UP = 0;
            }
            INITIATOR_COUNT = 0;
          }
        }
/*        if (TEST_STATE.END_TEST)
        {
          ClearOuts();
          time_counter = 0;
          actual_count = 0;
          TEST_STATE.END_TEST = 0;
        }*/
        
      }
      PIR1bits.TMR2IF = 0;
    };

}

void wait(int wt)
{ int i;
  for (i = 0; i< wt; i++)
  {
  }
}

/*
 * 
 */
int main(int, char**) {

  int counterdown;
  /* Initialize time base. */

  OPTION_REGbits.T0CS = 0b00; /* TMR0 Clock Source Select bit
                               0 = Internal instruction cycle clock (CLKOUT) */
  OPTION_REGbits.PSA = 0;   //Prescaler is assigned to the Timer0 module

  OPTION_REGbits.PS = 0b111;  /*
                               *                   */

  ADCON1bits.PCFG = 0b0111;

  TRISAbits.TRISA5 = 0;     //
  TRISAbits.TRISA4 = 0;
  TRISAbits.TRISA3 = 0;
  TRISAbits.TRISA2 = 0;
  PORTA = 0;
  
  TRISC = 0b00000000;     // RC 0-7 all outputs
  PORTC = 0;

  OPTION_REGbits.nRBPU = 0; //

  TMR0 = TMR0_DATA;
  
  INTCONbits.T0IF = 0;

  INTCONbits.T0IE = 1;        // Enable T0 interrupt.


  PR2 = TIMER_2_PRESET;
  T2CONbits.T2CKPS = 0b11;
  T2CONbits.TOUTPS = 0b1111;

  PIR1bits.TMR2IF = 0;
  PIE1bits.TMR2IE = 1;
  T2CONbits.TMR2ON = 1;

  INTCONbits.PEIE = 1;        // peripherial interrupt enable.
  INTCONbits.GIE = 1;         // Enable global interrupt

  SetProgramState(1);

  while (1)
  {

    switch (PROGRAM_STATE)
    {
      /* Program state = running test. */
      case 1:

    if (BT_FALL_EDGES & BT_B1)
    {
      if (!TEST_STATE.PR_START)
      {
        ResetProgram();
        TEST_STATE.PR_START = 1;
      } else
      {
        SetProgramState(1);
        TEST_STATE.PR_START = 0;
      }
      BT_FALL_EDGES &= ~BT_B1;
    };

/* ------------------- Reset the program state --------------------------- */
        
        if (BT_FALL_EDGES & BT_ESC) ClearEvents();
        if (BT_FALL_EDGES & BT_ENT)
        {
          TEST_STATE.PR_START = 0;
          ClearEvents();
          SetProgramState(2);
        };

/* ---------------------------------------------------------------------- */
        break;
      /* Program state = I/O test.*/
      case 2:

/* Task for I/O reset program state. */

    if (BT_FALL_EDGES & BT_B1)
    {
      /* Teste the slack down */
      if  (!SLACK_B_DOWN)
      {
        SLACK_B_DOWN = 1;
      }
      else
      {
        SLACK_B_DOWN = 0;
      }
      BT_FALL_EDGES &= ~BT_B1;
    };

    if (BT_FALL_EDGES & BT_B2)
    {
      /* Test start signal. */
      if  (!SLACK_B_UP)
      {
        SLACK_B_UP = 1;
      }
      else
      {
        SLACK_B_UP = 0;
      }
      BT_FALL_EDGES &= ~BT_B2;
    };

    if (BT_FALL_EDGES & BT_B3)
    {
      /* Test start signal. */
      if  (!START)
      {
        START = 1;
      }
      else
      {
        START = 0;
      }
      BT_FALL_EDGES &= ~BT_B3;
    };

    if (BT_FALL_EDGES & BT_B4)
    {
      /* Test start signal. */
      if  (!INITIATOR_COUNT)
      {
        INITIATOR_COUNT = 1;
      }
      else
      {
        INITIATOR_COUNT = 0;
      }
      BT_FALL_EDGES &= ~BT_B4;
    };

    if (BT_FALL_EDGES & BT_B5)
    {
      /* Test start signal. */
      if  (!INITIATOR_END)
      {
        INITIATOR_END = 1;
      }
      else
      {
        INITIATOR_END = 0;
      }
      BT_FALL_EDGES &= ~BT_B5;
    };

/* ------------------- Reset the program state --------------------------- */

        if (BT_FALL_EDGES & BT_ESC)
        {
          ClearEvents();
          SetProgramState(1);
        }
          else if (BT_FALL_EDGES & BT_ENT)
          {
            ClearEvents();
            SetProgramState(3);
          };
/* ---------------------------------------------------------------------- */
        break;
      /* Program state = Options.*/
      case 3: 
        if (BT_FALL_EDGES & BT_ESC)
        {
          ClearEvents();
          SetProgramState(2);
        };
        break;
    }



/*    if (BT_FALL_EDGES & BT_ESC)
    {
      for (counterdown = 0; counterdown < 128; counterdown++)
      {
        INITIATOR_COUNT = 1;
        wait(2000);
        INITIATOR_COUNT = 0;
        wait(2000);
      }
      BT_FALL_EDGES &= ~BT_ESC;
    };

    if (BT_FALL_EDGES & BT_DP)
    {

      if  (!SLACK_B_DOWN)
      {
        SLACK_B_DOWN = 1;
      }
      else
      {
        SLACK_B_DOWN = 0;
      }
      BT_FALL_EDGES &= ~BT_DP;
    };

    if (BT_FALL_EDGES & BT_ENT)
    {

      if  (!START)
      {
        START = 1;
      }
      else
      {
        START = 0;
      }
      BT_FALL_EDGES &= ~BT_ENT;
    };

    if (BT_FALL_EDGES & BT_B1)
    {
      desired_count = 30;
      desired_counter_start = 1;
      BT_FALL_EDGES &= ~BT_B1;

    };*/

/*  Increnementing, and decrementing the counting speed. */
/*    if (BT_FALL_EDGES & BT_B2)
    {
      PRESET_DATA += 20;
      if (PRESET_DATA > 239) { PRESET_DATA = 220; };
      PR2 = PRESET_DATA;
      BT_FALL_EDGES &= ~BT_B2;
    };
    if (BT_FALL_EDGES & BT_B8)
    {
      PRESET_DATA -= 20;
      if (PRESET_DATA < 61) {PRESET_DATA = 60; };
      PR2 = PRESET_DATA;
      BT_FALL_EDGES &= ~BT_B8;
    };*/

    

  }
  return (EXIT_SUCCESS);
}

