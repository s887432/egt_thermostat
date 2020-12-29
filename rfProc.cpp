#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

#include "uartFunc.h"
#include "rfProc.h"
// ******************************************
// global variables
// ******************************************
int fdUart;
unsigned char gDisplayTemp = 0;
RF_CALLBACK rfCallback = NULL;
// ******************************************
// internal constant
// ******************************************
#define CMD_HEADER_1	0x55
#define CMD_HEADER_2	0xAA

#define MIN_EXP_TEMP    15
#define MAX_EXP_TEMP    30
#define EXP_TEMP_OFFSET 15
#define EXP_IMG_INDEX(value) (value-EXP_TEMP_OFFSET)

#define CMD_LIGHTCTRL   0x02
#define CMD_SETTEMP     0x04
#define CMD_SETFAN      0x05

#define HEADER_OFFSET	0
#define HEADER_SIZE		2

#define COMMAND_OFFSET	2
#define COMMAND_SIZE	2

#define LENGTH_OFFSET	4
#define LENGTH_SIZE		2

#define PAYLOAD_OFFSET	6

#define RECEIVE_BUFFER_SIZE	512
// ******************************************
// internal functions
// ******************************************
int makeBufHeader(unsigned char *buf)
{
    buf[0] = CMD_HEADER_1;
    buf[1] = CMD_HEADER_2;
    
    return 2;
}

unsigned char makeBufChecksum(unsigned char *buf, int size)
{
    unsigned int sum = 0;
    int i;
    
    for(i=0; i<size; i++)
    {
        sum += buf[i];
    }
    
    return (sum&0xFF);
}

int makeCmdLightCtrl(unsigned char *buf, unsigned char val)
{
    int offset = 0;
    
    offset += makeBufHeader(buf);
    buf[offset++] = (CMD_LIGHTCTRL>>8)&0xFF;
    buf[offset++] = (CMD_LIGHTCTRL&0xff);

    buf[offset++] = 0;
    buf[offset++] = 1;

    buf[offset++] = val;
    
    buf[offset] = makeBufChecksum(buf, offset);
    offset++;
    
    buf[offset++] = 0x0D;
    buf[offset++] = 0x0A;
    
    return offset;
}

int makeCmdSetTemp(unsigned char *buf, unsigned char val)
{
    int offset = 0;
    
    offset += makeBufHeader(buf);
    buf[offset++] = (CMD_SETTEMP>>8)&0xFF;
    buf[offset++] = (CMD_SETTEMP&0xff);

    buf[offset++] = 0;
    buf[offset++] = 1;

    buf[offset++] = val;
    
    buf[offset] = makeBufChecksum(buf, offset);
    offset++;
    
    buf[offset++] = 0x0D;
    buf[offset++] = 0x0A;
    
    return offset;
}

int makeCmdSetFan(unsigned char *buf, unsigned char val)
{
    int offset = 0;
    
    offset += makeBufHeader(buf);
    buf[offset++] = (CMD_SETFAN>>8)&0xFF;
    buf[offset++] = (CMD_SETFAN&0xff);

    buf[offset++] = 0;
    buf[offset++] = 1;

    buf[offset++] = val;
    
    buf[offset] = makeBufChecksum(buf, offset);
    offset++;
    
    buf[offset++] = 0x0D;
    buf[offset++] = 0x0A;
    
    return offset;
}

static unsigned char cmd_checksum(unsigned char *buffer, unsigned char size)
{
	unsigned char i;
	unsigned int sum=0;
	
	for(i=0; i<size; i++)
	{
		sum += buffer[i];
	}
	
	return (sum&0xFF);
}

int cmd_proc(unsigned char *buffer, unsigned char size)
{
	unsigned short command;
	unsigned short payload_size;
	int ret = 0;

	if( buffer[HEADER_OFFSET] != CMD_HEADER_1 || buffer[HEADER_OFFSET+1] != CMD_HEADER_2)
	{
		// incorrect command header
		printf("incorrect command header\r\n");
		return -1;
	}
	
	payload_size = (buffer[LENGTH_OFFSET]<<8) + buffer[LENGTH_OFFSET+1];
	
	if( size != (payload_size+7) )
	{
		// command buffer size error
		printf("command buffer size error[%d][%d]\r\n", size, payload_size);
		return -1;
	}
	
	if( cmd_checksum(buffer+HEADER_OFFSET, size-1) != buffer[PAYLOAD_OFFSET+payload_size])
	{
		// checksum fail
		printf("checksum fail\r\n");
		return -1;
	}
	
	command = (buffer[COMMAND_OFFSET]<<8)+buffer[COMMAND_OFFSET+1];
	
	switch(command)
	{
		case 0x8003:
		{
            unsigned char integer;
            unsigned char dec;
            
            integer = buffer[PAYLOAD_OFFSET];
            dec = buffer[PAYLOAD_OFFSET+1];

			// TODO...
			if( gDisplayTemp )
			{
				printf("temp = %d.%d\r\n", integer, dec);
			}

			if( rfCallback != NULL )
			{
				rfCallback(integer, dec);
			}
		}
		break;
			
		default:
			// unknown command
			printf("Unkown command(%04X)\r\n", command);
			ret = -1;
			break;
	}
	
	return ret;
}

// TODO...
// re-position the buffer if 0x55 0xaa is not in beginning of buffer
int checkCommand(unsigned char *buffer, int length, unsigned char *cmdBuf)
{
	int cmdSize = 0;
	int header = -1;
	int tail = 0;
	int index = 0;

	int package_size = 0;

	// find header
	for(int i=0; i<length-1; i++)
	{
		if( buffer[i] == CMD_HEADER_1 && buffer[i+1] == CMD_HEADER_2 )
		{
			header = i;
			index = i+HEADER_SIZE;
		}
	}

	if( header >= 0 )
	{
		// header found
		if( (index + COMMAND_SIZE) <= length )
		{
			// command existed
			index += COMMAND_SIZE;
		}
		else
		{
			// command package is not complete
			return cmdSize;
		}

		// command found
		if( (index + LENGTH_SIZE) <= length )
		{
			// length existed
			package_size = buffer[index] * 256 + buffer[index+1];
			index += COMMAND_SIZE;			
		}
		else
		{
			// command package is not complete
			return cmdSize;
		}

		// package size found
		if( (index + package_size) <= length )
		{
			// payload existed
			index += package_size;
		}
		else
		{
			// command package is not complete
			return cmdSize;
		}

		// payload found
		if( (index + 2) <= length )
		{
			// tail existed
			index += 2;
			tail = index -1;
		}
		else
		{
			// command package is not complete
			return cmdSize;
		}

		// command is complete
		memcpy(cmdBuf, buffer+header, index);
		cmdSize = tail - header;

		// move buffer
		if( length >= index )
		{
			memcpy(buffer, buffer+index, length-index-2);
		}
	}

	if( header >= 0 && tail >=0 )
	{
		// command package is complete
		
	}

	return cmdSize;
}
// ******************************************
// extern functions
// ******************************************

// ******************************************
// RF device receiver thread process
//
// Paramaters
//    none
//
// Return value
//    none
// ******************************************
void *rfProc(void *ptr)
{
	int nread;
	unsigned char buff[RECEIVE_BUFFER_SIZE];	

	unsigned char receiveBuffer[RECEIVE_BUFFER_SIZE];
	int receiveIndex = 0;

	unsigned char cmdBuffer[RECEIVE_BUFFER_SIZE];
	int cmdSize;

	char dev[]  = "/dev/ttyS2";

	fdUart = uartOpen(dev);
	uartSetSpeed(fdUart, 115200);

	if (uartSetParity(fdUart,8,1,'N') == -1)
	{
		printf("Set Parity Error\n");
		exit (0);
	}

	gDisplayTemp = 0;

	while (1)
	{
		while((nread = read(fdUart, buff, 512))>0)
		{
			if( nread+receiveIndex >= RECEIVE_BUFFER_SIZE )
			{
				printf("receiver buffer overflow. it must be something wrong.\r\n");
				// TODO...
				receiveIndex = 0;
			} 

			memcpy(receiveBuffer+receiveIndex, buff, nread); 
			receiveIndex += nread;

			if( (cmdSize = checkCommand(receiveBuffer, receiveIndex, cmdBuffer)) > 0 )
			{
				cmd_proc(cmdBuffer, cmdSize);
				receiveIndex = 0;
			}
		}
	}

	close(fdUart);
	return NULL;
}

// ******************************************
// send data to RF device via UART port
// 
// Paramaters
//    cmd: the command which defined at rfProc.h
//    param: paramaters
//
// Return value
//    0: success
//    others: fail
// ******************************************
int rfSend(RF_COMMANDS cmd, int param)
{
	unsigned char buf[64];
    int size = 0;

	switch( cmd )
	{
		case COMMAND_LIGHT_ON:
			printf("[RF] ready to send light on command\r\n");
			size = makeCmdLightCtrl(buf, 1);
			break;

		case COMMAND_LIGHT_OFF:
			printf("[RF] ready to send light off command\r\n");
			size = makeCmdLightCtrl(buf, 0);
			break;

		case COMMAND_TEMP_SET:
			printf("[RF] ready to set temperature to %d degree\r\n", param);
			size = makeCmdSetTemp(buf, param);
			break;
		
		case COMMAND_FANS_CTRL:
			printf("[RF] Fans control\r\n");
			size = makeCmdSetFan(buf, param);
			break;

		case COMMAND_TEMP_DISP:
			gDisplayTemp = param;
			break;
		default:
			break;
	}

	if( size > 0 )
	{
		printf("size=%d\r\n", size);
		write(fdUart, buf, size);
	}

	return 0;
}

void rfStop(void)
{
}

void rfSetCallback(RF_CALLBACK func)
{
	rfCallback = func;
}

// end of file

