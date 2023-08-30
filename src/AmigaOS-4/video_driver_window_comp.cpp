
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

extern void BackFill_Func(struct RastPort *ArgRP, struct BackFillArgs *MyArgs);

extern struct MsgPort *periodic_msgPort;

#define IDCMP_common IDCMP_INACTIVEWINDOW | IDCMP_ACTIVEWINDOW | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW| IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_RAWKEY |  IDCMP_EXTENDEDMOUSE | IDCMP_DELTAMOVE

extern void (*set_palette_fn)(uint8 *pal, uint32 num, int maxcolors) ;

#define AllocShared(size) AllocVecTags(size,	\
		AVT_Type, MEMF_SHARED,		\
		AVT_ClearWithValue, 0,			\
		TAG_END)


static bool refreash_all_colors = true;
// extern void set_fn_set_palette( uint32 PixelFormat);
extern void set_fn_set_palette2( uint32 macMode, uint32 PixelFormat);
extern int get_max_palette_colors( int vdepth );

static void bitmap_comp_draw_internal( driver_base *drv );

static int maxpalcolors = 0;

driver_window_comp::driver_window_comp(Amiga_monitor_desc &m, const video_mode &mode)
	: black_pen(-1), white_pen(-1), driver_base(m)
{
	unsigned int vmem_size;
	struct DisplayInfo dispi;
	struct Screen *src;
	int ScreenW;
	int ScreenH;

	depth = mode.depth;

	// Set absolute mouse mode
	ADBSetRelMouseMode(false);

	if (src = LockPubScreen(NULL))
	{
		ScreenW = src -> Width;
		ScreenH = src -> Height;

		UnlockScreen(src);
	}

	// Open window
	the_win = OpenWindowTags(NULL,
		WA_Left, window_x, 
		WA_Top, window_y,
		WA_InnerWidth, mode.x, 
		WA_InnerHeight, mode.y,
		WA_MinWidth,	100,
	 	WA_MinHeight,	100,	
		WA_MaxWidth,		ScreenW,
		WA_MaxHeight,	ScreenH,	
		WA_SimpleRefresh, true,
		WA_NoCareRefresh, true,
		WA_Activate, true,
		WA_RMBTrap, true,
		WA_ReportMouse, true,
		WA_DragBar, true,
		WA_DepthGadget, true,
		WA_SizeGadget, TRUE,
		WA_SizeBBottom, TRUE,
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

//	set_fn_set_palette2( mode.depth, dispi.PixelFormat );

	maxpalcolors =	get_max_palette_colors( mode.depth );

	do_draw = window_draw_internal;
	switch (render_method)
	{
		case rm_internal: 

#ifdef _old_converts
			convert = (convert_type) get_convert_v2( dispi.PixelFormat, mode.depth );
			if (convert == (void (*)(char*, char*, int)) convert_4bit_lookup_to_16bit) convert = (void (*)(char*, char*, int))  convert_4bit_lookup_to_16bit_2pixels; 
			if (convert == (void (*)(char*, char*, int)) convert_8bit_lookup_to_16bit) convert = (void (*)(char*, char*, int))  convert_8bit_lookup_to_16bit_2pixels; 
#else
			gc = GC_GetMacFunctions( mode.depth, dispi.PixelFormat );
			convert = gc.gc_fn;

			if (v_lookup) FreeVec( v_lookup );
			v_lookup = NULL;	// maybe not allocated again.

			if (gc.LookupSizeInBytes) v_lookup = AllocShared( gc.LookupSizeInBytes ) ;

			if ( (gc.gc_initLookup) && (v_lookup) ) gc.gc_initLookup( NULL, v_lookup );
#endif

			if ( convert )
			{
				ULONG depth = GetBitMapAttr( the_win -> RPort -> BitMap,    BMA_DEPTH);
				the_bitmap =AllocBitMap( mode.x, mode.y+2, depth, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);	
				do_draw = bitmap_comp_draw_internal;
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

driver_window_comp::~driver_window_comp()
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

int driver_window_comp::draw()	// this should already be mutex protected.
{
	if (do_draw)
	{
		do_draw(this);
		WaitBOVP( &the_win -> WScreen -> ViewPort );
	}

	return 0;
}
#else

int driver_window_comp::draw()
{
	return 0;
}

#endif

#ifdef _old_converts

void set_palette_16bit_le(uint8 *pal, int num)
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

			v_lookup[num] = ((rgb & 0xFF00) >> 8) | ((rgb & 0xFF) <<8);		// to LE
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

		v_lookup[num] = ((rgb & 0xFF00) >> 8) | ((rgb & 0xFF) <<8);		// to LE
	}
}

void set_palette_16bit_be(uint8 *pal, int num)
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
	v_lookup[num] = r << 8 | g << 3 | b >> 3;
}


#define _LE_ARGB(n) 0xFF + (pal[(n)] << 8) +  (pal[(n)+1] << 16) + (pal[(n)+2] << 24) ;
#define _BE_ARGB(n) 0xFF000000 + (pal[(n)] << 16) +  (pal[(n)+1] << 8) + pal[(n)+2]  ; ;


void set_palette_32bit_4pixels_le(uint8 *pal, int num)
{
	int x;
	int n;
	uint32 *d;

	for (num=0;num<256;num++)
	{
		n=num*3;
		d = v_lookup + num*8;

		d[0]= _LE_ARGB( (n>>6)&3 );
		d[1]= _LE_ARGB( (n>>4)&3 );
		d[2]= _LE_ARGB( (n>>2)&3 );
		d[3]= _LE_ARGB( (n)&3 );
	}
}

void set_palette_32bit_4pixels_be(uint8 *pal, int num)
{
	int x;
	int n;
	uint32 *d;

	for (num=0;num<256;num++)
	{
		n=num*3;
		d = v_lookup + num*8;

		d[0]= _BE_ARGB( (n>>6)&3 );
		d[1]= _BE_ARGB( (n>>4)&3 );
		d[2]= _BE_ARGB( (n>>2)&3 );
		d[3]= _BE_ARGB( (n)&13);
	}
}

void set_palette_32bit_le(uint8 *pal, int num)
{
	int n;

	for (num=0;num<256;num++)
	{
		n=num*3;
		v_lookup[num]=0xFF + (pal[n] << 8) +  (pal[n+1] << 16) + (pal[n+2] << 24) ;
	}
}

void set_palette_32bit_be(uint8 *pal, int num)
{
	int n;

	for (num=0;num<256;num++)
	{
		n=num*3;
		v_lookup[num]=0xFF000000 + (pal[n] << 16) +  (pal[n+1] << 8) + pal[n+2]  ;
	 }
}

#endif

void driver_window_comp::set_palette(uint8 *pal, int num)
{
	if (set_palette_fn) set_palette_fn(pal, num, maxpalcolors);
}

void driver_window_comp::kill_gfx_output()
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

void driver_window_comp::restore_gfx_output()
{
	struct Screen *src;
	int ScreenW;
	int ScreenH;

	// Set absolute mouse mode
	ADBSetRelMouseMode(false);

	if (src = LockPubScreen(NULL))
	{
		ScreenW = src -> Width;
		ScreenH = src -> Height;

		UnlockScreen(src);
	}

	// Open window
	the_win = OpenWindowTags(NULL,
		WA_Left, window_x, 
		WA_Top, window_y,
		WA_InnerWidth, mode.x, 
		WA_InnerHeight, mode.y,
		WA_MinWidth,	100,
	 	WA_MinHeight,	100,	
		WA_MaxWidth,		ScreenW,
	 	WA_MaxHeight,	ScreenH,	
		WA_SimpleRefresh, true,
		WA_NoCareRefresh, true,
		WA_Activate, true,
		WA_RMBTrap, true,
		WA_ReportMouse, true,
		WA_DragBar, true,
		WA_DepthGadget, true,
		WA_SizeGadget, TRUE,
		WA_SizeBBottom, TRUE,
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

void bitmap_comp_draw_internal( driver_base *drv )
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
		for (nn=0; nn<drv ->mode.y;nn++)
		{
			n = nn;
			drv -> convert( v_lookup, (char *) drv -> VIDEO_BUFFER + (n* drv -> mode.bytes_per_row),  (char *) to_mem + (n*to_bpr),  drv -> mode.x  );
		}

		UnlockBitMap(BMLock);
	}

	BackFill_Func(NULL,NULL);
}

