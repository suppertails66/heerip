/* Stream that reads from and provides access to a file
   using a free-store buffer */

#include <string>
#include <fstream>

#include "datmanip.h"

namespace RipUtil
{


class FileOpenException : public std::exception 
{ 
public:
	FileOpenException(std::string s)
		: fname(s) { };
	virtual ~FileOpenException() throw() { };
	std::string fname;
};

class MembufStream 
{
public:
	// default buffer size in bytes 
	// -1 = size of input file
	const static int def_bufsize = -1;
	// file access modes
	enum Fmode 
	{ 
		rb
	};

	MembufStream(const std::string& fname, Fmode mode, char decoder = 0,
		int buffersize = def_bufsize);
	~MembufStream();
	
	std::string get_fname() { return filename; }
	int get_bufsize() { return bufsize; }
	int get_maxbufsize() { return maxbufsize; }
	int get_fsize() { return fsize; }
	int get_gpos() { return gpos; }
	int tellg() { return gpos; }
	int get_buf_gpos() { return buf_gpos; }
	char get_decoding_byte() { return decoding_byte; }

	void set_decoding_byte(char new_decoding_byte) 
	{ 
		decoding_byte = new_decoding_byte; 
	}

	// reached end of file?
	bool eof() { return eof_flag; }

	// clear state flags
	void clear() 
	{ 
		eof_flag = false;
		eof_clear();
	}

	// seek an absolute file position
	int seekg(int pos);
	// seek num bytes from the current file position
	int seek_off(int num);
	// seek to last byte of file
	int seek_end();
	// linear search for a byte sequence, returning
	// the position of the next occurence or EOF if none
	int seek_bytes(const char* b, int n);

	// return current char and increment get position
	char get();
	// return current char and decrement get position
	char reverse_get();
	// read n chars into s
	MembufStream& read(char* s, int n, DatManip::End e = DatManip::be);
	// read n chars and return the result as an int of the
	// specified endianess
	int read_int(int n, DatManip::End e = DatManip::be);
	// close and reopen file stream, resetting buffers/filepos
	// return new buf_gpos (should always be 0)
	int reset();

private:
	MembufStream(const MembufStream&);
	MembufStream& operator=(const MembufStream&);
	char* buf;				// array of buffered bytes
	std::string filename;	// name of currently open file
	int bufsize;			// size of buffer in bytes
	int maxbufsize;			// maximum size of buffer in bytes
	int fsize;				// size of input file
	Fmode fmode;			// file access mode (read/write)
	int gpos;				// get position (within entire file)
	int buf_gpos;			// buffer get position
	std::ifstream stream;	// ifstream for file access
	bool eof_flag;			// true if EOF reached
	char decoding_byte;		// optional XOR decoding byte

	// starting from pos, refill buffer and update buf pointer
	// return pointer to the new buffer
	char* fill_buffer(int pos);
	// advance get position num times, rebuffering as needed
	// return new buf_gpos
	int advanceg(int num = 1);
	// decrement get position num times, rebuffering as needed
	// return new buf_gpos
	int rewindg(int num = 1);
	// if stream has hit eof, clear flags
	bool eof_clear();
	// decode an XORed byte
	char decode_byte(char& byte);
};


};	// end namespace RipUtil

#pragma once
