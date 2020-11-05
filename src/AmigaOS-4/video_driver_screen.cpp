
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
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
#include "window_icons.h"
#include "common_screen.h"

#include "video_driver_classes.h"

extern int frame_dice;
extern int32 frame_skip;
extern int32 line_skip;
extern UWORD *null_pointer;			// Blank mouse pointer data
extern UWORD *current_pointer;		// Currently visible mouse pointer data
extern BPTR video_debug_out;
extern bool check_modeid(ULONG mode_id);
extern int use_lock;
extern int render_method;
extern void window_draw_internal( driver_base *drv );
extern void window_draw_internal_no_lock( driver_base *drv );
extern void bitmap_draw_internal( driver_base *drv );
extern void window_draw_wpa ( driver_base *drv );

int driver_screen::draw()
{
	if (do_draw)
	{
		frame_dice ++;
		if (frame_dice >  line_skip)  frame_dice = 0;

		do_draw(this);
		WaitBOVP( &the_screen -> ViewPort );
	}

	return 0;
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
	APTR BMLock;

	bool bpr_is_same = false;

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

	if (video_debug_out) FPrintf(video_debug_out,"info: %s:%ld\n",__FUNCTION__,__LINE__); 

	// Check if the mode is one we can handle

	if ( ! (
		(GetDisplayInfoData( NULL, &dimInfo, sizeof(dimInfo) , DTAG_DIMS, mode_id)) &&
		(GetDisplayInfoData( NULL, &dispi, sizeof(dispi) ,  DTAG_DISP, mode_id))
	))
	{
		init_ok = false;
		return;
	}

	if (video_debug_out) FPrintf(video_debug_out,"info: %s:%ld\n",__FUNCTION__,__LINE__); 

	switch ( dimInfo.MaxDepth )
	{
		case 1: scr_depth = VDEPTH_1BIT; break;
		case 8: scr_depth = VDEPTH_8BIT; break;
		case 15:
		case 16: scr_depth = VDEPTH_16BIT; break;
		case 24:
		case 32: scr_depth = VDEPTH_32BIT; break;
	}

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

	if (video_debug_out) FPrintf(video_debug_out,"info: %s:%ld\n",__FUNCTION__,__LINE__); 

	// Open window
	the_win = OpenWindowTags(NULL,
		WA_Left, 0,
		WA_Top, 0,
		WA_Width, scr_width,		// we fill the screen with the background window.
		WA_Height, scr_height,
		WA_SimpleRefresh, true,
		WA_NoCareRefresh, true,
		WA_Borderless, true,
		WA_CloseGadget,false,
		WA_Activate, true,
		WA_RMBTrap, true,
		WA_ReportMouse, true,
		WA_CustomScreen, (ULONG)the_screen,
		TAG_END
	);

	if (the_win == NULL)
	{
		ErrorAlert(STR_OPEN_WINDOW_ERR);
		init_ok = false;
		return;
	}

	switch ( GetBitMapAttr( the_win -> RPort -> BitMap,    BMA_BITSPERPIXEL) )
	{
		case 1: scr_depth = VDEPTH_1BIT; break;
		case 8: scr_depth = VDEPTH_8BIT; break;
		case 15:
		case 16: scr_depth = VDEPTH_16BIT; break;
		case 24:
		case 32: scr_depth = VDEPTH_32BIT; break;
	}

	if (video_debug_out) FPrintf(video_debug_out,"info: %s:%ld\n",__FUNCTION__,__LINE__); 

	RectFillColor(the_win -> RPort, 0, 0, the_win -> Width, the_win -> Height, 0x00000000);

	BMLock = LockBitMapTags(&(the_screen -> BitMap), 
		LBM_BaseAddress, &to_mem,
		LBM_BytesPerRow, &to_bpr,
		TAG_END);

	if (BMLock)
	{
		// check, if direct mode not possible...

		if ((to_bpr != mode.bytes_per_row) && (render_method == rm_direct))
		{
			render_method = rm_internal;
		}

		if (to_bpr == mode.bytes_per_row) bpr_is_same = true;

		UnlockBitMap(BMLock);
	}

	if (video_debug_out) FPrintf(video_debug_out,"info: %s:%ld\n",__FUNCTION__,__LINE__); 

	if (null_pointer)
	{
		// Hide mouse pointer inside window
		SetPointer(the_win, null_pointer, 1, 16, 0, 0);
		current_pointer = null_pointer;
	}

	if ((bpr_is_same == true) || (render_method == rm_direct))
	{
		VIDEO_BUFFER = NULL;
		monitor.set_mac_frame_base( (uint32) Host2MacAddr((uint8 *) to_mem) ) ;
	}
	else
	{
		vsize = mode.bytes_per_row  * (mode.y + 2);

		VIDEO_BUFFER = (char *) AllocVecTags(  vsize , 
			AVT_Type, MEMF_SHARED,
			AVT_Contiguous, TRUE,
			AVT_Lock,	TRUE,
			AVT_PhysicalAlignment, TRUE,
			TAG_END);

		monitor.set_mac_frame_base( (uint32) Host2MacAddr((uint8 *) VIDEO_BUFFER) ) ;
	}

	do_draw = NULL;
	switch (render_method)
	{
		case rm_internal: 
			do_draw = window_draw_internal;
			break;

		case rm_wpa:
			do_draw = window_draw_wpa;
			break;
	}

	if (video_debug_out) FPrintf(video_debug_out,"info: %s:%ld\n",__FUNCTION__,__LINE__); 

	convert = (convert_type) get_convert( scr_depth, depth );

	if (video_debug_out) FPrintf(video_debug_out,"info: %s:%ld\n",__FUNCTION__,__LINE__); 

	if (convert == NULL)
	{
		ErrorAlert(STR_OPEN_WINDOW_ERR);
		init_ok = false;
		return;
	}

	if (video_debug_out) FPrintf(video_debug_out,"info: %s:%ld\n",__FUNCTION__,__LINE__); 

	if (video_debug_out) FPrintf(video_debug_out,"info: %s:%ld\n",__FUNCTION__,__LINE__); 

	init_ok = true;
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
