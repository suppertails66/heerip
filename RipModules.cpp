#include "RipModules.h"
#include "modules/humongous.h"

namespace Ripper
{


RipModules::RipModules()
{
	rippers.push_back(new Humongous::HERip());
}

RipModules::~RipModules()
{
	for (std::vector<RipModule*>::size_type i = 0; i < rippers.size(); i++)
		delete rippers[i];
}

RipModule* RipModules::operator[](int n)
{
	return rippers[n];
}

int RipModules::num_mods()
{
	return rippers.size();
}


};	// end namespace Ripper