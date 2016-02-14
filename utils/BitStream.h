/* A stream that attaches to an existing char array and provides
   bit-by-bit access to its contents */

namespace RipUtil
{

// abstract base class for a stream providing unidirectional 
// bit-by-bit access to a byte array
// bit order is defined by derived classes
class BitStream
{
public:

	int get_datalen() { return datalen; }
	int get_datapos() { return datapos; }
	int get_bitpos() { return bitpos; }
	bool atend() { return (datapos < datalen); }

	int get_bit()
	{
		int bit = next_bit(*(data + datapos), bitpos);
		advance(1);
		return bit;
	}

	int get_byte()
	{
		return get_nbit_int(8);
	}

	virtual int get_nbit_int(int nbits) =0;

protected:

	BitStream(const char* d, int len, int bitp)
		: data(d), datalen(len), datapos(0), bitpos(bitp) { };
	
	virtual void advance(int nbits) =0;

	int next_bit(char byte, int bitn)
	{
		return (byte & get_bitmask(bitn)) >> bitn;
	}
	int get_bitmask(int bpos)
	{
		return (1 << bpos);
	}

	const char* data;
	int datalen;
	int datapos;
	int bitpos;
};

// least significant to most significant bitstream
class RLBitStream : public BitStream
{
public:

	RLBitStream(const char* d, int len)
		: BitStream(d, len, 0) { };

	int get_nbit_int(int nbits)
	{
		int byte = 0;
		for (int i = 0; i < nbits; i++)
			byte |= (get_bit() << i);
		return byte;
	}

private:

	void advance(int nbits)
	{
		bitpos += nbits;
		if (bitpos > 7)
		{
			datapos += bitpos/8;
			bitpos %= 8;
		}
	}

};

// most significant to least significant bitstream
class LRBitStream : public BitStream
{
public:

	LRBitStream(const char* d, int len)
		: BitStream(d, len, 7) { };

	int get_nbit_int(int nbits)
	{
		int byte = 0;
		for (int i = 0; i < nbits; i++)
			byte |= (get_bit() << (nbits - i - 1));
		return byte;
	}

private:

	void advance(int nbits)
	{
		bitpos -= nbits;
		if (bitpos < 0)
		{
			datapos += (-bitpos - 1)/8 + 1;
			bitpos = 7 - ((-bitpos - 1) % 8);
		}
	}

};


};	// end namespace RipUtil