#include "RipperFormats.h"
#include "utils/PCMData.h"
#include "utils/datmanip.h"
#include "utils/MembufStream.h"
#include "utils/IMAADPCMDecoder.h"
#include "utils/BitmapData.h"
#include "modules/common.h"

#include <string>
#include <cstring>
#include <vector>

using namespace RipUtil;

namespace RipperFormats
{


void format_PCMData(RipUtil::PCMData& dat, const RipperSettings& ripset)
{
	if (ripset.channels != RipConsts::not_set)
		dat.set_channels(ripset.channels);
	if (ripset.samprate != RipConsts::not_set)
		dat.set_samprate(ripset.samprate);
	if (ripset.sampwidth != RipConsts::not_set)
		dat.set_sampwidth(ripset.sampwidth);
	if (ripset.audsign != DatManip::sign_none)
		dat.set_signed(ripset.audsign);
	if (ripset.audend != DatManip::end_none)
		dat.set_end(ripset.audend);
}


};	// end namespace Ripper

