#include "libfaad/include/faad.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct WavFileHeader
{
    char        id[4];          // should always contain "RIFF"
    int     totallength;    // total file length minus 8
    char        wavefmt[8];     // should be "WAVEfmt "
    int     format;         // 16 for PCM format
    short     pcm;            // 1 for PCM format
    short     channels;       // channels
    int     frequency;      // sampling frequency
	int     bytes_per_second;
    short     bytes_by_capture;
    short     bits_per_sample;
    char        data[4];        // should always contain "data"
    int     bytes_in_data;
};
void write_wav_header(FILE* file, int totalsamcnt_per_channel, int samplerate, int channels){
    struct WavFileHeader filler;
    strcpy(filler.id, "RIFF");
    filler.bits_per_sample = 16;
    filler.totallength = (totalsamcnt_per_channel * channels * filler.bits_per_sample/8) + sizeof(filler) - 8; //81956
    strcpy(filler.wavefmt, "WAVEfmt ");
    filler.format = 16;
    filler.pcm = 1;
    filler.channels = channels;
    filler.frequency = samplerate;
    filler.bytes_per_second = filler.channels * filler.frequency * filler.bits_per_sample/8;
    filler.bytes_by_capture = filler.channels*filler.bits_per_sample/8;
    filler.bytes_in_data = totalsamcnt_per_channel * filler.channels * filler.bits_per_sample/8;    
    strcpy(filler.data, "data");
    fwrite(&filler, 1, sizeof(filler), file);
}

int main(int argc, char *argv[])
{
	printf("hello faad\n");
	NeAACDecHandle faadhandle = NeAACDecOpen();
	if (faadhandle) {
		printf("aacopen ok\n");	
		const char* aacfile = "aac20s.aac";
		FILE* file = fopen(aacfile, "rb");
		if (file) {
			printf("fopen aac ok\n");
			fseek(file, 0, SEEK_END);
			long filelen = ftell(file);
			fseek(file, 0, SEEK_SET);
			unsigned char* filebuf = (unsigned char*)malloc(filelen);
			int len = fread(filebuf, 1, filelen, file);
			fclose(file);
			unsigned long samplerate = 0;
			unsigned char channel = 0;
			int ret = NeAACDecInit(faadhandle, filebuf, len, &samplerate, &channel);
			if (ret >= 0) {
				printf("aacinit ok: sam=%lu, chn=%d\n", samplerate, channel);
				NeAACDecFrameInfo frameinfo;
				unsigned char* curbyte = filebuf;
				unsigned long leftsize = len;
				const char* wavename = "out.wav";
				FILE* wavfile = fopen(wavename, "wb");
				if (wavfile) {
					int wavheadsize = sizeof(struct WavFileHeader);
					fseek(wavfile, wavheadsize, SEEK_SET);
					int totalsmp_per_chl = 0;
					void* out = NULL;
					while (out = NeAACDecDecode(faadhandle, &frameinfo, curbyte, leftsize)) {
						printf("decode one frame ok: sam:%ld, chn=%d, samplecount=%ld, obj_type=%d, header_type=%d, consumed=%ld\n",
								frameinfo.samplerate, frameinfo.channels, frameinfo.samples, frameinfo.object_type,
								frameinfo.header_type, frameinfo.bytesconsumed);
						curbyte += frameinfo.bytesconsumed;
						leftsize -= frameinfo.bytesconsumed;
						fwrite(out, 1, frameinfo.samples*2, wavfile); // frameinfo.samples是所有声道数的样本总和；16bit位深
						totalsmp_per_chl += frameinfo.samples / frameinfo.channels;
					}
					printf("aac decode done, totalsmp_per_chl=%d\n", totalsmp_per_chl);
					fseek(wavfile, 0, SEEK_SET);
					write_wav_header(wavfile, totalsmp_per_chl, (int)samplerate, (int)channel);
					fclose(wavfile);
				}
			}
			free(filebuf);
		}
		NeAACDecClose(faadhandle);
	}
	return 0;
}
