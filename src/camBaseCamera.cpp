#include "camBaseCamera.h"
#include "Addresses.h"
#include <cstdint>

const rage::Mat34V& camBaseCamera::GetTransform() const
{
	const uint8_t* t = reinterpret_cast<const uint8_t*>(this);
	return *reinterpret_cast<const rage::Mat34V*>(t + 0x20 + 0x10);
}

camBaseCamera* camBaseCamera::GetCurrentCamera()
{
	return *reinterpret_cast<camBaseCamera**>(Addresses.CurrentCamera);
}
