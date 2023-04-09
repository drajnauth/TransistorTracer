#ifndef _UART_H_
#define _UART_H_


#define RBUFF 40		// Max RS232 Buffer Size
#define MAXIMUM_DIGITS 6	// Max numerical digits to process
#define MAX_COMMAND_ENTRIES 6 

void processSerial ( void );
unsigned char parseSerial ( char *str );
void resetSerial (void);
void errorOut ( void );
void flushSerialInput (void);

#endif // _UART_H_
