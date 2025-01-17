/*
	ASIO version of BASS simple DSP test
	Copyright (c) 2000-2019 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "bassasio.h"
#include "bass.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

HWND win;

DWORD chan;	// the channel... HMUSIC or HSTREAM

OPENFILENAME ofn;

// display error messages
void Error(const char *es)
{
	char mes[200];
	sprintf(mes, "%s\n(error code: %d/%d)", es, BASS_ErrorGetCode(), BASS_ASIO_ErrorGetCode());
	MessageBox(win, mes, "Error", 0);
}

// "rotate"
HDSP rotdsp;	// DSP handle
float rotpos;	// cur.pos
void CALLBACK Rotate(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
	float *d = (float*)buffer;
	DWORD a;

	for (a = 0; a < length / 4; a += 2) {
		d[a] *= fabs(sin(rotpos));
		d[a + 1] *= fabs(cos(rotpos));
		rotpos += 0.00003;
	}
	rotpos = fmod(rotpos, 2 * M_PI);
}

// "echo"
HDSP echdsp;	// DSP handle
#define ECHBUFLEN 1200	// buffer length
float echbuf[ECHBUFLEN][2];	// buffer
int echpos;	// cur.pos
void CALLBACK Echo(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
	float *d = (float*)buffer;
	DWORD a;

	for (a = 0; a < length / 4; a += 2) {
		float l = d[a] + (echbuf[echpos][1] / 2);
		float r = d[a + 1] + (echbuf[echpos][0] / 2);
#if 1 // 0=echo, 1=basic "bathroom" reverb
		echbuf[echpos][0] = d[a] = l;
		echbuf[echpos][1] = d[a + 1] = r;
#else
		echbuf[echpos][0] = d[a];
		echbuf[echpos][1] = d[a + 1];
		d[a] = l;
		d[a + 1] = r;
#endif
		echpos++;
		if (echpos == ECHBUFLEN) echpos = 0;
	}
}

// "flanger"
HDSP fladsp;	// DSP handle
#define FLABUFLEN 350	// buffer length
float flabuf[FLABUFLEN][2];	// buffer
int flapos;	// cur.pos
float flas, flasinc;	// sweep pos/increment
void CALLBACK Flange(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
	float *d = (float*)buffer;
	DWORD a;

	for (a = 0; a < length / 4; a += 2) {
		int p1 = (flapos + (int)flas) % FLABUFLEN;
		int p2 = (p1 + 1) % FLABUFLEN;
		float f = flas - (int)flas;
		float s;

		s = (d[a] + ((flabuf[p1][0] * (1 - f)) + (flabuf[p2][0] * f))) * 0.7;
		flabuf[flapos][0] = d[a];
		d[a] = s;

		s = (d[a + 1] + ((flabuf[p1][1] * (1 - f)) + (flabuf[p2][1] * f))) * 0.7;
		flabuf[flapos][1] = d[a + 1];
		d[a + 1] = s;

		flapos++;
		if (flapos == FLABUFLEN) flapos = 0;
		flas += flasinc;
		if (flas < 0 || flas > FLABUFLEN - 1) {
			flasinc = -flasinc;
			flas += flasinc;
		}
	}
}

#define MESS(id,m,w,l) SendDlgItemMessage(win,id,m,(WPARAM)(w),(LPARAM)(l))

INT_PTR CALLBACK dialogproc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch (m) {
		case WM_COMMAND:
			switch (LOWORD(w)) {
				case IDCANCEL:
					DestroyWindow(h);
					break;
				case 10:
					{
						BASS_CHANNELINFO info;
						char file[MAX_PATH] = "";
						ofn.lpstrFilter = "playable files\0*.mo3;*.xm;*.mod;*.s3m;*.it;*.mtm;*.umx;*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif\0All files\0*.*\0\0";
						ofn.lpstrFile = file;
						if (GetOpenFileName(&ofn)) {
							// stop the ASIO device, and free both MOD and stream (it must be one of them)
							BASS_ASIO_Stop();
							BASS_MusicFree(chan);
							BASS_StreamFree(chan);
							if (!(chan = BASS_StreamCreateFile(FALSE, file, 0, 0, BASS_SAMPLE_LOOP | BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE))
								&& !(chan = BASS_MusicLoad(FALSE, file, 0, 0, BASS_MUSIC_LOOP | BASS_MUSIC_RAMPS | BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE, 0))) {
								// whatever it is, it ain't playable
								MESS(10, WM_SETTEXT, 0, "click here to open a file...");
								Error("Can't play the file");
								break;
							}
							BASS_ChannelGetInfo(chan, &info);
							if (info.chans != 2) { // only stereo is allowed
								MESS(10, WM_SETTEXT, 0, "click here to open a file...");
								BASS_MusicFree(chan);
								BASS_StreamFree(chan);
								Error("Only stereo files are supported");
								break;
							}
							MESS(10, WM_SETTEXT, 0, file);
							// setup DSPs on new channel
							SendMessage(win, WM_COMMAND, 11, 0);
							SendMessage(win, WM_COMMAND, 12, 0);
							SendMessage(win, WM_COMMAND, 13, 0);
							// enable ASIO channels
							if (!BASS_ASIO_ChannelEnableBASS(FALSE, 0, chan, TRUE)) {
								MESS(10, WM_SETTEXT, 0, "click here to open a file...");
								BASS_MusicFree(chan);
								BASS_StreamFree(chan);
								Error("Can't enable ASIO channels");
								break;
							}
							BASS_ASIO_SetRate(info.freq); // try to set the device rate to avoid resampling
							if (!BASS_ASIO_Start(0, 0)) // start the device using default buffer/latency
								Error("Can't start ASIO device");
						}
					}
					break;
				case 11: // toggle "rotate"
					if (MESS(11, BM_GETCHECK, 0, 0)) {
						rotpos = M_PI / 4;
						rotdsp = BASS_ChannelSetDSP(chan, &Rotate, 0, 2);
					} else
						BASS_ChannelRemoveDSP(chan, rotdsp);
					break;
				case 12: // toggle "echo"
					if (MESS(12, BM_GETCHECK, 0, 0)) {
						memset(echbuf, 0, sizeof(echbuf));
						echpos = 0;
						echdsp = BASS_ChannelSetDSP(chan, &Echo, 0, 1);
					} else
						BASS_ChannelRemoveDSP(chan, echdsp);
					break;
				case 13: // toggle "flanger"
					if (MESS(13, BM_GETCHECK, 0, 0)) {
						memset(flabuf, 0, sizeof(flabuf));
						flapos = 0;
						flas = FLABUFLEN / 2;
						flasinc = 0.002f;
						fladsp = BASS_ChannelSetDSP(chan, &Flange, 0, 0);
					} else
						BASS_ChannelRemoveDSP(chan, fladsp);
					break;
			}
			break;

		case WM_INITDIALOG:
			win = h;
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = h;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
			// initialize first available ASIO device
			if (!BASS_ASIO_Init(-1, 0)) {
				Error("Can't find an available ASIO device");
				DestroyWindow(win);
				return 0;
			}
			// initialize BASS "no sound" device
			BASS_Init(0, 48000, 0, 0, NULL);
			return 1;

		case WM_DESTROY:
			BASS_ASIO_Free();
			BASS_Free();
			break;
	}
	return 0;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		MessageBox(0, "An incorrect version of BASS.DLL was loaded", 0, MB_ICONERROR);
		return 0;
	}

	DialogBox(hInstance, (char*)1000, 0, &dialogproc);

	return 0;
}
