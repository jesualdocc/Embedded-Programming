#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

char message[100];
int LEDG = 8; //LED FOR SPACE
int LEDR = 2; // LED FOR DOT
int LEDB = 4; // LED FOR DASH
int flag = 0;

void morseConverter(unsigned char *input);

int main(void) {
	unsigned long ulPeriod;

	//Clock set to 40MHz
	SysCtlClockSet(
	SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

	//System Peripheral Enables
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	//GPIO Pin Config
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,
	GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	//UART Config
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	//FIFO
	UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX4_8, UART_FIFO_RX4_8); //set FIFO level to 8 characters
	UARTFIFOEnable(UART0_BASE); //enable FIFOs
	//Interrupt
	IntEnable(INT_UART0); //enable the UART interrupt
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT); //only enable RX and TX interrupts
	//Initialize
	UARTStdioInit(0); //tells uartstdio functions to use UART0
	UARTprintf("\033[2J\033[HEnter Text: "); // erase screen, put cursor at home position (0,0),

	//Timer Config
	TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER);
	ulPeriod = (SysCtlClockGet() / 10) / 2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ulPeriod - 1);
	//Interrupt
	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	//Enable
	TimerEnable(TIMER0_BASE, TIMER_A);

	//Interrupt Enable
	IntMasterEnable();

	UARTprintf("\033[23\033[H ENTER A MESSAGE PRESS ENTER WHEN DONE\n");

	while (1) {
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
		if (flag) {
			morseConverter(message); // call funtion to convert message
			flag = 0;
		}

	}

}

// Interrupts Handlers

void Timer0IntHandler(void) {
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	// Read the current state of the GPIO pin and
	// write back the opposite state
	if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2)) {
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);
	} else {
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);	//
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);// To allow all LEDs to blink
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);	//
	}
}

// displays dash on Putty Terminal and also flashes the blue LED
void dash() {

	UARTprintf("-");
	SysCtlDelay(2000000 * 10);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, LEDB);
	SysCtlDelay(SysCtlClockGet() / (1000 * 6));

}

// displays dot on Putty Terminal and also flashes the red LED
void dot(void) {
	UARTprintf(".");
	SysCtlDelay(2000000 * 10);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, LEDR);
	SysCtlDelay(SysCtlClockGet() / (1000 * 6));

}

// displays space/blank on Putty Terminal and also flashes the gree LED
void space(void) {
	UARTprintf(" ");
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, LEDG);
	SysCtlDelay(2000000 * 10);
	SysCtlDelay(SysCtlClockGet() / (1000 * 6));
}

void UARTIntHandler(void) {
	int i = 0;
	unsigned long ulStatus;
	unsigned char received_character;
	ulStatus = UARTIntStatus(UART0_BASE, true); //get interrupt status
	UARTIntClear(UART0_BASE, ulStatus); //clear the asserted interrupts
	while (UARTCharsAvail(UART0_BASE)) //loop while there are characters in the receive FIFO
	{

		do { // stores each character entered in an array of strings until user presses enter
			received_character = UARTCharGet(UART0_BASE);
			message[i] = received_character;
			UARTCharPutNonBlocking(UART0_BASE, message[i]);
			i++;

		} while (received_character != 13); //ENTER key breaks out of loop

		if (received_character == 13) {
			UARTprintf("\n Your Message: %s\n", message);
			flag = 1;
		}

	}
}

// Read character in an array and Converts ASCII characters to Morse Code
void morseConverter(unsigned char *input) {
	int i = 0;
	for (i = 0; i < 100; i++) { // itera

		if (input[i] == 0) // Null - Since array size is fixed ignores the empty positions
				{
			break;
		}

		else if (input[i] == 13) // ENTER - Ignores the ASCII character for ENTER since it also stored in the array
				{
			break;
		} else if (input[i] == 32) // Space key
				{
			space();
		}

		else if (input[i] == 65 || input[i] == 97) // A
				{
			dot();
			dash();
		} else if (input[i] == 66 || input[i] == 98) //B
				{
			dash();
			dot();
			dot();
			dot();
		} else if (input[i] == 67 || input[i] == 99) // C
				{
			dash();
			dot();
			dash();
			dot();
		} else if (input[i] == 68 || input[i] == 100) // D
				{
			dash();
			dot();
			dot();
		} else if (input[i] == 69 || input[i] == 101) // E
				{
			dot();
		} else if (input[i] == 70 || input[i] == 102) // F
				{
			dot();
			dot();
			dash();
			dot();
		} else if (input[i] == 71 || input[i] == 103) // G
				{
			dash();
			dash();
			dot();
		} else if (input[i] == 72 || input[i] == 104) // H
				{
			dot();
			dot();
			dot();
			dot();
		} else if (input[i] == 73 || input[i] == 105) // I
				{
			dot();
			dot();
		} else if (input[i] == 74 || input[i] == 106) // J
				{
			dot();
			dash();
			dash();
			dash();
		} else if (input[i] == 75 || input[i] == 107) // K
				{
			dash();
			dot();
			dash();

		} else if (input[i] == 76 || input[i] == 108) // L
				{
			dot();
			dash();
			dot();
			dot();
		}

		else if (input[i] == 77 || input[i] == 109) // M
				{
			dash();
			dash();
		}

		else if (input[i] == 78 || input[i] == 110) // N
				{
			dash();
			dot();
		} else if (input[i] == 79 || input[i] == 111) // 0
				{
			dash();
			dash();
			dash();
		} else if (input[i] == 80 || input[i] == 112) // P
				{
			dot();
			dash();
			dash();
			dot();
		} else if (input[i] == 81 || input[i] == 113) // Q
				{
			dash();
			dash();
			dot();
			dash();
		} else if (input[i] == 82 || input[i] == 114) // R
				{
			dot();
			dash();
			dot();
		} else if (input[i] == 83 || input[i] == 115) // S
				{
			dot();
			dot();
			dot();
		} else if (input[i] == 84 || input[i] == 116) // T
				{
			dash();

		} else if (input[i] == 85 || input[i] == 117) // U
				{
			dot();
			dot();
			dash();
		} else if (input[i] == 86 || input[i] == 118) // V
				{
			dot();
			dot();
			dot();
			dash();
		} else if (input[i] == 87 || input[i] == 119) // W
				{
			dot();
			dash();
			dash();
		} else if (input[i] == 88 || input[i] == 120) // X
				{
			dash();
			dot();
			dot();
			dash();
		} else if (input[i] == 89 || input[i] == 121) // Y
				{
			dash();
			dot();
			dot();
			dash();
		} else if (input[i] == 90 || input[i] == 122) // Z
				{
			dash();
			dash();
			dot();
			dot();
		}

		else if (input[i] == 49) // 1
				{
			dot();
			dash();
			dash();
			dash();
		}

		else if (input[i] == 50) // 2
				{
			dot();
			dot();
			dash();
			dash();
			dash();
		}

		else if (input[i] == 51) // 3
				{
			dot();
			dot();
			dot();
			dash();
			dash();
		} else if (input[i] == 52) // 4
				{
			dot();
			dot();
			dot();
			dot();
			dash();

		} else if (input[i] == 53) // 5
				{
			dot();
			dot();
			dot();
			dot();
			dot();

		}

		else if (input[i] == 54) // 6
				{
			dash();
			dot();
			dot();
			dot();
			dot();

		}

		else if (input[i] == 55) // 7
				{
			dash();
			dash();
			dot();
			dot();
			dot();

		} else if (input[i] == 56) // 8
				{
			dash();
			dash();
			dash();
			dot();
			dot();

		}

		else if (input[i] == 57) // 9
				{
			dash();
			dash();
			dash();
			dash();
			dot();

		}

		else if (input[i] == 48) // 0
				{
			dash();
			dash();
			dash();
			dash();
			dash();

		} else {
			UARTprintf("\n INVALID CHARACTER ENTERED");
		}

	}

}
