/* Functions to convert from internally used Humongous data representation
   to more usable data types */

#include "humongous_structs.h"

#include "RipModule.h"
#include "../utils/MembufStream.h"
#include "../utils/BitmapData.h"
#include "../RipperFormats.h"
#include <string>
#include <map>
#include <vector>

namespace Humongous
{


enum RLEEncodingMethodHackValue
{ 
	rle_hack_is_not_set,
	rle_hack_always_use_lined,
	rle_hack_always_use_unlined 
};
extern RLEEncodingMethodHackValue rle_encoding_method_hack;
extern int rle_encoding_method_hack_images_to_test;
extern int rle_encoding_method_hack_lined_images;
extern int rle_encoding_method_hack_unlined_images;
extern bool rle_encoding_method_hack_was_user_overriden;

enum AKOS2ColorDecodingHackValue
{ 
	akos_2color_hack_is_not_set, 
	akos_2color_hack_always_use_rle,
	akos_2color_hack_always_use_bitmap
};

extern AKOS2ColorDecodingHackValue akos_2color_decoding_hack;
extern int akos_2color_decoding_hack_images_to_test;
extern int akos_2color_decoding_hack_rle_images;
extern int akos_2color_decoding_hack_bitmap_images;
extern bool akos_2color_decoding_hack_was_user_overriden;


// RMIM and OBIM decoding

void decode_imxx(const IMxxChunk& imxxc, RipUtil::BitmapData& bmap, int width, int height,
	int localtransind, int transind);

void decode_bmap(const SputmChunk& bmapc, RipUtil::BitmapData& bmap, int width, int height,
	int localtransind, int transind);

void decode_smap(const SputmChunk& smapc, RipUtil::BitmapData& bmap, int width, int height,
	int localtransind, int transind);

void decode_bomp(const SputmChunk& bompc, RipUtil::BitmapData& bmap, int localtransind, 
	int transind, ColorMap colormap, bool deindex);

void decode_bomp(const SputmChunk& bompc, RipUtil::BitmapData& bmap, int localtransind,
	int transind);

// AKOS decoding

void decode_akos(const AKOSChunk& akosc, RipUtil::BitmapData& bmap,
	int entrynum, RipUtil::BitmapPalette palette, int localtransind, int transind);

void decode_akos(const AKOSChunk& akosc, RipUtil::BitmapData& bmap,
	int entrynum, RipUtil::BitmapPalette palette, int localtransind, int transind,
	ColorMap colormap, bool deindex);

void decode_akos(const AKOSChunk& akosc, RipUtil::BitmapData& bmap,
	int entrynum, RipUtil::BitmapPalette palette, int localtransind, int transind,
	ColorMap colormap, bool deindex, ColorMap colorremap, bool remap);

void decode_auxd(const AUXDChunk& auxdc, RipUtil::BitmapData& bmap,
	int localtransind, int transind);

bool is_lined(const char* data, int datlen);

bool is_lined_rle(const char* data, int datlen);

// AKOS sequence ripping/interpreting

void decode_sequences(const AKSQChunk& aksqc, const AKCHChunk& akchc,
	const AKOSComponentContainer& components, FrameSequenceContainer& sequences,
	int akch_encoding);

void read_and_generate_aksq_static_frame_components(char* data, int& pos,
	int framenum, AKSQStaticFrameSequence& staticframes);

void read_and_generate_unheadered_aksq_static_frame_components(char* data, int& pos,
	int framenum, AKSQStaticFrameSequence& staticframes, int num_components);

void read_and_generate_aksq_dynamic_frame_components(char* data, int& pos,
	int framenum, AKSQDynamicFrameSequence& dynamicframes);

void read_and_generate_quick_aksq_dynamic_frame_components(char* data, int& pos,
	int framenum, AKSQDynamicFrameSequence& dynamicframes);

// calculate the size of the bitmap needed to complete enclose every
// component of every frame in a sequence
SequenceSizingInfo compute_sequence_enclosing_dimensions(
	const AKOSComponentContainer& components, const FrameSequence& sequences);

// AWIZ decoding

void decode_awiz(const AWIZChunk& awizc, RipUtil::BitmapData& bmap, 
	RipUtil::BitmapPalette palette, int localtransind, int transind,
	ColorMap colormap, bool deindex);

void decode_awiz(const AWIZChunk& awizc, RipUtil::BitmapData& bmap, 
	RipUtil::BitmapPalette palette, int localtransind, int transind);

// CHAR decoding

void decode_char(RipUtil::BitmapData& bmap, const CHAREntry& chare,
	int compr, const RipUtil::BitmapPalette& palette, int localtransind,
	int transind, ColorMap colormap, bool deindex);

void decode_char(RipUtil::BitmapData& bmap, const CHAREntry& chare,
	int compr, const RipUtil::BitmapPalette& palette, int localtransind,
	int transind);

// Low-level format decoders

// decode a standard variable-encoded image, starting after the encoding byte
// draws until the space delineated by x, y, width, and height is filled
void decode_encoded_bitmap(char* data, int encoding, int datlen, RipUtil::BitmapData& bmap,
	int x, int y, int width, int height, int localtransind, int transind);

void decode_multicomp_rle(const char* data, int width, int height, RipUtil::BitmapData& bmap,
	RipUtil::BitmapPalette palette, int clrcmp, int localtransind, int transind, 
	const ColorMap& colormap, bool deindex, const ColorMap& colorremap, bool remap);

void decode_uncompressed_img(char* data, int datlen, RipUtil::BitmapData& bmap, int x, int y,
	int width, int height, bool horiz, bool trans, int localtransind, int transind);

void decode_lined_rle(const char* data, int datlen, RipUtil::BitmapData& bmap, 
	int x, int y, int width, int height, int localtransind, int transind, bool trans);

void decode_lined_rle(const char* data, int datlen, RipUtil::BitmapData& bmap,
	int x, int y, int width, int height, int localtransind, int transind, bool trans,
	ColorMap colormap, bool deindex);

void decode_type2_lined_rle(const char* data, int datlen, RipUtil::BitmapData& bmap, 
	int x, int y, int width, int height, int localtransind, int transind);

void decode_unlined_rle(const char* data, int datlen, RipUtil::BitmapData& bmap,
	int x, int y, int width, int height, int localtransind, int transind, bool trans);

void decode_unlined_rle(const char* data, int datlen, RipUtil::BitmapData& bmap,
	int x, int y, int width, int height, int localtransind, int transind, bool trans,
	ColorMap colormap, bool deindex);

void decode_bitstream_img(char* data, int datlen, RipUtil::BitmapData& bmap, int x, int y,
	int width, int height, int bpabsol, int bprel, bool horiz, bool trans, bool exprange,
	int localtransind, int transind, const ColorMap& colorremap, bool remap);

void decode_bitstream_img(char* data, int datlen, RipUtil::BitmapData& bmap, int x, int y,
	int width, int height, int bpabsol, int bprel, bool horiz, bool trans, bool exprange,
	int localtransind, int transind);

int decode_type2_awiz_pixel(int full);

// draw according to parameters and update given DrawPos
void draw_and_update_pos(RipUtil::BitmapData& bmap, RipUtil::DrawPos& pos, int color, int count,
	int xoff, int yoff, int width, int height, bool horiz, bool trans);


};	// end of namespace Humongous

#pragma once