#include "aiTask.h"
#include "Addresses.h"

namespace rage
{
	uint8_t aiTask::GetCurrentState() const
	{
		const uint8_t* t = reinterpret_cast<const uint8_t*>(this);
		return *reinterpret_cast<const uint8_t*>(t + 0x36);
	}

	aiTask* aiTaskTree::FindTaskByTypeActive(uint32_t taskType)
	{
		using Fn = aiTask*(*)(aiTaskTree*, uint32_t);
		return reinterpret_cast<Fn>(Addresses::aiTaskTree_FindTaskByTypeActive)(this, taskType);
	}	
}
