/* Functions for manipulating data format */

#include <string>
#include <sstream>


namespace DatManip
{


	enum End
	{
		end_none, le, be
	};
	enum Sign
	{
		sign_none, has_sign, has_nosign
	};


};	// end namespace DatManip

namespace RipUtil
{


// return a string representation of in
template<typename T> std::string to_string(T in)
{
	std::ostringstream oss;
	oss << in;
	return oss.str();
}

// return a T representation of string in
template<typename T> T from_string(std::string in)
{
	std::istringstream iss(in);
	T out;
	iss >> out;
	return out;
}

// return a hex string representation of T in
template<typename T> std::string to_string_hex(T in)
{
	std::ostringstream oss;
	oss << std::hex << in;
	return oss.str();
}

// return a T representation of string in, interpreting
// numerical values as hexadecimal
template<typename T> T from_string_hex(std::string in)
{
	std::istringstream iss(in);
	T out;
	iss >> std::hex >> out;
	return out;
}

// force a value into the range [lower, upper]
template<typename T> T clamp(T& val, T lower, T upper)
{
	if (val < lower)
		val = lower;
	else if (val > upper)
		val = upper;
	return val;
}

// reduce a filepath to filename + extension
std::string get_short_filename(std::string filepath);

// get the name of the lowest-level directory in a file path
std::string get_lowest_directory(std::string filepath);

// decompose an int into individual bytes and store in out
char* to_bytes(int val, char* out, int n, DatManip::End end = DatManip::be);

// compose a big-endian byte array into an int
int to_int(const char* s, int n, DatManip::End end = DatManip::be,
	DatManip::Sign sign = DatManip::has_nosign);

// swap endianess of a byte array (i.e. reverse it)
char* swap_end(char* s, int n);

// the above, with ints
int set_end(int s, int n, DatManip::End e);

// return a filename with extension stripped
std::string strip_extension(const std::string& fname);

// return a filename with given characters removed from end
std::string strip_terminators(const std::string& fname, const std::string& chars);

// return true if a memcmp of 2 arrays returns 0
bool quickcmp(const char* a1, const char* a2, int s);

// return true if a strcmp of 2 c-strings returns 0
bool quickstrcmp(const char* a1, const char* a2);

// return true if the given file exists in the current directory
bool file_exists(std::string filename);

// return 2's complement signed integral value of an int with the
// given bit precision
int to_signed(int val, int bits);


};	// end namespace RipUtil

#pragma once