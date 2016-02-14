#include "humongous_structs.h"
#include "humongous_rip.h"

#include "../utils/MembufStream.h"
#include "../utils/BitStream.h"
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
#include <cmath>

using namespace RipUtil;
using namespace RipperFormats;
using namespace ErrLog;
using namespace Logger;

namespace Humongous
{


// hack to work around a change in semantics of BMAP/SMAP RLE encoding (codes 8 and 9) 
// between 3DO and later games
// in 3DO, this is an unlined data format; in later games, it is lined
// we test the first n such images decoded and remember the results for later entries
RLEEncodingMethodHackValue rle_encoding_method_hack = rle_hack_is_not_set;
int rle_encoding_method_hack_images_to_test = 10;
int rle_encoding_method_hack_lined_images = 0;
int rle_encoding_method_hack_unlined_images = 0;
bool rle_encoding_method_hack_was_user_overriden = false;

// similarly, for handling nominally 2-color AKOS
AKOS2ColorDecodingHackValue akos_2color_decoding_hack = akos_2color_hack_is_not_set;
int akos_2color_decoding_hack_images_to_test = 10;
int akos_2color_decoding_hack_rle_images = 0;
int akos_2color_decoding_hack_bitmap_images = 0;
bool akos_2color_decoding_hack_was_user_overriden = false;

// misc stuff






void decode_imxx(const IMxxChunk& imxxc, RipUtil::BitmapData& bitmap, int width, int height,
	int localtransind, int transind)
{
	if (imxxc.image_chunk.type == smap)
	{
		decode_smap(imxxc.image_chunk, bitmap, width, height, localtransind, transind);
	}
	else if (imxxc.image_chunk.type == bmap)
	{
		decode_bmap(imxxc.image_chunk, bitmap, width, height, localtransind, transind);
	}
	else if (imxxc.image_chunk.type == bomp)
	{
		decode_bomp(imxxc.image_chunk, bitmap, localtransind, transind);
	}
	else
	{
		logger.error("\tunrecognized bitmap subtype " + imxxc.image_chunk.name);
	}
}

void decode_bmap(const SputmChunk& bmapc, RipUtil::BitmapData& bmap, int width, int height,
	int localtransind, int transind)
{
	bmap.resize_pixels(width, height, 8);
	bmap.clear(transind);
	int encoding = to_int(bmapc.data + 8, 1);
	decode_encoded_bitmap(bmapc.data + 9, encoding, bmapc.datasize - 9, bmap,
		0, 0, width, height, localtransind, transind);
}

void decode_smap(const SputmChunk& smapc, RipUtil::BitmapData& bmap, int width, int height,
	int localtransind, int transind)
{
	// width and height are always rounded down to a multiple of 8... not??
//	bmap.resize_pixels(width/8*8, height/8*8, 8);
	bmap.resize_pixels(width, height, 8);
	bmap.clear(transind);
	int strips = width/8;
	char* offset = smapc.data + 8;

	for (int i = 0; i < strips; i++)
	{
		int offset_int = to_int(offset, 4, DatManip::le);
		int encoding = to_int(smapc.data + offset_int, 1);
		decode_encoded_bitmap(smapc.data + offset_int + 1, encoding,
			smapc.datasize - offset_int, bmap, i * 8, 0, 8, height,
			localtransind, transind);
		offset += 4;
	}
}

void decode_bomp(const SputmChunk& bompc, RipUtil::BitmapData& bmap, int localtransind, 
	int transind, ColorMap colormap, bool deindex)
{
	char* data = bompc.data + 8;
	int unknown = to_int(data++, 1);
	int bomptrans = to_int(data++, 1);
	int width = to_int(data, 2, DatManip::le);
	data += 2;
	int height = to_int(data, 2, DatManip::le);
	data += 2;
	int unknown3 = to_int(data, 2, DatManip::le);
	data += 2;
	int unknown4 = to_int(data, 2, DatManip::le);
	data += 2;

	bmap.resize_pixels(width, height, 8);
	bmap.clear(transind);

	int currx = 0;
	int curry = 0;

	int pos = 0;
	int next_pos = pos;
	int datlen = bompc.datasize - 18;
	while (pos < datlen && curry < height)
	{
		int bytecount = to_int(data + next_pos, 2, DatManip::le);
		pos = next_pos + 2;
		next_pos += bytecount + 2;
		while (pos < datlen && pos < next_pos)
		{
			int code = to_int(data + pos, 1);
			++pos;

			if (code & 1)		// encoded run
			{
				int count = (code >> 1) + 1;
				int color = to_int(data + pos, 1);
				++pos;
				if (color != bomptrans)
				{
					if (deindex)
						color = colormap[color];
					bmap.draw_row(color, count, currx, curry, 0, 0, width, height);
				}
				currx += count;
			}
			else				// absolute run
			{
				int count = (code >> 1) + 1;
				for (int i = 0; i < count; i++)
				{
					int color = to_int(data + pos, 1);
					if (color != bomptrans)
					{
						if (deindex)
							color = colormap[color];
						bmap.draw_row(color, 1, currx, curry, 0, 0, width, height);
					}
					++pos;
					++currx;
				}
			}
		}
		currx = 0;
		++curry;
	}
}

void decode_bomp(const SputmChunk& bompc, RipUtil::BitmapData& bmap, int localtransind, int transind)
{
	decode_bomp(bompc, bmap, localtransind, transind, dummy_colormap, false);
}



// decode AKOS (any palette, any colormap, deindexed or indexed
void decode_akos(const AKOSChunk& akosc, RipUtil::BitmapData& bmap,
	int entrynum, RipUtil::BitmapPalette palette, int localtransind, int transind,
	ColorMap colormap, bool deindex, ColorMap colorremap, bool remap)
{
	bmap.resize_pixels(akosc.akcd_entries[entrynum].width, 
		akosc.akcd_entries[entrynum].height, 8);
	bmap.set_palettized(true);
	bmap.set_palette(palette);
	bmap.clear(transind);

	// in "some" games, nominally 2-color AKOSs actually use bitmap encoding
	// however, "other" games, mostly newer ones, use lined RLE
	// unfortunately, there seems to be no distinction between these
	// in the data files, so we have to do some guesswork

	if (akosc.numcolors == 2)
	{
		if (!akos_2color_decoding_hack_was_user_overriden
			&& akos_2color_decoding_hack_images_to_test)
		{
			if (is_lined_rle(akosc.akcd_entries[entrynum].imgdat,
				akosc.akcd_entries[entrynum].size))
			{
				++akos_2color_decoding_hack_rle_images;
			}
			else
			{
				++akos_2color_decoding_hack_bitmap_images;
			}

			--akos_2color_decoding_hack_images_to_test;

			AKOS2ColorDecodingHackValue newval;

			if (akos_2color_decoding_hack_rle_images
				>= akos_2color_decoding_hack_bitmap_images)
			{
				newval = akos_2color_hack_always_use_rle;
			}
			else
			{
				newval = akos_2color_hack_always_use_bitmap;
			}

			if (akos_2color_decoding_hack != akos_2color_hack_is_not_set
				&& newval != akos_2color_decoding_hack)
			{
				logger.warning("changing value of AKOS 2-color encoding method hack. "
					"Initial images were probably incorrectly ripped; "
					"to rip these, try using --force_akos2c_rle or "
					"--force_akos2c_bitmap");
			}

			akos_2color_decoding_hack = newval;

		}
		if (akos_2color_decoding_hack == akos_2color_hack_always_use_rle)
		{
			decode_lined_rle(akosc.akcd_entries[entrynum].imgdat, akosc.akcd_entries[entrynum].size,
				bmap, 0, 0, akosc.akcd_entries[entrynum].width, akosc.akcd_entries[entrynum].height,
				akosc.akpl_chunk.alttrans, transind, true, dummy_colormap, false);
		}
		else if (akos_2color_decoding_hack == akos_2color_hack_always_use_bitmap)
		{
			int encoding = akosc.akcd_entries[entrynum].imgdat[0];

			// check if data is actually lined RLE before accepting encoding
			if ((encoding != 8 
				|| (encoding == 8 && is_lined_rle(akosc.akcd_entries[entrynum].imgdat, akosc.akcd_entries[entrynum].size)))
				&& !akos_2color_decoding_hack_was_user_overriden)
			{
				logger.warning("guessed bitmap encoding for 2-color AKOS, but specified "
					"encoding was not 8. "
					"Image data will be treated as lined RLE instead; if problems result, "
					"try using --force_akos2c_bitmap");

				decode_lined_rle(akosc.akcd_entries[entrynum].imgdat, akosc.akcd_entries[entrynum].size,
					bmap, 0, 0, akosc.akcd_entries[entrynum].width, akosc.akcd_entries[entrynum].height,
					akosc.akpl_chunk.alttrans, transind, true, dummy_colormap, false);
			}
			else if (encoding == 8 || akos_2color_decoding_hack_was_user_overriden)
			{
				// all games seem to use an encoding of 0x8 with the semantics
				// of encoding 0x58 + transparent color 0, possibly combined with an REMP
				encoding = 0x58;
				decode_bitstream_img(akosc.akcd_entries[entrynum].imgdat + 1,
					akosc.akcd_entries[entrynum].size - 1, bmap, 0, 0,
					akosc.akcd_entries[entrynum].width, akosc.akcd_entries[entrynum].height,
					8, 3, true, true, false,
					akosc.akpl_chunk.alttrans, transind, colorremap, remap);
			}
			else
			{
				logger.error("2-color AKOS has invalid encoding " + to_string(encoding)
					+ " and is not lined RLE");
			}
		}
	}
	else
	{
		decode_multicomp_rle(akosc.akcd_entries[entrynum].imgdat, akosc.akcd_entries[entrynum].width,
			akosc.akcd_entries[entrynum].height, bmap, palette, akosc.numcolors, localtransind, transind,
			colormap, deindex, colorremap, remap);
	}
}

void decode_akos(const AKOSChunk& akosc, RipUtil::BitmapData& bmap,
	int entrynum, RipUtil::BitmapPalette palette, int localtransind, int transind)
{
	decode_akos(akosc, bmap, entrynum, palette, localtransind, transind,
		dummy_colormap, false, dummy_colormap, false);
}

void decode_akos(const AKOSChunk& akosc, RipUtil::BitmapData& bmap,
	int entrynum, RipUtil::BitmapPalette palette, int localtransind, int transind,
	ColorMap colormap, bool deindex)
{
	decode_akos(akosc, bmap, entrynum, palette, localtransind, transind,
		colormap, deindex, dummy_colormap, false);
}

void decode_auxd(const AUXDChunk& auxdc, RipUtil::BitmapData& bmap,
	int localtransind, int transind)
{
	const AXFDChunk& axfdc = auxdc.axfd_chunk;

	decode_type2_lined_rle(axfdc.imgdat, axfdc.imgdat_size, bmap, 
		axfdc.off1 + 320, axfdc.off2 + 240, axfdc.width, axfdc.height, localtransind, transind);
}

// utility function to detect whether data uses a lined format
bool is_lined(const char* data, int datlen)
{
	// total the line counts until we exceed datlen
	int count = 0;
	int pos = 0;
	while (pos < datlen)
	{
		// entries are null-terminated; datlen isn't always accurate
		if (*(data + pos) == 0)
			return true;

		int jump = to_int(data + pos, 2, DatManip::le) + 2;
		count += jump;
		pos += jump;
	}
	// if lined, total should be datlen
	if (count == datlen)
		return true;
	else
		return false;
}

bool is_lined_rle(const char* data, int datlen)
{
	int pos = 0;

	if (datlen < 2)
		return false;

	while (pos < datlen - 1)
	{
		int jump = to_int(data + pos, 2, DatManip::le) + 2;
		pos += jump;
	}

	// entries are null-terminated; datlen isn't always accurate
	if ((pos >= datlen - 1) && pos <= datlen)
		return true;

	return false;
}



void decode_sequences(const AKSQChunk& aksqc, const AKCHChunk& akchc,
	const AKOSComponentContainer& components, FrameSequenceContainer& sequences,
	int akch_encoding)
{
	// find start and end positions
//	char* start = aksqc.data + 8;
//	char* end = start + aksqc.datasize - 8;

	int pos = 8;
	int end = aksqc.datasize;

	FrameSequence sequence;
	int framenum = 0;

	while (pos < end)
	{
		int code = to_int(aksqc.data + pos, 1, DatManip::le, DatManip::has_nosign);
		pos += 1;

		// animation commands from later games
		switch (code)
		{
		case 0x00:
			// skip
			break;
		case 0xC0:
			{
				int subcode = to_int(aksqc.data + pos, 1, DatManip::le, DatManip::has_nosign);
				++pos;

				switch (subcode)
				{
				// ??? length 2
				case 0x01:
					break;
				// ??? length 5
				case 0x10:
					pos += 3;
					break;
				// ??? length 3
				case 0x15:
					pos += 1;
					break;
				// ??? length 5
				case 0x16:
					pos += 3;
					break;
				// ??? length 5
				case 0x17:
					pos += 3;
					break;
				// ??? length 5
				case 0x18:
					pos += 3;
					break;
				// ??? length 5
				case 0x19:
					pos += 3;
					break;
				// display frame
				case 0x20:
					read_and_generate_aksq_static_frame_components(aksqc.data, pos,
						framenum, sequence.staticframes);
					framenum += 1;
					break;
				// set up frame for external manipulation?
				case 0x21:
					read_and_generate_aksq_dynamic_frame_components(aksqc.data, pos,
						framenum, sequence.dynamicframes);
					framenum += 1;
					break;
				// set up and display 3 dynamic frames?
				case 0x22:
					read_and_generate_quick_aksq_dynamic_frame_components(aksqc.data, pos,
						framenum, sequence.dynamicframes);
					framenum += 1;
					break;
				// set up and display 3 static frames?
				case 0x25:
					{
						// first 4 bytes always 0x0000FFFF??
						pos += 4;
						// get number of frames
						int numcomp = to_int(aksqc.data + pos, 1, DatManip::le, DatManip::has_nosign);
						pos += 1;

						read_and_generate_unheadered_aksq_static_frame_components(aksqc.data, pos,
							framenum, sequence.staticframes, numcomp);
						framenum += 1;
					}
					break;
				// ??? length 4
				case 0x30:
					pos += 2;
					break;
				// ??? length 5
				case 0x40:
					pos += 3;
					break;
				// ??? length 3
				case 0x42:
					pos += 1;
					break;
				// ??? length 3
				case 0x44:
					pos += 1;
					break;
				// ??? length 7
				case 0x70:
					pos += 5;
					break;
				// ??? length 7
				case 0x71:
					pos += 5;
					break;
				// ??? length 7
				case 0x72:
					pos += 5;
					break;
				// ??? length 7
				case 0x73:
					pos += 5;
					break;
				// ??? length 7
				case 0x74:
					pos += 5;
					break;
				// ??? length 7
				case 0x75:
					pos += 5;
					break;
				// ??? length 3
				case 0x80:
					pos += 1;
					break;
				// ??? length 3
				case 0x81:
					pos += 1;
					break;
				// ??? length 7
				case 0x82:
					pos += 5;
					break;
				// ??? length 3
				case 0x83:
					pos += 1;
					break;
				// ??? length 6
				case 0x85:
					pos += 4;
					break;
				// ??? length 2
				case 0x86:
					break;
				// ??? length 3
				case 0x8D:
					pos += 1;
					break;
				// ??? length 4
				case 0x8E:
					pos += 2;
					break;
				// set line "slot"? length 4
				case 0xA0:
					pos += 2;
					break;
				// ??? length 4
				case 0xA1:
					pos += 2;
					break;
				// ??? length 4
				case 0xA2:
					pos += 2;
					break;
				// ??? length 3
				case 0xA3:
					pos += 1;
					break;
				// ??? length 3
				case 0xA4:
					pos += 1;
					break;
				// end of sequence
				case 0xFF:
					// add sequence to list and reset parameters
					sequences.push_back(sequence);
					sequence.clear();
					framenum = 0;
					break;
				default:
					logger.error("Unrecognized 0xC0 subcode " + to_string(subcode)
						+ " at " + to_string(aksqc.address + pos - 1) + ", aborting sequence rip");
					return;
					break;
				}
			}
			break;
		// unrecognized code
		default:
			{
				logger.warning("Unrecognized AKSQ code " + to_string(code) + " at "
					+ to_string(aksqc.address + pos - 1) + ", skipping to next instead");
//				return;
			}
			break;
		}
	}
}

void read_and_generate_aksq_static_frame_components(char* data, int& pos,
	int framenum, AKSQStaticFrameSequence& staticframes)
{
	// read number of components
	int num_components = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);
	pos += 1;

	AKSQStaticFrame frame;

	// read each component and add to frame
	for (int i = 0; i < num_components; i++)
	{
		AKSQStaticFrameComponent component;

		// x offset
		component.xoffset = to_int(data + pos, 2, DatManip::le, DatManip::has_sign);
		pos += 2;
		// y offset
		component.yoffset = to_int(data + pos, 2, DatManip::le, DatManip::has_sign);
		pos += 2;
		// graphic index
		component.graphic = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);
		pos += 1;
		// if index has top byte set, extended component number
		if (component.graphic & 0x80)
		{
			int next = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);

			// components 128-255 must be escaped due to the top byte of the
			// component number being used as the escape sequence marker
			if (component.graphic == 0x80)
			{
				if (next >= 0x80)
				{
					component.graphic = next;
				}
				else
				{
					component.graphic = next + 0x0100;
				}
			}
			else
			{
				// multiply lower 7 bits of first byte by 256 and add second byte
				// to get real index
				component.graphic = (((component.graphic & 0x7F)) * 0x0100) + next;
			}
			pos += 1;
		}

		// sanity checking: if x or y is improbably high or low,
		// don't add it to the frame
		// (some games do have strange out of range values like this)
		if (std::abs(component.xoffset) <= 1024
			&& std::abs(component.yoffset) <= 1024)
		{
			frame.components.push_back(component);
		}
		else
		{
			logger.warning("Component out of defined range, skipping");
		}
	}

	frame.framenum = framenum;

	staticframes.push_back(frame);

}

void read_and_generate_unheadered_aksq_static_frame_components(char* data, int& pos,
	int framenum, AKSQStaticFrameSequence& staticframes, int num_components)
{
	AKSQStaticFrame frame;

	// read each component and add to frame
	for (int i = 0; i < num_components; i++)
	{
		AKSQStaticFrameComponent component;

		// x offset
		component.xoffset = to_int(data + pos, 2, DatManip::le, DatManip::has_sign);
		pos += 2;
		// y offset
		component.yoffset = to_int(data + pos, 2, DatManip::le, DatManip::has_sign);
		pos += 2;
		// graphic index
		component.graphic = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);
		pos += 1;
		// if index has top byte set, extended component number
		if (component.graphic & 0x80)
		{
			int next = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);

			// components 128-255 must be escaped due to the top byte of the
			// component number being used as the escape sequence marker
			if (component.graphic == 0x80)
			{
				if (next >= 0x80)
				{
					component.graphic = next;
				}
				else
				{
					component.graphic = next + 0x0100;
				}
			}
			else
			{
				// multiply lower 7 bits of first byte by 256 and add second byte
				// to get real index
				component.graphic = (((component.graphic & 0x7F)) * 0x0100) + next;
			}
			pos += 1;
		}

		// sanity checking: if x or y is improbably high or low,
		// don't add it to the frame
		// (some games do have strange out of range values like this)
		if (std::abs(component.xoffset) <= 1024
			&& std::abs(component.yoffset) <= 1024)
		{
			frame.components.push_back(component);
		}
		else
		{
			logger.warning("Component out of defined range, skipping");
		}
	}

	frame.framenum = framenum;

	staticframes.push_back(frame);
}

void read_and_generate_aksq_dynamic_frame_components(char* data, int& pos,
	int framenum, AKSQDynamicFrameSequence& dynamicframes)
{
	int start_pos = pos;

	// read data size
	int data_size = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);
	pos += 1;

	// read component ID block size
	int compid_size = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);
	pos += 1;

	// remember position of component IDs
	int compid_pos = pos;

	// skip component ID block and read component data
	pos += compid_size;

	// read number of frame components
	int num_components = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);
	pos += 1;

	AKSQDynamicFrame frame;

	// read each component and add to frame
	for (int i = 0; i < num_components; i++)
	{
		AKSQDynamicFrameComponent component;

		// x offset
		component.xoffset = to_int(data + pos, 2, DatManip::le, DatManip::has_sign);
		pos += 2;
		// y offset
		component.yoffset = to_int(data + pos, 2, DatManip::le, DatManip::has_sign);
		pos += 2;
		// graphic index
		component.graphic = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);
		pos += 1;
		// if index has top byte set, extended component number
		if (component.graphic & 0x80)
		{
			int next = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);

			// components 128-255 must be escaped due to the top byte of the
			// component number being used as the escape sequence marker
			if (component.graphic == 0x80)
			{
				if (next >= 0x80)
				{
					component.graphic = next;
				}
				else
				{
					component.graphic = next + 0x0100;
				}
			}
			else
			{
				// multiply lower 7 bits of first byte by 256 and add second byte
				// to get real index
				component.graphic = (((component.graphic & 0x7F)) * 0x0100) + next;
			}
			pos += 1;
		}

		// component ID (associative by position: 1st component = 1st component ID)
		component.compid = to_int(data + compid_pos, 1, DatManip::le, DatManip::has_nosign);
		compid_pos += 1;

		// sanity checking: if x or y is improbably high or low,
		// don't add it to the frame
		// (some games do have strange out of range values like this)
		if (std::abs(component.xoffset) <= 1024
			&& std::abs(component.yoffset) <= 1024)
		{
			frame.components.push_back(component);
		}
		else
		{
			logger.warning("Component out of defined range, skipping");
		}
	}

	frame.framenum = framenum;

	dynamicframes.push_back(frame);

	// the data size include the size of the code and subcode, which
	// this function doesn't handle
	pos = start_pos + data_size - 2;
}

void read_and_generate_quick_aksq_dynamic_frame_components(char* data, int& pos,
	int framenum, AKSQDynamicFrameSequence& dynamicframes)
{
	int start_pos = pos;

	// read data size
	int data_size = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);
	pos += 1;

	// read component ID block size
	int compid_size = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);
	pos += 1;

	// remember position of component IDs
	int compid_pos = pos;

	// skip component ID block and read component data
	pos += compid_size;

	// number of frame components = constant 3
	int num_components = 3;

	AKSQDynamicFrame frame;

	// read each component and add to frame
	for (int i = 0; i < num_components; i++)
	{
		AKSQDynamicFrameComponent component;

		// x offset
		component.xoffset = to_int(data + pos, 2, DatManip::le, DatManip::has_sign);
		pos += 2;
		// y offset
		component.yoffset = to_int(data + pos, 2, DatManip::le, DatManip::has_sign);
		pos += 2;
		// graphic index
		component.graphic = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);
		pos += 1;
		// if index has top byte set, extended component number
		if (component.graphic & 0x80)
		{
			int next = to_int(data + pos, 1, DatManip::le, DatManip::has_nosign);

			// components 128-255 must be escaped due to the top byte of the
			// component number being used as the escape sequence marker
			if (component.graphic == 0x80)
			{
				if (next >= 0x80)
				{
					component.graphic = next;
				}
				else
				{
					component.graphic = next + 0x0100;
				}
			}
			else
			{
				// multiply lower 7 bits of first byte by 256 and add second byte
				// to get real index
				component.graphic = (((component.graphic & 0x7F)) * 0x0100) + next;
			}
			pos += 1;
		}

		// component ID (associative by position: 1st component = 1st component ID)
		if (compid_pos < compid_size)
		{
			component.compid = to_int(data + compid_pos, 1, DatManip::le, DatManip::has_nosign);
			compid_pos += 1;
		}
		else
		{
			component.compid = -1;
		}

		// sanity checking: if x or y is improbably high or low,
		// don't add it to the frame
		// (some games do have strange out of range values like this)
		if (std::abs(component.xoffset) <= 1024
			&& std::abs(component.yoffset) <= 1024)
		{
			frame.components.push_back(component);
		}
		else
		{
			logger.warning("Component out of defined range, skipping");
		}
	}

	frame.framenum = framenum;

	dynamicframes.push_back(frame);

	// the data size include the size of the code and subcode, which
	// this function doesn't handle
	pos = start_pos + data_size - 2;
}

SequenceSizingInfo compute_sequence_enclosing_dimensions(
	const AKOSComponentContainer& components, const FrameSequence& sequences)
{
	SequenceSizingInfo seqsize;

	// find the topmost, rightmost, bottommost, and leftmost extent of
	// the components in the sequence
	int topbound = 0;
	int rightbound = 0;
	int leftbound = 0;
	int bottombound = 0;

	// check static frames
	for (AKSQStaticFrameSequence::const_iterator sit = sequences.staticframes.begin();
		sit != sequences.staticframes.end(); sit++)
	{
		// examine each component
		for (AKSQStaticFrameComponentContainer::const_iterator cit
			= sit->components.begin(); cit != sit->components.end();
			cit++)
		{
			// is this component the leftmost so far?
			// sanity check: anything further left than -1024
			// is probably invalid
			if (cit->xoffset < leftbound)
			{
				leftbound = cit->xoffset;

				// update the centerpoint
				seqsize.centerx = -leftbound;
			}
			// is this component the topmost so far?
			if (cit->yoffset < topbound)
			{
				topbound = cit->yoffset;

				// update the centerpoint
				seqsize.centery = -topbound;
			}

			// is this component the rightmost so far?
			if (cit->xoffset + components[cit->graphic].get_width() > rightbound)
			{
				rightbound = cit->xoffset + components[cit->graphic].get_width();
			}
			// is this component the bottommost so far?
			if (cit->yoffset + components[cit->graphic].get_height() > bottombound)
			{
				bottombound = cit->yoffset + components[cit->graphic].get_height();
			}
		}
	}

	// check dynamic frames
	for (AKSQDynamicFrameSequence::const_iterator sit = sequences.dynamicframes.begin();
		sit != sequences.dynamicframes.end(); sit++)
	{
		// examine each component
		for (AKSQDynamicFrameComponentContainer::const_iterator cit
			= sit->components.begin(); cit != sit->components.end();
			cit++)
		{
			// is this component the leftmost so far?
			if (cit->xoffset < leftbound)
			{
				leftbound = cit->xoffset;

				// update the centerpoint
				seqsize.centerx = -leftbound;
			}
			// is this component the topmost so far?
			if (cit->yoffset < topbound)
			{
				topbound = cit->yoffset;

				// update the centerpoint
				seqsize.centery = -topbound;
			}

			// is this component the rightmost so far?
			if (cit->xoffset + components[cit->graphic].get_width() > rightbound)
			{
				rightbound = cit->xoffset + components[cit->graphic].get_width();
			}
			// is this component the bottommost so far?
			if (cit->yoffset + components[cit->graphic].get_height() > bottombound)
			{
				bottombound = cit->yoffset + components[cit->graphic].get_height();
			}
		}
	}

	seqsize.width = rightbound - leftbound;
	seqsize.height = bottombound - topbound;

	return seqsize;
}



void decode_awiz(const AWIZChunk& awizc, RipUtil::BitmapData& bmap, 
	RipUtil::BitmapPalette palette, int localtransind, int transind,
	ColorMap colormap, bool deindex)
{
	bmap.resize_pixels(awizc.width, awizc.height, 8);
	bmap.clear(transind);
	bmap.set_palettized(true);
	bmap.set_palette(palette);

	// quick'n'dirty uncompressed image detection
	if (awizc.width * awizc.height == awizc.wizd_chunk.size - 8)
	{
		int* putpos = bmap.get_pixels();
		int numpix = awizc.wizd_chunk.size - 8;
		for (int i = 0; i < numpix; i++)
			*putpos++ = to_int(awizc.wizd_chunk.data + 8 + i, 1);
	}
	// 16bpp truecolor image
	else if ((awizc.width * awizc.height * 2 == awizc.wizd_chunk.size - 8)
			&& (awizc.width != 1) && (awizc.height != 1)
			&& (awizc.wizd_chunk.size != 2)) {

		bmap.set_palettized(false);
		bmap.set_bpp(24);

		int* putpos = bmap.get_pixels();
		int numbytes = (awizc.wizd_chunk.size - 8);
		
		for (int i = 0; i < numbytes; i += 2) {
			int full = to_int(awizc.wizd_chunk.data + 8 + i, 2, DatManip::be);
			*putpos++ = decode_type2_awiz_pixel(full);
		}
	}
	else
	{
		decode_lined_rle(awizc.wizd_chunk.data + 8, awizc.wizd_chunk.datasize, bmap, 
			0, 0, awizc.width, awizc.height, localtransind, transind, true, colormap, 
			deindex);
	}
}

void decode_awiz(const AWIZChunk& awizc, RipUtil::BitmapData& bmap, 
	RipUtil::BitmapPalette palette, int localtransind, int transind)
{
	decode_awiz(awizc, bmap, palette, localtransind, transind, dummy_colormap, false);
}



void decode_char(RipUtil::BitmapData& bmap, const CHAREntry& chare,
	int compr, const RipUtil::BitmapPalette& palette, int localtransind,
	int transind, ColorMap colormap, bool deindex)
{
	LRBitStream bits(chare.data, chare.datalen);

	bmap.resize_pixels(chare.width, chare.height, 8);
	bmap.set_palettized(true);
	bmap.set_palette(palette);
	bmap.clear(transind);

	if (compr == 1 || compr == 2 || compr == 4)		// uncompressed bitmap, n bpp
	{
		int* pix = bmap.get_pixels();
		int numpix = chare.width * chare.height;
		for (int i = 0; i < numpix; i++)
		{
			int color = bits.get_nbit_int(compr);
			if (color != 0)
				*pix++ = color;
			else
				*pix++;
		}
	}
	else if (compr == 0 || compr == 8)				// lined RLE
	{
		decode_lined_rle(chare.data, chare.datalen, bmap,
			0, 0, chare.width, chare.height, localtransind, transind, true);
	}
	else
	{
		logger.error("\tunrecognized CHAR encoding " + to_string(compr));
	}
}

void decode_char(RipUtil::BitmapData& bmap, const CHAREntry& chare,
	int compr, const RipUtil::BitmapPalette& palette, int localtransind,
	int transind)
{
	decode_char(bmap, chare, compr, palette, localtransind,
		transind, dummy_colormap, false);
}



void decode_encoded_bitmap(char* data, int encoding, int datlen, RipUtil::BitmapData& bmap,
	int x, int y, int width, int height, int localtransind, int transind)
{
	bool rle = false;		// uses RLE encoding?
	bool horiz = false;		// draws horizontal or vertical?
	bool trans = false;		// has transparency?
	bool exprange = false;	// expanded range for 3-bit relative palette set?
							// ([-4, -1] and [1, 4] instead of [-4, 3])
	int bpabsol;			// bits per absolute palette set
	int bprel;				// bits per relative palette set

	if (encoding == 1 || encoding == 149)	// uncompressed: 1 byte per pixel
	{
		rle = false;
		trans = false;
	}
	else if (encoding == 8)		// RLE, with transparency
	{
		rle = true;
		trans = true;
	}
	else if (encoding == 9)		// RLE, no transparency
	{
		rle = true;
		trans = false;
	}
	else if (encoding == 143)	// same as 150??
	{
		rle = false;
		trans = false;
	}
	else if (encoding == 150)	// always 1 byte giving fill color?
	{
		rle = false;
		trans = false;
	}
	else
	{
		rle = false;

		if (encoding >= 0xE && encoding <= 0x12)		// group 2
		{
			horiz = false;
			trans = false;
		}
		else if (encoding >= 0x18 && encoding <= 0x1C)	// group 3
		{
			horiz = true;
			trans = false;
		}
		else if (encoding >= 0x22 && encoding <= 0x26)	// group 4
		{
			horiz = false;
			trans = true;
		}
		else if (encoding >= 0x2C && encoding <= 0x30)	// group 5
		{
			horiz = true;
			trans = true;
		}
		else if (encoding >= 0x40 && encoding <= 0x44)	// group 6
		{
			horiz = true;
			trans = false;
		}
		else if (encoding >= 0x54 && encoding <= 0x58)	// group 7
		{
			horiz = true;
			trans = true;
		}
		else if (encoding >= 0x68 && encoding <= 0x6C)	// group 8
		{
			horiz = true;
			trans = false;
		}
		else if (encoding >= 0x7C && encoding <= 0x80)	// group 9
		{
			horiz = true;
			trans = true;
		}
		else if (encoding >= 0x86 && encoding <= 0x8A)	// group 10
		{
			horiz = true;
			trans = false;
			exprange = true;
		}
		else if (encoding >= 0x90 && encoding <= 0x94)	// group 11
		{
			horiz = true;
			trans = true;
			exprange = true;
		}
		else
		{
			logger.error("\tunrecognized bitmap encoding " + to_string(encoding));
			return;
		}

		bpabsol = encoding % 10;

		if (encoding <= 0x30)
			bprel = 1;
		else
			bprel = 3;
	}

	if (encoding == 1 || encoding == 149)			// uncompressed
	{
		decode_uncompressed_img(data, datlen, bmap, x, y, width, height, true, false, 
			localtransind, transind);
	}
	else if (encoding == 143 || encoding == 150)	// solid fill (probably)
	{
		int fillcolor = to_int(data, 1);
		bmap.clear(fillcolor);
	}
	else if (rle)			// RLE
	{
		// see hack explanation at start of file
		if (!rle_encoding_method_hack_was_user_overriden
			&& rle_encoding_method_hack_images_to_test)
		{
			if (is_lined_rle(data, datlen))
				++rle_encoding_method_hack_lined_images;
			else
				++rle_encoding_method_hack_unlined_images;

			--rle_encoding_method_hack_images_to_test;

			RLEEncodingMethodHackValue newval;

			// set encoding method to whichever type is in majority
			// (guessing lined if a tie)
			if (rle_encoding_method_hack_lined_images
				>= rle_encoding_method_hack_unlined_images)
			{
				newval = rle_hack_always_use_lined;
			}
			else
			{
				newval = rle_hack_always_use_unlined;
			}

			if (rle_encoding_method_hack != rle_hack_is_not_set
				&& newval != rle_encoding_method_hack)
			{	
				logger.warning("changing value of RLE encoding method hack. "
					"Initial images were probably incorrectly ripped; "
					"to rip these, try using --force_lined_rle or "
					"--force_unlined_rle");
			}
			rle_encoding_method_hack = newval;
		}

		if (rle_encoding_method_hack == rle_hack_always_use_lined)
			decode_lined_rle(data, datlen, bmap, x, y, width, height,
				localtransind, transind, trans);
		else if (rle_encoding_method_hack == rle_hack_always_use_unlined)
			decode_unlined_rle(data, datlen, bmap, x, y, width, height, 
				localtransind, transind, trans); 

/*	I sure wish this code worked	*/
/*		if (is_lined_rle(data, datlen))
			decode_lined_rle(data, datlen, bmap, x, y, width, height,
				localtransind, transind, trans);
		else
			decode_unlined_rle(data, datlen, bmap, x, y, width, height, 
				localtransind, transind, trans); */
	}
	else					// bitstream
	{
		decode_bitstream_img(data, datlen, bmap, x, y, width, height,
			bpabsol, bprel, horiz, trans, exprange, localtransind, transind);
	}
}

void decode_unlined_rle(const char* data, int datlen, RipUtil::BitmapData& bmap,
	int x, int y, int width, int height, int localtransind, int transind, bool trans)
{
	decode_unlined_rle(data, datlen, bmap, x, y, width, height,
		localtransind, transind, trans, dummy_colormap, false);
}

void decode_unlined_rle(const char* data, int datlen, RipUtil::BitmapData& bmap,
	int x, int y, int width, int height, int localtransind, int transind, bool trans,
	ColorMap colormap, bool deindex)
{
	RipUtil::DrawPos pos = { 0, 0 };

	const char* gpos = data;
	while (pos.y < height)
	{
		unsigned char code = *gpos++;
		unsigned int runlen = (code >> 1) + 1;

		if (code & 1)		// encoded run
		{
			unsigned int color = *gpos++;
			if (deindex)
				color = colormap[color];
			if (trans && color == localtransind)
				color = transind;
			draw_and_update_pos(bmap, pos, color, runlen, x, y, width, height, true, false);
		}
		else				// absolute run
		{
			for (unsigned int i = 0; i < runlen; i++)
			{
				unsigned int color = *gpos++;
				if (deindex)
					color = colormap[color];
				if (trans && color == localtransind)
					color = transind;
				draw_and_update_pos(bmap, pos, color, 1, x, y, width, height, true, false);
			}
		}
	}
}

void decode_multicomp_rle(const char* data, int width, int height, RipUtil::BitmapData& bmap,
	RipUtil::BitmapPalette palette, int clrcmp, int localtransind, int transind, 
	const ColorMap& colormap, bool deindex, const ColorMap& colorremap, bool remap)
{
	if (clrcmp == 16 || clrcmp == 32 || clrcmp == 64)
	{
		int clrmask;
		int runmask;
		int clrshift;
		switch(clrcmp)
		{
		case 16:
			clrmask = 0xF0;
			runmask = 0xF;
			clrshift = 4;
			break;
		case 32:
			clrmask = 0xF8;
			runmask = 0x7;
			clrshift = 3;
			break;
		case 64:
			clrmask = 0xFC;
			runmask = 0x3;
			clrshift = 2;
			break;
		}
		
		int totalpix = width * height;
		int drawn = 0;
		int x = 0;
		int y = 0;
		while (drawn < totalpix)
		{
			int code = to_int(data++, 1);
			int color = (code & clrmask) >> clrshift;
			int runlen = code & runmask;
			if (runlen == 0)
			{
				runlen = to_int(data++, 1);
			}
			if (color != 0)
			{
				// some games index into a reduced palette instead of the full
				// 256 color range given in the palette index chunk
				if (deindex)
					color = colormap[color];
				// some games additionally remap the deindexed colors into
				// another index into the room palette
				if (remap)
					color = colorremap[color];
				RipUtil::DrawPos result = bmap.draw_col_wrap(color, runlen, x, y);
				x = result.x;
				y = result.y;
			}
			else
			{
				y += runlen;
				if (y >= height)
				{
					x += y/height;
					y = y % height;
				}
			}
			drawn += runlen;
		} 
	}
	else if (clrcmp == 256)
	{
		int x = 0;
		int y = 0;

		const char* nextstart = data;
		while (y < height)
		{
			int bytecount = to_int(nextstart, 2, DatManip::le);
			data = nextstart + 2;
			nextstart += bytecount + 2;
			while (data < nextstart)
			{
				int code = to_int(data++, 1);
				if (code & 1)		// skip count
				{
					x += (code >> 1);
				}
				else if (code & 2)	// encoded run
				{
					int count = (code >> 2) + 1;
					int color = to_int(data++, 1);
					bmap.draw_row(color, count, x, y);
					x += count;
				}
				else				// absolute run
				{
					int count = (code >> 2) + 1;
					for (int i = 0; i < count; i++)
					{
						bmap.draw_row(to_int(data++, 1), 1, x, y);
						++x;
					}
				}
			}
			x = 0;
			++y;
		}
	}
	else
	{
		logger.error("\tinvalid RLE color compression " + to_string(clrcmp));
	}
}

void decode_uncompressed_img(char* data, int datlen, RipUtil::BitmapData& bmap, int x, int y,
	int width, int height, bool horiz, bool trans, int localtransind, int transind)
{
	int remaining = width * height;
	RipUtil::DrawPos pos = { 0, 0 };
	while (remaining > 0)
	{
		int color = to_int(data++, 1);
		draw_and_update_pos(bmap, pos, color, 1, x, y, width, height, true, false);
		--remaining;
	}
}

void decode_lined_rle(const char* data, int datlen, RipUtil::BitmapData& bmap,
	int x, int y, int width, int height, int localtransind, int transind, bool trans,
	ColorMap colormap, bool deindex)
{
	int currx = 0;
	int curry = 0;

	int pos = 0;
	int next_pos = pos;
	while (pos < datlen && curry < height)
	{
		int bytecount = to_int(data + next_pos, 2, DatManip::le);
		pos = next_pos + 2;
		next_pos += bytecount + 2;
		while (pos < datlen && pos < next_pos)
		{
			int code = to_int(data + pos, 1);
			++pos;

			if (code & 1)		// skip count
			{
				currx += (code >> 1);
			}
			else if (code & 2)	// encoded run
			{
				int count = (code >> 2) + 1;
				int color = to_int(data + pos, 1);
				++pos;
				if (deindex)
					color = colormap[color];
				if (trans && color == localtransind)
					color = transind;
				bmap.draw_row(color, count, currx, curry, x, y, width, height);
				currx += count;
			}
			else				// absolute run
			{
				int count = (code >> 2) + 1;
				for (int i = 0; i < count; i++)
				{
					int color = to_int(data + pos, 1);
					if (deindex)
						color = colormap[color];
					if (trans && color == localtransind)
						color = transind;
					bmap.draw_row(color, 1, currx, curry, x, y, width, height);
					++pos;
					++currx;
				}
			}
		}
		currx = 0;
		++curry;
	}
}

void decode_lined_rle(const char* data, int datlen, RipUtil::BitmapData& bmap,
	int x, int y, int width, int height, int localtransind, int transind, bool trans)
{
	decode_lined_rle(data, datlen, bmap, x, y, width, height, 
		localtransind, transind, trans, dummy_colormap, false);
}

void decode_type2_lined_rle(const char* data, int datlen, RipUtil::BitmapData& bmap,
	int x, int y, int width, int height, int localtransind, int transind)
{
	int currx = 0;
	int curry = 0;

	int pos = 0;
	int next_pos = pos;
	while (pos < datlen && curry < height)
	{
		int bytecount = to_int(data + next_pos, 2, DatManip::le);
		pos = next_pos + 2;
		next_pos += bytecount + 2;
		while (pos < datlen && pos < next_pos)
		{
			int code = to_int(data + pos, 1);
			++pos;

			if (code & 1)		// carry over from previous image
			{					// we assume the data has already been copied and simply skip it
				currx += (code >> 1);
			}
			else if (code & 2)	// encoded run
			{
				int count = (code >> 2) + 1;
				int color = to_int(data + pos, 1);
				++pos;
				bmap.draw_row(color, count, currx, curry, x, y, width, height);
				currx += count;
			}
			else				// skip count
			{					// we draw the transparent color on top of whatever's already there
				int count = (code >> 2) + 1;
				bmap.draw_row(transind, count, currx, curry, x, y, width, height);
				currx += count;
			}
		}
		currx = 0;
		++curry;
	}
}

void decode_bitstream_img(char* data, int datlen, RipUtil::BitmapData& bmap, int x, int y,
	int width, int height, int bpabsol, int bprel, bool horiz, bool trans, bool exprange,
	int localtransind, int transind, const ColorMap& colorremap, bool remap)
{
	int remaining = width * height;
	RipUtil::DrawPos pos = { 0, 0 };

	int color = to_int(data, 1);
	int firstdrawcolor = color;
	if (remap)
		firstdrawcolor = colorremap[color];
	if (trans && firstdrawcolor == localtransind)
		firstdrawcolor = transind;
	draw_and_update_pos(bmap, pos, firstdrawcolor, 1, x, y, width, height, horiz, trans);
	--remaining;
	RLBitStream bstr(data + 1, datlen - 1);
	bool shiftisdown = true;
	while (remaining > 0)
	{
		// for each 0, draw 1 pixel of the current color
		while (remaining > 0 && bstr.get_bit() == 0)
		{
			int drawcolor = color;
			if (remap)
				drawcolor = colorremap[color];
			if (trans && drawcolor == localtransind)
				drawcolor = transind;
			draw_and_update_pos(bmap, pos, drawcolor, 1, x, y, width, height, horiz, trans);
			--remaining;
		}
		if (remaining > 0)				// we hit a 1
		{
			if (bstr.get_bit() == 0)	// 01: absolute set
			{
				int newcol = 0;
				for (int i = 0; i < bpabsol; i++)
				{
					int bit = bstr.get_bit();
					newcol |= (bit << i);
				}
				color = newcol;

				int drawcolor = color;
				if (remap)
					drawcolor = colorremap[color];
				if (trans && drawcolor == localtransind)
					drawcolor = transind;

				// draw 1 pixel of the new color
				draw_and_update_pos(bmap, pos, drawcolor, 1, x, y, width, height, horiz, trans);
				--remaining;

				// reset direction of 1-bit draw shift
				if (bprel == 1)
					shiftisdown = true;
			}
			else						// 11: relative set
			{
				int shift = 0;
				int length = 1;
				for (int i = 0; i < bprel; i++)
				{
					shift |= (bstr.get_bit() << i);
				}

				if (bprel != 1)
				{
					shift -= (1 << (bprel - 1));

					if (exprange)
					{
						if (shift >= 0)
							shift += 1;
					}
					else if (!exprange)	// 0 shift = run length
					{
						if (shift == 0)	// get 8-bit count
						{
							int newlen = 0;
							for (int i = 0; i < 8; i++)
							{
								newlen |= (bstr.get_bit() << i);
							}
							length = newlen;
						}
					}

				}
				else if (bprel == 1)	// 1-bit mode: get/set shift direction
				{
					if (shift == 1)		// toggle direction of 0 shift
					{
						shiftisdown ? shiftisdown = false
							: shiftisdown = true;
					}
					shiftisdown ? shift = -1
						: shift = 1;
				}

				color += shift;

				int drawcolor = color;
				if (remap)
					drawcolor = colorremap[color];
				if (trans && drawcolor == localtransind)
					drawcolor = transind;

				// draw pixel(s) of the new color
				draw_and_update_pos(bmap, pos, drawcolor, length, x, y, width, height, horiz, trans);
				remaining -= length;
			}
		}
	}
}

void decode_bitstream_img(char* data, int datlen, RipUtil::BitmapData& bmap, int x, int y,
	int width, int height, int bpabsol, int bprel, bool horiz, bool trans, bool exprange,
	int localtransind, int transind)
{
	decode_bitstream_img(data, datlen, bmap, x, y, width, height, bpabsol, bprel,
		horiz, trans, exprange, localtransind, transind, dummy_colormap, false);
}

int decode_type2_awiz_pixel(int full) {
/*	int r, g, b;

	r = 0;
	g = 0;
	b = 0;
	
	int cr = (full & 0x1F00) >> 5;
	int cb = (full & 0xE000) >> 8;
	int cg = (full & 0x00FC) >> 0;

	cb >>= 2;
	cb |= (full & 0x0003) << 6;

	r = cr;
	g = cb;
	b = cg; */
	
	int r = (full & 0x7C00) >> 7;
	int g = (full & 0x03E0) >> 2;
	int b = (full & 0x001F) << 3;

	return (b << 16) | (g << 8) | (r);
}

void draw_and_update_pos(RipUtil::BitmapData& bmap, DrawPos& pos, int color, int count,
	int xoff, int yoff, int width, int height, bool horiz, bool trans)
{
	if (horiz)
		pos = bmap.draw_row_wrap(color, count, pos.x, pos.y, xoff, yoff, width, height);
	else
		pos = bmap.draw_col_wrap(color, count, pos.x, pos.y, xoff, yoff, width, height);
}


};	// end of namespace Humongous
