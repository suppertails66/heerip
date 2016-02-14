#include "launch.h"
#include "utils/datmanip.h"
#include "utils/PCMData.h"
#include "RipperFormats.h"

#include <iostream>

using std::cout;
using namespace RipUtil;

// print all non-module-dependent usage info
void print_base_usage_text()
{
	cout << "HEErip: utility to extract data from Humongous Entertainment archives" << '\n';
	cout << "Usage: heerip <infile> [options]" << '\n';
	cout << "Parameters: " << '\n';
	cout << '\t' << "-alttrans <val>" << '\t' << '\t' << "Force transparency color index (0-255)" << '\n'
		<< '\t' << "-bufsize <val>"<< '\t' << '\t'
		<< "Max read buffer size in bytes (def: " 
			<< (double)RipperFormats::RipConsts::default_bufsize/1000000 << " mb)" << '\n'
		<< '\t' << "-decode <val>" << '\t' << '\t' << "Force decoding byte (def: 0)" << '\n'
		<< '\t' << "-end <val>" << '\t' << '\t' << "Set ending room (def: read to end)" << '\n'
		<< '\t' << "-ignoreend <val>" << '\t' << "Audio: # of trailing sample bytes to ignore" << '\n'
		<< '\t' << "-ignorestart <val>" << '\t' << "Audio: # of initial sample bytes to ignore" << '\n'
		<< '\t' << "-output <val>" << '\t' << '\t' << "Set output prefix (def: filename w/o extension)" << '\n'
		<< '\t' << "-palettenum" << '\t' << '\t' << "Force use of this room number's palette" << '\n'
		<< '\t' << "-start <val>" << '\t' << '\t' << "Set starting room (def: 0)" << '\n';
	cout << '\n';
	cout << '\t' << "--noakos" << '\t' << '\t' << "Disable AKOS ripping" << '\n'
		<< '\t' << "--noawiz" << '\t' << '\t' << "Disable AWIZ ripping" << '\n'
		<< '\t' << "--nochar" << '\t' << '\t' << "Disable CHAR ripping" << '\n'
		<< '\t' << "--noextdmu" << '\t' << '\t' << "Disable external DMU file ripping" << '\n'
		<< '\t' << "--nometadata" << '\t' << '\t' << "Disable metadata ripping" << '\n'
		<< '\t' << "--noobim" << '\t' << '\t' << "Disable OBIM ripping" << '\n'
		<< '\t' << "--normim" << '\t' << '\t' << "Disable RMIM ripping" << '\n'
		<< '\t' << "--nosound" << '\t' << '\t' << "Disable DIGI/TALK/WSOU/DMU ripping" << '\n'
		<< '\t' << "--notlke" << '\t' << '\t' << "Disable TLKE ripping" << '\n'
		<< '\t' << "--usesequence" << '\t' << '\t' << "Enable animation sequence ripping" << '\n'
		<< '\n';
	cout << '\t' << "--akosonly" << '\t' << '\t' << "Enable only AKOS ripping" << '\n'
		<< '\t' << "--awizonly" << '\t' << '\t' << "Enable only AWIZ ripping" << '\n'
		<< '\t' << "--charonly" << '\t' << '\t' << "Enable only CHAR ripping" << '\n'
		<< '\t' << "--extdmuonly" << '\t' << '\t' << "Enable only external DMU ripping" << '\n'
		<< '\t' << "--metadataonly" << '\t' << '\t' << "Enable only metadata ripping" << '\n'
		<< '\t' << "--obimonly" << '\t' << '\t' << "Enable only OBIM ripping" << '\n'
		<< '\t' << "--rmimonly" << '\t' << '\t' << "Enable only RMIM ripping" << '\n'
		<< '\t' << "--soundonly" << '\t' << '\t' << "Enable only DIGI/TALK/WSOU/DMU ripping" << '\n'
		<< '\t' << "--sequenceonly" << '\t' << '\t' << "Enable only animation sequence ripping" << '\n'
		<< '\t' << "--tlkeonly" << '\t' << '\t' << "Enable only TLKE ripping" << '\n'
		<< '\n';
	cout << '\t' << "--decodeaudio" << '\t' << '\t' << "Decode audio instead of copying" << '\n'
		<< '\t' << "--decodeonly" << '\t' << '\t' << "Decode XOR encoded file: no other output" << '\n'
		<< '\t' << "--disablelog" << '\t' << '\t' << "Disable log file writing" << '\n'
		<< '\t' << "--force_lined_rle" << '\t' << "Force lined RLE hack" << '\n'
		<< '\t' << "--force_unlined_rle" << '\t' << "Force unlined RLE hack" << '\n'
		<< '\t' << "--force_akos2c_rle" << '\t' << "Force AKOS RLE hack" << '\n'
		<< '\t' << "--force_akos2c_bitmap" << '\t' << "Force AKOS bitmap hack" << '\n'
		<< '\t' << "--localpalettes" << '\t' << '\t' << "Use local instead of global palettes" << '\n'
		<< '\t' << "--norip" << '\t' << '\t' << '\t' << "Read-only mode: no output" << '\n'
		<< '\t' << "--normalize" << '\t' << '\t' << "Normalize audio (auto-decodeaudio)" << '\n';
}

// configure RipperSettings from command line params
void configure_parameters(int argc, char** argv, RipperFormats::RipperSettings& ripset)
{
	// first pass: single-flag params
	for (int i = 2; i < argc; i++)
	{
		if (quickstrcmp(argv[i], "--ripallraw"))
			ripset.ripallraw = true;
		else if (quickstrcmp(argv[i], "--copycommon"))
			ripset.copycommon = false;
		else if (quickstrcmp(argv[i], "--nographics"))
			ripset.ripgraphics = false;
		else if (quickstrcmp(argv[i], "--noanimations"))
			ripset.ripanimations = false;
		else if (quickstrcmp(argv[i], "--noaudio"))
			ripset.ripaudio = false;
		else if (quickstrcmp(argv[i], "--nostrings"))
			ripset.ripstrings = false;
		else if (quickstrcmp(argv[i], "--ripdata"))
			ripset.ripdata = true;
		else if (quickstrcmp(argv[i], "--palettesoff"))
			ripset.guesspalettes = false;
		else if (quickstrcmp(argv[i], "--localpalettes"))
			ripset.localpalettes = true;
		else if (quickstrcmp(argv[i], "--normalize"))
			ripset.normalize = true;
		else if (quickstrcmp(argv[i], "--decode_audio"))
			ripset.decode_audio = true;
	}

	// second pass: two-flag params
	for (int i = 2; i < argc - 1; i++)
	{		
		if (quickstrcmp(argv[i], "-output")
			|| quickstrcmp(argv[i], "-o"))
			ripset.outpath = argv[i + 1];
		else if (quickstrcmp(argv[i], "-encoding")
			|| quickstrcmp(argv[i], "-e"))
			ripset.encoding = from_string<int>(argv[i + 1]);
		else if (quickstrcmp(argv[i], "-bufsize")
			|| quickstrcmp(argv[i], "-b"))
			ripset.bufsize = from_string<int>(argv[i + 1]);
		else if (quickstrcmp(argv[i], "-start")
			|| quickstrcmp(argv[i], "-st"))
			ripset.startentry = from_string<int>(argv[i + 1]);
		else if (quickstrcmp(argv[i], "-end")
			|| quickstrcmp(argv[i], "-en"))
			ripset.endentry = from_string<int>(argv[i + 1]);
		else if (quickstrcmp(argv[i], "-palettenum")
			|| quickstrcmp(argv[i], "-pn"))
			ripset.palettenum = from_string<int>(argv[i + 1]);
		else if (quickstrcmp(argv[i], "-backgroundcolor")
			|| quickstrcmp(argv[i], "-bg"))
		{
			/* not implemented */
		}
		else if (quickstrcmp(argv[i], "-audsign")
			|| quickstrcmp(argv[i], "-as"))
		{
			if (quickstrcmp(argv[i + 1], "signed"))
				ripset.audsign = DatManip::has_sign;
			else if (quickstrcmp(argv[i + 1], "unsigned"))
				ripset.audsign = DatManip::has_nosign;
		}
		else if (quickstrcmp(argv[i], "-audend")
			|| quickstrcmp(argv[i], "-ae"))
		{
			if (quickstrcmp(argv[i + 1], "big"))
				ripset.audend = DatManip::be;
			else if (quickstrcmp(argv[i + 1], "little"))
				ripset.audend = DatManip::le;
		}
		else if (quickstrcmp(argv[i], "-channels")
			|| quickstrcmp(argv[i], "-ch"))
			ripset.channels = from_string<int>(argv[i + 1]);
		else if (quickstrcmp(argv[i], "-samprate")
			|| quickstrcmp(argv[i], "-sr"))
			ripset.samprate = from_string<int>(argv[i + 1]);
		else if (quickstrcmp(argv[i], "-sampwidth")
			|| quickstrcmp(argv[i], "-sw"))
			ripset.sampwidth = from_string<int>(argv[i + 1]);
		else if (quickstrcmp(argv[i], "-ignorestart")
			|| quickstrcmp(argv[i], "-is"))
			ripset.ignorebytes = from_string<int>(argv[i + 1]);
		else if (quickstrcmp(argv[i], "-ignoreend")
			|| quickstrcmp(argv[i], "-ie"))
			ripset.ignoreend = from_string<int>(argv[i + 1]);
		else if (quickstrcmp(argv[i], "-numloops")
			|| quickstrcmp(argv[i], "-nl"))
			ripset.numloops = from_string<int>(argv[i + 1]);
		else if (quickstrcmp(argv[i], "-loopstyle")
			|| quickstrcmp(argv[i], "-ls"))
		{
			if (quickstrcmp(argv[i + 1], "fade"))
				ripset.loopstyle = PCMData::fadeloop;
			else if (quickstrcmp(argv[i + 1], "tail"))
				ripset.loopstyle = PCMData::tailloop;
		}
		else if (quickstrcmp(argv[i], "-loopfadelen")
			|| quickstrcmp(argv[i], "-lf"))
			ripset.loopfadelen = from_string<double>(argv[i + 1]);
		else if (quickstrcmp(argv[i], "-loopfadesil")
			|| quickstrcmp(argv[i], "-ls"))
			ripset.loopfadesil = from_string<double>(argv[i + 1]);
	}
}