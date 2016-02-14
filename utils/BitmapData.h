/* Container for various bitmapped data formats */

#include <string>
#include <map>
#include <cstring>

namespace RipUtil
{


// a BitmapPalette defines the relation of an int in the range
// 2^bpp - 1 to a 24 bit little endian RGB value
typedef std::map<int, int> BitmapPalette;

struct DrawPos
{
	int x;
	int y;
};

class BitmapData
{
public:
	BitmapData()
		: pixels(0), width(0), height(0), bpp(0),
		allocation_size(0), palettized(0) { };
	BitmapData(int w, int h, int bits, bool pal = false)
		: pixels(0), width(w), height(h), palettized(pal)
	{
		resize_pixels(width, height, bits);
	}
	BitmapData(const BitmapData& b)
		: pixels(new int[b.allocation_size]), width(b.width), height(b.height),
		bpp(b.bpp), allocation_size(b.allocation_size), palettized(b.palettized)
	{
		std::memcpy(pixels, b.pixels, allocation_size * sizeof(int));
	}
	~BitmapData()
	{
		delete[] pixels;
	}
	BitmapData& operator=(const BitmapData& bmap);
	int* get_pixels() { return pixels; }
	int get_width() const { return width; }
	int get_height() const { return height; }
	int get_bpp() const { return bpp; }
	int get_allocation_size() const { return allocation_size; }
	bool get_palettized() const { return palettized; }
	BitmapPalette& get_palette() { return palette; }

	void set_height(int newheight) { height = newheight; }
	void set_width(int newwidth) { width = newwidth; }
	void set_bpp(int newbpp) { bpp = newbpp; }
	void set_palettized(bool newpalettized) { palettized = newpalettized; }
	void set_palette(const BitmapPalette& newpalette) { palette = newpalette; }

	// copy the specified portion of the image into an existing BitmapData object
	const void copy_rect(BitmapData& copy, int x, int y, int w, int h);
	// destroy existing pixel data and resize to given width/height/bpp
	void resize_pixels(int w, int h, int bits);
	// set palette to 8-bit linear grayscale
	void set_palette_8bit_grayscale();
	// erase all pixel data
	void clear();
	// set all pixel data to given color
	void clear(int color);
	// draw a color horizontally or vertically for n pixels, cutting off at end of row
	// "box" parameters specify a bounding box for the operation
	// return value: number of pixels "clipped" off end
	int draw_row(int color, int count, int x, int y);
	int draw_col(int color, int count, int x, int y);
	int draw_row(int color, int count, int x, int y,
		int boxx, int boxy, int boxw, int boxh);
	int draw_col(int color, int count, int x, int y,
		int boxx, int boxy, int boxw, int boxh);
	// draw a color horizontally or vertically for n pixels, wrapping across rows
	// "box" parameters specify a bounding box for the operation
	// return value: DrawPos giving new draw position
	DrawPos draw_row_wrap(int color, int count, int x, int y);
	DrawPos draw_col_wrap(int color, int count, int x, int y);
	DrawPos draw_row_wrap(int color, int count, int x, int y,
		int boxx, int boxy, int boxw, int boxh);
	DrawPos draw_col_wrap(int color, int count, int x, int y,
		int boxx, int boxy, int boxw, int boxh);
	// blit pixel data of a BitmapData object onto this one,
	// clipping as necessary
	void blit_bitmapdata(BitmapData& bmpdat, int xpos, int ypos);
	// blit pixel data of a BitmapData object onto this one,
	// clipping as necessary and omitting pixels of the given
	// transparent color
	void blit_bitmapdata(BitmapData& bmpdat, int xpos, int ypos,
		int transcolor);
	
	void write(const std::string& filename);

private:
	int* pixels;
	int width;
	int height;
	int bpp;
	int allocation_size;
	bool palettized;
	BitmapPalette palette;
};

namespace BMPWriterConsts
{
	const static char bmp_hd_id[2]
	= { 'B', 'M' };

	const static int max_8bit_colors = 256;
};

// this is what we should be using instead of BMPHeader
struct BMPDataHeader
{
	char filehd_type[2];
	int filehd_size;
	int filehd_reserved1;
	int filehd_reserved2;
	int filehd_offbits;

	int infohd_size;
	int infohd_width;
	int infohd_height;
	int infohd_planes;
	int infohd_bitcount;
	int infohd_compression;
	int infohd_sizeimage;
	int infohd_xpelsm;
	int infohd_ypelsm;
	int infohd_clrused;
	int infohd_clrimp;
};

struct BMPHeader
{
	BMPHeader() 
	{ 
		std::memcpy(bmp_filehd_type, BMPWriterConsts::bmp_hd_id, 2);
		std::memset(bmp_filehd_size, 0, 4);
		std::memset(bmp_filehd_reserved1, 0, 2);
		std::memset(bmp_filehd_reserved2, 0, 2);
		std::memset(bmp_filehd_offbits, 0, 4);

		std::memset(bmp_infohd_size, 0, 4);
		std::memset(bmp_infohd_width, 0, 4);
		std::memset(bmp_infohd_height, 0, 4);
		std::memset(bmp_infohd_planes, 0, 2);
		bmp_infohd_planes[0] = 1;
		std::memset(bmp_infohd_bitcount, 0, 2);
		std::memset(bmp_infohd_compression, 0, 4);
		std::memset(bmp_infohd_sizeimage, 0, 4);
		std::memset(bmp_infohd_xpelsm, 0, 4);
		std::memset(bmp_infohd_ypelsm, 0, 4);
		std::memset(bmp_infohd_clrused, 0, 4);
		std::memset(bmp_infohd_clrimp, 0, 4);
	}

	// BMP file header
	char bmp_filehd_type[2];
	char bmp_filehd_size[4];
	char bmp_filehd_reserved1[2];
	char bmp_filehd_reserved2[2];
	char bmp_filehd_offbits[4];

	// BMP info header
	char bmp_infohd_size[4];
	char bmp_infohd_width[4];
	char bmp_infohd_height[4];
	char bmp_infohd_planes[2];
	char bmp_infohd_bitcount[2];
	char bmp_infohd_compression[4];
	char bmp_infohd_sizeimage[4];
	char bmp_infohd_xpelsm[4];
	char bmp_infohd_ypelsm[4];
	char bmp_infohd_clrused[4];
	char bmp_infohd_clrimp[4];
};


void write_bmp_header(std::ofstream& ofs, const BMPHeader& bmphd);

void write_bitmapdata_bmp(BitmapData& bmpdat, const std::string& filename);

void write_bitmapdata_8bitpalettized_bmp(BitmapData& bmpdat, const std::string& filename);


};	// end namespace RipUtil


#pragma once