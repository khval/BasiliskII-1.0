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
#include "deviceClip.h"

#define DEBUG 0
#include "debug.h"

// Global variables

static bool we_put_this_data = false;

void do_putscrap(uint32 type, void *scrap, int32 length);

class ByteArray
{
	public:

		ByteArray() 
			{	data = NULL; _size = 0;		 }

		~ByteArray() 
			{	 if (data) free(data);		 }

		int size() 
			{ 	return _size; 		}

		uint8& operator[](int idx)
			{
				char c = data[ idx ];
				if (c<15) c='.';
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
	D(bug("PutScrap type %08lx, data %p, length %ld\n", type, scrap, length));
	if (we_put_this_data) {
		we_put_this_data = false;
		return;
	}
	if (length <= 0)
		return;

//	XDisplayLock();
	do_putscrap(type, scrap, length);
//	XDisplayUnlock();
}


void do_putscrap(uint32 type, void *scrap, int32 length)
{
	switch (type)
	{

		case 'TEXT':
		{
			D(bug(" clipping TEXT\n"));

			// Convert text from Mac charset to ISO-Latin1
			uint8 *buf = (uint8 *) AllocVec (length, MEMF_SHARED| MEMF_CLEAR);

			if (buf)
			{
				DeviceClip clip(0);

				uint8 *p = (uint8 *)scrap;
				uint8 *q = buf;

				for (int i=0; i<length; i++)
				{
					uint8 c = *p++;
					if (c == 13) c= 10;	// CR -> LF
					*q++ = c;
				}

				clip.WriteFTXT( (char *) buf, length);
				FreeVec(buf);
			}
		}
		break;

		case 'styl':
		 {
			D(bug(" clipping styl\n"));
			uint16 *p = (uint16 *)scrap;
			uint16 n = (*p++);
			D(bug(" %d styles (%d bytes)\n", n, length));
			for (int i = 0; i < n; i++)
			 {
				uint32 offset = (*(uint32 *)p); p += 2;
				uint16 line_height = (*p++);
				uint16 font_ascent = (*p++);
				uint16 font_family = (*p++);
				uint16 style_code = (*p++);
				uint16 char_size = (*p++);
				uint16 r = (*p++);
				uint16 g = (*p++);
				uint16 b = (*p++);

				D(bug("  offset=%d, height=%d, font ascent=%d, id=%d, style=%x, size=%d, RGB=%x/%x/%x\n",
					  offset, line_height, font_ascent, font_family, style_code, char_size, r, g, b));
			}
		}
		break;
	}

}

bool get_amiga_clip(ByteArray &data)
{
	DeviceClip clip(0) ;
	int len;

	len = clip.QuaryFTXT();

	if (len)
	{
		if (data.data) free(data.data);
		data.data = (uint8*) malloc(len);
		if (data.data) data._size = clip.read( data.data, len ) ? len : 0;		
	}
	clip.readDone();

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

	int size = data.size();
	uint8 *p = Mac2HostAddr(scrap_area);
	for (int i = 0; i < size; i++) 
	{
		uint8 c = data[i];

		if (c == 10) c = 13; 	// LF -> CR
		*p++ = c;
	}

	// Add new data to clipboard
	static uint8 proc[] = {
			0x59, 0x8f,					// subq.l	#4,sp
			0xa9, 0xfc,					// ZeroScrap()
			0x2f, 0x3c, 0, 0, 0, 0,		// move.l	#length,-(sp)
			0x2f, 0x3c, 0, 0, 0, 0,		// move.l	#type,-(sp)
			0x2f, 0x3c, 0, 0, 0, 0,		// move.l	#outbuf,-(sp)
			0xa9, 0xfe,					// PutScrap()  <------------ we do this....
			0x58, 0x8f,					// addq.l	#4,sp
			M68K_RTS >> 8, M68K_RTS & 0xff
		};

	r.d[0] = sizeof(proc);
	Execute68kTrap(0xa71e, &r);		// NewPtrSysClear()
	uint32 proc_area = r.a[0];

	if (proc_area)
	{
		// The procedure is run-time generated because it must lays in
		// Mac address space. This is mandatory for "33-bit" address
		// space optimization on 64-bit platforms because the static
		// proc[] array is not remapped

		Host2Mac_memcpy(proc_area, proc, sizeof(proc));

		WriteMacInt32(proc_area +  6, data.size());
		WriteMacInt32(proc_area + 12, type);
		WriteMacInt32(proc_area + 18, scrap_area);

		we_put_this_data = true;
/*
		dump_68k( 
			(char *) Mac2HostAddr( (ULONG) proc_area), 
			(char *) Mac2HostAddr( (ULONG) proc_area + sizeof(proc)) );
*/

		Execute68k(proc_area, &r);

		// We are done with scratch memory
		r.a[0] = proc_area;
		Execute68kTrap(0xa01f, &r);		// DisposePtr
	}

	r.a[0] = scrap_area;
	Execute68kTrap(0xa01f, &r);		// DisposePtr
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
			give2mac( data, type );
			break;
	}
}


