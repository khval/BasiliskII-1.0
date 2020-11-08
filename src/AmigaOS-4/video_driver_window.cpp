
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
extern int window_x;
extern int window_y;
extern void TheCloseWindow(struct Window *win);
extern struct kIcon iconifyIcon;
extern struct kIcon zoomIcon;
extern APTR video_mutex;

extern struct MsgPort *periodic_msgPort;

#define IDCMP_common IDCMP_GADGETUP | IDCMP_CLOSEWINDOW| IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_RAWKEY |  IDCMP_EXTENDEDMOUSE | IDCMP_DELTAMOVE

driver_window::driver_window(Amiga_monitor_desc &m, int w, int h)
	: black_pen(-1), white_pen(-1), driver_base(m)
{
	const video_mode &mode = m.get_current_mode();
	unsigned int vmem_size;
	ULONG scr_depth;

	depth = mode.depth;

	// Set absolute mouse mode
	ADBSetRelMouseMode(false);

	// Open window
	the_win = OpenWindowTags(NULL,
		WA_Left, window_x, 
		WA_Top, window_y,
		WA_InnerWidth, mode.x, WA_InnerHeight, mode.y,
		WA_SimpleRefresh, true,
		WA_NoCareRefresh, true,
		WA_Activate, true,
		WA_RMBTrap, true,
		WA_ReportMouse, true,
		WA_DragBar, true,
		WA_DepthGadget, true,
		WA_SizeGadget, false,
		WA_CloseGadget, TRUE,
		WA_UserPort,  periodic_msgPort ,
		WA_Title, (ULONG) GetString(STR_WINDOW_TITLE),
		WA_IDCMP, IDCMP_common,
		TAG_END
	);

	if (the_win == NULL)
	{
		init_ok = false;
		ErrorAlert(STR_OPEN_WINDOW_ERR);
		return;
	}
	else
	{
		open_icon( the_win, ICONIFYIMAGE, GID_ICONIFY, &iconifyIcon );
	}

	vmem_size = mode.bytes_per_row * (mode.y + 2);

	VIDEO_BUFFER = (char *)  AllocVecTags( vmem_size,
			AVT_Type, MEMF_SHARED,
			AVT_Contiguous, TRUE,
			AVT_Lock,	TRUE,
			AVT_PhysicalAlignment, TRUE,
			TAG_END);

	if ( VIDEO_BUFFER == NULL) {
		init_ok = false;
		ErrorAlert(STR_NO_MEM_ERR);
		return;
	}

	monitor.set_mac_frame_base ( (uint32)Host2MacAddr((uint8 *) VIDEO_BUFFER));

	switch ( GetBitMapAttr( the_win -> RPort -> BitMap,    BMA_BITSPERPIXEL) )
	{
		case 1: scr_depth = VDEPTH_1BIT; break;
		case 8: scr_depth = VDEPTH_8BIT; break;
		case 15:
		case 16: scr_depth = VDEPTH_16BIT; break;
		case 24:
		case 32: scr_depth = VDEPTH_32BIT; break;
	}

	convert = (convert_type) get_convert( scr_depth, mode.depth );

	do_draw = window_draw_internal;
	switch (render_method)
	{
		case rm_internal: 

			the_bitmap =AllocBitMap( mode.x, mode.y+2, 32, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);	
			do_draw = bitmap_draw_internal;
			break;

		case rm_wpa:
			do_draw = window_draw_wpa;
			break;
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
	MutexObtain(video_mutex);

	if (the_bitmap)
	{
		WaitBlit();
		FreeBitMap(the_bitmap);
		the_bitmap = NULL;
	}

	if (the_win)
	{
		ReleasePen(the_win->WScreen->ViewPort.ColorMap, black_pen);
		ReleasePen(the_win->WScreen->ViewPort.ColorMap, white_pen);

		window_x = the_win -> LeftEdge;
		window_y = the_win -> TopEdge;

		TheCloseWindow(the_win);
		the_win = NULL;
	}

	if (VIDEO_BUFFER) 
	{
		FreeVec(VIDEO_BUFFER); 
		VIDEO_BUFFER = NULL;
	}

	MutexRelease(video_mutex);
}

#if 1

int driver_window::draw()
{
	char *to_mem ;
	int to_bpr;  
	int n,nn;
	int min_bpr;

	MutexObtain(video_mutex);

	frame_dice ++;
	if (frame_dice >  line_skip)  frame_dice = 0;

	if (do_draw)
	{
		frame_dice ++;
		if (frame_dice >  line_skip)  frame_dice = 0;

		do_draw(this);
		WaitBOVP( &the_win -> WScreen -> ViewPort );
	}

	MutexRelease(video_mutex);

	return 0;
}
#else

int driver_window::draw()
{
	return 0;
}

#endif

void driver_window::set_palette(uint8 *pal, int num)
{
	int n;

	// Convert palette to 32 bits virtual buffer.

	for (int i=0; i<num; i++) {
		n = i *3;
		vpal[i]=0xFF000000 + (pal[n] << 16) +  (pal[n+1] << 8) + pal[n+2]  ;
	}
}


void driver_window::kill_gfx_output()
{
	MutexObtain(video_mutex);

	// Free pens and close window
	if (the_win) {
		ReleasePen(the_win->WScreen->ViewPort.ColorMap, black_pen);
		ReleasePen(the_win->WScreen->ViewPort.ColorMap, white_pen);

		window_x = the_win -> LeftEdge;
		window_y = the_win -> TopEdge;

		TheCloseWindow(the_win);

		the_win = NULL;
	}

	MutexRelease(video_mutex);
}

void driver_window::restore_gfx_output()
{
	// Set absolute mouse mode
	ADBSetRelMouseMode(false);

	// Open window
	the_win = OpenWindowTags(NULL,
		WA_Left, window_x, 
		WA_Top, window_y,
		WA_InnerWidth, mode.x, WA_InnerHeight, mode.y,
		WA_SimpleRefresh, true,
		WA_NoCareRefresh, true,
		WA_Activate, true,
		WA_RMBTrap, true,
		WA_ReportMouse, true,
		WA_DragBar, true,
		WA_DepthGadget, true,
		WA_SizeGadget, false,
		WA_CloseGadget, TRUE,
		WA_UserPort,  periodic_msgPort ,
		WA_Title, (ULONG) GetString(STR_WINDOW_TITLE),
		WA_IDCMP, IDCMP_common ,
		TAG_END
	);

	if (the_win)
	{
		open_icon( the_win, ICONIFYIMAGE, GID_ICONIFY, &iconifyIcon );
	}
}

