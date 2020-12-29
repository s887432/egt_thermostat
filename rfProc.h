#ifndef __RFPROC_H__
#define __RFPROC_H__

// ******************************************
// global type define
// ******************************************
typedef enum __RF_COMMANDS__
{
	COMMAND_NONE = 0,
	COMMAND_LIGHT_ON,
	COMMAND_LIGHT_OFF,
	COMMAND_TEMP_SET,
	COMMAND_FANS_CTRL,

	COMMAND_TEMP_DISP,
}RF_COMMANDS;

typedef void (*RF_CALLBACK)(unsigned char, unsigned char);
// ******************************************
// RF device receiver thread process
//
// Paramaters
//    none
//
// Return value
//    none
// ******************************************
void *rfProc(void *ptr);

// ******************************************
// send data to RF device via UART port
// 
// Paramaters
//    cmd: RF command
//    param: paramaters
//
// Return value
//    0: success
//    others: fail
// ******************************************
int rfSend(RF_COMMANDS cmd, int param);

void rfSetCallback(RF_CALLBACK func);
#endif // end of __RFPROC_H__

// end of file

