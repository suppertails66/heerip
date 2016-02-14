#include "humongous_structs.h"
#include "humongous_read.h"

#include "../utils/MembufStream.h"
#include "../utils/BitStream.h"
#include "../utils/BitmapData.h"
#include "../utils/PCMData.h"
#include "../utils/datmanip.h"
#include "../utils/ErrorLog.h"
#include "../utils/logger.h"
#include "../RipperFormats.h"
#include <fstream>
#include <string>
#include <cstring>
#include <iostream>

using namespace RipUtil;
using namespace RipperFormats;
using namespace ErrLog;
using namespace Logger;

namespace Humongous
{


ChunkType getchunktype(const std::string& id)
{
	if (id.size() != 4)
		return chunk_unknown;

	char cid[4];
	std::memcpy(cid, id.c_str(), 4);

	// this is terrible, do something sensible instead
	if (quickcmp(cid, id_LECF, 4)) return lecf;
	else if (quickcmp(cid, id_LFLF, 4))	return lflf;
	else if (quickcmp(cid, id_RMIM, 4))	return rmim;
	else if (quickcmp(cid, id_RMIH, 4)) return rmih;
	else if (quickcmp(cid, id_RMDA, 4)) return rmda;
	else if (quickcmp(cid, id_RMHD, 4)) return rmhd;
	else if (quickcmp(cid, id_CYCL, 4)) return cycl;
	else if (quickcmp(cid, id_TRNS, 4)) return trns;
	else if (quickcmp(cid, id_PALS, 4)) return pals;
	else if (quickcmp(cid, id_WRAP, 4)) return wrap;
	else if (quickcmp(cid, id_OFFS, 4)) return offs;
	else if (quickcmp(cid, id_APAL, 4)) return apal;
	else if (quickcmp(cid, id_RGBS, 4)) return rgbs;
	else if (quickcmp(cid, id_REMP, 4)) return remp;
	else if (quickcmp(cid, id_OBIM, 4)) return obim;
	else if (quickcmp(cid, id_IMHD, 4)) return imhd;
	else if (quickcmp(cid, id_SMAP, 4)) return smap;
	else if (quickcmp(cid, id_BMAP, 4)) return bmap;
	else if (quickcmp(cid, id_BOMP, 4)) return bomp;
	// IMxx goes at end of list to avoid naming conflicts
	// same with ZPxx for consistency
	else if (quickcmp(cid, id_TMSK, 4)) return tmsk;
	else if (quickcmp(cid, id_OBCD, 4)) return obcd;
	else if (quickcmp(cid, id_CDHD, 4)) return cdhd;
	else if (quickcmp(cid, id_VERB, 4)) return verb;
	else if (quickcmp(cid, id_OBNA, 4)) return obna;
	else if (quickcmp(cid, id_EXCD, 4)) return excd;
	else if (quickcmp(cid, id_ENCD, 4)) return encd;
	else if (quickcmp(cid, id_NLSC, 4)) return nlsc;
	else if (quickcmp(cid, id_LSCR, 4)) return lscr;
	else if (quickcmp(cid, id_LSC2, 4)) return lsc2;
	else if (quickcmp(cid, id_BOXD, 4)) return boxd;
	else if (quickcmp(cid, id_BOXM, 4)) return boxm;
	else if (quickcmp(cid, id_SCAL, 4)) return scal;
	else if (quickcmp(cid, id_POLD, 4)) return pold;
	else if (quickcmp(cid, id_SCRP, 4)) return scrp;
	else if (quickcmp(cid, id_TLKE, 4)) return tlke;
	else if (quickcmp(cid, id_TEXT, 4)) return text;
	else if (quickcmp(cid, id_TLKB, 4)) return tlkb;
	else if (quickcmp(cid, id_SONG, 4)) return song;
	else if (quickcmp(cid, id_SGHD, 4)) return sghd;
	else if (quickcmp(cid, id_SGEN, 4)) return sgen;
	else if (quickcmp(cid, id_SOUN, 4)) return soun;
	else if (quickcmp(cid, id_WSOU, 4)) return wsou;
	else if (quickcmp(cid, id_DIGI, 4)) return digi;
	else if (quickcmp(cid, id_TALK, 4)) return talk;
	else if (quickcmp(cid, id_SBNG, 4)) return sbng;
	else if (quickcmp(cid, id_MIDI, 4)) return midi;
	else if (quickcmp(cid, id_HSHD, 4)) return hshd;
	else if (quickcmp(cid, id_SDAT, 4)) return sdat;
	else if (quickcmp(cid, id_PETE, 4)) return pete;
	else if (quickcmp(cid, id_SRFS, 4)) return srfs;
	else if (quickcmp(cid, id_FMUS, 4)) return fmus;
	else if (quickcmp(cid, id_MRAW, 4)) return mraw;
	else if (quickcmp(cid, id_RIFF, 4)) return riff;
	else if (quickcmp(cid, id_AKOS, 4)) return akos;
	else if (quickcmp(cid, id_AKHD, 4)) return akhd;
	else if (quickcmp(cid, id_AKPL, 4)) return akpl;
	else if (quickcmp(cid, id_AKSQ, 4)) return aksq;
	else if (quickcmp(cid, id_AKFO, 4)) return akfo;
	else if (quickcmp(cid, id_AKCH, 4)) return akch;
	else if (quickcmp(cid, id_AKOF, 4)) return akof;
	else if (quickcmp(cid, id_AKCI, 4)) return akci;
	else if (quickcmp(cid, id_AKCD, 4)) return akcd;
	else if (quickcmp(cid, id_AKLC, 4)) return aklc;
	else if (quickcmp(cid, id_AKST, 4)) return akst;
	else if (quickcmp(cid, id_AKCT, 4)) return akct;
	else if (quickcmp(cid, id_AKAX, 4)) return akax;
	else if (quickcmp(cid, id_AUXD, 4)) return auxd;
	else if (quickcmp(cid, id_AXFD, 4)) return axfd;
	else if (quickcmp(cid, id_AXUR, 4)) return axur;
	else if (quickcmp(cid, id_AXER, 4)) return axer;
	else if (quickcmp(cid, id_SP2C, 4)) return sp2c;
	else if (quickcmp(cid, id_SPLF, 4)) return splf;
	else if (quickcmp(cid, id_CLRS, 4)) return clrs;
	else if (quickcmp(cid, id_IMGL, 4)) return imgl;
	else if (quickcmp(cid, id_SQDB, 4)) return sqdb;
	else if (quickcmp(cid, id_SEQI, 4)) return seqi;
	else if (quickcmp(cid, id_NAME, 4)) return name;
	else if (quickcmp(cid, id_STOF, 4)) return stof;
	else if (quickcmp(cid, id_SQLC, 4)) return sqlc;
	else if (quickcmp(cid, id_SIZE, 4)) return size;
	else if (quickcmp(cid, id_CHAR, 4)) return chunk_char;
	else if (quickcmp(cid, id_AWIZ, 4))	return awiz;
	else if (quickcmp(cid, id_XMAP, 4)) return xmap;
	else if (quickcmp(cid, id_CNVS, 4)) return cnvs;
	else if (quickcmp(cid, id_RELO, 4)) return relo;
	else if (quickcmp(cid, id_WIZH, 4)) return wizh;
	else if (quickcmp(cid, id_SPOT, 4)) return spot;
	else if (quickcmp(cid, id_WIZD, 4)) return wizd;
	else if (quickcmp(cid, id_MULT, 4)) return mult;
	else if (quickcmp(cid, id_DEFA, 4)) return defa;
	else if (quickcmp(cid, id_RMAP, 4)) return rmap;
	else if (quickcmp(cid, id_CUSE, 4)) return cuse;
	else if (quickcmp(cid, id_LOFF, 4)) return loff;
	else if (quickcmp(cid, id_ROOM, 4)) return room;

	else if (quickcmp(cid, id_IMxx, 2)) return imxx;
	else if (quickcmp(cid, id_ZPxx, 2)) return zpxx;

	else return chunk_unknown;
}

std::string safe_read_cstring(RipUtil::MembufStream& stream, int len)
{
	char* st = new char[len + 1];
	std::memset(st, 0, len + 1);
	stream.read(st, len);
	std::string outstr(st);
	delete[] st;
	return outstr;
}

void read_sputm_chunkhead(RipUtil::MembufStream& stream, SputmChunkHead& chunkhd)
{
	chunkhd.address = stream.tellg();
	chunkhd.name = safe_read_cstring(stream, 4);
	chunkhd.size = stream.read_int(4);
	chunkhd.type = getchunktype(chunkhd.name);
}

void read_sputm_chunk(RipUtil::MembufStream& stream, SputmChunk& chunk)
{
	read_sputm_chunkhead(stream, chunk);
	chunk.resize(chunk.size);
	stream.seekg(chunk.address);
	stream.read(chunk.data, chunk.datasize);
}

bool read_chunk_if_exists(RipUtil::MembufStream& stream, SputmChunk& dest,
		const ChunkType type)
{
	// rewrite rewrite rewrite rewrite
	if (!stream.eof())
	{
		SputmChunkHead checker;
		read_sputm_chunkhead(stream, checker);
		if (checker.type == type)
		{
			stream.seekg(checker.address);
			read_sputm_chunk(stream, dest);
			stream.seekg(checker.nextaddr());
			return true;
		}
		else
		{
			stream.seekg(checker.address);
			return false;
		}
	}
	else
	{
		return false;
	}
}



void read_lflf(RipUtil::MembufStream& stream, LFLFChunk& lflfc)
{
	logger.qprint("\tinitiating LFLF read at " + to_string(stream.tellg()));

	read_sputm_chunkhead(stream, lflfc);

	SputmChunkHead hdcheck;
	while (stream.tellg() < lflfc.nextaddr())
	{
		read_sputm_chunkhead(stream, hdcheck);
		stream.seekg(hdcheck.address);

		bool skiptonext = true;	// if true after read, skip to next chunk

		switch (hdcheck.type)
		{

		case rmim:
		{
			read_rmim(stream, lflfc.rmim_chunk);
			break;
		}

		// ignore ROOM and RMDA and handle their subchunks on subsequent passes
		case room: case rmda:
		{
			skiptonext = false;
			stream.seekg(hdcheck.address + 8);
			break;
		}

		case rmhd:
		{
			read_rmhd(stream, lflfc.rmhd_chunk);
			break;
		}

		case cycl:
		{
			read_cycl(stream, lflfc.cycl_chunk);
			break;
		}

		case trns:
		{
			read_trns(stream, lflfc.trns_chunk);
			break;
		}

		case pals:
		{
			read_pals(stream, lflfc.apals);
			break;
		}

		case remp:
		{
			read_remp(stream, lflfc.remp_chunk);
			break;
		}

		case obim:
		{
			OBIMChunk obimc;
			read_obim(stream, obimc);
			lflfc.obim_chunks[obimc.imhd_chunk.id] = obimc;
			break;
		}

		case obcd:
		{
			OBCDChunk obcdc;
			read_obcd(stream, obcdc);
			lflfc.obcd_chunks[obcdc.cdhd_chunk.id] = obcdc;
			break;
		}

		case excd:
		{
			read_sputm_chunk(stream, lflfc.excd_chunk);
			break;
		}

		case encd:
		{
			read_sputm_chunk(stream, lflfc.encd_chunk);
			break;
		}

		case nlsc:
		{
			read_nlsc(stream, lflfc.nlsc_chunk);
			break;
		}

		case lscr:
		{
			SputmChunk lscrc;
			read_sputm_chunk(stream, lscrc);
			lflfc.lscr_chunks.push_back(lscrc);
			break;
		}

		case lsc2:
		{
			SputmChunk lsc2c;
			read_sputm_chunk(stream, lsc2c);
			lflfc.lsc2_chunks.push_back(lsc2c);
			break;
		}

		case boxd:
		{
			read_sputm_chunk(stream, lflfc.boxd_chunk);
			break;
		}

		case boxm:
		{
			read_sputm_chunk(stream, lflfc.boxm_chunk);
			break;
		}

		case scal:
		{
			read_sputm_chunk(stream, lflfc.scal_chunk);
			break;
		}

		case pold:
		{
			read_sputm_chunk(stream, lflfc.pold_chunk);
			break;
		}

		case scrp:
		{
			SputmChunk scrpc;
			read_sputm_chunk(stream, scrpc);
			lflfc.scrp_chunks.push_back(scrpc);
			break;
		}

		case soun:
		{
			stream.seekg(hdcheck.address + 8);
			SputmChunkHead souncheck;
			read_sputm_chunkhead(stream, souncheck);
			stream.seekg(hdcheck.address + 8);
			
			// if SOUN type is MIDI, we need to rip here
			if (souncheck.type == midi)
			{
				SputmChunk midic;
				read_sputm_chunk(stream, midic);
				lflfc.midi_chunks.push_back(midic);
				break;
			}
			// 3DO games use this, strangely, for music file names
			else if (souncheck.type == fmus)
			{
				FMUSChunk fmusc;
				read_fmus(stream, fmusc);
				lflfc.fmus_chunks.push_back(fmusc);
				break;
			}
			else if (souncheck.type == digi
			|| souncheck.type == talk) { /* fall through */ }
			else
			{
				logger.warning("unrecognized SOUN subchunk "
					+ souncheck.name + " at " + to_string(souncheck.address)
					+ " (type " + to_string(souncheck.type) + "). "
					"This is assumed to be misheadered sample data, "
					"but may be a program error if the header appears legitimate");

				SoundChunk soundc;
				soundc.wave.resize_wave(hdcheck.size - 8);
				soundc.wave.set_channels(1);
				soundc.wave.set_sampwidth(8);
				soundc.wave.set_samprate(11025);
				soundc.wave.set_signed(DatManip::has_nosign);
				stream.read(soundc.wave.get_waveform(), soundc.wave.get_wavesize());
				lflfc.digi_chunks.push_back(soundc);
				break;
			}
		}

		case digi: case talk:
		{
			SoundChunk soundc;
			read_digi_talk(stream, soundc);
			if (soundc.type == digi)
				lflfc.digi_chunks.push_back(soundc);
			else if (soundc.type == talk)
				lflfc.talk_chunks.push_back(soundc);
			else
				logger.error("\tunknown SOUN type " + soundc.name
					+ " at " + to_string(soundc.address));
			break;
		}

		case wsou:
		{
			WSOUChunk wsouc;
			read_wsou(stream, wsouc);
			lflfc.wsou_chunks.push_back(wsouc);
			break;
		}

		case akos:
		{
			AKOSChunk akosc;
			read_akos(stream, akosc);
			lflfc.akos_chunks.push_back(akosc);
			break;
		}

		case chunk_char:
		{
			read_char(stream, lflfc.char_chunks);
			break;
		}

		case awiz:
		{
			AWIZChunk awizc;
			read_awiz(stream, awizc);
			lflfc.awiz_chunks.push_back(awizc);
			break;
		}

		case mult:
		{
			read_mult(stream, lflfc.mult_chunks);
			break;
		}

		case tlke:
		{
			read_tlke(stream, lflfc.tlke_chunks);
			break;
		}

		case chunk_unknown:
		{
			// check for known formatting errors
			// underrun in previous DIGI
			if (quickcmp(hdcheck.name.c_str() + 1, id_DIGI, 3))
			{
				stream.seekg(hdcheck.address + 1);
				skiptonext = false;
			}
			else
			{
				logger.error("\tunrecognized chunk " + hdcheck.name 
					+ " at " + to_string(hdcheck.address));
			}
			break;
		}

		case chunk_none:
		{
			logger.error("\tcould not read chunk at" + to_string(stream.tellg()));
			break;
		}

		default:
		{
			logger.error("\trecognized but invalid chunk " + hdcheck.name 
				+ " at " + to_string(hdcheck.address)
				+ " (type " + to_string(hdcheck.type) + ")");
			break;
		}

		}

		if (skiptonext)
			stream.seekg(hdcheck.nextaddr());

	}

	logger.qprint("\tended LFLF read at " + to_string(stream.tellg()));
}

void read_remp(RipUtil::MembufStream& stream, REMPChunk& rempc)
{
	read_sputm_chunkhead(stream, rempc);

	int rempsize = rempc.size - 8;
	for (int i = 0; i < rempsize; i++)
	{
		rempc.colormap.push_back(stream.read_int(1));
	}

	stream.seekg(rempc.nextaddr());
}

void read_rmim(RipUtil::MembufStream& stream, RMIMChunk& rmimc)
{
	read_sputm_chunkhead(stream, rmimc);

	// get RMIH value
	SputmChunkHead chunkhd;
	read_sputm_chunkhead(stream, chunkhd);
	rmimc.rmih_val = stream.read_int(2, DatManip::le);

	SputmChunkHead checker;
	read_sputm_chunkhead(stream, checker);
	while (checker.type == imxx)
	{
		stream.seekg(checker.address);
		IMxxChunk imxxc;
		read_imxx(stream, imxxc);
		rmimc.images.push_back(imxxc);
		read_sputm_chunkhead(stream, checker);
	}

	// return stream to start of next chunk
	stream.seekg(checker.address);
}

void read_rmhd(RipUtil::MembufStream& stream, RMHDChunk& rmhdc)
{
	read_sputm_chunkhead(stream, rmhdc);
	rmhdc.width = stream.read_int(2, DatManip::le);
	rmhdc.height = stream.read_int(2, DatManip::le);
	rmhdc.objects = stream.read_int(2, DatManip::le);
}

void read_wrap(RipUtil::MembufStream& stream, WRAPChunk& wrapc)
{
	read_sputm_chunkhead(stream, wrapc);
	read_offs(stream, wrapc.offs_chunk);
}

void read_offs(RipUtil::MembufStream& stream, OFFSChunk& offsc)
{
	read_sputm_chunkhead(stream, offsc);
	int numoffs = (offsc.size - 8)/4;
	// read offset table
	for (int i = 0; i < numoffs; i++)
	{
		offsc.offsets.push_back(stream.read_int(4, DatManip::le));
	}
}

void read_imxx(RipUtil::MembufStream& stream, IMxxChunk& imxxc)
{
	read_sputm_chunkhead(stream, imxxc);

	// get the integer value of the image number from the name string
	imxxc.number = to_int(imxxc.name.c_str() + 2, 2);
	
	// copy the contained BMAP/SMAP
	read_sputm_chunk(stream, imxxc.image_chunk);
	
	// copy contained ZPxx/TMSK chunks
	SputmChunkHead hdcheck;
	while (stream.tellg() < imxxc.nextaddr())
	{
		read_sputm_chunkhead(stream, hdcheck);
		stream.seekg(hdcheck.address);

		switch (hdcheck.type)
		{
		case zpxx:
		{
			SputmChunk zpxxc;
			read_sputm_chunk(stream, zpxxc);
			imxxc.zp_chunks.push_back(zpxxc);
			break;
		}
		case tmsk:
		{
			SputmChunk tmskc;
			read_sputm_chunk(stream, tmskc);
			imxxc.tmsk_chunks.push_back(tmskc);
			break;
		}
		case chunk_unknown:
		{
			logger.error("\tunknown IMxx subchunk " + hdcheck.name
				+ " at " + to_string(hdcheck.address));
			break;
		}
		case chunk_none:
		{
			logger.error("\tcould not read IMxx subchunk at " +  to_string(stream.tellg()));
			break;
		}
		default:
		{
			logger.error("\trecognized but invalid IMxx subchunk " + hdcheck.name 
				+ " at " + to_string(hdcheck.address)
				+ " (type " + to_string(hdcheck.type) + ")");
			break;
		}
		};
	}

	// return stream to start of next chunk
	stream.seekg(imxxc.nextaddr());
}

void read_apal_rgbs(RipUtil::MembufStream& stream, SputmChunk& sputc,
	RipUtil::BitmapPalette& pal)
{
	read_sputm_chunkhead(stream, sputc);
	read_palette(stream, sputc, pal);
}

void read_nlsc(RipUtil::MembufStream& stream, NLSCChunk& nlscc)
{
	read_sputm_chunkhead(stream, nlscc);
	nlscc.nlsc_val = stream.read_int(2, DatManip::le);
	stream.seekg(nlscc.nextaddr());
}

void read_tlke(RipUtil::MembufStream& stream, std::vector<TLKEChunk>& tlke_chunks)
{
	TLKEChunk tlkec;
	read_sputm_chunkhead(stream, tlkec);

	SputmChunkHead hdcheck;
	while (stream.tellg() < tlkec.nextaddr())
	{
		read_sputm_chunkhead(stream, hdcheck);
		stream.seekg(hdcheck.address);

		switch(hdcheck.type)
		{
		case text:
			read_text(stream, tlkec.text_chunk);
			break;
		case chunk_unknown:
			logger.error("\tunknown TLKE subchunk " + hdcheck.name
				+ " at " + to_string(hdcheck.address));
			break;
		case chunk_none:
			logger.error("\tcould not read TLKE subchunk at " +  to_string(stream.tellg()));
			break;
		default:
			logger.error("\trecognized but invalid TLKE subchunk " + hdcheck.name 
				+ " at " + to_string(hdcheck.address)
				+ " (type " + to_string(hdcheck.type) + ")");
			break;
		}
		stream.seekg(hdcheck.nextaddr());
	}
	tlke_chunks.push_back(tlkec);
	stream.seekg(tlkec.nextaddr());
}

void read_text(RipUtil::MembufStream& stream, TEXTChunk& textc)
{
	read_sputm_chunkhead(stream, textc);
	textc.text = safe_read_cstring(stream, textc.size - 8);
	stream.seekg(textc.nextaddr());
}


void read_rgbs(RipUtil::MembufStream& stream, RipUtil::BitmapPalette& palette)
{
	SputmChunkHead rgbsc;
	read_sputm_chunkhead(stream, rgbsc);

	int numcolors = (rgbsc.size - 8)/3;
	for (int i = 0; i < numcolors; i++)
	{
		palette[i] = read_color(stream);
	}
	stream.seekg(rgbsc.nextaddr());
}

int read_color(RipUtil::MembufStream & stream)
{
	int color = 0;
	color |= (stream.read_int(1));
	color |= (stream.read_int(1) << 8);
	color |= (stream.read_int(1) << 16);
	return color;
}

void read_palette(RipUtil::MembufStream& stream, const SputmChunkHead& palcontainer,
	RipUtil::BitmapPalette& pal)
{
	stream.seekg(palcontainer.address + 8);
	int entries = (palcontainer.size - 8)/3;
	for (int i = 0; i < entries; i++)
	{
		int r = stream.read_int(1);
		int g = stream.read_int(1);
		int b = stream.read_int(1);
		int color = r;
		color |= (g << 8);
		color |= (b << 16);
		pal[i] = color;
	}
}



void read_obim(RipUtil::MembufStream& stream, OBIMChunk& obimc)
{
	read_sputm_chunkhead(stream, obimc);
	read_imhd(stream, obimc.imhd_chunk);
	stream.seekg(obimc.imhd_chunk.nextaddr());

	SputmChunkHead checker;
	read_sputm_chunkhead(stream, checker);
	while (checker.type == imxx)
	{
		stream.seekg(checker.address);
		IMxxChunk imxxc;
		read_imxx(stream, imxxc);
		obimc.images.push_back(imxxc);
		read_sputm_chunkhead(stream, checker);
	}
	stream.seekg(checker.address);
}

void read_imhd(RipUtil::MembufStream& stream, IMHDChunk& imhdc)
{
	// is there any other useful information here?
	read_sputm_chunkhead(stream, imhdc);
	imhdc.id = stream.read_int(2, DatManip::le);
}

void read_obcd(RipUtil::MembufStream& stream, OBCDChunk& obcdc)
{
	read_sputm_chunkhead(stream, obcdc);
	// CDHD
	read_cdhd(stream, obcdc.cdhd_chunk);
	stream.seekg(obcdc.cdhd_chunk.nextaddr());
	// VERB
	read_sputm_chunk(stream, obcdc.verb_chunk);
	stream.seekg(obcdc.verb_chunk.nextaddr());
	// OBNA
	SputmChunk obna_chunk;
	read_sputm_chunkhead(stream, obna_chunk);
	int obnalen = obna_chunk.size - 8;
	char* obna_str = new char[obnalen + 1];
	// add an extra null in case there isn't one in the string
	obna_str[obnalen] = 0;
	stream.read(obna_str, obnalen);
	obcdc.obna_val = obna_str;
	delete[] obna_str;
}

void read_cdhd(RipUtil::MembufStream& stream, CDHDChunk& cdhdc)
{
	read_sputm_chunkhead(stream, cdhdc);
	cdhdc.id = stream.read_int(2, DatManip::le);
	cdhdc.x = stream.read_int(2, DatManip::le);
	cdhdc.y = stream.read_int(2, DatManip::le);
	cdhdc.width = stream.read_int(2, DatManip::le);
	cdhdc.height = stream.read_int(2, DatManip::le);
}

int read_obims_map(RipUtil::MembufStream& stream, std::map<ObjectID, OBIMChunk>& obimm)
{
	SputmChunkHead checker;
	read_sputm_chunkhead(stream, checker);
	int numread = 0;
	while (checker.type == obim)
	{
		stream.seekg(checker.address);
		OBIMChunk obimc;
		read_obim(stream, obimc);
		obimm[obimc.imhd_chunk.id] = obimc;
		++numread;
		stream.seekg(checker.nextaddr());
		read_sputm_chunkhead(stream, checker);
	}
	stream.seekg(checker.address);
	return numread;
}

int read_obcds_map(RipUtil::MembufStream& stream, std::map<ObjectID, OBCDChunk>& obcdm)
{
	SputmChunkHead checker;
	read_sputm_chunkhead(stream, checker);
	int numread = 0;
	while (checker.type == obcd)
	{
		stream.seekg(checker.address);
		OBCDChunk obcdc;
		read_obcd(stream, obcdc);
		obcdm[obcdc.cdhd_chunk.id] = obcdc;
		++numread;
		stream.seekg(checker.nextaddr());
		read_sputm_chunkhead(stream, checker);
	}
	stream.seekg(checker.address);
	return numread;
}



void read_pals(RipUtil::MembufStream& stream, std::vector<RipUtil::BitmapPalette>& apals)
{
	SputmChunkHead palsc;
	read_sputm_chunkhead(stream, palsc);
	WRAPChunk wrapc;
	read_wrap(stream, wrapc);
	for (std::vector<int>::size_type i = 0;
		i < wrapc.offs_chunk.offsets.size(); i++)
	{
		int offset = wrapc.offs_chunk.offsets[i];
		stream.seekg(wrapc.offs_chunk.address + offset);
		SputmChunk apalc;
		BitmapPalette pal;
		read_apal_rgbs(stream, apalc, pal);
		apals.push_back(pal);
	}
	stream.seekg(palsc.nextaddr());
}

void read_cycl(RipUtil::MembufStream& stream, CYCLChunk& cyclc)
{
	read_sputm_chunkhead(stream, cyclc);
	cyclc.cycl_val = stream.read_int(2, DatManip::le);
	stream.seekg(cyclc.nextaddr());
}

void read_trns(RipUtil::MembufStream& stream, TRNSChunk& trnsc)
{
	read_sputm_chunkhead(stream, trnsc);
	trnsc.trns_val = stream.read_int(2, DatManip::le);
	stream.seekg(trnsc.nextaddr());
}



void read_soun(RipUtil::MembufStream& stream, SoundChunk& soundc)
{
	SputmChunkHead sounc;
	read_sputm_chunkhead(stream, sounc);
	read_digi_talk(stream, soundc);
	stream.seekg(soundc.nextaddr());
}

void read_digi_talk(RipUtil::MembufStream& stream, SoundChunk& soundc)
{
	read_sputm_chunkhead(stream, soundc);
	
	SputmChunkHead hdcheck;
	while (stream.tellg() < soundc.nextaddr())
	{
		bool skiptonext = true;

		read_sputm_chunkhead(stream, hdcheck);
		stream.seekg(hdcheck.address);

		switch (hdcheck.type)
		{
		case hshd:
			read_hshd(stream, soundc.hshd_chunk);
			soundc.wave.set_channels(1);
			soundc.wave.set_signed(DatManip::has_nosign);
			soundc.wave.set_sampwidth(8);
			soundc.wave.set_samprate(soundc.hshd_chunk.samplerate);
			break;
		case sdat:
		{
			// spymustard has this unique and very abnormal chunk placement
			stream.seekg(hdcheck.address + 8);
			SputmChunkHead srfs_check;
			read_sputm_chunkhead(stream, srfs_check);
			if (srfs_check.type == srfs)
			{
				// subtract size of SRFS chunk
				soundc.wave.resize_wave(hdcheck.size - 40);
				stream.seekg(hdcheck.address + 40);
			}
			else
			{
				soundc.wave.resize_wave(hdcheck.size - 8);
				stream.seekg(hdcheck.address + 8);
			}

			stream.read(soundc.wave.get_waveform(), soundc.wave.get_wavesize());
			break;
		}
		case sbng:
			break;
		case chunk_unknown:
			logger.warning("encountered unknown DIGI or TALK subchunk " 
				+ hdcheck.name + " at " + to_string(hdcheck.address) + ". "
				"This is assumed to be misheadered sample data, " 
				"but may be a program error if the header appears legitimate");

			if (soundc.nextaddr() - hdcheck.address - 8 < 0)
			{
				logger.print("\tchunksize invalid: skipping to next known chunk");
				stream.seekg(soundc.nextaddr());
				skiptonext = false;
				break;
			}

			soundc.wave.resize_wave(soundc.nextaddr() - hdcheck.address - 8);
			stream.seekg(hdcheck.address + 8);
			stream.read(soundc.wave.get_waveform(), soundc.wave.get_wavesize());
			break;
		case pete:	// only used once in fbear??
		{
			logger.qprint("encountered a PETE chunk at " + to_string(hdcheck.address)
				+ ", concatenating to existing sample data");
			// since this has more sample data and we can't add a new entry from here,
			// we add it to the end of the existing entry
			int startpos = soundc.wave.get_wavesize();
			soundc.wave.resize_and_copy_wave(soundc.wave.get_wavesize()
				+ hdcheck.size - 8);
			stream.seekg(hdcheck.address + 8);
			stream.read(soundc.wave.get_waveform() + startpos,
				hdcheck.size - 8);
			break;
		}
		case chunk_none:
			logger.error("\tcould not read DIGI or TALK subchunk at " 
				+ to_string(stream.tellg()));
			break;
		default:
			logger.error("\trecognized but invalid DIGI or TALK subchunk " 
				+ hdcheck.name + " at " + to_string(hdcheck.address)
				+ " (type " + to_string(hdcheck.type) + ")");
			break;
		}

		if (skiptonext)
			stream.seekg(hdcheck.nextaddr());
	}

	stream.seekg(soundc.nextaddr());
}

void read_hshd(RipUtil::MembufStream& stream, HSHDChunk& hshdc)
{
	read_sputm_chunkhead(stream, hshdc);
	hshdc.unknown1 = stream.read_int(2, DatManip::le);
	hshdc.unknown2 = stream.read_int(2, DatManip::le);
	hshdc.unknown3 = stream.read_int(2, DatManip::le);
	hshdc.samplerate = stream.read_int(2, DatManip::le);
	hshdc.unknown4 = stream.read_int(2, DatManip::le);
	hshdc.unknown5 = stream.read_int(2, DatManip::le);
	hshdc.unknown6 = stream.read_int(2, DatManip::le);
	hshdc.unknown7 = stream.read_int(2, DatManip::le);
	stream.seekg(hshdc.nextaddr());
}

void read_fmus(RipUtil::MembufStream& stream, FMUSChunk& fmusc)
{
	read_sputm_chunkhead(stream, fmusc);

	SputmChunkHead hdcheck;
	while (stream.tellg() < fmusc.nextaddr())
	{
		read_sputm_chunkhead(stream, hdcheck);
		stream.seekg(hdcheck.address);
		bool seektonext = true;

		switch(hdcheck.type)
		{
		case hshd:
			read_hshd(stream, fmusc.hshd_chunk);
			break;
		case sdat:
			read_fmussdat(stream, fmusc.fmussdat_chunk);
			break;
		case sbng:
			break;
		case chunk_unknown:
			logger.error("\tunknown FMUS subchunk " + hdcheck.name
				+ " at " + to_string(hdcheck.address));
			break;
		case chunk_none:
			logger.error("\tcould not read FMUS subchunk at " +  to_string(stream.tellg()));
			break;
		default:
			logger.error("\trecognized but invalid FMUS subchunk " + hdcheck.name 
				+ " at " + to_string(hdcheck.address)
				+ " (type " + to_string(hdcheck.type) + ")");
			break;
		}

		if (seektonext)
			stream.seekg(hdcheck.nextaddr());
	}
	stream.seekg(fmusc.nextaddr());
}

void read_fmussdat(RipUtil::MembufStream& stream, FMUSSDATChunk& fmussdatc)
{
	read_sputm_chunkhead(stream, fmussdatc);
	fmussdatc.filestring = safe_read_cstring(stream, fmussdatc.size - 8);
	stream.seekg(fmussdatc.nextaddr());
}

void read_wsou(RipUtil::MembufStream& stream, WSOUChunk& wsouc)
{
	read_sputm_chunkhead(stream, wsouc);
	
	SputmChunkHead hdcheck;
	while (stream.tellg() < wsouc.nextaddr())
	{
		bool seektonext = true;
		read_sputm_chunkhead(stream, hdcheck);
		stream.seekg(hdcheck.address);

		switch (hdcheck.type)
		{
		case riff:
			read_riff(stream, wsouc.riff_entry);
			// this is technically not a canonical SputmChunk; the
			// size field is little-endian and non-header-inclusive,
			// so we trust read_riff() to get us to the next chunk
			seektonext = false;
			break;
		case chunk_unknown:
			logger.error("\tunknown WSOU subchunk " + hdcheck.name
				+ " at " + to_string(hdcheck.address));
			break;
		case chunk_none:
			logger.error("\tcould not read WSOU subchunk at " +  to_string(stream.tellg()));
			break;
		default:
			logger.error("\trecognized but invalid WSOU subchunk " + hdcheck.name 
				+ " at " + to_string(hdcheck.address)
				+ " (type " + to_string(hdcheck.type) + ")");
			break;
		}

		if (seektonext)
			stream.seekg(hdcheck.nextaddr());
	}
	stream.seekg(wsouc.nextaddr());
}

void read_riff(RipUtil::MembufStream& stream, RIFFEntry& riff_entry)
{
	int datstart = stream.tellg();
	stream.seekg(datstart + 4);
	int datlen = stream.read_int(4, DatManip::le) + 8;
	stream.seekg(datstart);

	riff_entry.resize(datlen);
	stream.read(riff_entry.riffdat, riff_entry.riffdat_size);
	stream.seekg(datstart + datlen);
}



void read_akos(RipUtil::MembufStream& stream, AKOSChunk& akosc)
{
	read_sputm_chunkhead(stream, akosc);
	
	// should move EOF check into read_sputm_chunkhead()?
	while (!stream.eof() && stream.tellg() < akosc.nextaddr())
	{
		SputmChunkHead hdcheck;
		read_sputm_chunkhead(stream, hdcheck);
		stream.seekg(hdcheck.address);

		bool seektonext = true;

		switch (hdcheck.type)
		{
		case akhd:	// header
			read_akhd(stream, akosc.akhd_chunk);
			break;
		case akpl:	// compressed palette indices
			read_akpl(stream, akosc.akpl_chunk);
			akosc.colormap = akosc.akpl_chunk.colormap;
			akosc.numcolors = akosc.akpl_chunk.numcolors;
			break;
		case rgbs:	// palette RGB data
			// if size is exactly 12, alternate compression used
			if (hdcheck.size == 12)
				akosc.palette[0] = 0;
			else
				read_palette(stream, hdcheck, akosc.palette);
			break;
		case aksq:	// ?
			read_aksq(stream, akosc.aksq_chunk);
			break;
		case akfo:	// ? old chunk type?
			read_sputm_chunk(stream, akosc.akfo_chunk);
			break;
		case akch:
			read_akch(stream, akosc.akch_chunk, akosc.akhd_chunk.anislots,
				akosc.akhd_chunk.sequence_encoding);
			break;
		case akof:	// image data/header offsets
			read_akof(stream, hdcheck, akosc.akof_entries);
			break;
		case akci:
			read_akci(stream, hdcheck, akosc.akof_entries, akosc.akci_entries);
			break;
		case akcd:
			read_akcd(stream, hdcheck, akosc.akof_entries, akosc.akci_entries,
				akosc.akcd_entries);
			break;
		case akax:
			read_akax(stream, akosc.akax_chunk);
			break;
		case aklc:
			read_sputm_chunk(stream, akosc.aklc_chunk);
			break;
		case akst:
			read_sputm_chunk(stream, akosc.akst_chunk);
			break;
		case akct:
			read_sputm_chunk(stream, akosc.akct_chunk);
			break;
		case sp2c:
			stream.seekg(hdcheck.address + 8);
			akosc.file_date = safe_read_cstring(stream, hdcheck.size - 8);
			break;
		case splf:
			stream.seekg(hdcheck.address + 8);
			akosc.file_name = safe_read_cstring(stream, hdcheck.size - 8);
			break;
		case clrs:
			stream.seekg(hdcheck.address + 8);
			akosc.file_compr = safe_read_cstring(stream, hdcheck.size - 8);
			break;
		case imgl:
			read_sputm_chunk(stream, akosc.imgl_chunk);
			break;
		case sqdb:
			read_sqdb(stream, akosc.sqdb_chunk);
			break;
		case chunk_unknown:
			logger.error("\tunknown AKOS subchunk " + hdcheck.name
				+ " at " + to_string(hdcheck.address));
			break;
		case chunk_none:
			logger.error("\tcould not read AKOS subchunk at " +  to_string(stream.tellg()));
			break;
		default:
			logger.error("\trecognized but invalid AKOS subchunk " + hdcheck.name 
				+ " at " + to_string(hdcheck.address)
				+ " (type " + to_string(hdcheck.type) + ")");
			break;
		}

		if (seektonext)
			stream.seekg(hdcheck.nextaddr());
	}

	stream.seekg(akosc.nextaddr());
}

void read_akhd(RipUtil::MembufStream& stream, AKHDChunk& akhdc)
{
	read_sputm_chunkhead(stream, akhdc);

	akhdc.unknown1 = stream.read_int(2, DatManip::le);
	akhdc.sequence_encoding = stream.read_int(2, DatManip::le);
	akhdc.anislots = stream.read_int(2, DatManip::le);
	akhdc.unknown3 = stream.read_int(2, DatManip::le);
	akhdc.encoding = stream.read_int(2, DatManip::le);
	akhdc.unknown5 = stream.read_int(2, DatManip::le);

	stream.seekg(akhdc.nextaddr());
}

void read_sqdb(RipUtil::MembufStream& stream, SQDBChunk& sqdbc)
{
	read_sputm_chunkhead(stream, sqdbc);

	WRAPChunk wrapc;
	read_wrap(stream, wrapc);

	std::vector<int>& offss = wrapc.offs_chunk.offsets;
	for (std::vector<int>::size_type i = 0;
		i < offss.size(); i++)
	{
		stream.seekg(wrapc.offs_chunk.nextaddr() + offss[i]);
		SEQIChunk seqic;
		read_seqi(stream, seqic);
		sqdbc.seqi_chunks.push_back(seqic);
	}
	stream.seekg(sqdbc.nextaddr());
}

void read_akcd(RipUtil::MembufStream& stream, const SputmChunkHead& akcdc, 
	std::vector<AKOFEntry>& akof_entries, std::vector <AKCIEntry>& akci_entries,
	std::vector<AKCDEntry>& akcd_entries)
{
	for (std::vector<AKOFEntry>::size_type i = 0;
		i < akof_entries.size(); i++)
	{
		stream.seekg(akcdc.address + 8 + akof_entries[i].akcd_offset);

		AKCDEntry akcde;

		if (i < akof_entries.size() - 1)
			akcde.resize(akof_entries[i + 1].akcd_offset - akof_entries[i].akcd_offset);
		else
			akcde.resize(akcdc.nextaddr() - akcdc.address - akof_entries[i].akcd_offset - 8);

		stream.read(akcde.imgdat, akcde.size);
		akcde.width = akci_entries[i].width;
		akcde.height = akci_entries[i].height;

		akcd_entries.push_back(akcde);
	}
	stream.seekg(akcdc.nextaddr());
}

void read_axfd(RipUtil::MembufStream& stream, AXFDChunk& axfdc)
{
	read_sputm_chunkhead(stream, axfdc);

	if (axfdc.size > 0xA)	// some AXFD chunks are empty
	{
		axfdc.unknown = stream.read_int(2, DatManip::le);
		axfdc.off1 = to_signed(stream.read_int(2, DatManip::le), 16);
		axfdc.off2 = to_signed(stream.read_int(2, DatManip::le), 16);
		axfdc.width = stream.read_int(2, DatManip::le);
		axfdc.height = stream.read_int(2, DatManip::le);

		axfdc.resize(axfdc.size - 0x12);
		stream.read(axfdc.imgdat, axfdc.imgdat_size);
	}

	stream.seekg(axfdc.nextaddr());
}

void read_auxd(RipUtil::MembufStream& stream, AUXDChunk& auxdc)
{
	read_sputm_chunkhead(stream, auxdc);

	read_axfd(stream, auxdc.axfd_chunk);

	read_sputm_chunk(stream, auxdc.axur_chunk);

	read_sputm_chunk(stream, auxdc.axer_chunk);

	stream.seekg(auxdc.nextaddr());
}

void read_akax(RipUtil::MembufStream& stream, AKAXChunk& akaxc)
{
	read_sputm_chunkhead(stream, akaxc);

	WRAPChunk wrapc;
	read_wrap(stream, wrapc);

	for (std::vector<int>::size_type i = 0;
		i < wrapc.offs_chunk.offsets.size(); i++)
	{
		stream.seekg(wrapc.offs_chunk.address + wrapc.offs_chunk.offsets[i]);

		AUXDChunk auxdc;
		read_auxd(stream, auxdc);
		akaxc.auxd_chunks.push_back(auxdc);
	}

	stream.seekg(akaxc.nextaddr());
}

void read_akci(RipUtil::MembufStream& stream, const SputmChunkHead& akcic,
	const std::vector<AKOFEntry>& akof_entries, std::vector<AKCIEntry>& akci_entries)
{
	for (std::vector<AKOFEntry>::size_type i = 0;
		i < akof_entries.size(); i++)
	{
		stream.seekg(akcic.address + 8 + akof_entries[i].akci_offset);

		AKCIEntry entry;
		entry.width = stream.read_int(2, DatManip::le);
		entry.height = stream.read_int(2, DatManip::le);

		akci_entries.push_back(entry);
	}
	stream.seekg(akcic.nextaddr());
}


void read_akpl(RipUtil::MembufStream& stream, AKPLChunk& akplc)
{
	read_sputm_chunk(stream, akplc);
	akplc.numcolors = akplc.size - 8;

	// for nominally 2-color compression, this is the transparency index
	if (akplc.numcolors == 2)
	{
		akplc.hasalttrans = true;
		stream.seekg(akplc.address + 8);
		akplc.alttrans = stream.read_int(2, DatManip::le);
	}
	// otherwise, this is a list of index mappings or a placeholder
	else
	{
		for (int i = 0; i < akplc.numcolors; i++)
		{
			akplc.colormap.push_back(to_int(akplc.data + 8 + i, 1));
		}
		// 1 entry of 0xFF = 256 color compression
		if (akplc.numcolors == 1)
			akplc.numcolors = 256;
	}
	stream.seekg(akplc.nextaddr());
}

void read_akof(RipUtil::MembufStream& stream, const SputmChunkHead& akofc, 
	std::vector<AKOFEntry>& akof_entries)
{
	stream.seekg(akofc.address + 8);
	int num_akofentries = (akofc.size - 8)/6;
	for (int i = 0; i < num_akofentries; i++)
	{
		AKOFEntry entry;
		entry.akcd_offset = stream.read_int(4, DatManip::le);
		entry.akci_offset = stream.read_int(2, DatManip::le);
		akof_entries.push_back(entry);
	}
	stream.seekg(akofc.nextaddr());
}

void read_seqi(RipUtil::MembufStream& stream, SEQIChunk& seqic)
{
	read_sputm_chunkhead(stream, seqic);
	SputmChunkHead hdcheck;
	
	while (stream.tellg() < seqic.nextaddr())
	{
		read_sputm_chunkhead(stream, hdcheck);
		stream.seekg(hdcheck.address);

		switch(hdcheck.type)
		{
		case name:
			stream.seekg(hdcheck.address + 8);
			seqic.name_val = safe_read_cstring(stream, hdcheck.size - 8);
			break;
		case stof:
			read_sputm_chunk(stream, seqic.stof_chunk);
			break;
		case sqlc:
			read_sputm_chunk(stream, seqic.sqlc_chunk);
			break;
		case size:
			read_sputm_chunk(stream, seqic.size_chunk);
			break;
		case chunk_unknown:
			logger.error("\tunknown SEQI subchunk " + hdcheck.name
				+ " at " + to_string(hdcheck.address));
			break;
		case chunk_none:
			logger.error("\tcould not read SEQI subchunk at " 
				+ to_string(stream.tellg()));
			break;
		default:
			logger.error("\trecognized but invalid SEQI subchunk " + hdcheck.name 
				+ " at " + to_string(hdcheck.address)
				+ " (type " + to_string(hdcheck.type) + ")");
			break;
		}
		stream.seekg(hdcheck.nextaddr());
	}
	stream.seekg(seqic.nextaddr());
}

void read_akch(RipUtil::MembufStream& stream, AKCHChunk& akchc,
	int pointer_table_1_size, int pointer_table_2_encoding)
{
	read_sputm_chunkhead(stream, akchc);

	// read pointer table 1 (pointers within this chunk)
	for (int i = 0; i < pointer_table_1_size; i++)
	{
		akchc.akch_pointers.push_back(stream.read_int(2, DatManip::le));
	}

	// read pointer table 2 (pointers within AKSQ chunk)
	//
	// Each entry in this table starts with a null byte, followed
	// by some sort of type code and a series of pointers each with
	// a 1-byte "label".
	// For some reason, the entire table is sometimes (irregularly)
	// terminated by another null byte, which makes the end-of-table
	// check more difficult.

	// discard leading null
	stream.read_int(1);

	if (pointer_table_2_encoding == 0x8000)
	{
		while (stream.tellg() < akchc.nextaddr())
		{
			AKCHPointerTable2Entry entry;

			// read type
			entry.type = stream.read_int(1);

			// read pointer list
			int pcode = stream.read_int(1);
			// 0 = terminator, but we also have to do a range check
			// because the final entry is not always 0-terminated
			while (pcode != 0)
			{
				AKCHPointerTable2EntryAKSQPointer pointerentry;
				pointerentry.id = pcode;
				pointerentry.pointer = stream.read_int(4, DatManip::le);

				entry.pointerentries.push_back(pointerentry);

				// if out of range, "manually" terminate
				if (!(stream.tellg() < akchc.nextaddr()))
				{
					pcode = 0;
				}
				// otherwise, read the next pointer label
				else
				{
					pcode = stream.read_int(1);
				}
			}

			// add pointer entry
			akchc.aksq_pointers.push_back(entry);

		}
	}
	else
	{
		logger.warning("Unrecognized AKCH encoding " + to_string(pointer_table_2_encoding));
//		int t2size = akchc.nextaddr() - stream.tellg();
//		akchc.table2_raw = new char[t2size];
//		akchc.table2_raw_size = t2size;
//		stream.read(akchc.table2_raw, t2size);
	}

	stream.seekg(akchc.nextaddr());
}

void read_aksq(RipUtil::MembufStream& stream, AKSQChunk& aksqc)
{
	read_sputm_chunk(stream, aksqc);

	stream.seekg(aksqc.nextaddr());
}



void read_awiz(RipUtil::MembufStream& stream, AWIZChunk& awizc)
{
	read_sputm_chunkhead(stream, awizc);
	while (stream.tellg() < awizc.nextaddr())
	{
		SputmChunkHead hdcheck;
		read_sputm_chunkhead(stream, hdcheck);
		stream.seekg(hdcheck.address);
		switch(hdcheck.type)
		{
		case xmap:
			read_sputm_chunk(stream, awizc.xmap_chunk);
			break;
		case cnvs:
			read_sputm_chunk(stream, awizc.cnvs_chunk);
			break;
		case relo:
			read_sputm_chunk(stream, awizc.relo_chunk);
			break;
		case wizh:
			read_sputm_chunk(stream, awizc.wizh_chunk);
			awizc.unknown = to_int(awizc.wizh_chunk.data + 8, 4, DatManip::le);
			awizc.width = to_int(awizc.wizh_chunk.data + 12, 4, DatManip::le);
			awizc.height = to_int(awizc.wizh_chunk.data + 16, 4, DatManip::le);
			break;
		case spot:
			read_sputm_chunk(stream, awizc.spot_chunk);
			break;
		case wizd:
			read_sputm_chunk(stream, awizc.wizd_chunk);
			break;
		case trns:
			read_trns(stream, awizc.trns_chunk);
			break;
		case rgbs:
			read_palette(stream, hdcheck, awizc.palette);
			break;
		case rmap:
		{
			read_sputm_chunkhead(stream, awizc.rmap_chunk);
			awizc.rmap_chunk.unknown = stream.read_int(4, DatManip::le);
			int size = awizc.rmap_chunk.size - 12;
			for (int i = 0; i < size; i++)
				awizc.rmap_chunk.colormap.push_back(stream.read_int(1));
			break;
		}
		case cuse:
			read_sputm_chunk(stream, awizc.cuse_chunk);
			break;
		case chunk_unknown:
			logger.error("\tunknown AWIZ subchunk " + hdcheck.name
				+ " at " + to_string(hdcheck.address));
			break;
		case chunk_none:
			logger.error("\tcould not read AWIZ subchunk at " 
				+ to_string(stream.tellg()));
			break;
		default:
			logger.error("\trecognized but invalid AWIZ subchunk " + hdcheck.name 
				+ " at " + to_string(hdcheck.address)
				+ " (type " + to_string(hdcheck.type) + ")");
			break;
		}

		stream.seekg(hdcheck.nextaddr());
	}
}

void read_awiz_wrap(RipUtil::MembufStream& stream, std::vector<AWIZChunk>& awiz_chunks)
{
	WRAPChunk wrapc;
	read_wrap(stream, wrapc);

	for (std::vector<int>::size_type i = 0;
		i < wrapc.offs_chunk.offsets.size(); i++)
	{
		stream.seekg(wrapc.offs_chunk.address + wrapc.offs_chunk.offsets[i]);

		AWIZChunk awizc;
		read_awiz(stream, awizc);
		awiz_chunks.push_back(awizc);
	}

	stream.seekg(wrapc.nextaddr());
}

// THIS IS ALL WRONG
void read_mult(RipUtil::MembufStream& stream, std::vector<MULTChunk>& mult_chunks)
{
	MULTChunk multc;
	read_sputm_chunkhead(stream, multc);
	
	SputmChunkHead defacheck;
	read_sputm_chunkhead(stream, defacheck);

	if (defacheck.type == defa)
	{
		stream.seekg(defacheck.address);
		read_sputm_chunkhead(stream, multc.defa_chunk);

		while (stream.tellg() < multc.defa_chunk.nextaddr())
		{
			SputmChunkHead hdcheck;
			read_sputm_chunkhead(stream, hdcheck);
			stream.seekg(hdcheck.address);

			switch(hdcheck.type)
			{
			case rgbs:
				read_rgbs(stream, multc.defa_chunk.palette);
				break;
			case rmap:
			{
				read_sputm_chunkhead(stream, multc.defa_chunk.rmap_chunk);
				multc.defa_chunk.rmap_chunk.unknown = stream.read_int(4, DatManip::le);
				int size = multc.defa_chunk.rmap_chunk.size - 12;
				for (int i = 0; i < size; i++)
					multc.defa_chunk.rmap_chunk.colormap.push_back(stream.read_int(1));
				break;
			}
			case cuse:
				read_sputm_chunk(stream, multc.defa_chunk.cuse_chunk);
				break;
			case cnvs:
				read_sputm_chunk(stream, multc.defa_chunk.cnvs_chunk);
				break;
			case chunk_unknown:
				logger.error("\tunknown DEFA subchunk " + hdcheck.name
					+ " at " + to_string(hdcheck.address));
				break;
			case chunk_none:
				logger.error("\tcould not read DEFA subchunk at " 
					+ to_string(stream.tellg()));
				break;
			default:
				logger.error("\trecognized but invalid DEFA subchunk " + hdcheck.name 
					+ " at " + to_string(hdcheck.address)
					+ " (type " + to_string(hdcheck.type) + ")");
				break;
			}

			stream.seekg(hdcheck.nextaddr());
		}
		stream.seekg(multc.defa_chunk.nextaddr());
	}
	else
	{
		stream.seekg(defacheck.address);
	}
	
	while (stream.tellg() < multc.nextaddr())
	{
		SputmChunkHead hdcheck;
		read_sputm_chunkhead(stream, hdcheck);
		stream.seekg(hdcheck.address);

		switch(hdcheck.type)
		{
		case rgbs:
			read_rgbs(stream, multc.defa_chunk.palette);
			break;
		case rmap:
			read_sputm_chunkhead(stream, multc.defa_chunk.rmap_chunk);
			multc.defa_chunk.rmap_chunk.unknown = stream.read_int(4, DatManip::le);
			for (int i = 0; i < 256; i++)
				multc.defa_chunk.rmap_chunk.colormap.push_back(stream.read_int(1));
			break;
		case cuse:
			read_sputm_chunk(stream, multc.defa_chunk.cuse_chunk);
			break;
		case cnvs:
			read_sputm_chunk(stream, multc.defa_chunk.cnvs_chunk);
			break;
		case wrap:
			read_awiz_wrap(stream, multc.awiz_chunks);
			break;
		case chunk_unknown:
			logger.error("\tunknown MULT subchunk " + hdcheck.name
				+ " at " + to_string(hdcheck.address));
			break;
		case chunk_none:
			logger.error("\tcould not read MULT subchunk at " 
				+ to_string(stream.tellg()));
			break;
		default:
			logger.error("\trecognized but invalid MULT subchunk " + hdcheck.name 
				+ " at " + to_string(hdcheck.address)
				+ " (type " + to_string(hdcheck.type) + ")");
			break;
		}

		stream.seekg(hdcheck.nextaddr());
	}

	mult_chunks.push_back(multc);

	stream.seekg(multc.nextaddr());
}

void read_char(RipUtil::MembufStream& stream, std::vector<CHARChunk>& chars)
{
	CHARChunk charc;
	read_sputm_chunkhead(stream, charc);

	int dataend = stream.read_int(4, DatManip::le) - 0x1C;
	int datastart = charc.address + 0x1D;
	int unknown = stream.read_int(1, DatManip::le);
	for (int i = 0; i < 16; i++)
		charc.colormap.push_back(stream.read_int(1));
	stream.seekg(datastart);
	charc.compr = stream.read_int(1);
	charc.rowspace = stream.read_int(1);

	int numentries = stream.read_int(2, DatManip::le);
	std::vector<int> offentries;
	for (int i = 0; i < numentries; i++)
	{
		stream.seekg(datastart + (i + 1) * 4);

		int offset = stream.read_int(4, DatManip::le);
		if (offset != 0)
		{
			offentries.push_back(offset);
		}
	}

	for (std::vector<int>::size_type i = 0;
		i < offentries.size(); i++)
	{
		
		stream.seekg(datastart + offentries[i]);

		CHAREntry entry;

		entry.width = stream.read_int(1);
		entry.height = stream.read_int(1);
		entry.off1 = stream.read_int(1);
		entry.off2 = stream.read_int(1);

		int charlen;
		if (i < offentries.size() - 1)
			charlen = offentries[i + 1] - offentries[i] - 4;
		else
			charlen = charc.nextaddr() - datastart - offentries[i] - 4;
		entry.resize(charlen);
		stream.read(entry.data, charlen);
		charc.char_entries.push_back(entry);
	}

	chars.push_back(charc);

	stream.seekg(charc.nextaddr());
}



void read_songhead_type1(RipUtil::MembufStream& stream, SONGHeader& song_header)
{
	SputmChunkHead sghdc;

	// read SGHD header
	read_sputm_chunkhead(stream, sghdc);
	if (sghdc.type != sghd)
	{
		logger.error("could not find SGHD chunk at "
			+ to_string(sghdc.address));
		return;
	}

	stream.seekg(sghdc.address + 8);
	int numentries = stream.read_int(4, DatManip::le);

	for (int i = 0; i < numentries; i++)
	{
		int entrystart = stream.tellg();
		SONGEntry songe;

		songe.idnum = stream.read_int(4, DatManip::le);
		songe.address = stream.read_int(4, DatManip::le);
		songe.length = stream.read_int(4, DatManip::le);

		song_header.song_entries.push_back(songe);

		stream.seekg(entrystart + 25);
	}
}

void read_songhead_type234(RipUtil::MembufStream& stream, SONGHeader& song_header)
{
	SputmChunkHead sghdc;

	// read SGHD header
	read_sputm_chunkhead(stream, sghdc);
	if (sghdc.type != sghd)
	{
		logger.error("could not find SGHD chunk at "
			+ to_string(sghdc.address));
		return;
	}

	stream.seekg(sghdc.address + 8);
	int numentries = stream.read_int(4, DatManip::le);

	// read SGEN entries
	stream.seekg(sghdc.nextaddr());
	for (int i = 0; i < numentries; i++)
	{
		SputmChunkHead hdcheck;
		read_sputm_chunkhead(stream, hdcheck);

		if (hdcheck.type != sgen)
		{
			logger.error("could not find SGEN chunk at "
				+ to_string(hdcheck.address));
			return;
		}

		stream.seekg(hdcheck.address + 8);

		SONGEntry songe;
		songe.idnum = stream.read_int(4, DatManip::le);
		songe.address = stream.read_int(4, DatManip::le);
		songe.length = stream.read_int(4, DatManip::le);

		song_header.song_entries.push_back(songe);

		stream.seekg(hdcheck.nextaddr());
	}
}


};	// end of namespace Humongous