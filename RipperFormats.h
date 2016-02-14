/* Structs and helper functions for basic ripping */

#include "utils/MembufStream.h"
#include "utils/PCMData.h"
#include "utils/BitmapData.h"
#include "utils/datmanip.h"

#include <string>
#include <cstring>
#include <vector>

namespace RipperFormats {

		
// enum of data types
// not currently used
enum Format
{
	format_none,
	format_unknown
};
	
namespace RipConsts
{


	// default size of the stream read buffer (bytes)
	const static int default_bufsize = 64000000;
	// default number of loops
	const static int default_num_loops = 1;
	// default length of loop fadeout time (secs)
	const static double default_loop_fade = 5.0;
	// default length of trailing silence appended to a loop fadeout (secs)
	const static double default_loop_silence = 1.0;
	// default looping style
	const static RipUtil::PCMData::LoopStyle default_loopstyle = RipUtil::PCMData::fadeloop;
	// a crappy attempt to get rid of magic constants
	const static int not_set = -1;


};	// end of namespace RipConsts

// container for ripper settings
class RipperSettings
{
public:
	RipperSettings()
		: argc(0), argv(0),
		ripallraw(false),
		copycommon(false),
		encoding(0),
		ripgraphics(true),
		ripanimations(true),
		ripaudio(true),
		ripstrings(true), 
		ripdata(false),
		bufsize(RipConsts::default_bufsize),
		startentry(RipConsts::not_set), endentry(RipConsts::not_set),
		guesspalettes(true), palettenum(RipConsts::not_set),
		backgroundcolor(0xFF00FF),
		localpalettes(false),
		normalize(false),
		audsign(DatManip::has_nosign), audend(DatManip::le),
		channels(RipConsts::not_set), samprate(RipConsts::not_set), 
		sampwidth(RipConsts::not_set), ignorebytes(0), ignoreend(0),
		numloops(RipConsts::default_num_loops),
		loopstyle(RipConsts::default_loopstyle),
		loopfadelen(RipConsts::default_loop_fade),
		loopfadesil(RipConsts::default_loop_silence),
		decode_audio(false) { };

	// command line params for use by individual modules
	int argc;
	char** argv;

	// general
	std::string outpath;	// output file path/prefix
	bool ripallraw;			// output raw files with no decoding
	bool copycommon;		// copy common formats directly to output
	int encoding;			// XOR decryption byte
	bool ripgraphics;		// rip graphics?
	bool ripanimations;		// rip animations?
	bool ripaudio;			// rip audio?
	bool ripstrings;		// rip text strings?
	bool ripdata;			// rip other (nondecodable) data?
	int bufsize;			// size of input read buffer
	int startentry;			// ignore all graphics entries before this number
	int endentry;			// ignore all graphics entries after this number

	// graphics
	bool guesspalettes;		// if set, guess palettes; otherwise, use grayscale
	int palettenum;			// if set, color all graphics with this palette
	int backgroundcolor;	// value to be used as background/transparent fill (BGR)
	bool localpalettes;		// use local palette instead of global when possible

	// audio
	DatManip::Sign audsign;	// sample signedness
	DatManip::End audend;	// sample endianess
	int channels;			// number of channels
	int samprate;			// sample rate
	int sampwidth;			// bits per sample
	int ignorebytes;		// ignore this many initial sample bytes
	int ignoreend;			// ignore this many trailing sample bytes
	bool normalize;			// should output data be normalized?
	int numloops;			// # of loops for looped sounds
	RipUtil::PCMData::LoopStyle loopstyle; // looping style
	double loopfadelen;		// loop fade length
	double loopfadesil;		// trailing silence for fade loops
	bool decode_audio;		// decode audio where unnecessary, but possible?
};

// container for results of ripping
struct RipResults
{
	RipResults() 
		: graphics_ripped(0), animations_ripped(0),
		animation_frames_ripped(0), strings_ripped(0),
		palettes_ripped(0),	audio_ripped(0), data_ripped(0) { };

	int graphics_ripped;
	int animations_ripped;
	int animation_frames_ripped;
	int audio_ripped;
	int strings_ripped;
	int palettes_ripped;
	int data_ripped;
};

// container for file format info
struct FileFormatData
{
	FileFormatData()
		: format(format_unknown), encoding(0) { };

	Format format;			// data format
	int encoding;			// XOR decryption byte
};

// format a PCMData object according to a RipperSettings object
void format_PCMData(RipUtil::PCMData& dat, const RipperSettings& ripset);


};	// end namespace Ripper

#pragma once