/*
 *  audio_amiga.cpp - Audio support, AmigaOS implementation using AHI
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

#include "sysdeps.h"

#include <dos/dostags.h>
///#include <devices/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/ahi.h>

#include "cpu_emulation.h"
#include "main.h"
#include "prefs.h"
#include "user_strings.h"
#include "audio.h"
#include "audio_defs.h"

#define DEBUG 0
#include "debug.h"

bool AHIDevice = -1;
extern struct AHIIFace	*IAHI;

#define AUDIO_FREQUENCY	48000

// The currently selected audio parameters (indices in audio_sample_rates[] etc. vectors)
static int audio_sample_rate_index = 0;
static int audio_sample_size_index = 0;
static int audio_channel_count_index = 0;

// Global variables
extern struct MsgPort	*StartupMsgPort;
extern struct Process	*Sound_Proc;
extern ULONG			SubTaskCount;

static long sound_buffer_size;					// Size of one audio buffer in bytes
static int audio_block_fetched = 0;				// Number of audio blocks fetched by interrupt routine
static int play_audio = 0;

int max_channels = 2;
int frequencies = AUDIO_FREQUENCY;
int ahi_id = 0;
AHIAudioCtrl* ahi_ctrl = NULL;
int sample_rate = 0;

ULONG	audio_irq_done;

bool open_audio(void);

/*
 *  Audio
*/

struct AudioBlock
{
	APTR buffer;
	LONG cleared;
};

static void atomic_add(int *ptr, int add)
{
	ptr[0]++;
}


/* static VOID SoundFunc(APTR buf1, APTR buf2,struct Message *msg) */

APTR sound_arg[2];

static VOID SoundFunc(void)
{
	APTR buf1 = sound_arg[0];
	APTR buf2 = sound_arg[1];
	struct AHIRequest req1, req2;
	struct MsgPort	*port;

	sound_arg[0]=0;
	sound_arg[1]=0;

	port = CreatePort("BasiliskII SoundFunc", 0);

	if (!port)
	{
		printf("ERROR ----- SoundFunc(void) - Failed to find process port ------ \n");
		return;
	}

	req1.ahir_Std.io_Message.mn_Node.ln_Name	= NULL;
	req1.ahir_Std.io_Message.mn_Node.ln_Pri		= 0;
	req1.ahir_Std.io_Message.mn_ReplyPort		= port;
	req1.ahir_Std.io_Message.mn_Length			= sizeof(struct AHIRequest);
	req1.ahir_Version	= 4;

	if (OpenDevice(AHINAME, AHI_DEFAULT_UNIT, (struct IORequest *)&req1, 0) == 0)
	{
		struct AudioBlock ab[2];

		ab[0].buffer	= buf1;
		ab[0].cleared	= 1;
		ab[1].buffer	= buf2;
		ab[1].cleared	= 1;

		req1.ahir_Std.io_Command	= CMD_WRITE;
		req1.ahir_Std.io_Offset		= 0;
		req1.ahir_Type				= AHIST_S16S;
		req1.ahir_Frequency		= AUDIO_FREQUENCY;
		req1.ahir_Position			= 0x8000;
		req1.ahir_Volume			= 0x10000;

		CopyMem(&req1, &req2, sizeof(struct AHIRequest));

		memset(buf1, 0, sound_buffer_size);
		memset(buf2, 0, sound_buffer_size);

		audio_irq_done	= 1 << port->mp_SigBit;

		req1.ahir_Std.io_Data	= buf1;
		req2.ahir_Std.io_Data	= buf2;
		req1.ahir_Std.io_Length	= 2048;
		req2.ahir_Std.io_Length	= 2048;
		req1.ahir_Link				= NULL;
		req2.ahir_Link				= &req1;

		SendIO((struct IORequest *)&req1);
		SendIO((struct IORequest *)&req2);

		for (;;)
		{
			struct AHIRequest *io;
			ULONG	sigs;

			sigs	= Wait(audio_irq_done | SIGBREAKF_CTRL_C);

			if (sigs & SIGBREAKF_CTRL_C)
			{
				break;
			}

			while ((io = (struct AHIRequest *)GetMsg(port)))
			{
				ULONG idx	= (io == &req1) ? 0 : 1;

				io->ahir_Std.io_Data = ab[idx].buffer;

				// New buffer available?
				if (play_audio && audio_block_fetched)
				{
					ab[idx].cleared = 0;

					audio_block_fetched--;

					// Get size of audio data
					uint32 apple_stream_info = ReadMacInt32(audio_data + adatStreamInfo);

					if (apple_stream_info)
					{
						int work_size = ReadMacInt32(apple_stream_info + scd_sampleCount) * (AudioStatus.sample_size >> 3) * AudioStatus.channels;
						D(bug("stream: work_size %d\n", work_size));
						if (work_size > sound_buffer_size)
							work_size = sound_buffer_size;

						// Put data into AHI buffer (convert 8-bit data unsigned->signed)
						Mac2Host_memcpy(io->ahir_Std.io_Data, ReadMacInt32(apple_stream_info + scd_buffer), work_size);

						if (work_size != sound_buffer_size)
						{
							memset((uint8 *)io->ahir_Std.io_Data + work_size, 0, sound_buffer_size - work_size);
						}
					}
				}
				else if (ab[idx].cleared == 0)
				{
					ab[idx].cleared = 1;
					memset(io->ahir_Std.io_Data, 0, sound_buffer_size);
				}

				io->ahir_Std.io_Length	= sound_buffer_size;
				io->ahir_Std.io_Offset	= 0;
				io->ahir_Type				= AHIST_S16S;
				io->ahir_Frequency		= AUDIO_FREQUENCY;
				io->ahir_Position			= 0x8000;
				io->ahir_Volume			= 0x10000;
				io->ahir_Link				= (io == &req1) ? &req2 : &req1;
				SendIO((struct IORequest *)io);

				// Trigger audio interrupt to get new buffer
				if (AudioStatus.num_sources)
				{
					D(bug("stream: triggering irq\n"));
					SetInterruptFlag(INTFLAG_AUDIO);
					TriggerInterrupt();
				}
			}
		}

		AbortIO((struct IORequest *)&req1);
		AbortIO((struct IORequest *)&req2);
		WaitIO((struct IORequest *)&req1);
		WaitIO((struct IORequest *)&req2);

		CloseDevice((struct IORequest *)&req1);
	}

	Sound_Proc	= NULL;
}

/*
 *  Initialization
 */


struct TagItem tags_SHARED[] = {
	{AVT_Type,MEMF_SHARED},
	{AVT_ClearWithValue, 0},
	{TAG_END,0}};


void AudioInit(void)
{
	// Init audio status and feature flags
	AudioStatus.sample_rate = 44100 << 16;
	AudioStatus.sample_size = 16;
	AudioStatus.channels = 2;
	AudioStatus.mixer = 0;
	AudioStatus.num_sources = 0;
	audio_component_flags = cmpWantsRegisterMessage | kStereoOut | k16BitOut;

	// Sound disabled in prefs? Then do nothing
	if (PrefsFindBool("nosound"))
	{
		printf("Prefs: No sound\n");
		return;
	}

	// Open and initialize audio device
	open_audio();

}


bool open_audio(void)
{
	struct Message *msg;
	APTR	buf1, buf2;

	audio_frames_per_block = 2048;
	sound_buffer_size = (AudioStatus.sample_size >> 3) * AudioStatus.channels * audio_frames_per_block;

	buf1	= AllocVecTagList(sound_buffer_size , tags_SHARED);
	buf2	= AllocVecTagList(sound_buffer_size , tags_SHARED);
	msg	= (struct Message *) AllocVecTagList( sizeof(*msg) , tags_SHARED );

	if (buf1 && buf2 && msg)
	{
		msg->mn_Node.ln_Type	= NT_MESSAGE;
		msg->mn_ReplyPort	= StartupMsgPort;
		msg->mn_Length		= sizeof(*msg);

		sound_arg[0] = buf1;
		sound_arg[1] = buf2;

		Sound_Proc = CreateNewProcTags(
			NP_Entry			, &SoundFunc,
			NP_Name		, "Basilisk II Sound Process",
			NP_Priority		, 1,
			NP_Input			, NULL,
			NP_Child			, TRUE,
			TAG_DONE);

		while (sound_arg[1]) { Delay(1); }

		if (Sound_Proc)
		{
			SubTaskCount++;
			audio_open	= true;
			printf("Audio open true?\n");

			return true;
		}
	}

	return false;
}

/*
 *  Deinitialization
 */

static void close_audio(void)
{
	// Close audio device

	if (Sound_Proc)
	{
		Signal( (Task *) Sound_Proc, SIGBREAKF_CTRL_C);
		while (Sound_Proc) Delay(1);
	}

	audio_open = false;
}


void AudioExit(void)
{
	// closed and opened in the process!!

	close_audio();

}


/*
 *  First source added, start audio stream
 */

void audio_enter_stream()
{
	play_audio	= 1;
}


/*
 *  Last source removed, stop audio stream
 */

void audio_exit_stream()
{
	play_audio	= 0;
//	audio_block_fetched	= 0;
}


/*
 *  MacOS audio interrupt, read next data block
 */

void AudioInterrupt(void)
{
	D(bug("AudioInterrupt\n"));

	// Get data from apple mixer
	if (AudioStatus.mixer) {
		M68kRegisters r;
		r.a[0] = audio_data + adatStreamInfo;
		r.a[1] = AudioStatus.mixer;
		Execute68k(audio_data + adatGetSourceData, &r);
		D(bug(" GetSourceData() returns %08lx\n", r.d[0]));
	} else
		WriteMacInt32(audio_data + adatStreamInfo, 0);

	// Signal stream function
	audio_block_fetched++;
	D(bug("AudioInterrupt done\n"));
}


/*
 *  Set sampling parameters
 *  "index" is an index into the audio_sample_rates[] etc. arrays
 *  It is guaranteed that AudioStatus.num_sources == 0
 */

bool audio_set_sample_rate(int index)
{
	close_audio();
	audio_sample_rate_index = index;
	return open_audio();
}

bool audio_set_sample_size(int index)
{
	close_audio();
	audio_sample_size_index = index;
	return open_audio();
}

bool audio_set_channels(int index)
{
	close_audio();
	audio_channel_count_index = index;
	return open_audio();
}


/*
 *  Get/set volume controls (volume values received/returned have the left channel
 *  volume in the upper 16 bits and the right channel volume in the lower 16 bits;
 *  both volumes are 8.8 fixed point values with 0x0100 meaning "maximum volume"))
 */

bool audio_get_main_mute(void)
{
	return false;
}

uint32 audio_get_main_volume(void)
{
	return 0x01000100;
}

bool audio_get_speaker_mute(void)
{
	return false;
}

uint32 audio_get_speaker_volume(void)
{
	return 0x01000100;
}

void audio_set_main_mute(bool mute)
{
}

void audio_set_main_volume(uint32 vol)
{
}

void audio_set_speaker_mute(bool mute)
{
}

void audio_set_speaker_volume(uint32 vol)
{
}
