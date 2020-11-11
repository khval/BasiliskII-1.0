/*
 *  prefs_amiga.cpp - Preferences handling, AmigaOS specifix stuff
 *
 *  Basilisk II (C) 1997-2001 Christian Bauer
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

#include <stdio.h>

#include "sysdeps.h"
#include "prefs.h"


// Platform-specific preferences items
prefs_desc platform_prefs_items[] = {
	{"sound", TYPE_STRING, false, "sound output mode description"},
	{"scsimemtype", TYPE_INT32, false, "SCSI buffer memory type"},
	{"lineskip", TYPE_INT32, false, "Skip scan lines"},
	{"windowdepth", TYPE_INT32, false, "Window depth" },
	{"use_bitmap_lock", TYPE_BOOLEAN, false, "Use bitmap lock" },
	{"render_method", TYPE_INT32, false, "how to render the gfx" },

	{"active_window_cpu_pri", TYPE_INT32, false, "Active window" },
	{"inactive_window_cpu_pri", TYPE_INT32, false, "Inactive window" },
	{"iconify_cpu_suspend", TYPE_BOOLEAN, false, "Supend cpu on iconify" },

	{NULL, TYPE_END, false, NULL} // End of list
};


// Prefs file name
extern char *PREFS_FILE_NAME;

/*
 *  Load preferences from settings file
 */

void LoadPrefs(const char *vmdir)
{
	// Read preferences from settings file
	FILE *f = fopen(PREFS_FILE_NAME, "r");
	if (f != NULL) {

		// Prefs file found, load settings
		LoadPrefsFromStream(f);
		fclose(f);

	} else {

		// No prefs file, save defaults
		SavePrefs();
	}
}


/*
 *  Save preferences to settings file
 */

void SavePrefs(void)
{
	FILE *f;
	if ((f = fopen(PREFS_FILE_NAME, "w")) != NULL) {
		SavePrefsToStream(f);
		fclose(f);
	}
}


/*
 *  Add defaults of platform-specific prefs items
 *  You may also override the defaults set in PrefsInit()
 */

void AddPlatformPrefsDefaults(void)
{
	PrefsReplaceString("extfs", "RAM:");
	PrefsAddInt32("scsimemtype", 0);
}
