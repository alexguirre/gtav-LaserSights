#pragma once
#include <cstdint>

namespace rage
{
	class parStructure;
	class parAttributeList;
	class parMemberCommonData;

	class parStructureStaticData
	{
	public:
		uint32_t m_NameHash;
		uint8_t padding_4[4];
		const char* m_Name;
		parStructure* m_Structure;
		parMemberCommonData** m_MemberData;
		uint32_t* m_MemberOffsets;
		const char** m_MemberNames;
		uint8_t padding_30[8];
	};
	static_assert(sizeof(parStructureStaticData) == 0x38);

	enum class parMemberType : uint8_t
	{
		BOOL = 0,
		UINT = 6,
		FLOAT = 7,
		STRUCT = 12,
		VEC3V = 20,
		VEC4V = 21,
	};

	class parMemberCommonData
	{
	public:
		uint32_t m_Name;
		uint8_t padding_4[4];
		uint32_t m_Offset;
		uint8_t padding_C[4];
		parMemberType m_Type;
		uint8_t m_SubType;
		uint16_t unk_12;
		uint16_t unk_14;
		uint16_t unk_16;
		const parAttributeList* m_Attributes;
	};
	static_assert(sizeof(parMemberCommonData) == 0x20);

	class parMemberSimpleData : public parMemberCommonData
	{
	public:
		union
		{
			float asFloat;
			int32_t asInt;
			uint32_t asUInt;
		} m_InitValue;
		uint8_t padding_24[4];
	};
	static_assert(sizeof(parMemberSimpleData) == 0x28);

	class parMemberStructData : public parMemberCommonData
	{
	public:
		parStructure* m_Structure;
		uint64_t unk_28;
		uint64_t unk_30;
		uint64_t unk_38;
	};
	static_assert(sizeof(parMemberStructData) == 0x40);

	class parMemberVectorData : public parMemberCommonData
	{
	public:
		float m_InitValues[4];
	};
	static_assert(sizeof(parMemberVectorData) == 0x30);
}
