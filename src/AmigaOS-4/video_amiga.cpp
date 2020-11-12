/*
 *  video_amiga.cpp - Video/graphics emulation, AmigaOS specific stuff
 *
 *  Basilisk II (C) 1997-2008 Christian Bauer
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

#define __USE_OLD_TIMEVAL__

#include <exec/types.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <dos/dostags.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <graphics/composite.h>
#include <proto/Picasso96API.h>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"
#include "adb.h"
#include "prefs.h"
#include "user_strings.h"
#include "video.h"

#define DEBUG 0
#include "debug.h"

#define MAC_CURSOR_UP	0x3E
#define MAC_CURSOR_DOWN	0x3D

int last_wheel = 0;
int delta_wheel = 0;

int window_x = 0;
int window_y = 0;

// ---- Default options!!! ----

int use_p96_lock = 1;
int use_direct_video_for_32bit_screens = 0;

// ----------------------------

#if 0
 extern void show_sigs(char *txt);
#else
#define show_sigs( x )
#endif

int frame_dice=0;

struct XYSTW_Vertex3D { 
float x, y; 
float s, t, w; 
}; 

APTR video_mutex = NULL;

// Supported video modes
static vector<video_mode> VideoModes;

// Display types
enum {
	DISPLAY_WINDOW,
	DISPLAY_WINDOW_COMP,
	DISPLAY_SCREEN_FRAME_SKIP,
	DISPLAY_SCREEN_MMU_HACK,
	DISPLAY_SCREEN_DIRECT_VIDEO,
	DISPLAY_SCREEN_P96,
};

// Global variables
static int32 frame_skip;
static int32 line_skip;
static UWORD *null_pointer = NULL;			// Blank mouse pointer data
static UWORD *current_pointer = (UWORD *)-1;		// Currently visible mouse pointer data
static struct Process *periodic_proc = NULL;		// Periodic process

extern struct Task *MainTask;				// Pointer to main task (from main_amiga.cpp)


// Amiga -> Mac raw keycode translation table
static const uint8 keycode2mac[0x80] = {
	0x0a, 0x12, 0x13, 0x14, 0x15, 0x17, 0x16, 0x1a,	//   `   1   2   3   4   5   6   7
	0x1c, 0x19, 0x1d, 0x1b, 0x18, 0x2a, 0xff, 0x52,	//   8   9   0   -   =   \ inv   0
	0x0c, 0x0d, 0x0e, 0x0f, 0x11, 0x10, 0x20, 0x22,	//   Q   W   E   R   T   Y   U   I
	0x1f, 0x23, 0x21, 0x1e, 0xff, 0x53, 0x54, 0x55,	//   O   P   [   ] inv   1   2   3
	0x00, 0x01, 0x02, 0x03, 0x05, 0x04, 0x26, 0x28,	//   A   S   D   F   G   H   J   K
	0x25, 0x29, 0x27, 0x2a, 0xff, 0x56, 0x57, 0x58,	//   L   ;   '   # inv   4   5   6
	0x32, 0x06, 0x07, 0x08, 0x09, 0x0b, 0x2d, 0x2e,	//   <   Z   X   C   V   B   N   M
	0x2b, 0x2f, 0x2c, 0xff, 0x41, 0x59, 0x5b, 0x5c,	//   ,   .   / inv   .   7   8   9
	0x31, 0x33, 0x30, 0x4c, 0x24, 0x35, 0x75, 0xff,	// SPC BSP TAB ENT RET ESC DEL inv
	0xff, 0xff, 0x4e, 0xff, 0x3e, 0x3d, 0x3c, 0x3b,	// inv inv   - inv CUP CDN CRT CLF
	0x7a, 0x78, 0x63, 0x76, 0x60, 0x61, 0x62, 0x64,	//  F1  F2  F3  F4  F5  F6  F7  F8
	0x65, 0x6d, 0x47, 0x51, 0x4b, 0x43, 0x45, 0x72,	//  F9 F10   (   )   /   *   + HLP
	0x38, 0x38, 0x39, 0x36, 0x3a, 0x3a, 0x37, 0x37,	// SHL SHR CAP CTL ALL ALR AML AMR
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	// inv inv inv inv inv inv inv inv
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	// inv inv inv inv inv inv inv inv
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff	// inv inv inv inv inv inv inv inv
};


class Amiga_monitor_desc : public monitor_desc {
public:
	Amiga_monitor_desc(const vector<video_mode> &available_modes, video_depth default_depth, uint32 default_id, int default_display_type) 
		:  monitor_desc(available_modes, default_depth, default_id), display_type(default_display_type) {};
	~Amiga_monitor_desc() {};

	virtual void switch_to_current_mode(void);
	virtual void set_palette(uint8 *pal, int num);

	bool video_open(void);
	void video_close(void);
public:
	int display_type;		// See enum above
};


/*
 *  Display "driver" classes
 */


class driver_base {
public:
	driver_base(Amiga_monitor_desc &m);
	virtual ~driver_base();

	virtual void set_palette(uint8 *pal, int num) {};
	virtual struct BitMap *get_bitmap() { return NULL; };
	virtual int draw() { return 1;}
	virtual int get_width() { return width; }
	virtual int get_height() { return height; }

public:
	Amiga_monitor_desc &monitor;	// Associated video monitor
	const video_mode &mode;		// Video mode handled by the driver
	BOOL init_ok;			// Initialization succeeded (we can't use exceptions because of -fomit-frame-pointer)
	ULONG vpal[256];
	struct Window *the_win;
	char *VIDEO_BUFFER;
	video_depth depth;

	int width;
	int height;
};

class driver_window : public driver_base {
public:
	driver_window(Amiga_monitor_desc &m, int width, int height);
	~driver_window();

	virtual void set_palette(uint8 *pal, int num);
	struct BitMap *get_bitmap() { return the_bitmap; };
	virtual int draw();

private:
	LONG black_pen, white_pen;
	struct BitMap *the_bitmap;
};

class driver_window_comp : public driver_base {
public:
	driver_window_comp(Amiga_monitor_desc &m, int width, int height);
	~driver_window_comp();
	virtual int draw();

	virtual void set_palette(uint8 *pal, int num);
	struct BitMap *get_bitmap() { return the_bitmap; };

private:
	LONG black_pen, white_pen;
	struct BitMap *the_bitmap;

	char *to_mem;
	int bytes_per_row;

};


class driver_screen_p96 : public driver_base {
public:
	driver_screen_p96(Amiga_monitor_desc &m, ULONG mode_id);
	~driver_screen_p96();
	virtual int draw();

	void set_palette(uint8 *pal, int num);

private:
	struct Screen *the_screen;
	struct BitMap *the_bitmap;
//	char *VIDEO_BUFFER;
};


static driver_base *drv = NULL;	// Pointer to currently used driver object


// Prototypes
static void periodic_func(void);
static void add_mode(uint32 width, uint32 height, uint32 resolution_id, uint32 bytes_per_row, video_depth depth);
static void add_modes(uint32 width, uint32 height, video_depth depth);
static ULONG find_mode_for_depth(uint32 width, uint32 height, uint32 depth);
static ULONG bits_from_depth(video_depth depth);
static bool is_valid_modeid(int display_type, ULONG mode_id);
static bool check_modeid_p96(ULONG mode_id);


/*
 *  Initialization
 */


static ULONG find_mode_for_depth(uint32 width, uint32 height, uint32 depth)
{
	struct List *ml;
	struct P96Mode	*mn;
	ULONG ID;

	if (depth == 1) depth=8; 
	if (depth == 24) depth=32;

	if(ml=p96AllocModeListTags(	
			P96MA_MinWidth, width, P96MA_MaxWidth, width,
			P96MA_MinHeight, height, P96MA_MaxHeight, height,
			P96MA_MinDepth, depth, P96MA_MaxDepth, depth,  
			TAG_DONE))
	{
		for(mn=(struct P96Mode *)(ml->lh_Head);mn->Node.ln_Succ;mn=(struct P96Mode *)mn->Node.ln_Succ)
		{
			if (check_modeid_p96(mn -> DisplayID))
			{
				ID = mn -> DisplayID;
			}
		}
		p96FreeModeList(ml);
	}

	return ID;
}



int p96_add_modes(int mode_id, video_depth depth)
{
	LONG depth_bits;
	int bpr;
	struct List *ml;
	struct P96Mode	*mn;

	int lw=0,lh=0;
	int w,h;

	switch (depth)
	{
		case VDEPTH_1BIT:	depth_bits = 8; break;
		case VDEPTH_8BIT: depth_bits = 8; break;
		case VDEPTH_16BIT: depth_bits = 16; break;
		case VDEPTH_32BIT: depth_bits = 32; break;
	}

	if(ml=p96AllocModeListTags(	P96MA_MinDepth, depth_bits, P96MA_MaxDepth, depth_bits,  TAG_DONE))
	{
		for(mn=(struct P96Mode *)(ml->lh_Head);mn->Node.ln_Succ;mn=(struct P96Mode *)mn->Node.ln_Succ)
		{

			if (check_modeid_p96(mn -> DisplayID))
			{
				w =  p96GetModeIDAttr( mn -> DisplayID, P96IDA_WIDTH );
				h =  p96GetModeIDAttr( mn -> DisplayID, P96IDA_HEIGHT );

				if (depth==VDEPTH_32BIT)
				{
					bpr =  p96GetModeIDAttr( mn -> DisplayID, P96IDA_STDBYTESPERROW );
				}
				else
				{
					bpr =  TrivialBytesPerRow( mn -> Width, depth );
				}

				if ((lw != w) && (lh != h))
				{
					lw = w; lh = h;

					if (bpr>0)
					{
						add_mode( mn -> Width, mn -> Height, mode_id,	 bpr, depth);
//						D(bug("%d,%d bpr %d -  %s\n",w,h, bpr,  mn->Description);
						mode_id ++;
					}
				}
			}
		}

		p96FreeModeList(ml);
	}

	return mode_id;
}



bool VideoInit(bool classic)
{
	video_depth default_depth = VDEPTH_32BIT;
	int default_width, default_height;
	int default_display_type = DISPLAY_WINDOW;
	int window_width, window_height;			// width and height for window display
	ULONG screen_mode_id;				// mode ID for screen display
	ULONG mode_opt = 0;
	int mode_id;
	int boot_depth;

	boot_depth = PrefsFindInt32("windowdepth");
    use_p96_lock = PrefsFindBool("use_p96_lock");
	use_direct_video_for_32bit_screens = PrefsFindBool("use_direct_video_32b");

	printf("boot_depth %d\n",boot_depth);

	show_sigs("START VideoInit(bool classic)\n");

	// Allocate blank mouse pointer data

	null_pointer = (UWORD *)AllocMem(12, MEMF_PUBLIC | MEMF_CHIP | MEMF_CLEAR);

	if (null_pointer == NULL) {
		ErrorAlert(STR_NO_MEM_ERR);
		return false;
	}

	video_mutex = AllocSysObject(ASOT_MUTEX,TAG_END);

	show_sigs("AFTER video_mutex = AllocSysObject(ASOT_MUTEX,TAG_END);)\n");

	// Read frame skip prefs
	frame_skip = PrefsFindInt32("frameskip");
	line_skip = PrefsFindInt32("lineskip");

	// in the gui its zero not in real life.
	frame_skip ++;
	line_skip ++;

	// Get screen mode from preferences
	const char *mode_str;
	if (classic)
		mode_str = "win/512/342";
	else
		mode_str = PrefsFindString("screen");

	default_width = window_width = 512;
	default_height = window_height = 384;

//	D(bug("%s\n",mode_str);

	if (mode_str) {
		if (sscanf(mode_str, "win/%d/%d", &window_width, &window_height) == 2)
			default_display_type = DISPLAY_WINDOW;
		else if (sscanf(mode_str, "wic/%d/%d", &window_width, &window_height) == 2 && P96Base)
			default_display_type = DISPLAY_WINDOW_COMP;
		else if (sscanf(mode_str, "scr/%d/%08x", &mode_opt,&screen_mode_id) == 2 && ( P96Base)) {
//			if (P96Base && p96GetModeIDAttr(screen_mode_id, P96IDA_ISP96))
				default_display_type = DISPLAY_SCREEN_P96;
//			else {
//				ErrorAlert(STR_NO_P96_MODE_ERR);
//				return false;
//			}
		}
	}

	show_sigs("before Construct list of supported modes");

	D(bug("default_display_type %08x, window_width %d, window_height %d\n", screen_mode_id, window_width, window_height));

	// Construct list of supported modes

	switch (default_display_type) {

		case DISPLAY_WINDOW:
			default_width = window_width;
			default_height = window_height;

			switch (boot_depth)
			{
				case 0: default_depth = VDEPTH_32BIT; break;
				case 1: default_depth = VDEPTH_1BIT; break;
				case 2: default_depth = VDEPTH_8BIT; break;
			}

			add_modes(window_width, window_height, VDEPTH_1BIT);
			add_modes(window_width, window_height, VDEPTH_8BIT);
			add_modes(window_width, window_height, VDEPTH_32BIT);
			break;

		case DISPLAY_WINDOW_COMP:
			default_width = window_width;
			default_height = window_height;

			switch (boot_depth)
			{
				case 0: default_depth = VDEPTH_32BIT; break;
				case 1: default_depth = VDEPTH_1BIT; break;
				case 2: default_depth = VDEPTH_8BIT; break;
			}

			add_modes(window_width, window_height, VDEPTH_1BIT);
			add_modes(window_width, window_height, VDEPTH_8BIT);
			add_modes(window_width, window_height, VDEPTH_32BIT);
			break;

		case DISPLAY_SCREEN_P96:

			struct DimensionInfo dimInfo;
			DisplayInfoHandle handle = FindDisplayInfo(screen_mode_id);

			if (handle == NULL) return false;

			if (GetDisplayInfoData(handle, (UBYTE *) &dimInfo, sizeof(dimInfo), DTAG_DIMS, 0) <= 0)	return false;

			default_width = 1 + dimInfo.Nominal.MaxX - dimInfo.Nominal.MinX;
			default_height = 1 + dimInfo.Nominal.MaxY - dimInfo.Nominal.MinY;

//			D(bug("default %08lx: %d x %d\n",screen_mode_id, default_width , default_height );

			switch (dimInfo.MaxDepth)
			{
				case 8: 
					default_depth = (boot_depth == 1) ? VDEPTH_1BIT : VDEPTH_8BIT;
					break;
				case 15:
				case 16: default_depth = VDEPTH_16BIT; break;
				case 24:
				case 32: default_depth = VDEPTH_32BIT; break;
			}

			mode_id = p96_add_modes(0x80 , VDEPTH_1BIT);
			mode_id = p96_add_modes(0x80 , VDEPTH_8BIT);
			mode_id = p96_add_modes(0x80 , VDEPTH_16BIT);
			mode_id = p96_add_modes(0x80 , VDEPTH_32BIT);

			break;
	}





#if DEBUG
	bug("Available video modes:\n");
	vector<video_mode>::const_iterator i = VideoModes.begin(), end = VideoModes.end();
	while (i != end) {
		bug(" %ld x %ld (ID %02lx), %ld colors\n", i->x, i->y, i->resolution_id, 1 << bits_from_depth(i->depth));
		++i;
	}
#endif

	show_sigs("before (VideoModes.size() == 1) in VideoInit(bool classic) \n");


	D(bug("VideoInit/%ld: def_width=%ld  def_height=%ld  def_depth=%ld\n", \
		__LINE__, default_width, default_height, default_depth));

	// Find requested default mode and open display
	if (VideoModes.size() == 1) {
		uint32 default_id ;

		// Create Amiga_monitor_desc for this (the only) display
		default_id = VideoModes[0].resolution_id;
		D(bug("VideoInit/%ld: default_id=%ld\n", __LINE__, default_id));
		Amiga_monitor_desc *monitor = new Amiga_monitor_desc(VideoModes, default_depth, default_id, default_display_type);
		VideoMonitors.push_back(monitor);

		// Open display
		return monitor->video_open();

	} else {

		// Find mode with specified dimensions
		std::vector<video_mode>::const_iterator i, end = VideoModes.end();
		for (i = VideoModes.begin(); i != end; ++i) {
			D(bug("VideoInit/%ld: w=%ld  h=%ld  d=%ld\n", __LINE__, i->x, i->y, bits_from_depth(i->depth)));
			if (i->x == default_width && i->y == default_height && i->depth == default_depth) {
				// Create Amiga_monitor_desc for this (the only) display
				uint32 default_id = i->resolution_id;
				D(bug("VideoInit/%ld: default_id=%ld default display type %d\n", __LINE__, default_id, default_display_type));
				Amiga_monitor_desc *monitor = new Amiga_monitor_desc(VideoModes, default_depth, default_id, default_display_type);
				VideoMonitors.push_back(monitor);

				// Open display
				return monitor->video_open();
			}
		}

		// Create Amiga_monitor_desc for this (the only) display
		uint32 default_id = VideoModes[0].resolution_id;
		D(bug("VideoInit/%ld: default_id=%ld\n", __LINE__, default_id));
		Amiga_monitor_desc *monitor = new Amiga_monitor_desc(VideoModes, default_depth, default_id, default_display_type);
		VideoMonitors.push_back(monitor);

		// Open display
		return monitor->video_open();
	}

	show_sigs("END VideoInit(bool classic)\n");

	return true;
}


bool Amiga_monitor_desc::video_open()
{
	const video_mode &mode = get_current_mode();
	ULONG depth_bits = bits_from_depth(mode.depth);
	ULONG ID = find_mode_for_depth(mode.x, mode.y, depth_bits);

	show_sigs("START Amiga_monitor_desc::video_open()\n");

	D(bug("video_open/%ld: width=%ld  height=%ld  depth=%ld  ID=%08lx\n", __LINE__, mode.x, mode.y, depth_bits, ID));

	if (ID == INVALID_ID) {
		ErrorAlert(STR_NO_VIDEO_MODE_ERR);
		return false;
	}

	D(bug("video_open/%ld: display_type=%ld\n", __LINE__, display_type));

	show_sigs("Before Amiga_monitor_desc::switch;()\n");

	MutexObtain(video_mutex);

	// Open display
	switch (display_type) {
		case DISPLAY_WINDOW:
			drv = new driver_window(*this, mode.x, mode.y);
			break;

		case DISPLAY_WINDOW_COMP:
			drv = new driver_window_comp(*this, mode.x, mode.y);
			break;

		case DISPLAY_SCREEN_P96:
			drv = new driver_screen_p96(*this, ID);
			break;
	}

	MutexRelease(video_mutex);

	show_sigs("AFTER Amiga_monitor_desc::switch;()\n");

	D(bug("video_open/%ld: drv=%08lx\n", __LINE__, drv));

	if (drv == NULL)
		return false;

	D(bug("video_open/%ld: init_ok=%ld\n", __LINE__, drv->init_ok));
	if (!drv->init_ok) {
		delete drv;
		drv = NULL;
		return false;
	}

	show_sigs("BEFORE Amiga_monitor_desc::Start periodic process\n");

	// Start periodic process
	periodic_proc = CreateNewProcTags(
		NP_Entry, (ULONG)periodic_func,
		NP_Name, (ULONG)"Basilisk II IDCMP Handler",
		NP_Priority, 0,
		TAG_END
	);

	D(bug("video_open/%ld: periodic_proc=%08lx\n", __LINE__, periodic_proc));

	if (periodic_proc == NULL) {
		ErrorAlert(STR_NO_MEM_ERR);
		return false;
	}

	show_sigs("END Amiga_monitor_desc::video_open()\n");

	return true;
}


void Amiga_monitor_desc::video_close()
{
	D(bug("Amiga_monitor_desc::video_close() START %d\n",__LINE__));
	// Stop periodic process
	if (periodic_proc) {
		SetSignal(0, SIGF_SINGLE);
		Signal(&periodic_proc->pr_Task, SIGBREAKF_CTRL_C);
		Wait(SIGF_SINGLE);
	}

	delete drv;
	drv = NULL;
	D(bug("Amiga_monitor_desc::video_close() END %d\n",__LINE__));
}


/*
 *  Deinitialization
 */

void VideoExit(void)
{
	D(bug("VideoExit(void) START %d\n",__LINE__));

	// Close displays
	vector<monitor_desc *>::iterator i, end = VideoMonitors.end();
	for (i = VideoMonitors.begin(); i != end; ++i)
		dynamic_cast<Amiga_monitor_desc *>(*i)->video_close();

	D(bug("VideoExit(void) %d\n",__LINE__));

	// Free mouse pointer
	if (null_pointer) {
		FreeMem(null_pointer, 12);
		null_pointer = NULL;
	}

	D(bug("VideoExit(void) %d\n",__LINE__));

	FreeSysObject(ASOT_MUTEX,video_mutex);

	D(bug("VideoExit(void) END %d\n",__LINE__));
}


/*
 *  Set palette
 */

void Amiga_monitor_desc::set_palette(uint8 *pal, int num)
{
	drv->set_palette(pal, num);
}


/*
 *  Switch video mode
 */

void Amiga_monitor_desc::switch_to_current_mode()
{
	// Close and reopen display
	video_close();
	if (!video_open()) {
		ErrorAlert(STR_OPEN_WINDOW_ERR);
		QuitEmulator();
	}
}


/*
 *  Close down full-screen mode (if bringing up error alerts is unsafe while in full-screen mode)
 */

void VideoQuitFullScreen(void)
{
}


/*
 *  Video message handling (not neccessary under AmigaOS, handled by periodic_func())
 */

void VideoInterrupt(void)
{
}


/*
 *  Process for window refresh and message handling
 */


void set_mouse_window(int mx,int my)
{

	int ww = (drv->the_win->Width - drv->the_win->BorderLeft - drv->the_win->BorderRight);
	int wh = (drv->the_win->Height - drv->the_win->BorderTop - drv->the_win->BorderBottom);

	mx -= drv-> the_win->BorderLeft;
	my -= drv->the_win->BorderTop;

//	D(bug("mx %d,my %d\n",mx,my);

	if (mx < 0	 || my < 0	 || mx >= ww  || my >= wh ) {
		if (current_pointer) {
			ClearPointer(drv->the_win);
			current_pointer = NULL;
		}
	} else {
		if (current_pointer != null_pointer) {
			// Hide mouse pointer inside window
			SetPointer(drv->the_win, null_pointer, 1, 16, 0, 0);
			current_pointer = null_pointer;
		}

//		D(bug("%d,%d - %d,%d - %d, %d\n", mx, my, ww, wh ,drv->get_width(),drv->get_height() );

		mx = ( mx * drv ->get_width() / ww );
		my = ( my * drv -> get_height() / wh );

		ADBMouseMoved(mx, my);
	}
}


static void periodic_func(void)
{
	struct MsgPort *timer_port = NULL;
	struct timerequest *timer_io = NULL;
	struct IntuiMessage *msg;
	struct MsgPort *win_port_old = NULL;
	struct MsgPort *win_port = NULL;

	ULONG win_mask = 0, timer_mask = 0;
	int error;
	int mx,my;

	D(bug("periodic_func/%ld: START \n", __LINE__));


	// create a new msgport becouse this runing in its own task.

	win_port_old = drv -> the_win -> UserPort;
	win_port = CreateMsgPort();

//	D(bug("periodic_func task: %08x\n",win_mask);


	if (win_port) win_mask = 1 << win_port->mp_SigBit;

		ModifyIDCMP(drv->the_win, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_RAWKEY |  IDCMP_EXTENDEDMOUSE |
			((drv->monitor.display_type == DISPLAY_SCREEN_P96) ? IDCMP_DELTAMOVE : 0));
	}

	D(bug("periodic_func/%ld: \n", __LINE__));

	// Start 60Hz timer for window refresh
//	if (drv->monitor.display_type == DISPLAY_WINDOW || drv->monitor.display_type == DISPLAY_WINDOW_COMP) {
		timer_port = CreateMsgPort();
		if (timer_port) {
			timer_io = (struct timerequest *) CreateIORequest(timer_port, sizeof(struct timerequest));
			if (timer_io) {
				if (!OpenDevice( (char *) TIMERNAME, UNIT_MICROHZ, (struct IORequest *)timer_io, 0)) {
					timer_mask = 1 << timer_port->mp_SigBit;
					timer_io->tr_node.io_Command = TR_ADDREQUEST;
					timer_io->tr_time.tv_secs = 0;
					timer_io->tr_time.tv_micro = 16667 * frame_skip;
					SendIO((struct IORequest *)timer_io);
				}
			}
		}
//	}

	D(bug("periodic_func/%ld: \n", __LINE__));

	// Main loop
	for (;;) {
		const video_mode &mode = drv->monitor.get_current_mode();

		// Wait for timer and/or window (CTRL_C is used for quitting the task)
		ULONG sig = Wait(win_mask | timer_mask | SIGBREAKF_CTRL_C);

		if (sig & SIGBREAKF_CTRL_C)
			break;

//		D(bug("periodic_func/%ld: display_type=%ld  the_win=%08lx\n", __LINE__, drv->monitor.display_type, drv->the_win));

		if (sig & timer_mask) {

			MutexObtain(video_mutex);
			error = drv->draw();
			MutexRelease(video_mutex);

			// Restart timer
			timer_io->tr_node.io_Command = TR_ADDREQUEST;
			timer_io->tr_time.tv_secs = 0;
			timer_io->tr_time.tv_micro = 16667 * frame_skip;
			SendIO((struct IORequest *)timer_io);
		}

		if (sig & win_mask) {

			// Handle window messages
			while (msg = (struct IntuiMessage *)GetMsg(win_port)) {

				// Get data from message and reply
				ULONG cl = msg->Class;
				UWORD code = msg->Code;
				UWORD qualifier = msg->Qualifier;


//				D(bug("msg class %08lx, display_type %d\n", cl, drv->monitor.display_type);

				struct IntuiWheelData *mouse_wheel = (struct IntuiWheelData *) msg -> IAddress;

				// Handle message according to class
				switch (cl) {

					case IDCMP_MOUSEMOVE:

						switch (drv->monitor.display_type) {
							case DISPLAY_SCREEN_P96:

								mx = msg->MouseX;
								my = msg->MouseY;

								D(bug("periodic_func/%ld: IDCMP_MOUSEMOVE mx=%ld  my=%ld\n", __LINE__, mx, my));
								ADBMouseMoved(mx, my);
								break;

							default:

								mx = drv->the_win->MouseX;
								my = drv->the_win->MouseY;

								MutexObtain(video_mutex);
								set_mouse_window( mx, my);
								MutexRelease(video_mutex);

								break;

						}
						break;
				

					case IDCMP_MOUSEBUTTONS:
						if (code == SELECTDOWN)
							ADBMouseDown(0);
						else if (code == SELECTUP)
							ADBMouseUp(0);
						else if (code == MENUDOWN)
							ADBMouseDown(1);
						else if (code == MENUUP)
							ADBMouseUp(1);
						else if (code == MIDDLEDOWN)
							ADBMouseDown(2);
						else if (code == MIDDLEUP)
							ADBMouseUp(2);
						break;

					case IDCMP_RAWKEY:
						if (qualifier & IEQUALIFIER_REPEAT)	// Keyboard repeat is done by MacOS
							break;
						if ((qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_LSHIFT | IEQUALIFIER_CONTROL)) ==
						    (IEQUALIFIER_LALT | IEQUALIFIER_LSHIFT | IEQUALIFIER_CONTROL) && code == 0x5f) {
							SetInterruptFlag(INTFLAG_NMI);
							TriggerInterrupt();
							break;
						}

						if (code & IECODE_UP_PREFIX)
							ADBKeyUp(keycode2mac[code & 0x7f]);
						else
							ADBKeyDown(keycode2mac[code & 0x7f]);
						break;

					case IDCMP_EXTENDEDMOUSE:

						last_wheel = delta_wheel;
						delta_wheel = mouse_wheel -> WheelY;

						if (delta_wheel<0)
						{
							ADBKeyDown( MAC_CURSOR_UP );
							ADBKeyUp( MAC_CURSOR_UP );
							ADBKeyDown( MAC_CURSOR_UP );
							ADBKeyUp( MAC_CURSOR_UP );
						}
						else	if (mouse_wheel -> WheelY>0)
						{
							ADBKeyDown( MAC_CURSOR_DOWN );
							ADBKeyUp( MAC_CURSOR_DOWN );
							ADBKeyDown( MAC_CURSOR_DOWN );
							ADBKeyUp( MAC_CURSOR_DOWN );
						}

						break;

				}

				ReplyMsg((struct Message *)msg);
			}

		}
	}

	D(bug("periodic_func/%ld: \n", __LINE__));

	// Stop timer
	if (timer_io) {
		if (!CheckIO((struct IORequest *)timer_io))
			AbortIO((struct IORequest *)timer_io);
		WaitIO((struct IORequest *)timer_io);
		CloseDevice((struct IORequest *)timer_io);
		DeleteIORequest( (IORequest *) timer_io);
	}

								D(bug("periodic_func/%ld: \n", __LINE__));

	if (timer_port)	DeleteMsgPort(timer_port);

								D(bug("periodic_func/%ld: \n", __LINE__));

	// Remove port from window and delete it
	Forbid();
	msg = (struct IntuiMessage *) win_port->mp_MsgList.lh_Head;
	struct Node *succ;
	while (succ = msg->ExecMessage.mn_Node.ln_Succ) {
		if (msg->IDCMPWindow == drv->the_win) {
			Remove((struct Node *)msg);
			ReplyMsg((struct Message *)msg);
		}
		msg = (struct IntuiMessage *)succ;
	}
	drv->the_win->UserPort = NULL;
	ModifyIDCMP(drv->the_win, 0);
	Permit();


	DeleteMsgPort(win_port);
								D(bug("periodic_func/%ld: \n", __LINE__));

	if (win_port_old) DeleteMsgPort(win_port_old);

	D(bug("periodic_func/%ld: END \n", __LINE__));

	Signal(MainTask, SIGF_SINGLE);
}


// Add mode to list of supported modes
static void add_mode(uint32 width, uint32 height, uint32 resolution_id, uint32 bytes_per_row, video_depth depth)
{
	video_mode mode;
	mode.x = width;
	mode.y = height;
	mode.resolution_id = resolution_id;
	mode.bytes_per_row = bytes_per_row;
	mode.depth = depth;

	D(bug("Added video mode: w=%ld  h=%ld  d=%ld\n", width, height, depth));

	VideoModes.push_back(mode);
}

// Add standard list of modes for given color depth
static void add_modes(uint32 width, uint32 height, video_depth depth)
{
	int n = 0x80;

	D(bug("add_modes: w=%ld  h=%ld  d=%ld\n", width, height, depth));

	if (width >= 320 && height >= 200)
		add_mode(320, 200, n++, TrivialBytesPerRow(320, depth), depth);

	if (width >= 512 && height >= 384)
		add_mode(512, 384, n++, TrivialBytesPerRow(512, depth), depth);

	if (width >= 640 && height >= 400)
		add_mode(640, 400, n++, TrivialBytesPerRow(640, depth), depth);

	if (width >= 640 && height >= 480)
		add_mode(640, 480, n++, TrivialBytesPerRow(640, depth), depth);

	if (width >= 800 && height >= 600)
		add_mode(800, 600, n++, TrivialBytesPerRow(800, depth), depth);

	if (width >= 1024 && height >= 768)
		add_mode(1024, 768, n++, TrivialBytesPerRow(1024, depth), depth);

	if (width >= 1152 && height >= 870)
		add_mode(1152, 870, n++, TrivialBytesPerRow(1152, depth), depth);

	if (width >= 1280 && height >= 1024)
		add_mode(1280, 1024, n++, TrivialBytesPerRow(1280, depth), depth);

	if (width >= 1600 && height >= 1200)
		add_mode(1600, 1200, n++, TrivialBytesPerRow(1600, depth), depth);
}


static ULONG bits_from_depth(video_depth depth)
{
	int bits = 1 << depth;
	if (bits == 16)
		bits = 15;
	else if (bits == 32)
		bits = 24;

	return bits;
}


static bool is_valid_modeid(int display_type, ULONG mode_id)
{
	if (INVALID_ID == mode_id)
		return false;

	switch (display_type) {
		case DISPLAY_SCREEN_P96:
			return check_modeid_p96(mode_id);
			break;
		default:
			return false;
			break;
	}
}


static bool check_modeid_p96(ULONG mode_id)
{
	// Check if the mode is one we can handle
	uint32 depth = p96GetModeIDAttr(mode_id, P96IDA_DEPTH);
	uint32 format = p96GetModeIDAttr(mode_id, P96IDA_RGBFORMAT);

	D(bug("check_modeid_p96: mode_id=%08lx  depth=%ld  format=%ld\n", mode_id, depth, format));

	if (!p96GetModeIDAttr(mode_id, P96IDA_ISP96))
		return false;

	switch (depth) {
		case 8:
			break;
		case 15:
		case 16:
			if (format != RGBFB_R5G5B5)
				return false;
			break;
		case 24:
		case 32:
			if (format != RGBFB_A8R8G8B8)
				return false;
			break;
		default:
			return false;
	}

	return true;
}



driver_base::driver_base(Amiga_monitor_desc &m)
 : monitor(m), mode(m.get_current_mode()), init_ok(false)
{
}

driver_base::~driver_base()
{
}


// Open window
driver_window::driver_window(Amiga_monitor_desc &m, int w, int h)
	: black_pen(-1), white_pen(-1), driver_base(m)
{
	const video_mode &mode = m.get_current_mode();

	unsigned int vmem;
	unsigned int vmem_size;

	depth = mode.depth;

	// Set absolute mouse mode
	ADBSetRelMouseMode(false);

	width = w;
	height = h;

	// Open window
	the_win = OpenWindowTags(NULL,
		WA_Left, window_x, 
		WA_Top, window_y,
		WA_InnerWidth, width, WA_InnerHeight, height,
		WA_SimpleRefresh, true,
		WA_NoCareRefresh, true,
		WA_Activate, true,
		WA_RMBTrap, true,
		WA_ReportMouse, true,
		WA_DragBar, true,
		WA_DepthGadget, true,
		WA_SizeGadget, false,
		WA_Title, (ULONG) GetString(STR_WINDOW_TITLE),
		TAG_END
	);
	if (the_win == NULL) {
		init_ok = false;
		ErrorAlert(STR_OPEN_WINDOW_ERR);
		return;
	}

	// Create bitmap ("height + 2" for safety)

	the_bitmap = p96AllocBitMap( width, height+2, 32, BMF_USERPRIVATE, NULL, RGBFB_A8R8G8B8 );

	if ( depth != VDEPTH_32BIT)
	{
		vmem_size = TrivialBytesPerRow(width, depth ) * (height + 2);

		VIDEO_BUFFER = (char *)  IExec->AllocVecTags( vmem_size,
			AVT_Type, MEMF_SHARED,
			AVT_Contiguous, TRUE,
			AVT_Lock,	TRUE,
			AVT_PhysicalAlignment, TRUE,
			TAG_END);

		vmem = (unsigned int) VIDEO_BUFFER;

		monitor.set_mac_frame_base ( (uint32)Host2MacAddr((uint8 *) VIDEO_BUFFER));

	} else {

		VIDEO_BUFFER = NULL;
		monitor.set_mac_frame_base ( (uint32)Host2MacAddr((uint8 *) p96GetBitMapAttr(the_bitmap, P96BMA_MEMORY)));

		vmem_size = p96GetBitMapAttr(the_bitmap, P96BMA_BYTESPERROW) * height ;
		vmem = p96GetBitMapAttr(the_bitmap, P96BMA_MEMORY), p96GetBitMapAttr(the_bitmap, P96BMA_MEMORY);
	}

	printf("Video mem %08X - %08X\n",vmem, vmem + vmem_size );

	if (the_bitmap == NULL) {
		init_ok = false;
		ErrorAlert(STR_NO_MEM_ERR);
		return;
	}

	// Add resolution and set VideoMonitor



	// Set FgPen and BgPen
	black_pen = ObtainBestPenA(the_win->WScreen->ViewPort.ColorMap, 0, 0, 0, NULL);
	white_pen = ObtainBestPenA(the_win->WScreen->ViewPort.ColorMap, 0xffffffff, 0xffffffff, 0xffffffff, NULL);
	SetAPen(the_win->RPort, black_pen);
	SetBPen(the_win->RPort, white_pen);
	SetDrMd(the_win->RPort, JAM2);

	init_ok = true;
}

driver_window::~driver_window()
{

	Delay(1);
	D(bug("driver_window::~driver_window() START %d\n",__LINE__));

	MutexObtain(video_mutex);

	// Window mode, free bitmap
	if (the_bitmap) {
		WaitBlit();
		p96FreeBitMap(the_bitmap);
		the_bitmap = NULL;
	}

	D(bug("driver_window::~driver_window() %d\n",__LINE__));

	// Free pens and close window
	if (the_win) {
		ReleasePen(the_win->WScreen->ViewPort.ColorMap, black_pen);
		ReleasePen(the_win->WScreen->ViewPort.ColorMap, white_pen);

		window_x = the_win -> LeftEdge;
		window_y = the_win -> TopEdge;


		// just need this, or else the sig is not freed.
		ModifyIDCMP(the_win, IDCMP_CLOSEWINDOW );
		CloseWindow(the_win);

		the_win = NULL;
	}

	D(bug("driver_window::~driver_window() %d\n",__LINE__));

	if (VIDEO_BUFFER) 
	{
		FreeVec(VIDEO_BUFFER); 
		VIDEO_BUFFER = NULL;
	}

	D(bug("driver_window::~driver_window() %d\n",__LINE__));

	MutexRelease(video_mutex);

	D(bug("driver_window::~driver_window() DONE %d\n",__LINE__));
}



driver_window_comp::driver_window_comp(Amiga_monitor_desc &m, int w, int h)
	: black_pen(-1), white_pen(-1), driver_base(m), bytes_per_row(0)
{
	struct Screen *src;
	int ScreenW;
	int ScreenH;
	int vmem_size;

	const video_mode &mode = m.get_current_mode();

	depth = mode.depth;

	if (src = LockPubScreen(NULL))
	{
		ScreenW = src -> Width;
		ScreenH = src -> Height;

		UnlockScreen(src);
	}

	// Set absolute mouse mode
	ADBSetRelMouseMode(false);

	width = w;
	height = h;

	// Open window
	the_win = OpenWindowTags(NULL,
			WA_Left, window_x, 
			WA_Top, window_y,

			WA_InnerWidth, width, 
			WA_InnerHeight, height,

			WA_MinWidth,	100,
		 	WA_MinHeight,	100,	

			WA_MaxWidth,		ScreenW,
		 	WA_MaxHeight,	ScreenH,	

			WA_SimpleRefresh, TRUE,
			WA_NoCareRefresh, TRUE,
			WA_Activate, TRUE,
			WA_RMBTrap, TRUE,
			WA_ReportMouse, TRUE,
			WA_DragBar, TRUE,
			WA_DepthGadget, TRUE,
			WA_SizeGadget, TRUE,
			WA_CloseGadget, FALSE,
			WA_Title, "Basilisk II",
			WA_IDCMP,0 ,
			TAG_END
	);

	if (the_win == NULL) {
		init_ok = false;
		ErrorAlert(STR_OPEN_WINDOW_ERR);
		return;
	}


	the_bitmap =AllocBitMap( width, height, 32, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);
	bytes_per_row = p96GetBitMapAttr(the_bitmap, P96BMA_BYTESPERROW);	
	FreeBitMap(the_bitmap);
	the_bitmap = NULL;

	vmem_size = TrivialBytesPerRow(width, depth ) * (height + 2);

	VIDEO_BUFFER = (char *)  IExec->AllocVecTags( vmem_size , 
			AVT_Type, MEMF_SHARED,
			AVT_Contiguous, TRUE,
			AVT_Lock,	TRUE,
			AVT_PhysicalAlignment, TRUE,
			TAG_END);

	printf("Video mem %08X - %08X\n",VIDEO_BUFFER, VIDEO_BUFFER + vmem_size );

	monitor.set_mac_frame_base( (uint32) Host2MacAddr((uint8 *) VIDEO_BUFFER) ) ;

	// Set FgPen and BgPen
	black_pen = ObtainBestPenA(the_win->WScreen->ViewPort.ColorMap, 0, 0, 0, NULL);
	white_pen = ObtainBestPenA(the_win->WScreen->ViewPort.ColorMap, 0xffffffff, 0xffffffff, 0xffffffff, NULL);
	SetAPen(the_win->RPort, black_pen);
	SetBPen(the_win->RPort, white_pen);
	SetDrMd(the_win->RPort, JAM2);

	init_ok = true;
}

driver_window_comp::~driver_window_comp()
{
	Delay(1);

	MutexObtain(video_mutex);

	// Window mode, free bitmap
	if (the_bitmap) {
		WaitBlit();
		FreeBitMap(the_bitmap);
		the_bitmap = NULL;
	}

	if (VIDEO_BUFFER) { FreeVec(VIDEO_BUFFER); VIDEO_BUFFER = NULL; }

	// Free pens and close window
	if (the_win) {
		ReleasePen(the_win->WScreen->ViewPort.ColorMap, black_pen);
		ReleasePen(the_win->WScreen->ViewPort.ColorMap, white_pen);

		window_x = the_win -> LeftEdge;
		window_y = the_win -> TopEdge;

		// just need this, or else the sig is not freed.
		ModifyIDCMP(the_win, IDCMP_CLOSEWINDOW );

		CloseWindow(the_win);
		the_win = NULL;
	}

	MutexRelease(video_mutex);
}

char convert_1bit_to_32bit_asm( ULONG *pal, char *from, uint32 *to,int  bytes )
{
	asm
	(
		"li 9,0 \n"

		"loop_1bit: \n"

			"lbz		0,0(%1) \n"

			"li		3,128 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip128 \n"
			"li		3,-1 \n"
			"skip128: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,64 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip64 \n"
			"li		3,-1 \n"
			"skip64: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,32 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip32 \n"
			"li		3,-1 \n"
			"skip32: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,16 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip16 \n"
			"li		3,-1 \n"
			"skip16: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,8 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip8 \n"
			"li		3,-1 \n"
			"skip8: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,4 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip4 \n"
			"li		3,-1 \n"
			"skip4: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,2 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip2 \n"
			"li		3,-1 \n"
			"skip2: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"li		3,2 \n"
			"mr		4,3 \n"
			"and		3,3,0 \n"
			"cmpw	cr7,3,4 \n"
			"blt+	cr7,skip1 \n"
			"li		3,-1 \n"
			"skip1: \n"
			"stw 		3,0(%2) \n "
			"addi	%2,%2,4 \n"

			"addi	%1,%1,1 \n"
			"addi	9,9,1 \n"

		"cmpw	cr7,9,%3 \n"
		"blt+	cr7,loop_1bit"

		::"r" (pal), "r" (from), "r" (to), "r" (bytes): "r9","r0","r3","r4"
	);
}


char convert_1bit_to_8bit( ULONG *pal, char *from, char *to,int  bytes )
{
	register int n;

	for (n=0; n<bytes;n++)
	{
		*to++ = (from[n] & 128) >>7;
		*to++ = (from[n] & 64)>>6;
		*to++ = (from[n] & 32)>>5;
		*to++ = (from[n] & 16)>>4;
		*to++ = (from[n] & 8)>>3;
		*to++ = (from[n] & 4)>>2;
		*to++ = (from[n] & 2)>>1;
		*to++ = (from[n] & 1);
	}
}


char convert_1bit_to_32bit( ULONG *pal, char *from, uint32 *to,int  bytes )
{
	register int n;

	for (n=0; n<bytes;n++)
	{
		*to++ = from[n] & 128 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 64 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 32 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 16 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 8 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 4 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 2 ? 0xFF000000 : 0xFFFFFFFF;
		*to++ = from[n] & 1 ? 0xFF000000 : 0xFFFFFFFF;
	}
}

char convert_8bit_to_32bit_db( ULONG *pal, char *from, uint32 *to,int  bytes )
{
	register int n;
	register int v;

	for (n=0; n<bytes;n++)
	{
		v = pal[from[n]];
		*to++=v;
		*to++=v;
	}
}

void convert_8bit_to_32bit_asm( ULONG *pal, char *from, uint32 *to,int  bytes )
{
	asm
	(
		"li 9,0 \n"

		"loop: \n"
			"lbz		0,0(%1) \n"
			"slwi		0,0,2\n"
			"add		3,%0,0 \n"
			"lwz		0,0(3) \n"
			"stw		0,0(%2) \n"

			"lbz		0,1(%1) \n"
			"slwi		0,0,2\n"
			"add		3,%0,0 \n"
			"lwz		0,0(3) \n"
			"stw		0,4(%2) \n"

			"lbz		0,2(%1) \n"
			"slwi		0,0,2\n"
			"add		3,%0,0 \n"
			"lwz		0,0(3) \n"
			"stw		0,8(%2) \n"

			"lbz		0,3(%1) \n"
			"slwi		0,0,2\n"
			"add		3,%0,0 \n"
			"lwz		0,0(3) \n"
			"stw		0,12(%2) \n"

			"addi	%2,%2,16 \n"
			"addi	%1,%1,4 \n"
			"addi	9,9,4 \n"
		"cmpw	cr7,9,%3 \n"
		"blt+	cr7,loop"

		::"r" (pal), "r" (from), "r" (to), "r" (bytes): "r9","r0","r3"
	);
}

void convert_8bit_to_32bit( ULONG *pal, char *from, uint32 *to,int  bytes )
{
	int n;

	for (n=0; n<bytes;n++)
	{
		to[n] = pal[from[n]];
	}
}


int driver_screen_p96::draw()
{
	char *to_mem ;
	int bytes_per_row;  
	int from_bytes_per_row;  
	int n,nn;
	struct RenderInfo ri;
	uint32 BMLock;


	if (VIDEO_BUFFER)
	{

		frame_dice ++;
		if (frame_dice >  line_skip)  frame_dice = 0;

		from_bytes_per_row = TrivialBytesPerRow(width, depth );

		if (use_p96_lock)
		{
			if (BMLock = p96LockBitMap(&the_screen -> BitMap, (UBYTE*) &ri, sizeof(ri)))
			{
				bytes_per_row = ri.BytesPerRow;
				to_mem = (char *) ri.Memory;
			}
		}
		else
		{
			bytes_per_row = p96GetBitMapAttr(&the_screen -> BitMap, P96BMA_BYTESPERROW);	
			to_mem = (char *) p96GetBitMapAttr(&the_screen ->BitMap, P96BMA_MEMORY);
			BMLock = 0;
		}

		switch (depth)
		{
			case VDEPTH_1BIT:

				for (nn=0; nn<height/line_skip;nn++)
				{
					n = frame_dice+(nn*line_skip);
					n = n <= height ? n : height-1;
					convert_1bit_to_8bit( vpal , (char *) VIDEO_BUFFER + (n*from_bytes_per_row),  (char *) to_mem + (n*bytes_per_row),  from_bytes_per_row );
				}
				break;

			case VDEPTH_8BIT:

				for (nn=0; nn<height/line_skip;nn++)
				{
					n = frame_dice+(nn*line_skip);
					n = n <= height ? n : height-1;
					CopyMemQuick( (char *) VIDEO_BUFFER + (n*from_bytes_per_row ),   (char *) to_mem + (n*bytes_per_row),  from_bytes_per_row );
				}

			case VDEPTH_32BIT:

				for (nn=0; nn<height/line_skip;nn++)
				{
					n = frame_dice+(nn*line_skip);
					n = n <= height ? n : height-1;
					CopyMemQuick( (char *) VIDEO_BUFFER + (n*from_bytes_per_row ),   (char *) to_mem + (n*bytes_per_row),  from_bytes_per_row );
				}
				break;
		}

		WaitBOVP( &the_screen -> ViewPort );

		if  (BMLock)
		{
			p96UnlockBitMap(&the_screen -> BitMap,BMLock);
		}

/*
		BltBitMapRastPort( the_bitmap, 0, 0,drv->the_win->RPort, 
			drv->the_win->BorderLeft, drv->the_win->BorderTop,
			mode.x, mode.y,0x0C0 );
*/
	}

	return 0;
}



int driver_window::draw()
{
	char *to_mem ;
	int bytes_per_row;  
	int from_bytes_per_row;  
	int n,nn;

	from_bytes_per_row = TrivialBytesPerRow(width, depth );

	frame_dice ++;
	if (frame_dice >  line_skip)  frame_dice = 0;


	switch (depth)
	{
		case VDEPTH_1BIT:

			bytes_per_row = p96GetBitMapAttr(the_bitmap, P96BMA_BYTESPERROW);	
			to_mem = (char *) p96GetBitMapAttr(the_bitmap, P96BMA_MEMORY);
			for (nn=0; nn<height/line_skip;nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= height ? n : height-1;
				convert_1bit_to_32bit( vpal , (char *) VIDEO_BUFFER + (n*from_bytes_per_row), (uint32 *)   ((char *) to_mem + (n*bytes_per_row)),  from_bytes_per_row );
			}
			break;

		case VDEPTH_8BIT:

			bytes_per_row = p96GetBitMapAttr(the_bitmap, P96BMA_BYTESPERROW);	
			to_mem = (char *) p96GetBitMapAttr(the_bitmap, P96BMA_MEMORY);
			for (nn=0; nn<height/line_skip;nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= height ? n : height-1;
				convert_8bit_to_32bit_asm( vpal , (char *) VIDEO_BUFFER + (n*from_bytes_per_row), (uint32 *)   ((char *) to_mem + (n*bytes_per_row)),  from_bytes_per_row );
			}
			break;

	}

	BltBitMapRastPort( drv->get_bitmap(), 0, 0,drv->the_win->RPort, 
		drv->the_win->BorderLeft, drv->the_win->BorderTop,
		mode.x, mode.y,0x0C0 );

	return 0;
}




int driver_window_comp::draw()
{
	#define STEP(a,xx,yy,ss,tt,ww)   P[a].x= xx; P[a].y= yy; P[a].s= ss; P[a].t= tt; P[a].w= ww;  

	float nw;
	float nh;
	int n,nn;
	int from_bytes_per_row;  
	int ww,wh;

	int error;
	float sx;
	float sy;

	float wx;
	float wy;

	struct XYSTW_Vertex3D P[6];


	if (!the_bitmap)
	{
		the_bitmap =AllocBitMap( width, height, 32, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);
		bytes_per_row = p96GetBitMapAttr(the_bitmap, P96BMA_BYTESPERROW);	
		to_mem = (char *) p96GetBitMapAttr(the_bitmap, P96BMA_MEMORY);
	}

	if (!the_bitmap) return 0xFFFFFF;
	if (!VIDEO_BUFFER) return 0xFFFFFE;


	frame_dice ++;
	if (frame_dice >  line_skip)  frame_dice = 0;

	 from_bytes_per_row = TrivialBytesPerRow(width, depth );

	switch (depth)
	{
		case VDEPTH_1BIT:
			for (nn=0; nn<(height/line_skip);nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= height ? n : height-1;
				convert_1bit_to_32bit( vpal , (char *) VIDEO_BUFFER + (n*from_bytes_per_row), (uint32 *)   ((char *) to_mem + (n*bytes_per_row)),  from_bytes_per_row );
			}
			break;

		case VDEPTH_8BIT:
			for (nn=0; nn<(height/line_skip);nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= height ? n : height-1;
				convert_8bit_to_32bit_asm( vpal , (char *) VIDEO_BUFFER + (n*from_bytes_per_row), (uint32 *)   ((char *) to_mem + (n*bytes_per_row)),  from_bytes_per_row );
			}
			break;

		case VDEPTH_32BIT:

			for (nn=0; nn<(height/ line_skip);nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= height ? n : height-1;
				CopyMemQuick( (char *) VIDEO_BUFFER + (n*from_bytes_per_row ),   (char *) to_mem + (n*bytes_per_row),  from_bytes_per_row );
			}
			break;
	}

	wx = the_win->BorderLeft + the_win -> LeftEdge;
	wy = the_win->BorderTop + the_win -> TopEdge;

	ww = the_win->Width - the_win->BorderLeft - the_win->BorderRight;
	wh = the_win->Height -  the_win->BorderTop - the_win->BorderBottom;

	STEP(0, wx, wy ,0 ,0 ,1);
	STEP(1, wx+ww,wy,width,0,1);
	STEP(2, wx+ww,wy+wh,width,height,1);

	STEP(3, wx,wy, 0,0,1);
	STEP(4, wx+ww,wy+wh,width,height,1);
	STEP(5, wx, wy+wh ,0 ,height ,1);

	error = IGraphics -> CompositeTags(COMPOSITE_Src, 
			the_bitmap, the_win->RPort -> BitMap,

			COMPTAG_VertexArray, P, 
			COMPTAG_VertexFormat,COMPVF_STW0_Present,
		    	COMPTAG_NumTriangles,2,

			COMPTAG_ScaleX, (uint32) ( (float) 0x0010000 * sx ),
			COMPTAG_ScaleY, (uint32) ( (float) 0x0010000 * sy ),

			COMPTAG_SrcAlpha, (uint32) (0x0010000 ),
			COMPTAG_Flags, COMPFLAG_SrcAlphaOverride | COMPFLAG_HardwareOnly | COMPFLAG_SrcFilter ,
			TAG_DONE);

	return error;
}

// Open Picasso96 screen
driver_screen_p96::driver_screen_p96(Amiga_monitor_desc &m, ULONG mode_id)
	: driver_base(m)
{
	int vsize;
	const video_mode &mode = m.get_current_mode();
	depth = mode.depth;


	// Set relative mouse mode
	ADBSetRelMouseMode(true);

	// Check if the mode is one we can handle
	if (!check_modeid_p96(mode_id))
	{
		init_ok = false;
		ErrorAlert(STR_WRONG_SCREEN_FORMAT_ERR);
		return;
	}

/*
	// Yes, get width and height

	switch ( p96GetModeIDAttr(mode_id, P96IDA_DEPTH))
	{
		case 1: depth = VDEPTH_1BIT; break;
		case 8: depth = VDEPTH_8BIT; break;
		case 15:
		case 16: depth = VDEPTH_16BIT; break;
		case 24:
		case 32: depth = VDEPTH_32BIT; break;
	}
*/

	width = p96GetModeIDAttr(mode_id, P96IDA_WIDTH);
	height = p96GetModeIDAttr(mode_id, P96IDA_HEIGHT);

	// Open screen
	the_screen = p96OpenScreenTags(
		P96SA_DisplayID, mode_id,
		P96SA_Title, (ULONG)GetString(STR_WINDOW_TITLE),
		P96SA_Quiet, true,
		P96SA_NoMemory, true,
		P96SA_NoSprite, true,
		P96SA_Exclusive, true,
		TAG_END
	);
	if (the_screen == NULL) {
		ErrorAlert(STR_OPEN_SCREEN_ERR);
		init_ok = false;
		return;
	}

	// Open window
	the_win = OpenWindowTags(NULL,
		WA_Left, 0,
		WA_Top, 0,
		WA_Width, width,
		WA_Height, height,
		WA_SimpleRefresh, true,
		WA_NoCareRefresh, true,
		WA_Borderless, true,
		WA_Activate, true,
		WA_RMBTrap, true,
		WA_ReportMouse, true,
		WA_CustomScreen, (ULONG)the_screen,
		TAG_END
	);

	if ( (depth!=VDEPTH_32BIT) || (!use_direct_video_for_32bit_screens) )
	{	
		vsize = TrivialBytesPerRow(width, depth ) * (height + 2);

		VIDEO_BUFFER = (char *) IExec->AllocVecTags(  vsize , 
				AVT_Type, MEMF_SHARED,
				AVT_Contiguous, TRUE,
				AVT_Lock,	TRUE,
				AVT_PhysicalAlignment, TRUE,
				TAG_END);
		monitor.set_mac_frame_base( (uint32) Host2MacAddr((uint8 *) VIDEO_BUFFER) ) ;

	} else {
		VIDEO_BUFFER = NULL;
		monitor.set_mac_frame_base ( (uint32)Host2MacAddr((uint8 *) p96GetBitMapAttr(the_screen->RastPort.BitMap, P96BMA_MEMORY)));
	}

	printf("Video memory: %08X - %08X - %d x %d\n", 
		(ULONG) Mac2HostAddr( monitor.get_mac_frame_base() ),
		(ULONG) Mac2HostAddr( monitor.get_mac_frame_base() ) + vsize,
		width, height );

	if (the_win == NULL) {
		ErrorAlert(STR_OPEN_WINDOW_ERR);
		init_ok = false;
		return;
	}

	ScreenToFront(the_screen);
	init_ok = true;
}

driver_screen_p96::~driver_screen_p96()
{
	// Close window
	if (the_win)
	{
		CloseWindow(the_win);
		the_win = NULL;
	}

	if (VIDEO_BUFFER)
	{
		FreeVec(VIDEO_BUFFER);
		VIDEO_BUFFER = NULL;
	}

	// Close screen
	if (the_screen) {
		p96CloseScreen(the_screen);
		the_screen = NULL;
	}
}


void driver_window::set_palette(uint8 *pal, int num)
{
	int n;

	// Convert palette to 32 bits virtual buffer.

	for (int i=0; i<num; i++) {
		n = i *3;
		vpal[i]=0xFF000000 + (pal[n] << 16) +  (pal[n+1] << 8) + pal[n+2]  ;
	}
}



void driver_window_comp::set_palette(uint8 *pal, int num)
{
	int n;

	// Convert palette to 32 bits virtual buffer.

	for (int i=0; i<num; i++) {
		n = i *3;
		vpal[i]=0xFF000000 + (pal[n] << 16) +  (pal[n+1] << 8) + pal[n+2]  ;
	}
}

void driver_screen_p96::set_palette(uint8 *pal, int num)
{
	// Convert palette to 32 bits
	ULONG table[2 + 256 * 3];
	table[0] = num << 16;
	table[num * 3 + 1] = 0;
	for (int i=0; i<num; i++) {
		table[i*3+1] = pal[i*3] * 0x01010101;
		table[i*3+2] = pal[i*3+1] * 0x01010101;
		table[i*3+3] = pal[i*3+2] * 0x01010101;
	}

	// And load it
	LoadRGB32(&the_screen->ViewPort, table);
}

