/*
	CD version of the BASS console WAVE writer
	Copyright (c) 2002-2022 Un4seen Developments Ltd.
*/

#include <stdio.h>
#include <conio.h>
#include "bass.h"
#include "basscd.h"

// display error messages
void Error(char *text)
{
	printf("Error(%d): %s\n", BASS_ErrorGetCode(), text);
	BASS_Free();
	ExitProcess(0);
}

int main(int argc, char **argv)
{
	DWORD chan, p;
	QWORD len, pos;
	FILE *fp;
	WAVEFORMATEX wf;

	printf("BASSCD WAVE writer example\n"
		"--------------------------\n");

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		printf("An incorrect version of BASS was loaded");
		return;
	}

	if (argc != 2) {
		printf("\tusage: writewav <file.cda>\n");
		return;
	}

	// initalize "no sound" device
	if (!BASS_Init(0, 44100, 0, 0, NULL))
		Error("Can't initialize device");

	// try streaming the file
	chan = BASS_CD_StreamCreateFile(argv[1], BASS_STREAM_DECODE);
	if (!chan) Error("Can't handle the file");

	len = BASS_ChannelGetLength(chan, BASS_POS_BYTE);

	printf("output: bass.wav\n");
	if (!(fp = fopen("bass.wav", "wb"))) Error("Can't create output file");
	// write WAV header
	wf.wFormatTag = 1;
	wf.nChannels = 2;
	wf.wBitsPerSample = 16;
	wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
	wf.nSamplesPerSec = 44100;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
	fwrite("RIFF\0\0\0\0WAVEfmt \20\0\0\0", 20, 1, fp);
	fwrite(&wf, 16, 1, fp);
	fwrite("data\0\0\0\0", 8, 1, fp);

	while (!_kbhit()) {
		BYTE buf[20000];
		int c = BASS_ChannelGetData(chan, buf, sizeof(buf));
		if (c == -1) break;
		fwrite(buf, 1, c, fp);
		pos = BASS_ChannelGetPosition(chan, BASS_POS_BYTE);
		printf(" (press any key to stop)\rdone: %lld / %lld", pos, len);
		fflush(stdout);
	}
	printf("                        \n");

	// complete WAV header
	fflush(fp);
	p = ftell(fp);
	fseek(fp, 4, SEEK_SET);
	putw(p - 8, fp);
	fflush(fp);
	fseek(fp, 40, SEEK_SET);
	putw(p - 44, fp);
	fflush(fp);
	fclose(fp);

	BASS_Free();
	return 0;
}
