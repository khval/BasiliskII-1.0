
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

#define IDCMP_common IDCMP_INACTIVEWINDOW | IDCMP_ACTIVEWINDOW | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW| IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_RAWKEY |  IDCMP_EXTENDEDMOUSE | IDCMP_DELTAMOVE

void (*set_palette_fn)(uint8 *pal, int num) = NULL;

 void set_vpal_16bit_le(uint8 *pal, int num);
 void set_vpal_16bit_be(uint8 *pal, int num);
 void set_vpal_32bit_le(uint8 *pal, int num);
 void set_vpal_32bit_be(uint8 *pal, int num);

bool refreash_all_colors = true;


void set_fn_set_palette( uint32 PixelFormat)
{
	switch (PixelFormat)
	{
		case PIXF_NONE:	// not RTG format.
		case PIXF_CLUT: 
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
				set_palette_fn = NULL;		// not supported yet.
				break;

		case PIXF_R5G6B5:
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
				set_palette_fn = set_vpal_16bit_be;	
				break;

		case PIXF_R5G6B5PC:	
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
				set_palette_fn = set_vpal_16bit_le;	
				break;

		case PIXF_A8R8G8B8: 
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
				set_palette_fn = set_vpal_32bit_be;
				break;

		case PIXF_B8G8R8A8: 
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
				set_palette_fn = set_vpal_32bit_le;
				break;

		default:
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
				set_palette_fn = NULL;
	}
}

driver_window::driver_window(Amiga_monitor_desc &m, const video_mode &mode)
	: black_pen(-1), white_pen(-1), driver_base(m)
{
	unsigned int vmem_size;
	struct DisplayInfo dispi;

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

	dispi.PixelFormat = GetBitMapAttr( the_win -> RPort -> BitMap,    BMA_PIXELFORMAT);

	set_fn_set_palette( dispi.PixelFormat );

	do_draw = window_draw_internal;
	switch (render_method)
	{
		case rm_internal: 

			convert = (convert_type) get_convert_v2( dispi.PixelFormat, mode.depth );
			if (  convert )
			{
				ULONG depth = GetBitMapAttr( the_win -> RPort -> BitMap,    BMA_DEPTH);
				the_bitmap =AllocBitMap( mode.x, mode.y+2, depth, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);	
				do_draw = bitmap_draw_internal;
			}
			else
			{
				init_ok = false;
				ErrorAlert(STR_OPEN_WINDOW_ERR);
				return;
			}
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

	refreash_all_colors = true;

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

int driver_window::draw()	// this should already be mutex protected.
{
	char *to_mem ;
	int to_bpr;  
	int n,nn;
	int min_bpr;

	frame_dice ++;
	if (frame_dice >  line_skip)  frame_dice = 0;

	if (do_draw)
	{
		frame_dice ++;
		if (frame_dice >  line_skip)  frame_dice = 0;

		do_draw(this);
		WaitBOVP( &the_win -> WScreen -> ViewPort );
	}

	return 0;
}
#else

int driver_window::draw()
{
	return 0;
}

#endif

void set_vpal_16bit_le(uint8 *pal, int num)
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	// Convert palette to 32 bits virtual buffer.

	if (num & 0xFFFFFF00) refreash_all_colors=true;

	if (refreash_all_colors)
	{
		for (num=0;num<256;num++)
		{
			n = num *3;
			r = pal[n] & 0xF8;		// 4+1 = 5 bit
			g = pal[n+1]  & 0xFC;	// 4+2 = 6 bit
			b = pal[n+2]  & 0xF8;	// 4+1 = 5 bit
			rgb = r << 8 | g << 3 | b >> 3;	

			vpal16[num] = ((rgb & 0xFF00) >> 8) | ((rgb & 0xFF) <<8);		// to LE
			refreash_all_colors = false;
		}
	}
	else
	{
		n = num *3;
		r = pal[n] & 0xF8;		// 4+1 = 5 bit
		g = pal[n+1]  & 0xFC;	// 4+2 = 6 bit
		b = pal[n+2]  & 0xF8;	// 4+1 = 5 bit
		rgb = r << 8 | g << 3 | b >> 3;	

		vpal16[num] = ((rgb & 0xFF00) >> 8) | ((rgb & 0xFF) <<8);		// to LE
	}
}

void set_vpal_16bit_be(uint8 *pal, int num)
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	// Convert palette to 32 bits virtual buffer.

	n = num *3;
	r = pal[n] & 0xF8;		// 4+1 = 5 bit
	g = pal[n+1]  & 0xFC;	// 4+2 = 6 bit
	b = pal[n+2]  & 0xF8;	// 4+1 = 5 bit
	vpal16[num] = r << 8 | g << 3 | b >> 3;
}

void set_vpal_32bit_le(uint8 *pal, int num)
{
	int n = num *3;		// BGRA
	vpal32[num]=0xFF + (pal[n] << 8) +  (pal[n+1] << 16) + (pal[n+2] << 24) ;
}

void set_vpal_32bit_be(uint8 *pal, int num)
{
	int n = num *3;		// ARGB
	vpal32[num]=0xFF000000 + (pal[n] << 16) +  (pal[n+1] << 8) + pal[n+2]  ;
}

void driver_window::set_palette(uint8 *pal, int num)
{
	if (set_palette_fn) set_palette_fn(pal, num);
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

