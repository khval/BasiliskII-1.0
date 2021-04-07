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

extern struct Window **win;
extern Object **layout;
extern Object **refresh;
extern Object **obj;

char *last_asl_path = NULL;


void imagefile_asl( int win_id, int str_gad_id, BOOL opt )
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
					sprintf(name_whit_path,"");
				}

				RSetAttrO( win_id, str_gad_id, STRINGA_TextVal, name_whit_path);
				free(name_whit_path);
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
					sprintf(name_whit_path,"");
				}

				RSetAttrO( win_id, str_gad_id, STRINGA_TextVal, name_whit_path);
				free(name_whit_path);
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

