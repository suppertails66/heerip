#include "humongous.h"
#include "humongous_read.h"
#include "humongous_rip.h"
#include "humongous_structs.h"

#include "../utils/MembufStream.h"
#include "../utils/BitmapData.h"
#include "../utils/PCMData.h"
#include "../utils/datmanip.h"
#include "../utils/ErrorLog.h"
#include "../utils/logger.h"
#include "../RipperFormats.h"
#include "common.h"
#include <fstream>
#include <string>
#include <cstring>
#include <iostream>
#include <ctime>

using namespace RipUtil;
using namespace RipperFormats;
using namespace ErrLog;
using namespace Logger;

namespace Humongous
{


bool HERip::can_rip(RipUtil::MembufStream& stream, const RipperFormats::RipperSettings& ripset,
		RipperFormats::FileFormatData& fmtdat)
{
	if (stream.get_fsize() < 16)
		return false;

	check_params(ripset);

	int initial_encoding = stream.get_decoding_byte();
	bool detected = false;
	FormatSubtype fmtguess = fmt_unknown;

	if (ripset.encoding != 0)
		stream.set_decoding_byte(ripset.encoding);
	else
		stream.set_decoding_byte(0x69);
	while (detected == false)
	{
		detected = true;

		stream.seekg(0);
		SputmChunkHead chunkhd1;
		read_sputm_chunkhead(stream, chunkhd1);

		// fail if first chunk ID is invalid
		if (chunkhd1.type == lecf)
			fmtguess = lecf_type2;
		else if (chunkhd1.type == tlkb)
			fmtguess = tlkb_type1;
		else if (chunkhd1.type == song)
			fmtguess = song_type1;
		else if (chunkhd1.type == mraw)
			fmtguess = song_dmu;
		else
			detected = false;

		
		// fail if first chunksize != filesize
		if (detected != false && chunkhd1.size != stream.get_fsize())
			detected = false;

		// check for subformat; fail if next chunk type does not match expected
		if (detected != false)
		{
			SputmChunkHead chunkhd2;
			read_sputm_chunkhead(stream, chunkhd2);
			if (chunkhd1.type == lecf)
			{
				if (chunkhd2.type == lflf)
					fmtguess = lecf_type2;
				else if (chunkhd2.type == loff)
					fmtguess = lecf_type1;
				else
					detected = false;
			}
			else if (chunkhd1.type == tlkb)
			{
				if (chunkhd2.type == talk || chunkhd2.type == wsou)
					fmtguess = tlkb_type1;
				else
					detected = false;
			}
			else if (chunkhd1.type == song && chunkhd2.type == sghd)
			{
				// special case: file is valid, but has no tracks (pajama3 demo)
				if (chunkhd2.nextaddr() >= stream.get_fsize())
				{
					/* error message here? */
					return false;
				}

				// check additional chunks to determine format subtype
				stream.seekg(chunkhd2.address + chunkhd2.size);
				SputmChunkHead chunkhd3;
				read_sputm_chunkhead(stream, chunkhd3);

				// next chunk unheadered: type 1 or type 2
				if (chunkhd3.type == chunk_unknown)
					fmtguess = song_type1;
				// next chunk is SGEN: type 3 or type 4
				else if (chunkhd3.type == sgen)
					fmtguess = song_type3;

				// type2 and type4 detection would go here, but the ripping
				// functions as written don't actually need to know
			}
			else if (chunkhd1.type == mraw && chunkhd2.type == hshd)
			{
				fmtguess = song_dmu;
			}
			else
				detected = false;
		}

		// pass was successful: we can rip
		if (detected == true)
		{
			formatsubtype = fmtguess;
			encoding = stream.get_decoding_byte();
			stream.set_decoding_byte(initial_encoding);
			return true;
		}
		// pass was unsuccessful
		else
		{
			// if we haven't tried some encodings, use them
			if (ripset.encoding != 0 && stream.get_decoding_byte() == ripset.encoding)
			{
				// first encoding was user-specified
				if (ripset.encoding != 0)
					stream.set_decoding_byte(0x69);
				// first encoding was user-specified 0x69
				else
					stream.set_decoding_byte(0);
			}
			// if 0x69 didn't work, try 0
			else if (stream.get_decoding_byte() == 0x69)
			{
				stream.set_decoding_byte(0);
			}
			// give up
			else if (stream.get_decoding_byte() == 0)
			{
				stream.set_decoding_byte(initial_encoding);
				return false;
			}
		}
	}
	// should never reach this
	stream.set_decoding_byte(initial_encoding);
	return false;
}

RipperFormats::RipResults HERip::rip(RipUtil::MembufStream& stream, const std::string& fprefix,
		const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat)
{
	disablelog ? logger.disable() :
		logger.open(fprefix + "-log.txt");

	logger.qprint("initiating rip of " + stream.get_fname()
		+ ", size " + to_string(stream.get_fsize()) + " bytes"
		+ ", encoding " + to_string(encoding));
	logger.qprint("output prefix: " + fprefix);

	RipResults results;

	if (encoding != -1)
		stream.set_decoding_byte(encoding);

	stream.seekg(0);

	if (decode_only)
	{
		logger.qprint("decoding only");

		std::ofstream ofs((fprefix + "-decoded").c_str(), std::ios_base::binary);
		char* outbytes = new char[stream.get_fsize()];
		stream.read(outbytes, stream.get_fsize());
		ofs.write(outbytes, stream.get_fsize());
		delete[] outbytes;
		return results;
	}

	// HE1/(A)/(B) data file
	if (formatsubtype == lecf_type1 || formatsubtype == lecf_type2)
	{
		rip_lecf(stream, fprefix, ripset, fmtdat, results);
	}
	// HE2 dialog file
	else if (formatsubtype == tlkb_type1)
	{
		rip_tlkb(stream, fprefix, ripset, fmtdat, results);
	}
	// HE4, type 1 or 2
	else if (formatsubtype == song_type1 || formatsubtype == song_type2)
	{
		rip_song_type12(stream, fprefix, ripset, fmtdat, results);
	}
	// HE4, type 3 or 4
	else if (formatsubtype == song_type3 || formatsubtype == song_type4)
	{
		rip_song_type34(stream, fprefix, ripset, fmtdat, results);
	}
	// DMU music file
	else if (formatsubtype == song_dmu)
	{
		rip_song_dmu(stream, fprefix, ripset, fmtdat, results);
	}

	if (logger.get_errflag())
	{
		std::cout << "One or more errors occured; please check the log file" << '\n';
		logger.qprint("Reported number of errors: " + to_string(logger.get_errcount()));
	}
	if (logger.get_warnflag())
	{
		std::cout << "One or more warnings were issued; "
		"please check the log file" << '\n';
		logger.qprint("Reported number of warnings: "
			+ to_string(logger.get_warncount()));
	}

	return results;
}

void HERip::check_params(const RipperFormats::RipperSettings& ripset)
{
	if (!ripset.ripgraphics)
	{
		rmimrip = false;
		obimrip = false;
		awizrip = false;
		charrip = false;
	}

	if (!ripset.ripanimations)
	{
		akosrip = false;
	}

	if (!ripset.ripaudio)
	{
		digirip = false;
		talkrip = false;
		wsourip = false;
	}

	if (ripset.startentry != RipConsts::not_set)
	{
		roomstart = ripset.startentry;
	}

	if (ripset.endentry != RipConsts::not_set)
	{
		roomend = ripset.endentry;
	}

	for (int i = 2; i < ripset.argc; i++)
	{
		if (quickstrcmp(ripset.argv[i], "--decodeonly"))
			decode_only = true;
		else if (quickstrcmp(ripset.argv[i], "--normim"))
			rmimrip = false;
		else if (quickstrcmp(ripset.argv[i], "--noobim"))
			obimrip = false;
		else if (quickstrcmp(ripset.argv[i], "--noakos"))
			akosrip = false;
		else if (quickstrcmp(ripset.argv[i], "--usesequence"))
			sequencerip = true;
		else if (quickstrcmp(ripset.argv[i], "--noawiz"))
			awizrip = false;
		else if (quickstrcmp(ripset.argv[i], "--nochar"))
			charrip = false;
		else if (quickstrcmp(ripset.argv[i], "--nosound"))
		{
			digirip = false;
			talkrip = false;
			wsourip = false;
			extdmurip = false;
		}
		else if (quickstrcmp(ripset.argv[i], "--notlke"))
			tlkerip = false;
		else if (quickstrcmp(ripset.argv[i], "--noextdmu"))
			extdmurip = false;
		else if (quickstrcmp(ripset.argv[i], "--usescripts"))
			sequencerip = true;
		else if (quickstrcmp(ripset.argv[i], "--nometadata"))
			metadatarip = false;
		else if (quickstrcmp(ripset.argv[i], "--rmimonly"))
		{
			disable_all_ripping();
			rmimrip = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--obimonly"))
		{
			disable_all_ripping();
			obimrip = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--akosonly"))
		{
			disable_all_ripping();
			akosrip = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--sequenceonly"))
		{
			disable_all_ripping();
			sequencerip = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--awizonly"))
		{
			disable_all_ripping();
			awizrip = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--charonly"))
		{
			disable_all_ripping();
			charrip = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--soundonly"))
		{
			rmimrip = false;
			obimrip = false;
			akosrip = false;
			awizrip = false;
			charrip = false;
			tlkerip = false;
			metadatarip = false;
		}
		else if (quickstrcmp(ripset.argv[i], "--extdmuonly"))
		{
			disable_all_ripping();
			extdmurip = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--tlkeonly"))
		{
			disable_all_ripping();
			tlkerip = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--scriptsonly"))
		{
			disable_all_ripping();
			scriptrip = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--metadataonly"))
		{
			disable_all_ripping();
			metadatarip = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--norip"))
		{
			disable_all_ripping();
		}
		else if (quickstrcmp(ripset.argv[i], "--catscripts"))
		{
			catscripts = true;
			scriptrip = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--disablelog"))
		{
			disablelog = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--force_lined_rle"))
		{
			rle_encoding_method_hack = rle_hack_always_use_lined;
			rle_encoding_method_hack_was_user_overriden = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--force_unlined_rle"))
		{
			rle_encoding_method_hack = rle_hack_always_use_unlined;
			rle_encoding_method_hack_was_user_overriden = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--force_akos2c_rle"))
		{
			akos_2color_decoding_hack = akos_2color_hack_always_use_rle;
			akos_2color_decoding_hack_was_user_overriden = true;
		}
		else if (quickstrcmp(ripset.argv[i], "--force_akos2c_bitmap"))
		{
			akos_2color_decoding_hack = akos_2color_hack_always_use_bitmap;
			akos_2color_decoding_hack_was_user_overriden = true;
		}
		if (i < ripset.argc - 1)	// parameterized settings
		{
			if (quickstrcmp(ripset.argv[i], "-alttrans"))
			{
				alttrans = true;
				transcol = from_string<int>(std::string(ripset.argv[i + 1]));
			}
		}
	}
}

void HERip::disable_all_ripping()
{
	rmimrip = false;
	obimrip = false;
	akosrip = false;
	sequencerip = false;
	awizrip = false;
	charrip = false;
	digirip = false;
	talkrip = false;
	wsourip = false;
	extdmurip = false;
	tlkerip = false;
	metadatarip = false;
	scriptrip = false;
}

void HERip::rip_lecf(RipUtil::MembufStream& stream, const std::string& fprefix,
	const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat,
	RipperFormats::RipResults& results)
{
	// clear any files we need to write to
	if (metadatarip)
	{
		std::ofstream((fprefix + "-metadata.txt").c_str(), std::ios_base::trunc);
	}
	if (catscripts && scriptrip) {
		std::ofstream((fprefix + "-scripts").c_str(), std::ios_base::trunc);
	}

	SputmChunkHead lecf_hd;
	SputmChunk loffc;
	read_sputm_chunkhead(stream, lecf_hd);
	read_chunk_if_exists(stream, loffc, loff);

	if (ripset.palettenum != RipperFormats::RipConsts::not_set)
	{
		int lecfstart = stream.tellg();
		scan_palettes(stream);
		stream.clear();
		stream.seekg(lecfstart);
	}

	int rmnum = 0;
	while (stream.tellg() < lecf_hd.nextaddr())
	{
		if (roomend != not_set && rmnum > roomend)
		{
			logger.print("ending read at room " + to_string(rmnum));
			break;
		}
		else if (roomstart != not_set && rmnum < roomstart)
		{
			logger.print("skipping room " + to_string(rmnum));
			SputmChunkHead skiphd;
			read_sputm_chunkhead(stream, skiphd);
			stream.seekg(skiphd.nextaddr());
			++rmnum;
			continue;
		}

		int starttime = std::clock();

		std::string rmstr = "-room-" + to_string(rmnum);

		logger.print("reading room " + to_string(rmnum) + "...");

		LFLFChunk lflfc;
		read_lflf(stream, lflfc);

		logger.print("...finished read");
			
		starttime = std::clock() - starttime;
		logger.qprint("\tread time: " + to_string((double)starttime/CLOCKS_PER_SEC)
			+ " s");
		logger.qprint("\tread stats:");
		if (lflfc.rmim_chunk.images.size())
			logger.qprint("\t\tRMIM: " + to_string(lflfc.rmim_chunk.images.size()));
		if (lflfc.obim_chunks.size())
			logger.qprint("\t\tOBIM: " + to_string(lflfc.obim_chunks.size()));
		if (lflfc.akos_chunks.size())
			logger.qprint("\t\tAKOS: " + to_string(lflfc.akos_chunks.size()));
		if (lflfc.awiz_chunks.size())
			logger.qprint("\t\tAWIZ: " + to_string(lflfc.awiz_chunks.size()));
		if (lflfc.mult_chunks.size())
			logger.qprint("\t\tMULT: " + to_string(lflfc.mult_chunks.size()));
		if (lflfc.char_chunks.size())
			logger.qprint("\t\tCHAR: " + to_string(lflfc.char_chunks.size()));
		if (lflfc.digi_chunks.size())
			logger.qprint("\t\tDIGI: " + to_string(lflfc.digi_chunks.size()));
		if (lflfc.talk_chunks.size())
			logger.qprint("\t\tTALK: " + to_string(lflfc.talk_chunks.size()));
		if (lflfc.wsou_chunks.size())
			logger.qprint("\t\tWSOU: " + to_string(lflfc.wsou_chunks.size()));

		if (!stream.eof())
		{
			stream.seekg(lflfc.nextaddr());
		}
		else if (stream.eof() && lflfc.nextaddr() < lecf_hd.nextaddr())
		{
			logger.error("stream unexpectedly reached end of file -- "
				"probably hit a misaligned chunk, recovering to next room");
			stream.clear();
			stream.seekg(lflfc.nextaddr());
		}

		// set alternate transparency color, if requested
		if (!alttrans)
			transcol = lflfc.trns_chunk.trns_val;

		if (rmimrip && lflfc.rmim_chunk.type == rmim)
		{
			logger.print("\tripping RMIM");
			rip_rmim(lflfc, ripset, fprefix + rmstr, results, transcol);
		}

		if (obimrip && lflfc.obim_chunks.size())
		{
			logger.print("\tripping OBIM");
			rip_obim(lflfc, ripset, fprefix + rmstr, results, transcol);
		}

		if ((akosrip || sequencerip) && lflfc.akos_chunks.size())
		{
			if (akosrip && sequencerip)
			{
				logger.print("\tripping AKOS and sequences");
			}
			else if (akosrip)
			{
				logger.print("\tripping AKOS");
			}
			else if (sequencerip)
			{
				logger.print("\tripping sequences");
			}
			rip_akos(lflfc, ripset, fprefix + rmstr, results, transcol);
		}

		if (awizrip && lflfc.awiz_chunks.size())
		{
			logger.print("\tripping AWIZ");
			rip_awiz(lflfc, ripset, fprefix + rmstr, results, transcol);
		}

		if (charrip && lflfc.char_chunks.size())
		{
			logger.print("\tripping CHAR");
			rip_char(lflfc, ripset, fprefix + rmstr, results, transcol);
		}

		if (digirip && lflfc.digi_chunks.size())
		{
			logger.print("\tripping DIGI");
			rip_sound(lflfc.digi_chunks, ripset, fprefix + rmstr + "-digi-",
				results);
		}

		if (talkrip && lflfc.talk_chunks.size())
		{
			logger.print("\tripping TALK");
			rip_sound(lflfc.talk_chunks, ripset, fprefix + rmstr + "-talk-",
				results);
		}

		if (wsourip && lflfc.wsou_chunks.size())
		{
			logger.print("\tripping WSOU");
			rip_wsou(lflfc, ripset, fprefix + rmstr, results,
				// override decoding settings if normaliziation requested
				ripset.normalize ? true : ripset.decode_audio);
		}

		if (extdmurip && lflfc.fmus_chunks.size())
		{
			rip_extdmu(lflfc, fprefix, ripset, fmtdat, results);
		}

		if (tlkerip && lflfc.tlke_chunks.size())
		{
			logger.print("\tripping TLKE");
			rip_tlke(lflfc, ripset, fprefix + "-tlke.txt",
				rmnum, results);
		}

		if (scriptrip)
		{
			logger.print("\tripping scripts");
			rip_scripts(lflfc, ripset, fprefix, 
				rmnum, results);
		}

		if (metadatarip)
		{
			logger.print("\tripping metadata");
			rip_metadata(lflfc, ripset, fprefix + "-metadata.txt", 
				rmnum, results);
		}

	++rmnum;

	}
}

void HERip::rip_tlkb(RipUtil::MembufStream& stream, const std::string& fprefix,
	const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat,
	RipperFormats::RipResults& results)
{
	logger.print("\tripping TLKB");

	SputmChunkHead tlkbhd;
	read_sputm_chunkhead(stream, tlkbhd);
	
	int sndnum = 0;
	SputmChunkHead hdcheck;
	while (stream.tellg() < tlkbhd.nextaddr())
	{
		bool seektonext = true;

		read_sputm_chunkhead(stream, hdcheck);
		stream.seekg(hdcheck.address);

		switch (hdcheck.type)
		{
		case talk:
		{
			SoundChunk soundc;
			read_digi_talk(stream, soundc);
			if (ripset.normalize)
				soundc.wave.normalize();

			write_pcmdata_wave(soundc.wave, fprefix
				+ "-tlkb-talk-" + to_string(sndnum)
				+ ".wav",
				ripset.ignorebytes,
				ripset.ignoreend);

			++results.audio_ripped;
			break;
		}
		case wsou:
		{
			WSOUChunk wsouc;
			read_wsou(stream, wsouc);
			RIFFEntry& riffe = wsouc.riff_entry;

			if (ripset.decode_audio)
			{
				PCMData wave;
				CommFor::riff::decode_riff(riffe.riffdat, riffe.riffdat_size,
					wave);

				if (ripset.normalize)
					wave.normalize();

				write_pcmdata_wave(wave, fprefix
					+ "-tlkb-wsou-" + to_string(sndnum)
					+ ".wav",
					ripset.ignorebytes,
					ripset.ignoreend);

				++results.audio_ripped;
			}
			else
			{
				std::ofstream ofs((fprefix
					+ "-tlkb-wsou-" + to_string(sndnum)
					+ ".wav").c_str(), std::ios_base::binary);
				ofs.write(riffe.riffdat, riffe.riffdat_size);

				++results.audio_ripped;
			}
			break;
		}
		default:
			logger.error("unrecognized TLKB subcontainer "
				+ hdcheck.name + " (type " + to_string(hdcheck.type)
				+ ") at " + to_string(hdcheck.address) + '\n'
				+ "searching for next chunk");

			// crappy error recovery code
			// this isn't watertight and hopefully will only ever run
			// on the pajama sam demo
			while (!stream.eof())
			{
				int startaddress = hdcheck.address;

				stream.seekg(stream.seek_bytes(id_TALK, 4));
				if (!stream.eof())
				{
					read_sputm_chunkhead(stream, hdcheck);

					// check if this is actually a valid chunk
					if (hdcheck.nextaddr() > stream.get_fsize() + 1
						|| hdcheck.size <= 0)
					{
						// not valid: keep trying from new location,
						// potentially skipping WSOU chunks
						continue;
					}
					else
					{
						logger.print("found new TALK chunk, continuing");
						seektonext = false;
						stream.seekg(hdcheck.address);
						break;
					}
				}

				// the probably-theoretical case where we need to find a WSOU
				stream.clear();
				stream.seekg(startaddress);
				stream.seekg(stream.seek_bytes(id_WSOU, 4));
				if (!stream.eof())
				{
					read_sputm_chunkhead(stream, hdcheck);

					// check if this is actually a valid chunk
					if (hdcheck.nextaddr() > stream.get_fsize() + 1
						|| hdcheck.size <= 0)
					{
						// not valid: keep trying from new location,
						// potentially having skipped TALK chunks
						continue;
					}
					else
					{
						logger.print("found new WSOU chunk, continuing");
						seektonext = false;
						stream.seekg(hdcheck.address);
						break;
					}
				}
			}

			// found new chunk: proceed
			if (hdcheck.type == talk || hdcheck.type == wsou)
				break;

			logger.print("no next chunk found, giving up");
			return;
		}
		
		++sndnum;

		if (seektonext)
			stream.seekg(hdcheck.nextaddr());
	}
}

void HERip::rip_song_type12(RipUtil::MembufStream& stream, const std::string& fprefix,
	const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat,
	RipperFormats::RipResults& results)
{
	logger.print("\tripping SONG");

	SputmChunkHead songhd;
	read_sputm_chunkhead(stream, songhd);

	SputmChunkHead sghdhd;
	read_sputm_chunkhead(stream, sghdhd);
	stream.seekg(sghdhd.address);

	SONGHeader song_header;
	read_songhead_type1(stream, song_header);
	stream.seekg(sghdhd.nextaddr());

	rip_song_entries(stream, song_header, fprefix, ripset, results);
}

void HERip::rip_song_type34(RipUtil::MembufStream& stream, const std::string& fprefix,
	const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat,
	RipperFormats::RipResults& results)
{
	logger.print("\tripping SONG");

	SputmChunkHead songhd;
	read_sputm_chunkhead(stream, songhd);

	SONGHeader song_header;
	read_songhead_type234(stream, song_header);

	rip_song_entries(stream, song_header, fprefix, ripset, results);
}

void HERip::rip_song_entries(RipUtil::MembufStream& stream, const SONGHeader& song_header,
	const std::string& fprefix, const RipperFormats::RipperSettings& ripset, 
	RipperFormats::RipResults& results)
{
	for (std::vector<SONGEntry>::size_type i = 0;
		i < song_header.song_entries.size(); i++)
	{
		if (roomend != not_set && i > (unsigned int)roomend)
		{
			break;
		}
		else if (roomstart != not_set && i < (unsigned int)roomstart)
		{
			continue;
		}

		const SONGEntry& songe = song_header.song_entries[i];

		stream.seekg(songe.address);

		SputmChunkHead hdcheck;
		read_sputm_chunkhead(stream, hdcheck);
		stream.seekg(hdcheck.address);
		switch (hdcheck.type)
		{
		case digi:
		{
			SoundChunk soundc;
			read_digi_talk(stream, soundc);

			if (ripset.normalize)
				soundc.wave.normalize();

			write_pcmdata_wave(soundc.wave, fprefix
				+ "-song-digi-" + to_string(i)
				+ ".wav",
				ripset.ignorebytes,
				ripset.ignoreend);

			++results.audio_ripped;
			break;
		}
		case riff:
		{
			if (ripset.decode_audio)
			{
				RIFFEntry riffe;
				read_riff(stream, riffe);

				PCMData wave;
				CommFor::riff::decode_riff(riffe.riffdat, riffe.riffdat_size,
					wave);

				if (ripset.normalize)
					wave.normalize();

				write_pcmdata_wave(wave, fprefix
					+ "-song-riff-" + to_string(i)
					+ ".wav",
					ripset.ignorebytes,
					ripset.ignoreend);

				++results.audio_ripped;
			}
			else
			{
				// RIFF uses little-endian, noninclusive chunk sizes
				int sz = set_end(hdcheck.size, 4, DatManip::le) + 8;
				char* data = new char[sz];
				stream.read(data, sz);
				std::ofstream ofs((fprefix
					+ "-song-riff-" + to_string(i)
					+ ".wav").c_str(), std::ios_base::binary);
				ofs.write(data, sz);
				delete[] data;

				++results.audio_ripped;
			}

			break;
		}
		case chunk_unknown:		// assume to be unheadered PCM (SONG type 1)
		{
			// assume samplerate of 11025 Hz
			PCMData wave(songe.length, 1, 11025, 8);
			wave.set_signed(DatManip::has_nosign);
			stream.read(wave.get_waveform(), songe.length);

			if (ripset.normalize)
				wave.normalize();

			write_pcmdata_wave(wave, fprefix
				+ "-song-unheadered-" + to_string(i)
				+ ".wav",
				ripset.ignorebytes,
				ripset.ignoreend);

			++results.audio_ripped;
			break;
		}
		case chunk_none:
			logger.error("\tcould not read SONG subchunk at " +  to_string(stream.tellg()));
			break;
		default:
			logger.error("\trecognized but invalid SONG subchunk " + hdcheck.name 
				+ " at " + to_string(hdcheck.address)
				+ " (type " + to_string(hdcheck.type) + ")");
			break;
		}
	}
}

void HERip::rip_song_dmu(RipUtil::MembufStream& stream, const std::string& fprefix,
	const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat,
	RipperFormats::RipResults& results)
{
	logger.print("\tripping DMU");

	stream.seekg(0);
	SoundChunk soundc;
	read_digi_talk(stream, soundc);

	// account for DMU signedness
	soundc.wave.set_signed(DatManip::has_sign);
	soundc.wave.convert_signedness(DatManip::has_nosign);

	if (ripset.normalize)
		soundc.wave.normalize();

	write_pcmdata_wave(soundc.wave, fprefix
		+ ".wav",
		ripset.ignorebytes,
		ripset.ignoreend);

	++results.audio_ripped;
}

void HERip::rip_extdmu(const LFLFChunk& lflfc, const std::string& fprefix, 
	const RipperFormats::RipperSettings& ripset, 
	const RipperFormats::FileFormatData& fmtdat, 
	RipperFormats::RipResults& results)
{
	// check if files in FMUS exist and rip them if possible
	for (std::vector<FMUSChunk>::size_type i = 0;
		i < lflfc.fmus_chunks.size(); i++)
	{
		const FMUSChunk& fmusc = lflfc.fmus_chunks[i];
		// strip nonstandard terminators from filename
		std::string checkchars;
		checkchars += 0x20;
		checkchars += 0xD;
		checkchars += 0xA;
		checkchars += 0x1A;
		std::string filename = strip_terminators(fmusc.fmussdat_chunk.filestring,
			checkchars);
		std::string filepath = get_lowest_directory(ripset.argv[1]) + filename;
		logger.print("checking for external sound file " + filename
			+ " at " + filepath);

		if (file_exists(filepath))
		{
			RipUtil::MembufStream dmustream(filepath, RipUtil::MembufStream::rb);
			rip_song_dmu(dmustream, fprefix + "-" + strip_extension(filename), 
				ripset, fmtdat, results);
		}
		else
		{
			logger.print("couldn't find file -- file is missing or source "
				"referenced nonexistent file");
		}
	}
}

void HERip::rip_rmim(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
	const std::string& fprefix, RipperFormats::RipResults& results, int transind)
{
	for (std::vector<RMIMChunk>::size_type i = 0; 
		i < lflfc.rmim_chunk.images.size(); i++)
	{
		const IMxxChunk& imxxc = lflfc.rmim_chunk.images[i];
		
		BitmapData bmp;
		decode_imxx(imxxc, bmp, lflfc.rmhd_chunk.width, lflfc.rmhd_chunk.height,
			lflfc.trns_chunk.trns_val, transind);
		// use user-specified room palette if enabled
		if (ripset.palettenum != RipperFormats::RipConsts::not_set)
		{
			bmp.set_palettized(true);
			bmp.set_palette(room_palettes[ripset.palettenum]);
			write_bitmapdata_8bitpalettized_bmp(bmp, fprefix + "-rmim-" 
				+ to_string(i) + ".bmp");
			++results.graphics_ripped;
		}
		else if (lflfc.apals.size() == 1)	// one palette: use abbreviated filenames
		{
			bmp.set_palettized(true);
			bmp.set_palette(lflfc.apals[0]);
			write_bitmapdata_8bitpalettized_bmp(bmp, fprefix + "-rmim-" 
				+ to_string(i) + ".bmp");
			++results.graphics_ripped;
		}
		else if (lflfc.apals.size())	// multiple palettes: use full filenames
		{
			bmp.set_palettized(true);
			for (std::vector<BitmapPalette>::size_type j = 0; 
				j < lflfc.apals.size(); j++)
			{
				bmp.set_palette(lflfc.apals[j]);
				write_bitmapdata_8bitpalettized_bmp(bmp, fprefix + "-rmim-" 
					+ to_string(i) + "-apal-" + to_string(j) + ".bmp");
				++results.graphics_ripped;
			}
		}
	}
}

void HERip::rip_obim(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
	const std::string& fprefix, RipperFormats::RipResults& results, int transind)
{
	for (std::map<int, OBIMChunk>::const_iterator obim_it = lflfc.obim_chunks.begin(); 
			obim_it != lflfc.obim_chunks.end(); obim_it++)
		{
			std::map<int, OBCDChunk>::const_iterator obcd_it 
				= lflfc.obcd_chunks.find((*obim_it).first);
			// check if this OBIM has a corresponding OBCD
			if (obcd_it != lflfc.obcd_chunks.end())
			{
				int width = (*obcd_it).second.cdhd_chunk.width;
				int height = (*obcd_it).second.cdhd_chunk.height;
				for (std::vector<IMxxChunk>::size_type i = 0;
					i < (*obim_it).second.images.size(); i++)
				{
					BitmapData bmp;
					decode_imxx((*obim_it).second.images[i], bmp, width, height,
						lflfc.trns_chunk.trns_val, transind);
					// use user-specified room palette if enabled
					if (ripset.palettenum != RipperFormats::RipConsts::not_set)
					{
						bmp.set_palettized(true);
						bmp.set_palette(room_palettes[ripset.palettenum]);
						write_bitmapdata_8bitpalettized_bmp(bmp, fprefix + "-obim-" 
							+ to_string((*obim_it).first) + "-im-" + to_string(i) + ".bmp");
						++results.graphics_ripped;
					}
					else if (lflfc.apals.size() == 1)
					{
						bmp.set_palettized(true);
						bmp.set_palette(lflfc.apals[0]);
						write_bitmapdata_8bitpalettized_bmp(bmp, fprefix + "-obim-" 
							+ to_string((*obim_it).first) + "-im-" + to_string(i) + ".bmp");
						++results.graphics_ripped;
					}
					else if (lflfc.apals.size())
					{
						bmp.set_palettized(true);
						for (std::vector<BitmapPalette>::size_type j = 0;
							j < lflfc.apals.size(); j++)
						{
							bmp.set_palette(lflfc.apals[j]);
							write_bitmapdata_8bitpalettized_bmp(bmp, fprefix + "-obim-" 
								+ to_string((*obim_it).first) + "-im-" + to_string(i) 
								+ "-apal-" + to_string(j) + ".bmp");
							++results.graphics_ripped;
						}
					}
				}
			}
			else	// OBIM has no OBCD
			{
				logger.error("OBIM with ID " + to_string((*obim_it).first) + " has no corresponding OBCD");
			}
		}
}

void HERip::rip_akos(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
	const std::string& fprefix, RipperFormats::RipResults& results, int transind)
{
	// rip each AKOS in the room
	for (std::vector<AKOSChunk>::size_type i = 0;
		i < lflfc.akos_chunks.size(); i++)
	{
		const AKOSChunk& akosc = lflfc.akos_chunks[i];
		AKOSComponentContainer akos_components;

		// if sequence ripping is enabled, we have to decode the AKOSes first
		if (akosrip || sequencerip)
		{

			for (std::vector<AKOFEntry>::size_type j = 0; j < akosc.akof_entries.size(); j++)
			{
				BitmapData bmp;

				// pointer to ripping palette
				const RipUtil::BitmapPalette* ripping_palette;

				// is a REMP chunk present?
				bool has_remap_chunk = (lflfc.remp_chunk.type == remp);

				// should the local colormap be used?
				bool use_colormap = true;

				// base name of the file to output
				std::string outfile_base;
				outfile_base += fprefix + "-akos-" + to_string(i) + "-im-"
					+ to_string(j);

				// adjust parameters based on chunk properties and user settings

				// use local palette if enabled and existent,
				// OR if the palette is full (implying a remap)
				if ((ripset.localpalettes && akosc.palette.size())
					|| (ripset.palettenum == RipperFormats::RipConsts::not_set
					&& !ripset.localpalettes && akosc.palette.size() == 256))
				{
					ripping_palette = &(akosc.palette);

					// only use the colormap if the local palette is full and there
					// is no remap chunk
					use_colormap = (akosc.palette.size() == 256 && !has_remap_chunk);

					decode_akos(akosc, bmp, j, *ripping_palette,
						lflfc.trns_chunk.trns_val, transind,
						akosc.colormap, use_colormap,
						lflfc.remp_chunk.colormap, has_remap_chunk);

					// don't write file if only sequence ripping is enabled
					if (akosrip)
					{
						write_bitmapdata_8bitpalettized_bmp(bmp, outfile_base + ".bmp");
					}
				}
				// use user-specified room palette if enabled
				else if (ripset.palettenum != RipperFormats::RipConsts::not_set)
				{
					ripping_palette = &(room_palettes[ripset.palettenum]);

					decode_akos(akosc, bmp, j, *ripping_palette,
						lflfc.trns_chunk.trns_val, transind,
						akosc.colormap, use_colormap,
						lflfc.remp_chunk.colormap, has_remap_chunk);
					
					// don't write file if only sequence ripping is enabled
					if (akosrip)
					{
						write_bitmapdata_8bitpalettized_bmp(bmp, outfile_base + ".bmp");
					}
				}
				// otherwise, use current room palette(s)
				else
				{
					for (std::vector<BitmapPalette>::size_type k = 0;
						k < lflfc.apals.size(); k++)
					{
						ripping_palette = &(lflfc.apals[k]);

						// extend filename to include palette number if using
						// multiple palettes for same image
						if (lflfc.apals.size() > 1)
						{
							outfile_base += "-apal-" + to_string(k);
						}

						decode_akos(akosc, bmp, j, *ripping_palette,
							lflfc.trns_chunk.trns_val, transind,
							akosc.colormap, use_colormap,
							lflfc.remp_chunk.colormap, has_remap_chunk);
						
						// don't write file if only sequence ripping is enabled
						if (akosrip)
						{
							write_bitmapdata_8bitpalettized_bmp(bmp, outfile_base + ".bmp");
						}
					}
				}

	/*			// temporary: decode a second time to preserve raw palette information
				BitmapData rawbmp;

				decode_akos(akosc, rawbmp, j, lflfc.apals[0],
					lflfc.trns_chunk.trns_val, transind,
					akosc.colormap, true,
					lflfc.remp_chunk.colormap, has_remap_chunk); */

				// save each component so we can assemble the animation sequences later
				// TODO: proper handling for multiple palettes, currently only the
				// last one used will be saved
				akos_components.push_back(bmp);
			}
		}

		// extract animation sequences
		if (sequencerip)
		{
			FrameSequenceContainer sequences;
			

			// I'm tried of working on this :(
			bool decoded = true;
			
			if (akosc.akhd_chunk.sequence_encoding == 0x8000)
			{
				logger.qprint("Decoding sequences for AKOS " + to_string(i));
				decode_sequences(akosc.aksq_chunk, akosc.akch_chunk,
					akos_components, sequences, akosc.akhd_chunk.sequence_encoding);
			}
			else
			{
				logger.error("Unrecognized AKCH code "
					+ to_string(akosc.akhd_chunk.sequence_encoding)
					+ ", skipping sequence rip");
				decoded = false;
			}

			// vector of bools used to mark which frames have been used
			std::vector<bool> usedgraphics(akos_components.size(), false);

			int seqnum = 0;

			// generate and write out each sequence
			for (FrameSequenceContainer::iterator it = sequences.begin();
				it != sequences.end(); it++)
			{
				// compute the image dimensions necessary to contain every frame of the sequence
				SequenceSizingInfo sizeinf = compute_sequence_enclosing_dimensions(akos_components, (*it));

				// assemble and write each static frame
				for (AKSQStaticFrameSequence::iterator fit = it->staticframes.begin();
					fit != it->staticframes.end(); fit++)
				{
					RipUtil::BitmapData seqbmp(sizeinf.width, sizeinf.height, 8, true);
					seqbmp.clear(lflfc.trns_chunk.trns_val);
					// set palette
					if (ripset.localpalettes)
					{
						seqbmp.set_palette(akosc.palette);
					}
					else if (ripset.palettenum != RipperFormats::RipConsts::not_set)
					{
						seqbmp.set_palette(room_palettes[ripset.palettenum]);
					}
					else if (lflfc.apals.size())
					{
						seqbmp.set_palette(lflfc.apals[0]);
					}
					else
					{
						// help
					}
//					seqbmp.set_palette(lflfc.apals[0]);

					// blit each component to frame
					for (AKSQStaticFrameComponentContainer::iterator cit = fit->components.begin();
						cit != fit->components.end(); cit++)
					{
						seqbmp.blit_bitmapdata(akos_components[cit->graphic],
							cit->xoffset + sizeinf.centerx,
							cit->yoffset + sizeinf.centery,
							lflfc.trns_chunk.trns_val);

						// mark component as used
						usedgraphics[cit->graphic] = true;
					}

					// write out frame
					write_bitmapdata_8bitpalettized_bmp(seqbmp, fprefix + "-akos-"
						+ to_string(i) + "-sequence-" + to_string(seqnum)
						+ "-frame-" + to_string(fit->framenum)
						+ ".bmp");

					++results.animation_frames_ripped;
				}

				// assemble and write each dynamic frame
				for (AKSQDynamicFrameSequence::iterator fit = it->dynamicframes.begin();
					fit != it->dynamicframes.end(); fit++)
				{
					// write out each component
					int componentnum = 0;
					for (AKSQDynamicFrameComponentContainer::iterator cit = fit->components.begin();
						cit != fit->components.end(); cit++)
					{
						RipUtil::BitmapData compbmp(sizeinf.width, sizeinf.height, 8, true);
						compbmp.clear(lflfc.trns_chunk.trns_val);
						// set palette
						if (ripset.localpalettes)
						{
							compbmp.set_palette(akosc.palette);
						}
						else if (ripset.palettenum != RipperFormats::RipConsts::not_set)
						{
							compbmp.set_palette(room_palettes[ripset.palettenum]);
						}
						else if (lflfc.apals.size())
						{
							compbmp.set_palette(lflfc.apals[0]);
						}
						else
						{
							// help
						}
//						compbmp.set_palette(lflfc.apals[0]);

						compbmp.blit_bitmapdata(akos_components[cit->graphic],
							cit->xoffset + sizeinf.centerx,
							cit->yoffset + sizeinf.centery);

						// mark component as used
						usedgraphics[cit->graphic] = true;

						// write out frame
						write_bitmapdata_8bitpalettized_bmp(compbmp, fprefix + "-akos-"
							+ to_string(i) + "-sequence-" + to_string(seqnum)
							+ "-frame-" + to_string(fit->framenum)
							+ "-component-" + /*to_string(cit->compid)*/ to_string(componentnum)
							+ "-id-" + to_string(cit->compid)
							+ ".bmp");

						++componentnum;
					}
					++results.animation_frames_ripped;
				}

				++seqnum;
				++results.animations_ripped;
			}

			if (decoded)
			{
				bool foundunused = false;
				for (std::vector<bool>::size_type j = 0; j < usedgraphics.size(); j++)
				{
					if (!usedgraphics[j])
					{
						// initial unused graphics alert
						if (!foundunused)
						{
							logger.print("AKOS " + to_string(i)
								+ " may have unused components:");
							foundunused = true;
						}

						logger.print("\t" + to_string(j));
					}
				}
			}
		}
			
		// rip AKAX frames if they exist
		if (akosrip)
		{
			const AKAXChunk& akaxc = akosc.akax_chunk;

			BitmapData bmp(640, 480, 8, true);

			if (ripset.localpalettes)
				bmp.set_palette(akosc.palette);
			else if (ripset.palettenum != RipperFormats::RipConsts::not_set)
				bmp.set_palette(room_palettes[ripset.palettenum]);
			else if (lflfc.apals.size())
				bmp.set_palette(lflfc.apals[0]);

			bmp.clear(lflfc.trns_chunk.trns_val);

			for (std::vector<AUXDChunk>::size_type j = 0; 
				j < akaxc.auxd_chunks.size(); j++)
			{
				const AUXDChunk& auxdc = akaxc.auxd_chunks[j];
				if (auxdc.axfd_chunk.imgdat_size != 0)
				{
					decode_auxd(auxdc, bmp, lflfc.trns_chunk.trns_val, transind);

					write_bitmapdata_8bitpalettized_bmp(bmp, fprefix
						+ "-akos-" + to_string(i)
						+ "-auxd-" + to_string(j)
						+ ".bmp");

					++results.animation_frames_ripped;
				}
			}
		}
	}
}

void HERip::rip_awiz(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
	const std::string& fprefix, RipperFormats::RipResults& results, int transind)
{
	// rewrite these to call a common function

	// rip regular AWIZ
	for (std::vector<AWIZChunk>::size_type i = 0;
		i < lflfc.awiz_chunks.size(); i++)
	{
		const AWIZChunk& awizc = lflfc.awiz_chunks[i];

		// some games have "empty" AWIZs just to make us mad
		if (awizc.wizd_chunk.type == wizd)
		{

			BitmapData bmp;

			// use local palette if enabled and existent,
			// OR if the palette is full (implying a remap)
			if ((ripset.localpalettes && awizc.palette.size())
				|| (ripset.palettenum == RipperFormats::RipConsts::not_set
				&& !ripset.localpalettes && awizc.palette.size() == 256))
			{
				decode_awiz(awizc, bmp, awizc.palette,
					lflfc.trns_chunk.trns_val, transind, awizc.rmap_chunk.colormap, awizc.rmap_chunk.type == rmap);
				bmp.write(fprefix
					+ "-awiz-" + to_string(i) + ".bmp");
				++results.graphics_ripped;
			}
			// use user-specified room palette if enabled
			else if (ripset.palettenum != RipperFormats::RipConsts::not_set)
			{
				decode_awiz(awizc, bmp, room_palettes[ripset.palettenum],
					lflfc.trns_chunk.trns_val, transind);
				bmp.write(fprefix
					+ "-awiz-" + to_string(i) + ".bmp");
				++results.graphics_ripped;
			}
			// otherwise, use room palette(s)
			else if (lflfc.apals.size() == 1)
			{
				decode_awiz(awizc, bmp, lflfc.apals[0],
					lflfc.trns_chunk.trns_val, transind);
				bmp.write(fprefix
					+ "-awiz-" + to_string(i) + ".bmp");
				++results.graphics_ripped;
			}
			else if (lflfc.apals.size())
			{
				for (std::vector<BitmapPalette>::size_type j = 0;
					j < lflfc.apals.size(); j++)
				{
					decode_awiz(awizc, bmp, lflfc.apals[j],
						lflfc.trns_chunk.trns_val, transind);
					bmp.write(fprefix
						+ "-awiz-" + to_string(i) 
						+ "-apal-" + to_string(j) + ".bmp");
					++results.graphics_ripped;
				}
			}
		}
	}

	// rip MULT-embedded AWIZ
	for (std::vector<MULTChunk>::size_type i = 0; 
		i < lflfc.mult_chunks.size(); i++)
	{
		const MULTChunk& multc = lflfc.mult_chunks[i];

		for (std::vector<AWIZChunk>::size_type j = 0;
			j < multc.awiz_chunks.size(); j++)
		{
			const AWIZChunk& awizc = multc.awiz_chunks[j];

			if (awizc.wizd_chunk.type == wizd)
			{

				BitmapData bmp;

				// use local palette if enabled and existent
				// OR if the palette is full (implying a remap)
				if ((ripset.localpalettes && awizc.palette.size())
				|| (ripset.palettenum == RipperFormats::RipConsts::not_set
				&& !ripset.localpalettes && awizc.palette.size() == 256))
				{
					decode_awiz(awizc, bmp, awizc.palette,
						lflfc.trns_chunk.trns_val, transind,
						awizc.rmap_chunk.colormap, awizc.rmap_chunk.type == rmap);
					bmp.write(fprefix
						+ "-mult-" + to_string(i)
						+ "-awiz-" + to_string(j) + ".bmp");
					++results.graphics_ripped;
				}
				// use user-specified room palette if enabled
				else if (ripset.palettenum != RipperFormats::RipConsts::not_set)
				{
					decode_awiz(awizc, bmp, room_palettes[ripset.palettenum],
						lflfc.trns_chunk.trns_val, transind,
						multc.defa_chunk.rmap_chunk.colormap, multc.defa_chunk.rmap_chunk.type == rmap);
					bmp.write(fprefix
						+ "-mult-" + to_string(i)
						+ "-awiz-" + to_string(j) + ".bmp");
					++results.graphics_ripped;
				}
				// otherwise, use MULT palette if it exists
				else if (ripset.localpalettes && multc.defa_chunk.palette.size())
				{
					decode_awiz(awizc, bmp, multc.defa_chunk.palette,
						lflfc.trns_chunk.trns_val, transind,
						multc.defa_chunk.rmap_chunk.colormap, multc.defa_chunk.rmap_chunk.type == rmap);
					bmp.write(fprefix
						+ "-mult-" + to_string(i)
						+ "-awiz-" + to_string(j) + ".bmp");
					++results.graphics_ripped;
				}
				// otherwise, use room palette(s)
				else if (lflfc.apals.size() == 1)
				{
					decode_awiz(awizc, bmp, lflfc.apals[0],
						lflfc.trns_chunk.trns_val, transind,
						multc.defa_chunk.rmap_chunk.colormap, multc.defa_chunk.rmap_chunk.colormap.size() != 0);
					bmp.write(fprefix
						+ "-mult-" + to_string(i)
						+ "-awiz-" + to_string(j) + ".bmp");
					++results.graphics_ripped;
				}
				else if (lflfc.apals.size() != 0)
				{
					for (std::vector<BitmapPalette>::size_type k = 0;
						k < lflfc.apals.size(); k++)
					{
						decode_awiz(awizc, bmp, lflfc.apals[k],
							lflfc.trns_chunk.trns_val, transind,
							multc.defa_chunk.rmap_chunk.colormap, multc.defa_chunk.rmap_chunk.colormap.size() != 0);
						bmp.write(fprefix
							+ "-mult-" + to_string(i)
							+ "-awiz-" + to_string(j) 
							+ "-apal-" + to_string(k) + ".bmp");
						++results.graphics_ripped;
					}
				}
			}
		}
	}
}

void HERip::rip_char(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
	const std::string& fprefix, RipperFormats::RipResults& results, int transind)
{
	// chars have variable palettes; this one is arbitrary, but
	// should at least provide distinct tones
	BitmapPalette testpal;
	for (int k = 0; k < 256; k++)
		testpal[k] = (k | (k << 8) | (k << 16));
	testpal[0] = 0x000000;
	testpal[1] = 0xFFFFFE;
	testpal[2] = 0x555554;
	testpal[3] = 0xAAAAA9;
	testpal[4] = 0x888887;
	testpal[lflfc.trns_chunk.trns_val] = 0xAB00AB;	// background
	for (std::vector<CHARChunk>::size_type i = 0;
		i < lflfc.char_chunks.size(); i++)
	{
		const CHARChunk& charc = lflfc.char_chunks[i];
		for (std::vector<CHAREntry>::size_type j = 0;
			j < charc.char_entries.size(); j++)
		{
			const CHAREntry& chare = charc.char_entries[j];
			BitmapData bmp;
			decode_char(bmp, chare, charc.compr, testpal,
				lflfc.trns_chunk.trns_val, transind);
			write_bitmapdata_8bitpalettized_bmp(bmp, fprefix
				+ "-char-" + to_string(i)
				+ "-num-" + to_string(j)
				+ ".bmp");
			++results.graphics_ripped;
		}
	}
}

void HERip::rip_sound(std::vector<SoundChunk>& sound_chunks, 
	const RipperFormats::RipperSettings& ripset, const std::string& fprefix,
	RipperFormats::RipResults& results)
{
	for (std::vector<SoundChunk>::size_type i = 0;
		i < sound_chunks.size(); i++)
	{
		SoundChunk& soundc = sound_chunks[i];

		if (ripset.normalize)
			soundc.wave.normalize();

		write_pcmdata_wave(soundc.wave, fprefix
			+ to_string(i)
			+ ".wav",
			ripset.ignorebytes,
			ripset.ignoreend);

		++results.audio_ripped;
	}
}

void HERip::rip_wsou(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
	const std::string& fprefix, RipperFormats::RipResults& results,
	bool decode_audio)
{
	for (std::vector<WSOUChunk>::size_type i = 0;
		i < lflfc.wsou_chunks.size(); i++)
	{
		const RIFFEntry& riff_entry = lflfc.wsou_chunks[i].riff_entry;

		if (!decode_audio)
		{
			std::ofstream ofs((fprefix
				+ "-wsou-" + to_string(i)
				+ ".wav").c_str(), std::ios_base::binary);
			ofs.write(riff_entry.riffdat, riff_entry.riffdat_size);
		}
		else
		{
			// who would do this to us :(
			PCMData wave;
			CommFor::riff::decode_riff(riff_entry.riffdat, riff_entry.riffdat_size,
				wave);
			if (ripset.normalize)
				wave.normalize();
			write_pcmdata_wave(wave, fprefix
				+ "-wsou-" + to_string(i)
				+ ".wav",
				ripset.ignorebytes,
				ripset.ignoreend);
		}

		++results.audio_ripped;
	}
}

void HERip::rip_tlke(LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
	const std::string& filename, int rmnum, RipperFormats::RipResults& results)
{
	// only create the TLKE file if there are TLKEs to rip
	if (!cleared_tlke_file)
	{
		std::ofstream(filename.c_str(), std::ios_base::trunc);
		cleared_tlke_file = true;
	}

	std::ofstream ofs(filename.c_str(), std::ios_base::app);

	ofs << "room " << rmnum << '\n';
	for (std::vector<TLKEChunk>::size_type i = 0;
		i < lflfc.tlke_chunks.size(); i++)
	{
		ofs << '\t' << lflfc.tlke_chunks[i].text_chunk.text << '\n';
		++results.strings_ripped;
	}
	ofs << '\n';
}

void HERip::rip_scripts(LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
	const std::string& filename, int rmnum, RipperFormats::RipResults& results)
{
	// SCRPs
	for (int i = 0; i < lflfc.scrp_chunks.size(); i++) {
		SputmChunk& scrpc = lflfc.scrp_chunks[i];
		
		if (catscripts) {
			std::ofstream ofs((filename + "-scripts").c_str(),
				std::ios_base::app | std::ios_base::binary);

			ofs << std::string("<--SCRIPT ")
				+ to_string(rmnum)
				+ "-" + to_string(i)
				+ " START-->";
			ofs.write(scrpc.data, scrpc.datasize);
			ofs << std::string("<--SCRIPT ")
				+ to_string(rmnum)
				+ "-" + to_string(i)
				+ " END-->";
		}
		else {
			std::ofstream ofs((filename
				+ "-room-" + to_string(rmnum)
				+ "-scrp-" + to_string(i)).c_str(), std::ios_base::binary);
			ofs.write(scrpc.data, scrpc.datasize);
		}
	}

	// LSCRs
	for (int i = 0; i < lflfc.lscr_chunks.size(); i++) {
		SputmChunk& lscrc = lflfc.lscr_chunks[i];
		
		if (catscripts) {
			std::ofstream ofs((filename + "-scripts").c_str(),
				std::ios_base::app | std::ios_base::binary);

			ofs << std::string("<--SCRIPT ")
				+ to_string(rmnum)
				+ "-" + to_string(i)
				+ " START-->";
			ofs.write(lscrc.data, lscrc.datasize);
			ofs << std::string("<--SCRIPT ")
				+ to_string(rmnum)
				+ "-" + to_string(i)
				+ " END-->";
		}
		else {
			std::ofstream ofs((filename
				+ "-room-" + to_string(rmnum)
				+ "-lscr-" + to_string(i)).c_str(), std::ios_base::binary);
			ofs.write(lscrc.data, lscrc.datasize);
		}
	}

	// LSC2s
	for (int i = 0; i < lflfc.lsc2_chunks.size(); i++) {
		SputmChunk& lsc2c = lflfc.lsc2_chunks[i];
		
		if (catscripts) {
			std::ofstream ofs((filename + "-scripts").c_str(),
				std::ios_base::app | std::ios_base::binary);

			ofs << std::string("<--SCRIPT ")
				+ to_string(rmnum)
				+ "-" + to_string(i)
				+ " START-->";
			ofs.write(lsc2c.data, lsc2c.datasize);
			ofs << std::string("<--SCRIPT ")
				+ to_string(rmnum)
				+ "-" + to_string(i)
				+ " END-->";
		}
		else {
			std::ofstream ofs((filename
				+ "-room-" + to_string(rmnum)
				+ "-lsc2-" + to_string(i)).c_str(), std::ios_base::binary);
			ofs.write(lsc2c.data, lsc2c.datasize);
		}
	}
}

void HERip::rip_metadata(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
	const std::string& filename, int rmnum, RipperFormats::RipResults& results)
{
	std::ofstream ofs(filename.c_str(), std::ios_base::app);
	ofs << "room " << rmnum << '\n';
	
	ofs << '\t' << "TRNS: " << lflfc.trns_chunk.trns_val << '\n';

	for (std::vector<AKOSChunk>::size_type i = 0;
		i < lflfc.akos_chunks.size(); i++)
	{
		const AKOSChunk& akosc = lflfc.akos_chunks[i];

		if (akosc.file_date.size()
			|| akosc.file_name.size()
			|| akosc.file_compr.size()
			|| akosc.sqdb_chunk.seqi_chunks.size())
		{
			ofs << '\t' << "AKOS " << i << ":" << '\n';
			if (akosc.file_date.size())
			{
				ofs << '\t' << '\t' << "SP2C: " << akosc.file_date << '\n';
			}
			if (akosc.file_name.size())
			{
				ofs << '\t' << '\t' << "SPLF: " << akosc.file_name << '\n';
			}
			if (akosc.file_compr.size())
			{
				ofs << '\t' << '\t' << "CLRS: " << akosc.file_compr << '\n';
			}
			if (akosc.sqdb_chunk.seqi_chunks.size())
			{
				ofs << '\t' << '\t' << "SQDB:" << '\n';
				for (std::vector<SEQIChunk>::size_type j = 0;
					j < akosc.sqdb_chunk.seqi_chunks.size(); j++)
				{
					const SEQIChunk& seqic = akosc.sqdb_chunk.seqi_chunks[j];

					ofs << '\t' << '\t' << '\t' << "SEQI " << j << ": "
						<< seqic.name_val << '\n';
				}
			}
		}
	}

	for (std::map<ObjectID, OBCDChunk>::const_iterator it = lflfc.obcd_chunks.begin();
		it != lflfc.obcd_chunks.end(); it++)
	{
		const OBCDChunk& obcdc = (*it).second;

		if (obcdc.obna_val.size())
		{
			ofs << '\t' << "OBCD " << (*it).first << ":" << '\n';

			ofs << '\t' << '\t' << "OBNA: " << obcdc.obna_val << '\n';
		}
	}

	ofs << '\n';
}

void HERip::scan_palettes(RipUtil::MembufStream& stream)
{
	int start = stream.tellg();

	// doing this the lazy way: read each LFLF in full,
	// then add palettes to class storage
	SputmChunkHead hdcheck;
	read_sputm_chunkhead(stream, hdcheck);
	stream.seekg(hdcheck.address);
	while (!stream.eof() && hdcheck.type == lflf)
	{
		LFLFChunk lflfc;
		read_lflf(stream, lflfc);
		for (std::vector<RipUtil::BitmapPalette>::size_type i = 0; 
			i < lflfc.apals.size(); i++)
			room_palettes.push_back(lflfc.apals[i]);
	}
	stream.clear();
	stream.seekg(start);
}


};	// end of namespace Humongous
