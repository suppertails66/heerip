/* Humongous RipModule declaration
   Top-level handler for Humongous data ripping
   Responsible for detecting valid formats and initiating ripping */

#include "humongous_structs.h"

#include "RipModule.h"
#include "../utils/MembufStream.h"
#include "../utils/BitmapData.h"
#include "../RipperFormats.h"
#include <map>
#include <vector>

namespace Humongous
{


class HERip : public RipModule
{
public:
	HERip()
		: RipModule("Humongous Entertainment Datafile", "",
			true, "", RipModule::unsupported, RipModule::supported,
			RipModule::supported, RipModule::supported,
			RipModule::partial),
	
		encoding(not_set), decode_only(false),
		roomstart(not_set), roomend(not_set),
		rmimrip(true), obimrip(true), akosrip(true), sequencerip(false), awizrip(true), charrip(true),
		digirip(true), talkrip(true), wsourip(true), extdmurip(true), tlkerip(true),
		scriptrip(false), metadatarip(true),
		alttrans(false), transcol(not_set),
		catscripts(false),
		disablelog(false),
		cleared_tlke_file(false) { };

	bool can_rip(RipUtil::MembufStream& stream, const RipperFormats::RipperSettings& ripset,
		RipperFormats::FileFormatData& fmtdat);

	RipperFormats::RipResults rip(RipUtil::MembufStream& stream, const std::string& fprefix,
		const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat);

	void rip_lecf(RipUtil::MembufStream& stream, const std::string& fprefix,
		const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat,
		RipperFormats::RipResults& results);

	void rip_tlkb(RipUtil::MembufStream& stream, const std::string& fprefix,
		const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat,
		RipperFormats::RipResults& results);

	void rip_song_type12(RipUtil::MembufStream& stream, const std::string& fprefix,
		const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat,
		RipperFormats::RipResults& results);

	void rip_song_type34(RipUtil::MembufStream& stream, const std::string& fprefix,
		const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat,
		RipperFormats::RipResults& results);

	void rip_song_entries(RipUtil::MembufStream& stream, const SONGHeader& song_header,
		const std::string& fprefix, const RipperFormats::RipperSettings& ripset, 
		RipperFormats::RipResults& results);

	void rip_song_dmu(RipUtil::MembufStream& stream, const std::string& fprefix,
		const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat,
		RipperFormats::RipResults& results);

	void rip_extdmu(const LFLFChunk& lflfc, const std::string& fprefix, 
		const RipperFormats::RipperSettings& ripset, 
		const RipperFormats::FileFormatData& fmtdat, 
		RipperFormats::RipResults& results);

	// Top-level rippers

	void rip_rmim(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
		const std::string& fprefix, RipperFormats::RipResults& results,
		int transind);

	void rip_obim(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
		const std::string& fprefix, RipperFormats::RipResults& results,
		int transind);

	void rip_akos(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
		const std::string& fprefix, RipperFormats::RipResults& results,
		int transind);

	void rip_awiz(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
		const std::string& fprefix, RipperFormats::RipResults& results,
		int transind);

	void rip_char(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
		const std::string& fprefix, RipperFormats::RipResults& results,
		int transind);

	void rip_sound(std::vector<SoundChunk>& sound_chunks, 
		const RipperFormats::RipperSettings& ripset, const std::string& fprefix,
		RipperFormats::RipResults& results);

	void rip_digi(LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
		const std::string& fprefix, RipperFormats::RipResults& results);

	void rip_talk(LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
		const std::string& fprefix, RipperFormats::RipResults& results);

	void rip_wsou(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
		const std::string& fprefix, RipperFormats::RipResults& results,
		bool decode_audio);

	void rip_tlke(LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
		const std::string& filename, int rmnum, RipperFormats::RipResults& results);

	void rip_scripts(LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
		const std::string& filename, int rmnum, RipperFormats::RipResults& results);

	void rip_metadata(const LFLFChunk& lflfc, const RipperFormats::RipperSettings& ripset,
		const std::string& filename, int rmnum, RipperFormats::RipResults& results);



	// read palettes from a SPUTM datafile, with stream starting at first LECF
	void scan_palettes(RipUtil::MembufStream& stream);

private:
	const static int not_set = -1;	// placeholder for unset data

	enum FormatSubtype
	{
		fmt_none, fmt_unknown, 
		lecf_type1, lecf_type2,
		tlkb_type1,
		song_type1, song_type2, song_type3, song_type4, song_dmu
	};

	FormatSubtype formatsubtype;

	int encoding;		// XOR decoding byte
	bool decode_only;	// skip ripping and decode file only?

	int roomstart;		// number of first room to rip
	int roomend;		// number of last room to rip

	bool rmimrip;		// types of data to rip
	bool obimrip;
	bool akosrip;
	bool sequencerip;
	bool awizrip;
	bool charrip;
	bool digirip;
	bool talkrip;
	bool wsourip;
	bool extdmurip;
	bool tlkerip;
	bool scriptrip;
	bool metadatarip;

	bool alttrans;		// is alternate transparency index set?
	int transcol;		// internal transparency index

	bool catscripts;	// when dumping scripts, concatenate to single file

	bool disablelog;

	bool cleared_tlke_file;

	std::vector<RipUtil::BitmapPalette> room_palettes;

	void check_params(const RipperFormats::RipperSettings& ripset);

	// disable all rip settings
	void disable_all_ripping();

};


};	// end of namespace Humongous

#pragma once