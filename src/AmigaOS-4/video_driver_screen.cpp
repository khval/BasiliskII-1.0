
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

extern struct MsgPort *periodic_msgPort;

#define IDCMP_common IDCMP_GADGETUP | IDCMP_CLOSEWINDOW| IDCMP_MOUSEBUTTONS | 		\
		IDCMP_MOUSEMOVE | IDCMP_RAWKEY |  IDCMP_EXTENDEDMOUSE | IDCMP_DELTAMOVE	


extern void (*set_palette_fn)(uint8 *pal, int num);

extern void set_vpal_16bit_le(uint8 *pal, int num);
extern void set_vpal_16bit_be(uint8 *pal, int num);
extern void set_vpal_32bit_le(uint8 *pal, int num);
extern void set_vpal_32bit_be(uint8 *pal, int num);
void set_screen_palette_8bit(uint8 *pal, int num);

static struct Screen *_the_screen = NULL;

static bool refreash_all_colors = false;


void window_draw_internal_nop( driver_base *drv )
{
	if (drv == NULL) return;

	FPrintf( video_debug_out, "%s:%ld -- the_win %lx \n",__FUNCTION__,__LINE__, drv -> convert);
	FPrintf( video_debug_out, "%s:%ld -- the_win %lx \n",__FUNCTION__,__LINE__, drv -> the_win);

}

int driver_screen::draw()
{
	if ((do_draw) && (the_screen))
	{
		frame_dice ++;
		if (frame_dice >  line_skip)  frame_dice = 0;

		do_draw(this);
		WaitBOVP( &the_screen -> ViewPort );
	}

	return 0;
}

static void set_fn_set_palette( uint32 PixelFormat)
{
	switch (PixelFormat)
	{
		case PIXF_NONE:	// not RTG format.
		case PIXF_CLUT: 
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
				set_palette_fn = set_screen_palette_8bit;	
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

void convert_nop( char *from, char *to,int  pixels )
{
//	if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
}

// Open Picasso screen
driver_screen::driver_screen(Amiga_monitor_desc &m, const video_mode &mode, ULONG mode_id)
	: driver_base(m)
{
	int vmem_size;
	depth = mode.depth;

	int scr_width;
	int scr_height;
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

	// Check if the mode is one we can handle

	if ( ! (
		(GetDisplayInfoData( NULL, &dimInfo, sizeof(dimInfo) , DTAG_DIMS, mode_id)) &&
		(GetDisplayInfoData( NULL, &dispi, sizeof(dispi) ,  DTAG_DISP, mode_id))
	))
	{
		init_ok = false;
		return;
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

	_the_screen = the_screen;	// make local copy out side of class....

	// Open window
	the_win = OpenWindowTags(NULL,
		WA_Left, 0,
		WA_Top, 0,
		WA_Width, scr_width,		// we fill the screen with the background window.
		WA_Height, scr_height,
		WA_SimpleRefresh, true,
		WA_NoCareRefresh, true,
		WA_Borderless, false,
		WA_CloseGadget,true,
		WA_Activate, true,
		WA_RMBTrap, true,
		WA_ReportMouse, true,
		WA_UserPort,  periodic_msgPort ,
		WA_CustomScreen, (ULONG)the_screen,
		WA_IDCMP, IDCMP_common,
		TAG_END
	);

	if (the_win == NULL)
	{
		ErrorAlert(STR_OPEN_WINDOW_ERR);
		init_ok = false;
		return;
	}

	if (video_debug_out) FPrintf(video_debug_out,"info: %s:%ld -- mode %ld \n",__FUNCTION__,__LINE__, mode_id); 

	switch (dispi.PixelFormat)
	{
		case PIXF_CLUT:
			SetAPen( the_win -> RPort, 2 );
			RectFill(the_win -> RPort, 0, 0, the_win -> Width, the_win -> Height);		// need a green screen to see the screen.

			SetAPen( the_win -> RPort, 1 );
			SetBPen( the_win -> RPort, 0 );
			Move ( the_win -> RPort, 10,10);
			Text(  the_win -> RPort, "CLUT", 4);
			break;

		case PIXF_R5G6B5:
		case PIXF_R5G6B5PC:
		case PIXF_A8R8G8B8:
			RectFillColor(the_win -> RPort, 0, 0, the_win -> Width, the_win -> Height, 0xFF0000FF);		// need a green screen to see the screen.
			break;
	}

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

	if (null_pointer)
	{
		// Hide mouse pointer inside window
		SetPointer(the_win, null_pointer, 1, 16, 0, 0);
		current_pointer = null_pointer;
	}

	vmem_size = mode.bytes_per_row  * (mode.y + 2);

	VIDEO_BUFFER = (char *) AllocVecTags(  vmem_size , 
			AVT_Type, MEMF_SHARED,
			AVT_Contiguous, TRUE,
			AVT_Lock,	TRUE,
			AVT_PhysicalAlignment, TRUE,
			TAG_END);

	monitor.set_mac_frame_base( (uint32) Host2MacAddr((uint8 *) VIDEO_BUFFER) ) ;

	dispi.PixelFormat = GetBitMapAttr( the_win -> RPort -> BitMap,    BMA_PIXELFORMAT);

	set_fn_set_palette( dispi.PixelFormat );

	if (( dispi.PixelFormat == PIXF_CLUT ) && (mode.depth == VDEPTH_1BIT))
	{
		SetRGB32( &_the_screen->ViewPort, 0, 0x00000000, 0x00000000, 0x00000000 );
		SetRGB32( &_the_screen->ViewPort, 1, 0xFFFFFFFF , 0xFFFFFFFF, 0xFFFFFFFF );
		SetRGB32( &_the_screen->ViewPort, 2, 0xFFFFFFFF , 0x0000000, 0x0000000 );
		SetRGB32( &_the_screen->ViewPort, 3,  0x0000000, 0xFFFFFFFF, 0x0000000 );
		SetRGB32( &_the_screen->ViewPort, 4, 0x0000000 , 0x0000000, 0xFFFFFFFF );
		SetRGB32( &_the_screen->ViewPort, 5, 0x0000000 , 0x0000000, 0x5555555 );
		SetRGB32( &_the_screen->ViewPort, 6, 0x0000000 , 0xFFFFFFFF, 0xFFFFFFFF );
		SetRGB32( &_the_screen->ViewPort, 7, 0xFFFFFFFF , 0x0000000 , 0xFFFFFFFF );
	}

	convert = NULL;
	do_draw = window_draw_internal;
	switch (render_method)
	{
		case rm_internal: 

			convert = convert_nop;	// do nothing safe...

			convert = (convert_type) get_convert_v2( dispi.PixelFormat, mode.depth );
			if (  convert )
			{
				const char *name;
				ULONG depth = GetBitMapAttr( the_win -> RPort -> BitMap,    BMA_DEPTH);
				the_bitmap =AllocBitMap( mode.x, mode.y+2, depth, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);	
				do_draw = window_draw_internal;

				name = get_name_converter_fn_ptr( (void *) convert );

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

	refreash_all_colors = true;
	init_ok = true;
}

ULONG amiga_color_table[2 + 256 * 3];	

inline void set_screen_color(uint8 *pal, int num)
{
	register int byte_off = num *3;

	amiga_color_table[byte_off+1] = pal[byte_off] * 0x01010101;
	amiga_color_table[byte_off+2] = pal[byte_off+1] * 0x01010101;
	amiga_color_table[byte_off+3] = pal[byte_off+2] * 0x01010101;
}

void set_screen_palette_8bit(uint8 *pal, int num)
{
	if (_the_screen) 
	{
		if (num & 0xFFFFFF00)		// bad ramge
		{
			for (num = 0; num<256 ; num++) set_screen_color(pal,  num);

			amiga_color_table[0] = (256L << 16) + 0;	// load 256 colors, first colors is 0
//			LoadRGB32(&_the_screen->ViewPort, amiga_color_table);
		}
	}
	else
	{
		set_screen_color(pal,  num);
		amiga_color_table[0] = 1 << 16 | num;	// load 1 color, first colors number is [num]
//		LoadRGB32(&_the_screen->ViewPort, amiga_color_table);

		SetRGB32( &_the_screen->ViewPort, num, pal[num*3] * 0x01010101 , pal[num*3+1] * 0x01010101 , pal[num*3+2] * 0x01010101 );
	}
}

void driver_screen::set_palette(uint8 *pal, int num)
{
	if (set_palette_fn) set_palette_fn(pal, num);
}

driver_screen::~driver_screen()
{
	MutexObtain(video_mutex);

	if (the_win)
	{
		ModifyIDCMP( the_win, 0L );
		empty_que( the_win -> UserPort );

		CloseWindow(the_win);
		the_win = NULL;
	}

	if (the_screen)
	{
		CloseScreen(the_screen);
		the_screen = NULL;
		_the_screen = NULL;	// local copy, outside of class...
	}

	if (VIDEO_BUFFER)
	{
		FreeVec(VIDEO_BUFFER);
		VIDEO_BUFFER = NULL;
	}

	MutexRelease(video_mutex);
}

