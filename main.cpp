/* HEErip: utility to extract data from Humongous Entertainment archives
   by Supper (suppertails66@gmail.com)

   All the code in these files was written by me.
   If anything in here is somehow useful to you, do whatever you want with it. */

#include "launch.h"
#include "utils/datmanip.h"
#include "RipperFormats.h"
#include "RipModules.h"
#include <ctime>
#include <iostream>
#include <fstream>
#include <string>

using std::cout;
using std::cerr;
using namespace Ripper;
using namespace RipperFormats;
using namespace RipUtil;

int main(int argc, char** argv)
{
	// not enough parameters: print usage info and exit
	if (argc < 2)
	{
		print_base_usage_text();
		return 1;
	}

	try {

	// load modules
	// despite how this code is written, there is only one "mod,"
	// which handles all recognized Humongous files
	RipModules mods;

	// no mods loaded: should be impossible
	if (mods.num_mods() == 0)
	{
		cerr << "Program was built with no modules!" << '\n';
		return 5;
	}
	
	// create settings object and configure from parameters
	RipperSettings ripset;
	configure_parameters(argc, argv, ripset);
	ripset.argc = argc;
	ripset.argv = argv;

	// start timer
	int timer = std::clock();

	// get filename; strip prefix to get filename for display
	std::string filename = argv[1];
	std::string shortfname = get_short_filename(filename);
	std::string fprefix = strip_extension(filename);
	if (ripset.outpath != "")
		fprefix = ripset.outpath;

	cout << "Input file: " << filename << '\n';
	cout << "Output destination: " << fprefix << '\n';

	MembufStream stream(filename, MembufStream::rb, ripset.encoding, ripset.bufsize); 

/*	MembufStream stream("baseball_hakk", MembufStream::rb, ripset.encoding, ripset.bufsize);

	// temp debug code to re-encode decoded files
	char* fl = new char[stream.get_fsize()];
	stream.read(fl, stream.get_fsize());
	for (int i = 0; i < stream.get_fsize(); i++) {
		fl[i] = fl[i] ^ 0x69;
	}
	std::ofstream ofs("baseball.he1", std::ios_base::binary);
	ofs.write(fl, stream.get_fsize());
	delete fl;
	return 0;  */

	FileFormatData fmtdat;
	RipResults results;

	// try to rip
	for (int i = 0; i < mods.num_mods(); i++)
	{
		if (mods[i]->can_rip(stream, ripset, fmtdat))
		{
			results = mods[i]->rip(stream, fprefix, ripset, fmtdat);
			cout << "Rip results:" << '\n';
			cout << '\t' << "Graphics ripped: " << results.graphics_ripped << '\n'
				<< '\t' << "Animations ripped: " << results.animations_ripped << '\n'
				<< '\t' << "Animation frames ripped: " << results.animation_frames_ripped << '\n'
				<< '\t' << "Audio files ripped: " << results.audio_ripped << '\n'
				<< '\t' << "Strings ripped: " << results.strings_ripped << '\n'
				<< '\t' << "Raw files ripped: " << results.data_ripped << '\n'
				<< '\t' << "Total: " << results.graphics_ripped + results.animation_frames_ripped 
					+ results.audio_ripped + results.strings_ripped
					+ results.data_ripped << '\n';
		}
		else
		{
			cerr << "File is not not a recognized Humongous datafile";
		}

		stream.reset();
	}

	// end timer
	timer = clock() - timer;
	cout << "Time elapsed: " << (double)timer/CLOCKS_PER_SEC << " secs" << '\n';
	cout << '\n';

//	char c;
//	std::cin >> c;

	return 0;

	}
	catch (FileOpenException e)
	{
		cerr << "Error opening file " << e.fname << " for reading " << '\n';
		return 4;
	}
	catch (std::exception& e)
	{
		cerr << "Error: " << e.what() << '\n';
		return 3;
	}
	catch (...)
	{
		cerr << "Unhandled exception: aborting" << '\n';
		return 2;
	}
}