#include "BitmapData.h"

#include "datmanip.h"
#include "DefaultException.h"
#include <cstring>
#include <fstream>
#include <cmath>

namespace RipUtil
{


BitmapData& BitmapData::operator=(const BitmapData& bmap)
{
	delete[] pixels;
	pixels = new int[bmap.allocation_size];
	for (int i = 0; i < bmap.allocation_size; i++)
		pixels[i] = bmap.pixels[i];
	width = bmap.width;
	height = bmap.height;
	bpp = bmap.bpp;
	allocation_size = bmap.allocation_size;
	palettized = bmap.palettized;
	palette = bmap.palette;

	return *this;
}

const void BitmapData::copy_rect(BitmapData& copy, int x, int y, int w, int h)
{
	copy.resize_pixels(w, h, bpp);
	if (palettized)
	{
		copy.set_palettized(true);
		copy.set_palette(palette);
	}
	else
		copy.set_palettized(false);
	int* getpos = pixels;
	int* putpos = copy.get_pixels();
	for (int i = y; i < y + h; i++)
	{
		getpos = pixels + (i * width) + x;
		for (int j = x; j < x + w; j++)
		{
			*putpos++ = *getpos++;
		}
	}
}

void BitmapData::resize_pixels(int w, int h, int bits)
{
	delete[] pixels;
	allocation_size = w * h;
	pixels = new int[allocation_size];
	width = w;
	height = h;
	bpp = bits;
}

void BitmapData::set_palette_8bit_grayscale()
{
	for (int i = 0; i < 256; i++)
	{
		unsigned int color = 0;
		color |= i;
		color |= i << 8;
		color |= i << 16;
		palette[i] = color;
	}
}

void BitmapData::clear()
{
	for (int i = 0; i < allocation_size; i++)
		pixels[i] = 0;
}

void BitmapData::clear(int color)
{
	for (int i = 0; i < allocation_size; i++)
		pixels[i] = color;
}

int BitmapData::draw_row(int color, int count, int x, int y)
{
	return draw_row(color, count, x, y, 0, 0, width, height);
}

DrawPos BitmapData::draw_row_wrap(int color, int count, int x, int y)
{
	return draw_row_wrap(color, count, x, y, 0, 0, width, height);
}

int BitmapData::draw_col(int color, int count, int x, int y)
{
	return draw_col(color, count, x, y, 0, 0, width, height);
}

DrawPos BitmapData::draw_col_wrap(int color, int count, int x, int y)
{
	return draw_col_wrap(color, count, x, y, 0, 0, width, height);
}

int BitmapData::draw_row(int color, int count, int x, int y,
	int boxx, int boxy, int boxw, int boxh)
{
	if (x < 0 || x >= boxw 
		|| y < 0 || y >= boxh
		|| boxx < 0 || boxy < 0
		|| count < 0)
		return count;

	// bound drawing at the end of the row
	int drawcount = count - std::max(0, x + count - (boxx + boxw));
	int startpos = boxx + x + (y + boxy) * width;
	for (int i = 0; i < drawcount; i++)
		pixels[startpos + i] = color;

	return count - drawcount;
}

DrawPos BitmapData::draw_row_wrap(int color, int count, int x, int y,
	int boxx, int boxy, int boxw, int boxh)
{
	DrawPos d = { x, y };
	if (x < 0 || x >= boxw 
		|| y < 0 || y >= boxh
		|| boxx < 0 || boxy < 0
		|| count < 0)
		return d;

	// take the minimum of pixels requested for drawing and pixels to end of box
	int totalpix = std::min(count, (boxw - x) + (boxw * (boxh - y)));
	int currx = x;
	int curry = y;
	while (totalpix > 0)
	{
		int drawcount = std::min(totalpix, boxw - currx);
		int cutoff = draw_row(color, drawcount, currx, curry, boxx, boxy, boxw, boxh);
		currx += drawcount;
		if (currx >= boxw)
		{
			curry += currx/boxw;
			currx = currx % boxw;
		}
		totalpix -= drawcount;
	}
	d.x = currx;
	d.y = curry;
	return d;
}

int BitmapData::draw_col(int color, int count, int x, int y,
	int boxx, int boxy, int boxw, int boxh)
{
	if (x < 0 || x >= boxw 
		|| y < 0 || y >= boxh
		|| boxx < 0 || boxy < 0
		|| count < 0)
		return count;

	int drawcount = count - std::max(0, y + count - (boxy + boxh));
	int startpos = boxx + x + (y + boxy) * width;
	for (int i = 0; i < drawcount; i++)
		pixels[startpos + width * i] = color;

	return count - drawcount;
}

DrawPos BitmapData::draw_col_wrap(int color, int count, int x, int y,
	int boxx, int boxy, int boxw, int boxh)
{
	DrawPos d = { x, y };
	if (x < 0 || x >= boxw 
		|| y < 0 || y >= boxh
		|| boxx < 0 || boxy < 0
		|| count < 0)
		return d;
	
	int totalpix = std::min(count, (boxh - y) + (boxh * (boxw - x)));
	int currx = x;
	int curry = y;
	while (totalpix > 0)
	{
		int drawcount = std::min(totalpix, boxh - curry);
		int cutoff = draw_col(color, drawcount, currx, curry, boxx, boxy, boxw, boxh);
		curry += drawcount;
		if (curry >= height)
		{
			currx += curry/height;
			curry = curry % height;
		}
		totalpix -= drawcount;
	}
	d.x = currx;
	d.y = curry;
	return d;
}

void BitmapData::blit_bitmapdata(BitmapData& bmpdat, int xpos, int ypos)
{
	// if no overlap, do nothing
	if (xpos >= width
		|| ypos >= height
		|| xpos + bmpdat.get_width() < 0 
		|| ypos + bmpdat.get_height() < 0)
		return;

	// get coordinate within source data of top left corner of blit
	int xdiff = std::max(0, -xpos);
	int ydiff = std::max(0, -ypos);
	// get coordinate within destination data of top left corner of blit
	int thisx = std::max(0, xpos);
	int thisy = std::max(0, ypos);

	// calculate cutoff on each edge
	int lcut = xdiff;
	int rcut = std::max(0, xpos + bmpdat.get_width() - width);
	int tcut = ydiff;
	int bcut = std::max(0, ypos + bmpdat.get_height() - height);

	// calculate width and height of blit area from edge cutoff
	int bwidth = bmpdat.get_width() - lcut - rcut;
	int bheight = bmpdat.get_height() - tcut - bcut;

	// copy pixel rows
	int* source = bmpdat.get_pixels();
	source += xdiff + bmpdat.get_width() * ydiff;
	int* dest = pixels;
	dest += thisx + width * thisy;
	for (int i = 0; i < bheight; i++)
	{
		for (int j = 0; j < bwidth; j++)
		{
			*(dest + j) = *(source + j);
		}
		source += bmpdat.get_width();
		dest += width;
	}
}

void BitmapData::blit_bitmapdata(BitmapData& bmpdat, int xpos, int ypos,
	int transcolor)
{
	// if no overlap, do nothing
	if (xpos >= width
		|| ypos >= height
		|| xpos + bmpdat.get_width() < 0 
		|| ypos + bmpdat.get_height() < 0)
		return;

	// get coordinate within source data of top left corner of blit
	int xdiff = std::max(0, -xpos);
	int ydiff = std::max(0, -ypos);
	// get coordinate within destination data of top left corner of blit
	int thisx = std::max(0, xpos);
	int thisy = std::max(0, ypos);

	// calculate cutoff on each edge
	int lcut = xdiff;
	int rcut = std::max(0, xpos + bmpdat.get_width() - width);
	int tcut = ydiff;
	int bcut = std::max(0, ypos + bmpdat.get_height() - height);

	// calculate width and height of blit area from edge cutoff
	int bwidth = bmpdat.get_width() - lcut - rcut;
	int bheight = bmpdat.get_height() - tcut - bcut;

	// copy pixel rows
	int* source = bmpdat.get_pixels();
	source += xdiff + bmpdat.get_width() * ydiff;
	int* dest = pixels;
	dest += thisx + width * thisy;
	for (int i = 0; i < bheight; i++)
	{
		for (int j = 0; j < bwidth; j++)
		{
			if (*(source + j) != transcolor)
			{
				*(dest + j) = *(source + j);
			}
		}
		source += bmpdat.get_width();
		dest += width;
	}
}

void write_bmp_header(std::ofstream& ofs, const BMPHeader& bmphd)
{
	// file header
	ofs.write(bmphd.bmp_filehd_type, 2);
	ofs.write(bmphd.bmp_filehd_size, 4);
	ofs.write(bmphd.bmp_filehd_reserved1, 2);
	ofs.write(bmphd.bmp_filehd_reserved2, 2);
	ofs.write(bmphd.bmp_filehd_offbits, 4);

	// info header
	ofs.write(bmphd.bmp_infohd_size, 4);
	ofs.write(bmphd.bmp_infohd_width, 4);
	ofs.write(bmphd.bmp_infohd_height, 4);
	ofs.write(bmphd.bmp_infohd_planes, 2);
	ofs.write(bmphd.bmp_infohd_bitcount, 2);
	ofs.write(bmphd.bmp_infohd_compression, 4);
	ofs.write(bmphd.bmp_infohd_sizeimage, 4);
	ofs.write(bmphd.bmp_infohd_xpelsm, 4);
	ofs.write(bmphd.bmp_infohd_ypelsm, 4);
	ofs.write(bmphd.bmp_infohd_clrused, 4);
	ofs.write(bmphd.bmp_infohd_clrimp, 4);
}

void write_bitmapdata_bmp(BitmapData& bmpdat, const std::string& filename)
{
	BMPHeader bmphd;
	
	int infohd_size = 40;
	int offbits = 14 + infohd_size;
	int bitcount = 24;
	int sizeimage = bmpdat.get_width() * bmpdat.get_height();
	int fsize = offbits + sizeimage;
	int xpels = 0;
	int ypels = 0;

	to_bytes(bmpdat.get_width(), bmphd.bmp_infohd_width, 4, DatManip::le);
	to_bytes(bmpdat.get_height(), bmphd.bmp_infohd_height, 4, DatManip::le);
	to_bytes(fsize, bmphd.bmp_filehd_size, 4, DatManip::le);
	to_bytes(offbits, bmphd.bmp_filehd_offbits, 4, DatManip::le);
	to_bytes(infohd_size, bmphd.bmp_infohd_size, 4, DatManip::le);
	to_bytes(bitcount, bmphd.bmp_infohd_bitcount, 2, DatManip::le);
	to_bytes(sizeimage, bmphd.bmp_infohd_sizeimage, 4, DatManip::le);
	to_bytes(xpels, bmphd.bmp_infohd_xpelsm, 4, DatManip::le);
	to_bytes(ypels, bmphd.bmp_infohd_ypelsm, 4, DatManip::le);

	// file header
	std::ofstream ofs(filename.c_str(), std::ios_base::binary);
	write_bmp_header(ofs, bmphd);

	// pixel data
	for (int i = bmpdat.get_height() - 1; i >= 0; i--)
	{
		int* rowstart = bmpdat.get_pixels() + i * bmpdat.get_width();
		for (int j = 0; j < bmpdat.get_width(); j++)
		{
			int r = 0;
			int g = 0;
			int b = 0;
			int output = 0;
			if (bmpdat.get_palettized())
			{
				unsigned int palettecolor = *(rowstart + j);
				unsigned int color = bmpdat.get_palette()[palettecolor];
				r = (color & 0xFF);
				g = (color & 0xFF00) >> 8;
				b = (color & 0xFF0000) >> 16;
			}
			else
			{
				unsigned int color = *(rowstart + j);
				switch(bmpdat.get_bpp())
				{
				case 8:
					r = color & 3;
					g = color & (3 << 2);
					b = color & (3 << 4);
					break;
				case 16:
					r = (color & 0xF);
					g = (color & 0xF0) >> 4;
					b = (color & 0xF00) >> 8;
					break;
				case 24: case 32:
					r = (color & 0xFF);
					g = (color & 0xFF00) >> 8;
					b = (color & 0xFF0000) >> 16;
					break;
				default:
					throw(DefaultException("tried to save image with invalid bpp"));
				}
			}
			output |= r;
			output |= (g << 8);
			output |= (b << 16);
			char outbytes[3];
			to_bytes(output, outbytes, 3);
			ofs.write(outbytes, 3);
		}
		// pad line to 4-byte boundary
		int padbytes = bmpdat.get_width() % 4;
		for (int j = 0; j < padbytes; j++)
		{
			ofs.put(0);
		}
	}
}

void write_bitmapdata_8bitpalettized_bmp(BitmapData& bmpdat, const std::string& filename)
{
	BMPHeader bmphd;
	char colortable[1024];
	
	int infohd_size = 40;
	int offbits = 14 + infohd_size + 1024;
	int bitcount = 8;
	int sizeimage = bmpdat.get_width() * bmpdat.get_height();
	int fsize = offbits + sizeimage;
	int xpels = 0;
	int ypels = 0;
	int clrused = BMPWriterConsts::max_8bit_colors;
	int clrimp = BMPWriterConsts::max_8bit_colors;

	// fill colortable with colors from palette
	for (int i = 0; i < 1024; i += 4)
	{
		colortable[i + 3] = 0;
		int color = bmpdat.get_palette()[i/4];
		colortable[i + 2] = color & 0xFF;
		colortable[i + 1] = (color & 0xFF00) >> 8;
		colortable[i] = (color & 0xFF0000) >> 16;
	}

	to_bytes(bmpdat.get_width(), bmphd.bmp_infohd_width, 4, DatManip::le);
	to_bytes(bmpdat.get_height(), bmphd.bmp_infohd_height, 4, DatManip::le);
	to_bytes(fsize, bmphd.bmp_filehd_size, 4, DatManip::le);
	to_bytes(offbits, bmphd.bmp_filehd_offbits, 4, DatManip::le);
	to_bytes(infohd_size, bmphd.bmp_infohd_size, 4, DatManip::le);
	to_bytes(bitcount, bmphd.bmp_infohd_bitcount, 2, DatManip::le);
	to_bytes(sizeimage, bmphd.bmp_infohd_sizeimage, 4, DatManip::le);
	to_bytes(xpels, bmphd.bmp_infohd_xpelsm, 4, DatManip::le);
	to_bytes(ypels, bmphd.bmp_infohd_ypelsm, 4, DatManip::le);
	to_bytes(clrused, bmphd.bmp_infohd_clrused, 4, DatManip::le);
	to_bytes(clrimp, bmphd.bmp_infohd_clrimp, 4, DatManip::le);

	std::ofstream ofs(filename.c_str(), std::ios_base::binary);

	write_bmp_header(ofs, bmphd);
	ofs.write(colortable, BMPWriterConsts::max_8bit_colors * 4);

	// pixel data
	for (int i = bmpdat.get_height() - 1; i >= 0; i--)
	{
		int* rowstart = bmpdat.get_pixels() + i * bmpdat.get_width();
		for (int j = 0; j < bmpdat.get_width(); j++)
		{
			
			char color = *(rowstart + j);
			ofs.put(color);
		}
		// pad line to 4-byte boundary
		int padbytes = bmpdat.get_width() % 4;
		if (padbytes != 0)
			for (int j = 0; j < 4 - padbytes; j++)
				ofs.put(0);
	}
}

void BitmapData::write(const std::string& filename) {
	if (palettized && bpp == 8) {
		write_bitmapdata_8bitpalettized_bmp(*this, filename);
	}
	else {
		write_bitmapdata_bmp(*this, filename);
	}
}


};	// end namespace RipUtil
