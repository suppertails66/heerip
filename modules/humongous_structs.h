/* Constants and data structures used to internally represent Humongous data */

#include "../utils/MembufStream.h"
#include "../utils/BitmapData.h"
#include "../RipperFormats.h"
#include <cstring>
#include <map>
#include <vector>
#include <list>

namespace Humongous
{


// SPUTM 4CCs

// general room IDs
const static char id_LECF[4]	= { 'L', 'E', 'C', 'F' };	// game data container
const static char id_LFLF[4]	= { 'L', 'F', 'L', 'F' };	// room data container
const static char id_RMIM[4]	= { 'R', 'M', 'I', 'M' };	// room header 1
const static char id_RMIH[4]	= { 'R', 'M', 'I', 'H' };	// room id?
const static char id_RMDA[4]	= { 'R', 'M', 'D', 'A' };	// room resources
const static char id_RMHD[4]	= { 'R', 'M', 'H', 'D' };	// room header 2

// palettes
const static char id_CYCL[4]	= { 'C', 'Y', 'C', 'L' };	// ?
const static char id_TRNS[4]	= { 'T', 'R', 'N', 'S' };	// transparency color index
const static char id_PALS[4]	= { 'P', 'A', 'L', 'S' };	// room palette container
const static char id_WRAP[4]	= { 'W', 'R', 'A', 'P' };	// general wrapper
const static char id_OFFS[4]	= { 'O', 'F', 'F', 'S' };	// offsets to stuff in WRAP
const static char id_APAL[4]	= { 'A', 'P', 'A', 'L' };	// palette entries
const static char id_RGBS[4]	= { 'R', 'G', 'B', 'S' };	// palette entries
const static char id_REMP[4]	= { 'R', 'E', 'M', 'P' };	// remaps deindexed AKOS colors from
															// local palette to room palette

// graphics
const static char id_OBIM[4]	= { 'O', 'B', 'I', 'M' };	// object image data
const static char id_IMHD[4]	= { 'I', 'M', 'H', 'D' };	// object image header
const static char id_SMAP[4]	= { 'S', 'M', 'A', 'P' };	// SMAP-type bitmap
const static char id_BMAP[4]	= { 'B', 'M', 'A', 'P' };	// BMAP-type bitmap
const static char id_BOMP[4]	= { 'B', 'O', 'M', 'P' };	// DOS cursors
const static char id_IMxx[2]	= { 'I', 'M' };				// image container
															// last 2 chars are # string
const static char id_ZPxx[2]	= { 'Z', 'P' };				// extra chunk for some B/SMAPs
															// last 2 chars are # string
const static char id_TMSK[4]	= { 'T', 'M', 'S', 'K' };	// something

// objects
const static char id_OBCD[4]	= { 'O', 'B', 'C', 'D' };	// object code container
const static char id_CDHD[4]	= { 'C', 'D', 'H', 'D' };	// object code header
const static char id_VERB[4]	= { 'V', 'E', 'R', 'B' };	// object code data
const static char id_OBNA[4]	= { 'O', 'B', 'N', 'A' };	// object name

// scripts
const static char id_EXCD[4]	= { 'E', 'X', 'C', 'D' };	// code?
const static char id_ENCD[4]	= { 'E', 'N', 'C', 'D' };	// code?
const static char id_NLSC[4]	= { 'N', 'L', 'S', 'C' };	// number of LSCRs
const static char id_LSCR[4]	= { 'L', 'S', 'C', 'R' };	// script?
const static char id_LSC2[4]	= { 'L', 'S', 'C', '2' };	// newer script?
const static char id_BOXD[4]	= { 'B', 'O', 'X', 'D' };	// old games: ??
const static char id_BOXM[4]	= { 'B', 'O', 'X', 'M' };	// old games: ??
const static char id_SCAL[4]	= { 'S', 'C', 'A', 'L' };	// old games: scaling?
const static char id_POLD[4]	= { 'P', 'O', 'L', 'D' };	// ?
const static char id_SCRP[4]	= { 'S', 'C', 'R', 'P' };	// script?
// separate containers for subtitles introduced in later games
const static char id_TLKE[4]	= { 'T', 'L', 'K', 'E' };	// subtitle container
const static char id_TEXT[4]	= { 'T', 'E', 'X', 'T' };	// subtitle subcontainer

// sounds
const static char id_TLKB[4]	= { 'T', 'L', 'K', 'B' };	// dialog file container
const static char id_SONG[4]	= { 'S', 'O', 'N', 'G' };	// music file container
const static char id_SGHD[4]	= { 'S', 'G', 'H', 'D' };	// music file header
const static char id_SGEN[4]	= { 'S', 'G', 'E', 'N' };	// music file subheader
const static char id_SOUN[4]	= { 'S', 'O', 'U', 'N' };	// old games: sound container
const static char id_WSOU[4]	= { 'W', 'S', 'O', 'U' };	// newer games: sound container
const static char id_DIGI[4]	= { 'D', 'I', 'G', 'I' };	// sound (sub)container
const static char id_TALK[4]	= { 'T', 'A', 'L', 'K' };	// dialog (sub)container
const static char id_SBNG[4]	= { 'S', 'B', 'N', 'G' };	// ?
const static char id_MIDI[4]	= { 'M', 'I', 'D', 'I' };	// MIDI data (sub)container
const static char id_HSHD[4]	= { 'H', 'S', 'H', 'D' };	// sound header
const static char id_SDAT[4]	= { 'S', 'D', 'A', 'T' };	// sound sample data
const static char id_PETE[4]	= { 'P', 'E', 'T', 'E' };	// used only once in fbear?
const static char id_SRFS[4]	= { 'S', 'R', 'F', 'S' };	// used only once in spymustard?
const static char id_FMUS[4]	= { 'F', 'M', 'U', 'S' };	// music file name (3DO only)
const static char id_MRAW[4]	= { 'M', 'R', 'A', 'W' };	// music file (3DO only)
const static char id_RIFF[4]	= { 'R', 'I', 'F', 'F' };	// standard RIFF file

// actors
const static char id_AKOS[4]	= { 'A', 'K', 'O', 'S' };	// animation container
const static char id_AKHD[4]	= { 'A', 'K', 'H', 'D' };	// animation header
const static char id_AKPL[4]	= { 'A', 'K', 'P', 'L' };	// anim color index table
const static char id_AKSQ[4]	= { 'A', 'K', 'S', 'Q' };	// ?
const static char id_AKCH[4]	= { 'A', 'K', 'C', 'H' };	// ?
const static char id_AKFO[4]	= { 'A', 'K', 'F', 'O' };	// ? fatty bear only?
const static char id_AKOF[4]	= { 'A', 'K', 'O', 'F' };	// AKCD/AKCI offsets
const static char id_AKCI[4]	= { 'A', 'K', 'C', 'I' };	// subimage dimension table
const static char id_AKCD[4]	= { 'A', 'K', 'C', 'D' };	// animation data
const static char id_AKLC[4]	= { 'A', 'K', 'L', 'C' };	// ?
const static char id_AKST[4]	= { 'A', 'K', 'S', 'T' };	// ?
const static char id_AKCT[4]	= { 'A', 'K', 'C', 'T' };	// ?
// secondary graphics containers used in later games
const static char id_AKAX[4]	= { 'A', 'K', 'A', 'X' };	// main container
const static char id_AUXD[4]	= { 'A', 'U', 'X', 'D' };	// subcontainer
const static char id_AXFD[4]	= { 'A', 'X', 'F', 'D' };	// image data
const static char id_AXUR[4]	= { 'A', 'X', 'U', 'R' };	// ?
const static char id_AXER[4]	= { 'A', 'X', 'E', 'R' };	// ?
// optional metadata
const static char id_SP2C[4]	= { 'S', 'P', '2', 'C' };	// metadata: date
const static char id_SPLF[4]	= { 'S', 'P', 'L', 'F' };	// metadata: filename
const static char id_CLRS[4]	= { 'C', 'L', 'R', 'S' };	// metadata: colors/compress.
const static char id_IMGL[4]	= { 'I', 'M', 'G', 'L' };	// ?
const static char id_SQDB[4]	= { 'S', 'Q', 'D', 'B' };	// ?
const static char id_SEQI[4]	= { 'S', 'E', 'Q', 'I' };	// ?
const static char id_NAME[4]	= { 'N', 'A', 'M', 'E' };	// metadata: sequence name?
const static char id_STOF[4]	= { 'S', 'T', 'O', 'F' };	// ?
const static char id_SQLC[4]	= { 'S', 'Q', 'L', 'C' };	// ?
const static char id_SIZE[4]	= { 'S', 'I', 'Z', 'E' };	// ?
const static char id_CHAR[4]	= { 'C', 'H', 'A', 'R' };	// ?

// animations
const static char id_AWIZ[4]	= { 'A', 'W', 'I', 'Z' };	// ?
const static char id_XMAP[4]	= { 'X', 'M', 'A', 'P' };	// later games: ??
const static char id_CNVS[4]	= { 'C', 'N', 'V', 'S' };	// later games: ??
const static char id_RELO[4]	= { 'R', 'E', 'L', 'O' };	// later games: ??
const static char id_WIZH[4]	= { 'W', 'I', 'Z', 'H' };	// ?
const static char id_SPOT[4]	= { 'S', 'P', 'O', 'T' };	// ?
const static char id_WIZD[4]	= { 'W', 'I', 'Z', 'D' };	// ?
const static char id_MULT[4]	= { 'M', 'U', 'L', 'T' };	// container?
const static char id_DEFA[4]	= { 'D', 'E', 'F', 'A' };	// ?
const static char id_RMAP[4]	= { 'R', 'M', 'A', 'P' };	// ?
const static char id_CUSE[4]	= { 'C', 'U', 'S', 'E' };	// ?

// DOS/old versions only
const static char id_LOFF[4]	= { 'L', 'O', 'F', 'F' };	// offset table?
const static char id_ROOM[4]	= { 'R', 'O', 'O', 'M' };	// room subcontainer

typedef int ObjectID;

typedef std::vector<int> ColorMap;

// enum of chunk types
enum ChunkType
{
	chunk_none, chunk_unknown,
	lecf, lflf, rmim, rmih, rmda, rmhd,
	cycl, trns, pals, wrap, offs, apal, rgbs, remp,
	obim, imhd, smap, bmap, bomp, imxx, zpxx, tmsk,
	obcd, cdhd, verb, obna,
	excd, encd, nlsc, lscr, lsc2, boxd, boxm, scal, pold, scrp, tlke, text,
	tlkb, song, sghd, sgen, soun, wsou, digi, talk, sbng, midi, hshd, sdat, 
		pete, srfs, fmus, mraw, riff,
	akos, akhd, akpl, aksq, akfo, akch, akof, akci, akcd, aklc, akst, akct,
			akax, auxd, axfd, axur, axer,
		sp2c, splf, clrs, imgl, sqdb, seqi, name, stof, sqlc, size, chunk_char,
	awiz, xmap, cnvs, relo, wizh, spot, wizd, mult, defa, rmap, cuse,
	loff, room
};

// struct for SPUTM chunks (header only)
struct SputmChunkHead
{
	SputmChunkHead()
		: size(0), address(0), type(chunk_none) { };
	virtual ~SputmChunkHead() { };

	int nextaddr() const
	{
		return address + size;
	}
		
	std::string name;
	int size;
	int address;
	ChunkType type;

};

// general struct for SPUTM chunks (all data)
struct SputmChunk : public SputmChunkHead
{
	SputmChunk()
		: SputmChunkHead(), datasize(0), data(NULL) { };
	SputmChunk(const SputmChunk& s)
		: SputmChunkHead(s), datasize(s.datasize)
	{
		data = new char[s.datasize];
		std::memcpy(data, s.data, s.datasize);
	}
	virtual ~SputmChunk()
	{
		delete[] data;
	}
	virtual SputmChunk& operator=(const SputmChunk& s)
	{
		name = s.name;
		size = s.size;
		address = s.address;
		type = s.type;
		resize(s.datasize);
		std::memcpy(data, s.data, s.datasize);
		return *this;
	}

	void resize(int newsize)
	{
		delete[] data;
		datasize = newsize;
		data = new char[newsize];
	}
		
	int datasize;
	char* data;
};

// structs for specific chunk types we're interested in

struct RMHDChunk : public SputmChunk
{
	RMHDChunk()
		: SputmChunk() { };

	int width;
	int height;
	int objects;
};

struct IMHDChunk : public SputmChunk
{
	IMHDChunk()
		: SputmChunk() { };

	ObjectID id;
};

struct IMxxChunk : public SputmChunk
{
	IMxxChunk()
		: SputmChunk() { };

	int number;
//	int width;
//	int height;
	SputmChunk image_chunk;	// can be either BMAP or SMAP
	std::vector<SputmChunk> zp_chunks;
	std::vector<SputmChunk> tmsk_chunks;
};

struct RMIMChunk : public SputmChunk
{
	RMIMChunk()
		: SputmChunk() { };

	int rmih_val;
	std::vector<IMxxChunk> images;
};
	
struct OFFSChunk : public SputmChunk
{
	OFFSChunk()
		: SputmChunk() { };

	std::vector<int> offsets;
};

struct WRAPChunk : public SputmChunk
{
	WRAPChunk()
		: SputmChunk() { };

	OFFSChunk offs_chunk;
};

struct PALSChunk : public SputmChunk
{
	PALSChunk()
		: SputmChunk() { };

	std::vector<RipUtil::BitmapPalette> apals;
};

struct OBIMChunk : public SputmChunk
{
	OBIMChunk()
		: SputmChunk() { };

	IMHDChunk imhd_chunk;
	std::vector<IMxxChunk> images;
};

struct CDHDChunk : public SputmChunk
{
	CDHDChunk()
		: SputmChunk() { };

	ObjectID id;
	int x;
	int y;
	int width;
	int height;
};

// not currently used
struct OBNAChunk : public SputmChunk
{
	OBNAChunk()
		: SputmChunk() { };

	std::string name;
};

struct OBCDChunk : public SputmChunk
{
	OBCDChunk()
		: SputmChunk() { };

	CDHDChunk cdhd_chunk;
	SputmChunk verb_chunk;
	std::string obna_val;
};

struct REMPChunk : public SputmChunk
{
	REMPChunk()
		: SputmChunk() { };

	ColorMap colormap;
};

struct HSHDChunk : public SputmChunk
{
	HSHDChunk()
		: SputmChunk() { };

	int unknown1;
	int unknown2;
	int unknown3;
	int samplerate;
	int unknown4;
	int unknown5;
	int unknown6;
	int unknown7;
};

struct FMUSSDATChunk : public SputmChunk
{
	FMUSSDATChunk()
		: SputmChunk() { };

	std::string filestring;
};

struct SoundChunk : public SputmChunk
{
	SoundChunk()
		: SputmChunk() { };

	HSHDChunk hshd_chunk;
	RipUtil::PCMData wave;
};

struct RIFFEntry
{
	RIFFEntry()
		: riffdat(0), riffdat_size(0) { };

	~RIFFEntry()
	{
		delete[] riffdat;
	}

	RIFFEntry(const RIFFEntry& a)
		: riffdat_size(a.riffdat_size)
	{
		riffdat = new char[a.riffdat_size];
		std::memcpy(riffdat, a.riffdat, a.riffdat_size);
	}

	RIFFEntry& operator=(const RIFFEntry& a)
	{
		delete[] riffdat;
		riffdat = new char[a.riffdat_size];
		riffdat_size = a.riffdat_size;
		return *this;
	}

	void resize(int n)
	{
		delete[] riffdat;
		riffdat = new char[n];
		riffdat_size = n;
	}

	char* riffdat;
	int riffdat_size;
};

struct WSOUChunk : public SputmChunk
{
	WSOUChunk()
		: SputmChunk() { };

	RIFFEntry riff_entry;
};

struct FMUSChunk : public SputmChunk
{
	FMUSChunk()
		: SputmChunk() { };

	HSHDChunk hshd_chunk;
	FMUSSDATChunk fmussdat_chunk;
};

struct SEQIChunk : public SputmChunk
{
	SEQIChunk()
		: SputmChunk() { };

	std::string name_val;
	SputmChunk stof_chunk;
	SputmChunk sqlc_chunk;
	SputmChunk size_chunk;
};

struct SQDBChunk : public SputmChunk
{
	SQDBChunk()
		: SputmChunk() { };

	std::vector<SEQIChunk> seqi_chunks;
};

// AKOS header
struct AKHDChunk : public SputmChunk
{
	AKHDChunk()
		: SputmChunk(),
		unknown1(0), sequence_encoding(0), anislots(0),
		unknown3(0), encoding(0), unknown5(0) { };

	int unknown1;
	int sequence_encoding;
	int anislots;	// number of animation "slots"
	int unknown3;
	int encoding;
	int unknown5;
};

// container for decompressed AKOS components
typedef std::vector<RipUtil::BitmapData> AKOSComponentContainer;

typedef std::vector<RipUtil::BitmapData> GenericBitmapContainer;

typedef std::vector<GenericBitmapContainer> AKOSSequenceContainer;

struct AKSQStaticFrameComponent
{
	AKSQStaticFrameComponent()
		: xoffset(0), yoffset(0), graphic(0) { };

	int xoffset;	// x-offset from center point
	int yoffset;	// y-offset from center point
	int graphic;	// index number of component image in source AKOS
};

struct AKSQDynamicFrameComponent
{
	AKSQDynamicFrameComponent()
		: xoffset(0), yoffset(0), graphic(0), compid(0) { };

	int xoffset;	// x-offset from center point
	int yoffset;	// y-offset from center point
	int graphic;	// index number of component image in source AKOS
	int compid;		// internal ID number to reference this component
};

typedef std::list<AKSQStaticFrameComponent> AKSQStaticFrameComponentContainer;

typedef std::list<AKSQDynamicFrameComponent> AKSQDynamicFrameComponentContainer;

struct AKSQStaticFrame
{
	AKSQStaticFrame()
		: framenum(0) { };
	
	int framenum;	// sequence number assigned by this program
					// for the purpose of keeping mixed static and
					// dynamic frames sorted
	AKSQStaticFrameComponentContainer components;
};

struct AKSQDynamicFrame
{
	AKSQDynamicFrame()
		: framenum(0) { };
	
	int framenum;	// sequence number assigned by this program
					// for the purpose of keeping mixed static and
					// dynamic frames sorted
	AKSQDynamicFrameComponentContainer components;
};

typedef std::list<AKSQStaticFrame> AKSQStaticFrameSequence;

typedef std::list<AKSQDynamicFrame> AKSQDynamicFrameSequence;

struct FrameSequence
{
	FrameSequence() { };

	void clear()
	{
		staticframes.clear();
		dynamicframes.clear();
	}

	AKSQStaticFrameSequence staticframes;
	AKSQDynamicFrameSequence dynamicframes;
};

typedef std::list<FrameSequence> FrameSequenceContainer;

struct SequenceSizingInfo
{
	SequenceSizingInfo()
		: width(0), height(0), centerx(0), centery(0) { };

	int width;
	int height;
	int centerx;
	int centery;
};

struct AKSQChunk : public SputmChunk
{
	AKSQChunk()
		: SputmChunk() { };
};

struct AKCHPointerTable2EntryAKSQPointer
{
	AKCHPointerTable2EntryAKSQPointer() { };

	int id;
	int pointer;
};

struct AKCHPointerTable2Entry
{
	AKCHPointerTable2Entry() { };

	int type;
	std::vector<AKCHPointerTable2EntryAKSQPointer> pointerentries;
};

struct AKCHChunk : public SputmChunk
{
	AKCHChunk()
		: SputmChunk(),
		table2_raw(0), table2_raw_size(0) { };
	~AKCHChunk()
	{
		delete table2_raw;
	}

	// table 1: local pointer to entries of table 2
	// the size of this table is given in the AKHD chunk
	std::vector<int> akch_pointers;
	// table 2: labelled pointers into AKSQ
	std::vector<AKCHPointerTable2Entry> aksq_pointers;

	// I think table2 may be replaced by some animation data
	// depending on the AKHD settings, so for now,
	// table2 will be read into this block as-is if the
	// AKCH encoding in AKHD is unrecognized
	char* table2_raw;
	int table2_raw_size;
};

struct AKPLChunk : public SputmChunk
{
	AKPLChunk()
		: SputmChunk(),
		hasalttrans(false), alttrans(0) { };
	
	int numcolors;
	ColorMap colormap;
	bool hasalttrans;
	int alttrans;
};

struct AXFDChunk : public SputmChunk
{
	AXFDChunk()
		: SputmChunk(),
		width(0), height(0), imgdat(0), imgdat_size(0) { };
	~AXFDChunk()
	{
		delete[] imgdat;
	}
	AXFDChunk(const AXFDChunk& a)
		: unknown(a.unknown), off1(a.off1), off2(a.off2),
		width(a.width), height(a.height), imgdat_size(a.imgdat_size)
	{
		imgdat = new char[a.imgdat_size];
		std::memcpy(imgdat, a.imgdat, a.imgdat_size);
	}
	AXFDChunk& operator=(const AXFDChunk& a)
	{
		unknown = a.unknown;
		off1 = a.off1;
		off2 = a.off2;
		width = a.width;
		height = a.height;
		imgdat_size = a.imgdat_size;
		delete[] imgdat;
		imgdat = new char[a.imgdat_size];
		return *this;
	}
	void resize(int n)
	{
		delete[] imgdat;
		imgdat = new char[n];
		imgdat_size = n;
	}

	int unknown;
	int off1;
	int off2;
	int width;
	int height;
	char* imgdat;
	int imgdat_size;
};

struct AUXDChunk : public SputmChunk
{
	AUXDChunk()
		: SputmChunk() { };

	AXFDChunk axfd_chunk;
	SputmChunk axur_chunk;
	SputmChunk axer_chunk;
};

struct AKAXChunk : public SputmChunk
{
	AKAXChunk()
		: SputmChunk() { };

	std::vector<AUXDChunk> auxd_chunks;
};

// AKOF animation offset table entry
struct AKOFEntry
{
	AKOFEntry() { };

	int akcd_offset;	// offset to image data in AKCD
	int akci_offset;	// offset to image dimensions in AKCI
};

struct AKCDEntry
{
	AKCDEntry()
		: imgdat(0), size(0) { };
	AKCDEntry(const AKCDEntry& akcdc)
	{
		imgdat = new char[akcdc.size];
		std::memcpy(imgdat, akcdc.imgdat, akcdc.size);
		size = akcdc.size;
		width = akcdc.width;
		height = akcdc.height;
	}
	~AKCDEntry()
	{
		delete[] imgdat;
	}
	void resize(int newsize)
	{
		delete[] imgdat;
		size = newsize;
		imgdat = new char[size];
	}
	AKCDEntry& operator=(const AKCDEntry& akcdc)
	{
		imgdat = new char[akcdc.size];
		std::memcpy(imgdat, akcdc.imgdat, akcdc.size);
		size = akcdc.size;
		width = akcdc.width;
		height = akcdc.height;
		return *this;
	}
		
	char* imgdat;
	int size;
	int width;
	int height;
};

struct AKCIEntry
{
	AKCIEntry() { };

	int width;
	int height;
};

struct AKOSChunk : public SputmChunk
{
	AKOSChunk()
		: SputmChunk() { };

	AKHDChunk akhd_chunk;
	AKPLChunk akpl_chunk;
	ColorMap colormap;
	RipUtil::BitmapPalette palette;
	AKSQChunk aksq_chunk;
	SputmChunk akfo_chunk;
	AKCHChunk akch_chunk;
	std::vector<AKOFEntry> akof_entries;
	std::vector<AKCIEntry> akci_entries;
	std::vector<AKCDEntry> akcd_entries;
	AKAXChunk akax_chunk;
	SputmChunk aklc_chunk;
	SputmChunk akst_chunk;
	SputmChunk akct_chunk;
	std::string file_date;
	std::string file_name;
	std::string file_compr;
	SputmChunk imgl_chunk;
	SQDBChunk sqdb_chunk;

	int numcolors;
};

struct CHAREntry
{
	CHAREntry() 
		: data(0), datalen(0), width(0), height(0), off1(0), off2(0) { };
	CHAREntry(const CHAREntry& c)
	{
		data = new char[c.datalen];
		std::memcpy(data, c.data, c.datalen);
		datalen = c.datalen;
		width = c.width;
		height = c.height;
		off1 = c.off1;
		off2 = c.off2;
	}
	CHAREntry& operator=(const CHAREntry& c)
	{
		data = new char[c.datalen];
		std::memcpy(data, c.data, c.datalen);
		datalen = c.datalen;
		width = c.width;
		height = c.height;
		off1 = c.off1;
		off2 = c.off2;
	}
	~CHAREntry()
	{
		delete[] data;
	}

	void resize(int n)
	{
		delete[] data;
		data = new char[n];
		datalen = n;
	}
	
	char* data;
	int datalen;

	int width;
	int height;
	int off1;
	int off2;
};

struct CHARChunk: public SputmChunk
{
	CHARChunk()
		: SputmChunk() { };

	int compr;
	int rowspace;
	ColorMap colormap;
	std::vector<CHAREntry> char_entries;
};

struct CYCLChunk : public SputmChunk
{
	CYCLChunk()
		: SputmChunk() { };

	int cycl_val;
};

struct TRNSChunk : public SputmChunk
{
	TRNSChunk()
		: SputmChunk() { };

	int trns_val;
};

struct NLSCChunk : public SputmChunk
{
	NLSCChunk()
		: SputmChunk() { };

	int nlsc_val;
};

struct TEXTChunk : public SputmChunk
{
	TEXTChunk()
		: SputmChunk() { };

	std::string text;
};

struct TLKEChunk : public SputmChunk
{
	TLKEChunk()
		: SputmChunk() { };

	TEXTChunk text_chunk;
};

struct RMAPChunk : public SputmChunk
{
	RMAPChunk()
		: SputmChunk() { };

	int unknown;
	ColorMap colormap;
};

struct AWIZChunk : public SputmChunk
{
	AWIZChunk()
		: SputmChunk() { };

	SputmChunk xmap_chunk;
	SputmChunk cnvs_chunk;
	SputmChunk relo_chunk;
	SputmChunk wizh_chunk;
	TRNSChunk trns_chunk;
	RipUtil::BitmapPalette palette;
	SputmChunk spot_chunk;
	SputmChunk wizd_chunk;
	RMAPChunk rmap_chunk;
	SputmChunk cuse_chunk;

	int unknown;
	int width;
	int height;
};

struct DEFAChunk : public SputmChunk
{
	DEFAChunk()
		: SputmChunk() { };

	RipUtil::BitmapPalette palette;
	RMAPChunk rmap_chunk;
	SputmChunk cuse_chunk;
	SputmChunk cnvs_chunk;
};

struct MULTChunk : public SputmChunk
{
	MULTChunk()
		: SputmChunk() { };

	DEFAChunk defa_chunk;
	std::vector<AWIZChunk> awiz_chunks;
};

// LFLF struct to contain all room data
struct LFLFChunk : public SputmChunkHead
{
	LFLFChunk()
		: SputmChunkHead() { };

	RMIMChunk rmim_chunk;
	RMHDChunk rmhd_chunk;
	CYCLChunk cycl_chunk;
	TRNSChunk trns_chunk;
	std::vector<RipUtil::BitmapPalette> apals;
	REMPChunk remp_chunk;
	std::map<ObjectID, OBIMChunk> obim_chunks;
	std::map<ObjectID, OBCDChunk> obcd_chunks;
	SputmChunk excd_chunk;
	SputmChunk encd_chunk;
	NLSCChunk nlsc_chunk;
	std::vector<SputmChunk> lscr_chunks;
	std::vector<SputmChunk> lsc2_chunks;
	SputmChunk boxd_chunk;
	SputmChunk boxm_chunk;
	SputmChunk scal_chunk;
	SputmChunk pold_chunk;
	std::vector<SputmChunk> scrp_chunks;
	std::vector<SoundChunk> digi_chunks;
	std::vector<SoundChunk> talk_chunks;
	std::vector<SputmChunk> midi_chunks;
	std::vector<WSOUChunk> wsou_chunks;
	std::vector<FMUSChunk> fmus_chunks;
	std::vector<AKOSChunk> akos_chunks;
	std::vector<CHARChunk> char_chunks;
	std::vector<AWIZChunk> awiz_chunks;
	std::vector<MULTChunk> mult_chunks;
	std::vector<TLKEChunk> tlke_chunks;
};

struct SONGEntry
{
	SONGEntry() { };

	int idnum;
	int address;
	int length;
};

struct SONGHeader
{
	SONGHeader() { };

	std::vector<SONGEntry> song_entries;
};

// empty colormap to be used as a parameter when calling a
// function requiring a colormap on data that doesn't need one
const ColorMap dummy_colormap;


};

#pragma once