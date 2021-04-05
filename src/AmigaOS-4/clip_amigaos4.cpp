/*
 *  clip_amiga.cpp - Clipboard handling, AmigaOS implementation
 *
 *  Basilisk II (C) 1997-2001 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdint.h>
#include <stdbool.h>

#include <libraries/iffparse.h>
#include <devices/clipboard.h>
#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/dos.h>


#include "sysdeps.h"
#include "cpu_emulation.h"
#include "emul_op.h"
#include "main.h"


#include "clip.h"
#include "prefs.h"

#define DEBUG 0
#include "debug.h"


// Global variables


class ClipBox
{
	public:

		struct IFFHandle *iff;
		struct ClipboardHandle *ch;
		bool clipboard_open ;

		ClipBox()
		{
			iff = NULL;
			ch = NULL;
			clipboard_open = false;

			// Create clipboard IFF handle
			iff = AllocIFF();
			if (iff)
			{
				ch = OpenClipboard(PRIMARY_CLIP);
				if (ch)
				{
					iff->iff_Stream = (ULONG)ch;
					InitIFFasClip(iff);
					clipboard_open = true;
				}
			}
			
		}

		~ClipBox()
		{
			// free it only if its open.
			if (ch) CloseClipboard(ch);
			if (iff) FreeIFF(iff);

			// make sure, its not freed twice.
			ch = NULL;
			iff = NULL;
		}
};

class ByteArray
{
	public:

		ByteArray() 
			{
				printf("ByteArray init\n");
				data = NULL; _size = 0;
			 }

		~ByteArray() 
			{
				printf("ByteArray free data %08x\n",data);
				 if (data) free(data);
			 }

		int size() 
			{ 
				printf("ByteArray get Size\n");
				return _size; 
			}

		uint8& operator[](int idx)
			{
				char c = data[ idx ];
				if (c<15) c='.';

				printf("ByteArray get %d of %d -- %c\n",idx,_size, c);
				return data[idx];
			 }

	uint8 *data;
	int _size;
};


static bool no_clip_conversion;


/*
 *  Initialization
 */

void ClipInit(void)
{
}


/*
 *  Deinitialization
 */

void ClipExit(void)
{
}


/*
 *  Mac application wrote to clipboard
 */


void PutScrap(uint32 type, void *scrap, int32 length)
{
	ClipBox cb;

	D(bug("PutScrap type %08lx, data %08lx, length %ld\n", type, scrap, length));
	if (length <= 0 || !cb.clipboard_open)
		return;

	switch (type)
	{
/*
		case 'styl':
		{
			dump_str( (char *) scrap, length );
		}
		break;
*/		

		case 'TEXT':
		{
			D(bug(" clipping TEXT\n"));

			// Open IFF stream
			if (OpenIFF(cb.iff, IFFF_WRITE))
				break;

			// Convert text from Mac charset to ISO-Latin1
			uint8 *buf = (uint8 *) AllocVec (length, MEMF_SHARED| MEMF_CLEAR);

			if (buf)
			{
				uint8 *p = (uint8 *)scrap;
				uint8 *q = buf;

				for (int i=0; i<length; i++)
				{
					uint8 c = *p++;
					if (c == 13) c= 10;	// CR -> LF
					*q++ = c;
				}

				// Write text
				if (!PushChunk(cb.iff, MAKE_ID('F','T','X','T'), ID_FORM, IFFSIZE_UNKNOWN))
				{
					if (!PushChunk(cb.iff, 0, MAKE_ID('C','H','R','S'), IFFSIZE_UNKNOWN))
					{
						WriteChunkBytes(cb.iff, buf, length);
						PopChunk(cb.iff);
					}
					PopChunk(cb.iff);
				}

				// Close IFF stream
				CloseIFF(cb.iff);
				FreeVec(buf);
			}
		}
		break;
	}
}

class Dev
{
	public:

		struct MsgPort *mp ;
		struct IOClipReq *io;
		bool open ;

		void init()
		{
			mp = NULL;
			io = NULL;
			open = false;
		}

		Dev(int unit)
		{
			init();
	
			mp = (MsgPort *) AllocSysObjectTags(ASOT_PORT, TAG_END);

			io = (struct IOClipReq *) AllocSysObjectTags( ASOT_IOREQUEST, 
					ASOIOR_Size, sizeof(struct IOClipReq) ,
					ASOIOR_ReplyPort, mp,
					TAG_END);

			if (! OpenDevice("clipboard.device", unit, (IORequest*) io, 0))
			{
				open = true;
			}
		}

		~Dev()
		{
			if (open)
			{
				CloseDevice( (IORequest*) io);
				open = false;
			}

			if (io) FreeSysObject( ASOT_IOREQUEST, io );
			if (mp) FreeSysObject( ASOT_PORT, mp );
		}

		void initIO()
		{
			io->io_Offset = 0;
			io->io_Error = 0;
			io->io_ClipID = 0;
		}

		int QuaryFTXT()		// returns length if true :-)
		{
			uint32_t cbuff[4];	// ID_FROM, size, FTEXT, CHARS
			initIO();
			io -> io_Command = CMD_READ;
			io -> io_Data = (STRPTR) cbuff;
			io -> io_Length = 16;
			DoIO( (IORequest*) io );
			
			if (io-> io_Actual == 16)
			{
				if (cbuff[0] == ID_FORM )
				{
					if (cbuff[2] == MAKE_ID('F','T','X','T')) return cbuff[1] - 12;
				}
			}
			return 0;
		}

		bool read(uint8 *ptr,int len)
		{
			io -> io_Command = CMD_READ;
			io -> io_Data = (STRPTR) ptr;
			io -> io_Length = len;
			DoIO( (IORequest*) io );
			
			if (io-> io_Actual == len) return true;
			return false;
		}
};


bool get_amiga_clip(ByteArray &data)
{
	Dev clip(0) ;
	int len;

	len = clip.QuaryFTXT();

	if (len)
	{
		if (data.data) free(data.data);

		data.data = (uint8*) malloc(len);

		if (data.data) 
		{
			data._size = clip.read( data.data, len ) ? len : 0;
		}
	}

	return data._size ? true: false;
}



void give2mac(ByteArray &data, uint32 type)
{
	// Allocate space for new scrap in MacOS side
	M68kRegisters r;
	r.d[0] = data.size();
	Execute68kTrap(0xa71e, &r);			// NewPtrSysClear()
	uint32 scrap_area = r.a[0];

	if (scrap_area == 0) return;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	
	int size = data.size();
	uint8 *p = Mac2HostAddr(scrap_area);
	for (int i = 0; i < size; i++) 
	{
		uint8 c = data[i];

		if (c == 10) c = 13; 	// LF -> CR
		*p++ = c;
	}

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	// Add new data to clipboard
	static uint8 proc[] = {
			0x59, 0x8f,					// subq.l	#4,sp
			0xa9, 0xfc,					// ZeroScrap()
			0x2f, 0x3c, 0, 0, 0, 0,		// move.l	#length,-(sp)
			0x2f, 0x3c, 0, 0, 0, 0,		// move.l	#type,-(sp)
			0x2f, 0x3c, 0, 0, 0, 0,		// move.l	#outbuf,-(sp)
			0xa9, 0xfe,					// PutScrap()
			0x58, 0x8f,					// addq.l	#4,sp
			M68K_RTS >> 8, M68K_RTS & 0xff
		};

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	r.d[0] = sizeof(proc);
	Execute68kTrap(0xa71e, &r);		// NewPtrSysClear()
	uint32 proc_area = r.a[0];

	if (proc_area)
	{
		printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		// The procedure is run-time generated because it must lays in
		// Mac address space. This is mandatory for "33-bit" address
		// space optimization on 64-bit platforms because the static
		// proc[] array is not remapped

		Host2Mac_memcpy(proc_area, proc, sizeof(proc));

		WriteMacInt32(proc_area +  6, data.size());
		WriteMacInt32(proc_area + 12, type);
		WriteMacInt32(proc_area + 18, scrap_area);

//		we_put_this_data = true;

		Execute68k(proc_area, &r);		// <--------------------------------- gets stuck here...

		printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		// We are done with scratch memory
		r.a[0] = proc_area;
		Execute68kTrap(0xa01f, &r);		// DisposePtr
	}

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	r.a[0] = scrap_area;
	Execute68kTrap(0xa01f, &r);		// DisposePtr

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
}


void GetScrap(void **handle, uint32 type, int32 offset)
{
	ByteArray data;

	D(bug("GetScrap handle %p, type %08x, offset %d\n", handle, type, offset));

	if ( get_amiga_clip( data) == false) return;

	switch (type) 
	{
		case 'TEXT':

			int size = data.size();
//			give2mac( data, type );
			break;
	}
}


