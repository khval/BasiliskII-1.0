
/*
 *  sys_amigaos4.cpp - System dependent routines, AmigaOS4 implementation
 *                    Based on MorphOS implementation
*
 *  sys_morphos.cpp - System dependent routines, MorphOS implementation
 *                    Based on Amiga implementation
 *
 *  Basilisk II (C) 1997-2001 Christian Bauer
 *                  2005-2006 Ilkka Lehtoranta
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

#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <devices/scsidisk.h>
#include <resources/disk.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "sysdeps.h"
#include "main.h"
#include "macos_util.h"
#include "prefs.h"
#include "user_strings.h"
#include "sys.h"
#include "amiga_rdb.i"

#define DEBUG 0
#include "debug.h"


// File handles are pointers to these structures
struct file_handle
{
	bool is_file;			// Flag: plain file or /dev/something?
	bool read_only;		// Copy of Sys_open() flag
	UQUAD start_byte;		// Size of file header (if any)
	UQUAD size;			   // Size of file/device (minus header)

	BPTR f;					// AmigaDOS file handle (if is_file == true)

	struct IOStdReq *io;	// Pointer to IORequest (if is_file == false)
	ULONG block_size;		// Block size of device (must be a power of two)
	bool is_ejected;		// Volume has been (logically) ejected
};


// Message port for device communication
static struct MsgPort *the_port = NULL;

// Temporary buffer
static UBYTE tmp_buf[0x10000];


/*
 *  Initialization
 */

void SysInit(void)
{
	// Create port and temporary buffer
	the_port = CreateMsgPort();
	if (the_port == NULL)
	{
		ErrorAlert(GetString(STR_NO_MEM_ERR));
		QuitEmulator();
	}
}


/*
 *  Deinitialization
 */

void SysExit(void)
{
	// Delete port and temporary buffer
	if (the_port) {
		DeleteMsgPort(the_port);
		the_port = NULL;
	}
}


/*
 *  This gets called when no "floppy" prefs items are found
 *  It scans for available floppy drives and adds appropriate prefs items
 */

void SysAddFloppyPrefs(void)
{
}


/*
 *  This gets called when no "disk" prefs items are found
 *  It scans for available HFS volumes and adds appropriate prefs items
 */

void SysAddDiskPrefs(void)
{
	// AmigaOS doesn't support MacOS partitioning, so this probably doesn't make much sense...
}


/*
 *  This gets called when no "cdrom" prefs items are found
 *  It scans for available CD-ROM drives and adds appropriate prefs items
 */

void SysAddCDROMPrefs(void)
{
	// Don't scan for drives if nocdrom option given
#if 0
	if (PrefsFindBool("nocdrom"))
		return;
#endif

	//!!
}


/*
 *  Add default serial prefs (must be added, even if no ports present)
 */

void SysAddSerialPrefs(void)
{
	PrefsAddString("seriala", "serial.device/0");
	PrefsAddString("serialb", "*parallel.device/0");
}


/*
 *  Open file/device, create new file handle (returns NULL on error)
 *
 *  Format for device names: /dev/<name>/<unit>/<open flags>/<start block>/<size (blocks)>/<block size>
 *  new format: /dev/<name>/<unit>/<vol_devicename>
 *
 */

struct initSysOpen
{
	struct ExamineData *Edata;
	BPTR fd;
	uint64_t size;

	initSysOpen();

	void errorFreeAll( const char *error );
};

initSysOpen::initSysOpen()
{
	Edata = NULL;
	fd = 0;
	size = 0;
}

void initSysOpen::errorFreeAll( const char *error)
{
	printf("Error: %s\n",error);

	if (Edata) FreeDosObject(DOS_EXAMINEDATA,Edata);
	Edata = NULL;
	if (fd) Close(fd);
	fd = 0;
}

void *Sys_open(const char *name, bool read_only)
{
	bool is_file = (strstr(name, "/dev/") != name);

//	D(bug("Sys_open(%s, %s)\n", name, read_only ? "read-only" : "read/write"));

	printf("Sys_open(%s, %s)\n", name, read_only ? "read-only" : "read/write");

	// File or device?
	if (is_file)
	{
		struct initSysOpen i;

		i.Edata = ExamineObjectTags(EX_StringNameInput,name,TAG_END);

		if ( ! i.Edata )
		{
			i.errorFreeAll("Failed no Examine object");
			return NULL;
		}
		

		// File, open it and get stats
		 i.fd = Open((char *)name, MODE_OLDFILE);
		if (!i.fd)
		{
			i.errorFreeAll("no file lock\n");
			return NULL;
		}

//		Edata = ExamineObjectTags( EX_LockInput, f, TAG_DONE );

		if (i.Edata)
		{
			printf("got Examine object\n");

			if ( i.Edata -> Protection & EXDF_NO_WRITE ) read_only = true;
			i.size = i.Edata -> FileSize;

			printf("size: %lld\n", i.size);

			if ((i.size == ~0) || (i.size == 0))
			{
				i.errorFreeAll("Sorry bad size");
				return NULL;
			}
			
			FreeDosObject(DOS_EXAMINEDATA,i.Edata);
		}

		// Create file_handle
		file_handle *fh = new file_handle;
		fh->f = i.fd;
		fh->is_file = true;
		fh->read_only = read_only;

		// Detect disk image file layout
		ChangeFilePosition(fh->f, 0, OFFSET_BEGINNING);
		Read(fh->f, &tmp_buf, 256);
		FileDiskLayout(i.size, tmp_buf, fh->start_byte, fh->size);
		return fh;

	} else {

		// Device, parse string
		char dev_name[256];
		char vol_devicename[256];
		ULONG dev_unit = 0, dev_flags = 0, dev_start = 0, dev_size = 16, dev_bsize = 512;

		if (sscanf(name, "/dev/%[^/]/%ld/%[^/]", dev_name, &dev_unit, vol_devicename) < 2)
			return NULL;

		if (!get_blocks(dev_name,dev_unit,vol_devicename,&dev_start,&dev_size,&dev_bsize)) 
		{
			if ((vol_devicename[0]>='0')&&(vol_devicename[0]<='9'))
			{	
				if (sscanf(name, "/dev/%[^/]/%ld/%ld/%ld/%ld/%ld", 
					dev_name, &dev_unit, &dev_flags, &dev_start, &dev_size, &dev_bsize) < 2)
				{
					return NULL;
				}
			}
			else 	return NULL;
		}

		// Create IORequest
		struct IOStdReq *io = (struct IOStdReq *)CreateIORequest(the_port, sizeof(struct IOExtTD));
		if (io == NULL)
			return NULL;

		// Open device
		if (OpenDevice(dev_name, dev_unit, (struct IORequest *)io, dev_flags)) {
			D(bug(" couldn't open device\n"));
			DeleteIORequest((IORequest*) io);
			return NULL;
		}

		// Create file_handle
		file_handle *fh = new file_handle;
		fh->io = io;
		fh->is_file = false;
		fh->read_only = read_only;
		fh->start_byte = (UQUAD)dev_start * dev_bsize;
		fh->size = (UQUAD)dev_size * dev_bsize;
		fh->block_size = dev_bsize;
		fh->is_ejected = false;
		return fh;
	}
}


/*
 *  Close file/device, delete file handle
 */

void Sys_close(void *arg)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return;

	D(bug("Sys_close(%08lx)\n", arg));

	// File or device?
	if (fh->is_file) {

		// File, simply close it
		Close(fh->f);
		fh -> f = NULL;

	} else {

		// Device, close it and delete IORequest
		fh->io->io_Command = CMD_UPDATE;
		DoIO((struct IORequest *)fh->io);

		fh->io->io_Command = TD_MOTOR;
		fh->io->io_Length = 0;
		DoIO((struct IORequest *)fh->io);
		CloseDevice((struct IORequest *)fh->io);
		DeleteIORequest((IORequest*) fh->io);

		fh -> io == NULL;
	}
	delete fh;
}


/*
 *  Send one I/O request using 64-bit addressing
 */

static UQUAD send_io_request(file_handle *fh, bool writing, ULONG length, UQUAD offset, APTR data)
{
//	printf("%s offset %lld\n",writing ? "Write64" : "Read64", offset );

	fh->io->io_Command = writing ? NSCMD_TD_WRITE64 : NSCMD_TD_READ64;
	fh->io->io_Actual = offset >> 32;
	fh->io->io_Length = length;
	fh->io->io_Offset = offset;
	fh->io->io_Data = data;

	if (DoIO((struct IORequest *)fh->io) || fh->io->io_Actual != length)
		return 0;
	return fh->io->io_Actual;
}


/*
 *  Read "length" bytes from file/device, starting at "offset", to "buffer",
 *  returns number of bytes read (or 0)
 */

size_t Sys_read(void *arg, void *buffer, loff_t offset, size_t length)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return 0;

	// File or device?
	if (fh->is_file) {

		// File, seek to position
		if (ChangeFilePosition(fh->f, offset + fh->start_byte, OFFSET_BEGINNING) == 0)
			return 0;

		// Read data
		LONG actual = Read(fh->f, buffer, length);
		if (actual == -1)
			return 0;
		else
			return actual;

	} else {

		// Device, pre-read (partial read of first block) necessary?
		UQUAD pos = offset + fh->start_byte;
		size_t actual = 0;
		uint32 pre_offset = pos % fh->block_size;

		if (pre_offset) {

			// Yes, read one block
			if (send_io_request(fh, false, fh->block_size, pos - pre_offset, &tmp_buf) == 0)
				return 0;

			// Copy data to destination buffer
			size_t pre_length = fh->block_size - pre_offset;

			if (pre_length > length) pre_length = length;

			memcpy(buffer, ((char *) &tmp_buf) + pre_offset, pre_length);

			// Adjust data pointers
			buffer = ((char *) buffer) + pre_length;
			pos += pre_length;
			length -= pre_length;
			actual += pre_length;
		}

		// Main read (complete reads of middle blocks) possible?
		if (length >= fh->block_size) {

			// Yes, read blocks
			size_t main_length = length & ~(fh->block_size - 1);
			if (send_io_request(fh, false, main_length, pos, buffer) == 0)
				return 0;

			// Adjust data pointers
			buffer = (uint8 *)buffer + main_length;
			pos += main_length;
			length -= main_length;
			actual += main_length;
		}

		// Post-read (partial read of last block) necessary?
		if (length) {

			// Yes, read one block
			if (send_io_request(fh, false, fh->block_size, pos, &tmp_buf) == 0)
				return 0;

			// Copy data to destination buffer
			memcpy(buffer, &tmp_buf, length);
			actual += length;
		}

		return actual;
	}
}


/*
 *  Write "length" bytes from "buffer" to file/device, starting at "offset",
 *  returns number of bytes written (or 0)
 */

size_t Sys_write(void *arg, void *buffer, loff_t offset, size_t length)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return 0;

	// File or device?
	if (fh->is_file) {

		// File, seek to position if necessary
		if (ChangeFilePosition(fh->f, offset + fh->start_byte, OFFSET_BEGINNING) == 0)
			return 0;

		// Write data
		LONG actual = Write(fh->f, buffer, length);
		if (actual == -1)
			return 0;
		else
			return actual;

	} else {

		// Device, pre-write (partial write of first block) necessary
		UQUAD pos = offset + fh->start_byte;
		size_t actual = 0;
		uint32 pre_offset = pos % fh->block_size;
		if (pre_offset) {

			// Yes, read one block
			if (send_io_request(fh, false, fh->block_size, pos - pre_offset, &tmp_buf) == 0)
				return 0;

			// Copy data from source buffer
			size_t pre_length = fh->block_size - pre_offset;
			if (pre_length > length)
				pre_length = length;
			memcpy( ((char *) &tmp_buf) + pre_offset, buffer, pre_length);

			// Write block back
			if (send_io_request(fh, true, fh->block_size, pos - pre_offset, &tmp_buf) == 0)
				return 0;

			// Adjust data pointers
			buffer = (uint8 *)buffer + pre_length;
			pos += pre_length;
			length -= pre_length;
			actual += pre_length;
		}

		// Main write (complete writes of middle blocks) possible?
		if (length >= fh->block_size) {

			// Yes, write blocks
			size_t main_length = length & ~(fh->block_size - 1);
			if (send_io_request(fh, true, main_length, pos, buffer) == 0)
				return 0;

			// Adjust data pointers
			buffer = (uint8 *)buffer + main_length;
			pos += main_length;
			length -= main_length;
			actual += main_length;
		}

		// Post-write (partial write of last block) necessary?
		if (length) {

			// Yes, read one block
			if (send_io_request(fh, false, fh->block_size, pos, &tmp_buf) == 0)
				return 0;

			// Copy data from source buffer
			memcpy(buffer, &tmp_buf, length);

			// Write block back
			if (send_io_request(fh, true, fh->block_size, pos, &tmp_buf) == 0)
				return 0;
			actual += length;
		}

		return actual;
	}
}


/*
 *  Return size of file/device (minus header)
 */

loff_t SysGetFileSize(void *arg)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return true;

	return fh->size;
}


/*
 *  Eject volume (if applicable)
 */

void SysEject(void *arg)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return;

	if (!fh->is_file) {

		// Flush buffer, turn off the drive motor and eject volume
		fh->io->io_Command = CMD_UPDATE;
		DoIO((struct IORequest *)fh->io);

		fh->io->io_Command = TD_MOTOR;
		fh->io->io_Length = 0;
		DoIO((struct IORequest *)fh->io);

		fh->io->io_Command = TD_EJECT;
		fh->io->io_Length = 1;
		DoIO((struct IORequest *)fh->io);

		fh->is_ejected = true;
	}
}


/*
 *  Format volume (if applicable)
 */

bool SysFormat(void *arg)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return false;

	//!!
	return true;
}


/*
 *  Check if file/device is read-only (this includes the read-only flag on Sys_open())
 */

bool SysIsReadOnly(void *arg)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return true;

	if (fh->is_file) {

		// File, return flag given to Sys_open
		return fh->read_only;

	} else {

		// Device, check write protection
		fh->io->io_Command = TD_PROTSTATUS;
		DoIO((struct IORequest *)fh->io);
		if (fh->io->io_Actual)
			return true;
		else
			return fh->read_only;
	}
}


/*
 *  Check if the given file handle refers to a fixed or a removable disk
 */

bool SysIsFixedDisk(void *arg)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return true;

	return true;
}


/*
 *  Check if a disk is inserted in the drive (always true for files)
 */

bool SysIsDiskInserted(void *arg)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return false;

	if (fh->is_file)
		return true;
	else {

		// Check medium status
		fh->io->io_Command = TD_CHANGESTATE;
		fh->io->io_Actual = 0;
		DoIO((struct IORequest *)fh->io);
		bool inserted = (fh->io->io_Actual == 0);

		if (!inserted) {
			// Disk was ejected and has now been taken out
			fh->is_ejected = false;
		}

		if (fh->is_ejected) {
			// Disk was ejected but has not yet been taken out, report it as
			// no longer in the drive
			return false;
		} else
			return inserted;
	}
}


/*
 *  Prevent medium removal (if applicable)
 */

void SysPreventRemoval(void *arg)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return;

	if (!fh->is_file) {

		// Send PREVENT ALLOW MEDIUM REMOVAL SCSI command
		struct SCSICmd scsi;
		static const UBYTE the_cmd[6] = {0x1e, 0, 0, 0, 1, 0};
		scsi.scsi_Length = 0;
		scsi.scsi_Command = (UBYTE *)the_cmd;
		scsi.scsi_CmdLength = 6;
		scsi.scsi_Flags = SCSIF_READ;
		scsi.scsi_Status = 0;
		fh->io->io_Data = &scsi;
		fh->io->io_Length = sizeof(scsi);
		fh->io->io_Command = HD_SCSICMD;
		DoIO((struct IORequest *)fh->io);
	}
}


/*
 *  Allow medium removal (if applicable)
 */

void SysAllowRemoval(void *arg)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return;

	if (!fh->is_file) {

		// Send PREVENT ALLOW MEDIUM REMOVAL SCSI command
		struct SCSICmd scsi;
		static const UBYTE the_cmd[6] = {0x1e, 0, 0, 0, 0, 0};
		scsi.scsi_Length = 0;
		scsi.scsi_Command = (UBYTE *)the_cmd;
		scsi.scsi_CmdLength = 6;
		scsi.scsi_Flags = SCSIF_READ;
		scsi.scsi_Status = 0;
		fh->io->io_Data = &scsi;
		fh->io->io_Length = sizeof(scsi);
		fh->io->io_Command = HD_SCSICMD;
		DoIO((struct IORequest *)fh->io);
	}
}


/*
 *  Read CD-ROM TOC (binary MSF format, 804 bytes max.)
 */

bool SysCDReadTOC(void *arg, uint8 *toc)
{
	file_handle *fh = (file_handle *)arg;

	if (fh && !fh->is_file)
	{
		// Send READ TOC MSF SCSI command
		struct SCSICmd scsi;
		static const UBYTE read_toc_cmd[10] = {0x43, 0x02, 0, 0, 0, 0, 0, 0x03, 0x24, 0};
		scsi.scsi_Data = (UWORD *)&tmp_buf;
		scsi.scsi_Length = 804;
		scsi.scsi_Command = (UBYTE *)read_toc_cmd;
		scsi.scsi_CmdLength = 10;
		scsi.scsi_Flags = SCSIF_READ;
		scsi.scsi_Status = 0;
		fh->io->io_Data = &scsi;
		fh->io->io_Length = sizeof(scsi);
		fh->io->io_Command = HD_SCSICMD;

		if (!DoIO((struct IORequest *)fh->io) && !scsi.scsi_Status)
		{
			CopyMem(tmp_buf, toc, 804);
			return true;
		}
	}

	return false;
}


/*
 *  Read CD-ROM position data (Sub-Q Channel, 16 bytes, see SCSI standard)
 */

bool SysCDGetPosition(void *arg, uint8 *pos)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return false;

	if (fh->is_file)
		return false;
	else {

		// Send READ SUB-CHANNEL SCSI command
		struct SCSICmd scsi;
		static const UBYTE read_subq_cmd[10] = {0x42, 0x02, 0x40, 0x01, 0, 0, 0, 0, 0x10, 0};
		scsi.scsi_Data = (UWORD *)&tmp_buf;
		scsi.scsi_Length = 16;
		scsi.scsi_Command = (UBYTE *)read_subq_cmd;
		scsi.scsi_CmdLength = 10;
		scsi.scsi_Flags = SCSIF_READ;
		scsi.scsi_Status = 0;
		fh->io->io_Data = &scsi;
		fh->io->io_Length = sizeof(scsi);
		fh->io->io_Command = HD_SCSICMD;
		if (DoIO((struct IORequest *)fh->io) || scsi.scsi_Status)
			return false;
		memcpy(pos, &tmp_buf, 16);
		return true;
	}
}


/*
 *  Play CD audio
 */

bool SysCDPlay(void *arg, uint8 start_m, uint8 start_s, uint8 start_f, uint8 end_m, uint8 end_s, uint8 end_f)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return false;

	if (fh->is_file)
		return false;
	else {

		// Send PLAY AUDIO MSF SCSI command
		struct SCSICmd scsi;
		UBYTE play_cmd[10] = {0x47, 0, 0, start_m, start_s, start_f, end_m, end_s, end_f, 0};
		scsi.scsi_Data = (UWORD *)&tmp_buf;
		scsi.scsi_Length = 0;
		scsi.scsi_Command = play_cmd;
		scsi.scsi_CmdLength = 10;
		scsi.scsi_Flags = SCSIF_READ;
		scsi.scsi_Status = 0;
		fh->io->io_Data = &scsi;
		fh->io->io_Length = sizeof(scsi);
		fh->io->io_Command = HD_SCSICMD;
		if (DoIO((struct IORequest *)fh->io) || scsi.scsi_Status)
			return false;
		return true;
	}
}


/*
 *  Pause CD audio
 */

bool SysCDPause(void *arg)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return false;

	if (fh->is_file)
		return false;
	else {

		// Send PAUSE RESUME SCSI command
		struct SCSICmd scsi;
		static const UBYTE pause_cmd[10] = {0x4b, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		scsi.scsi_Data = (UWORD *)&tmp_buf;
		scsi.scsi_Length = 0;
		scsi.scsi_Command = (UBYTE *)pause_cmd;
		scsi.scsi_CmdLength = 10;
		scsi.scsi_Flags = SCSIF_READ;
		scsi.scsi_Status = 0;
		fh->io->io_Data = &scsi;
		fh->io->io_Length = sizeof(scsi);
		fh->io->io_Command = HD_SCSICMD;
		if (DoIO((struct IORequest *)fh->io) || scsi.scsi_Status)
			return false;
		return true;
	}
}


/*
 *  Resume paused CD audio
 */

bool SysCDResume(void *arg)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return false;

	if (fh->is_file)
		return false;
	else {

		// Send PAUSE RESUME SCSI command
		struct SCSICmd scsi;
		static const UBYTE resume_cmd[10] = {0x4b, 0, 0, 0, 0, 0, 0, 0, 1, 0};
		scsi.scsi_Data = (UWORD *)&tmp_buf;
		scsi.scsi_Length = 0;
		scsi.scsi_Command = (UBYTE *)resume_cmd;
		scsi.scsi_CmdLength = 10;
		scsi.scsi_Flags = SCSIF_READ;
		scsi.scsi_Status = 0;
		fh->io->io_Data = &scsi;
		fh->io->io_Length = sizeof(scsi);
		fh->io->io_Command = HD_SCSICMD;
		if (DoIO((struct IORequest *)fh->io) || scsi.scsi_Status)
			return false;
		return true;
	}
}


/*
 *  Stop CD audio
 */

bool SysCDStop(void *arg, uint8 lead_out_m, uint8 lead_out_s, uint8 lead_out_f)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return false;

	if (fh->is_file)
		return false;
	else {

		uint8 end_m = lead_out_m;
		uint8 end_s = lead_out_s;
		uint8 end_f = lead_out_f + 1;
		if (end_f >= 75) {
			end_f = 0;
			end_s++;
			if (end_s >= 60) {
				end_s = 0;
				end_m++;
			}
		}

		// Send PLAY AUDIO MSF SCSI command (play first frame of lead-out area)
		struct SCSICmd scsi;
		UBYTE play_cmd[10] = {0x47, 0, 0, lead_out_m, lead_out_s, lead_out_f, end_m, end_s, end_f, 0};
		scsi.scsi_Data = (UWORD *)&tmp_buf;
		scsi.scsi_Length = 0;
		scsi.scsi_Command = play_cmd;
		scsi.scsi_CmdLength = 10;
		scsi.scsi_Flags = SCSIF_READ;
		scsi.scsi_Status = 0;
		fh->io->io_Data = &scsi;
		fh->io->io_Length = sizeof(scsi);
		fh->io->io_Command = HD_SCSICMD;
		if (DoIO((struct IORequest *)fh->io) || scsi.scsi_Status)
			return false;
		return true;
	}
}


/*
 *  Perform CD audio fast-forward/fast-reverse operation starting from specified address
 */

bool SysCDScan(void *arg, uint8 start_m, uint8 start_s, uint8 start_f, bool reverse)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return false;

	//!!
	return false;
}


/*
 *  Set CD audio volume (0..255 each channel)
 */

void SysCDSetVolume(void *arg, uint8 left, uint8 right)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return;

	if (!fh->is_file) {

		// Send MODE SENSE (CD-ROM Audio Control Parameters Page) SCSI command
		struct SCSICmd scsi;
		static const UBYTE mode_sense_cmd[6] = {0x1a, 0x08, 0x0e, 0, 20, 0};
		scsi.scsi_Data = (UWORD *)&tmp_buf;
		scsi.scsi_Length = 20;
		scsi.scsi_Command = (UBYTE *)mode_sense_cmd;
		scsi.scsi_CmdLength = 6;
		scsi.scsi_Flags = SCSIF_READ;
		scsi.scsi_Status = 0;
		fh->io->io_Data = &scsi;
		fh->io->io_Length = sizeof(scsi);
		fh->io->io_Command = HD_SCSICMD;
		if (DoIO((struct IORequest *)fh->io) || scsi.scsi_Status)
			return;

		tmp_buf[6] = 0x04;		// Immed
		tmp_buf[9] = 0;		// LBA/sec format
		tmp_buf[10] = 0;		// LBA/sec
		tmp_buf[11] = 0;
		tmp_buf[13] = left;		// Port 0 volume
		tmp_buf[15] = right;	// Port 1 volume

		// Send MODE SELECT (CD-ROM Audio Control Parameters Page) SCSI command
		static const UBYTE mode_select_cmd[6] = {0x15, 0x10, 0, 0, 20, 0};
		scsi.scsi_Data = (UWORD *)&tmp_buf;
		scsi.scsi_Length = 20;
		scsi.scsi_Command = (UBYTE *)mode_select_cmd;
		scsi.scsi_CmdLength = 6;
		scsi.scsi_Flags = SCSIF_WRITE;
		scsi.scsi_Status = 0;
		fh->io->io_Data = &scsi;
		fh->io->io_Length = sizeof(scsi);
		fh->io->io_Command = HD_SCSICMD;
		DoIO((struct IORequest *)fh->io);
	}
}


/*
 *  Get CD audio volume (0..255 each channel)
 */

void SysCDGetVolume(void *arg, uint8 &left, uint8 &right)
{
	file_handle *fh = (file_handle *)arg;
	if (!fh)
		return;

	if (!fh->is_file) {

		// Send MODE SENSE (CD-ROM Audio Control Parameters Page) SCSI command
		struct SCSICmd scsi;
		static const UBYTE mode_sense_cmd[6] = {0x1a, 0x08, 0x0e, 0, 20, 0};
		scsi.scsi_Data = (UWORD *)&tmp_buf;
		scsi.scsi_Length = 20;
		scsi.scsi_Command = (UBYTE *)mode_sense_cmd;
		scsi.scsi_CmdLength = 6;
		scsi.scsi_Flags = SCSIF_READ;
		scsi.scsi_Status = 0;
		fh->io->io_Data = &scsi;
		fh->io->io_Length = sizeof(scsi);
		fh->io->io_Command = HD_SCSICMD;
		if (DoIO((struct IORequest *)fh->io) || scsi.scsi_Status)
			return;
		left = tmp_buf[13];		// Port 0 volume
		right = tmp_buf[15];	// Port 1 volume
	}
}
