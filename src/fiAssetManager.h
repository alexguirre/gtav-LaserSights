#pragma once

namespace rage
{
	class fiAssetManager
	{
	public:
		void PushFolder(const char* path);
		void PopFolder();

		static fiAssetManager* Instance();
	};
}
