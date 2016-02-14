#include "PCMData.h"
#include "datmanip.h"
#include "DefaultException.h"
#include <fstream>
#include <cmath>
#include <cstring>

namespace RipUtil
{

int PCMData::get_zerolevel()
{
	if (sign == DatManip::has_sign)
		return 0;
	else
		return static_cast<int> (std::pow((double)2, sampwidth)/2);
}

void PCMData::convert_signedness(DatManip::Sign s)
{
	// signed to unsigned
	if (s == DatManip::has_nosign && sign == DatManip::has_sign)
	{
		int bytespersamp = sampwidth/8;
		int range = static_cast<int>(std::pow((double)2, sampwidth));
		for (int i = 0; i < wavesize/bytespersamp; i++)
		{
			int value = to_int(waveform + i * bytespersamp, bytespersamp, end, sign);
			value -= range/2;
			to_bytes(value, waveform + i * bytespersamp, bytespersamp, end);
		}
		sign = DatManip::has_nosign;
	}
	// unsigned to signed
	else if (s == DatManip::has_sign && sign == DatManip::has_nosign)
	{
		int bytespersamp = sampwidth/8;
		int range = static_cast<int>(std::pow((double)2, sampwidth));
		for (int i = 0; i < wavesize/bytespersamp; i++)
		{
			int value = to_int(waveform + i * bytespersamp, bytespersamp, end, sign);
			value += range/2;
			to_bytes(value, waveform + i * bytespersamp, bytespersamp, end);
		}
		sign = DatManip::has_sign;
	}
}

void PCMData::convert_endianess(DatManip::End e)
{
	if (end != e)
	{
		int bytespersamp = sampwidth/8;
		for (int i = 0; i < wavesize; i+= bytespersamp)
		{
			swap_end(waveform + i, bytespersamp);
		}
		end = e;
	}
}

void PCMData::linear_fade(int fadestart, int fadeend, double level)
{
	bool was_unsigned;
	// temporarily convert unsigned data to signed
	if (sign == DatManip::has_nosign)
	{
		convert_signedness(DatManip::has_sign);
		was_unsigned = true;
	}
	else
		was_unsigned = false;

	int bytespersamp = sampwidth/8;
	
	int fadesamps = fadeend - fadestart;
	// convert sample positions to byte positions
	fadestart *= bytespersamp;
	fadeend *= bytespersamp;

	double fadediff = ((double)1 - level)/fade_granularity;
	int fadestep = fadesamps/fade_granularity;

	int samplerange = static_cast<int> (std::pow((double)2, sampwidth));

	double fadeamt = 1.0;
	char* bytes = new char[bytespersamp];
	for (int i = 0; i < fadesamps; i++)
	{
		int sample = to_int(waveform + fadestart + i * bytespersamp, bytespersamp, 
			end, sign);
		sample = static_cast<int> (sample * fadeamt);
		to_bytes(sample, bytes, bytespersamp, end);
		std::memcpy(waveform + fadestart + i * bytespersamp, bytes, bytespersamp);
		if (!(i % fadestep))
			fadeamt -= fadediff;
	}
	delete[] bytes;

	if (was_unsigned)
		convert_signedness(DatManip::has_nosign);
}

void PCMData::add_loop(int loopstart, int loopend, int loops, 
	LoopStyle loopstyle, double fadetime, double trailsilence)
{
	// convert sample positions to byte positions
	int bytespersamp = sampwidth/8;
	loopstart *= bytespersamp;
	loopend *= bytespersamp;
	// calculate new wavesize and allocate memory
	int looplen = loopend - loopstart;
	int newlen;
	if (loopstyle == fadeloop)
	{
		// compensate for fadeout time
		newlen = static_cast<int> (loopend + looplen * loops
			+ (fadetime + trailsilence) * samprate * bytespersamp);
	}
	else if (loopstyle == tailloop)
	{
		//compensate for sample tail
		newlen = static_cast<int> (wavesize + looplen * loops
			+ trailsilence * samprate * bytespersamp);
	}
	char* newwave = new char[newlen];
	// copy original wave up to loop point
	std::memcpy(newwave, waveform, loopend);
	// copy as many loops as needed
	for (int i = 0; i < loops; i++)
	{
		std::memcpy(newwave + loopend + (i) * looplen, waveform + loopstart, looplen);
	}
	if (loopstyle == fadeloop)
	{
		int fadesamps = static_cast<int> (fadetime * samprate);
		int fadestart = loopend + looplen * loops;
		int silstart = fadestart + fadesamps * bytespersamp;
		int silend = static_cast<int> (silstart + trailsilence * samprate * bytespersamp);
		// copy portion of loop needed
		for (int i = 0; i < fadesamps; i++)
		{
			std::memcpy(newwave + fadestart + i * bytespersamp,
				waveform + loopstart + i * bytespersamp % looplen, bytespersamp);
		}
		// clear trailing silence
		std::memset(newwave + silstart, get_zerolevel(), newlen - silstart);
		delete[] waveform;
		waveform = newwave;
		wavesize = newlen;
		linear_fade(fadestart/bytespersamp, fadestart/bytespersamp + fadesamps);
	}
	else if (loopstyle == tailloop)
	{
		// copy "tail" from past loop point
		std::memcpy(newwave + loopend + looplen * loops, waveform + loopend, wavesize - loopend);
		// clear trailing silence
		int silstart = loopend + looplen * loops + wavesize - loopend;
		std::memset(newwave + silstart, 0, newlen - silstart);
		delete[] waveform;
		waveform = newwave;
		wavesize = newlen;
	}
}

bool PCMData::convert_sampwidth(int bitspersamp, DatManip::Sign s)
{
	if (bitspersamp % 8)
		return false;

	if (bitspersamp != sampwidth)
	{
		int bytespersamp = bitspersamp/8;
		int range = static_cast<int> (std::pow((double)2, sampwidth));
		char* new_wave = new char[wavesize/(sampwidth/8) * bytespersamp];
		char* in_bytes = new char[sampwidth/8];
		char* out_bytes = new char[bytespersamp];
		for (int i = 0; i < wavesize; i += sampwidth/8)
		{
			std::memcpy(in_bytes, waveform + i, sampwidth/8);
			if (end == DatManip::le)
				swap_end(in_bytes, sampwidth/8);
			int val = to_int(in_bytes, sampwidth/8);
			// unsigned to signed
			if (sign != DatManip::has_sign && s == DatManip::has_sign)
			{
				val -= range/2;
			}
			// signed to unsigned
			else if (sign != DatManip::has_nosign && s == DatManip::has_nosign)
			{
				// temporarily convert to signed
				if (val >= range/2)
					val -= range;
			}
			// lower sampwidth
			if (bitspersamp < sampwidth)
			{
				// reduce to specified bit width
				val /= static_cast<int> (std::pow((double)2, sampwidth - bitspersamp));
			}
			// higher sampwidth
			else
			{
				// increase to specified bit width
				val *= static_cast<int> (std::pow((double)2, bitspersamp - sampwidth));
			}
			// signed to unsigned
			if (sign != DatManip::has_nosign && s == DatManip::has_nosign)
			{
				// convert signed data to unsigned
				val += bitspersamp/8 * static_cast<int> (std::pow((double)2, bitspersamp)/2);
			}
			to_bytes(val, out_bytes, bytespersamp);
			if (end == DatManip::le)
				swap_end(out_bytes, bytespersamp);
			// write to waveform
			std::memcpy(new_wave + i/(sampwidth/8) * bytespersamp, out_bytes, bytespersamp);
		}
		loopstart *= static_cast<int>((double)bitspersamp/sampwidth);
		loopend *= static_cast<int>((double)bitspersamp/sampwidth);
		delete[] in_bytes;
		delete[] out_bytes;
		delete[] waveform;
		sign = s;
		wavesize = wavesize/(sampwidth/8) * bytespersamp;
		sampwidth = bitspersamp;
		waveform = new_wave;
	}
	return true;
}

void PCMData::normalize()
{
	// we need enough resolution to avoid quality loss
	if (sampwidth == 8)
		convert_sampwidth(16, DatManip::has_sign);
	// find difference between max amplitude and highest sample
	int range = static_cast<int> (std::pow((double)2, sampwidth));
	int maxpeak = static_cast<int> ((std::pow((double)2, sampwidth))/2 - 1);
	int minpeak = -maxpeak - 1;
	double scalefactor = 1;
	int bytespersamp = sampwidth/8;
	char* bytes = new char[bytespersamp];
	std::memcpy(bytes, waveform, bytespersamp);
	if (end == DatManip::le)
		swap_end(bytes, bytespersamp);
	int highest, lowest;
	int first = to_int(bytes, bytespersamp);
	// convert to signed for comparison
	if (sign == DatManip::has_nosign)
		first -= range/2;
	if (first > maxpeak)
		first -= (maxpeak + 1) * 2;
	highest = first;
	lowest = first;
	for (int i = 0; i < wavesize; i += bytespersamp)
	{
		std::memcpy(bytes, waveform + i, bytespersamp);
		if (end == DatManip::le)
			swap_end(bytes, bytespersamp);
		int cmp = to_int(bytes, bytespersamp);
		if (sign == DatManip::has_nosign)
			cmp -= range/2;
		if (cmp > maxpeak)
			cmp -= (maxpeak + 1) * 2;
		if (cmp > highest)
			highest = cmp;
		else if (cmp < lowest)
		{
			lowest = cmp;
		}
	}
	// amplify to maximum possible level
	if (highest < maxpeak && lowest > minpeak)
	{
		int top = std::abs(highest);
		int bottom = std::abs(lowest);
		if (top < bottom)
			scalefactor = (double)-minpeak/bottom;
		else
			scalefactor = (double)maxpeak/top;
		for (int i = 0; i < wavesize; i += bytespersamp)
		{
			std::memcpy(bytes, waveform + i, bytespersamp);
			if (end == DatManip::le)
				swap_end(bytes, bytespersamp);
			int val = to_int(bytes, bytespersamp);
			if (sign == DatManip::has_nosign)
				val -= range/2;
			if (val > maxpeak)
				val -= (maxpeak + 1) * 2;
			val = static_cast<int> ((float)val * scalefactor);
			if (sign == DatManip::has_nosign)
				val += range/2;
			to_bytes(val, bytes, bytespersamp);
			if (end == DatManip::le)
				swap_end(bytes, bytespersamp);
			std::memcpy(waveform + i, bytes, bytespersamp);
		}
	}
	delete[] bytes;
}

void write_pcmdata_wave(PCMData& dat, const std::string& outfile,
	int ignorebytes, int ignoreend)
{
	// disallow negative ignore values
	ignorebytes = std::max(0, ignorebytes);
	ignoreend = std::max(0, ignoreend);

	int size = std::max(0, dat.get_wavesize() - ignorebytes - ignoreend);

	std::ofstream ofs(outfile.c_str(), std::ios_base::binary);
	if (!ofs) throw(DefaultException("error writing to file"));
	// RIFF header
	// chunk id: "RIFF"
	char chunksize[4];
	// format id: "WAVE"
	// subchunk 1
	// id: "fmt "
	char subchunk1size[4] = { 16, 0, 0, 0 };
	char audioformat[2] = { 1, 0 };
	char numchannels[2];
	char samplerate[4];
	char byterate[4];
	char blockalign[2];
	char bitspersample[2];
	// subchunk 2
	// id: "data"
	char subchunk2size[4];

	// set file parameters
	to_bytes(36 + size, chunksize, 4);
	to_bytes(dat.get_channels(), numchannels, 2);
	to_bytes(dat.get_samprate(), samplerate, 4);
	to_bytes(dat.get_byterate(), byterate, 4);
	to_bytes(dat.get_blockalign(), blockalign, 2);
	to_bytes(dat.get_sampwidth(), bitspersample, 2);
	to_bytes(size, subchunk2size, 4);
	

	// set endianess
	swap_end(chunksize, 4);
	swap_end(numchannels, 2);
	swap_end(samplerate, 4);
	swap_end(byterate, 4);
	swap_end(blockalign, 2);
	swap_end(bitspersample, 2);
	swap_end(subchunk2size, 4);

	// write data
	ofs.write(WaveWriterConsts::riff_chunk_id, 4);
	ofs.write(chunksize, 4);
	ofs.write(WaveWriterConsts::riff_format_id, 4);
	ofs.write(WaveWriterConsts::riff_schunk1_id, 4);
	ofs.write(subchunk1size, 4);
	ofs.write(audioformat, 2);
	ofs.write(numchannels, 2);
	ofs.write(samplerate, 4);
	ofs.write(byterate, 4);
	ofs.write(blockalign, 2);
	ofs.write(bitspersample, 2);
	ofs.write(WaveWriterConsts::riff_schunk2_id, 4);
	ofs.write(subchunk2size, 4);

	if (dat.get_end() == DatManip::be)
	{
		dat.convert_endianess(DatManip::le);
		ofs.write(dat.get_waveform() + ignorebytes, 
			size);
		dat.convert_endianess(DatManip::be);
	}
	else
		ofs.write(dat.get_waveform() + ignorebytes,
			size);
}


};	// end namespace RipUtil
