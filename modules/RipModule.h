/*	Abstract base class for ripper modules
	A ripper module provides functions for identifying and
	extracting data from a file */

#include "../utils/MembufStream.h"
#include "../RipperFormats.h"

class RipModule
{
public:

	// levels of support for ripping types
	enum SupportLevel
	{
		unsupported,
		supported,
		partial,
		unnecessary
	};

	// module name
	const std::string module_name;
	// module description
	const std::string module_desc;
	// explanation of special command parameters (printed with usage info)
	// does the module take special command parameters?
	const bool has_extra_parameters;
	const std::string extra_parameters_text;

	// what ripping types does the module support?
	const SupportLevel supports_raw;
	const SupportLevel supports_graphics;
	const SupportLevel supports_animations;
	const SupportLevel supports_audio;
	const SupportLevel supports_strings;

	// given a stream positioned at file start, return true if
	// this module is capable of extracting data from it
	virtual bool can_rip(RipUtil::MembufStream& stream, const RipperFormats::RipperSettings& ripset,
		RipperFormats::FileFormatData& fmtdat) =0;

	// given a stream positioned at file start and the rip settings,
	// rip data from the file
	virtual RipperFormats::RipResults rip(RipUtil::MembufStream& stream, const std::string& fprefix,
		const RipperFormats::RipperSettings& ripset, const RipperFormats::FileFormatData& fmtdat) =0;

protected:
	RipModule(const std::string& name, const std::string& desc,
		bool hasext, const std::string& ext,
		SupportLevel rw, SupportLevel grp, 
		SupportLevel ani, SupportLevel aud, SupportLevel str)
		: module_name(name),
		module_desc(desc),
		extra_parameters_text(ext),
		has_extra_parameters(hasext),
		supports_raw(rw),
		supports_graphics(grp),
		supports_animations(ani),
		supports_audio(aud),
		supports_strings(str) { };
private:
};


#pragma once