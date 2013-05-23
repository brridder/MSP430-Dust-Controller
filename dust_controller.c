#include <msp430.h>
#include <legacymsp430.h>

#define LED0                BIT0
#define LED1                BIT1
#define LED2                BIT2

#define OUT0                BIT3
#define OUT1                BIT4
#define OUT2                BIT5

#define BUTTON1             BIT6
#define BUTTON2             BIT7

#define MAX_ON_TIME         30
#define MIN_ON_TIME         10
#define ON_TIME_INTERVAL    10

#define MAX_OFF_TIME        OFF_TIME_INTERVAL*3
#define MIN_OFF_TIME        OFF_TIME_INTERVAL*1
#define OFF_TIME_INTERVAL   10//3600

#define STATE_RELAY_OFF     1
#define STATE_RELAY_ON      2

char button_1_flag = 0;
char button_2_flag = 0;
char one_second_timer_counter = 0;
char current_state = STATE_RELAY_OFF;

int off_time = MIN_OFF_TIME;
int on_time = MIN_ON_TIME;

int off_time_flash_counter = 0;
int on_time_flash_counter = 0;

int main_time_counter = 0;


void initLEDs(void) {
    P1DIR |= (LED0 + LED1 + LED2); // Set the LED pins as outputs
    P1OUT = 0;
}

void initOffboardOutputs() {
    P1DIR |= (OUT0 + OUT1 + OUT2);
    // TODO
}

void initButtons(void) {
    P1IE |= BUTTON1 + BUTTON2; // Enable interrupts on buttons
    P1IFG &= ~(BUTTON1); // Clear button1 interrupt flag
    P1IFG &= ~(BUTTON2); // Clear button2 intterupt flag
}

void initTimer(void) {
    TACTL = TASSEL_1 | MC_1; // Use ACLK, up to CCR0
    TACCTL0 = CCIE;
    TA0CCR0 = 32765/10; // Approximately 100 milliseconds using the external clk
}

void relayOn(void) {
    P1OUT |= (LED0);
    P1OUT |= OUT0;
}

void relayOff(void) {
    P1OUT &= ~(LED0);
    P1OUT &= ~(OUT0);
}

void checkState(void) {
    if (current_state == STATE_RELAY_OFF) {
        if (main_time_counter >= off_time) {
            main_time_counter = 0;
            current_state = STATE_RELAY_ON;
            relayOn();
        }
    } else if (current_state == STATE_RELAY_ON) {
        if (main_time_counter >= on_time) {
            main_time_counter = 0;
            current_state = STATE_RELAY_OFF;
            relayOff();
        }
    }
}


int main(void) {
    WDTCTL = WDTPW + WDTHOLD; // Stop WDT

    initLEDs();
    initButtons();
    initTimer();
    initOffboardOutputs();

    main_time_counter = 0;

    WRITE_SR(GIE);

    while(1) {
        checkState();
    }
}

interrupt(PORT1_VECTOR) port1_isr(void) {
    if ((P1IFG & BUTTON1) == BUTTON1) {
        if (button_1_flag == 0) {
            button_1_flag++;
            off_time += OFF_TIME_INTERVAL;
            if (off_time > MAX_OFF_TIME) {
                off_time = MIN_OFF_TIME;
            }
        }
        P1IFG &= ~BUTTON1;
    } else if ((P1IFG & BUTTON2) == BUTTON2) {
        if (button_2_flag == 0) {
            button_2_flag++;
            on_time += ON_TIME_INTERVAL;
            if (on_time > MAX_ON_TIME) {
                on_time = MIN_ON_TIME;
            }
        }
        P1IFG &= ~BUTTON2;
    }
}

interrupt(TIMER0_A0_VECTOR) timer_a0_isr(void) {
    one_second_timer_counter++;
    on_time_flash_counter += ON_TIME_INTERVAL/2;
    off_time_flash_counter += OFF_TIME_INTERVAL/2;
    if (one_second_timer_counter >= 10) {
        one_second_timer_counter = 0;
        P1OUT &= ~(LED1);
        P1OUT &= ~(LED2);
        on_time_flash_counter = 0;
        off_time_flash_counter = 0;
        main_time_counter++;
    }
    if (on_time_flash_counter < on_time) {
        P1OUT ^= (LED1);
    }
    if (off_time_flash_counter < off_time) {
        P1OUT ^= (LED2);
    }
    button_1_flag = 0;
    button_2_flag = 0;
}
