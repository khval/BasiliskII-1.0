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

#include <proto/dos.h>

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

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"
#include "adb.h"
#include "prefs.h"
#include "user_strings.h"
#include "video.h"

#include "video_convert.h"

#define convert_type void (*)(ULONG*, char*, char*, int)

#define DEBUG 0
#include "debug.h"

#define MAC_CURSOR_UP	0x3E
#define MAC_CURSOR_DOWN	0x3D

int last_wheel = 0;
int delta_wheel = 0;

extern bool quit_program_gui;

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
	DISPLAY_SCREEN,
};

// Global variables
static int32 frame_skip;
static int32 line_skip;
static UWORD *null_pointer = NULL;			// Blank mouse pointer data
static UWORD *current_pointer = (UWORD *)-1;		// Currently visible mouse pointer data
static struct Process *periodic_proc = NULL;		// Periodic process

extern struct Task *MainTask;				// Pointer to main task (from main_amiga.cpp)
BPTR out = 0;

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
	virtual int get_width() { return mac_width; }
	virtual int get_height() { return mac_height; }

public:
	Amiga_monitor_desc &monitor;	// Associated video monitor
	const video_mode &mode;		// Video mode handled by the driver
	BOOL init_ok;			// Initialization succeeded (we can't use exceptions because of -fomit-frame-pointer)
	ULONG vpal[256];
	struct Window *the_win;
	char *VIDEO_BUFFER;
	video_depth depth;

	int mac_width;
	int mac_height;
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
	int to_bpr;

};


class driver_screen : public driver_base {
public:
	driver_screen(Amiga_monitor_desc &m, ULONG mode_id, int w, int h);
	~driver_screen();
	virtual int draw();

	void set_palette(uint8 *pal, int num);

private:
	struct Screen *the_screen;
	struct BitMap *the_bitmap;
	char *to_mem;
	int to_bpr;
	void (*convert)( ULONG *pal, char *from, char *to,int  bytes );
};


static driver_base *drv = NULL;	// Pointer to currently used driver object


// Prototypes
static void periodic_func(void);
static void add_mode(uint32 width, uint32 height, uint32 resolution_id, uint32 bytes_per_row, video_depth depth);
static void add_modes(uint32 width, uint32 height, video_depth depth);
//static ULONG find_mode_for_depth(uint32 width, uint32 height, uint32 depth);
static ULONG bits_from_depth(video_depth depth);
static bool is_valid_modeid(int display_type, ULONG mode_id);
static bool check_modeid(ULONG mode_id);


/*
 *  Initialization
 */


uint32 find_mode_for_depth( int get_w, int get_h , uint32 depth_bits)
{
	ULONG ID;
	struct DisplayInfo dispi;
	struct DimensionInfo di;
	uint32_t w,h;
	uint32_t dx=~0,dy=~0;
	uint32_t last_dx=~0,last_dy=~0;

	uint64 a,get_a, diff_a, found_a, found_better_a;
	uint32 found_mode;

	get_a = get_w * get_h;

	found_mode = INVALID_ID;

	depth_bits = depth_bits == 32 ? 24 : depth_bits;

	for( ID = NextDisplayInfo( INVALID_ID ) ; ID !=INVALID_ID ;  ID = NextDisplayInfo( ID ) )
	{
		if (
			(GetDisplayInfoData( NULL, &di, sizeof(di) , DTAG_DIMS, ID)) &&
			(GetDisplayInfoData( NULL, &dispi, sizeof(dispi) ,  DTAG_DISP, ID))
		)
		{
			if (depth_bits == di.MaxDepth )
			{
				w =  di.Nominal.MaxX -di.Nominal.MinX +1;
				h =  di.Nominal.MaxY -di.Nominal.MinY +1;

				if ((get_w <= w) && (get_h <= h))
				{
					dx = w - get_w;
					dy = h - get_h;

					if ((dx<last_dx) &&(dy<last_dy))
					{
						found_mode = ID;

						last_dx = dx;
						last_dy = dy;
					}
				}

				if ((get_w == w) && (get_h == h))
				{
					found_mode = ID;
					break;
				}
			}
		}
	}

	return  found_mode ;
}





int add_modes(int mode_id, video_depth depth)
{
	LONG depth_bits;
	int bpr;
	struct List *ml;
	struct DisplayInfo di;
	struct DimensionInfo dimi;
	uint32_t DisplayID;

	int lw=0,lh=0;
	int w,h;

	switch (depth)
	{
		case VDEPTH_1BIT:	depth_bits = 8; break;
		case VDEPTH_8BIT: depth_bits = 8; break;
		case VDEPTH_16BIT: depth_bits = 16; break;
		case VDEPTH_32BIT: depth_bits = 32; break;
	}

	for( DisplayID = NextDisplayInfo( INVALID_ID ) ; DisplayID !=INVALID_ID ;  DisplayID = NextDisplayInfo( DisplayID ) )
	{

		if (check_modeid( DisplayID))
		{
			if (GetDisplayInfoData( NULL, &di, sizeof(di) , DTAG_DISP,  DisplayID)&&
				GetDisplayInfoData( NULL, &dimi, sizeof(dimi) , DTAG_DIMS,  DisplayID))
			{
				w =  dimi.Nominal.MaxX -dimi.Nominal.MinX +1;
				h =  dimi.Nominal.MaxY -dimi.Nominal.MinY +1;

				if (depth==VDEPTH_32BIT)
				{
					bpr =  GetBoardBytesPerRow( di.RTGBoardNum, (PIX_FMT) di.PixelFormat, w );
				}
				else
				{
					bpr =  TrivialBytesPerRow( w, depth );
				}

				if ((lw != w) && (lh != h))
				{
					lw = w; lh = h;

					if (bpr>0)
					{
						add_mode( w, h, mode_id,	 bpr, depth);
						mode_id ++;
					}
				}
			}
		}
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

	if (mode_str) {
		if (sscanf(mode_str, "win/%d/%d", &window_width, &window_height) == 2)
			default_display_type = DISPLAY_WINDOW;
		else if (sscanf(mode_str, "wic/%d/%d", &window_width, &window_height) == 2 )
			default_display_type = DISPLAY_WINDOW_COMP;
		else if (sscanf(mode_str, "scr/%d/%08x", &mode_opt,&screen_mode_id) == 2 ) {
			default_display_type = DISPLAY_SCREEN;
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

		case DISPLAY_SCREEN:

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

			mode_id = add_modes(0x80 , VDEPTH_1BIT);
			mode_id = add_modes(0x80 , VDEPTH_8BIT);
			mode_id = add_modes(0x80 , VDEPTH_16BIT);
			mode_id = add_modes(0x80 , VDEPTH_32BIT);

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

// uint32 find_best_screenmode(int RTGBoardNum, uint32 depth_bits, float aspect, int get_w, int get_h)

bool Amiga_monitor_desc::video_open()
{
	const video_mode &mode = get_current_mode();
	ULONG depth_bits = bits_from_depth(mode.depth);
	ULONG ID = find_mode_for_depth(mode.x, mode.y, depth_bits);

	show_sigs("START Amiga_monitor_desc::video_open()\n");

	D(bug("video_open/%ld: width=%ld  height=%ld  depth=%ld  ID=%08lx\n", __LINE__, mode.x, mode.y, depth_bits, ID));

	if ((ID == INVALID_ID) && ( display_type == DISPLAY_SCREEN))
	{
		display_type = DISPLAY_WINDOW;
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

		case DISPLAY_SCREEN:
			drv = new driver_screen(*this, ID, mode.x, mode.y);
			break;
	}

	MutexRelease(video_mutex);

	show_sigs("AFTER Amiga_monitor_desc::switch;()\n");

	D(bug("video_open/%ld: drv=%08lx\n", __LINE__, drv));

	if (drv == NULL)
	{
		ErrorAlert(STR_NO_VIDEO_MODE_ERR);
		return false;
	}

	D(bug("video_open/%ld: init_ok=%ld\n", __LINE__, drv->init_ok));
	if (!drv->init_ok) {
		ErrorAlert(STR_NO_VIDEO_MODE_ERR);
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
	win_port = (MsgPort*) AllocSysObjectTags(ASOT_PORT, TAG_DONE);

//	D(bug("periodic_func task: %08x\n",win_mask);

	if (win_port) {
		win_mask = 1 << win_port->mp_SigBit;
		drv->the_win->UserPort = win_port;

		ModifyIDCMP(drv->the_win, IDCMP_CLOSEWINDOW| IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_RAWKEY |  IDCMP_EXTENDEDMOUSE |
			((drv->monitor.display_type == DISPLAY_SCREEN) ? IDCMP_DELTAMOVE : 0));
	}

	D(bug("periodic_func/%ld: \n", __LINE__));

	// Start 60Hz timer for window refresh
//	if (drv->monitor.display_type == DISPLAY_WINDOW || drv->monitor.display_type == DISPLAY_WINDOW_COMP) {
		timer_port = (MsgPort*) AllocSysObjectTags(ASOT_PORT, TAG_DONE);
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

					case IDCMP_CLOSEWINDOW:
						quit_program_gui = true;
						break;

					case IDCMP_MOUSEMOVE:

						switch (drv->monitor.display_type) {
							case DISPLAY_SCREEN:

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

	if (timer_port)	FreeSysObject(ASOT_PORT,timer_port);

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


	FreeSysObject(ASOT_PORT,win_port);
								D(bug("periodic_func/%ld: \n", __LINE__));

	if (win_port_old) FreeSysObject(ASOT_PORT,win_port_old);

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
		case DISPLAY_SCREEN:
			return check_modeid(mode_id);
			break;
		default:
			return false;
			break;
	}
}


static bool check_modeid(ULONG mode_id)
{
	struct DisplayInfo dispi;
	struct DimensionInfo di;

	// Check if the mode is one we can handle

	if ( ! (
		(GetDisplayInfoData( NULL, &di, sizeof(di) , DTAG_DIMS, mode_id)) &&
		(GetDisplayInfoData( NULL, &dispi, sizeof(dispi) ,  DTAG_DISP, mode_id))
	))
	{
		return false;
	}


	uint32 depth = di.MaxDepth;
	uint32 format = dispi.PixelFormat;

	D(bug("check_modeid_p96: mode_id=%08lx  depth=%ld  format=%ld\n", mode_id, depth, format));

	switch (depth) {
		case 8:
			break;
		case 15:
		case 16:
			if (format != PIXF_R5G5B5)
				return false;
			break;
		case 24:
		case 32:
			if (format != PIXF_A8R8G8B8)
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

	mac_width = w;
	mac_height = h;

//	out = Open("CON:",MODE_NEWFILE);
//	FPrintf(out,"Hello world\n");

	// Open window
	the_win = OpenWindowTags(NULL,
		WA_Left, window_x, 
		WA_Top, window_y,
		WA_InnerWidth, mac_width, WA_InnerHeight, mac_height,
		WA_SimpleRefresh, true,
		WA_NoCareRefresh, true,
		WA_Activate, true,
		WA_RMBTrap, true,
		WA_ReportMouse, true,
		WA_DragBar, true,
		WA_DepthGadget, true,
		WA_SizeGadget, false,
		WA_CloseGadget, TRUE,
		WA_Title, (ULONG) GetString(STR_WINDOW_TITLE),
		WA_IDCMP,0 ,
		TAG_END
	);
	if (the_win == NULL) {
		init_ok = false;
		ErrorAlert(STR_OPEN_WINDOW_ERR);
		return;
	}

	// Create bitmap ("height + 2" for safety)

	the_bitmap = AllocBitMapTags( mac_width, mac_height+2, 32, 
			BMATags_PixelFormat,  PIXF_A8R8G8B8,
			BMATags_Clear, TRUE,
			BMATags_UserPrivate, TRUE,				
			TAG_END);

		vmem_size = mode.bytes_per_row * (mac_height + 2);

		VIDEO_BUFFER = (char *)  AllocVecTags( vmem_size,
			AVT_Type, MEMF_SHARED,
			AVT_Contiguous, TRUE,
			AVT_Lock,	TRUE,
			AVT_PhysicalAlignment, TRUE,
			TAG_END);

		vmem = (unsigned int) VIDEO_BUFFER;

		monitor.set_mac_frame_base ( (uint32)Host2MacAddr((uint8 *) VIDEO_BUFFER));




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
	if (out) 	Close(out);

	Delay(1);
	D(bug("driver_window::~driver_window() START %d\n",__LINE__));

	MutexObtain(video_mutex);

	// Window mode, free bitmap
	if (the_bitmap) {
		WaitBlit();
		FreeBitMap(the_bitmap);
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
	: black_pen(-1), white_pen(-1), driver_base(m)
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

	mac_width = w;
	mac_height = h;

	// Open window
	the_win = OpenWindowTags(NULL,
			WA_Left, window_x, 
			WA_Top, window_y,

			WA_InnerWidth, mac_width, 
			WA_InnerHeight, mac_height,

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
			WA_CloseGadget, TRUE,
			WA_Title, "Basilisk II",
			WA_IDCMP,0 ,
			TAG_END
	);

	if (the_win == NULL) {
		init_ok = false;
		ErrorAlert(STR_OPEN_WINDOW_ERR);
		return;
	}

	the_bitmap =AllocBitMap( mac_width, mac_height, 32, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);
	FreeBitMap(the_bitmap);
	the_bitmap = NULL;

	vmem_size = mode.bytes_per_row * (mode.y + 2);

	VIDEO_BUFFER = (char *)  AllocVecTags( vmem_size , 
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


struct RenderInfo
{
	uint32_t BytesPerRow;
	char *Memory;
};

int driver_screen::draw()
{
	char *to_mem ;
	int to_bpr;  
	int n,nn;
	struct RenderInfo ri;
	APTR BMLock;

	if (VIDEO_BUFFER)
	{

		frame_dice ++;
		if (frame_dice >  line_skip)  frame_dice = 0;

		use_p96_lock = 0;

		if (use_p96_lock)
		{
			if (BMLock = LockBitMapTags(&the_screen -> BitMap, 
				LBM_BaseAddress, &ri.BytesPerRow,
				LBM_BytesPerRow, &ri.Memory,
				TAG_END))
			{
				to_bpr = ri.BytesPerRow;
				to_mem = (char *) ri.Memory;
			}
		}
		else
		{
			to_bpr = the_bitmap -> BytesPerRow;	
			to_mem = (char *) the_bitmap -> Planes[0];
			BMLock = 0;
		}

		mode.mode.bytes_per_row = mode.mode.bytes_per_row;

		if (convert)
		{
			for (nn=0; nn<mac_height/line_skip;nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= mac_height ? n : mac_height-1;
				convert( vpal , (char *) VIDEO_BUFFER + (n*mac_bpr),  (char *) to_mem + (n*to_bpr),  mac_width );
			}
		}

		if  (BMLock)
		{
			UnlockBitMap(BMLock);
		}


		BltBitMapRastPort( the_bitmap, 0, 0,drv->the_win->RPort, 
			drv->the_win->BorderLeft, drv->the_win->BorderTop,
			mode.x, mode.y,0x0C0 );

		WaitBOVP( &the_screen -> ViewPort );
	}

	return 0;
}



int driver_window::draw()
{
	char *to_mem ;
	int to_bpr;  
	int n,nn;


	frame_dice ++;
	if (frame_dice >  line_skip)  frame_dice = 0;


	switch (depth)
	{
		case VDEPTH_1BIT:

			to_bpr = the_bitmap -> BytesPerRow;	
			to_mem = (char *) the_bitmap -> Planes[0];

			for (nn=0; nn<mac_height/line_skip;nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= mac_height ? n : mac_height-1;
				convert_1bit_to_32bit( vpal , (char *) VIDEO_BUFFER + (n*mode.bytes_per_row), (uint32 *)   ((char *) to_mem + (n*to_bpr)),  mode.x );
			}
			break;

		case VDEPTH_8BIT:

			to_bpr = the_bitmap -> BytesPerRow;	
			to_mem = (char *) the_bitmap -> Planes[0];

			for (nn=0; nn<mac_height/line_skip;nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= mac_height ? n : mac_height-1;
				convert_8bit_to_32bit_asm( vpal , (char *) VIDEO_BUFFER + (n*mode.bytes_per_row), (uint32 *)   ((char *) to_mem + (n*to_bpr)),  mode.x);
			}
			break;

		case VDEPTH_32BIT:

			to_bpr = the_bitmap -> BytesPerRow;	
			to_mem = (char *) the_bitmap -> Planes[0];

			for (nn=0; nn<(mac_height/ line_skip);nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= mac_height ? n : mac_height-1;
				CopyMemQuick( (char *) VIDEO_BUFFER + (n*mode.bytes_per_row ),   (char *) to_mem + (n*to_bpr),  mode.bytes_per_row );
			}

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
	int ww,wh;

	int error;
	float sx;
	float sy;

	float wx;
	float wy;

	struct XYSTW_Vertex3D P[6];


	if (!the_bitmap)
	{
		the_bitmap =AllocBitMap( mac_width, mac_height, 32, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);
	}

	if (the_bitmap)
	{
		to_bpr = the_bitmap -> BytesPerRow;	
		to_mem = (char *) the_bitmap -> Planes[0];
	}
	

	if (!the_bitmap) return 0xFFFFFF;
	if (!VIDEO_BUFFER) return 0xFFFFFE;


	frame_dice ++;
	if (frame_dice >  line_skip)  frame_dice = 0;

	switch (depth)
	{
		case VDEPTH_1BIT:
			for (nn=0; nn<(mac_height/line_skip);nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= mac_height ? n : mac_height-1;
				convert_1bit_to_32bit( vpal , (char *) VIDEO_BUFFER + (n*mode.bytes_per_row ), (uint32 *)   ((char *) to_mem + (n*to_bpr)),  mac_width );
			}
			break;

		case VDEPTH_8BIT:
			for (nn=0; nn<(mac_height/line_skip);nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= mac_height ? n : mac_height-1;
				convert_8bit_to_32bit_asm( vpal , (char *) VIDEO_BUFFER + (n*mode.bytes_per_row ), (uint32 *)   ((char *) to_mem + (n*to_bpr)),  mac_width );
			}
			break;

		case VDEPTH_32BIT:

			for (nn=0; nn<(mac_height/ line_skip);nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= mac_height ? n : mac_height-1;
				CopyMemQuick( (char *) VIDEO_BUFFER + (n*mode.bytes_per_row ),   (char *) to_mem + (n*to_bpr),  mode.bytes_per_row );
			}
			break;
	}

	wx = the_win->BorderLeft + the_win -> LeftEdge;
	wy = the_win->BorderTop + the_win -> TopEdge;

	ww = the_win->Width - the_win->BorderLeft - the_win->BorderRight;
	wh = the_win->Height -  the_win->BorderTop - the_win->BorderBottom;

	STEP(0, wx, wy ,0 ,0 ,1);
	STEP(1, wx+ww,wy,mac_width,0,1);
	STEP(2, wx+ww,wy+wh,mac_width,mac_height,1);

	STEP(3, wx,wy, 0,0,1);
	STEP(4, wx+ww,wy+wh,mac_width,mac_height,1);
	STEP(5, wx, wy+wh ,0 ,mac_height ,1);

	error = CompositeTags(COMPOSITE_Src, 
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

// Open Picasso screen
driver_screen::driver_screen(Amiga_monitor_desc &m, ULONG mode_id, int w,int h)
	: driver_base(m)
{
	int vsize;
	const video_mode &mode = m.get_current_mode();
	depth = mode.depth;

	int scr_width;
	int scr_height;
	int scr_depth;

	// Set relative mouse mode
	ADBSetRelMouseMode(true);

	// Check if the mode is one we can handle
	if (!check_modeid(mode_id))
	{
		init_ok = false;
		ErrorAlert(STR_WRONG_SCREEN_FORMAT_ERR);
		return;
	}

	struct DisplayInfo dispi;
	struct DimensionInfo dimInfo;

	// Check if the mode is one we can handle

	if ( ! (
		(GetDisplayInfoData( NULL, &dimInfo, sizeof(dimInfo) , DTAG_DIMS, mode_id)) &&
		(GetDisplayInfoData( NULL, &dispi, sizeof(dispi) ,  DTAG_DISP, mode_id))
	))
	{
		init_ok = false;
		return;
	}

	switch ( dimInfo.MaxDepth )
	{
		case 1: scr_depth = VDEPTH_1BIT; break;
		case 8: scr_depth = VDEPTH_8BIT; break;
		case 15:
		case 16: scr_depth = VDEPTH_16BIT; break;
		case 24:
		case 32: scr_depth = VDEPTH_32BIT; break;
	}

	mac_width = w;
	mac_height = h;

	scr_width = 1 + dimInfo.Nominal.MaxX - dimInfo.Nominal.MinX;
	scr_height = 1 + dimInfo.Nominal.MaxY - dimInfo.Nominal.MinY;

	// Open screen
	the_screen = OpenScreenTags(
		NULL,
		SA_DisplayID, mode_id,
		SA_Title, (ULONG)GetString(STR_WINDOW_TITLE),
		SA_Quiet, true,
		SA_Exclusive, true,
		TAG_END);

	if (the_screen == NULL) {
		ErrorAlert(STR_OPEN_SCREEN_ERR);
		init_ok = false;
		return;
	}

	// Open window
	the_win = OpenWindowTags(NULL,
		WA_Left, 0,
		WA_Top, 0,
		WA_Width, scr_width,		// we fill the screen with the background window.
		WA_Height, scr_height,
		WA_SimpleRefresh, true,
		WA_NoCareRefresh, true,
		WA_Borderless, true,
		WA_Activate, true,
		WA_RMBTrap, true,
		WA_ReportMouse, true,
		WA_CustomScreen, (ULONG)the_screen,
		TAG_END
	);

	if ( ! ((use_direct_video_for_32bit_screens) && (dimInfo.MaxDepth == 32)))
	{
		if (dimInfo.MaxDepth == 32)
		{
			the_bitmap =AllocBitMap( mac_width, mac_height, dimInfo.MaxDepth, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);
		}
		else
		{
			the_bitmap = AllocBitMapTags( mac_width, mac_height+2, dimInfo.MaxDepth, 
				BMATags_PixelFormat,  dispi.PixelFormat,
				BMATags_Clear, TRUE,
				BMATags_UserPrivate, TRUE,
				TAG_END);
		}
	}

	vsize = mode.bytes_per_row  * (mac_height + 2);

	VIDEO_BUFFER = (char *) AllocVecTags(  vsize , 
			AVT_Type, MEMF_SHARED,
			AVT_Contiguous, TRUE,
			AVT_Lock,	TRUE,
			AVT_PhysicalAlignment, TRUE,
			TAG_END);

	monitor.set_mac_frame_base( (uint32) Host2MacAddr((uint8 *) VIDEO_BUFFER) ) ;

	convert = NULL;

	switch (scr_depth)
	{
		case VDEPTH_8BIT:
			if (depth == VDEPTH_1BIT)	convert = (convert_type) &convert_1bit_to_8bit;
			if (depth == VDEPTH_8BIT)	convert = (convert_type) &convert_copy_8bit;
			break;
		case VDEPTH_16BIT:
			if (depth == VDEPTH_1BIT)	convert = (convert_type) &convert_1bit_to_16bit;
			if (depth == VDEPTH_8BIT)	convert = (convert_type) &convert_1bit_to_16bit;
			if (depth == VDEPTH_16BIT)	convert = (convert_type) &convert_copy_16bit;
			break;
		case VDEPTH_32BIT:
			if (depth == VDEPTH_1BIT)	convert = (convert_type) &convert_1bit_to_32bit;
			if (depth == VDEPTH_8BIT)	convert = (convert_type) &convert_8bit_to_32bit;
			if (depth == VDEPTH_16BIT)	convert = (convert_type) &convert_16bit_to_32bit;
			if (depth == VDEPTH_32BIT)	convert = (convert_type) &convert_copy_32bit;
			break;
	}

	printf("Video memory: %08X - %08X - %d x %d\n", 
		(ULONG) Mac2HostAddr( monitor.get_mac_frame_base() ),
		(ULONG) Mac2HostAddr( monitor.get_mac_frame_base() ) + vsize,
		mac_width, mac_height );

	if (convert == NULL)
	{
		ErrorAlert(STR_OPEN_WINDOW_ERR);
		init_ok = false;
		return;
	}

	if (the_win == NULL)
	{
		ErrorAlert(STR_OPEN_WINDOW_ERR);
		init_ok = false;
		return;
	}

	ScreenToFront(the_screen);
	init_ok = true;
}

driver_screen::~driver_screen()
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
		CloseScreen(the_screen);
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

void driver_screen::set_palette(uint8 *pal, int num)
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

