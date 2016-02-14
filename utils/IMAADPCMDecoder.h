/* Class for decoding 4-bit IMA-ADPCM samples to 16-bit signed PCM */

namespace RipUtil
{


namespace IMAADPCMConsts
{
	// IMA-ADPCM decoding tables
	const int indexTable[16] = { -1, -1, -1, -1, 2, 4, 6, 8,
								-1, -1, -1, -1, 2, 4, 6, 8 };
	const int stepsizeTable[89] = { 7, 8, 9, 10, 11, 12, 13,
									14, 16, 17, 19, 21, 23, 25, 28,
									31, 34, 37, 41, 45, 50, 55, 60,
									66, 73, 80, 88, 97, 107, 118,
									130, 143, 157, 173, 190, 209, 230,
									253, 279, 307, 337, 371, 408, 449,
									494, 544, 598, 658, 724, 796, 876,
									963, 1060, 1166, 1282, 1411, 1552,
									1707, 1878, 2066, 2272, 2499, 2749,
									3024, 3327, 3660, 4026, 4428, 4871,
									5358, 5894, 6484, 7132, 7845, 8630,
									9493, 10442, 11487, 12635, 13899,
									15289, 16818, 18500, 20350, 22385,
									24623, 27086, 29794, 32767 };
};

class IMAADPCMDecoder
{
public:
	IMAADPCMDecoder()
		: predictedSample(0), index(0), stepsize(7) { };
	IMAADPCMDecoder(int pred, int ind, int step)
		: predictedSample(pred), index(ind), stepsize(step) { };

	void reset()
	{
		predictedSample, index = 0;
		stepsize = 7;
	}
	int get_predictedSample() { return predictedSample; }
	int get_index() { return index; }
	int get_stepsize() { return stepsize; }
	void set_predictedSample(int pred)
	{
		predictedSample = pred;
	}
	void set_index(int ind)
	{
		index = ind;
	}
	void set_stepsize(int step)
	{
		stepsize = step;
	}

	// decode 1 sample, update parameters, and return as 16-bit signed value
	int decode_samp(char originalSample);
private:
	int predictedSample;
	int index;
	int stepsize;
};


};	// end namespace RipUtil