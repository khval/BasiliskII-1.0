

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


struct XYSTW_Vertex3D { 
float x, y; 
float s, t, w; 
}; 

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


	the_bitmap =AllocBitMap( mode.x, mode.y, 32, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);
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
		the_bitmap =AllocBitMap( mode.x, mode.y, 32, BMF_DISPLAYABLE, the_win ->RPort -> BitMap);
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
			for (nn=0; nn<(mode.y/line_skip);nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= mode.y ? n : mode.y-1;
				convert_1bit_to_32bit( vpal , (char *) VIDEO_BUFFER + (n*mode.bytes_per_row ), (uint32 *)   ((char *) to_mem + (n*to_bpr)),  mode.x );
			}
			break;

		case VDEPTH_8BIT:
			for (nn=0; nn<(mode.y/line_skip);nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= mode.y ? n : mode.y-1;
				convert_8bit_to_32bit_asm( vpal , (char *) VIDEO_BUFFER + (n*mode.bytes_per_row ), (uint32 *)   ((char *) to_mem + (n*to_bpr)),  mode.x );
			}
			break;

		case VDEPTH_32BIT:

			for (nn=0; nn<(mode.y/ line_skip);nn++)
			{
				n = frame_dice+(nn*line_skip);
				n = n <= mode.y ? n : mode.y-1;
				CopyMemQuick( (char *) VIDEO_BUFFER + (n*mode.bytes_per_row ),   (char *) to_mem + (n*to_bpr),  mode.bytes_per_row );
			}
			break;
	}

	wx = the_win->BorderLeft + the_win -> LeftEdge;
	wy = the_win->BorderTop + the_win -> TopEdge;

	ww = the_win->Width - the_win->BorderLeft - the_win->BorderRight;
	wh = the_win->Height -  the_win->BorderTop - the_win->BorderBottom;

	STEP(0, wx, wy ,0 ,0 ,1);
	STEP(1, wx+ww,wy,mode.x,0,1);
	STEP(2, wx+ww,wy+wh,mode.x,mode.y,1);

	STEP(3, wx,wy, 0,0,1);
	STEP(4, wx+ww,wy+wh,mode.x,mode.y,1);
	STEP(5, wx, wy+wh ,0 ,mode.y ,1);

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


void driver_window_comp::set_palette(uint8 *pal, int num)
{
	int n;

	// Convert palette to 32 bits virtual buffer.

	for (int i=0; i<num; i++) {
		n = i *3;
		vpal[i]=0xFF000000 + (pal[n] << 16) +  (pal[n+1] << 8) + pal[n+2]  ;
	}
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

		TheCloseWindow(the_win);
		the_win = NULL;
	}

	MutexRelease(video_mutex);
}
