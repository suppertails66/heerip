/* Container for PCM data and related metadata (channels,
   samplerate, etc.) */

#include <cstring>
#include <algorithm>
#include <string>

#include "datmanip.h"

namespace RipUtil
{


namespace WaveWriterConsts
{
	const static char riff_chunk_id[4]
		= { 'R', 'I', 'F', 'F' };
	const static char riff_format_id[4]
		= { 'W', 'A', 'V', 'E' };
	const static char riff_schunk1_id[4]
		= { 'f', 'm', 't', ' ' };
	const static char riff_schunk2_id[4]
		= { 'd', 'a', 't', 'a' };
};

class PCMData 
{
public:
	enum LoopStyle
	{
		fadeloop, tailloop
	};
	PCMData()
		: waveform(0), wavesize(0), channels(0), samprate(0), sampwidth(0),
		sign(DatManip::has_nosign), end(DatManip::le), 
		looping(false), loopstart(0), loopend(0) { };
	PCMData(int chans, int srate, int swidth)
		: waveform(0), wavesize(0), channels(chans), samprate(srate), sampwidth(swidth),
		sign(DatManip::has_nosign), end(DatManip::le), 
		looping(false), loopstart(0), loopend(0) { };
	PCMData(int size, int chans, int srate, int swidth)
		: waveform(0), wavesize(size), channels(chans), samprate(srate), sampwidth(swidth),
		sign(DatManip::has_nosign), end(DatManip::le), 
		looping(false), loopstart(0), loopend(0)
	{ 
		resize_wave(size);
	}
	PCMData(char* s, int size, int chans, int srate, int swidth)
		: waveform(0), wavesize(size), channels(chans), samprate(srate), sampwidth(swidth),
		sign(DatManip::has_nosign), end(DatManip::le), 
		looping(false), loopstart(0), loopend(0)
	{ 
		set_wave(s, size);
	}

	char* get_waveform()	{ return waveform; }
	int get_wavesize()		{ return wavesize; }
	int get_channels()		{ return channels; }
	int get_samprate()		{ return samprate; }
	int get_sampwidth()		{ return sampwidth; }
	int get_signed()		{ return sign; }
	int get_end()			{ return end; }
	bool get_looping()		{ return looping; }
	int get_loopstart()		{ return loopstart; }
	int get_loopend()		{ return loopend; }
	int get_byterate()
	{
		return samprate * channels * sampwidth / 8;
	}
	int get_blockalign()
	{
		return channels * sampwidth / 8;
	}
	int get_zerolevel();
	// destroy existing waveform and set to s
	void give_wave(char* s, int n)
	{
		delete[] waveform;
		waveform = s;
		wavesize = n;
	}
	// destroy existing waveform and copy in s
	void set_wave(char* s, int n)
	{
		resize_wave(n);
		memcpy(waveform, s, n);
	}
	// destroy existing waveform and change size
	void resize_wave(int n)
	{
		delete[] waveform;
		waveform = new char[n];
		wavesize = n;
	}
	// resize, preserving existing waveform
	void resize_and_copy_wave(int n)
	{
		char* newwaveform = new char[n];
		int sz = std::min(wavesize, n);
		std::memcpy(newwaveform, waveform, sz);
		delete[] waveform;
		waveform = newwaveform;
		wavesize = n;
	}

	void set_waveform(char* new_waveform)	{ waveform = new_waveform; }
	void set_wavesize(int new_wavesize)		{ wavesize = new_wavesize; }
	void set_channels(int new_channels)		{ channels = new_channels; }
	void set_samprate(int new_samprate)		{ samprate = new_samprate; }
	void set_sampwidth(int new_sampwidth)	{ sampwidth = new_sampwidth; }
	void set_signed(DatManip::Sign new_sign){ sign = new_sign; }
	void set_end(DatManip::End new_end)		{ end = new_end; }
	void set_looping(bool newlooping)		{ looping = newlooping; }
	void set_loopstart(int newloopstart)	{ loopstart = newloopstart; }
	void set_loopend(int newloopend)		{ loopend = newloopend; }
	// switch signedness of waveform
	void convert_signedness(DatManip::Sign s);
	// switch endianess of waveform
	void convert_endianess(DatManip::End e);
	// perform a linear fade between the given sample points
	void linear_fade(int fadestart, int fadeend, double level = 0);
	// given loop start and end positions in samples,
	// add the given number of loops to the waveform
	void add_loop(int loopstart, int loopend, int loops, 
		LoopStyle loopstyle = fadeloop, double fadetime = 0,
		double trailsilence = 0);
	// attempt to convert format to the specified parameters
	// return false if conversion cannot not be performed
	bool convert_sampwidth(int bitspersamp, DatManip::Sign s);
	// amplify waveform to maximum threshold
	void normalize();
private:
	// number of stages in adding a fade
	const static int fade_granularity = 1024;

	char* waveform;			// raw waveform data (channel interleaved)
	int wavesize;			// length of waveform data array
	int channels;			// number of channels
	int samprate;			// playback rate
	int sampwidth;			// bits per sample
	DatManip::Sign sign;	// sample signedness
	DatManip::End end;		// sample endianess
	bool looping;
	int loopstart;
	int loopend;
};

// write out PCMData as a RIFF WAVE file
void write_pcmdata_wave(PCMData& dat, const std::string& outfile,
	int ignorebytes = 0, int ignoreend = 0);


};


#pragma once