/* Definitions for enabled rip modules */

#include "modules/RipModule.h"
#include <vector>

class RipModule;

namespace Ripper
{


class RipModules
{
public:
	RipModules();
	~RipModules();
	
	int num_mods();
	RipModule* operator[](int n);
private:
	std::vector<RipModule*> rippers;
};


};	// end namespace Ripper



#pragma once