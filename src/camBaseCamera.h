#pragma once
#include "Matrix.h"

class camBaseCamera
{
public:
	virtual ~camBaseCamera() = 0;

	const rage::Mat34V& GetTransform() const;

	static camBaseCamera* GetCurrentCamera();
};

