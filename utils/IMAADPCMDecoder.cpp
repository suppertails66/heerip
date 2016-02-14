#include "IMAADPCMDecoder.h"
#include "datmanip.h"

namespace RipUtil
{


int IMAADPCMDecoder::decode_samp(char originalSample)
{
	int difference = 0;
	if (originalSample & 4)
		difference += stepsize;
	if (originalSample & 2)
		difference += stepsize >> 1;
	if (originalSample & 1)
		difference += stepsize >> 2;
	difference += stepsize >> 3;
	if (originalSample & 8)
		difference = -difference;
	predictedSample += difference;
	clamp(predictedSample, -32768, 32767);
	index += IMAADPCMConsts::indexTable[originalSample];
	clamp(index, 0, 88);
	stepsize = IMAADPCMConsts::stepsizeTable[index];
	return predictedSample;
}


};	// end namespace RipUtil