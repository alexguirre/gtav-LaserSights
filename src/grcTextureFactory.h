#pragma once

namespace rage
{
	class grcTexture;

	class grcTextureFactory
	{
	public:
		virtual ~grcTextureFactory() = 0;
		virtual void f_1() = 0;
		virtual void f_2() = 0;
		virtual void f_3() = 0;
		virtual grcTexture* Create(const char* name, void* createParams = nullptr) = 0;

		static grcTextureFactory* Instance();
	};
}
