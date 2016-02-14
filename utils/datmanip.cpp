#include "datmanip.h"
#include <string>
#include <cstring>
#include <cmath>
#include <fstream>

namespace RipUtil
{


std::string get_short_filename(std::string filepath)
{
	if (filepath[filepath.length() - 1] == '/'
		|| filepath[filepath.length() - 1] == '\\')
	{
		filepath = filepath.substr(0, filepath.length() - 2);
	}
	std::string::size_type strfirst = filepath.find_last_of("/\\");
	if (strfirst == filepath.npos)
		strfirst = -1;
	filepath = filepath.substr(strfirst + 1, filepath.npos);
	return filepath;
}

std::string get_lowest_directory(std::string filepath)
{
	std::string::size_type lastslash = filepath.find_last_of("/\\");
	if (lastslash == filepath.npos)
		return "";
	else
		return filepath.substr(0, lastslash + 1);
}

char* to_bytes(int val, char* out, int n, DatManip::End end)
{
	for (int i = 0; i < n; i++) 
	{
		int mask = 0xFF;
		// shift mask by target byte
		mask <<= 8 * i;
		int outbyte = val & mask;
		outbyte >>= 8 * i;
		if (end == DatManip::be)
			out[n - i - 1] = outbyte;
		else
			out[i] = outbyte;
	}
	return out;
}

int to_int(const char* s, int n, DatManip::End end, DatManip::Sign sign)
{
	int out = 0;
	for (int i = 0; i < n; i++)
	{
		int mask;
		if (end == DatManip::be)
			mask = static_cast<unsigned char> (s[n - i - 1]);
		else
			mask = static_cast<unsigned char> (s[i]);
		mask <<= 8 * i;
		out |= mask;
	}
	if (sign == DatManip::has_sign)
	{
		int range = static_cast<int>(std::pow((double)2, n * 8));
		if (out > range/2 - 1)
			out -= range;
	}
	return out;
}

char* swap_end(char* s, int n) 
{
	if (n <= 1)
		return s;
	char* f = s + n - 1;
	while (s < f)
	{
		char temp = *s;
		*s = *f;
		*f = temp;
		++s;
		--f;
	}  
	return s;
}

int set_end(int s, int n, DatManip::End e)
{
	char* temp = new char[n];
	to_bytes(s, temp, n);
	s = to_int(temp, n, e);
	delete[] temp;
	return s;
}

std::string strip_extension(const std::string& fname)
{
	return fname.substr(0, fname.find_last_of('.'));
}

std::string strip_terminators(const std::string& fname, const std::string& chars)
{
	std::string::size_type lastper = fname.rfind(".");
	std::string name = fname.substr(0, lastper + 1);
	std::string ext = fname.substr(lastper + 1, fname.npos);

	int occ = ext.find_first_of(chars);
	while (occ != ext.npos)
	{
		ext.erase(occ, 1);
		occ = ext.find_first_of(chars);
	}

	return name + ext;
}

bool quickcmp(const char* a1, const char* a2, int s)
{
	if (std::memcmp(a1, a2, s) == 0)
		return true;
	return false;
}

bool quickstrcmp(const char* a1, const char* a2)
{
	if (std::strcmp(a1, a2) == 0)
		return true;
	return false;
}

bool file_exists(std::string filename)
{
	try
	{
		std::ifstream ifs(filename.c_str());
		if (ifs.fail())
			return false;
	}
	catch (std::ios_base::failure)
	{
		return false;
	}

	return true;
}

int to_signed(int val, int bits)
{
	int neg = static_cast<int>(std::pow((double)2, bits)/2);

	if (val >= neg)
		return val - neg * 2;
	else
		return val;
}


};	// end namespace RipUtil
