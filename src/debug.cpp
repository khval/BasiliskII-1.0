
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define DEBUG 1
#include <debug.h>


static void printAsc(char *ptr, int size)
{
	char ch;
	char *buffer;
	char *dptr;

	if (size <= 0) return ;

	buffer =  (char *) alloca( size + 1);		// alloc on stack...
	dptr = buffer;
	while (size--)
	{
		ch = *ptr ++;
		if ((ch >='0') && (ch<='9')
		|| (ch >='a') && (ch<='z')
		|| (ch >='A') && (ch<='Z'))
		{
			*dptr++ = ch;
		}
		else *dptr++='.';
	}
	*dptr = 0;
	printf("%s",buffer);
}

static void printHex(char *ptr, int size)
{
	if (size <= 0) return ;

	while (size--) printf("%02X ", *ptr++ );	
}

void hexDump(char *start, int size)
{
	uint32_t offset = 0;
	int bytesPerLine = 20;
	int showBytes;
	int lines,l,n;

	lines = size / bytesPerLine;
	lines +=  size % bytesPerLine ? 1 : 0;

	for (l=0;l<lines;l++)
	{
		offset = l * bytesPerLine;
		
#ifdef __amigaos4__
		printf("%c[1m%08X:%c[0m ",0x1B,offset,0x1B);
#else
		printf("%08X: ",offset);
#endif

		showBytes = size - offset;
		showBytes = showBytes > bytesPerLine ? bytesPerLine : showBytes;

		printAsc( start + offset, showBytes );
		for (n = showBytes ; n<bytesPerLine;n++) printf(" ");

		printf(" ");
		printHex( start + offset, showBytes );
		printf("\n");

	}
}

