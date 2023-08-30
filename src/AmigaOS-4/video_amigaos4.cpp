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
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <dos/dostags.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"
#include "adb.h"
#include "prefs.h"
#include "user_strings.h"
#include "video.h"

#define DEBUG 0
#include "debug.h"

#ifdef _L
#warning wtf
#undef _L
#endif

#define CATCOMP_NUMBERS
#include "locale/locale.h"

#include "video_convert.h"
#include "window_icons.h"
#include "common_screen.h"

#include "video_driver_classes.h"

extern const char *(*_L) (unsigned int num) ;




#define MAC_CURSOR_UP	0x3E
#define MAC_CURSOR_DOWN	0x3D
#define MAC_CURSOR_LEFT	0x3B
#define MAC_CURSOR_RIGHT	0x3C
#define MAC_OPTION_KEY 0x3A

int last_wheel = 0;
int delta_wheel = 0;

uint32 *vpal32 = NULL;
uint16 *vpal16 = NULL;

extern bool quit_program_gui;

int window_x = 0;
int window_y = 0;

extern int req(const char *title,const  char *body,const char *buttons, ULONG image);


void (*do_draw) ( driver_base *drv ) = NULL;

// ---- Default options!!! ----

int use_lock = 1;
int render_method = 0;

struct kIcon iconifyIcon = { NULL, NULL };
struct kIcon zoomIcon = { NULL, NULL };

struct windowclass window_save_state;

// ----------------------------

#if 0
 extern void show_sigs(char *txt);
#else
#define show_sigs( x )
#endif

extern APTR video_mutex ;

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
int32 frame_skip;
int32 line_skip;

int32 active_window_cpu_pri ;
int32 inactive_window_cpu_pri ;

UWORD *null_pointer = NULL;			// Blank mouse pointer data
UWORD *current_pointer = (UWORD *)-1;		// Currently visible mouse pointer data

struct MsgPort *periodic_msgPort = NULL;
static struct Process *periodic_proc = NULL;		// Periodic process

extern struct Task *main_task;				// Pointer to main task (from main_amiga.cpp)
BPTR video_debug_out = 0;

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



driver_base *drv = NULL;	// Pointer to currently used driver object


// Prototypes
static void periodic_func(void);
static void add_mode(uint32 width, uint32 height, uint32 resolution_id, uint32 bytes_per_row, video_depth depth);
static void add_modes(uint32 width, uint32 height, video_depth depth);
//static ULONG find_mode_for_depth(uint32 width, uint32 height, uint32 depth);
static ULONG mac_depth_from_amiga_depth(video_depth depth);
static ULONG amiga_depth_from_mac_depth(video_depth depth);
static bool is_valid_modeid(int display_type, ULONG mode_id);
bool check_modeid(ULONG mode_id);


/*
 *  Initialization
 */


uint32 find_mode_for_depth( int get_w, int get_h , uint32 get_depth)
{
	ULONG ID;
	struct DisplayInfo dispi;
	struct DimensionInfo di;
	uint32 w,h;
	uint32 dx=~0,dy=~0,dz=~0,dd=~0;
	uint32 last_dz=~0,last_dd=~0;

	uint32 found_mode, closed_match_found_mode;

	if (get_depth==32) get_depth = 24;

 //	if (video_debug_out) FPrintf( video_debug_out, "%s:%ld -- looking for w %ld h %ld d %ld \n",__FUNCTION__,__LINE__, get_w,get_h, get_depth );

	found_mode = INVALID_ID;
	closed_match_found_mode = INVALID_ID;

	for( ID = NextDisplayInfo( INVALID_ID ) ; ID !=INVALID_ID ;  ID = NextDisplayInfo( ID ) )
	{
		if (
			(GetDisplayInfoData( NULL, &di, sizeof(di) , DTAG_DIMS, ID)) &&
			(GetDisplayInfoData( NULL, &dispi, sizeof(dispi) ,  DTAG_DISP, ID))
		)
		{

			if (dispi.PixelFormat == PIXF_NONE) continue;

			w =  di.Nominal.MaxX -di.Nominal.MinX +1;
			h =  di.Nominal.MaxY -di.Nominal.MinY +1;

//		 	if (video_debug_out) FPrintf( video_debug_out, "%s:%ld -- w %ld h %ld d %ld \n",__FUNCTION__,__LINE__, w,h, di.MaxDepth );


			if (get_depth <= di.MaxDepth )
			{
				if ((get_w <= w) && (get_h <= h))
				{
					dx = w - get_w;
					dy = h - get_h;
					dz=dx+dy;
					dd = di.MaxDepth - get_depth;

					if ((dz<=last_dz) && (dd<=last_dd))
					{
						closed_match_found_mode = ID;
						last_dz = dz;
						last_dd =dd;

//					 	if (video_debug_out) FPrintf( video_debug_out, "%s:%ld -- w %ld h %ld d %ld -- best match \n",__FUNCTION__,__LINE__, w,h, di.MaxDepth );
					}
				}

				if ((get_w == w) && (get_h == h) && (get_depth == di.MaxDepth)) // perfect match....
				{
					found_mode = ID;
					break;
				}
			}
		}
	}

	return  found_mode != INVALID_ID ? found_mode : closed_match_found_mode  ;
}

int add_modes(int mode_id, video_depth depth)
{
	LONG depth_bits;
	int bpr1,bpr2,bpr;
	struct List *ml;
	struct DisplayInfo di;
	struct DimensionInfo dimi;
	uint32 DisplayID;

	int lw=0,lh=0;
	int w,h;

#if DEBUG==1

#warning D is found....

	if ( !video_debug_out ) video_debug_out = Open("CON:0/0/640/500",MODE_NEWFILE);
#endif

	for( DisplayID = NextDisplayInfo( INVALID_ID ) ; DisplayID !=INVALID_ID ;  DisplayID = NextDisplayInfo( DisplayID ) )
	{

		if (check_modeid( DisplayID))
		{
			if (GetDisplayInfoData( NULL, &di, sizeof(di) , DTAG_DISP,  DisplayID)&&
				GetDisplayInfoData( NULL, &dimi, sizeof(dimi) , DTAG_DIMS,  DisplayID))
			{
				w =  dimi.Nominal.MaxX -dimi.Nominal.MinX +1;
				h =  dimi.Nominal.MaxY -dimi.Nominal.MinY +1;

				if ((depth == VDEPTH_32BIT) && (di.PixelFormat == PIXF_A8R8G8B8))
				{
					bpr1 =  GetBoardBytesPerRow( di.RTGBoardNum, (PIX_FMT) di.PixelFormat, w );
					bpr2 =  TrivialBytesPerRow( w, depth );
					bpr = bpr1 > bpr2 ? bpr1 : bpr2;
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
					else
					{
						printf("%s: bad bpr\n",__FUNCTION__);
					}
				}
			}
		}
	}


	return mode_id;
}

#define AllocShared(size) AllocVecTags(size,	\ 
		AVT_Type, MEMF_SHARED,		\
		AVT_ClearWithValue, 0,			\
		TAG_END)


bool have_mac_mode( uint32 width, uint32 height, uint32 depth )
{
	// Find mode with specified dimensions
	std::vector<video_mode>::const_iterator i, end = VideoModes.end();
	for (i = VideoModes.begin(); i != end; ++i)
	{
		if (i->x == width && i->y == height && i->depth == depth)
		{
			return true;
		}
	}
	return false;
}

uint32 last_mac_id()
{
	uint32 id = 0;

	// Find mode with specified dimensions
	std::vector<video_mode>::const_iterator i, end = VideoModes.end();

	for (i = VideoModes.begin(); i != end; ++i)
		if ( i->resolution_id > id) id = i->resolution_id;

	return id;
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

#if DEBUG==1
	if (!video_debug_out ) video_debug_out = Open("CON:0/0/640/500",MODE_NEWFILE);
#else
	video_debug_out = 0;
#endif

	boot_depth = PrefsFindInt32("windowdepth");
	use_lock = PrefsFindBool("use_bitmap_lock");
	render_method = PrefsFindInt32("render_method");

	active_window_cpu_pri = PrefsFindInt32("active_window_cpu_pri");
	inactive_window_cpu_pri = PrefsFindInt32("inactive_window_cpu_pri");
//	iconify_cpu_suspend = PrefsFindInt32("windowdepth");

	frame_skip = PrefsFindInt32("active_window_frameskip") + 1;
	line_skip = PrefsFindInt32("active_window_lineskip") + 1;

	// Allocate blank mouse pointer data

	null_pointer = (UWORD *)AllocShared(12);

	if (null_pointer == NULL) {
		ErrorAlert(STR_NO_MEM_ERR);
		return false;
	}

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

	// Construct list of supported modes

	switch (default_display_type)
	{
		case DISPLAY_WINDOW:
		case DISPLAY_WINDOW_COMP:

			if (window_width<320) window_width=320;
			if (window_height<200) window_height=200;

			default_width = window_width;
			default_height = window_height;

			switch (boot_depth)
			{
				case 0: default_depth = VDEPTH_32BIT; break;
				case 1: default_depth = VDEPTH_1BIT; break;
				case 2: default_depth = VDEPTH_2BIT; break;
				case 3: default_depth = VDEPTH_4BIT; break;
				case 4: default_depth = VDEPTH_8BIT; break;
				case 5: default_depth = VDEPTH_16BIT; break;
				case 6: default_depth = VDEPTH_32BIT; break;
			}

			add_modes(window_width, window_height, VDEPTH_1BIT);
			add_modes(window_width, window_height, VDEPTH_2BIT);
			add_modes(window_width, window_height, VDEPTH_4BIT);
			add_modes(window_width, window_height, VDEPTH_8BIT);
			add_modes(window_width, window_height, VDEPTH_16BIT);
			add_modes(window_width, window_height, VDEPTH_32BIT);

			if (have_mac_mode( default_width, default_height, default_depth) == false)
			{
				int n = last_mac_id() + 1;
				add_mode(default_width, default_height, n, TrivialBytesPerRow(default_width, VDEPTH_1BIT), VDEPTH_1BIT);
				add_mode(default_width, default_height, n, TrivialBytesPerRow(default_width, VDEPTH_2BIT), VDEPTH_2BIT);
				add_mode(default_width, default_height, n, TrivialBytesPerRow(default_width, VDEPTH_4BIT), VDEPTH_4BIT);
				add_mode(default_width, default_height, n, TrivialBytesPerRow(default_width, VDEPTH_8BIT), VDEPTH_8BIT);
				add_mode(default_width, default_height, n, TrivialBytesPerRow(default_width, VDEPTH_16BIT), VDEPTH_16BIT);
				add_mode(default_width, default_height, n, TrivialBytesPerRow(default_width, VDEPTH_32BIT), VDEPTH_32BIT);
			}


			break;

		case DISPLAY_SCREEN:

			struct DimensionInfo dimInfo;

			DisplayInfoHandle handle = FindDisplayInfo(screen_mode_id);
			if (handle == NULL) return false;
			if (GetDisplayInfoData(handle, (UBYTE *) &dimInfo, sizeof(dimInfo), DTAG_DIMS, 0) <= 0)	return false;

			default_width = 1 + dimInfo.Nominal.MaxX - dimInfo.Nominal.MinX;
			default_height = 1 + dimInfo.Nominal.MaxY - dimInfo.Nominal.MinY;

			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld -- default w %ld h %ld \n",__FUNCTION__,__LINE__, default_width,default_height);

			switch (boot_depth)
			{
				case 0: default_depth = VDEPTH_32BIT; break;
				case 1: default_depth = VDEPTH_1BIT; break;
				case 2: default_depth = VDEPTH_2BIT; break;
				case 3: default_depth = VDEPTH_4BIT; break;
				case 4: default_depth = VDEPTH_8BIT; break;
				case 5: default_depth = VDEPTH_16BIT; break;
				case 6: default_depth = VDEPTH_32BIT; break;
			}

#if 1
			add_modes(0x80, VDEPTH_1BIT);
			add_modes(0x80, VDEPTH_2BIT);
			add_modes(0x80, VDEPTH_4BIT);
			add_modes(0x80, VDEPTH_8BIT);
			add_modes(0x80, VDEPTH_16BIT);
			add_modes(0x80, VDEPTH_32BIT);
#endif

			break;
	}


#if DEBUG==1
	bug("Available video modes:\n");
	vector<video_mode>::const_iterator i = VideoModes.begin(), end = VideoModes.end();
	while (i != end) {
		bug(" %ld x %ld (ID %02lx), %ld colors\n", i->x, i->y, i->resolution_id, 1 << bits_from_depth(i->depth));
		++i;
	}
#endif

	D(bug("VideoInit/%ld: def_width=%ld  def_height=%ld  def_depth=%ld\n", \
		__LINE__, default_width, default_height, default_depth));

	// Find requested default mode and open display
	if (VideoModes.size() == 1)
	{
		uint32 default_id ;

		if (video_debug_out) FPrintf( video_debug_out, "%s:%ld - VideoModes size is 1 \n",__FUNCTION__,__LINE__);

		// Create Amiga_monitor_desc for this (the only) display
		default_id = VideoModes[0].resolution_id;
		D(bug("VideoInit/%ld: default_id=%ld\n", __LINE__, default_id));

		Amiga_monitor_desc *monitor = new Amiga_monitor_desc(VideoModes, default_depth, default_id, default_display_type);
		if (monitor)
		{
			VideoMonitors.push_back(monitor);

			// Open display
			return monitor->video_open();
		}
		else return false;

	} else {

		if (video_debug_out) D(FPrintf)( video_debug_out, "%s:%ld - VideoModes size is not 1 \n",__FUNCTION__,__LINE__);


		// Find mode with specified dimensions
		std::vector<video_mode>::const_iterator i, end = VideoModes.end();
		for (i = VideoModes.begin(); i != end; ++i)
		{
			D(bug("VideoInit/%ld: w=%ld  h=%ld  d=%ld\n", __LINE__, i->x, i->y, bits_from_depth(i->depth)));
			if (i->x == default_width && i->y == default_height && i->depth == default_depth)
			{
				// Create Amiga_monitor_desc for this (the only) display
				uint32 default_id = i->resolution_id;
				D(bug("VideoInit/%ld: default_id=%ld default display type %d\n", __LINE__, default_id, default_display_type));

				Amiga_monitor_desc *monitor = new Amiga_monitor_desc(VideoModes, default_depth, default_id, default_display_type);
				if (monitor)
				{
					VideoMonitors.push_back(monitor);
					// Open display
					return monitor->video_open();
				}
			}
		}

		// Create Amiga_monitor_desc for this (the only) display
		uint32 default_id = VideoModes[0].resolution_id;
		D(bug("VideoInit/%ld: default_id=%ld\n", __LINE__, default_id));
		Amiga_monitor_desc *monitor = new Amiga_monitor_desc(VideoModes, default_depth, default_id, default_display_type);

		if (monitor)
		{
			VideoMonitors.push_back(monitor);

			// Open display
			return monitor->video_open();
		}
		else return false;
	}

	return true;
}

// uint32 find_best_screenmode(int RTGBoardNum, uint32 depth_bits, float aspect, int get_w, int get_h)

bool Amiga_monitor_desc::video_open()
{
#if DEBUG==1
	if ( !video_debug_out ) video_debug_out = Open("CON:",MODE_NEWFILE);
#endif

	// Start periodic process
	periodic_proc = CreateNewProcTags(
		NP_Entry, (ULONG)periodic_func,
		NP_Name, (ULONG)"Basilisk II IDCMP Handler",
		NP_Priority, 0,
		TAG_END
	);

	if (periodic_proc == NULL) {
		ErrorAlert(STR_NO_MEM_ERR);
		return false;
	}

	while ( periodic_msgPort ==  NULL) Delay(1);	// wait for a public window msgport...

	const video_mode &mode = get_current_mode();
	ULONG depth_bits = amiga_depth_from_mac_depth(mode.depth);
	ULONG ID = find_mode_for_depth(mode.x, mode.y, depth_bits);

	if ((ID == INVALID_ID) && ( display_type == DISPLAY_SCREEN))
	{
		display_type = DISPLAY_WINDOW;
	}

	MutexObtain(video_mutex);

	// Open display
	switch (display_type) {
		case DISPLAY_WINDOW:
			drv = new driver_window(*this, mode);
			break;

		case DISPLAY_WINDOW_COMP:
			drv = new driver_window_comp(*this, mode);
			break;

		case DISPLAY_SCREEN:
			drv = new driver_screen(*this, mode, ID);
			break;
	}

	MutexRelease(video_mutex);

	if (drv == NULL)
	{
		ErrorAlert(STR_NO_VIDEO_MODE_ERR);
		return false;
	}

	if (!drv->init_ok) {
		ErrorAlert(STR_NO_VIDEO_MODE_ERR);
		delete drv;
		drv = NULL;
		return false;
	}

	return true;
}


void Amiga_monitor_desc::video_close()
{
	// Close window / screen.

	delete drv;
	drv = NULL;

	// if window is closed, periodic function can delete msgport.

	// Stop periodic process
	if (periodic_proc) {
		SetSignal(0, SIGF_SINGLE);
		Signal(&periodic_proc->pr_Task, SIGBREAKF_CTRL_C);
		Wait(SIGF_SINGLE);
	}

	// clean up if we used a lookup....
	if (lookup16bit) free(lookup16bit);
	lookup16bit = NULL;
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


	if (vpal16)
	{
		FreeVec(vpal16);
		vpal16 = NULL;
	}

	if (vpal32) 
	{
		FreeVec(vpal32);
		vpal32 = NULL;
	}

	if (null_pointer)
	{
		FreeVec(null_pointer);
		null_pointer = NULL;
	}

	if (video_debug_out) 
	{
		Close(video_debug_out);
		video_debug_out = NULL;
	}

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
	if (drv == NULL) return;
	if (drv -> the_win == NULL) return;

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

		mx = ( mx * drv -> mode.x / ww );
		my = ( my * drv -> mode.y / wh );

		ADBMouseMoved(mx, my);
	}
}


void empty_que(struct MsgPort *win_port)
{
	struct IntuiMessage *msg;

	while (msg = (struct IntuiMessage *) GetMsg(win_port))
	{
		ReplyMsg((struct Message *) msg);
	}
}

void TheCloseWindow(struct Window *win)
{
 	dispose_icon( win, &iconifyIcon);
	CloseWindow( win );
}

extern struct MsgPort *iconifyPort;

void wait_for_uniconify()
{
	struct Message *msg;

	SuspendTask(main_task,0);

	if (iconifyPort)
	{
		 Wait(1 << iconifyPort->mp_SigBit);

		// empty que.
		while (msg = (Message *) GetMsg( iconifyPort ) )
		{
			ReplyMsg( (Message*) msg );
		}
	}

	RestartTask(main_task,0);
}


static void periodic_func(void)
{
	ULONG win_mask;
	struct MsgPort *timer_port = NULL;
	struct timerequest *timer_io = NULL;
	struct IntuiMessage *msg;
	ULONG  timer_mask = 0;
	int error;
	int mx,my;
	UWORD GadgetID;

	periodic_msgPort = (MsgPort*) AllocSysObjectTags(ASOT_PORT, TAG_DONE);
	win_mask = 1 << periodic_msgPort->mp_SigBit;

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

	// Main loop
	for (;;) {
		const video_mode &mode = drv->monitor.get_current_mode();

		// Wait for timer and/or window (CTRL_C is used for quitting the task)
		ULONG sig = Wait( win_mask | timer_mask | SIGBREAKF_CTRL_C);

		if (sig & SIGBREAKF_CTRL_C)	break;

		if (sig & timer_mask)
		{
			MutexObtain(video_mutex);
			if (drv) error = drv->draw();
			MutexRelease(video_mutex);

			// Restart timer
			timer_io->tr_node.io_Command = TR_ADDREQUEST;
			timer_io->tr_time.tv_secs = 0;
			timer_io->tr_time.tv_micro = 16667 * frame_skip;
			SendIO((struct IORequest *)timer_io);
		}

		if (sig & win_mask)
		{
			bool que_emptied = false;

			MutexObtain(video_mutex);		// we can't risk window being closed, while we are reading events.

			// Handle window messages
			while (msg = (struct IntuiMessage *)GetMsg( periodic_msgPort ))
			{
				// Get data from message and reply
				ULONG cl = msg->Class;
				UWORD code = msg->Code;
				UWORD qualifier = msg->Qualifier;
				WORD mouseX = msg -> MouseX;

				if ( cl == IDCMP_GADGETUP) 
				{
					GadgetID = ((struct Gadget *) ( msg -> IAddress)) -> GadgetID ;
				}
				else
				{
					GadgetID = 0;
				}

				struct IntuiWheelData *mouse_wheel = (struct IntuiWheelData *) msg -> IAddress;

				// Handle message according to class
				switch (cl) {

					case IDCMP_ACTIVEWINDOW:

							frame_skip = PrefsFindInt32("active_window_frameskip") +1;
							line_skip = PrefsFindInt32("active_window_lineskip") + 1;

						 	if (video_debug_out) D(FPrintf)( video_debug_out, "%s:%ld - active window\n",__FUNCTION__,__LINE__);
							if (main_task) 
							{
								Forbid();
								SetTaskPri(main_task, active_window_cpu_pri );
								Permit();
							}
							break;

					case IDCMP_INACTIVEWINDOW:

							frame_skip = PrefsFindInt32("deactive_window_frameskip") + 1;
							line_skip = PrefsFindInt32("deactive_window_lineskip") + 1;

						 	if (video_debug_out) D(FPrintf)( video_debug_out, "%s:%ld -inactive window\n",__FUNCTION__,__LINE__);
							if (main_task)
							{
								Forbid();
								SetTaskPri(main_task, inactive_window_cpu_pri );
								Permit();
							}
							break;

					case IDCMP_GADGETUP:

							switch (GadgetID)
							{
								case GID_ICONIFY:

									ReplyMsg((struct Message *)msg);

									ModifyIDCMP( drv -> the_win, 0L );	// don't allow more messages from the window.
									empty_que( drv -> the_win -> UserPort );

									enable_Iconify( drv->the_win ); 	
									drv -> kill_gfx_output();	// this will remove messages from queue...

									wait_for_uniconify();

									dispose_Iconify(); 	
									MutexObtain(video_mutex);	// try to prohibit nasty stuff...
									drv -> restore_gfx_output();
									MutexRelease(video_mutex);

									que_emptied = true;	// don't try to replay to message that is removed from queue...

									if (video_debug_out) FPrintf( video_debug_out, "%s:%ld -- Time to return... to main event loop\n",__FUNCTION__,__LINE__);

									break;
							}
							break;


					case IDCMP_CLOSEWINDOW:

						{		
							char opt_str[100];
				
							sprintf(opt_str,"%s|%s",_L(ID_CANCEL_GAD) ,_L(ID_PREFS_QUIT_GAD));

							ULONG opt =req(
									GetString(STR_WINDOW_TITLE),
									_L(MSG_QUIT_WHILE_RUNING_INFORMATION),
									opt_str, 2);

							switch (opt)
							{
								case 0:

									ModifyIDCMP( drv -> the_win, 0L );
									empty_que( drv -> the_win -> UserPort );
									quit_program_gui = true;
									break;
							}
						}
						break;

					case IDCMP_MOUSEMOVE:

						switch (drv->monitor.display_type) 
						{
							case DISPLAY_SCREEN:

								mx = msg->MouseX;
								my = msg->MouseY;
								ADBMouseMoved(mx, my);
								break;

							default:

								mx = drv->the_win->MouseX;
								my = drv->the_win->MouseY;
								set_mouse_window( mx, my);
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
			
						// LALT+LSHIFT+CONTROL 
						if ((qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_LSHIFT | IEQUALIFIER_CONTROL)) ==    (IEQUALIFIER_LALT | IEQUALIFIER_LSHIFT | IEQUALIFIER_CONTROL) )
						{
							bool is_break = false;

							switch (code)
							{
								case 0x5F:	// scroll lock
											SetInterruptFlag(INTFLAG_NMI);
											TriggerInterrupt();
											is_break = true;
											break;

								case 0x5E:	// +
											if (frame_skip<20) frame_skip++;
											break;

								case 0x4A:	// -
											if (frame_skip>0) frame_skip--;
											break;

								case 0xED:	// Print Screen
											quit_program_gui = true;
											break;
						
							}

							if (is_break) break;
						}

						switch (code)
						{
							case 112:	// home key
						
								ADBKeyDown( MAC_OPTION_KEY );
								ADBKeyDown( MAC_CURSOR_LEFT );
								ADBKeyUp( MAC_CURSOR_LEFT );
								ADBKeyUp( MAC_OPTION_KEY );
								break;

							case 113: // end key

								ADBKeyDown( MAC_OPTION_KEY );
								ADBKeyDown( MAC_CURSOR_RIGHT );
								ADBKeyUp( MAC_CURSOR_RIGHT );
								ADBKeyUp( MAC_OPTION_KEY );
								break;
						
							default:
						
								if (code & IECODE_UP_PREFIX)
								{
									ADBKeyUp(keycode2mac[code & 0x7f]);
								}
								else
								{
									ADBKeyDown(keycode2mac[code & 0x7f]);
								}
								break;
						}
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

				if (que_emptied) if (video_debug_out) FPrintf( video_debug_out, "%s:%ld -- brefore ReplyMsg\n",__FUNCTION__,__LINE__);

				if ( que_emptied  == false )	ReplyMsg((struct Message *)msg);

				if (que_emptied) if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
			}
			if (que_emptied) if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);


			MutexRelease(video_mutex);
		}
	}

	// Stop timer
	if (timer_io) {
		if (!CheckIO((struct IORequest *)timer_io))
			AbortIO((struct IORequest *)timer_io);
		WaitIO((struct IORequest *)timer_io);
		CloseDevice((struct IORequest *)timer_io);
		DeleteIORequest( (IORequest *) timer_io);
	}

	if (timer_port)	FreeSysObject(ASOT_PORT,timer_port);

	while (drv) Delay(1);	// wait for driver to close...

	if (periodic_msgPort)	FreeSysObject(ASOT_PORT,periodic_msgPort);
	periodic_msgPort = NULL;

	Signal(main_task, SIGF_SINGLE);
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


static ULONG mac_depth_from_amiga_depth(video_depth vdepth)
{
	uint32 depth = 1 << vdepth;

	switch (depth)
	{
		case 8:	return 8;
		case 16:	return 15;
		case 24:	return 32;
	}

	return depth;
}

static ULONG amiga_depth_from_mac_depth(video_depth mac_depth)
{
	uint32 depth = 1 << mac_depth;

	switch (depth)
	{
		case 1:	return 8;
		case 2:	return 8;
		case 4:	return 8;
		case 8:	return 8;
		case 15:	return 16;
		case 32:	return 24;
	}

	return depth;
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


bool check_modeid(ULONG mode_id)
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

void driver_base::kill_gfx_output()
{
}

void driver_base::restore_gfx_output()
{
}


void window_draw_internal( driver_base *drv )
{
	int n,nn;
	char *to_mem ;
	int to_bpr;  
	int to_bpp;
	APTR BMLock;
	uint32 iw,ih;
	uint32 dx,dy;

	if (drv == NULL) return;
	if (drv -> convert == NULL) return;

	GetWindowAttr( drv->the_win, WA_InnerWidth, &iw, sizeof(int32));
	GetWindowAttr( drv->the_win, WA_InnerHeight, &ih, sizeof(int32));

	dx = iw /2 - drv -> mode.x / 2;
	dy = ih /2 - drv -> mode.y / 2;

	to_bpp = GetBitMapAttr(drv->the_win->RPort ->BitMap, BMA_BYTESPERPIXEL);

	BMLock = LockBitMapTags(drv->the_win->RPort ->BitMap, 
		LBM_BaseAddress, &to_mem,
		LBM_BytesPerRow, &to_bpr,
		TAG_END);

	if (BMLock)
	{
		to_mem += (dy*to_bpr) + (dx*to_bpp);

		for (nn=0; nn<drv ->mode.y;nn++)
		{
			n = nn;
			drv -> convert( (char *) drv -> VIDEO_BUFFER + (n* drv -> mode.bytes_per_row),  (char *) to_mem + (n*to_bpr),  drv -> mode.x  );
		}

		UnlockBitMap(BMLock);
	}
}

void window_draw_internal_no_lock( driver_base *drv )
{
	int n,nn;
	char *to_mem = (char *) drv->the_win->RPort ->BitMap -> Planes[0];
	uint32 to_bpr = drv->the_win->RPort ->BitMap -> BytesPerRow;  
	uint32 to_bpp;
	APTR BMLock;
	uint32 iw,ih;
	uint32 dx,dy;

	to_bpp = GetBitMapAttr(drv->the_win->RPort ->BitMap, BMA_BYTESPERPIXEL);

	GetWindowAttr( drv->the_win, WA_InnerWidth, &iw, sizeof(int32));
	GetWindowAttr( drv->the_win, WA_InnerHeight, &ih, sizeof(int32));

	dx = iw /2 - drv -> mode.x / 2;
	dy = ih /2 - drv -> mode.y / 2;

	to_mem += (dy*to_bpr) + (dx * to_bpp);

	for (nn=0; nn<drv ->mode.y;nn++)
	{
		n = nn;
		drv -> convert( (char *) drv -> VIDEO_BUFFER + (n* drv -> mode.bytes_per_row),  (char *) to_mem + (n*to_bpr),  drv -> mode.x  );
	}
}

uint32 line_skip_start = 0;

void bitmap_draw_internal( driver_base *drv )
{
	int n,nn;
	char *to_mem ;
	int to_bpr;  
	APTR BMLock;
	uint32 iw,ih;
	uint32 dx,dy;

	GetWindowAttr( drv->the_win, WA_InnerWidth, &iw, sizeof(int32));
	GetWindowAttr( drv->the_win, WA_InnerHeight, &ih, sizeof(int32));

	dx = iw /2 - drv -> mode.x / 2;
	dy = ih /2 - drv -> mode.y / 2;

	BMLock = LockBitMapTags(drv->the_bitmap, 
		LBM_BaseAddress, &to_mem,
		LBM_BytesPerRow, &to_bpr,
		TAG_END);

	if (BMLock)
	{
		line_skip_start = (line_skip_start + 1) % line_skip;

		for (nn=line_skip_start; nn<drv ->mode.y;nn+=line_skip)
		{
			n = nn;
			drv -> convert( (char *) drv -> VIDEO_BUFFER + (n* drv -> mode.bytes_per_row),  (char *) to_mem + (n*to_bpr),  drv -> mode.x  );
		}

		UnlockBitMap(BMLock);
	}

	BltBitMapRastPort( drv->the_bitmap, 0, 0,drv->the_win->RPort, 
		dx+drv->the_win->BorderLeft, 
		dy+drv->the_win->BorderTop,
		drv->mode.x, 
		drv->mode.y,0x0C0 );
}

void bitmap_draw_internal_no_lock( driver_base *drv )
{
	int n,nn;
	char *to_mem = (char *) drv->the_bitmap -> Planes[0];
	int to_bpr = drv->the_bitmap -> BytesPerRow;  
	APTR BMLock;

	uint32 iw,ih;
	uint32 dx,dy;

	GetWindowAttr( drv->the_win, WA_InnerWidth, &iw, sizeof(int32));
	GetWindowAttr( drv->the_win, WA_InnerHeight, &ih, sizeof(int32));

	dx = iw /2 - drv -> mode.x / 2;
	dy = ih /2 - drv -> mode.y / 2;

	line_skip_start = (line_skip_start + 1) % line_skip;

	for (nn=line_skip_start; nn<drv ->mode.y;nn+=line_skip)
	{
		n = nn;
		drv -> convert( (char *) drv -> VIDEO_BUFFER + (n* drv -> mode.bytes_per_row),  (char *) to_mem + (n*to_bpr),  drv -> mode.x  );
	}

	BltBitMapRastPort( drv->the_bitmap, 0, 0,drv->the_win->RPort, 
		dx+drv->the_win->BorderLeft + dx, 
		dy+drv->the_win->BorderTop + dy,
		drv->mode.x, 
		drv->mode.y,0x0C0 );
}


void window_draw_wpa ( driver_base *drv )
{
	uint32 iw,ih;
	uint32 dx,dy;

	GetWindowAttr( drv->the_win, WA_InnerWidth, &iw, sizeof(int32));
	GetWindowAttr( drv->the_win, WA_InnerHeight, &ih, sizeof(int32));

	dx = iw /2 - drv -> mode.x / 2;
	dy = ih /2 - drv -> mode.y / 2;

	switch (drv->mode.depth)
	{
		case VDEPTH_8BIT:

			       WritePixelArray( (uint8*) drv->VIDEO_BUFFER,
					0, 0,
					drv->mode.bytes_per_row, PIXF_CLUT,
					drv->the_win->RPort, 
					drv->the_win->BorderLeft, drv->the_win->BorderTop,
					drv->mode.x, drv ->mode.y);
					break;

		case VDEPTH_16BIT:

			       WritePixelArray( (uint8*) drv->VIDEO_BUFFER,
					0, 0,
					drv->mode.bytes_per_row, PIXF_R5G6B5,
					drv->the_win->RPort, 
					drv->the_win->BorderLeft, drv->the_win->BorderTop,
					drv->mode.x, drv ->mode.y);
					break;

		case VDEPTH_32BIT:

			       WritePixelArray( (uint8*) drv->VIDEO_BUFFER,
					0, 0,
					drv->mode.bytes_per_row, PIXF_A8R8G8B8,
					drv->the_win->RPort, 
					dx + drv->the_win->BorderLeft, 
					dy + drv->the_win->BorderTop,
					drv->mode.x, drv ->mode.y);
					break;
	}
}


void *get_convert( uint32_t scr_depth, uint32_t depth )	// this thing is stupid needs to be replaced.
{
	void *convert = NULL;

	switch (scr_depth)
	{
		case VDEPTH_8BIT:
			if (depth == VDEPTH_1BIT)	convert = (void *) &convert_1bit_to_8bit;
			if (depth == VDEPTH_8BIT)	convert = (void *) &convert_copy_8bit;
			break;
		case VDEPTH_16BIT:
			if (depth == VDEPTH_1BIT)	convert = (void *) &convert_1bit_to_16bit;
			if (depth == VDEPTH_8BIT)	convert = (void *) &convert_8bit_lookup_to_16bit;

			if (depth == VDEPTH_16BIT)
			{
				init_lookup_15bit_to_16bit_le();
				if (lookup16bit)
				{
					convert = (void *) &convert_16bit_lookup_to_16bit;
				}
				else
				{
					convert = (void *) &convert_15bit_to_16bit_le;
				} 
			}

			if (depth == VDEPTH_32BIT)	convert = (void *) &convert_32bit_to_16bit_le;
			break;

		case VDEPTH_32BIT:
			if (depth == VDEPTH_1BIT)	convert = (void *) &convert_1bit_to_32bit_8pixels;
			if (depth == VDEPTH_2BIT)	convert = (void *) &convert_2bit_to_32bit_4pixels;
			if (depth == VDEPTH_4BIT)	convert = (void *) &convert_4bit_lookup_to_32bit_2pixels;
			if (depth == VDEPTH_8BIT)	convert = (void *) &convert_8bit_lookup_to_32bit_2pixels;
			if (depth == VDEPTH_16BIT)	convert = (void *) &convert_15bit_to_32bit;
			if (depth == VDEPTH_32BIT)	convert = (void *) &convert_copy_32bit;
			break;
	}

	return convert;
}


void *get_convert_v2( uint32 scr_depth, uint32 depth )
{
	void *convert = NULL;

	switch (scr_depth)
	{
		case PIXF_CLUT:

			switch (depth)
			{
				case VDEPTH_1BIT:
						init_lookup_1bit_to_8bit_8pixels();
						convert = (void *) &convert_1bit_to_8bit_8pixels;
						break;
		
				case VDEPTH_2BIT:
						init_lookup_2bit_to_8bit_8pixels();
						convert = (void *) &convert_2bit_to_8bit_8pixels;
						break;

				case VDEPTH_4BIT:
						init_lookup_4bit_to_8bit_4pixels();
						convert = (void *) &convert_4bit_to_8bit_4pixels;
						break;

				case VDEPTH_8BIT:
						convert = (void *) &convert_copy_8bit;
						break;
			}
			break;

		case PIXF_R5G6B5:

			if (depth == VDEPTH_1BIT)	convert = (void *) &convert_1bit_to_16bit;		//  black and white is the same in be and le
			if (depth == VDEPTH_2BIT)	convert = (void *) &convert_2bit_lookup_to_16bit;	// endiness is set in vpal16
			if (depth == VDEPTH_4BIT)	convert = (void *) &convert_4bit_lookup_to_16bit;	// endiness is set in vpal16
			if (depth == VDEPTH_8BIT)	convert = (void *) &convert_8bit_lookup_to_16bit;	// endiness is set in vpal16

			if (depth == VDEPTH_16BIT)
			{
				init_lookup_15bit_to_16bit_be();
				if (lookup16bit)
				{
					convert = (void *) &convert_16bit_lookup_to_16bit;
				}
				else
				{
					convert = (void *) &convert_15bit_to_16bit_be;
				} 
			}

			if (depth == VDEPTH_32BIT)	convert = (void *) &convert_32bit_to_16bit_be;
			break;

		case PIXF_R5G6B5PC:

			if (depth == VDEPTH_1BIT)	convert = (void *) &convert_1bit_to_16bit;		//  black and white is the same in be and le
			if (depth == VDEPTH_2BIT)	convert = (void *) &convert_2bit_lookup_to_16bit;	// endiness is set in vpal16
			if (depth == VDEPTH_4BIT)	convert = (void *) &convert_4bit_lookup_to_16bit;	// endiness is set in vpal16
			if (depth == VDEPTH_8BIT)	convert = (void *) &convert_8bit_lookup_to_16bit;	// endiness is set in vpal16

			if (depth == VDEPTH_16BIT)
			{
				init_lookup_15bit_to_16bit_le();
				if (lookup16bit)
				{
					convert = (void *) &convert_16bit_lookup_to_16bit; // endiness is set in vpal16
				}
				else
				{
					convert = (void *) &convert_15bit_to_16bit_le;
				} 
			}

			if (depth == VDEPTH_32BIT)	convert = (void *) &convert_32bit_to_16bit_le;
			break;


		case PIXF_A8R8G8B8:

			if (depth == VDEPTH_1BIT)	convert = (void *) &convert_1bit_to_32bit_8pixels;
			if (depth == VDEPTH_2BIT)	convert = (void *) &convert_2bit_to_32bit_4pixels;
			if (depth == VDEPTH_4BIT)	convert = (void *) &convert_4bit_lookup_to_32bit_2pixels;
			if (depth == VDEPTH_8BIT)	convert = (void *) &convert_8bit_lookup_to_32bit_2pixels;
			if (depth == VDEPTH_16BIT)	convert = (void *) &convert_15bit_to_32bit;
			if (depth == VDEPTH_32BIT)	convert = (void *) &convert_copy_32bit;
			break;
	}

	return convert;
}


