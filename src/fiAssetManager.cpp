#include "fiAssetManager.h"
#include "Addresses.h"

namespace rage
{
	void fiAssetManager::PushFolder(const char* path)
	{
		using Fn = void(*)(fiAssetManager*, const char*);
		reinterpret_cast<Fn>(Addresses.fiAssetManager_PushFolder)(this, path);
	}

	void fiAssetManager::PopFolder()
	{
		using Fn = void(*)(fiAssetManager*);
		reinterpret_cast<Fn>(Addresses.fiAssetManager_PopFolder)(this);
	}

	fiAssetManager* fiAssetManager::Instance()
	{
		return reinterpret_cast<fiAssetManager*>(Addresses.fiAssetManager_Instance);
	}
}
