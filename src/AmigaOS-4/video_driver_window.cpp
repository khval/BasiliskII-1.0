
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

 void set_vpal_16bit_le(uint8 *pal, uint32 num);
 void set_vpal_16bit_be(uint8 *pal, uint32 num);
 void set_vpal_16bit_be(uint8 *pal, uint32 num);
 void set_vpal_32bit_le(uint8 *pal, uint32 num);
 void set_vpal_32bit_be(uint8 *pal, uint32 num);

 void set_vpal_4bit_to_16bit_le_2pixels(uint8 *pal, uint32 num);
 void set_vpal_4bit_to_16bit_be_2pixels(uint8 *pal, uint32 num);
 void set_vpal_8bit_to_16bit_le_2pixels(uint8 *pal, uint32 num);
 void set_vpal_8bit_to_16bit_be_2pixels(uint8 *pal, uint32 num);


 void set_vpal_1bit_to_32bit_8pixels(uint8 *pal, uint32 num);

 void set_vpal_2bit_to_32bit_be_4pixels(uint8 *pal, uint32 num);
 void set_vpal_4bit_to_32bit_be_2pixels(uint8 *pal, uint32 num);
 void set_vpal_8bit_to_32bit_be_2pixels(uint8 *pal, uint32 num);

 void set_vpal_2bit_to_32bit_le_4pixels(uint8 *pal, uint32 num);
 void set_vpal_4bit_to_32bit_le_2pixels(uint8 *pal, uint32 num);
 void set_vpal_8bit_to_32bit_le_2pixels(uint8 *pal, uint32 num);

static bool refreash_all_colors = true;

static int maxpalcolors = 0;

#define AllocShared(size) AllocVecTags(size,	\
		AVT_Type, MEMF_SHARED,		\
		AVT_ClearWithValue, 0,			\
		TAG_END)

void set_fn_set_palette2( uint32 macMode, uint32 PixelFormat)
{
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

	switch (PixelFormat)
	{
		case PIXF_NONE:	// not RTG format.
		case PIXF_CLUT: 
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
				set_palette_fn = NULL;		// not supported yet.
				break;

		case PIXF_R5G6B5:
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
	
				switch (macMode)
				{
					case VDEPTH_4BIT:
						set_palette_fn = set_vpal_4bit_to_16bit_be_2pixels;
						vpal32 = (uint32 *) AllocShared ( 4 * 16 * 16 );	// (input 2 x 4bit pixels) , 256 combos. (output 2 x 16bit pixels).
						if (vpal32) memset( vpal32, 0, 4 * 16 * 16 );
						break;

					case VDEPTH_8BIT:
						set_palette_fn = set_vpal_8bit_to_16bit_be_2pixels;
						vpal32 = (uint32 *) AllocShared ( 4 * 256 * 256 );	// (input 2 x 8bit pixels) , 65536 combos. (output 2 x 16bit pixels).
						if (vpal32) memset( vpal32, 0, 4 * 256 * 256 );
						break;

					default:
						set_palette_fn = set_vpal_16bit_be;	
						vpal16 = (uint16 *) AllocShared ( 2 * 256);	// 2 pixels , 256 colors, 1 x 16 bit pixel
				}
				break;

		case PIXF_R5G6B5PC:	
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);

				switch (macMode)
				{
					case VDEPTH_4BIT:
						set_palette_fn = set_vpal_4bit_to_16bit_le_2pixels;
						vpal32 = (uint32 *) AllocShared ( 4 * 16 * 16 );	// (input 2 x 4bit pixels) , 256 combos. (output 2 x 16bit pixels).
						if (vpal32) memset( vpal32, 0, 4 * 16 * 16 );
						break;

					case VDEPTH_8BIT:
						set_palette_fn = set_vpal_8bit_to_16bit_le_2pixels;
						vpal32 = (uint32 *) AllocShared ( 4 * 256 * 256 );	// (input 2 x 8bit pixels) , 65536 combos. (output 2 x 16bit pixels).
						if (vpal32) memset( vpal32, 0, 4 * 256 * 256 );
						break;

					default:
						set_palette_fn = set_vpal_16bit_le;	
						vpal16 = (uint16 *) AllocShared ( 2 * 256);	// 1 pixels , 256 colors, 1 x 16 bit pixel
						break;
				}
				break;

		case PIXF_A8R8G8B8: 
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);

				switch (macMode)
				{
					case VDEPTH_4BIT:
						set_palette_fn = set_vpal_4bit_to_32bit_be_2pixels;
						vpal32 = (uint32 *) AllocShared (8 * 256 );	// (input 2 x 4bit pixels) , 256 combos. (output 2 x 32bit pixels).
						if (vpal32) memset( vpal32, 0, 8 * 256 );
						break;

					case VDEPTH_8BIT:
						set_palette_fn = set_vpal_8bit_to_32bit_be_2pixels;
						vpal32 = (uint32 *) AllocShared ( 8 * 256 * 256  );	// 2 input pixel , 256 colors,  2 x 32bit output pixel. (0.5Mb)
						break;

					default:
						set_palette_fn = set_vpal_32bit_be;
						vpal32 = (uint32 *) AllocShared ( 4 * 256);	// 1 pixels , 256 colors, 1 x 32bit pixel
						break;
				}
				break;

		case PIXF_B8G8R8A8: 
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
				set_palette_fn = set_vpal_32bit_le;
				vpal32 = (uint32 *) AllocShared (sizeof(uint32) * 256  );	// 1 input pixel , 256 colors,  1 x 32bit output pixel.
				break;

		default:
			if (video_debug_out) FPrintf( video_debug_out, "%s:%ld \n",__FUNCTION__,__LINE__);
				set_palette_fn = NULL;
	}
}


extern void show_set_palette_fn();

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

//	set_fn_set_palette( dispi.PixelFormat );
	set_fn_set_palette2( mode.depth, dispi.PixelFormat );
	show_set_palette_fn();

	maxpalcolors =	get_max_palette_colors( mode.depth );

	do_draw = window_draw_internal;
	switch (render_method)
	{
		case rm_internal: 

 			convert = (convert_type) get_convert_v2( dispi.PixelFormat, mode.depth );

			if (convert == (void (*)(char*, char*, int)) convert_4bit_lookup_to_16bit) convert = (void (*)(char*, char*, int))  convert_4bit_lookup_to_16bit_2pixels; 
			if (convert == (void (*)(char*, char*, int)) convert_8bit_lookup_to_16bit) convert = (void (*)(char*, char*, int))  convert_8bit_lookup_to_16bit_2pixels; 
//			if (convert == (void (*)(char*, char*, int)) convert_4bit_to_32bit) convert = (void (*)(char*, char*, int))  convert_4bit_lookup_to_32bit_2pixels; 
//			if (convert == (void (*)(char*, char*, int)) convert_8bit_to_32bit) convert = (void (*)(char*, char*, int))  convert_8bit_lookup_to_32bit_2pixels; 

			if (  convert )
			{
				const char *name;
				ULONG depth = GetBitMapAttr( the_win -> RPort -> BitMap,    BMA_DEPTH);
				the_bitmap =AllocBitMap( mode.x, mode.y+2, depth, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);	
				do_draw = bitmap_draw_internal;

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

inline uint16 __pal_to_16bit(uint8 *pal, int num)
{
	register unsigned int n;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	n = num *3;
	r = pal[n] & 0xF8;		// 4+1 = 5 bit
	g = pal[n+1]  & 0xFC;	// 4+2 = 6 bit
	b = pal[n+2]  & 0xF8;	// 4+1 = 5 bit
	return ( (r << 8) | (g << 3) | (b >> 3));
}



void set_vpal_16bit_le(uint8 *pal, uint32 num)
{
	int n;
	register unsigned int rgb;
	register unsigned int r;
	register unsigned int g;
	register unsigned int b;

	// Convert palette to 32 bits virtual buffer.

	for (num=0;num<256;num++)
	{
		rgb = __pal_to_16bit(pal, num);
		vpal16[num] = ((rgb & 0xFF00) >> 8) | ((rgb & 0xFF) <<8);		// to LE
	}
}

void set_vpal_8bit_to_16bit_le_2pixels(uint8 *pal, uint32 num1)
{
	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	if (video_debug_out) FPrintf( video_debug_out, "%s\n",__FUNCTION__);

	// pixel [0..0],[0..255]

	for (num1=0;num1<256;num1++)
	{
		rgb = __pal_to_16bit(pal, num1);
		vpal32[num1] = ((rgb & 0xFF00) >> 8) | ((rgb & 0xFF) <<8);		// to LE
	}

	// pixel [0..255],[0..255]

	for (index=0;index<256*256;index++)
	{
		num2 = (index & 0x00FF);
		num1 = (index & 0xFF00) >> 8;

		vpal32[index] =  ((vpal32[num1] & 0xFFFF) << 16) | (vpal32[num2] & 0xFFFF) ;
	}

}

 void set_vpal_4bit_to_16bit_le_2pixels(uint8 *pal, uint32 num1)
{
	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	if (video_debug_out) FPrintf( video_debug_out, "%s\n",__FUNCTION__);

	// pixel [0..0],[0..15]

	for (num1=0;num1<16;num1++)
	{
		rgb = __pal_to_16bit(pal, num1);
		vpal32[num1] = ((rgb & 0xFF00) >> 8) | ((rgb & 0xFF) <<8);		// to LE
	}

	// pixel [0..15],[0..15]

	for (index=0;index<256;index++)		// because color 0 is not always black... (we need to redo the first 16 colors also)
	{
		num1 = (index & 0xF0) >> 4;
		num2 = (index & 0x0F);

		vpal32[index] =  ((vpal32[num1] & 0xFFFF) << 16) | (vpal32[num2] & 0xFFFF) ;
	}
}

 void set_vpal_4bit_to_16bit_be_2pixels(uint8 *pal, uint32 num1)
{
	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	if (video_debug_out) FPrintf( video_debug_out, "%s\n",__FUNCTION__);

	// pixel [0..0],[0..15]

	for (num1=0;num1<16;num1++)
	{
		vpal32[num1] = __pal_to_16bit(pal, num1);
	}

	// pixel [0..15],[0..15]

	for (index=0;index<256;index++)		// because color 0 is not always black... (we need to redo the first 16 colors also)
	{
		num1 = (index & 0xF0) >> 4;
		num2 = (index & 0x0F);

		vpal32[index] =  ((vpal32[num1] & 0xFFFF) << 16) | (vpal32[num2] & 0xFFFF) ;
	}
}


void set_vpal_16bit_be(uint8 *pal, uint32 num)
{
	// Convert palette to 32 bits virtual buffer.

	for (num=0;num<256;num++)
	{
		vpal16[num] = __pal_to_16bit(pal, num);
	}
}

void set_vpal_8bit_to_16bit_be_2pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0..0],[0..256]

	for (num1=0;num1<256;num1++)
	{
		vpal32[num1] = __pal_to_16bit(pal, num1);
	}

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256*256;index++)
	{
		num2 = index & 255;
		num1 = (index >> 8) & 255;

		vpal32[index] =  vpal32[num1] << 16 | vpal32[num2];
	}
}


#define _BW32(n) (n ? 0xFF000000 : 0xFFFFFFFF)  
#define _LE_ARGB(n) 0xFF + (pal[(n)] << 8) +  (pal[(n)+1] << 16) + (pal[(n)+2] << 24) 
#define _BE_ARGB(n) 0xFF000000 + (pal[(n)] << 16) +  (pal[(n)+1] << 8) + pal[(n)+2]   

void set_vpal_1bit_to_32bit_8pixels(uint8 *pal, uint32 num)
{
	int x;
	int n;
	uint32 *d;

	pal[0]=0xFF;
	pal[1]=0xFF;
	pal[2]=0xFF;
	pal[3]=0x00;
	pal[4]=0x00;
	pal[5]=0x00;

	for (num=0;num<256;num++)
	{
		n=num*3;
		d = vpal32 + num*8;

		d[0]= _BW32( (num>>7)&1 );
		d[1]= _BW32( (num>>6)&1 );
		d[2]= _BW32( (num>>5)&1 );
		d[3]= _BW32( (num>>4)&1 );
		d[4]= _BW32( (num>>3)&1 );
		d[5]= _BW32( (num>>2)&1 );
		d[6]= _BW32( (num>>1)&1 );
		d[7]= _BW32( num&1);
	}
}

void set_vpal_2bit_to_32bit_le_4pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2,num3,num4;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0..0],[0..256]

	for (num1=0;num1<256;num1++)
	{
		vpal32[num1*2] = _LE_ARGB( num1 );
	}

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256*256;index++)
	{
		num1 = ( (index >> 6) & 0x3)*3;
		num2 = ( (index >> 4) & 0x3)*3;
		num3 = ( (index >> 2) & 0x3)*3;
		num4 = ( (index) & 0x3)*3;

		vpal32[index*4] = vpal32[num1*2+1];
		vpal32[index*4+1] = vpal32[num2*2+1];
		vpal32[index*4+2] = vpal32[num2*2+1];
		vpal32[index*4+3] = vpal32[num2*2+1];
	}
}


void set_vpal_2bit_to_32bit_be_4pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2,num3,num4;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256;index++)
	{
		num1 = ( (index >> 6) & 0x3)*3;
		num2 = ( (index >> 4) & 0x3)*3;
		num3 = ( (index >> 2) & 0x3)*3;
		num4 = ( (index) & 0x3)*3;

		vpal32[index*4] = 0xFF000000 + (pal[num1] << 16) +  (pal[num1+1] << 8) + pal[num1+2]  ;  // ARGB

		vpal32[index*4+1] = 0xFF000000 + (pal[num2] << 16) +  (pal[num2+1] << 8) + pal[num2+2]  ;  // ARGB
		vpal32[index*4+2] = 0xFF000000 + (pal[num3] << 16) +  (pal[num3+1] << 8) + pal[num3+2]  ;  // ARGB
		vpal32[index*4+3] = 0xFF000000 + (pal[num4] << 16) +  (pal[num4+1] << 8) + pal[num4+2]  ;  // ARGB
	}
}

void set_vpal_8bit_to_32bit_le_2pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0..0],[0..256]

	for (num1=0;num1<256;num1++)
	{
		vpal32[num1*2] = _LE_ARGB( num1 );
	}

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256*256;index++)
	{
		num1 = (index >> 8) & 255;
		num2 = index & 255;

		vpal32[index*2] = vpal32[num1*2+1];
		vpal32[index*2+1] = vpal32[num2*2+1];
	}
}


void set_vpal_4bit_to_32bit_le_2pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256;index++)
	{
		num1 = ((index >> 4) & 0xF)*3;
		num2 = (index & 0xF)*3;

		vpal32[index*2] = 0xFF000000 + (pal[num1] << 16) +  (pal[num1+1] << 8) + pal[num1+2]  ;  // ARGB
		vpal32[index*2+1] = 0xFF000000 + (pal[num2] << 16) +  (pal[num2+1] << 8) + pal[num2+2]  ;  // ARGB
	}
}


void set_vpal_4bit_to_32bit_be_2pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256;index++)
	{
		num1 = ((index >> 4) & 0xF)*3;
		num2 = (index & 0xF)*3;

		vpal32[index*2] = 0xFF000000 + (pal[num1] << 16) +  (pal[num1+1] << 8) + pal[num1+2]  ;  // ARGB
		vpal32[index*2+1] = 0xFF000000 + (pal[num2] << 16) +  (pal[num2+1] << 8) + pal[num2+2]  ;  // ARGB
	}
}


void set_vpal_8bit_to_32bit_be_2pixels(uint8 *pal, uint32 num1)
{

	int index;
	int num2;
	register unsigned int rgb;
	// Convert palette to 32 bits virtual buffer.

	// pixel [0,256],[0..256]		// because color 0 is not always black... (we need to redo the first 256 colors also)

	for (index=0;index<256*256;index++)
	{
		num1 = ((index >> 8) & 255)*3;
		num2 = (index & 255)*3;

		vpal32[index*2] = 0xFF000000 + (pal[num1] << 16) +  (pal[num1+1] << 8) + pal[num1+2]  ;  // ARGB
		vpal32[index*2+1] = 0xFF000000 + (pal[num2] << 16) +  (pal[num2+1] << 8) + pal[num2+2]  ;  // ARGB
	}
}

void set_vpal_32bit_le(uint8 *pal, uint32 num)
{
	int n;

	for (num=0;num<256;num++)
	{
		n = num *3;		// BGRA
		vpal32[num]=0xFF + (pal[n] << 8) +  (pal[n+1] << 16) + (pal[n+2] << 24) ;
	}
}

void set_vpal_32bit_be(uint8 *pal, uint32 num)
{
	int n;

	for (num=0;num<256;num++)
	{
		n = num *3;		// ARGB
		vpal32[num]=0xFF000000 + (pal[n] << 16) +  (pal[n+1] << 8) + pal[n+2]  ;
	}
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

