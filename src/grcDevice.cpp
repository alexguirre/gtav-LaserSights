#include "grcDevice.h"
#include "Addresses.h"

namespace rage
{
	void grcWorldIdentity()
	{
		using Fn = decltype(&grcWorldIdentity);
		reinterpret_cast<Fn>(Addresses::grcWorldIdentity)();
	}

	grcVertexDeclaration* grcDevice::CreateVertexDeclaration(const grcVertexElement* elements, uint32_t elementCount)
	{
		using Fn = decltype(&CreateVertexDeclaration);
		return reinterpret_cast<Fn>(Addresses::grcDevice_CreateVertexDeclaration)(elements, elementCount);
	}

	int grcDevice::SetVertexDeclaration(grcVertexDeclaration* vertexDecl)
	{
		using Fn = decltype(&SetVertexDeclaration);
		return reinterpret_cast<Fn>(Addresses::grcDevice_SetVertexDeclaration)(vertexDecl);
	}

	void* grcDevice::BeginVertices(grcDrawMode mode, uint32_t vertexCount, uint32_t vertexSize)
	{
		using Fn = decltype(&BeginVertices);
		return reinterpret_cast<Fn>(Addresses::grcDevice_BeginVertices)(mode, vertexCount, vertexSize);
	}

	void grcDevice::EndVertices()
	{
		using Fn = decltype(&EndVertices);
		reinterpret_cast<Fn>(Addresses::grcDevice_EndVertices)();
	}
}