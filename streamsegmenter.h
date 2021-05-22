#include <stdio.h>
void insert_integer(unsigned char*, int, unsigned int);
void insert_short(unsigned char*, int, unsigned short);
void write_init(unsigned char*, int);
typedef struct Box {
	int size; // refers to how many bytes there are
	unsigned char* name;
	unsigned char* data;
} Box;
void insert_box(unsigned char*, int, Box, int);
Box write_trex();
Box write_mvex();
Box write_stco();
Box write_stsz();
Box write_stsc();
Box write_stts();
Box write_avcC(unsigned char*, int);
Box write_audio_sample_entry();
Box write_visual_sample_entry();
Box write_avc1(unsigned char*, int);
Box write_stsd(unsigned char*, int);
Box write_stbl(unsigned char*, int);
Box write_dref();
Box write_dinf();
Box write_vmhd();
Box write_minf(unsigned char*, int);
Box write_hdlr();
Box write_mdhd();
Box write_mdia(unsigned char*, int);
Box write_tkhd();
Box write_trak();
Box write_mvhd();
Box write_moov(unsigned char*, int);
void write_segment(unsigned char*, int, int);
void write_playlist(FILE*);
void append_segment(FILE*, int);
