
// 
// This file is copyrighted Kjetil Hvalstrand, Lincence MIT.
//
// this files is used in many diffrent projects, 
// and your allowed to modify it and use it in your own.
// 

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/diskfont.h>
#include <diskfont/diskfonttag.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/Amigainput.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/intuition.h>
#include <intuition/pointerclass.h>
#include <proto/retroMode.h>

struct Catalog *catalog = NULL;

extern struct TextFont *open_font( char const *filename, int size );

struct Process *main_task = NULL;

struct Library				*DataTypesBase = NULL;
struct DataTypesIFace		*IDataTypes = NULL;

extern struct Library		 	*DOSBase ;
extern struct DOSIFace		*IDOS ;

struct DebugIFace		*IDebug = NULL;

struct Library 			*AHIBase = NULL;
struct AHIIFace			*IAHI = NULL;

struct Library 			*AslBase = NULL;
struct AslIFace			*IAsl = NULL;

struct LocaleIFace		*ILocale  = NULL;
struct Library			*LocaleBase = NULL;

struct Library			*DiskfontBase = NULL;
struct DiskfontIFace		*IDiskfont = NULL;

struct Library			*GadToolsBase = NULL;
struct DiskfontIFace		*IGadTools = NULL;

struct KeymapIFace		*IKeymap = NULL;
struct Library			*KeymapBase = NULL;

struct Locale			*_locale = NULL;
ULONG				*codeset_page = NULL;

struct Library 			* RetroModeBase = NULL;
struct RetroModeIFace 	*IRetroMode = NULL;

struct WorkbenchIFace	*IWorkbench = NULL;
struct Library			*WorkbenchBase = NULL;

struct IconIFace		*IIcon = NULL;
struct Library			*IconBase = NULL;

struct Library			*IntuitionBase = NULL;
struct IntuitionIFace		*IIntuition = NULL;

struct Library			*GraphicsBase = NULL;
struct GraphicsIFace		*IGraphics = NULL;

struct Library			*LayersBase = NULL;
struct LayersIFace		*ILayers = NULL;

struct Library			*IFFParseBase = NULL;
struct IFFParseIFace *IIFFParse = NULL;


struct Library			*StringBase = NULL;
struct Library			*LayoutBase = NULL;
struct Library			*LabelBase = NULL;
struct Library			*ChooserBase = NULL;
struct Library			*IntegerBase = NULL;
struct Library			*ListBrowserBase = NULL;
struct Library			*ClickTabBase = NULL;
struct Library			*WindowBase = NULL;
struct Library			*CheckBoxBase = NULL;
struct Library			*RequesterBase = NULL;

struct StringIFace *IString = NULL;
struct LayoutIFace *ILayout = NULL;
struct LabelIFace *ILabel = NULL;
struct ChooserIFace *IChooser = NULL;
struct IntegerIFace *IInteger = NULL;
struct ListBrowserIFace *IListBrowser = NULL;
struct ClickTabIFace *IClickTab = NULL;
struct WindowIFace *IWindow = NULL;
struct CheckBoxIFace *ICheckBox = NULL;
struct RequesterIFace *IRequester = NULL;

APTR video_mutex = NULL;

bool remove_words(char *name,const char **list);

UWORD *EmptyPointer = NULL;

#ifdef __amigaos3__
UWORD *ImagePointer = NULL;
#endif

#ifdef __amigaos4__
uint32 *ImagePointer = NULL;
Object *objectPointer = NULL;
#endif

struct BitMap *pointerBitmap = NULL;

BOOL open_lib( const char *name, int ver , const char *iname, int iver, struct Library **base, struct Interface **interface)
{
	*interface = NULL;
	*base = OpenLibrary( name , ver);

	if (*base)
	{
		 *interface = GetInterface( *base,  iname , iver, TAG_END );
		if (!*interface) printf("Unable to getInterface %s for %s %ld!\n",iname,name,ver);
	}
	else
	{
	   	printf("Unable to open the %s %ld!\n",name,ver);
	}
	return (*interface) ? TRUE : FALSE;
}

BOOL openlibs()
{
	int i;

	main_task = (struct Process *) FindTask(NULL);


	IDebug = (DebugIFace*) GetInterface( SysBase,"debug",1,TAG_END);
	if ( IDebug == NULL ) return FALSE;

	if ( ! open_lib( "asl.library", 0L , "main", 1, &AslBase, (struct Interface **) &IAsl  ) ) return FALSE;
	if ( ! open_lib( "datatypes.library", 0L , "main", 1, &DataTypesBase, (struct Interface **) &IDataTypes  ) ) return FALSE;
	if ( ! open_lib( "locale.library", 53 , "main", 1, &LocaleBase, (struct Interface **) &ILocale  ) ) return FALSE;
	if ( ! open_lib( "keymap.library", 53, "main", 1, &KeymapBase, (struct Interface **) &IKeymap) ) return FALSE;
	if ( ! open_lib( "diskfont.library", 50L, "main", 1, &DiskfontBase, (struct Interface **) &IDiskfont  ) ) return FALSE;
	if ( ! open_lib( "gadtools.library", 53L , "main", 1, &GadToolsBase, (struct Interface **) &IGadTools  ) ) return FALSE;
	if ( ! open_lib( "intuition.library", 51L , "main", 1, &IntuitionBase, (struct Interface **) &IIntuition  ) ) return FALSE;
	if ( ! open_lib( "graphics.library", 54L , "main", 1, &GraphicsBase, (struct Interface **) &IGraphics  ) ) return FALSE;
	if ( ! open_lib( "layers.library", 54L , "main", 1, &LayersBase, (struct Interface **) &ILayers  ) ) return FALSE;
	if ( ! open_lib( "workbench.library", 53 , "main", 1, &WorkbenchBase, (struct Interface **) &IWorkbench ) ) return FALSE;
	if ( ! open_lib( "icon.library", 53, "main", 1, &IconBase, (struct Interface **) &IIcon) ) return FALSE;
	if ( ! open_lib( "iffparse.library", 53, "main", 1, &IFFParseBase, (struct Interface **) &IIFFParse) ) return FALSE;

	if ( ! open_lib( "string.gadget", 53, "main", 1, &StringBase, (struct Interface **) &IString) ) return FALSE;
	if ( ! open_lib( "layout.gadget", 53, "main", 1, &LayoutBase, (struct Interface **) &ILayout) ) return FALSE;
	if ( ! open_lib( "label.image", 53, "main", 1, &LabelBase, (struct Interface **) &ILabel) ) return FALSE;
	if ( ! open_lib( "chooser.gadget", 53, "main", 1, &ChooserBase, (struct Interface **) &IChooser) ) return FALSE;
	if ( ! open_lib( "integer.gadget", 53, "main", 1, &IntegerBase, (struct Interface **) &IInteger) ) return FALSE;
	if ( ! open_lib( "listbrowser.gadget", 53, "main", 1, &ListBrowserBase, (struct Interface **) &IListBrowser) ) return FALSE;
	if ( ! open_lib( "clicktab.gadget", 53, "main", 1, &ClickTabBase, (struct Interface **) &IClickTab) ) return FALSE;
	if ( ! open_lib( "window.class", 53, "main", 1, &WindowBase, (struct Interface **) &IWindow) ) return FALSE;
	if ( ! open_lib( "checkbox.gadget", 53, "main", 1, &CheckBoxBase, (struct Interface **) &ICheckBox) ) return FALSE;

	if ( ! open_lib( "requester.class", 53, "main", 1, &RequesterBase, (struct Interface **) &IRequester) ) return FALSE;


	_locale = (struct Locale *) OpenLocale(NULL);

	if (_locale)
	{
		codeset_page = (ULONG *) ObtainCharsetInfo(DFCS_NUMBER, (ULONG) _locale -> loc_CodeSet , DFCS_MAPTABLE);
		printf("codeset is %d\n",_locale -> loc_CodeSet);
	}

	catalog = OpenCatalog(NULL, "basilisk.catalog", OC_BuiltInLanguage, "english", TAG_DONE);

	if (catalog == NULL)
	{
		printf("failed to open catalog... maybe not installed\n");
	}
	else
	{
		int n =0;
		const char *str;
		for (n=0;n<4;n++)
		{
			str = GetCatalogStr(catalog, n, NULL);
			if (str) printf("%s\n",str);
		}
	}


	video_mutex = (APTR) AllocSysObjectTags(ASOT_MUTEX, TAG_DONE);
	if ( ! video_mutex) return FALSE;


	return TRUE;
}


void closedown()
{
	int i;

	if (ILocale)	// check if lib is open...
	{
		if (catalog)
		{
			CloseCatalog(catalog); 
			catalog = NULL;
		}
	
		if (_locale)
		{
			CloseLocale(_locale); 
			_locale = NULL;
		}
	}

#ifdef __amigaos4__
	if ( objectPointer )
	{
		DisposeObject( objectPointer );
		objectPointer = NULL;
	}
#endif

	if (IDebug) DropInterface((struct Interface*) IDebug); IDebug = 0;

	if (IIcon) DropInterface((struct Interface*) IIcon); IIcon = 0;
	if (IconBase) CloseLibrary(IconBase); IconBase = 0;

	if (IWorkbench) DropInterface((struct Interface*) IWorkbench); IWorkbench = 0;
	if (WorkbenchBase) CloseLibrary(WorkbenchBase); WorkbenchBase = 0;

	if (IAsl) DropInterface((struct Interface*) IAsl); IAsl = 0;
	if (AslBase) CloseLibrary(AslBase); AslBase = 0;

	if (IDataTypes) DropInterface((struct Interface*) IDataTypes); IDataTypes = 0;
	if (DataTypesBase) CloseLibrary(DataTypesBase); DataTypesBase = 0;

	if (ILocale) DropInterface((struct Interface*) ILocale); ILocale = 0;
	if (LocaleBase) CloseLibrary(LocaleBase); LocaleBase = 0;

	if (IKeymap) DropInterface((struct Interface*) IKeymap); IKeymap = 0;
	if (KeymapBase) CloseLibrary(KeymapBase); KeymapBase = 0;

	if (DiskfontBase) CloseLibrary(DiskfontBase); DiskfontBase = 0;
	if (IDiskfont) DropInterface((struct Interface*) IDiskfont); IDiskfont = 0;

	if (IntuitionBase) CloseLibrary(IntuitionBase); IntuitionBase = 0;
	if (IIntuition) DropInterface((struct Interface*) IIntuition); IIntuition = 0;

	if (LayersBase) CloseLibrary(LayersBase); LayersBase = 0;
	if (ILayers) DropInterface((struct Interface*) ILayers); ILayers = 0;

	if (GraphicsBase) CloseLibrary(GraphicsBase); GraphicsBase = 0;
	if (IGraphics) DropInterface((struct Interface*) IGraphics); IGraphics = 0;

	if (IFFParseBase) CloseLibrary(IFFParseBase); IFFParseBase = 0;
	if (IIFFParse) DropInterface((struct Interface*) IIFFParse); IIFFParse = 0;

// -- free gui classes

	if (StringBase) CloseLibrary(StringBase); StringBase = 0;
	if (IString) DropInterface((struct Interface*) IString); IString = 0;

	if (LayoutBase) CloseLibrary(LayoutBase); LayoutBase = 0;
	if (ILayout) DropInterface((struct Interface*) ILayout); ILayout = 0;

	if (LabelBase) CloseLibrary(LabelBase); LabelBase = 0;
	if (ILabel) DropInterface((struct Interface*) ILabel); ILabel = 0;

	if (ChooserBase) CloseLibrary(ChooserBase); ChooserBase = 0;
	if (IChooser) DropInterface((struct Interface*) IChooser); IChooser = 0;

	if (IntegerBase) CloseLibrary(IntegerBase); IntegerBase = 0;
	if (IInteger) DropInterface((struct Interface*) IInteger); IInteger = 0;

	if (ListBrowserBase) CloseLibrary(ListBrowserBase); ListBrowserBase = 0;
	if (IListBrowser) DropInterface((struct Interface*) IListBrowser); IListBrowser = 0;

	if (ClickTabBase) CloseLibrary(ClickTabBase); ClickTabBase = 0;
	if (IClickTab) DropInterface((struct Interface*) IClickTab); IClickTab = 0;

	if (WindowBase) CloseLibrary(WindowBase); WindowBase = 0;
	if (IWindow) DropInterface((struct Interface*) IWindow); IWindow = 0;

	if (ClickTabBase) CloseLibrary(ClickTabBase); ClickTabBase = 0;
	if (IClickTab) DropInterface((struct Interface*) IClickTab); IClickTab = 0;

	if (CheckBoxBase) CloseLibrary(CheckBoxBase); GraphicsBase = 0;
	if (ICheckBox) DropInterface((struct Interface*) IGraphics); ICheckBox = 0;

	if (RequesterBase) CloseLibrary(RequesterBase); RequesterBase = 0;
	if (IRequester) DropInterface((struct Interface*) IRequester); IRequester = 0;

	if (video_mutex) 
	{
		FreeSysObject(ASOT_MUTEX, video_mutex); 
		video_mutex = NULL;
	}

}

bool remove_words(char *name,const char **list)
{
	const char **i;
	char *src;
	char *dest = name;
	bool found;
	bool word_removed = false;
	int sym;

	for (src = name;*src;src++)
	{

		found = false;

		for (i=list;*i;i++)
		{
			if (strncasecmp( src, *i , strlen(*i) ) == 0 )
			{
				src += strlen(*i);
				src--;
				found = true;
				word_removed = true;
				continue;
			}
		}

		if (found) continue;

		sym = *src;

		if ((sym>='A')&&(sym<='Z')) sym=sym-'A'+'a';

		*dest = sym;
		dest++;
	}
	*dest = 0;

	if (strcmp(name,"") == 0)
	{
		sprintf(name,"cmd");
	}
	return word_removed;
}


