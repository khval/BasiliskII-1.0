
// deviceClip.cpp.
//
// is written by Kjetil Hvalstrand, (C) 2021,
// this code snippet is under MIT license

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <libraries/iffparse.h>
#include <devices/clipboard.h>
#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/dos.h>

#include "deviceClip.h"

#define ID_FORM MAKE_ID('F','O','R','M')
#define ID_FTXT MAKE_ID('F','T','X','T')
#define ID_CHRS MAKE_ID('C','H','R','S')

void DeviceClip::Init()
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	mp = NULL;
	io = NULL;
	open = false;
}

DeviceClip::DeviceClip(int unit)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	Init();
	
	mp = (MsgPort *) AllocSysObjectTags(ASOT_PORT, TAG_END);

	io = (struct IOClipReq *) AllocSysObjectTags( ASOT_IOREQUEST, 
			ASOIOR_Size, sizeof(struct IOClipReq) ,
			ASOIOR_ReplyPort, mp,
			TAG_END);

	if (! OpenDevice("clipboard.device", unit, (IORequest*) io, 0))
	{
		printf("****Devices is open\n");
		open = true;
	}
}

DeviceClip::~DeviceClip()
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	if (open)
	{
		printf("****Closing device\n");

		CloseDevice( (IORequest*) io);
		open = false;
	}

	if (io) FreeSysObject( ASOT_IOREQUEST, io );
	if (mp) FreeSysObject( ASOT_PORT, mp );
}

void DeviceClip::initIO()
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	io->io_Offset = 0;
	io->io_Error = 0;
	io->io_ClipID = 0;
}

int DeviceClip::WriteFTXT(char *txt)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return WriteFTXT(txt, strlen(txt));
}

int DeviceClip::WriteFTXT(char *txt, int slen)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	int odd;
	int  length;
	uint32_t cbuff[5] = {ID_FORM, 0, ID_FTXT, ID_CHRS, 0};	

	odd = slen & 1;

	length = slen + odd + 12;
	cbuff[1] = length;
	cbuff[4] = slen;
			
	initIO();
	write( (uint8 *) cbuff, 20 );
	write( (uint8 *) txt, cbuff[1] ); 
	if (odd) write( (uint8 *) "", 1 ); 
	writeDone();
}


int DeviceClip::QuaryFTXT()		// returns length if true :-)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	uint32_t cbuff[5];	// ID_FROM, size, FTEXT, CHARS, len
	initIO();

	if (read( (uint8 *) cbuff, 20 ))
	{
		if (cbuff[0] == ID_FORM )
		{
			if (cbuff[2] == MAKE_ID('F','T','X','T')) return cbuff[4] ;	// header is only (ID_FROM, size, FTEXT)
		}
	}
	return 0;
}

bool  DeviceClip::writeDone()
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	io -> io_Command = CMD_UPDATE;
	io -> io_Data = NULL;
	io -> io_Length = 1;
	DoIO( (IORequest*) io );
			
	return io -> io_Error ? false : true ;
}

bool DeviceClip::write(uint8 * ptr, int len)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	io -> io_Command = CMD_WRITE;
	io -> io_Data = (STRPTR) ptr;
	io -> io_Length = len;
	DoIO( (IORequest*) io );
			
	if (io-> io_Actual == len) return true;
	return false;
}

bool DeviceClip::read(uint8 *ptr,int len)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	io -> io_Command = CMD_READ;
	io -> io_Data = (STRPTR) ptr;
	io -> io_Length = len;
	DoIO( (IORequest*) io );
			
	if (io-> io_Actual == len) return true;
	return false;
}

void DeviceClip::readDone()	// read until everything is read.
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	uint8 buffer[256];
	while (io -> io_Actual) read( buffer,  256);
}


