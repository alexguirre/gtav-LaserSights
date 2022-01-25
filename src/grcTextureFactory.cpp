#include "grcTextureFactory.h"
#include "Addresses.h"

namespace rage
{
	grcTextureFactory* grcTextureFactory::Instance()
	{
		return *reinterpret_cast<grcTextureFactory**>(Addresses.grcTextureFactory_Instance);
	}
}
