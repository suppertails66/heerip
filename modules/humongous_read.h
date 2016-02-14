/* Functions for reading Humongous-format data into an internally
   usable representation */

#include "humongous_structs.h"

#include "RipModule.h"
#include "../utils/MembufStream.h"
#include "../utils/BitmapData.h"
#include "../RipperFormats.h"
#include <map>
#include <vector>

namespace Humongous
{


// given a chunk 4CC, return its ChunkType entry or chunk_unknown/chunk_none
ChunkType getchunktype(const std::string& id);

// read a c-string, limiting length to given value
std::string safe_read_cstring(RipUtil::MembufStream& stream, int len);

// read from stream into a SputmChunkHead
void read_sputm_chunkhead(RipUtil::MembufStream& stream, SputmChunkHead& chunkhd);

// read from stream into a SputmChunk (including data portion)
void read_sputm_chunk(RipUtil::MembufStream& stream, SputmChunk& chunkhd);

// if the chunk in stream is of specified type, read into given SputmChunk,
// returning true if chunk was read
bool read_chunk_if_exists(RipUtil::MembufStream& stream, SputmChunk& dest,
	const ChunkType type);

// while the chunk type read from stream matches the given type,
// read chunks using the supplied read function and put them in a vector
template<typename T> int read_chunks_while_exist(RipUtil::MembufStream& stream, ChunkType type,
	std::vector<T>& dest, void (*readfunc)(RipUtil::MembufStream&, T&))
{
	int numread = 0;
	SputmChunkHead checker;
	read_sputm_chunkhead(stream, checker);
	while (checker.type == type)
	{
		stream.seekg(checker.address);
		T chunk;
		readfunc(stream, chunk);
		dest.push_back(chunk);
		++numread;
		stream.seekg(checker.nextaddr());
		read_sputm_chunkhead(stream, checker);
	}
	stream.seekg(checker.address);
	return numread;
}

// helper functions for LFLF reading

// read from stream into an LFLFChunk (no ordering enforced)
void read_lflf(RipUtil::MembufStream& stream, LFLFChunk& lflfc);

// rooms/general
void read_rmim(RipUtil::MembufStream& stream, RMIMChunk& rmimc);
void read_rmhd(RipUtil::MembufStream& stream, RMHDChunk& rmhdc);
void read_wrap(RipUtil::MembufStream& stream, WRAPChunk& wrapc);
void read_offs(RipUtil::MembufStream& stream, OFFSChunk& offsc);
void read_imxx(RipUtil::MembufStream& stream, IMxxChunk& imxxc);
void read_nlsc(RipUtil::MembufStream& stream, NLSCChunk& nlscc);
void read_tlke(RipUtil::MembufStream& stream, std::vector<TLKEChunk>& tlke_chunks);
void read_text(RipUtil::MembufStream& stream, TEXTChunk& textc);

// palettes
void read_pals(RipUtil::MembufStream& stream, std::vector<RipUtil::BitmapPalette>& apals);
void read_cycl(RipUtil::MembufStream& stream, CYCLChunk& cyclc);
void read_trns(RipUtil::MembufStream& stream, TRNSChunk& trnsc);
void read_remp(RipUtil::MembufStream& stream, REMPChunk& rempc);
void read_apal_rgbs(RipUtil::MembufStream& stream, SputmChunk& sputc,
	RipUtil::BitmapPalette& pal);
void read_rgbs(RipUtil::MembufStream& stream, RipUtil::BitmapPalette& palette);
int read_color(RipUtil::MembufStream & stream);
void read_palette(RipUtil::MembufStream& stream, const SputmChunkHead& palcontainer,
	RipUtil::BitmapPalette& pal);

// objects
void read_obim(RipUtil::MembufStream& stream, OBIMChunk& obimc);
void read_imhd(RipUtil::MembufStream& stream, IMHDChunk& imhdc);
void read_obcd(RipUtil::MembufStream& stream, OBCDChunk& obcdc);
void read_cdhd(RipUtil::MembufStream& stream, CDHDChunk& cdhdc);
int read_obims_map(RipUtil::MembufStream& stream, std::map<ObjectID, OBIMChunk>& obimm);
int read_obcds_map(RipUtil::MembufStream& stream, std::map<ObjectID, OBCDChunk>& obcdm);

// sounds
void read_soun(RipUtil::MembufStream& stream, SoundChunk& soundc);
void read_digi_talk(RipUtil::MembufStream& stream, SoundChunk& soundc);
void read_hshd(RipUtil::MembufStream& stream, HSHDChunk& hshdc);
void read_fmus(RipUtil::MembufStream& stream, FMUSChunk& fmusc);
void read_fmussdat(RipUtil::MembufStream& stream, FMUSSDATChunk& fmussdatc);
void read_wsou(RipUtil::MembufStream& stream, WSOUChunk& wsouc);
void read_riff(RipUtil::MembufStream& stream, RIFFEntry& riff_entry);

// actors
void read_akos(RipUtil::MembufStream& stream, AKOSChunk& akosc);
void read_akhd(RipUtil::MembufStream& stream, AKHDChunk& akhdc);
void read_akpl(RipUtil::MembufStream& stream, AKPLChunk& akplc);
void read_seqi(RipUtil::MembufStream& stream, SEQIChunk& seqic);
void read_aksq(RipUtil::MembufStream& stream, AKSQChunk& aksqc);
void read_akch(RipUtil::MembufStream& stream, AKCHChunk& akchc,
	int pointer_table_1_size, int pointer_table_2_encoding);
void read_akof(RipUtil::MembufStream& stream, const SputmChunkHead& akofc, 
	std::vector<AKOFEntry>& akof_entries);
void read_akci(RipUtil::MembufStream& stream, const SputmChunkHead& akcic,
	const std::vector<AKOFEntry>& akof_entries, std::vector<AKCIEntry>& akci_entries);
void read_akcd(RipUtil::MembufStream& stream, const SputmChunkHead& akcdc, 
	std::vector<AKOFEntry>& akofe, std::vector <AKCIEntry>& akcie,
	std::vector<AKCDEntry>& akcd_entries);
void read_axfd(RipUtil::MembufStream& stream, AXFDChunk& axfdc);
void read_auxd(RipUtil::MembufStream& stream, AUXDChunk& auxdc);
void read_akax(RipUtil::MembufStream& stream, AKAXChunk& akaxc);
void read_sqdb(RipUtil::MembufStream& stream, SQDBChunk& sqdbc);

// animations
void read_awiz(RipUtil::MembufStream& stream, AWIZChunk& awizc);
void read_mult(RipUtil::MembufStream& stream, std::vector<MULTChunk>& mult_chunks);
void read_awiz_wrap(RipUtil::MembufStream& stream, std::vector<AWIZChunk>& awiz_chunks);
void read_char(RipUtil::MembufStream& stream, std::vector<CHARChunk>& chars);

// Helper functions for SONG reading

void read_songhead_type1(RipUtil::MembufStream& stream, SONGHeader& song_header);
void read_songhead_type234(RipUtil::MembufStream& stream, SONGHeader& song_header);


};	// end of namepace Humongous

#pragma once