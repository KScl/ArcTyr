/**** ArcTyr - OpenTyrianArcade ****
 *
 * Copyright          (C) 2007-2020  The OpenTyrian Development Team
 * Portions copyright (C) 2019-2020  Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  loudness.c
/// \brief Tyrian's interface for the Loudness sound system, for music

#include "file.h"
#include "lds_play.h"
#include "loudness.h"
#include "nortsong.h"
#include "opentyr.h"
#include "params.h"

float music_volume = 0, sample_volume = 0;

static float real_music_volume = 0;
static unsigned int music_fade_tic = 0;

static bool music_force_no_loop = false;

bool music_stopped = true;
unsigned int song_playing = 0;

bool audio_disabled = false, music_disabled = false, samples_disabled = false;

/* SYN: These shouldn't be used outside this file. Hands off! */
FILE *music_file = NULL;
Uint32 *song_offset;
Uint16 song_count = 0;

SAMPLE_TYPE *channel_buffer[SFX_CHANNELS] = { NULL };
SAMPLE_TYPE *channel_pos[SFX_CHANNELS] = { NULL };
Uint32 channel_len[SFX_CHANNELS] = { 0 };
Uint8 channel_vol[SFX_CHANNELS] = { 0 };

int sound_init_state = false;
int freq = 11025 * OUTPUT_QUALITY;

static SDL_AudioCVT audio_cvt; // used for format conversion

void audio_cb( void *userdata, unsigned char *feedme, int howmuch );

void load_song( unsigned int song_num );

bool init_audio( void )
{
	if (audio_disabled)
		return false;
	
	SDL_AudioSpec ask, got;
	
	ask.freq = freq;
	ask.format = (BYTES_PER_SAMPLE == 2) ? AUDIO_S16SYS : AUDIO_S8;
	ask.channels = 1;
	if (getenv("NUMSAMPLES"))
		ask.samples = atoi(getenv("NUMSAMPLES"));
	else
		ask.samples = 2048;
	ask.callback = audio_cb;

#ifndef TARGET_WIN32
	// use alsa by default, because defaulting to pulse is complete garbage
	// don't override -- if user wants to force another audio driver let them do so
	setenv("SDL_AUDIODRIVER", "alsa", false);
	printf("\tusing audio driver \"%s\"\n", getenv("SDL_AUDIODRIVER"));
#endif

	printf("\trequested %d Hz, %d channels, %d samples\n", ask.freq, ask.channels, ask.samples);
	
	if (SDL_OpenAudio(&ask, &got) == -1)
	{
		fprintf(stderr, "error: failed to initialize SDL audio: %s\n", SDL_GetError());
		audio_disabled = true;
		return false;
	}
	
	printf("\tobtained  %d Hz, %d channels, %d samples\n", got.freq, got.channels, got.samples);
	
	SDL_BuildAudioCVT(&audio_cvt, ask.format, ask.channels, ask.freq, got.format, got.channels, got.freq);
	
	opl_init();
	
	SDL_PauseAudio(0); // unpause
	
	return true;
}

void audio_cb( void *user_data, unsigned char *sdl_buffer, int howmuch )
{
	(void)user_data;
	
	// prepare for conversion
	howmuch /= audio_cvt.len_mult;
	audio_cvt.buf = sdl_buffer;
	audio_cvt.len = howmuch; 
	
	static long ct = 0;
	
	SAMPLE_TYPE *feedme = (SAMPLE_TYPE *)sdl_buffer;

	if (!music_disabled && !music_stopped)
	{
		/* SYN: Simulate the fm synth chip */
		SAMPLE_TYPE *music_pos = feedme;
		long remaining = howmuch / BYTES_PER_SAMPLE;
		while (remaining > 0)
		{
			while (ct < 0)
			{
				ct += freq;
				lds_update(); /* SYN: Do I need to use the return value for anything here? */
			}
			/* SYN: Okay, about the calculations below. I still don't 100% get what's going on, but...
			- freq is samples/time as output by SDL.
			- REFRESH is how often the play proc would have been called in Tyrian. Standard speed is
			70Hz, which is the default value of 70.0f
			- ct represents the margin between play time (representing # of samples) and tick speed of
			the songs (70Hz by default). It keeps track of which one is ahead, because they don't
			synch perfectly. */
			
			/* set i to smaller of data requested by SDL and a value calculated from the refresh rate */
			long i = (long)((ct / REFRESH) + 4) & ~3;
			i = (i > remaining) ? remaining : i; /* i should now equal the number of samples we get */
			opl_update((SAMPLE_TYPE *)music_pos, i);
			music_pos += i;
			remaining -= i;
			ct -= (long)(REFRESH * i);
		}

		if (music_force_no_loop && songlooped)
		{
			music_volume = 0;
			music_stopped = true;
		}
		else if (music_fade_tic)
		{
			if (SDL_GetTicks() > music_fade_tic)
			{
				music_volume = 0;
				music_stopped = true;
			}
			else
				music_volume = real_music_volume * ((music_fade_tic - SDL_GetTicks()) / 1000.0f);
		}

		/* Reduce the music volume. */
		int qu = howmuch / BYTES_PER_SAMPLE;
		for (int smp = 0; smp < qu; smp++)
		{
			feedme[smp] *= music_volume;
		}
	}
	
	if (!samples_disabled)
	{
		/* SYN: Mix sound channels and shove into audio buffer */
		for (int ch = 0; ch < SFX_CHANNELS; ch++)
		{
			float volume = sample_volume * (channel_vol[ch] / (float)STANDARD_SFX_CHANNELS);
			
			/* SYN: Don't copy more data than is in the channel! */
			unsigned int qu = ((unsigned)howmuch > channel_len[ch] ? channel_len[ch] : (unsigned)howmuch) / BYTES_PER_SAMPLE;
			for (unsigned int smp = 0; smp < qu; smp++)
			{
#if (BYTES_PER_SAMPLE == 2)
				Sint32 clip = (Sint32)feedme[smp] + (Sint32)(channel_pos[ch][smp] * volume);
				feedme[smp] = (clip > 0x7fff) ? 0x7fff : (clip <= -0x8000) ? -0x8000 : (Sint16)clip;
#else  /* BYTES_PER_SAMPLE */
				Sint16 clip = (Sint16)feedme[smp] + (Sint16)(channel_pos[ch][smp] * volume);
				feedme[smp] = (clip > 0x7f) ? 0x7f : (clip <= -0x80) ? -0x80 : (Sint8)clip;
#endif  /* BYTES_PER_SAMPLE */
			}
			
			channel_pos[ch] += qu;
			channel_len[ch] -= qu * BYTES_PER_SAMPLE;
			
			/* SYN: If we've emptied a channel buffer, let's free the memory and clear the channel. */
			if (channel_len[ch] == 0)
			{
				free(channel_buffer[ch]);
				channel_buffer[ch] = channel_pos[ch] = NULL;
			}
		}
	}
	
	// do conversion
	SDL_ConvertAudio(&audio_cvt);
}

void deinit_audio( void )
{
	if (audio_disabled)
	{
		return;
	}

	SDL_PauseAudio(1); // pause

	SDL_CloseAudio();
	
	for (unsigned int i = 0; i < SFX_CHANNELS; i++)
	{
		free(channel_buffer[i]);
		channel_buffer[i] = channel_pos[i] = NULL;
		channel_len[i] = 0;
	}
	
	lds_free();
}


void load_music( void )
{
	if (music_file == NULL)
	{
		music_file = dir_fopen_die(data_dir(), "music.mus", "rb");
		
		efread(&song_count, sizeof(song_count), 1, music_file);
		
		song_offset = malloc((song_count + 1) * sizeof(*song_offset));
		
		efread(song_offset, 4, song_count, music_file);
		song_offset[song_count] = ftell_eof(music_file);
	}
}

void load_song( unsigned int song_num )
{
	if (audio_disabled)
		return;
	
	SDL_LockAudio();
	
	if (song_num < song_count)
	{
		unsigned int song_size = song_offset[song_num + 1] - song_offset[song_num];
		lds_load(music_file, song_offset[song_num], song_size);
	}
	else
	{
		fprintf(stderr, "warning: failed to load song %d\n", song_num + 1);
	}
	
	SDL_UnlockAudio();
}

void play_song_once( unsigned int song_num )
{
	// primarily for the title screen -- won't restart the song
	if (song_num == song_playing)
		return;

	play_song(song_num);
	music_force_no_loop = true;
}

void play_song( unsigned int song_num )
{
	if (song_num != song_playing)
	{
		load_song(song_num);
		song_playing = song_num;
	}

	music_volume = real_music_volume;
	music_stopped = false;
	music_fade_tic = 0;
	music_force_no_loop = false;
}

void restart_song( void )
{
	unsigned int temp = song_playing;
	song_playing = -1;
	play_song(temp);
}

void stop_song( void )
{
	music_stopped = true;
}

void fade_song( void )
{
	// This implementation is black-boxed

	// Fade lasts one second
	music_fade_tic = SDL_GetTicks() + 1000;
}

void set_volume( unsigned int music, unsigned int sample )
{
	music_volume = music * (1.5f / 255.0f);
	sample_volume = sample * (1.0f / 255.0f);
	
	real_music_volume = music_volume;
	music_fade_tic = 0;
}

void JE_multiSamplePlay(JE_byte *buffer, JE_word size, JE_byte chan, JE_byte vol)
{
	if (audio_disabled || samples_disabled)
		return;
	
	SDL_LockAudio();

	//printf("JE_multiSamplePlay(%p, %d, %d, %d)\n", buffer, size, chan, vol);

	if (channel_buffer[chan])
		free(channel_buffer[chan]);
	
	channel_len[chan] = size * BYTES_PER_SAMPLE * SAMPLE_SCALING;
	channel_buffer[chan] = malloc(channel_len[chan]);
	channel_pos[chan] = channel_buffer[chan];
	channel_vol[chan] = vol + 1;

	for (int i = 0; i < size; i++)
	{
		for (int ex = 0; ex < SAMPLE_SCALING; ex++)
		{
#if (BYTES_PER_SAMPLE == 2)
			channel_buffer[chan][(i * SAMPLE_SCALING) + ex] = (Sint8)buffer[i] << 8;
#else  /* BYTES_PER_SAMPLE */
			channel_buffer[chan][(i * SAMPLE_SCALING) + ex] = (Sint8)buffer[i];
#endif  /* BYTES_PER_SAMPLE */
		}
	}

	SDL_UnlockAudio();
}

