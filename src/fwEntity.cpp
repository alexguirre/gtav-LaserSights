#include "fwEntity.h"
#include "Addresses.h"

namespace rage 
{
	uint32_t fwEntity::GetBoneIndex(uint16_t boneId) const
	{
		return reinterpret_cast<uint32_t(*)(const fwEntity*, uint16_t)>(Addresses.fwEntity_GetBoneIndex)(this, boneId);
	}
}