/* Ripping functions for common formats */

#include "../utils/MembufStream.h"
#include "../utils/BitmapData.h"
#include "../utils/PCMData.h"
#include <string>

// common, non-game specific formats
namespace CommFor
{


	namespace aiff
	{


		const static char hd_id[4]
		= { 'F', 'O', 'R', 'M' };
		const static char aiff_id[4]
		= { 'A', 'I', 'F', 'F' };
		const static char comm_id[4]
		= { 'C', 'O', 'M', 'M' };
		const static char ssnd_id[4]
		= { 'S', 'S', 'N', 'D' };
		const static char mark_id[4]
		= { 'M', 'A', 'R', 'K' };
		const static char inst_id[4]
		= { 'I', 'N', 'S', 'T' };
		const static char appl_id[4]
		= { 'A', 'P', 'P', 'L' };

		const static int playmode_noloop = 0;
		const static int playmode_forloop = 1;
		const static int playmode_bidiloop = 2;

		// marker info from MARK chunk
		struct Mark
		{
			Mark()
				: id(0), pos(0), name("") { };

			int id;
			int pos;
			std::string name;
		};


	};	// end of namespace aiff

	namespace riff
	{


		enum ChunkType
		{
			chunk_unknown, chunk_none,
			chunk_riff, chunk_wave, chunk_fmt, 
			chunk_data, chunk_fact, chunk_list,
			chunk_ieng, chunk_isft, chunk_cue,
			chunk_ltxt, chunk_adtl, chunk_rgn, 
			chunk_labl, chunk_info, chunk_icrd
		};

		struct RIFFChunkHead
		{
			RIFFChunkHead() 
				: type(chunk_none), address(0), size(0) { };
			
			int nextaddr() { return address + size; }
			
			std::string name;
			ChunkType type;
			int address;
			int dataddress;
			int size;
		};

		ChunkType getchunktype(const std::string& id);

		void read_riff_chunkhead(const char* data, int address,
			RIFFChunkHead& riffc);

		void decode_riff_uncompressed(const char* data, int datalen,
			RipUtil::PCMData& wave);

		void decode_riff_imaadpcm(const char* data, int datalen,
			RipUtil::PCMData& wave, int nibsperblock);

		void decode_riff(const char* riffdat, int riffdatlen,
			RipUtil::PCMData& wave);


	};	// end of namespace riff

	namespace bmp
	{


		// read a standard BMP/DIB from stream into BitmapData
		void read_bmp_bitmapdata(RipUtil::MembufStream& stream,
			RipUtil::BitmapData& dat);


	};	// end of namespace bmp


};	// end of namespace CommFor


#pragma once