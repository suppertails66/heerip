#include "common.h"
#include "../utils/MembufStream.h"
#include "../utils/BitmapData.h"
#include "../utils/PCMData.h"
#include "../utils/IMAADPCMDecoder.h"
#include "../utils/BitStream.h"
#include "../utils/datmanip.h"

#include <iostream>

using namespace RipUtil;

namespace CommFor
{

	namespace aiff
	{




	};	// end of namespace aiff

	namespace riff
	{


		ChunkType getchunktype(const std::string& id)
		{
			if (id.size() != 4)
				return chunk_unknown;

			char cid[4];
			std::memcpy(cid, id.c_str(), 4);

			if (quickcmp(cid, "RIFF", 4))
				return chunk_riff;
			else if (quickcmp(cid, "WAVE", 4))
				return chunk_wave;
			else if (quickcmp(cid, "fmt ", 4))
				return chunk_fmt;
			else if (quickcmp(cid, "data", 4))
				return chunk_data;
			else if (quickcmp(cid, "fact", 4))
				return chunk_fact;
			else if (quickcmp(cid, "LIST", 4))
				return chunk_list;
			else if (quickcmp(cid, "IENG", 4))
				return chunk_ieng;
			else if (quickcmp(cid, "ISFT", 4))
				return chunk_isft;
			else if (quickcmp(cid, "cue ", 4))
				return chunk_cue;
			else if (quickcmp(cid, "adtl", 4))
				return chunk_adtl;
			else if (quickcmp(cid, "ltxt", 4))
				return chunk_ltxt;
			else if (quickcmp(cid, "rgn ", 4))
				return chunk_rgn;
			else if (quickcmp(cid, "labl", 4))
				return chunk_labl;
			else if (quickcmp(cid, "INFO", 4))
				return chunk_info;
			else if (quickcmp(cid, "ICRD", 4))
				return chunk_icrd;
			else
				return chunk_unknown;
		}

		void read_riff_chunkhead(const char* data, int address,
			RIFFChunkHead& riffc)
		{
			riffc.address = address;
			const char* datapos = data + address;
			int pos = address;
			char chunkid[5];
			chunkid[4] = 0;
			std::memcpy(chunkid, datapos, 4);

			std::string namestr(chunkid);
			riffc.name = namestr;
			riffc.type = getchunktype(namestr);
			pos += 4;

			riffc.size = to_int(data + pos, 4, DatManip::le) + pos + 4 - address;
			riffc.dataddress = pos + 4;
		}

		void decode_riff_uncompressed(const char* data, int datalen,
			RipUtil::PCMData& wave)
		{
			wave.resize_wave(datalen);

			if (wave.get_sampwidth() == 8)
				wave.set_signed(DatManip::has_nosign);
			else
				wave.set_signed(DatManip::has_sign);
			std::memcpy(wave.get_waveform(), data, datalen);
		}

		void decode_riff_imaadpcm(const char* data, int datalen,
			RipUtil::PCMData& wave, int nibsperblock)
		{
			// always decode to 16 bits per sample, signed
			wave.set_sampwidth(16);
			wave.set_signed(DatManip::has_sign);

			// set decoding constants
			const int sampbytesperblock = nibsperblock/2;
			const int numblocks = datalen/(sampbytesperblock + 4);

			wave.resize_wave(((sampbytesperblock) * 4 + 2) * numblocks);

			const char* gpos = data;
			char* ppos = wave.get_waveform();
			IMAADPCMDecoder dec;
			for (int i = 0; i < numblocks; i++)
			{
				// get next predicted sample and index
				int nextpred = to_int(gpos, 2, DatManip::le);
				gpos += 2;
				int nextind = to_int(gpos, 1);
				gpos += 2;

				// convert unsigned to signed
				if (nextpred > 0x8000)
					nextpred -= 0x10000;

				// set first sample in block to predicted sample
				*ppos++ = nextpred & 0xFF;
				*ppos++ = (nextpred & 0xFF00) >> 8;

				// prepare decoder for samples in next block
				dec.set_predictedSample(nextpred);
				dec.set_index(clamp(nextind, 0, 88));

				// decode block samples
				for (int j = 0; j < sampbytesperblock; j++)
				{
					char next = *gpos++;
					char left = (next & 0xF0) >> 4;
					char right = (next & 0xF);

					int decoded = dec.decode_samp(right);
					*ppos++ = decoded & 0xFF;
					*ppos++ = (decoded & 0xFF00) >> 8;

					decoded = dec.decode_samp(left);
					*ppos++ = decoded & 0xFF;
					*ppos++ = (decoded & 0xFF00) >> 8;
				}
			}
		}

		void decode_riff(const char* riffdat, int riffdatlen,
			RipUtil::PCMData& wave)
		{
			int format = 0;
			int nibsperblock = 0;

			RIFFChunkHead hdcheck;
			int nextpos = 0;
			while (nextpos < riffdatlen)
			{
				read_riff_chunkhead(riffdat, nextpos, hdcheck);
				nextpos = hdcheck.nextaddr();
				switch (hdcheck.type)
				{
				case chunk_riff:
					nextpos = hdcheck.address + 12;
					break;
				case chunk_fmt:
					format = to_int(riffdat + hdcheck.dataddress, 2, DatManip::le);
					wave.set_channels(to_int(riffdat + hdcheck.dataddress + 2, 2, DatManip::le));
					wave.set_samprate(to_int(riffdat + hdcheck.dataddress + 4, 4, DatManip::le));
					wave.set_sampwidth(to_int(riffdat + hdcheck.dataddress + 14, 2, DatManip::le));
					if (format == 17)
						nibsperblock = to_int(riffdat + hdcheck.dataddress + 18, 2, DatManip::le);
					break;
				case chunk_fact:
					break;
				case chunk_data:	// assumed to skip if in a LIST
					if (format == 17)
					{
						decode_riff_imaadpcm(riffdat + hdcheck.dataddress, hdcheck.size - 8,
							wave, nibsperblock);
					}
					else if (format == 1)
					{
						decode_riff_uncompressed(riffdat + hdcheck.dataddress, hdcheck.size - 8,
							wave);
					}
					break;
				// skip everything else
				case chunk_list:
					break;
				case chunk_ieng:
					break;
				case chunk_isft:
					break;
				case chunk_cue:
					break;
				case chunk_ltxt:
					break;
				case chunk_rgn:
					break;
				case chunk_labl:
					break;
				case chunk_icrd:
					break;
				default:
					break;
				}
			}
		}


	};	// end of namespace riff

	namespace bmp
	{


		const int bmp_bi_rgb = 0;
		const int bmp_bi_rle8 = 1;
		const int bmp_bi_rle4 = 2;

		void read_bmp_bitmapdata(RipUtil::MembufStream& stream,
			RipUtil::BitmapData& dat)
		{
			int datastart = stream.tellg();

			BMPDataHeader header;
			stream.read(header.filehd_type, 2);
			header.filehd_size = stream.read_int(4, DatManip::le);
			header.filehd_reserved1 = stream.read_int(2, DatManip::le);
			header.filehd_reserved2 = stream.read_int(2, DatManip::le);
			header.filehd_offbits = stream.read_int(4, DatManip::le);

			header.infohd_size = stream.read_int(4, DatManip::le);
			header.infohd_width = stream.read_int(4, DatManip::le);
			header.infohd_height = stream.read_int(4, DatManip::le);
			header.infohd_planes = stream.read_int(2, DatManip::le);
			header.infohd_bitcount = stream.read_int(2, DatManip::le);
			header.infohd_compression = stream.read_int(4, DatManip::le);
			header.infohd_sizeimage = stream.read_int(4, DatManip::le);
			header.infohd_xpelsm = stream.read_int(4, DatManip::le);
			header.infohd_ypelsm = stream.read_int(4, DatManip::le);
			header.infohd_clrused = stream.read_int(4, DatManip::le);
			header.infohd_clrimp = stream.read_int(4, DatManip::le);

			stream.seekg(datastart + header.infohd_size + 14);

			datastart += header.filehd_offbits;

			if (header.infohd_bitcount <= 8)
			{

				BitmapPalette palette;
				// if color num not specified, assume from bpp
				if (header.infohd_clrused == 0 && header.infohd_bitcount == 1)
					header.infohd_clrused = 2;
				else if (header.infohd_clrused == 0 && header.infohd_bitcount == 4)
					header.infohd_clrused = 16;
				else if (header.infohd_clrused == 0 && header.infohd_bitcount == 8)
					header.infohd_clrused = 256;
				dat.set_palettized(true);
				// read the color table
				for (int i = 0; i < header.infohd_clrused; i++)
				{
					int color = 0;
					color |= (stream.read_int(1) << 16);
					color |= (stream.read_int(1) << 8);
					color |= stream.read_int(1);
					stream.seek_off(1);
					palette[i] = color;
				}
				dat.set_palette(palette);
			}

			dat.resize_pixels(header.infohd_width, header.infohd_height,
				header.infohd_bitcount);
			std::memset(dat.get_pixels(), 0, dat.get_allocation_size());

			stream.seekg(datastart);

			switch(header.infohd_bitcount)
			{
			case 1:
				{
					for (int i = dat.get_height() - 1; i >= 0; i--)
					{
						int* putpos = dat.get_pixels() + dat.get_width() * i;
						int remaining = dat.get_width();
						while (remaining > 0)
						{
							int byte = stream.read_int(1);
							for (int k = 0x80; k > 0; k /= 2)
							{
								if (remaining > 0)
								{
									if ((byte & k))
										*putpos++ = 1;
									else
										*putpos++ = 0;
								}
								--remaining;
							}
						}
						// skip pad bytes
						int numbytes = dat.get_width()/8;
						if (dat.get_width() % 8)
							numbytes += 1;
						if (numbytes % 4)
							stream.seek_off(4 - (numbytes % 4));
					}
				}
				break;
			case 4: case 8:

				switch (header.infohd_compression)
				{
				case bmp_bi_rgb:
					for (int i = dat.get_height() - 1; i >= 0; i--)
					{
						int* putpos = dat.get_pixels() + i * dat.get_width();
						int remaining = dat.get_width();
						while (remaining > 0)
						{
							int byte = stream.read_int(1);
							if (dat.get_bpp() == 8)
							{
								*putpos++ = byte;
							}
							else
							{
								*putpos++ = (byte & 0xF0) >> 4;
								*putpos++ = byte & 0xF;
							}
							--remaining;
						}
						if (dat.get_width() % 4)
							stream.seek_off(4 - dat.get_width() % 4);
					}
					break;
				case bmp_bi_rle8: case bmp_bi_rle4:

					bool eob = false;

					for (int i = dat.get_height() - 1; i >= 0; i--)
					{
						if (eob)
							break;

						int* putpos = dat.get_pixels() + dat.get_width() * i;
						int remaining = dat.get_width();

						while (!eob)
						{
							int code = stream.read_int(1);
					
							if (code == 0)
							{
								int val = stream.read_int(1);

								if (val == 0)
								{
									// end of line
									remaining = -1;
									break;
								}
								else if (val == 1)
								{
									// end of bitmap
									remaining = -1;
									eob = true;
									break;
								}
								else if (val == 2)
								{
									// delta
									int xoff = stream.read_int(1);
									int yoff = stream.read_int(1);
									putpos += xoff;
									i += yoff;
								}
								else
								{
									// absolute pixel run
									if (header.infohd_compression == bmp_bi_rle8)
									{
										for (int j = 0; j < val; j++)
										{
											if (!(remaining <= 0))
											{
												int byte = stream.read_int(1);
												*(putpos++) = byte;
											}
											--remaining;
										}
										if (val % 2)
											stream.seek_off(1);
									}
									else if (header.infohd_compression == bmp_bi_rle4)
									{
										int byte;
										for (int j = 0; j < val; j++)
										{
											if (!(j % 2))
												byte = stream.read_int(1);
											if (!(remaining <= 0))
											{
												if (j % 2)
												{
													*(putpos++) = byte & 0xF;
												}
												else
												{
													*(putpos++) = (byte & 0xF0) >> 4;
												}
											}
											--remaining;
										}
										if (val % 4 == 2)
											stream.seek_off(1);
										else if (val % 4 == 1)
											stream.seek_off(1);
									}
								}

							}
							else
							{
								// encoded pixel run
								int byte = stream.read_int(1);
								for (int j = 0; j < code; j++)
								{
									if (header.infohd_compression == bmp_bi_rle8)
									{
										if (!(remaining <= 0))
										{
											*(putpos++) = byte;
											--remaining;
										}
									}
									else if (header.infohd_compression == bmp_bi_rle4)
									{
										if (!(remaining <= 0))
										{
											if (j % 2)
												*(putpos++) = (byte & 0xF);
											else
												*(putpos++) = (byte & 0xF0) >> 4;
										}
										--remaining;
									}
								}
							}
						}
					}

					break;
				}

				break;
			case 24:
				for (int i = 0; i < dat.get_height(); i++)
				{
					int* putpos = dat.get_pixels() + dat.get_width() * i;
					int remaining = dat.get_width();
					while (remaining > 0)
					{
						int pixel = stream.read_int(4, DatManip::le);
						*putpos++ = pixel;
						--remaining;
					}
					if (dat.get_width() % 4)
						stream.seek_off(4 - dat.get_width() % 4);
				}
				break;
			}
		}


	};	// end of namespace bmp


};	// end of namespace CommFor
