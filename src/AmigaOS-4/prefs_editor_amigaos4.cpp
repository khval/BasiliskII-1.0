#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "amiga_rdb.h"

void	init_create_volume(int win_nr);
void init_win_disks(int win_nr, LONG page);

int add_vol_opt = 0;

/*
 *  prefs_editor_AmigaOS4.cpp - Preferences editor, AmigaOS4 implementation
 *
 * Basilisk II Reaction GUI (C) 2007-2020 Kjetil Hvalstrand
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


#include <dos/filehandler.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <libraries/iffparse.h>

#include <proto/dos.h>
#include <proto/exec.h>

#define ALL_REACTION_CLASSES
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/icon.h>

struct List list_files;

#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#define CATCOMP_ARRAY

#include "locale/locale.c"
#include "../include/prefs.h"
#include "main.h"


#define ScreenTitle "BasiliskII II 1.0"


#include "cdrom.h"

static void update_volume(int type, char *str, ULONG add_new_volume, ULONG read_only);

struct MsgPort *appport;

const char	*window_depth_names[]=
{
	"Default",
	"Black & White",
	"256 colors (8bit)",
	NULL
};

const char	*window_render_method_names[]=
{
	"Internal convertion",
	"System Write Pixel Array",
	"Direct write (if possible)",
	NULL
};

const char	*device_names[]=
{
	"a1ide.device",
	"sii0680ide.device",
	"sii3112ide.device",
	"sii3114ide.device",
	"sii3512ide.device",
	"lsi53c8xx.device",
	"it8212ide.device",
	"diskimage.device",
	"usbdisk.device",
	"sb600sata.device",
	"peg2ide.device",
	NULL
};

int find_device_name(char *txt)
{
	for (int i=0;device_names[i]!=NULL;i++)
	{
		if (strcasecmp(txt,device_names[i])==0) return i;
	}
	return 0;
}

enum
{
	win_null,
	win_prefs,
	win_disks,
	win_runtime,
	win_create_volume,
	win_end
};

struct Window *	win[win_end];
Object *			layout[win_end];
Object *			refresh[win_end];
Object *			obj[ID_END];

char *_L(unsigned int num) 
{
	unsigned int n;
	char ret = 0;

	for (n=0;n< ( sizeof(CatCompArray) / sizeof( struct CatCompArrayType ) );n++)
	{
		if (CatCompArray[n].cca_ID == num) { ret = n; break; }
	}

	return (char *) CatCompArray[ret].cca_Str ;
}

// define SetListBrowserNodeAttrsA(node, tags) IListBrowser->SetListBrowserNodeAttrsA(node, tags) 

#define MakeSlider(n,a,b) obj[n] = (Object*) SliderObject, \
	GA_ID, n,				\
	GA_RelVerify, TRUE,		\
	SLIDER_Min, a,			\
	SLIDER_Max, b,		\
	SLIDER_Ticks, b+1,		\
	SLIDER_Orientation,SLIDER_HORIZONTAL, \
	SliderEnd

#define MakeRect(n)  obj[n] = (Object*) ButtonObject, \
	GA_ID, n,				\
	GA_Text, _L(n),		\
	GA_RelVerify, TRUE,		\
	ButtonEnd

#define MakeButton(n)  obj[n] = (Object*) ButtonObject, \
	GA_ID, n,				\
	GA_Text, _L(n),		\
	GA_RelVerify, TRUE,		\
	ButtonEnd

#define MakeImageButton(n,nn)  obj[n] = (Object*) ButtonObject, \
	GA_ID, n,				\
	BUTTON_AutoButton, nn , \
	GA_RelVerify, TRUE,		\
	ButtonEnd
//	CHILD_Label, _L(n)


#define MakeCheck(n,v)	obj[n] = (Object*) CheckBoxObject, \
	GA_ID, n,				\
	GA_Selected, v,		\
	CheckBoxEnd

#define MakeString(n) obj[n] =  (Object*) StringObject, \
	GA_ID, n,				\
	GA_Text, _L(n),		\
	GA_RelVerify, TRUE,		\
	StringEnd

#define MakeHexString(n) obj[n] =  (Object*) StringObject, \
	GA_ID, n,				\
	GA_Text, _L(n),		\
	GA_RelVerify, TRUE,		\
	STRINGA_HookType, SHK_HEXIDECIMAL, \
	StringEnd



// 	STRINGA_TextVal, _L(n)	

#define MakeLabel(n)  LabelObject, \
	GA_ID, n,				\
	LABEL_Text, _L(n),		\
	LabelEnd

#define MakePopFile(n,l,v)  obj[n] = (Object*) StringObject, \
	GA_ID, n, 				\
	GA_Text, _L(n),		\
	GA_RelVerify, TRUE,		\
	STRINGA_TextVal, v,	\
	StringEnd

#define MakeInteger(n,v)  obj[n] = (Object*) IntegerObject, \
	GA_ID, n,				\
	GA_Text, _L(n),		\
	GA_RelVerify, TRUE,		\
	INTEGER_Number, v,	\
	IntegerEnd

#define MakeInteger_min_max(n,nr,min,max)  obj[n] = (Object*) IntegerObject,	\
	GA_ID, n, 					\
	GA_TabCycle, TRUE,		\
	GA_RelVerify, TRUE, 		\
	INTEGER_Number, nr, 		\
	INTEGER_Minimum, min,		\
	INTEGER_Maximum, max,	\
	StringEnd

#define MakeCycle(n,v)  obj[n] = (Object*) ChooserObject, \
	GA_ID, n,				\
	GA_Text, _L(n),		\
	GA_RelVerify, TRUE,		\
	CHOOSER_LabelArray, v, \
	ChooserEnd 

#define MakeList(n,ci,list) StartMember, obj[n] =(Object*) ListBrowserObject, \
	GA_ID, n,							\
	GA_RelVerify, TRUE,					\
	LISTBROWSER_Labels, list,			\
	LISTBROWSER_ColumnInfo, ci,			\
	LISTBROWSER_ColumnTitles, TRUE,		\
	LISTBROWSER_ShowSelected, TRUE,	\
	EndMember,  

#define GFrame(txt) \
	LAYOUT_SpaceOuter, TRUE,		\
	LAYOUT_DeferLayout, TRUE,		\
	LAYOUT_BevelStyle, GroupFrame,	\
	LAYOUT_Label, txt

#define RSetAttrO( win_nr, index , opt, value ) \
	if ( win[win_nr])	 SetGadgetAttrs( (struct Gadget *) obj[index], \
	win[win_nr],  NULL,  opt, value, TAG_DONE); 

#define RSetAttr( win_nr, obj , opt, value ) \
	if ( win[win_nr])	 SetGadgetAttrs( (struct Gadget *) obj, \
	win[win_nr],  NULL,  opt, value, TAG_DONE); 

#define RDetach(win_nr, index) \
	if (win[win_nr]) SetGadgetAttrs( (struct Gadget *) obj[index], win[win_nr] , NULL, \
		LISTBROWSER_MultiSelect, FALSE, \
		LISTBROWSER_Labels, ~0, \
		TAG_DONE); 

#define RAttach(win_nr,index,list) \
	if (win[win_nr]) SetGadgetAttrs( (struct Gadget *) obj[index], win[win_nr], NULL, \
	LISTBROWSER_Labels, list, TAG_DONE); 


ULONG getv( Object *obj, ULONG arg ) 
{
	ULONG ret;

	GetAttr( arg, obj, &ret );
	return ret; 
}


/*
 *  ReturnIDs
 */

#define MAX_STRING_LENGTH 256

/*
 *  External variables
 */

extern Object *app;

STATIC CONST CONST_STRPTR ModeNames[] = { "Window","Window composition","Full screen",NULL};
STATIC CONST CONST_STRPTR CPUNames[] = {"68000","68010","68020","68030","68040", NULL};
STATIC CONST CONST_STRPTR RamNames[] ={"8Mb","16Mb","32Mb","64Mb","128Mb","256Mb","512Mb",NULL};

STATIC CONST CONST_STRPTR ModelNames[] =
{
	"Mac IIci (MacOS 7.x)",
	"Quadra 900 (MacOS 8.x)",
	NULL
};

STATIC CONST_STRPTR VolumeTypes[] =
{
	(CONST_STRPTR) _L(ID_PREFS_VOLUMETYPE_FILE_GAD),
	(CONST_STRPTR) _L(ID_PREFS_VOLUMETYPE_DEVICE_GAD),
	NULL
};

static void read_volumes_settings( void )
{
	int n;
	const char *str;

	n = getv( obj[ ID_PREFS_CD_DEVICE_GAD], CHOOSER_Selected );
	str = device_names[n];

	printf("cdrom device: (%d) %s \n",n,str);

	if (*str)
	{
		char buf[MAX_STRING_LENGTH + 128];
		sprintf(buf, "/dev/%s/%ld/%d/%d/%d/%d", str, getv(obj[ ID_PREFS_CD_UNIT_GAD ], INTEGER_Number), 0, 0, 0, 2048);
		PrefsReplaceString("cdrom", buf);
	}
	else
	{
		PrefsRemoveItem("cdrom");
	}

	PrefsReplaceInt32("bootdriver", getv(obj[ ID_PREFS_CD_BOOT_GAD ], GA_Selected ) ? CDROMRefNum : 0 ); 
	PrefsReplaceBool("nocdrom", getv(obj[ ID_PREFS_CD_DISABLE_DRIVER_GAD ], GA_Selected));

	str = (char *) getv( obj[ID_PREFS_AMIGAOS4_ROOT_GAD], STRINGA_TextVal);

	if (*str)
	{
		PrefsReplaceString("extfs", str);
	}
}

static void read_scsi_settings( void )
{
	for (int i=0; i<7; i++)
	{
		CONST_STRPTR scsi_dev;
		char prefs_name[16];

		sprintf(prefs_name, "scsi%d", i);
		scsi_dev = (CONST_STRPTR) getv(obj[i + ID_PREFS_ID_0_DEVICE_GAD], STRINGA_TextVal);

		if (*scsi_dev)
		{
			char str[MAX_STRING_LENGTH + 32];
			sprintf(str, "%s/%ld", scsi_dev, getv(obj[i + ID_PREFS_ID_0_UNIT_GAD], INTEGER_Number));
			PrefsReplaceString(prefs_name, str);
		}
		else
		{
			PrefsRemoveItem(prefs_name);
		}
	}
}

static void make_serial_prefs( const char *prefs, Object *str_name, Object *str_unit, Object *ch_ispar)
{
	CONST_STRPTR dev;

	dev = (CONST_STRPTR)getv(str_name, STRINGA_TextVal);

	if (*dev)
	{
		char str[MAX_STRING_LENGTH + 32];
		sprintf(str, "%s%s/%ld", getv(ch_ispar, GA_Selected) ? "*" : "", dev, getv(str_unit, INTEGER_Number));
		PrefsReplaceString(prefs, str);
	}
	else
	{
		PrefsRemoveItem(prefs);
	}
}

static void read_serial_settings( void )
{
	CONST_STRPTR ether_dev;

	make_serial_prefs( "seriala", 
		obj[ ID_PREFS_MODEM_DEVICE_GAD ], 
		obj[ ID_PREFS_MODEM_UNIT_GAD ],
		obj[ ID_PREFS_MODEM_PARALLEL_GAD ] );

	make_serial_prefs( "serialb", 
		obj[ ID_PREFS_PRINTER_DEVICE_GAD ], 
		obj[ ID_PREFS_PRINTER_UNIT_GAD ],
		obj[ ID_PREFS_PRINTER_PARALLEL_GAD ] );

	ether_dev = (CONST_STRPTR)getv( obj[ID_PREFS_ETHERNET_DEVICE_GAD], STRINGA_TextVal );

	if (*ether_dev)
	{
		char str[MAX_STRING_LENGTH + 32];

		sprintf(str, "%s/%ld", ether_dev, getv( obj[ID_PREFS_ETHERNET_UNIT_GAD], INTEGER_Number));
		PrefsReplaceString("ether", str);
	}
	else
	{
		PrefsRemoveItem("ether");
	}
}

static void read_emulation_settings( void )
{
	char *str;
	char buf[128];
	const char *opt[]={"win","wic","scr"};
	int mode;
	int value;

	mode = getv( obj[ID_PREFS_GFX_MODE_GAD], CHOOSER_Selected);

	switch (mode)
	{
		case 0:	// win
		case 1:	// pip
				sprintf(buf, "%s/%ld/%ld", 
					opt[mode], 
					getv( obj[ID_PREFS_GFX_WIDTH_GAD], INTEGER_Number), 
					getv( obj[ID_PREFS_GFX_HEIGHT_GAD], INTEGER_Number));
				break;
		case 2:	// scr src
		case 3:
		case 4:
				sprintf(buf, "scr/%d/%s", mode, (char *) getv( obj[ID_PREFS_GFX_MODE_ID_GAD], STRINGA_TextVal));
				break;
	}



	PrefsReplaceString("screen", buf);

	printf("use_bitmap_lock: %s \n",	getv( obj[ID_PREFS_GFX_LOCK_GAD], GA_Selected) ? "True" : "False" ); 
	printf("render_method: %d\n",	getv( obj[ID_PREFS_GFX_RENDER_METHOD_GAD], CHOOSER_Selected) ); 

	PrefsReplaceBool("use_bitmap_lock",		getv( obj[ID_PREFS_GFX_LOCK_GAD], GA_Selected)); 

	PrefsReplaceInt32("render_method",		getv( obj[ID_PREFS_GFX_RENDER_METHOD_GAD], CHOOSER_Selected)); 

	PrefsReplaceInt32("windowdepth",	getv( obj[ID_PREFS_GFX_WINDOW_DEPTH_GAD], CHOOSER_Selected) ) ;

	PrefsReplaceBool("nosound",		getv( obj[ID_PREFS_DISABLE_SOUND], GA_Selected));

	// 0 = 1mb,	1 = 2 mb,	2 = 4 mb,	3 = 8 mb,	4 = 16 mb
	value =getv( obj[ID_PREFS_SYSTEM_RAM_GAD], CHOOSER_Selected)  + 3 ;

	PrefsReplaceInt32("ramsize",		(1 << value) * 1024 *1024 );

	PrefsReplaceBool("fpu",			getv( obj[ID_PREFS_SYSTEM_FPU_GAD], GA_Selected));
	PrefsReplaceInt32("cpu",			getv( obj[ID_PREFS_SYSTEM_CPU_GAD], CHOOSER_Selected ) );

	PrefsReplaceInt32("active_window_cpu_pri",		getv( obj[ID_CPU_ACTIVE_GAD], INTEGER_Number));
	PrefsReplaceInt32("inactive_window_cpu_pri",		getv( obj[ID_CPU_INACTIVE_GAD], INTEGER_Number));

	PrefsReplaceInt32("modelid",		getv( obj[ID_PREFS_SYSTEM_MODEL_GAD], CHOOSER_Selected) == 0 ? 5 : 14);
	PrefsReplaceInt32("frameskip",	getv( obj[ID_PREFS_GFX_FRAMESKIP_GAD], INTEGER_Number));
	PrefsReplaceInt32("lineskip",		getv( obj[ID_PREFS_GFX_LINESKIP_GAD], INTEGER_Number));

	str = (char *) getv( obj[ID_PREFS_SYSTEM_ROM_GAD] , STRINGA_TextVal );

	if (*str)
	{
		PrefsReplaceString("rom", str);
	}
	else
	{
		PrefsRemoveItem("rom");
	}
}

void add_volume(struct List *list,char *str, LONG num)
{
	ULONG read_only = 0;
	struct Node *		node;
	long long int size;
	char str_size[30];
	int 	v_unit;
	const char *str_unit[]={"Byte","KByte","MByte","GByte"};

	size = 0;
	v_unit = 0;

	if (*str == '*') { str++; read_only = 1; }

	if (strncmp(str,"/dev/",5)==0)
	{
		size = get_rdb_partition_size(str);
	}
	else
	{
		size = get_file_size(str);
	}

	for (;size>(50*1024);size/=1024)
	{
		v_unit++;
	}

	sprintf(str_size,"%lld %s",size,str_unit[v_unit]);

	if ((node = AllocListBrowserNode(3,
		LBNA_Column, 0,
		LBNCA_CopyText, TRUE,
		LBNCA_Text, str,
		LBNCA_Editable, FALSE,

		LBNA_Column, 1,
		LBNCA_CopyText, TRUE,
		LBNCA_Text, str_size,
		LBNCA_Editable, FALSE,

		LBNA_Column, 2,
		LBNCA_CopyText, TRUE,
		LBNCA_Text, read_only ? "Read Only" : "",
		LBNCA_Editable, FALSE,

		TAG_DONE)))
	{
		AddTail(list, node);
	}
}

void edit_volume(struct Node *node,char *str)
{
	ULONG read_only = 0;
	long long int size;
	char str_size[30];
	int 	v_unit;
	const char *str_unit[]={"Byte","KByte","MByte","GByte"};

	size = 0;
	v_unit = 0;

	if (*str == '*') { str++; read_only = 1; }

	if (strncmp(str,"/dev/",5)==0)
	{
		size = get_rdb_partition_size(str);
	}
	else
	{
		size = get_file_size(str);
	}

	for (;size>(1024*1024);size/=1024)
	{
		v_unit++;
	}

	sprintf(str_size,"%lld %s",size,str_unit[v_unit]);

	SetListBrowserNodeAttrs (node, 

		LBNA_Column, 0,
		LBNCA_CopyText, TRUE,
		LBNCA_Text, str,
		LBNCA_Editable, FALSE,

		LBNA_Column, 1,
		LBNCA_CopyText, TRUE,
		LBNCA_Text, str_size,
		LBNCA_Editable, FALSE,

		LBNA_Column, 2,
		LBNCA_CopyText, TRUE,
		LBNCA_Text, read_only ? "Read Only" : "",
		LBNCA_Editable, FALSE,

		TAG_DONE);
}

static void set_volumes_settings()
{
	char *str;

	RDetach(win_prefs, ID_MAC_VOLUMES );

	for (int i=0; (str = (char *) PrefsFindString("disk", i)) != NULL; i++)
	{
		add_volume(&list_files,str,0);
	}

	RAttach(win_prefs, ID_MAC_VOLUMES, &list_files);

	str = (char *) PrefsFindString("cdrom");

	if (str)
	{
		ULONG	cdrom_unit;
		ULONG	cdrom_dummy;
		UBYTE	cdrom_name[MAX_STRING_LENGTH];

		cdrom_unit		= 0;
		cdrom_name[0]	= 0;

		sscanf(str, "/dev/%[^/]/%ld/%ld/%ld/%ld/%ld", cdrom_name, &cdrom_unit, &cdrom_dummy, &cdrom_dummy, &cdrom_dummy, &cdrom_dummy);

		RSetAttrO( win_prefs, ID_PREFS_CD_DEVICE_GAD, CHOOSER_Selected, find_device_name( (char *) cdrom_name));
		RSetAttrO( win_prefs, ID_PREFS_CD_UNIT_GAD, INTEGER_Number, cdrom_unit);
	}

	RSetAttrO( win_prefs, ID_PREFS_CD_BOOT_GAD, GA_Selected, PrefsFindInt32("bootdriver") == CDROMRefNum ? TRUE : FALSE );
	RSetAttrO( win_prefs, ID_PREFS_CD_DISABLE_DRIVER_GAD, GA_Selected, PrefsFindBool("nocdrom"));

	if ((str = (char *) PrefsFindString("extfs")))
	{
		RSetAttrO( win_prefs, ID_PREFS_AMIGAOS4_ROOT_GAD, STRINGA_TextVal, str);
	}

}

static void set_scsi_settings( void )
{
	for (int i=0; i<7; i++)
	{
		char prefs_name[16];
		sprintf(prefs_name, "scsi%d", i);
		const char *str = PrefsFindString(prefs_name);

		if (str)
		{
			UBYTE scsi_dev[MAX_STRING_LENGTH];
			ULONG scsi_unit;

			scsi_dev[0] = 0;
			scsi_unit = 0;

			sscanf(str, "%[^/]/%ld", scsi_dev, &scsi_unit);

			RSetAttrO( win_prefs,  i+ID_PREFS_ID_0_DEVICE_GAD, STRINGA_TextVal, scsi_dev);
			RSetAttrO( win_prefs,  i+ID_PREFS_ID_0_UNIT_GAD, INTEGER_Number, scsi_unit);
		}
	}
}

static void parse_ser_prefs( const char *prefs, Object *str_devname, Object *str_unit, Object *ch_ispar)
{
	LONG	unit, ispar;
	UBYTE	dev[MAX_STRING_LENGTH];

	dev[0] = 0;
	unit = 0;
	ispar = false;

	char *str = (char *) PrefsFindString(prefs);
	if (str)
	{
		if (str[0] == '*')
		{
			ispar = true;
			str++;
		}
		sscanf(str, "%[^/]/%ld", dev, &unit);

		RSetAttr( win_prefs, str_devname, STRINGA_TextVal, &dev);
		RSetAttr( win_prefs, str_unit, INTEGER_Number, unit);
		RSetAttr( win_prefs, ch_ispar, GA_Selected, ispar);
	}
}

static void set_serial_settings( void )
{
	CONST_STRPTR str;

	parse_ser_prefs( "seriala", 
		obj[ ID_PREFS_MODEM_DEVICE_GAD ], 
		obj[ ID_PREFS_MODEM_UNIT_GAD ],
		obj[ ID_PREFS_MODEM_PARALLEL_GAD ] );

	parse_ser_prefs( "serialb", 
		obj[ ID_PREFS_PRINTER_DEVICE_GAD ], 
		obj[ ID_PREFS_PRINTER_UNIT_GAD ],
		obj[ ID_PREFS_PRINTER_PARALLEL_GAD ] );

	str = PrefsFindString("ether");

	if (str)
	{
		UBYTE	ether_dev[MAX_STRING_LENGTH];
		ULONG	ether_unit;

		const char *FirstSlash = strchr(str, '/');
		const char *LastSlash = strrchr(str, '/');

		if (FirstSlash && FirstSlash && FirstSlash != LastSlash)
		{
			// Device name contains path, i.e. "Networks/xyzzy.device"
			const char *lp = str;
			UBYTE *dp = ether_dev;

			while (lp != LastSlash)
				*dp++ = *lp++;
			*dp = '\0';

			sscanf(LastSlash, "/%ld", &ether_unit);
		}
		else
		{
			sscanf(str, "%[^/]/%ld", ether_dev, &ether_unit);
		}

		RSetAttrO( win_prefs, ID_PREFS_ETHERNET_DEVICE_GAD, STRINGA_TextVal, &ether_dev );
		RSetAttrO( win_prefs, ID_PREFS_ETHERNET_UNIT_GAD, INTEGER_Number, ether_unit );
	}
}

static void set_emulation_settings( void )
{
	CONST_STRPTR str;
	ULONG ramsize_mb, value, width = 800, height = 600;
	LONG		id;
	char			opt[4];
	char			*mode = NULL; 

	// Window width and height

	mode = (char *) AllocVec(50,MEMF_SHARED);
	if (!mode) return;

	sprintf(mode,"50051102");
	sprintf(opt,"win");

	value = 0; // window

	str = PrefsFindString("screen");
	if (str)
	{
		sscanf(str, "%c%c%c",&opt[0],&opt[1],&opt[2]);

		if (strcasecmp( (char *) &opt,"win")==0)
		{ sscanf(str,"win/%ld/%ld", (long int*) &width,(long int*) &height); value = 0; }

		if (strcasecmp( (char *) &opt,"wic")==0)
		{ sscanf(str,"winc/%ld/%ld", (long int*) &width,(long int*) &height); value = 1; }

		if (strcasecmp( (char *) &opt,"scr")==0)
		{
			sscanf(str,"scr/%d/%s",&value, (char *) mode); 
		}
	}

	RSetAttrO( win_prefs,  ID_PREFS_GFX_MODE_GAD,
		CHOOSER_Selected, value );

	RSetAttrO( win_prefs,  ID_PREFS_GFX_WIDTH_GAD,
		INTEGER_Number, width);

	RSetAttrO( win_prefs,  ID_PREFS_GFX_HEIGHT_GAD,
		INTEGER_Number, height);

	RSetAttrO( win_prefs,  ID_PREFS_GFX_MODE_ID_GAD,
		STRINGA_TextVal, mode);

	if (mode)	{ FreeVec(mode); mode=NULL; }


	RSetAttrO( win_prefs, ID_PREFS_GFX_WINDOW_DEPTH_GAD, 
		CHOOSER_Selected, PrefsFindInt32("windowdepth")) ;

	RSetAttrO( win_prefs,  ID_PREFS_GFX_RENDER_METHOD_GAD,
		CHOOSER_Selected, PrefsFindInt32("render_method")) ;

	RSetAttrO( win_prefs, ID_PREFS_GFX_FRAMESKIP_GAD,
		INTEGER_Number, PrefsFindInt32("frameskip"));

	RSetAttrO( win_prefs, ID_PREFS_GFX_LINESKIP_GAD,
		INTEGER_Number, PrefsFindInt32("lineskip"));

	RSetAttrO( win_prefs,  ID_PREFS_GFX_LOCK_GAD,
		GA_Selected, PrefsFindBool("use_bitmap_lock"));

	// Sound

	RSetAttrO( win_prefs,  ID_PREFS_DISABLE_SOUND,
		GA_Selected, PrefsFindBool("nosound"));

	// FPU

	RSetAttrO( win_prefs,  ID_PREFS_SYSTEM_FPU_GAD,
		GA_Selected, PrefsFindBool("fpu"));

	// RAM

	ramsize_mb	= PrefsFindInt32("ramsize") / (1024 *1024);

	for (value=0;(ramsize_mb>>value)>1;value++);
	// 0 = 1mb,	1 = 2mb,		2 = 4mb,		3 = 8mb,		4 = 16mb,

	RSetAttrO( win_prefs,  ID_PREFS_SYSTEM_RAM_GAD,
		CHOOSER_Selected, value - 3);

	RSetAttrO( win_prefs,  ID_PREFS_SYSTEM_CPU_GAD,
		CHOOSER_Selected, PrefsFindInt32("cpu") );


	RSetAttrO( win_prefs, ID_CPU_ACTIVE_GAD,
		INTEGER_Number, PrefsFindInt32("active_window_cpu_pri"));

	RSetAttrO( win_prefs, ID_CPU_INACTIVE_GAD,
		INTEGER_Number, PrefsFindInt32("inactive_window_cpu_pri"));


	// Model

	id = PrefsFindInt32("modelid");
	RSetAttrO( win_prefs,  ID_PREFS_SYSTEM_MODEL_GAD, 
		CHOOSER_Selected, id == 5 ? 0 : 1);	// id is 5 or 14

	// ROM

	str = PrefsFindString("rom");
	RSetAttrO( win_prefs,  ID_PREFS_SYSTEM_ROM_GAD,
		STRINGA_TextVal, str);
}

void close_window(int layout_nr)
{
	if (layout[ layout_nr ])
	{
		DisposeObject( (Object *) layout[ layout_nr ] );
		layout[ layout_nr ]	= 0;
		win[ layout_nr ]	= 0;
	}
}

/*
static void check_hardfile(void)
{
	STRPTR filename;

	filename = (STRPTR) getv(str_vol_hardfile, STRINGA_TextVal);

	if (filename)
	{
		BPTR lock;

		lock	= Lock(filename, ACCESS_READ);

		RSetAttrO( win_prefs, grp_show_hf_size, CLICKTAB_Current, lock ? 0 : 1);
		RSetAttrO( win_prefs, bt_create, GA_Disabled, lock ? TRUE : FALSE);
		RSetAttrO( win_prefs, str_hf_size, INTEGER_Number, 1024);

		if (lock)
		{
			struct FileInfoBlock fib;

			Examine(lock, &fib);
			UnLock(lock);
			DoMethod(tx_hf_size, MUIM_SetAsString, STRINGA_TextVal, "%ld", fib.fib_Size / 1024 / 1024);
		}
	}
}
*/

/*
static void set_hardfile(void)
{
	struct FileRequester *req = (struct FileRequester *)REG_A1;
	STRPTR dir, file, buf;
	ULONG	size;

	dir	= req->fr_Drawer;
	file	= req->fr_File;
	size	= strlen(dir) + strlen(file) + 10;
	buf	= (STRPTR)AllocTaskPooled(size);

	if (buf)
	{
		strcpy(buf, dir);
		AddPart(buf, file, size);
		RSetAttrO( win_prefs, str_vol_hardfile, STRINGA_TextVal, buf);
		FreeTaskPooled(buf, size);
		check_hardfile();
	}
}
*/


 int create_hardfile(STRPTR filename , int size)
{
	int ret = 0;

	if (filename)
	{
		BPTR fh;

		fh	= Open(filename, MODE_OLDFILE);

		if (!fh)
		{
			fh = Open(filename, MODE_NEWFILE);

			if (!fh)
			{
				return 0;
			}

			if (size == 0)
			{
				size	= 128;
			}
			else if (size > 2047)
			{
				size	= 2047;
			}

			SetFileSize(fh, size * 1024 * 1024, OFFSET_BEGINNING);

			ret = 1;
		}

		Close(fh);
	}

	return ret;
}


#if 0
static VOID drive_display(void)
{
	CONST_STRPTR *array = (CONST_STRPTR *)REG_A2;

	array[0] = (STRPTR)REG_A1;

	if ((IPTR)array[-1] % 2)
		array[ -9 ] = (STRPTR)10;
}
#endif

void create_volume()
{
	ULONG	read_only =0 ;
	char str[MAX_STRING_LENGTH + 128];
	STRPTR	diskimage;
	int size;


	diskimage = (STRPTR) getv( obj[ID_CREATE_NAME_GAD], STRINGA_TextVal );
	size = getv( obj[ID_CREATE_SIZE_GAD], INTEGER_Number);

	if (create_hardfile( diskimage, size))
	{
		sprintf(str, "%s%s", read_only ? "*" : "", (STRPTR) diskimage );
		update_volume( 1, str, 1, read_only);
	}
}

static void add_or_update_volume(ULONG add_new_volume)
{
	ULONG	type, unit, read_only;
	const char *device;
	const char *partition;
	const char *diskimage;
	char str[MAX_STRING_LENGTH + 128];
	int i;
	char *mstr = NULL;
	type		= getv( obj[ID_PREFS_TYPE_GAD], CLICKTAB_Current);
	read_only	= 0;


	switch( type )
	{
		case 0:	// partition

				device  = device_names[ getv( obj[ID_PREFS_DEVICE_GAD], CHOOSER_Selected ) ];
				unit = getv( obj[ID_PREFS_UNIT_GAD], INTEGER_Number );
				partition = (STRPTR) getv( obj[ID_PREFS_PARTITION_NAME_GAD], STRINGA_TextVal );
				sprintf(str, "%s/dev/%s/%ld/%s", read_only ? "*" : "", device, unit, partition );
				break;

		case 1:	// diskimage

				read_only = getv(obj[ ID_PREFS_READ_ONLY_GAD ] , GA_Selected ) ? 1 : 0 ; 
				diskimage = (STRPTR) getv( obj[ID_PREFS_FILE_GAD], STRINGA_TextVal );
				sprintf(str, "%s%s", read_only ? "*" : "", (STRPTR) diskimage );
				break;
	}

	update_volume( type, str, add_new_volume, read_only);

}



static void update_volume(int type, char *str, ULONG add_new_volume, ULONG read_only)
{
	ULONG	unit;
//	char		keyword[50];
	int i;
	char *mstr = NULL;

	switch (add_new_volume)
	{
		case 1:
				// Add new item

				RDetach(win_prefs, ID_MAC_VOLUMES );

				PrefsAddString("disk", str);
				for (i=0; PrefsFindString("disk", i); i++);
				add_volume(&list_files,(char *) PrefsFindString("disk", i - 1),0);

				RAttach(win_prefs, ID_MAC_VOLUMES, &list_files);
				break;

		case 2:
				for (i=100; i>-1; i--)
				{
					if ((mstr = (char *) PrefsFindString("disk", i)))
					{
						if (PrefsFindString("disk", i+1))
						{
							PrefsReplaceString("disk", mstr, i+1);
						}
						else
						{
							PrefsAddString("disk", mstr);
						}
					}
				}

				if (PrefsFindString("disk", 0))
				{
					PrefsReplaceString("disk", str, 0);
				}
				else
				{
					PrefsAddString("disk", str);
				}
	
				RDetach(win_prefs, ID_MAC_VOLUMES );
				FreeListBrowserList(&list_files);

				for ( i=0; (mstr = (char *) PrefsFindString("disk", i)) != NULL; i++)
				{
					add_volume(&list_files,mstr,0);
				}

				RAttach(win_prefs, ID_MAC_VOLUMES, &list_files);

				// Add to top
				break;

		default:
				// Replace existing item
				struct Node *node = (struct Node *) getv(obj[ID_MAC_VOLUMES], LISTBROWSER_SelectedNode);
				ULONG pos =getv(obj[ID_MAC_VOLUMES], LISTBROWSER_Selected);

				PrefsReplaceString("disk", str, pos);

//				printf("Node 0x%x Node nr %lu\n",(unsigned int) node, pos);

				if (node) edit_volume(node,str);

//				RSetAttrO( win_prefs, obj[ID_MAC_VOLUMES], LISTBROWSER_Selected, pos);
	}
}


//--ADD-EDIT

void open_create_volume()
{
	init_create_volume(win_create_volume);

	if ( ( win[win_create_volume] = RA_OpenWindow( layout[win_create_volume] ) ) )
	{
		RSetAttrO( win_prefs, ID_PREFS_ADD_BOOTDISK_GAD , GA_Disabled, TRUE);
		RSetAttrO( win_prefs, ID_PREFS_ADD_GAD , GA_Disabled, TRUE);
		RSetAttrO( win_prefs, ID_PREFS_CREATE_GAD , GA_Disabled, TRUE);
		RSetAttrO( win_prefs, ID_PREFS_EDIT_GAD , GA_Disabled, TRUE);
		RSetAttrO( win_prefs, ID_PREFS_REMOVE_GAD , GA_Disabled, TRUE);
		RSetAttrO( win_prefs, ID_MAC_VOLUMES , GA_Disabled, TRUE);
	}

}

void add_edit_volume( int adding )
{
	UBYTE	dev_name[MAX_STRING_LENGTH];
	UBYTE	partition_name[MAX_STRING_LENGTH];
	LONG	read_only, is_device, dev_unit;
	char		*str = NULL;

	dev_name[0]	= 0;
	partition_name[0] = 0;
	read_only		= FALSE;
	is_device		= FALSE;
	dev_unit		= -1;

	if (!adding)
	{
		str = (char *) PrefsFindString("disk", getv(obj[ID_MAC_VOLUMES], LISTBROWSER_Selected));

		if (str == NULL) return;

		if (str[0] == '*')
		{
			read_only = TRUE;
			str++;
		}

		if (strstr(str, "/dev/") == str)
		{
			is_device = TRUE;
			sscanf(str, "/dev/%[^/]/%ld/%[^/]", dev_name, &dev_unit, partition_name);
		}
		else
		{
			is_device = FALSE;
		}
	}

	init_win_disks(win_disks,is_device);

	if ( ( win[win_disks] = RA_OpenWindow( layout[win_disks] ) ) )
	{
		RSetAttrO( win_prefs, ID_PREFS_ADD_BOOTDISK_GAD , GA_Disabled, TRUE);
		RSetAttrO( win_prefs, ID_PREFS_ADD_GAD , GA_Disabled, TRUE);
		RSetAttrO( win_prefs, ID_PREFS_CREATE_GAD , GA_Disabled, TRUE);
		RSetAttrO( win_prefs, ID_PREFS_EDIT_GAD , GA_Disabled, TRUE);
		RSetAttrO( win_prefs, ID_PREFS_REMOVE_GAD , GA_Disabled, TRUE);
		RSetAttrO( win_prefs, ID_MAC_VOLUMES , GA_Disabled, TRUE);
	}

	if (!adding)
	{
		if (is_device)
		{
			RSetAttrO( win_disks, ID_PREFS_DEVICE_GAD , CHOOSER_Selected, find_device_name( (char *) dev_name));
			RSetAttrO( win_disks, ID_PREFS_UNIT_GAD , INTEGER_Number , dev_unit);
			RSetAttrO( win_disks, ID_PREFS_PARTITION_NAME_GAD, STRINGA_TextVal, &partition_name);
		}
		else
		{
			RSetAttrO( win_disks, ID_PREFS_FILE_GAD, STRINGA_TextVal, str);
		}
	}

	add_vol_opt = adding;

	return;
}


static void remove_volume( void )
{
	struct Node *entry = (struct Node *) getv(obj[ID_MAC_VOLUMES], LISTBROWSER_SelectedNode);

	if (entry)
	{
		PrefsRemoveItem("disk", getv(obj[ID_MAC_VOLUMES], LISTBROWSER_Selected));

		RDetach(win_prefs, ID_MAC_VOLUMES );
		Remove (entry);
		FreeListBrowserNode(entry);
		RAttach(win_prefs, ID_MAC_VOLUMES, &list_files);
	
		RSetAttrO( win_prefs, ID_PREFS_EDIT_GAD , GA_Disabled, TRUE);
		RSetAttrO( win_prefs, ID_PREFS_REMOVE_GAD , GA_Disabled, TRUE);
	}
}



/*
 *  Show preferences editor
 *  Returns true when user clicked on "Start", false otherwise
 */

int get_r_event( int (*fn_event) (int id,int code) )
{
	int n,result,code;
	int done = FALSE;

	for (n=1;n<win_end;n++)
	{
		if ( layout[n] != 0 )
		{
			while ((result = RA_HandleInput( layout[n] ,&code)) != WMHI_LASTMSG)
			{

//				printf("Reaction event: result %x -- %x\n", result, code);

				switch(result & WMHI_CLASSMASK)
				{
					case WMHI_GADGETUP:
						done = fn_event(result & WMHI_GADGETMASK,code / 0x10000);
						break;

					case WMHI_ICONIFY:
						if ( RA_Iconify( layout[ win_prefs ] ) )
						win[ win_prefs ] = NULL;
						break;
								 
					case WMHI_UNICONIFY:
						win[ win_prefs ] = RA_OpenWindow( layout[ win_prefs] );
						break;

					case WMHI_CLOSEWINDOW:
						done = TRUE;
						break;
				}
			}
		}
	}
	Delay(1);

	return done;
}

char *last_asl_path = NULL;

void imagefile_asl( int win_id, int str_gad_id, BOOL opt )
{
	struct FileRequester *freq;
	BOOL rc;
	char *name_whit_path = NULL;
	char *str = NULL;
	char *path = NULL;
	int i;

	freq = (struct FileRequester *) AllocAslRequestTags (ASL_FileRequest,TAG_END);
	if (freq)
	{
		if ( ! last_asl_path) last_asl_path = strdup("");

		rc = AslRequestTags (freq, ASLFR_InitialDrawer,last_asl_path,ASLFR_InitialFile,"",ASLFR_DrawersOnly, opt ,TAG_END);

		if (rc)
		{
			name_whit_path = (char *) AllocVec(strlen(freq->fr_Drawer)+strlen(freq->fr_File)+2,MEMF_ANY);


			if (last_asl_path) free (last_asl_path);
			last_asl_path = strdup(freq->fr_Drawer);

			if (name_whit_path) 
			{
				if (strlen(freq->fr_Drawer)>0)
				{
					switch (freq->fr_Drawer[strlen(freq->fr_Drawer)-1])
					{
						case '/':
						case ':':
								sprintf(name_whit_path,"%s%s",freq->fr_Drawer,freq->fr_File);
								break;
						default:
								sprintf(name_whit_path,"%s/%s",freq->fr_Drawer,freq->fr_File);
					}
				}
				else
				{
					sprintf(name_whit_path,"");
				}

				RSetAttrO( win_id, str_gad_id, STRINGA_TextVal, name_whit_path);
				FreeVec(name_whit_path);
			}
		}
		else	Printf ("requester was cancelled\n");

		if (path) { free(path); path = NULL; }
		if (freq) FreeAslRequest (freq);
	}
}

void DO_ASL(int win_id, int str_gad_id, BOOL opt)
{
	struct FileRequester *freq;
	BOOL rc;
	char *name_whit_path = NULL;
	char *str = NULL;
	char *path = NULL;
	int i;

	freq = (struct FileRequester *) AllocAslRequestTags (ASL_FileRequest,TAG_END);
	if (freq)
	{
		str = (char *) getv( obj[str_gad_id], STRINGA_TextVal);

		i =(int) (strrchr(str, '/') - str);
		if (i<=0) i =(int) (strrchr(str, ':') - str);

		if (i>0)
		{ path = strndup(str,i); }
		else
		{ path = strdup(""); }

		rc = AslRequestTags (freq, ASLFR_InitialDrawer,path,ASLFR_InitialFile,"",ASLFR_DrawersOnly, opt ,TAG_END);

		if (rc)
		{
			name_whit_path = (char *) AllocVec(strlen(freq->fr_Drawer)+strlen(freq->fr_File)+2,MEMF_ANY);

			if (name_whit_path) 
			{
				if (strlen(freq->fr_Drawer)>0)
				{
					switch (freq->fr_Drawer[strlen(freq->fr_Drawer)-1])
					{
						case '/':
						case ':':
								sprintf(name_whit_path,"%s%s",freq->fr_Drawer,freq->fr_File);
								break;
						default:
								sprintf(name_whit_path,"%s/%s",freq->fr_Drawer,freq->fr_File);
					}
				}
				else
				{
					sprintf(name_whit_path,"");
				}

				RSetAttrO( win_id, str_gad_id, STRINGA_TextVal, name_whit_path);
				FreeVec(name_whit_path);
			}
		}
		else	Printf ("requester was cancelled\n");

		if (path) { free(path); path = NULL; }
		if (freq) FreeAslRequest (freq);
	}
}

void DO_ASL_MODE_ID(int win_id, int str_gad_id)
{
	struct  ScreenModeRequester *mreq;
	BOOL rc;
	int mode;
	char *str;
	char tmpbuffer[50];
	int width,height;

	mreq = (struct ScreenModeRequester *) AllocAslRequestTags (ASL_ScreenModeRequest,TAG_END);
	if (mreq)
	{
		str = (char *) getv( obj[str_gad_id], STRINGA_TextVal);

		sscanf(str,"%x",&mode);

		rc = AslRequestTags (mreq,ASLSM_InitialDisplayID,mode,TAG_END);

		if (rc)
		{
			struct DimensionInfo dimInfo;

			sprintf(tmpbuffer,"%X",(unsigned int) mreq -> sm_DisplayID);

			GetDisplayInfoData( NULL, &dimInfo, sizeof(dimInfo) , DTAG_DIMS, (unsigned int) mreq -> sm_DisplayID );

			width = 1 + dimInfo.Nominal.MaxX - dimInfo.Nominal.MinX;
			height = 1 + dimInfo.Nominal.MaxY - dimInfo.Nominal.MinY;

			RSetAttrO( win_id, ID_PREFS_GFX_MODE_ID_GAD,STRINGA_TextVal, tmpbuffer);
			RSetAttrO( win_id, ID_PREFS_GFX_WIDTH_GAD,INTEGER_Number, width);
			RSetAttrO( win_id, ID_PREFS_GFX_HEIGHT_GAD, INTEGER_Number, height);

		}
		else	Printf ("requester was cancelled\n");

		FreeAslRequest (mreq);
	}
}

static int runtime_event(int id,int code)
{
	int retval = 0;

//	printf("Event %d code %d\n",id,code);

	switch (id)
	{
		case ID_PREFS_START_GAD:
			retval = 1;
			break;

		case ID_PREFS_QUIT_GAD:
			retval = 2;
			break;
	}

	return retval;
}

int event(int id, int code)
{
	int retval = 0;

//	printf("Event %d code %d\n",id,code);

	switch (id)
	{
		case ID_PREFS_START_GAD:
			retval = 1;
			break;

		case ID_PREFS_QUIT_GAD:
			retval = 2;
			break;

		case ID_PREFS_ADD_BOOTDISK_GAD:
			add_edit_volume(2); 
			break;

		case ID_PREFS_ADD_GAD:
			add_edit_volume(1); 
			break;

		case ID_PREFS_CREATE_GAD:
			open_create_volume(); 
			break;

		case ID_PREFS_EDIT_GAD:
			add_edit_volume(FALSE); 
			break;

		case ID_PREFS_REMOVE_GAD:
			remove_volume(); 
			break;

		case ID_MAC_VOLUMES:
			RSetAttrO( win_prefs, ID_PREFS_EDIT_GAD , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_REMOVE_GAD , GA_Disabled, FALSE);
			break;

		case ID_PREFS_FILE_SELECT_GAD:
			imagefile_asl (win_disks, ID_PREFS_FILE_GAD, FALSE);
			break;

		case ID_PREFS_AMIGAOS4_ROOT_SELECT_GAD:
			DO_ASL (win_prefs, ID_PREFS_AMIGAOS4_ROOT_GAD, TRUE);
			break;

		case ID_PREFS_SYSTEM_ROM_SELECT_GAD:
			DO_ASL (win_prefs,ID_PREFS_SYSTEM_ROM_GAD, FALSE);
			break;

    		case ID_PREFS_GFX_MODE_ID_SELECT_GAD:
			DO_ASL_MODE_ID (win_prefs,ID_PREFS_GFX_MODE_ID_GAD);
			break;

    		case ID_PREFS_ETHERNET_DEVICE_SELECT_GAD:
			DO_ASL(win_prefs, ID_PREFS_ETHERNET_DEVICE_GAD, FALSE);
			break;

		case ID_CREATE_NAME_ASL_GAD:
			DO_ASL(win_create_volume, ID_CREATE_NAME_GAD, FALSE);
			break;

		case ID_CREATE_OK_GAD:

			create_volume( );
			close_window(win_create_volume);
			RefreshGList( (Gadget *) refresh[win_prefs], win[win_prefs], NULL, -1 );
 			
			RSetAttrO( win_prefs, ID_MAC_VOLUMES , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_ADD_BOOTDISK_GAD , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_ADD_GAD , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_CREATE_GAD , GA_Disabled, FALSE);
			break;

		case ID_CREATE_CANCEL_GAD:
 			close_window(win_create_volume);

			RSetAttrO( win_prefs, ID_MAC_VOLUMES , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_ADD_BOOTDISK_GAD , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_ADD_GAD , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_CREATE_GAD , GA_Disabled, FALSE);
			break;

		case ID_OK_GAD:

			add_or_update_volume( add_vol_opt );	// 1 = add, 0 = update
 			close_window(win_disks);
			RefreshGList( (Gadget *) refresh[win_prefs], win[win_prefs], NULL, -1 );

			RSetAttrO( win_prefs, ID_MAC_VOLUMES , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_ADD_BOOTDISK_GAD , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_ADD_GAD , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_CREATE_GAD , GA_Disabled, FALSE);
			break;

		case ID_CANCEL_GAD:
 			close_window(win_disks);

			RSetAttrO( win_prefs, ID_MAC_VOLUMES , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_ADD_BOOTDISK_GAD , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_ADD_GAD , GA_Disabled, FALSE);
			RSetAttrO( win_prefs, ID_PREFS_CREATE_GAD , GA_Disabled, FALSE);
			break;
	}

	return retval;
}

const char *tablabels[]={"Disk","SCSI","IO","CPU","Graphics",NULL};
const char *disk_tablabels[]={"Partition","Diskimage",NULL};

struct ColumnInfo volumes_ci[] =
{
	{ 60, "name", 0 },
	{ 20, "size", 0 },
	{ 20, "RW", 0 },
	{ ~0, (STRPTR) ~0, ~0 }
};

void init_create_volume(int win_nr)
{
	layout[win_nr] = (Object*) WindowObject,
			WA_ScreenTitle, ScreenTitle,
			WA_Title, ScreenTitle,
			WA_SizeGadget, TRUE,
			WA_Width, 300,
			WA_Left, 40,
			WA_Top, 30,
			WA_DepthGadget, TRUE,
			WA_DragBar, TRUE,
			WA_CloseGadget, TRUE,
			WA_Activate, TRUE,
			WA_SmartRefresh, TRUE,
			WINDOW_ParentGroup, VLayoutObject,
				LAYOUT_SpaceOuter, TRUE,
				LAYOUT_DeferLayout, TRUE,

				LAYOUT_AddChild, HGroupObject,

					LAYOUT_AddChild, MakeString(ID_CREATE_NAME_GAD), 
					CHILD_Label, MakeLabel(ID_CREATE_NAME_GAD),

					LAYOUT_AddChild, MakeImageButton(ID_CREATE_NAME_ASL_GAD,BAG_POPDRAWER),
					CHILD_WeightedWidth, 0,

					LAYOUT_AddChild, MakeInteger(ID_CREATE_SIZE_GAD, 8),
					CHILD_Label, MakeLabel(ID_CREATE_SIZE_GAD),

				LayoutEnd,
				CHILD_WeightedHeight, 0,

				LAYOUT_AddChild, HGroupObject,
					LAYOUT_AddChild, MakeButton(ID_CREATE_OK_GAD),
					LAYOUT_AddChild, MakeButton(ID_CREATE_CANCEL_GAD),
				LayoutEnd,
				CHILD_WeightedHeight, 0,

			EndMember,
		EndWindow;
}


void init_win_disks(int win_nr, LONG is_device)
{
	layout[win_nr] = (Object*) WindowObject,
			WA_ScreenTitle, ScreenTitle,
			WA_Title, ScreenTitle,
			WA_SizeGadget, TRUE,
			WA_Width, 300,
			WA_Left, 40,
			WA_Top, 30,
			WA_DepthGadget, TRUE,
			WA_DragBar, TRUE,
			WA_CloseGadget, TRUE,
			WA_Activate, TRUE,
			WA_SmartRefresh, TRUE,
			WINDOW_ParentGroup, VLayoutObject,
				LAYOUT_SpaceOuter, TRUE,
				LAYOUT_DeferLayout, TRUE,

				LAYOUT_AddChild,  obj[ID_PREFS_TYPE_GAD] = (Object*) ClickTabObject,
					GA_RelVerify, TRUE,
					GA_Text, disk_tablabels,
					GA_ID, ID_PREFS_TYPE_GAD ,

					CLICKTAB_PageGroup,  PageObject,

						LAYOUT_DeferLayout, TRUE,
//----------------------------------------------------------------------------
#include "gui_pages/page_add_partition.i"
#include "gui_pages/page_add_diskimage.i"
//----------------------------------------------------------------------------
					PageEnd,

					CLICKTAB_Current, (is_device == TRUE ? 0:1) ,
				ClickTabEnd,

				LAYOUT_AddChild, HGroupObject,
					LAYOUT_AddChild, MakeButton(ID_OK_GAD),
					LAYOUT_AddChild, MakeButton(ID_CANCEL_GAD),
				LayoutEnd,
				CHILD_WeightedHeight, 0,

			EndMember,
		EndWindow;
}

void init_win_runtime(int win_nr)
{
	layout[win_nr] = (Object *) WindowObject,
		
				WA_IDCMP, IDCMP_RAWKEY | IDCMP_GADGETUP  | IDCMP_GADGETDOWN,
				WA_Top, 0,
				WA_Left, 0,
				WA_Width, 150,
				WA_SizeGadget, FALSE,
				WA_DepthGadget, TRUE,
				WA_DragBar, TRUE,
				WA_CloseGadget, FALSE,
				WA_Activate, TRUE,
								
				WA_Title, "BasiliskII-0.9",
				WA_ScreenTitle, ScreenTitle,

				WINDOW_ParentGroup,(Object *)  VGroupObject,
					LAYOUT_SpaceOuter, TRUE,
					LAYOUT_BevelStyle, BVS_THIN,

					LAYOUT_AddChild, HGroupObject,
						LAYOUT_AddChild, MakeButton(ID_PREFS_QUIT_GAD),
					LayoutEnd,
					CHILD_WeightedHeight, 0,

				EndGroup,
			EndWindow;
}

void init_win_prefs(int win_nr)
{
	layout[win_nr] = (Object *) WindowObject,
				
				/* these tags describe the window 
				 */
		
				WA_IDCMP, IDCMP_RAWKEY | IDCMP_GADGETUP  | IDCMP_GADGETDOWN,
				WA_Top, 20,
				WA_Left, 20,
				WA_Width, 500,
				WA_Height, 400,
				WA_SizeGadget, TRUE,
				WA_DepthGadget, TRUE,
				WA_DragBar, TRUE,
				WA_CloseGadget, FALSE,
				WA_Activate, TRUE,
								
				WA_Title, "BasiliskII 1.0" ,
				WA_ScreenTitle, "BasiliskII 1.0",
				
				/* Turn on gadget help in the window  */
				
				// WINDOW_GadgetHelp, TRUE,
				
				/* Add an iconification gadget. If you have this, you must listen to
				 * WMHI_ICONIFY.
				 */
				 
				WINDOW_IconifyGadget, FALSE,
				
				/* Below is the layout of the window  */
				
				WINDOW_ParentGroup,(Object *)  VGroupObject,
					LAYOUT_SpaceOuter, TRUE,
					LAYOUT_BevelStyle, BVS_THIN,
						
					StartVGroup, BAligned,
	
				LAYOUT_AddChild,  ClickTabObject,

					GA_RelVerify, TRUE,
					GA_Text, tablabels,

					CLICKTAB_PageGroup,  PageObject,
					LAYOUT_DeferLayout, TRUE,

#include "gui_pages/page_disk.i"
#include "gui_pages/page_scsi.i"
#include "gui_pages/page_io.i"
#include "gui_pages/page_cpu.i"
#include "gui_pages/page_gfx.i"

					PageEnd,
				ClickTabEnd,

				LAYOUT_AddChild, HGroupObject,
					LAYOUT_AddChild, MakeButton(ID_PREFS_START_GAD),
					LAYOUT_AddChild, MakeButton(ID_PREFS_QUIT_GAD),
				LayoutEnd,
				CHILD_WeightedHeight, 0,

					EndGroup,					
				EndGroup,
			EndWindow;
}


bool RunPrefs(void)
{
	int		n = 0;
	bool		retval;
	retval	= false;

	appport = (MsgPort *) AllocSysObjectTags(ASOT_PORT, TAG_DONE);
	if (!appport) return FALSE;

	NewList( &list_files );


	for (n=1;n<win_end;n++)
	{
		win[n]	= NULL;
		layout[n]	= NULL;
	}

	init_win_prefs(win_prefs);

	if ( ( win[win_prefs] = RA_OpenWindow( layout[win_prefs] ) ) )
	{
		RSetAttrO( win_prefs, ID_PREFS_EDIT_GAD , GA_Disabled, TRUE);
		RSetAttrO( win_prefs, ID_PREFS_REMOVE_GAD , GA_Disabled, TRUE);

		set_volumes_settings();
		set_scsi_settings();
		set_serial_settings(); 
		set_emulation_settings();

		RefreshGList( (Gadget *) refresh[win_prefs], win[win_prefs], NULL, -1 );

		for (;;)
		{
			switch( get_r_event( event ) )
			{
				case 1:	// printf("Start\n");
						retval = true;
						goto done;

				case 2:	// printf("Quit\n");
						retval = false;
						goto done;
			}
		}

done:
		read_volumes_settings();
		read_scsi_settings();
		read_serial_settings(); 
		read_emulation_settings();
		SavePrefs();

	}


	for (n=0;n<win_end;n++) if (win[n]) close_window(n);
	if (appport) FreeSysObject(ASOT_PORT,appport);

	if (last_asl_path) free(last_asl_path);
	last_asl_path = NULL;

	return retval;
}

bool PrefsEditor(void)
{
	return RunPrefs();
}

int runtime_gui_tread(void)
{
	int		n = 0;
	bool		retval;
	retval	= false;

	appport = (MsgPort *) AllocSysObjectTags(ASOT_PORT, TAG_DONE);
	if (!appport) return FALSE;

	NewList( &list_files );

	for (n=1;n<win_end;n++)
	{
		win[n]	= NULL;
		layout[n]	= NULL;
	}

	init_win_runtime(win_runtime);

	if ( ( win[win_runtime] = RA_OpenWindow( layout[win_runtime] ) ) )
	{
		for (;;)
		{
			/* Check & clear CTRL_C signal */
			if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
			{
				retval = false;
				goto quit;
			}

			switch( get_r_event(  runtime_event ) )
			{
				case 2:	retval = false;
						goto quit;
			}
		}
	}

quit:
	for (n=0;n<win_end;n++) if (win[n]) close_window(n);
	if (appport) FreeSysObject(ASOT_PORT,appport);


	return retval;
}

