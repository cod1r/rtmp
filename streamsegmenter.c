#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "streamsegmenter.h"
// NOTE: We are assuming the integers are 32 bits; Might have to rewrite...
// NOTE: when you use any bitwise operators, the number that is used to AND, for example is also affected by the endianness
// NOTE: the base used in the ISO/IEC 14496-12:2015 spec to represent bytes is 16, which is why some numbers might look weird or seem like a random value

void insert_integer(unsigned char* data, int offset, unsigned int number) {
	// converting back to system endian
	number = ntohl(number);
	data[offset] = (number >> 24) & 255;
	data[offset + 1] = (number >> 16) & 255;
	data[offset + 2] = (number >> 8) & 255;
	data[offset + 3] = number & 255;
}

void insert_short(unsigned char* data, int offset, unsigned short number) {
	// converting back to system endian
	number = ntohs(number);
	data[offset] = (number >> 8) & 255;
	data[offset + 1] = (number & 255);
}

void insert_box(unsigned char* data, int offset, Box box, int box_size) {
	for (int i = offset; i < box_size + offset; i++) {
		data[i] = box.data[i-offset];
	}
	free(box.data);
}

Box write_trex() {
	Box trex;
	trex.name = "trex";

	unsigned int version_and_flags = htonl(0);
	unsigned int track_id = htonl(1);
	unsigned int def_sample_desc_index = htonl(1);
	unsigned int def_sample_duration = htonl(0);
	unsigned int def_sample_size = htonl(0);
	unsigned int def_sample_flags = htonl(0);

	int trex_size = 
		sizeof(version_and_flags) + 
		sizeof(track_id) + 
		sizeof(def_sample_desc_index) + 
		sizeof(def_sample_duration) + 
		sizeof(def_sample_size) + 
		sizeof(def_sample_flags) + 4 + 4;
	trex.size = htonl(trex_size);
	trex.data = (unsigned char*)malloc(trex_size);

	insert_integer(trex.data, 0, trex.size);
	for (int i = 4; i < 8; i++) {
		trex.data[i] = trex.name[i-4];
	}
	insert_integer(trex.data, 8, version_and_flags);
	insert_integer(trex.data, 12, track_id);
	insert_integer(trex.data, 16, def_sample_desc_index);
	insert_integer(trex.data, 20, def_sample_duration);
	insert_integer(trex.data, 24, def_sample_size);
	insert_integer(trex.data, 28, def_sample_flags);
	return trex;
}

Box write_mvex() {
	Box mvex;
	mvex.name = "mvex";

	Box trex = write_trex();
	int trex_size = ntohl(trex.size);

	int mvex_size = trex_size + 4 + 4;
	mvex.size = htonl(mvex_size);
	mvex.data = (unsigned char*)malloc(mvex_size);

	insert_integer(mvex.data, 0, mvex.size);
	for (int i = 4; i < 8; i++) {
		mvex.data[i] = mvex.name[i-4];
	}
	insert_box(mvex.data, 8, trex, trex_size);
	return mvex;
}

Box write_stco() {
	Box stco;
	stco.name = "stco";
	unsigned int version_and_flags = 0;
	unsigned int entry_count = 0;
	int size = sizeof(version_and_flags) + sizeof(entry_count) + 4 + 4;
	stco.size = htonl(size);
	stco.data = (unsigned char*)malloc(size);
	insert_integer(stco.data, 0, stco.size);
	for (int i = 4; i < 8; i ++) {
		stco.data[i] = stco.name[i-4];
	}
	insert_integer(stco.data, 8, version_and_flags);
	insert_integer(stco.data, 12, entry_count);
	return stco;
}

Box write_stsz() {
	Box stsz;
	stsz.name = "stsz";
	unsigned int version_and_flags = 0;
	unsigned int samplesize = 0;
	unsigned int samplecount = 0;
	int size = 20;
	stsz.size = htonl(size);
	stsz.data = (unsigned char*)malloc(size);
	insert_integer(stsz.data, 0, stsz.size);
	for (int i = 4; i < 8; i++) {
		stsz.data[i] = stsz.name[i-4];
	}
	insert_integer(stsz.data, 8, version_and_flags);
	insert_integer(stsz.data, 12, samplesize);
	insert_integer(stsz.data, 16, samplecount);
	return stsz;
}

Box write_stsc() {
	Box stsc;
	unsigned int version_and_flags = 0;
	unsigned int entry_count = 0;
	stsc.name = "stsc";
	int size = 16;
	stsc.size = htonl(size);
	stsc.data = (unsigned char*)malloc(size);
	insert_integer(stsc.data, 0, stsc.size);
	for (int i = 4; i < 8; i++) {
		stsc.data[i] = stsc.name[i-4];
	}
	insert_integer(stsc.data, 8, version_and_flags);
	insert_integer(stsc.data, 12, entry_count);
	return stsc;
}

Box write_stts() {
	Box stts;
	unsigned int version_and_flags = 0;
	unsigned int entry_count = 0;
	stts.name = "stts";
	int stts_size = 16;
	stts.size = htonl(stts_size);
	stts.data = (unsigned char*)malloc(stts_size);
	insert_integer(stts.data, 0, stts.size);
	for (int i = 4; i < 8; i++) {
		stts.data[i] = stts.name[i-4];
	}
	insert_integer(stts.data, 8, version_and_flags);
	insert_integer(stts.data, 12, entry_count);
	return stts;
}

Box write_avcC(unsigned char* decoder_data, int decoder_size) {
	Box avcC;
	avcC.name = "avcC";
	int avcC_size = decoder_size + 4 + 4;
	avcC.size = htonl(avcC_size);
	avcC.data = (unsigned char*)malloc(avcC_size);

	insert_integer(avcC.data, 0, avcC.size);
	for (int i = 4; i < 8; i++) {
		avcC.data[i] = avcC.name[i-4];
	}
	for (int i = 8; i < avcC_size; i++) {
		avcC.data[i] = decoder_data[i-8];
	}
	return avcC;
}

Box write_audio_sample_entry() {
	Box audio_sample_entry;
	return audio_sample_entry;
}

/*
	SampleEntry
		aligned(8) abstract class SampleEntry (unsigned int(32) format)
			extends Box(format){
				const unsigned int(8)[6] reserved = 0;
				unsigned int(16) data_reference_index;
		} 
*/
// extends SampleEntry
Box write_visual_sample_entry() {
	Box visual_sample_entry;
	/*
		unsigned int(16) pre_defined = 0;
		const unsigned int(16) reserved = 0;
		unsigned int(32)[3] pre_defined = 0;
		unsigned int(16) width;
		unsigned int(16) height;
		template unsigned int(32) horizresolution = 0x00480000; // 72 dpi
		template unsigned int(32) vertresolution = 0x00480000; // 72 dpi
		const unsigned int(32) reserved = 0;
		template unsigned int(16) frame_count = 1;
		string[32] compressorname;
		template unsigned int(16) depth = 0x0018;
		int(16) pre_defined = -1;
		// other boxes from derived specifications
		CleanApertureBox clap; // optional
		PixelAspectRatioBox pasp; // optional 
	*/
	unsigned char sampleentry_reserved[6] = {0, 0, 0, 0, 0, 0};
	unsigned short data_reference_index = htons(1);
	unsigned short pre_defined = htons(0);
	unsigned short reserved = htons(0);
	unsigned int pre_defined2[3] = {htonl(0), htonl(0), htonl(0)};
	unsigned short width = htons(1280);
	unsigned short height = htons(720);
	unsigned int horizresolution = htonl(4718592);
	unsigned int vertresolution = htonl(4718592);
	unsigned int reserved2 = htonl(0);
	unsigned short frame_count = htons(1);
	unsigned char compressorname[32];
	for (int i = 0; i < 32; i ++) {
		compressorname[i] = 0;
	}
	unsigned short depth = htons(24);
	// same bit string as -1 but unsigned
	unsigned short pre_defined3 = htons(65535);

	int visual_size = sizeof(char)*6 + sizeof(data_reference_index) + sizeof(pre_defined) + sizeof(reserved) + sizeof(int)*3 + sizeof(width) + sizeof(height) + sizeof(horizresolution) + sizeof(vertresolution) + sizeof(reserved2) + sizeof(frame_count) + sizeof(char)*32 + sizeof(depth) + sizeof(pre_defined3); // no string name or size bytes
	visual_sample_entry.size = htonl(visual_size);
	visual_sample_entry.data = (unsigned char*)malloc(visual_size);
	for (int i = 0; i < 6; i++) {
		visual_sample_entry.data[i] = sampleentry_reserved[i];
	}
	insert_short(visual_sample_entry.data, 6, data_reference_index);
	insert_short(visual_sample_entry.data, 8, pre_defined);
	insert_short(visual_sample_entry.data, 10, reserved);
	for (int i = 0; i < 3; i++) {
		insert_integer(visual_sample_entry.data, 12+(i*4), pre_defined2[i]);
	}
	insert_short(visual_sample_entry.data, 24, width);
	insert_short(visual_sample_entry.data, 26, height);
	insert_integer(visual_sample_entry.data, 28, horizresolution);
	insert_integer(visual_sample_entry.data, 32, vertresolution);
	insert_integer(visual_sample_entry.data, 36, reserved2);
	insert_short(visual_sample_entry.data, 40, frame_count);
	for (int i = 42; i < 74; i ++) {
		visual_sample_entry.data[i] = compressorname[i-42];
	}
	insert_short(visual_sample_entry.data, 74, depth);
	insert_short(visual_sample_entry.data, 76, pre_defined3);
	return visual_sample_entry;
}


Box write_avc1(unsigned char* data, int size) {
	Box avc1;
	avc1.name = "avc1";

	Box visual_sample_entry = write_visual_sample_entry();
	int visual_size = ntohl(visual_sample_entry.size);
	//printf("visual size: %i\n", visual_size);

	Box avcC = write_avcC(data, size);
	int avcC_size = ntohl(avcC.size);

	int avc1_size = visual_size + avcC_size + 4 + 4; // visual size plus avcC size plus size bytes plus "avc1"
	avc1.size = htonl(avc1_size);
	avc1.data = (unsigned char*)malloc(avc1_size);

	insert_integer(avc1.data, 0, avc1.size);
	for (int i = 4; i < 8; i++) {
		avc1.data[i] = avc1.name[i-4];
	}
	insert_box(avc1.data, 8, visual_sample_entry, visual_size);
	insert_box(avc1.data, visual_size+8, avcC, avcC_size);
	return avc1;
}


Box write_stsd(unsigned char* data, int size) {
	Box stsd;
	/*
		unsigned int version_and_flags = 0;
		unsigned int entry_count;
	*/
	unsigned int version_and_flags = htonl(0);
	unsigned int entry_count = htonl(1);
	Box avc1 = write_avc1(data, size);
	int avc1_size = ntohl(avc1.size);
	stsd.name = "stsd";
	int stsd_size = avc1_size + 4 + 4 + 4 + 4; // avc1 size plus size bytes plus "stsd" plus version_and_flags plus entry_count
	stsd.size = htonl(stsd_size);
	stsd.data = (unsigned char*)malloc(stsd_size);
	insert_integer(stsd.data, 0, stsd.size);
	for (int i = 4; i < 8; i++) {
		stsd.data[i] = stsd.name[i-4];
	}
	insert_integer(stsd.data, 8, version_and_flags);
	insert_integer(stsd.data, 12, entry_count);
	// insert another box
	insert_box(stsd.data, 16, avc1, avc1_size);
	return stsd;
}

Box write_stbl(unsigned char* data, int size) {
	Box stbl;
	stbl.name = "stbl";

	Box stsd = write_stsd(data, size);
	int stsd_size = ntohl(stsd.size);

	Box stts = write_stts();
	int stts_size = ntohl(stts.size);
	
	Box stsc = write_stsc();
	int stsc_size = ntohl(stsc.size);

	Box stsz = write_stsz();
	int stsz_size = ntohl(stsz.size);

	Box stco = write_stco();
	int stco_size = ntohl(stco.size);

	int stbl_size = stsd_size + stts_size + stsc_size + stsz_size + stco_size + 4 + 4;
	stbl.size = htonl(stbl_size);
	stbl.data = (unsigned char*)malloc(stbl_size);
	insert_integer(stbl.data, 0, stbl.size);
	for (int i = 4; i < 8; i ++) {
		stbl.data[i] = stbl.name[i-4];
	}
	insert_box(stbl.data, 8, stsd, stsd_size);
	insert_box(stbl.data, stsd_size+8, stts, stts_size);
	insert_box(stbl.data, stts_size+stsd_size+8, stsc, stsc_size);
	insert_box(stbl.data, stsc_size+stts_size+stsd_size+8, stsz, stsz_size);
	insert_box(stbl.data, stsz_size+stsc_size+stts_size+stsd_size+8, stco, stco_size);

	return stbl;
}

Box write_dref() {
	Box dref;
	/*
		unsigned int version_and_flags = 0;
		unsigned int entry_count;
		unsigned int size;
		'url '
			{
				entry_flags: 32 bits
			}
	*/
	unsigned int version_and_flags = htonl(0);
	unsigned int entry_count = htonl(1);
	unsigned int url_size = htonl(12);
	char* url = "url ";
	unsigned int entry_flags = htonl(1);
	dref.name = "dref";
	int size = sizeof(version_and_flags) + sizeof(entry_count) + sizeof(url_size) + 4 + sizeof(entry_flags) + 4 + 4;
	dref.size = htonl(size);
	dref.data = (unsigned char*)malloc(size);
	insert_integer(dref.data, 0, dref.size);
	for (int i = 4; i < 8; i++) {
		dref.data[i] = dref.name[i-4];
	}
	insert_integer(dref.data, 8, version_and_flags);
	insert_integer(dref.data, 12, entry_count);
	insert_integer(dref.data, 16, url_size);
	for (int i = 20; i < 24; i++) {
		dref.data[i] = url[i-20];
	}
	insert_integer(dref.data, 24, entry_flags);
	return dref;
}

Box write_dinf() {
	Box dinf;
	dinf.name = "dinf";
	Box dref = write_dref();
	int dref_size = ntohl(dref.size);
	int dinf_size = dref_size + 8;
	dinf.data = (unsigned char*)malloc(dinf_size);
	dinf.size = htonl(dinf_size);
	insert_integer(dinf.data, 0, dinf.size);
	for (int i = 4; i < 8; i++) {
		dinf.data[i] = dinf.name[i-4];
	}
	insert_box(dinf.data, 8, dref, dref_size);
	return dinf;
}

Box write_vmhd() {
	Box vmhd;
	/*
		unsigned int(32) version_and_flags = 0;
		template unsigned int(16) graphicsmode = 0; // copy, see below 
		template unsigned int(16)[3] opcolor = {0, 0, 0}; 
	*/
	unsigned int version_and_flags = htonl(0);
	unsigned short graphics_mode = htons(0);
	unsigned short opcolor[3] = {htons(0), htons(0), htons(0)};

	vmhd.name = "vmhd";
	int vmhd_size = sizeof(version_and_flags) + sizeof(graphics_mode) + sizeof(short)*3 + 4 + 4;

	//printf("vmhd size: %i\n", vmhd_size);
	vmhd.size = htonl(vmhd_size);
	vmhd.data = (unsigned char*)malloc(vmhd_size);

	insert_integer(vmhd.data, 0, vmhd.size);
	for (int i = 4; i < 8; i ++) {
		vmhd.data[i] = vmhd.name[i-4];
	}

	insert_integer(vmhd.data, 8, version_and_flags);
	insert_short(vmhd.data, 12, graphics_mode);
	for (int i = 0; i < 3; i++) {
		insert_short(vmhd.data, 14+(i*2), opcolor[i]);
	}
	return vmhd;
}

Box write_minf(unsigned char* data, int size) {
	Box minf;
	minf.name = "minf";

	Box vmhd = write_vmhd();
	int vmhd_size = ntohl(vmhd.size);

	Box dinf = write_dinf();
	int dinf_size = ntohl(dinf.size);

	Box stbl = write_stbl(data, size);
	int stbl_size = ntohl(stbl.size);

	int minf_size = vmhd_size + dinf_size + stbl_size + 4 + 4;
	minf.size = htonl(minf_size);
	minf.data = (unsigned char*)malloc(minf_size);
	
	insert_integer(minf.data, 0, minf.size);
	for (int i = 4; i < 8; i++) {
		minf.data[i] = minf.name[i-4];
	}
	insert_box(minf.data, 8, vmhd, vmhd_size);
	insert_box(minf.data, vmhd_size+8, dinf, dinf_size);
	insert_box(minf.data, dinf_size+vmhd_size+8, stbl, stbl_size);
	return minf;
}

Box write_hdlr() {
	Box hdlr;
	/*
		unsigned int(32) version_and_flags = 0;
		unsigned int(32) pre_defined = 0;
		unsigned int(32) handler_type;
		const unsigned int(32)[3] reserved = 0;
		string name; 
	*/
	unsigned int version_and_flags = htonl(0);
	unsigned int pre_defined = htonl(0);
	char *handler_type = "vide"; // audio media uses 'soun'
	unsigned int reserved[3] = {0, 0, 0};
	char name[13] = {'V', 'i', 'd', 'e', 'o', 'H', 'a', 'n', 'd', 'l', 'e', 'r', 0};
	int hdlr_size = 
		sizeof(version_and_flags) + 
		sizeof(pre_defined) + 
		strlen(handler_type) + 
		sizeof(reserved) + 
		sizeof(name) + 4 + 4;
	hdlr.name = "hdlr";
	hdlr.size = htonl(hdlr_size);
	hdlr.data = (unsigned char*)malloc(hdlr_size);
	insert_integer(hdlr.data, 0, hdlr.size);
	for (int i = 4; i < 8; i++) {
		hdlr.data[i] = hdlr.name[i-4];
	}
	insert_integer(hdlr.data, 8, version_and_flags);
	insert_integer(hdlr.data, 12, pre_defined);
	for (int i = 16; i < 20; i++) {
		hdlr.data[i] = handler_type[i-16];
	}
	for (int i = 0; i < 3; i++) {
		insert_integer(hdlr.data, 20+(i*4), reserved[i]);
	}
	for (int i = 32; i < 32 + sizeof(name); i++) {
		hdlr.data[i] = name[i-32];
	}
	return hdlr;
}

Box write_mdhd() {
	Box mdhd;
	/*
		unsigned int(32) version_and_flags;
		unsigned int(32) creation_time;
		unsigned int(32) modification_time;
		unsigned int(32) timescale;
		unsigned int(32) duration; 
		bit(1) pad = 0;
		unsigned int(5)[3] language; // ISO-639-2/T language code
		unsigned int(16) pre_defined = 0;
	*/
	unsigned int version_and_flags = htonl(0);
	unsigned int creation_time = htonl(0);
	unsigned int modification_time = htonl(0);
	unsigned int timescale = htonl(30000);
	unsigned int duration = htonl(0);
	unsigned short pad_and_language_code = htons(21956);
	unsigned short pre_defined = htons(0);
	mdhd.name = "mdhd";
	int mdhd_size = 
		sizeof(version_and_flags) + 
		sizeof(creation_time) + 
		sizeof(modification_time) + 
		sizeof(timescale) + 
		sizeof(duration) +
		sizeof(pad_and_language_code) + 
		sizeof(pre_defined) + 4 + 4;
	//printf("mdhd size: %i\n", mdhd_size);
	mdhd.size = htonl(mdhd_size);
	mdhd.data = (unsigned char*)malloc(mdhd_size);
	insert_integer(mdhd.data, 0, mdhd.size);
	for (int i = 4; i < 8; i++) {
		mdhd.data[i] = mdhd.name[i-4];
	}
	insert_integer(mdhd.data, 8, version_and_flags);
	insert_integer(mdhd.data, 12, creation_time);
	insert_integer(mdhd.data, 16, modification_time);
	insert_integer(mdhd.data, 20, timescale);
	insert_integer(mdhd.data, 24, duration);
	insert_short(mdhd.data, 28, pad_and_language_code);
	insert_short(mdhd.data, 30, pre_defined);
	return mdhd;
}

Box write_mdia(unsigned char* decoderinfo, int decoder_size) {
	Box mdia;
	mdia.name = "mdia";

	Box mdhd = write_mdhd();
	int mdhd_size = ntohl(mdhd.size);

	Box hdlr = write_hdlr();
	int hdlr_size = ntohl(hdlr.size);

	Box minf = write_minf(decoderinfo, decoder_size);
	int minf_size = ntohl(minf.size);

	int mdia_size = mdhd_size + hdlr_size + minf_size + 4 + 4;
	mdia.size = htonl(mdia_size);
	mdia.data = (unsigned char*)malloc(mdia_size);

	insert_integer(mdia.data, 0, mdia.size);
	for (int i = 4; i < 8; i ++) {
		mdia.data[i] = mdia.name[i-4];
	}
	insert_box(mdia.data, 8, mdhd, mdhd_size);
	insert_box(mdia.data, mdhd_size+8, hdlr, hdlr_size);
	insert_box(mdia.data, hdlr_size + mdhd_size + 8, minf, minf_size);
	return mdia;
}

Box write_tkhd() {
	Box tkhd;
	tkhd.name = "tkhd";
	/*
		unsigned int(32) version_and_flags;
		unsigned int(32) creation_time;
		unsigned int(32) modification_time;
		unsigned int(32) track_ID;
		const unsigned int(32) reserved = 0;
		unsigned int(32) duration;
		const unsigned int(32)[2] reserved = 0;
		template int(16) layer = 0;
		template int(16) alternate_group = 0;
		template int(16) volume = {if track_is_audio 0x0100 else 0};
		const unsigned int(16) reserved = 0;
		template int(32)[9] matrix=
		{ 0x00010000,0,0,0,0x00010000,0,0,0,0x40000000 };
		// unity matrix
		unsigned int(32) width;
		unsigned int(32) height; 
	*/
	unsigned int version_and_flags = htonl(0 + 1 + 2);
	unsigned int creation_time = htonl(0);
	unsigned int modification_time = htonl(0);
	unsigned int track_ID = htonl(1);
	unsigned int reserved = htonl(0);
	unsigned int duration = htonl(0);
	unsigned int reserved_two[2] = {htonl(0), htonl(0)};
	unsigned short layer = htons(0);
	unsigned short alt_group = htons(0);
	unsigned short volume = htons(0); // if audio then this needs to be 8
	unsigned short reserved_three = htons(0);
	unsigned int matrix[9] = {htonl(65536), htonl(0), htonl(0), htonl(0), htonl(65536), htonl(0), htonl(0), htonl(0), htonl(1073741824)};
	unsigned int width = htonl(0);
	unsigned int height = htonl(0);

	int tkhd_size = 
		sizeof(version_and_flags) + 
		sizeof(creation_time) + 
		sizeof(modification_time) + 
		sizeof(track_ID) + 
		sizeof(reserved) + 
		sizeof(duration) +
		sizeof(int)*2 + 
		sizeof(layer) + 
		sizeof(alt_group) + 
		sizeof(volume) + 
		sizeof(reserved_three) + 
		sizeof(int)*9 + 
		sizeof(width) + 
		sizeof(height) + 4 + 4;

	tkhd.size = htonl(tkhd_size);
	tkhd.data = (unsigned char*)malloc(tkhd_size);

	insert_integer(tkhd.data, 0, tkhd.size);
	for (int i = 4; i < 8; i++) {
		tkhd.data[i] = tkhd.name[i - 4];
	}
	insert_integer(tkhd.data, 8, version_and_flags);
	insert_integer(tkhd.data, 12, creation_time);
	insert_integer(tkhd.data, 16, modification_time);
	insert_integer(tkhd.data, 20, track_ID);
	insert_integer(tkhd.data, 24, reserved);
	insert_integer(tkhd.data, 28, duration);
	for (int i = 0; i < 2; i++) {
		insert_integer(tkhd.data, 32 + (i * 4), reserved_two[i]);
	}
	insert_short(tkhd.data, 40, layer);
	insert_short(tkhd.data, 42, alt_group);
	insert_short(tkhd.data, 44, volume);
	insert_short(tkhd.data, 46, reserved_three);
	for (int i = 0; i < 9; i++) {
		insert_integer(tkhd.data, 48 + (i * 4), matrix[i]);
	}
	insert_integer(tkhd.data, 84, width);
	insert_integer(tkhd.data, 88, height);
	return tkhd;
}

Box write_trak(unsigned char* decoderinfo, int decoder_size) {
	Box trak;
	trak.name = "trak";
	Box tkhd = write_tkhd();
	int tkhd_size = ntohl(tkhd.size);
	Box mdia = write_mdia(decoderinfo, decoder_size);
	int mdia_size = ntohl(mdia.size);
	int trak_size = tkhd_size + mdia_size + 4 + 4;
	trak.size = htonl(trak_size);
	trak.data = (unsigned char *)malloc(trak_size);
	insert_integer(trak.data, 0, trak.size);
	for (int i = 4; i < 8; i ++) {
		trak.data[i] = trak.name[i-4];
	}
	insert_box(trak.data, 8, tkhd, tkhd_size);
	insert_box(trak.data, tkhd_size+8, mdia, mdia_size);
	return trak;
}

Box write_mvhd() {
	Box mvhd;
	/*
		unsigned int(32) version_and_flags;
		unsigned int(32) creation_time;
		unsigned int(32) modification_time;
		unsigned int(32) timescale;
		unsigned int(32) duration; 
		template int(32) rate = 0x00010000; // typically 1.0
		template int(16) volume = 0x0100; // typically, full volume
		const bit(16) reserved = 0;
		const unsigned int(32)[2] reserved = 0;
		template int(32)[9] matrix =
		{ 0x00010000,0,0,0,0x00010000,0,0,0,0x40000000 };
		// Unity matrix
		bit(32)[6] pre_defined = 0;
		unsigned int(32) next_track_ID; 
	*/
	unsigned int version_and_flags = htonl(0);
	unsigned int creation_time = htonl(0);
	unsigned int modification_time = htonl(0);
	unsigned int timescale = htonl(1000);
	unsigned int duration = htonl(0);
	unsigned int rate = htonl(0x00010000);
	unsigned short volume = htons(256);
	unsigned short reserved = htons(0);
	unsigned int reserved_two[2] = {htonl(0), htonl(0)};
	unsigned int matrix[9] = {htonl(65536), htonl(0), htonl(0), htonl(0), htonl(65536), htonl(0), htonl(0), htonl(0), htonl(1073741824)};
	unsigned int pre_defined[6] = {0, 0, 0, 0, 0, 0};
	// no idea why this is 2. The init file generated from ffmpeg makes this 2
	unsigned int next_track_ID = htonl(2);
	int size = 
		sizeof(version_and_flags) +
		sizeof(creation_time) +
		sizeof(modification_time) +
		sizeof(timescale) +
		sizeof(duration) +
		sizeof(rate) +
		sizeof(volume) +
		sizeof(reserved) +
		sizeof(reserved_two) +
		sizeof(matrix) +
		sizeof(pre_defined) +
		sizeof(next_track_ID) + 4 + 4;
	mvhd.size = htonl(size);
	mvhd.data = (unsigned char*)malloc(size);
	insert_integer(mvhd.data, 0, mvhd.size);
	mvhd.name = "mvhd";
	for (int i = 4; i < 8; i++) {
		mvhd.data[i] = mvhd.name[i - 4];
	}
	insert_integer(mvhd.data, 8, version_and_flags);
	insert_integer(mvhd.data, 12, creation_time);
	insert_integer(mvhd.data, 16, modification_time);
	insert_integer(mvhd.data, 20, timescale);
	insert_integer(mvhd.data, 24, duration);
	insert_integer(mvhd.data, 28, rate);
	insert_short(mvhd.data, 32, volume);
	insert_short(mvhd.data, 34, reserved);
	for (int i = 0; i < 2; i++) {
		insert_integer(mvhd.data, 36 + (i * 4), reserved_two[i]);
	}
	for (int i = 0; i < 9; i++) {
		insert_integer(mvhd.data, 44 + (i * 4), matrix[i]);
	}
	for (int i = 0; i < 6; i++) {
		insert_integer(mvhd.data, 80 + (i * 4), pre_defined[i]);
	}
	insert_integer(mvhd.data, 104, next_track_ID);
	return mvhd;
}

Box write_moov(unsigned char* decoder_info, int decoder_size) {
	Box moov;
	moov.name = "moov";

	Box mvhd = write_mvhd();
	int mvhd_size = ntohl(mvhd.size);

	Box trak = write_trak(decoder_info, decoder_size);
	int trak_size = ntohl(trak.size);

	Box mvex = write_mvex();
	int mvex_size = ntohl(mvex.size);

	int moov_size = mvhd_size + trak_size + mvex_size + 4 + 4;
	moov.size = htonl(moov_size);
	moov.data = (unsigned char*)malloc(moov_size);

	insert_integer(moov.data, 0, moov.size);
	for (int i = 4; i < 8; i++) {
		moov.data[i] = moov.name[i-4];
	}

	insert_box(moov.data, 8, mvhd, mvhd_size);
	insert_box(moov.data, mvhd_size + 8, trak, trak_size);
	insert_box(moov.data, trak_size + mvhd_size + 8, mvex, mvex_size);
	return moov;
}

Box write_ftyp() {
	Box ftyp;
	ftyp.name = "ftyp";
	const unsigned char ftyp_data[] = {0, 0, 0, 24, 102, 116, 121, 112, 105, 115, 111, 53, 0, 0, 2, 0, 105, 115, 111, 54, 109, 112, 52, 49};
	int ftyp_size = 24;
	ftyp.size = htonl(ftyp_size);
	ftyp.data = (unsigned char*)malloc(ftyp_size);

	for (int i = 0; i < ftyp_size; i++) {
		ftyp.data[i] = ftyp_data[i];
	}
	return ftyp;
}

void write_init(unsigned char* avcC, int size_of_avcC) {
	FILE *file = fopen("init.mp4", "w");
	
	Box ftyp = write_ftyp();
	fwrite(ftyp.data, ntohl(ftyp.size), 1, file);

	Box moov = write_moov(avcC, size_of_avcC);
	fwrite(moov.data, ntohl(moov.size), 1, file);

	free(ftyp.data);
	free(moov.data);
	fclose(file);
}

// --- Segment Part ---

Box write_styp() {
	Box styp;
	styp.name = "styp";
	const unsigned char styp_data[] = {0, 0, 0, 24, 115, 116, 121, 112, 105, 115, 111, 53, 0, 0, 2, 0, 105, 115, 111, 54, 109, 112, 52, 49};
	int styp_size = 24;
	styp.size = htonl(styp_size);
	styp.data = (unsigned char*)malloc(styp_size);

	for (int i = 0; i < styp_size; i++) {
		styp.data[i] = styp_data[i];
	}
	return styp;
}

Box write_sidx(int ept, int referenced_size, int composition_time_offset_sum /* <- this is the subsegment_duration */) {
	Box sidx;
	sidx.name = "sidx";

	unsigned int version_and_flags = htonl(0);
	unsigned int reference_id = htonl(1);
	unsigned int timescale = htonl(30000);
	// we need this so that the decoder can know which segments are before which
	unsigned int earliest_presentation_time = htonl(ept);
	unsigned int first_offset = htonl(0);
	unsigned short reserved = htons(0);
	unsigned short reference_count = htons(1);
	unsigned int reference_type_reference_size = htonl(referenced_size); //size minus all the bytes before the moof atom I think. But for now I will just put it as zero
	// this might have to change if any problems arise...
	unsigned int subsegment_duration = htonl(composition_time_offset_sum);
	// SAP -> Stream Access Point
	unsigned int starts_with_sap_and_sap_type_and_sap_delta_time = htonl(1 << 31);

	int sidx_size = 
		sizeof(version_and_flags) + 
		sizeof(reference_id) + 
		sizeof(timescale) + 
		sizeof(earliest_presentation_time) + 
		sizeof(first_offset) + 
		sizeof(reserved) + 
		sizeof(reference_count) + 
		sizeof(reference_type_reference_size) + 
		sizeof(subsegment_duration) + 
		sizeof(starts_with_sap_and_sap_type_and_sap_delta_time) + 4 + 4;

	sidx.size = htonl(sidx_size);
	sidx.data = (unsigned char*)malloc(sidx_size);

	insert_integer(sidx.data, 0, sidx.size);
	for (int i = 4; i < 8; i++) {
		sidx.data[i] = sidx.name[i-4];
	}
	insert_integer(sidx.data, 8, version_and_flags);
	insert_integer(sidx.data, 12, reference_id);
	insert_integer(sidx.data, 16, timescale);
	insert_integer(sidx.data, 20, earliest_presentation_time);
	insert_integer(sidx.data, 24, first_offset);
	insert_short(sidx.data, 28, reserved);
	insert_short(sidx.data, 30, reference_count);
	insert_integer(sidx.data, 32, reference_type_reference_size);
	insert_integer(sidx.data, 36, subsegment_duration);
	insert_integer(sidx.data, 40, starts_with_sap_and_sap_type_and_sap_delta_time);
	return sidx;
}

Box write_trun(SampleData samples[], int samples_size) {
	/*
	 * Flags:
	 * 		sample size (each sample has its own size)
	 * 		sample flags
	 * 		sample composition time
	 * 		data offset
	 */
	Box trun;
	trun.name = "trun";
	int num_samples = 0;
	unsigned int version_and_flags = htonl(0 + 512 + 1024 + 2048 + 1);
	for (int i = 0; i < samples_size; i++) {
		int byte = 0;
		while (byte < samples[i].sample_size) {
			int nal_unit_size = (samples[i].data[byte] << 24) | (samples[i].data[byte+1] << 16) | (samples[i].data[byte+2] << 8) | (samples[i].data[byte+3]);
			int nal_unit_type = samples[i].data[byte+4] & 31;
			if (nal_unit_type < 6 || nal_unit_type > 18) {
				num_samples ++;
			}
			byte += nal_unit_size + 4;
		}

	}
	SampleData partitioned_samples[num_samples];
	for (int i = 0; i < num_samples; i++) {
		partitioned_samples[i].sample_size = 0;
		partitioned_samples[i].composition_time = 0;
		partitioned_samples[i].flags = 0;
	}
	int partition_index = 0;
	for (int i = 0; i < samples_size; i++) {
		int byte = 0;
		while (byte < samples[i].sample_size) {
			int nal_unit_size = (samples[i].data[byte] << 24) | (samples[i].data[byte+1] << 16) | (samples[i].data[byte+2] << 8) | (samples[i].data[byte+3]);
			int nal_unit_type = samples[i].data[byte+4] & 31;
			partitioned_samples[partition_index].sample_size += nal_unit_size + 4;
			partitioned_samples[partition_index].composition_time = samples[i].composition_time;
			if (nal_unit_type < 6 || nal_unit_type > 18) {
				partition_index++;
			}
			byte += nal_unit_size + 4;
		}
	}
	unsigned int big_endian_sample_count = htonl(num_samples);
	// adding constants to the data offset which includes atoms above the 'trun' atom
	unsigned int data_offset = htonl(
									// each sample has 12 bytes and we add 8 because of the 'mdat' string and the mdat size bytes
									12* num_samples + 8 +
									strlen("moof") + 4 +
									strlen("mfhd") + 4 + 4 + 4 +
									strlen("traf") + 4 + 
									strlen("tfhd") + 4 + 4 + 4 + 4 + 4 + 4 +
									strlen("tfdt") + 4 + 4 + 4 +
									strlen("trun") + 4 + 4 + 4 + 4
								);

	int trun_size =
		sizeof(version_and_flags) +
		sizeof(big_endian_sample_count) +
		sizeof(data_offset) +
		12* num_samples + 4 + 4;

	trun.size = htonl(trun_size);
	trun.data = (unsigned char*)malloc(trun_size);

	insert_integer(trun.data, 0, trun.size);
	for (int i = 4; i < 8; i++) {
		trun.data[i] = trun.name[i-4];
	}
	insert_integer(trun.data, 8, version_and_flags);
	insert_integer(trun.data, 12, big_endian_sample_count);
	insert_integer(trun.data, 16, data_offset);
	for (int i = 0; i < num_samples; i++) {
		insert_integer(trun.data, 20+(i*12), htonl(partitioned_samples[i].sample_size));
		insert_integer(trun.data, 24+(i*12), htonl(partitioned_samples[i].flags));
		insert_integer(trun.data, 28+(i*12), htonl(partitioned_samples[i].composition_time));
	}
	return trun;
}

Box write_tfdt(int decode_time) {
	Box tfdt;
	tfdt.name = "tfdt";
	unsigned int version_and_flags = htonl(0);
	unsigned int baseMediaDecodeTime_1 = htonl(decode_time);

	int tfdt_size = 
		sizeof(version_and_flags) + 
		sizeof(baseMediaDecodeTime_1) + 4 + 4;

	tfdt.size = htonl(tfdt_size);
	tfdt.data = (unsigned char*)malloc(tfdt_size);

	insert_integer(tfdt.data, 0, tfdt.size);
	for (int i = 4; i < 8; i++) {
		tfdt.data[i] = tfdt.name[i-4];
	}
	insert_integer(tfdt.data, 8, version_and_flags);
	insert_integer(tfdt.data, 12, baseMediaDecodeTime_1);
	return tfdt;
}

Box write_tfhd() {
	/*
	 * Flags: 
	 * 		def sample duration
	 * 		def sample size
	 * 		def sample flags
	 */
	Box tfhd;
	tfhd.name = "tfhd";

	unsigned int version_and_flags = htonl(0 + 131072 + 16 + 32 + 8);
	unsigned int track_id = htonl(1);
	// optional below
	unsigned int def_sample_duration = htonl(1001);
	unsigned int def_sample_size = htonl(0);
	unsigned int def_sample_flags = htonl(0);

	int tfhd_size = 
		sizeof(version_and_flags) + 
		sizeof(track_id) + 
		sizeof(def_sample_duration) +
		sizeof(def_sample_size) + 
		sizeof(def_sample_flags) + 4 + 4;
	tfhd.size = htonl(tfhd_size);
	tfhd.data = (unsigned char*)malloc(tfhd_size);
	
	insert_integer(tfhd.data, 0, tfhd.size);
	for (int i = 4; i < 8; i++) {
		tfhd.data[i] = tfhd.name[i-4];
	}
	insert_integer(tfhd.data, 8, version_and_flags);
	insert_integer(tfhd.data, 12, track_id);
	insert_integer(tfhd.data, 16, def_sample_duration);
	insert_integer(tfhd.data, 20, def_sample_size);
	insert_integer(tfhd.data, 24, def_sample_flags);
	return tfhd;
}

Box write_traf(SampleData samples[], int samples_size, int decode_time) {
	Box traf;
	traf.name = "traf";

	Box tfhd = write_tfhd();
	int tfhd_size = ntohl(tfhd.size);

	Box tfdt = write_tfdt(decode_time);
	int tfdt_size = ntohl(tfdt.size);

	Box trun = write_trun(samples, samples_size);
	int trun_size = ntohl(trun.size);
	
	int traf_size = tfhd_size + tfdt_size + trun_size + 4 + 4;
	traf.size = htonl(traf_size);
	traf.data = (unsigned char *)malloc(traf_size);

	insert_integer(traf.data, 0, traf.size);
	for (int i = 4; i < 8; i++) {
		traf.data[i] = traf.name[i-4];
	}
	insert_box(traf.data, 8, tfhd, tfhd_size);
	insert_box(traf.data, tfhd_size + 8, tfdt, tfdt_size);
	insert_box(traf.data, tfdt_size + tfhd_size + 8, trun, trun_size);
	return traf;
}

Box write_mfhd(int file_number /* <- sequence_number */) {
	Box mfhd;
	mfhd.name = "mfhd";

	unsigned int version_and_flags = htonl(0);
	unsigned int sequence_number = htonl(file_number);

	int mfhd_size = sizeof(version_and_flags) + sizeof(sequence_number) + 4 + 4;
	mfhd.size = htonl(mfhd_size);
	mfhd.data = (unsigned char*)malloc(mfhd_size);

	insert_integer(mfhd.data, 0, mfhd.size);
	for (int i = 4; i < 8; i++) {
		mfhd.data[i] = mfhd.name[i-4];
	}
	insert_integer(mfhd.data, 8, version_and_flags);
	insert_integer(mfhd.data, 12, sequence_number);
	return mfhd;
}

Box write_moof(SampleData samples[], int samples_size, int file_number, int decode_time) {
	Box moof;
	moof.name = "moof";

	Box mfhd = write_mfhd(file_number);
	int mfhd_size = ntohl(mfhd.size);

	Box traf = write_traf(samples, samples_size, decode_time);
	int traf_size = ntohl(traf.size);

	int moof_size = mfhd_size + traf_size + 4 + 4;
	moof.size = htonl(moof_size);
	moof.data = (unsigned char*)malloc(moof_size);

	insert_integer(moof.data, 0, moof.size);
	for (int i = 4; i < 8; i++) {
		moof.data[i] = moof.name[i-4];
	}
	insert_box(moof.data, 8, mfhd, mfhd_size);
	insert_box(moof.data, mfhd_size + 8, traf, traf_size);
	return moof;
}

Box write_mdat(unsigned char* data, int size) {
	Box mdat;
	mdat.name = "mdat";

	int mdat_size = size + 4 + 4;
	mdat.size = htonl(mdat_size);
	mdat.data = (unsigned char*)malloc(mdat_size);

	insert_integer(mdat.data, 0, mdat.size);
	for (int i = 4; i < 8; i++) {
		mdat.data[i] = mdat.name[i-4];
	}

	for (int i = 8; i < size + 8; i++) {
		mdat.data[i] = data[i-8];
	}

	return mdat;
}
// This function creates an MP4 Fragment/Segment
// NOTE: ept means earliest presentation time
void write_segment(SampleData *samples, int samples_size, int file_number, int ept, int composition_time_offset_sum, int decode_time) {
	char file_name[50000];
	sprintf(file_name, "sequence%i.mp4", file_number);
	FILE *segment_file = fopen(file_name, "w");

	unsigned int summed_size = 0;
	for (int i = 0; i < samples_size; i++) {
		summed_size += samples[i].sample_size;
	}
	unsigned char* data = (unsigned char*)malloc(summed_size);
	int data_index = 0;
	for (int i = 0; i < samples_size; i++) {
		for (int j = data_index; j < samples[i].sample_size + data_index; j++) {
			data[j] = samples[i].data[j-data_index];
		}
		data_index += samples[i].sample_size;
	}

	//Box styp = write_styp();
	Box moof = write_moof(samples, samples_size, file_number, decode_time);
	Box mdat = write_mdat(data, summed_size);
	Box sidx = write_sidx(ept, ntohl(moof.size) + ntohl(mdat.size), composition_time_offset_sum);

	//fwrite(styp.data, ntohl(styp.size), 1, segment_file);
	fwrite(sidx.data, ntohl(sidx.size), 1, segment_file);
	fwrite(moof.data, ntohl(moof.size), 1, segment_file);
	fwrite(mdat.data, ntohl(mdat.size), 1, segment_file);
	//free(styp.data);
	free(sidx.data);
	free(moof.data);
	free(mdat.data);
	fclose(segment_file);
}

// Playlist file part
// This function creates the playlist file
void write_playlist() {
	FILE* playlist_file = fopen("index.m3u8", "w");
	char* format = "#EXTM3U\n";
	char* version = "#EXT-X-VERSION:7\n";
	char target_duration[5000];
	sprintf(target_duration, "#EXT-X-TARGETDURATION:%i\n", INTERVAL/1000);
	char* playlist_type = "#EXT-X-PLAYLIST-TYPE:EVENT\n";
	char* uri_init = "#EXT-X-MAP:URI=\"init.mp4\"\n";
	fwrite(format, strlen(format), 1, playlist_file);
	fwrite(version, strlen(version), 1, playlist_file);
	fwrite(target_duration, strlen(target_duration), 1, playlist_file);
	fwrite(playlist_type, strlen(playlist_type), 1, playlist_file);
	fwrite(uri_init, strlen(uri_init), 1, playlist_file);
	fclose(playlist_file);
}

void append_playlist(int file_number) {
	char seq[5000];
	sprintf(seq, "#EXTINF:%i.000000,\nsequence%i.mp4\n", INTERVAL/1000, file_number);
	FILE* playlist_file = fopen("index.m3u8", "a");
	fwrite(seq, strlen(seq), 1, playlist_file);
	fclose(playlist_file);
}
