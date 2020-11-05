// ------------------------------------------------------------------------
//    Copyright: Kjetil Hvalstrand (LiveForIt), license MIT.
//    this file is used many different projects,
//-------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <vector>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <proto/keymap.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/gadtools.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/diskfont.h>
#include <diskfont/diskfonttag.h>
#include <workbench/startup.h>

#include "common_screen.h"

struct MsgPort *iconifyPort = NULL;
struct DiskObject *_dobj = NULL;
struct AppIcon *appicon;
ULONG iconify_sig;


void save_window_attr(windowclass *self)
{
	GetWindowAttr( self -> win,  WA_Left, &self -> window_left, sizeof(int *));
	GetWindowAttr( self -> win,  WA_Top, &self -> window_top, sizeof(int *));
	GetWindowAttr( self -> win,  WA_InnerWidth, &self -> window_width, sizeof(int *));
	GetWindowAttr( self -> win,  WA_InnerHeight, &self -> window_height, sizeof(int *));

	self -> ModeID = GetVPModeID( &self -> win -> WScreen ->ViewPort);
}

extern  struct Screen *fullscreen_screen;

static bool had_fullscreen = false;

void enable_Iconify(struct Window *My_Window)
{
	int n;

	const char *files[]={
		"progdir:BasiliskII",
		"progdir:BasiliskII.db",
		NULL};

	for (n=0;files[n];n++)
	{
		_dobj = GetDiskObject( files[n] );
		if (_dobj) break;
	}

	if (_dobj)
	{
		iconifyPort = (struct MsgPort *) AllocSysObject(ASOT_PORT,NULL);

		if (iconifyPort)
		{
			iconify_sig = 1L<<iconifyPort -> mp_SigBit;

			appicon = AddAppIcon(1, 0, "Basilisk II", iconifyPort, 0, _dobj, 
					WBAPPICONA_SupportsOpen, TRUE,
					TAG_END);

			if (appicon) 
			{
				window_save_state.win = My_Window;
				save_window_attr(&window_save_state);
			}
		}
	}
}

void dispose_Iconify()
{
	if (_dobj)
	{
		RemoveAppIcon( appicon );
		FreeDiskObject(_dobj);
		appicon = NULL;
		_dobj = NULL;
	}

	if (iconifyPort)
	{
		FreeSysObject ( ASOT_PORT, iconifyPort ); 
		iconifyPort = NULL;
		iconify_sig  = 0;
	}
}


