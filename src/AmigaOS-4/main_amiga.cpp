

/*
 *  main_amiga.cpp - Startup code for AmigaOS4
 *
 *  Basilisk II (C)	1997-2001 Christian Bauer
 *				2005-2006 Ilkka Lehtoranta
 *				2007-2009 Kjetil Hvalstrand
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

#include <exec/execbase.h>
#include <exec/tasks.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <devices/timer.h>
#include <devices/ahi.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/locale.h>

#define ALL_REACTION_CLASSES
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
#include <proto/intuition.h>
#include <proto/icon.h>

#include "guithread.h"

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"
#include "xpram.h"
#include "timer.h"
#include "sony.h"
#include "disk.h"
#include "cdrom.h"
#include "scsi.h"
#include "audio.h"
#include "video.h"
#include "serial.h"
#include "ether.h"
#include "clip.h"
#include "emul_op.h"
#include "rom_patches.h"
#include "prefs.h"
#include "prefs_editor.h"
#include "sys.h"
#include "user_strings.h"
#include "version.h"

#define DEBUG 0
#include "debug.h"

#define 	MaxROMSize 0x100000;

#define 	safecloselib(a) { CloseLibrary(a); a = NULL; }

#define FREE_STR(str) if (str) { free(str);str=NULL; }

#define MERGE_STR(varn,str1,str2) \
	FREE_STR(varn) \
 	if (varn = (char *) malloc(strlen(str1)+strlen(str2)+1)) \
 	{ sprintf(varn,"%s%s",(char *) str1,(char *) str2); }


// extern APTR _WBenchMsg;

extern int runtime_gui_tread ();

// Options for libnix
const int __nocommandline = 1;			// Disable command line parsing


// Constants
static const char ROM_FILE_NAME[] = "ROM";
static const char __ver[] = "$VER: " VERSION_STRING " " __DATE__;
static const int SCRATCH_MEM_SIZE = 65536;

// CPU and FPU type, addressing mode
int CPUType = 4;
bool CPUIs68060;
int FPUType;
bool TwentyFourBitAddressing;

struct Library * AHIBase = NULL;
struct AHIIFace *IAHI = NULL;

// Global variables
struct Library *GfxBase = NULL;
struct Library *IFFParseBase = NULL;
struct Library *AslBase = NULL;
// struct Library *CyberGfxBase = NULL;
struct Library *TimerBase = NULL;
// struct Library *MUIMasterBase;
struct Library *IconBase;
struct Library *LocaleBase;

struct Task *MainTask;							// Our task
uint8 *ScratchMem = NULL;						// Scratch memory for Mac ROM writes

ULONG	SubTaskCount;
struct MsgPort	*StartupMsgPort, *GUIPort;
struct Proc	*Sound_Proc;

static struct TimeRequest timereq;				// IORequest for timer

#ifdef USE_SDL
extern struct TimerIFace *ITimer;				// when compling whit SDL, use extern
#else
 struct TimerIFace *ITimer;				// when compling whit SDL, use extern
#endif
static struct MsgPort *ahi_port = NULL;			// Port for AHI
static struct AHIRequest *ahi_io = NULL;		// IORequest for AHI

static struct Process *xpram_proc = NULL;		// XPRAM watchdog
static struct Process *gui_proc = NULL;		// XPRAM watchdog
static struct Process *tick_proc = NULL;		// 60Hz process
static volatile bool tick_proc_active = true;

struct DiskObject *dobj = NULL;

// Prototypes
static void xpram_func(void);
static void tick_func(void);

static bool ChoiceAlert2(const char *text, const char *pos, const char *neg);

extern void ADBInit(void);
extern void ADBExit(void);

struct Catalog *catalog;

 char *PREFS_FILE_NAME = NULL; 
 char *PREFS_FILE_NAME_ARC = NULL; 
 char *XPRAM_FILE_NAME = NULL; 
 char *XPRAM_FILE_NAME_ARC = NULL; 

#define AllocVecSharedClear(size) AllocVecTags( size, AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_END );

/*
 * Open libraries
 */

static int openlibs(void)
{
	if ((GfxBase = OpenLibrary("graphics.library", 39)))
	if ((IntuitionBase = OpenLibrary("intuition.library", 39)))
	if ((IFFParseBase = OpenLibrary("iffparse.library", 39)))
	if ((AslBase = OpenLibrary("asl.library", 36)))
	if ((IconBase = OpenLibrary("icon.library", 0)))
	if ((LocaleBase = OpenLibrary("locale.library", 0)))
	{
		catalog = OpenCatalog(NULL, "basilisk.catalog", OC_BuiltInLanguage, "english", TAG_DONE);
		return 1;
	}

	return 0;
}

/*
 *  Main program
 */


int MacAddressSpace = 0;


void show_sigs(char *txt)
{
	struct Task *t;
	int n;

	t = FindTask(NULL);

	printf("\n%s\n",txt);

	for (n=0;n<31;n++)
	{
		if ((t -> tc_SigAlloc & 0x1FFF0000) & (1 << n))	printf("Sig %08x\n",1 << n);
	}

/*
	printf("\n%s\n",txt);

	printf("BIT: ");
	for (n=31;n>=0;n--) printf("%d", (t -> tc_SigAlloc & 0x1FFF0000) & (1<<n) ? 1 : 0);
	printf("\n");

	printf("HEX: %08X\n",t -> tc_SigAlloc & 0x1FFF0000 );

*/
}


void open_ahi()
{
	// Open AHI

	ahi_io	= NULL;
	IAHI		= NULL;
	AHIBase	= NULL;

	ahi_port = (MsgPort*) AllocSysObject(ASOT_PORT, TAG_END );
	if (ahi_port) {

		printf("AHI port %x\n", (unsigned int) ahi_port);

		ahi_io = (struct AHIRequest *) CreateIORequest(ahi_port, sizeof(struct AHIRequest));
		if (ahi_io) {

			printf("AHI Request = %x\n", (unsigned int) ahi_io);

			ahi_io->ahir_Version = 2;
			if (OpenDevice( AHINAME, AHI_NO_UNIT, (struct IORequest *) ahi_io, 0) == 0) {

				AHIBase = (struct Library *)ahi_io->ahir_Std.io_Device;
				IAHI = (struct AHIIFace*) GetInterface(AHIBase,"main",1L,NULL) ;

				printf("AHI Base = %x\n", (unsigned int) AHIBase);
				printf("AHI Interface = %x\n", (unsigned int) IAHI);

/*				AudioInit(); */
			}
		}
	}
}

void close_ahi()
{
	printf("AHI: Close device\n");
	if (AHIBase)	CloseDevice((struct IORequest *)ahi_io);
	printf("AHI: Remove IO Request\n");
	if (ahi_io)		DeleteIORequest((struct IORequest *)ahi_io);
	printf("AHI: Delete MsgPort\n");
	if (ahi_port)	FreeSysObject(ASOT_PORT,ahi_port);
}

#define only_prefs_filename "BasiliskII_prefs"
#define only_xpram_filename "BasiliskII_XPRAM"

int main(int argc, char **argv)
{
	static int wbstart = 0; 
	int n;
	char c;

	// Initialize variables
	RAMBaseHost = NULL;
	ROMBaseHost = NULL;
	MainTask = FindTask(NULL);
	struct DateStamp ds;
	DateStamp(&ds);
	srand(ds.ds_Tick);


	PREFS_FILE_NAME =		strdup("ENV:"    only_prefs_filename);
	PREFS_FILE_NAME_ARC =	strdup("ENVARC:" only_prefs_filename);
	XPRAM_FILE_NAME =		strdup("ENV:"    only_xpram_filename);
	XPRAM_FILE_NAME_ARC =	strdup("ENVARC:" only_xpram_filename);


	if (openlibs() == 0)
	{
//		if (MUIMasterBase == NULL)
//			exit(1);
//		else
			QuitEmulator();
	}

	// Find program icon

	if (argc==0)
	{
		wbstart = 1;

		struct WBStartup *wbmsg = (struct WBStartup *) argv;
		struct WBArg	*wargs;
		BPTR	lock;

		wargs	= wbmsg->sm_ArgList;

		if (wbmsg->sm_NumArgs > 1)
			wargs++;

		lock	= SetCurrentDir(wargs->wa_Lock);
		dobj	= GetDiskObject((char *const)wargs->wa_Name);
		SetCurrentDir(lock);
	}
	else
	{
		if (argc==2)
		{
			if ((strlen(argv[1])>3)&&(argv[1][0]!='-')&&(argv[1][1]!='-'))
			{
				c = argv[1][strlen(argv[1])-1];

				if ((c=='/')||(c==':'))
				{
					MERGE_STR(PREFS_FILE_NAME,		argv[1],	only_prefs_filename);
					MERGE_STR(PREFS_FILE_NAME_ARC,	argv[1],	only_prefs_filename);
					MERGE_STR(XPRAM_FILE_NAME,		argv[1],	only_xpram_filename);
					MERGE_STR(XPRAM_FILE_NAME_ARC,	argv[1],	only_xpram_filename);
				}
			}
		}

		printf(GetString(STR_ABOUT_TEXT1), VERSION_MAJOR, VERSION_MINOR);
		printf(" %s\n", GetString(STR_ABOUT_TEXT2));
	}


	if (dobj == NULL)
	{
		dobj	= GetDiskObject("PROGDIR:BasiliskII");
	}

	StartupMsgPort = (MsgPort*) AllocSysObjectTags(ASOT_PORT, TAG_DONE);

	if ( StartupMsgPort == NULL) 
	{
		QuitEmulator();
	}

	// Read preferences
	PrefsInit(NULL,argc, argv);

//	open_ahi();

	// Init system routines
	SysInit();

	// Show preferences editor
	if (!PrefsFindBool("nogui"))
	{
		if (!PrefsEditor())
			QuitEmulator();
	}

	// Open timer.device
	if (OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)&timereq, 0)) {
		ErrorAlert(GetString(STR_NO_TIMER_DEV_ERR));
		QuitEmulator();
	}
	TimerBase = (struct Library *) timereq.Request.io_Device;
	ITimer = (struct TimerIFace *) GetInterface(TimerBase,"main",1L,NULL) ;

	if (!ITimer)
	{
		ErrorAlert("Timer interface problem!!\n");
		QuitEmulator();
	}

	// Allocate scratch memory
	ScratchMem = (uint8 *)AllocVecSharedClear(SCRATCH_MEM_SIZE);
	if (ScratchMem == NULL) {
		ErrorAlert(GetString(STR_NO_MEM_ERR));
		QuitEmulator();
	}
	ScratchMem += SCRATCH_MEM_SIZE/2;	// ScratchMem points to middle of block

	// Create area for Mac RAM and ROM (ROM must be higher in memory,
	// so we allocate one big chunk and put the ROM at the top of it)
	RAMSize = PrefsFindInt32("ramsize") & 0xfff00000;	// Round down to 1MB boundary

	if (RAMSize < 1024*1024) {
		WarningAlert(GetString(STR_SMALL_RAM_WARN));
		RAMSize = 1024*1024;
	}

	do
	{
		MacAddressSpace = RAMSize +  0x300000;
		RAMBaseHost = (uint8 *) AllocVecSharedClear( MacAddressSpace );

		if (!RAMBaseHost)
		{
			printf("Can get %d MB of MacAddressSpace!\n tying less memory!\n",MacAddressSpace / (1024*1024) );
			RAMSize = 7 * (1024*1024); // 7 MB!
		}
		
	} while (!RAMBaseHost);

	if (RAMBaseHost == NULL)
	{
		ErrorAlert(GetString(STR_NO_MEM_ERR));
		QuitEmulator();
	}

	RAMBaseMac	= 0;
	ROMBaseMac	= RAMBaseMac + RAMSize;
	ROMBaseHost = RAMBaseHost + RAMSize;
	MEMBaseDiff	= (uintptr)RAMBaseHost;

	printf("RAMBaseHost 0x%x - 0x%x (Size %d byte)\n", (unsigned int) RAMBaseHost, (unsigned int) RAMBaseHost + RAMSize , (int) RAMSize);
	printf("ROMBaseHost 0x%x - 0x%x (Size %d byte)\n", (unsigned int) ROMBaseHost, (unsigned int) ROMBaseHost + (int) (MacAddressSpace - RAMSize) , (int) (MacAddressSpace - RAMSize) );
	printf("End address 0x%x (Size %d byte)\n",(unsigned int) (RAMBaseHost + MacAddressSpace), MacAddressSpace);

	// Get rom file path from preferences
	const char *rom_path = PrefsFindString("rom");

	// Load Mac ROM
	BPTR rom_fh = Open(rom_path ? rom_path : ROM_FILE_NAME, MODE_OLDFILE);
	if (!rom_fh)
	{
		ErrorAlert(GetString(STR_NO_ROM_FILE_ERR));
		QuitEmulator();
	}
	if (!wbstart)
	{
		printf(GetString(STR_READING_ROM_FILE));
	}

	struct ExamineData *fib;

	if (fib = ExamineObjectTags(EX_FileHandleInput, rom_fh))
	{
		ROMSize = fib -> FileSize;
		FreeDosObject(DOS_EXAMINEDATA,fib);
	}

	if (ROMSize != 512*1024 && ROMSize != 1024*1024)
	{
		ErrorAlert(GetString(STR_ROM_SIZE_ERR));
		Close(rom_fh);
		QuitEmulator();
	}

	printf("ROM 0x%x\n",(unsigned int) ROMBaseHost);
	printf("ROM Size %d  (%x)\n", (int) ROMSize, (unsigned int) ROMSize);

	if (Read(rom_fh, ROMBaseHost, ROMSize) != (LONG)ROMSize)
	{
		Close(rom_fh);
		ErrorAlert(GetString(STR_ROM_FILE_READ_ERR));
		QuitEmulator();
	}

	Close(rom_fh);

	printf("ROM loaded!\n");

	Delay(5);

	// Initialize everything
	if (!InitAll(NULL)) QuitEmulator();

	struct Message msg1, msg2;

	msg1.mn_Node.ln_Type	= NT_MESSAGE;
	msg1.mn_ReplyPort	= StartupMsgPort;
	msg1.mn_Length		= sizeof(msg1);
	msg2.mn_Node.ln_Type	= NT_MESSAGE;
	msg2.mn_ReplyPort	= StartupMsgPort;
	msg2.mn_Length		= sizeof(msg2);


	ADBInit();	

/*
	// Start runtime gui
	gui_proc = CreateNewProcTags(
		NP_Entry, (ULONG) runtime_gui_tread,
		NP_Name, (ULONG) "Basilisk II runtime gui tread",
		NP_Priority, 0,
		TAG_END
	);
*/
	printf("Start XPRAM watchdog process\n");

	// Start XPRAM watchdog process
	xpram_proc = CreateNewProcTags(
		NP_Entry, (ULONG)xpram_func,
		NP_Name, (ULONG)"Basilisk II XPRAM Watchdog",
		NP_Priority, 0,
		TAG_END
	);

	if (xpram_proc)
	{
		SubTaskCount++;
	}

	printf("Start 60Hz process\n");

	// Start 60Hz process
	tick_proc = CreateNewProcTags(
		NP_Entry, (ULONG)tick_func,
		NP_Name, (ULONG)"Basilisk II 60Hz",
		NP_Priority, 5,
		TAG_END
	);

	if (tick_proc)
	{
		SubTaskCount++;

		SetTaskPri(MainTask, -1);

		// Jump to ROM boot routine
		//VNewRawDoFmt("Start emulation\n", (APTR (*)(APTR, UBYTE))1, NULL, NULL);

		printf("main_amiga.cpp / Start680x0()\n");

		Start680x0();

		printf("Emulation done... \n");
	}

	QuitEmulator();
	return 0;
}

void QuitEmulator(void)
{

	// Abort processes

	printf("** Abort processes\n");

	if (Sound_Proc)	Signal((struct Task *)Sound_Proc, SIGBREAKF_CTRL_C);

	print_sigs( __FUNCTION__ ,  __LINE__ );

	printf("** Stop 60Hz process\n");

	// Stop 60Hz process

	if (tick_proc) Signal((struct Task *)tick_proc, SIGBREAKF_CTRL_C);
	tick_proc_active = false;

	print_sigs( __FUNCTION__ ,  __LINE__ );

	printf("** Stop XPRAM watchdog process\n");

	// Stop XPRAM watchdog process

	if (xpram_proc)	Signal((struct Task *)xpram_proc, SIGBREAKF_CTRL_C);

	print_sigs( __FUNCTION__ ,  __LINE__ );

	ADBExit();

	if (gui_proc) Signal((struct Task *)gui_proc, SIGBREAKF_CTRL_C);

	print_sigs( __FUNCTION__ ,  __LINE__ );

	// Deinitialize everything
	ExitAll();

	// Close timer.device
	if (TimerBase) CloseDevice((struct IORequest *)&timereq);

	// Exit system routines
	SysExit();

	printf("close ahi()\n");
//	close_ahi();

	printf("PrefsExit()\n");

	// Exit preferences
	PrefsExit();

	if (StartupMsgPort) FreeSysObject(ASOT_PORT,StartupMsgPort);

	// Close libraries

	if (LocaleBase)
	{
		CloseCatalog(catalog);
		CloseLibrary(LocaleBase);
	}

	if (IconBase)
	{
		FreeDiskObject(dobj);
		CloseLibrary(IconBase);
	}

	safecloselib(AslBase);
	safecloselib(IFFParseBase);
	safecloselib(IntuitionBase);
	safecloselib(GfxBase);

	if (RAMBaseHost) 
	{
		FreeVec(RAMBaseHost);
		RAMBaseHost=NULL;
	}

	FREE_STR(PREFS_FILE_NAME);
	FREE_STR(PREFS_FILE_NAME_ARC);
	FREE_STR(XPRAM_FILE_NAME);
	FREE_STR(XPRAM_FILE_NAME_ARC);


	exit(0);
}


/*
 *  Code was patched, flush caches if neccessary (i.e. when using a real 680x0
 *  or a dynamically recompiling emulator)
 */

void FlushCodeCache(void *start, uint32 size)
{
}

/*
 *  Interrupt flags (must be handled atomically!)
 */

uint32 InterruptFlags;


static void SetIntFlag(uint32 flag, uint32 *ptr)
{
	Forbid();
	*ptr |= flag;
	Permit();
}

static void ClearIntFlag(uint32 flag, uint32 *ptr)
{
	Forbid();
	*ptr &= ~flag;
	Permit();
}

void SetInterruptFlag(uint32 flag)
{
	SetIntFlag(flag, &InterruptFlags);
}

void ClearInterruptFlag(uint32 flag)
{
	ClearIntFlag(flag, &InterruptFlags);
}

/*
 *  60Hz thread (really 60.15Hz)
 */


static void tick_func(void)
{
	int tick_counter = 0;
	struct MsgPort *timer_port = NULL;
	struct TimeRequest *timer_io = NULL;
	ULONG timer_mask = 0;

	// Start 60Hz timer
	timer_port = (MsgPort*) AllocSysObject(ASOT_PORT, TAG_END );
	if (timer_port) {
		timer_io = (struct TimeRequest *) CreateIORequest(timer_port, sizeof(struct TimeRequest));
		if (timer_io) {
			if (!OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *) timer_io, 0)) {
				timer_mask = 1 << timer_port->mp_SigBit;
				timer_io->Request.io_Command = TR_ADDREQUEST;
				timer_io->Time.Seconds = 0;
				timer_io->Time.Microseconds = 16625 ;
				SendIO( (struct IORequest *) timer_io);
			}
		}
	}

	while (tick_proc_active) {

		// Wait for timer tick
		Wait(timer_mask);

		// Restart timer
		timer_io->Request.io_Command = TR_ADDREQUEST;
		timer_io->Time.Seconds = 0;
		timer_io->Time.Microseconds = 16625 ;
		SendIO((struct IORequest *)timer_io);

		// Pseudo Mac 1Hz interrupt, update local time
		if (++tick_counter > 60) {
			tick_counter = 0;
			WriteMacInt32(0x20c, TimerDateTime());
			SetInterruptFlag(INTFLAG_1HZ);
			TriggerInterrupt();
		}

		// Trigger 60Hz interrupt
		SetInterruptFlag(INTFLAG_60HZ);
		TriggerInterrupt();
	}

	// Stop timer
	if (timer_io) {
		if (!CheckIO( (struct IORequest *) timer_io))
			AbortIO( (struct IORequest *) timer_io);
		WaitIO( (struct IORequest *) timer_io);
		CloseDevice( (struct IORequest *) timer_io);
		DeleteIORequest( (struct IORequest*) timer_io);
	}
	if (timer_port)
		FreeSysObject(ASOT_PORT,timer_port);

	// Main task asked for termination, send signal
	Forbid();
	Signal(MainTask, SIGF_SINGLE);
}

/*
 *  XPRAM watchdog thread (saves XPRAM every minute)
 */

static void xpram_func(void)
{
#if 0
	uint8 last_xpram[256];
	memcpy(last_xpram, XPRAM, 256);

	while (xpram_proc_active) {
		for (int i=0; i<60 && xpram_proc_active; i++)
			Delay(50);		// Only wait 1 second so we quit promptly when xpram_proc_active becomes false
		if (memcmp(last_xpram, XPRAM, 256)) {
			memcpy(last_xpram, XPRAM, 256);
			SaveXPRAM();
		}
	}
#else
	Wait(SIGBREAKF_CTRL_C);
	xpram_proc	= NULL;
	SaveXPRAM();
#endif
}


/*
 *  Display error alert
 */

void ErrorAlert(const char *text)
{
	if (PrefsFindBool("nogui")) {
		printf(GetString(STR_SHELL_ERROR_PREFIX), text);
		return;
	}

	printf("ErrorAlert: %s\n",(char *) text);
}


/*
 *  Display warning alert
 */

void WarningAlert(const char *text)
{
	if (PrefsFindBool("nogui")) {
		printf(GetString(STR_SHELL_WARNING_PREFIX), text);
		return;
	}

	printf("WarningAlert: %s\n",(char *) text);
}


/*
 *  Display choice alert
 */

static bool ChoiceAlert2(const char *text, const char *pos, const char *neg)
{
	int opt;

	printf("\n");
	printf(" 1. %s\n",pos);
	printf(" 2. %s\n",neg);
	scanf("\nSelect 1 or 2: %d\n",&opt);

	if (opt==1) return true;
	return false;
}


/*
 *  Mutexes
 */

 struct B2_mutex;
/* typedef APTR B2_mutex;*/

#ifdef ENABLE_EXCLUSIVE_SPCFLAGS


B2_mutex *B2_create_mutex(void)
{
	B2_mutex *mutex;

	mutex = (B2_mutex *) IExec->AllocSysObjectTags( ASOT_MUTEX, 
			ASOMUTEX_Recursive, TRUE,
			TAG_DONE );

	if (!mutex)
	{
		printf("Worning: B2_create_mutex() failed\n");
	}
	return mutex;
}

void B2_lock_mutex(B2_mutex *mutex)
{
	if (mutex)
	{
		MutexObtain( (APTR) mutex );
	}
	else
	{
		struct Task *task = FindTask(NULL);
		printf("Warning: no mutex found B2_lock_mutex()\n");
		printf("task name %s\n", task->tc_Node.ln_Name);
	}
}

void B2_unlock_mutex(B2_mutex *mutex)
{
	if (mutex)
	{
		MutexRelease( (APTR) mutex );
	}
	else
	{
		struct Task *task = FindTask(NULL);
		printf("Warning: no mutex found B2_unlock_mutex()\n");
		printf("task name %s\n", task -> tc_Node.ln_Name);
	}
}

void B2_delete_mutex(B2_mutex *mutex)
{
	if (mutex)
	{
		FreeSysObject(ASOT_MUTEX, (APTR) mutex);
	}
	else
	{
		struct Task *task = FindTask(NULL);
		printf("Warning: no mutex found B2_delete_mutex()\n");
		printf("task name %s\n", task->tc_Node.ln_Name);
	}
}

#else

B2_mutex *B2_create_mutex(void)  {	return (B2_mutex *) 0xFFFF0000;}

void B2_lock_mutex(B2_mutex *mutex) {}

void B2_unlock_mutex(B2_mutex *mutex) {}

void B2_delete_mutex(B2_mutex *mutex) {}

#endif
