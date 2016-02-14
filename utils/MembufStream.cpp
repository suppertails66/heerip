#include "MembufStream.h"
#include "datmanip.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cmath>

namespace RipUtil
{


MembufStream::MembufStream(const std::string& fname, Fmode mode, char decoder, int buffersize)
	: filename(fname), buf(0), bufsize(0), eof_flag(false), decoding_byte(decoder)
{
	// open stream to file
	switch(mode) 
	{
	case MembufStream::rb:
		stream.open(fname.c_str(), std::ios_base::binary);
		fmode = mode;
		break;
	}
	if (!stream.good()) throw(FileOpenException(fname));
	// get filesize
	stream.seekg(0, stream.end);
	if (stream.good())
	{
		fsize = static_cast<int> (stream.tellg());
		stream.seekg(0, stream.beg);
		// create buffer
		// buffersize of -1 = buffer entire file
		if (buffersize == -1 || buffersize > fsize) 
		{
			maxbufsize = fsize;
		}
		else 
		{
			maxbufsize = buffersize;
		}
		// fill buffer from file start
		fill_buffer(0);
		gpos = 0;
		buf_gpos = 0;
	}
}

MembufStream::~MembufStream() 
{
	delete[] buf;
}

char* MembufStream::fill_buffer(int pos) 
{
	// calculate size of new buffer
	int nextbufpos = pos + maxbufsize;
	if (nextbufpos <= fsize) 
		stream.seekg(nextbufpos);
	else 
		stream.seekg(0, stream.end);
	int newsize = (int)stream.tellg() - pos;
	// clear buffer and read new data
	delete[] buf;
	if (newsize > 0) 
	{
		buf = new char[newsize];
		bufsize = newsize;
		stream.seekg(pos);
		stream.read(buf, newsize);
		eof_clear();
		return buf;
	}
	else 
	{
		bufsize = 0;
		return 0;
	}
}

int MembufStream::seekg(int pos) 
{
	int num = pos - gpos;
	seek_off(num);
	return gpos;
}

int MembufStream::seek_off(int num) 
{
	if (num > 0) advanceg(num);
	if (num < 0) rewindg(-num);
	return gpos;
}

int MembufStream::seek_end() 
{
	if (eof()) 
		return gpos;
	if (gpos < fsize - 1) 
	{
		while (gpos < fsize - 1) 
		{
			advanceg();
		}
	}
	else if (gpos > fsize - 1) 
	{
		while (gpos > fsize - 1) 
		{
			rewindg();
		}
	}
	return gpos;
}

int MembufStream::seek_bytes(const char* b, int n)
{
	char* check = new char[n];
	check[0] = b[0];
	while (!eof()) 
	{
		// only check when sufficient bytes remain
		if (get() == b[0] && fsize - gpos >= n - 1) 
		{
			int startpos = gpos - 1;
			read(check + 1, n - 1);
			if (std::memcmp(b, check, n) == 0)
			{
				delete[] check;
				return startpos;
			}
			else
			{
				seekg(startpos + 1);
			}
		}
	}
	delete[] check;
	return gpos;
}

char MembufStream::get() 
{
	char c = buf[buf_gpos];
	advanceg();
	if (decoding_byte == 0)
		return c;
	else
		return decode_byte(c);
}

char MembufStream::reverse_get() 
{
	char c = buf[buf_gpos];
	rewindg();
	if (decoding_byte == 0)
		return c;
	else
		return decode_byte(c);
}

MembufStream& MembufStream::read(char* s, int n, DatManip::End e) 
{
	int remaining = n;
	char* f = s + n;
	while (remaining > 0) 
	{
		// copy to end of buffer
		int bytestocopy = std::min(bufsize - buf_gpos, remaining);
		char* start = f - remaining;
		memcpy(start, buf + buf_gpos, bytestocopy);
		advanceg(bytestocopy);
		remaining -= bytestocopy;
	}
	// decode if needed
	if (decoding_byte != 0) 
	{
		while (s != f)
		{
			decode_byte(*s++);
		}
	}
	// swap endianess if needed
	if (e == DatManip::le)
		swap_end(s, n);
	return *this;
}

int MembufStream::read_int(int n, DatManip::End e)
{
	char* bytes = new char[n];
	read(bytes, n);
	if (e == DatManip::le)
		swap_end(bytes, n);
	int result = to_int(bytes, n);
	delete[] bytes;
	return result;
}

int MembufStream::reset()
{
	stream.close();
	if (eof()) eof_clear();
	decoding_byte = 0;
	switch (fmode)
	{
	case rb:
		stream.open(filename.c_str(), std::ios_base::binary);
		break;
	default:
		stream.open(filename.c_str(), std::ios_base::binary);
	}
	if (!stream.good()) throw(FileOpenException(filename));
	// get filesize
	stream.seekg(0, stream.end);
	if (stream.good())
	{
		// fill buffer from file start
		fill_buffer(0);
		gpos = 0;
		buf_gpos = 0;
	}
	return gpos;
}

char MembufStream::decode_byte(char& byte) 
{
	return byte ^= decoding_byte;
}

int MembufStream::advanceg(int num) 
{
	for (int i = 0; i < num; i++) 
	{
		// can't advance past EOF
		if (gpos != fsize) 
		{
			gpos += 1;
			buf_gpos += 1;
			// if we advanced past end of buf, rebuffer
			if (buf_gpos >= bufsize && gpos != fsize) 
			{
				fill_buffer(gpos);
				buf_gpos = 0;
			}
			// if we hit EOF, set EOF flag
			else if (gpos >= fsize) 
			{
				eof_flag = true;
			}
		}
	}
	return buf_gpos;
}

int MembufStream::rewindg(int num) 
{
	for (int i = 0; i < num; i++) 
	{
		// can't rewind past beginning of file
		if (gpos >= 1) 
		{
			gpos -= 1;
			buf_gpos -= 1;
			// if we rewound past beginning of buf, rebuffer
			if (buf_gpos < 0) 
			{
				int newstartpos = gpos - maxbufsize + 1;
				// buffer maximum allowed bytes if possible
				if (newstartpos >= 0) 
				{
					fill_buffer(newstartpos);
					buf_gpos = maxbufsize - 1;
				}
				// otherwise, buffer from file start
				else 
				{
					fill_buffer(0);
					buf_gpos = maxbufsize + newstartpos;
				}
			}
		}
	}
	return buf_gpos;
}

bool MembufStream::eof_clear() 
{
	if (stream.eof()) 
	{
		stream.clear();
		return true;
	}
	return false;
}


};	// end namespace RipUtil
