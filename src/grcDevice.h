#pragma once
#include <stdint.h>
#include <d3d11.h>

namespace rage
{
	using grcBlendStateHandle = uint32_t;
	grcBlendStateHandle grcCreateBlendState(const D3D11_BLEND_DESC& desc);
	void grcSetBlendState(grcBlendStateHandle blendStateHandle);

	using grcDepthStencilStateHandle = uint32_t;
	grcDepthStencilStateHandle grcCreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc);
	void grcSetDepthStencilState(grcDepthStencilStateHandle blendStateHandle);

	class grcVertexElement
	{
	public:
		enum class SemanticName : uint32_t
		{
			POSITION = 0,
			POSITIONT = 1,
			NORMAL = 2,
			BINORMAL = 3,
			TANGENT = 4,
			TEXCOORD = 5,
			BLENDWEIGHT = 6,
			BLENDINDICES = 7,
			COLOR = 8,
		};

		enum class Format : uint32_t
		{
			R16_FLOAT = 0,
			R16G16_FLOAT = 1,
			UNSUPPORTED = 2, // possibly meant to be R16G16B16_FLOAT, but the DXGI_FORMAT doesn't exist
			R16G16B16A16_FLOAT = 3,
			R32_FLOAT = 4,
			R32G32_FLOAT = 5,
			R32G32B32_FLOAT = 6,
			R32G32B32A32_FLOAT = 7,
			R8G8B8A8_UINT = 8,
			R8G8B8A8_UNORM = 9,
			R8G8B8A8_SNORM = 10,
			R16_UNORM = 11,
			R16G16_UNORM = 12,
			R8G8_UNORM = 13,
			R16G16_SINT = 14,
			R16G16B16A16_SINT = 15,
		};

		uint32_t m_InputSlot;
		SemanticName m_SemanticName;
		uint32_t m_SemanticIndex;
		uint32_t m_ByteSize;
		Format m_Format;
		uint32_t m_InputSlotClass;
		uint32_t m_InstanceDataStepRate;
	};

	class grcVertexDeclaration
	{
	public:
		uint32_t m_ElementCount;
		uint8_t padding_4[0x4];
		uint32_t m_VertexSize;
		uint8_t padding_C[0x4];
		D3D11_INPUT_ELEMENT_DESC m_Elements[];
	};

	enum class grcDrawMode
	{
		PointList = 0,
		LineList = 1,
		LineStrip = 2,
		TriangleList = 3,
		TriangleStrip = 4,
		// ...
	};

	void grcWorldIdentity();

	class grcDevice
	{
	public:
		static grcVertexDeclaration* CreateVertexDeclaration(const grcVertexElement* elements, uint32_t elementCount);
		static int SetVertexDeclaration(grcVertexDeclaration* vertexDecl);
		static void* BeginVertices(grcDrawMode mode, uint32_t vertexCount, uint32_t vertexSize);
		static void EndVertices();
	};
}
