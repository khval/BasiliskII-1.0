
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

#ifdef _old_converts
	#include "video_convert.h"
#else
	#include <libraries/gfxconvert.h>
	#include <proto/gfxconvert.h>
	extern struct gc_functions gc;
	extern void *v_lookup;
#endif


#include "window_icons.h"
#include "common_screen.h"

#include "video_driver_classes.h"

#define DEBUG 0
#include "debug.h"

extern int32 frame_skip;
extern int32 line_skip;
extern UWORD *null_pointer;			// Blank mouse pointer data
extern UWORD *current_pointer;		// Currently visible mouse pointer data
extern BPTR video_debug_out;
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

void (*set_palette_fn)(uint8 *pal, uint32 num) = NULL;

#define AllocShared(size) AllocVecTags(size,	\
		AVT_Type, MEMF_SHARED,		\
		AVT_ClearWithValue, 0,			\
		TAG_END)

#if 0
 void set_v_lookup_16bit_le(uint8 *pal, uint32 num);
 void set_v_lookup_16bit_be(uint8 *pal, uint32 num);
 void set_v_lookup_16bit_be(uint8 *pal, uint32 num);
 void set_v_lookup_32bit_le(uint8 *pal, uint32 num);
 void set_v_lookup_32bit_be(uint8 *pal, uint32 num);
 void set_v_lookup_4bit_to_16bit_le_2pixels(uint8 *pal, uint32 num);
 void set_v_lookup_4bit_to_16bit_be_2pixels(uint8 *pal, uint32 num);
 void set_v_lookup_8bit_to_16bit_le_2pixels(uint8 *pal, uint32 num);
 void set_v_lookup_8bit_to_16bit_be_2pixels(uint8 *pal, uint32 num);
 void set_v_lookup_1bit_to_32bit_8pixels(uint8 *pal, uint32 num);
 void set_v_lookup_2bit_to_32bit_be_4pixels(uint8 *pal, uint32 num);
 void set_v_lookup_4bit_to_32bit_be_2pixels(uint8 *pal, uint32 num);
 void set_v_lookup_8bit_to_32bit_be_2pixels(uint8 *pal, uint32 num);
 void set_v_lookup_2bit_to_32bit_le_4pixels(uint8 *pal, uint32 num);
 void set_v_lookup_4bit_to_32bit_le_2pixels(uint8 *pal, uint32 num);
 void set_v_lookup_8bit_to_32bit_le_2pixels(uint8 *pal, uint32 num);
#endif

 bool refreash_v_lookup = true;

static bool refreash_all_colors = true;
static int maxpalcolors = 0;

uint32 vpal[256];		// its 32bit (max size), but it can be compressed to 15bit.

void set_palette_fn_ARGB (uint8 *pal, uint32 num) 
{
	int ii;
	uint32 *vpal32 = (uint32 *) vpal;

	num = num % maxpalcolors;

	ii = num * 3;
 	vpal32[ num ] = 0xFF000000 | (pal[ii] << 16) | (pal[ii+1] << 8) | pal[ii+2];

	refreash_v_lookup = true;		// we need to regnerate the table !!
}


void set_palette_fn_RGB16 (uint8 *pal, uint32 num) 
{
	register int r,g,b;
	register int ii;
	uint16 *vpal16 = (uint16 *) vpal;

	num = num % maxpalcolors;

	ii = num * 3;

	r =  (pal[ii] & 0xF8);
	g = (pal[ii+1] & 0xFC);
	b = pal[ii+2] ;

	vpal16[ num ] = ( r << 8) | ( g << 3) | ( b >> 3);

	refreash_v_lookup = true;		// we need to regnerate the table !!
}

void set_palette_fn_RGB16PC (uint8 *pal, uint32 num) 
{
	int r,g,b;

	int ii;
	uint16 tmp;
	uint16 *vpal16 = (uint16 *) vpal;

	num = num % maxpalcolors;

	ii = num * 3;

	r =  (pal[ii] & 0xF8);
	g = (pal[ii+1] & 0xFC);
	b = pal[ii+2] ;
	tmp = ( r << 8) | ( g << 3) | ( b >> 3);
 	vpal16[ num ] = (tmp >> 8) | (tmp <<8);

	refreash_v_lookup = true;		// we need to regnerate the table !!
}


int get_max_palette_colors( int vdepth )
{
	switch (vdepth)
	{
		case VDEPTH_1BIT: return 2;
		case VDEPTH_2BIT: return 4;
		case VDEPTH_4BIT: return 16;
		case VDEPTH_8BIT: return 256;
	}

	return 256;
}

driver_window::driver_window(Amiga_monitor_desc &m, const video_mode &mode)
	: black_pen(-1), white_pen(-1), driver_base(m)
{
	unsigned int vmem_size;
	struct DisplayInfo dispi;

	depth = mode.depth;

	// Set absolute mouse mode
	ADBSetRelMouseMode(false);

	window_x = 200;


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
		WA_ScreenTitle, ScreenTitle,
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

//	set_fn_set_palette( dispi.PixelFormat );
//	set_fn_set_palette2( mode.depth, dispi.PixelFormat );
//	show_set_palette_fn();

	maxpalcolors =	get_max_palette_colors( mode.depth );

	do_draw = window_draw_internal;
	switch (render_method)
	{
		case rm_internal: 

 //			convert = (convert_type) get_convert_v2( dispi.PixelFormat, mode.depth );

			gc = GC_GetMacFunctions( mode.depth, dispi.PixelFormat );
			convert = gc.gc_fn;

			if (v_lookup) FreeVec( v_lookup );
			v_lookup = NULL;	// maybe not allocated again.

printf("start init...\n");
			if (gc.LookupSizeInBytes) v_lookup = AllocShared( gc.LookupSizeInBytes ) ;
			if ( (gc.gc_initLookup) && (v_lookup) ) gc.gc_initLookup( vpal, v_lookup );
printf("end init...\n");

//			set_palette_fn = set_palette_fn_ARGB;
			set_palette_fn = set_palette_fn_RGB16;

			if ( convert )
			{
				const char *name;
				ULONG depth = GetBitMapAttr( the_win -> RPort -> BitMap,    BMA_DEPTH);
				the_bitmap =AllocBitMap( mode.x, mode.y+2, depth, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);	
				do_draw = bitmap_draw_internal;

#if 0
				name = get_name_converter_fn_ptr( (void *) convert );
#else
				name = GC_GetNameOfConverter( convert );
#endif
				if (video_debug_out) FPrintf(video_debug_out,"converter used : %s\n", name ? name : "<no name found>"); 

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

	do_draw = NULL;

	MutexRelease(video_mutex);
}

#if 1

int driver_window::draw()	// this should already be mutex protected.
{
	if (refreash_v_lookup)
	{
		printf("trying... to refresh v_lookup\n");

		if ( (gc.gc_initLookup) && (v_lookup) ) gc.gc_initLookup( vpal, v_lookup );
		refreash_v_lookup = false;
	}

	if (do_draw)
	{
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


void driver_window::set_palette(uint8 *pal, int num)
{
	if (set_palette_fn)
	{
		if (num >= maxpalcolors)
		{
			for (num = 0; num < maxpalcolors; num ++ )
			{
				set_palette_fn(pal, num);
			}
			return;
		}
		else
		{
			 set_palette_fn(pal, num);
		}
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

