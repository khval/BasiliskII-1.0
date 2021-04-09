#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/locale.h>

#define ALL_REACTION_CLASSES
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/icon.h>

#include "reaction_macros.h"

#define CATCOMP_NUMBERS 1
#include "locale/locale.h"

#include "asl.h"


char *last_asl_path = NULL;

void imagefile_asl( void (*action) (const char *name), BOOL opt )
{
	struct FileRequester *freq;
	BOOL rc;

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
			char *name_whit_path = name_whit_path = (char *) malloc(strlen(freq->fr_Drawer)+strlen(freq->fr_File)+2);

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
					sprintf(name_whit_path,freq->fr_File);
				}

				action(name_whit_path);
				free(name_whit_path);
			}
		}

		if (path) { free(path); path = NULL; }
		if (freq) FreeAslRequest (freq);
	}
}

void DO_ASL( const char *(*current_str) (), void (*action) ( const char *value ), BOOL drawers_only )
{
	struct FileRequester *freq;
	BOOL rc;
	const char *str = NULL;
	char *path = NULL;
	int i;

	freq = (struct FileRequester *) AllocAslRequestTags (ASL_FileRequest,TAG_END);
	if (freq)
	{
		str = current_str();

		i =(int) (strrchr(str, '/') - str);
		if (i<=0) i =(int) (strrchr(str, ':') - str);

		if (i>0)
		{ path = strndup(str,i); }
		else
		{ path = strdup(""); }

		rc = AslRequestTags (freq, ASLFR_InitialDrawer,path,ASLFR_InitialFile,"",ASLFR_DrawersOnly, drawers_only ,TAG_END);

		if (rc)
		{
			char *name_whit_path = NULL;
			name_whit_path = (char *) malloc(strlen(freq->fr_Drawer)+strlen(freq->fr_File)+2);

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
					sprintf(name_whit_path,freq->fr_File);
				}

				action(name_whit_path);
				free(name_whit_path);
			}
		}

		if (path) { free(path); path = NULL; }
		if (freq) FreeAslRequest (freq);
	}
}

void DO_ASL_MODE_ID(  	int (*current_ModeID) (), 
					void (*action) (ASL_ModeID  *asl_modeid) )
{
	struct  ScreenModeRequester *mreq;
	BOOL rc = false;
	ASL_ModeID mode;
	char *str;


	int width,height;

	mreq = (struct ScreenModeRequester *) AllocAslRequestTags (ASL_ScreenModeRequest,TAG_END);
	if (mreq)
	{
		rc = AslRequestTags (mreq,ASLSM_InitialDisplayID,current_ModeID(),TAG_END);

		if (rc)
		{
			struct DimensionInfo dimInfo;
			GetDisplayInfoData( NULL, &dimInfo, sizeof(dimInfo) , DTAG_DIMS, (unsigned int) mreq -> sm_DisplayID );

			mode.modeID = mreq -> sm_DisplayID;
			mode.width = 1 + dimInfo.Nominal.MaxX - dimInfo.Nominal.MinX;
			mode.height = 1 + dimInfo.Nominal.MaxY - dimInfo.Nominal.MinY;
			action( &mode );
		}

		FreeAslRequest (mreq);
	}
}

