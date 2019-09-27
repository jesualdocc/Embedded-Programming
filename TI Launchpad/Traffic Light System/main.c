#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "tm4c123gh6pm.h"

#ifdef DEBUG
void__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

struct States {
	unsigned long lightsCars; //South and West LEDs (6 bits)
	unsigned long lightsPedestrians; //Walk LEDs output (2 bits)
	unsigned long time;    //Delay between states
	unsigned long nextState[8]; //
};

typedef const struct States trafficLights;

//Name of the States
#define GoWest          0
#define WaitWest        1
#define GoSouth         2
#define WaitSouth       3
#define WalkPedestrian  4
#define HurryOnPed1     5//
#define HurryOffPed1    6
#define HurryOnPed2     7
#define HurryOffPed2    8
#define HurryOnPed3     9 // (States 5 to 13) to flash red walk LED 5 times
#define HurryOffPed3    10
#define HurryOnPed4     11
#define HurryOffPed4    12
#define HurryOnPed5     13 //


unsigned long currentState;
unsigned long input;

trafficLights FSM[14] = {

		// Hex South and West Pins, Hex Walk Pins, Delay, Array for the states

		{ 0x0C, 0x04, 100, { 0, 0, 1, 1, 1, 1, 1, 1 } },//state 1

		{ 0x14, 0x04, 50, { 2, 2, 2, 2, 4, 4, 4, 2 } }, //state 2

		{ 0x21, 0x04, 100, { 2, 3, 2, 3, 3, 3, 3, 3 } }, //state 3

		{ 0x22, 0x04, 50, { 0, 0, 0, 0, 4, 4, 4, 4 } },//state 4

		{ 0x24, 0x08, 100, { 4, 5, 5, 5, 4, 5, 5, 5 } },       //state 5

		{ 0x24, 0x04, 25, { 6, 6, 6, 6, 4, 6, 6, 6 } },       //state 6

		{ 0x24, 0x00, 25, { 7, 7, 7, 7, 4, 7, 7, 7 } },       //state 7

		{ 0x24, 0x04, 25, { 8, 8, 8, 8, 4, 8, 8, 8 } },       //state 8

		{ 0x24, 0x00, 25, { 9, 9, 9, 9, 4, 9, 9, 9 } },       //state 9

		{ 0x24, 0x04, 25, { 10, 10, 10, 10, 4, 10, 10, 10 } },       //state 10

		{ 0x24, 0x00, 25, { 11, 11, 11, 11, 4, 11, 11, 11 } },       //state 11

		{ 0x24, 0x04, 25, { 12, 12, 12, 12, 4, 12, 12, 12 } },       //state 12

		{ 0x24, 0x00, 15, { 13, 13, 13, 13, 4, 13, 13, 13 } },       //state 13

		{ 0x24, 0x00, 10, { 0, 0, 2, 0, 4, 0, 2, 0 } }        //state 14

};

void SysTick_Wait(unsigned long delay1);
void SysTick_Wait10ms(unsigned long delay1);
void PortsInitialization(void);
void SysTick_Init(void);

int main(void) {
	PortsInitialization();
	currentState = GoWest;

	while (1) {

		GPIO_PORTE_DATA_R = FSM[currentState].lightsCars; //Set Car signal lights
		GPIO_PORTA_DATA_R = FSM[currentState].lightsPedestrians; //Set walk Signal lights
		SysTick_Wait10ms(FSM[currentState].time); // Delay between states
		input = GPIO_PORTD_DATA_R;                      //Read Buttons (sensors)
		currentState = FSM[currentState].nextState[input];

	}
}

void Timer0IntHandler(void) {

	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	// Read the current state of the GPIO pin and
	// write back the opposite state

}

void SysTick_Wait10ms(unsigned long delay1) {
	unsigned long i;
	for (i = 0; i < delay1; i++) {
		SysTick_Wait(800000);  // wait 10ms
	}
}

void SysTick_Wait(unsigned long delay1) {
	NVIC_ST_RELOAD_R = delay1 - 1;  // number of counts to wait
	NVIC_ST_CURRENT_R = 0;       // any value written to CURRENT clears
	while ((NVIC_ST_CTRL_R & 0x00010000) == 0) { // wait for count flag
	}
}

void SysTick_Init(void) {
	NVIC_ST_CTRL_R = 0;               // disable SysTick during setup
	NVIC_ST_CTRL_R = 0x00000005;      // enable SysTick with core clock
}

void PortsInitialization(void) {
	SysTick_Init();
	SYSCTL_RCGC2_R = 0x19;        //activate clock for port A,D,E
	SYSCTL_RCGC2_R;       // no need to unlock port D,E,A
	GPIO_PORTE_AMSEL_R = 0x00;          //disable analog on port E
	GPIO_PORTD_AMSEL_R = 0x00;          //disable analog on port D
	GPIO_PORTA_AMSEL_R = 0x00;          //disable analog on port A
	GPIO_PORTE_PCTL_R = 0x00000000;     //enable regular GPIO
	GPIO_PORTD_PCTL_R = 0x00000000;     //enable regular GPIO
	GPIO_PORTA_PCTL_R = 0x00000000;     //enable regular GPIO
	GPIO_PORTE_DIR_R = 0x3F;            //outputs on PE0-5
	GPIO_PORTD_DIR_R = 0x00;            //inputs on PD0-2
	GPIO_PORTA_DIR_R = 0x0C;            //outputs on PA2 and PA3
	GPIO_PORTE_AFSEL_R = 0x00;          //disable alternate function
	GPIO_PORTD_AFSEL_R = 0x00;          //disable alternate function
	GPIO_PORTA_AFSEL_R = 0x00;          //disable alternate function
	GPIO_PORTE_DEN_R = 0x3F;            //enable digital I/O on PE0-5
	GPIO_PORTD_DEN_R = 0x07;            //enable digital I/O on PD0-2
	GPIO_PORTA_DEN_R = 0x0C;            //enable digital I/O on PA2 and PA3

}
