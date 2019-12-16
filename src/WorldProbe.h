#pragma once
#include <stdint.h>
#include "Vector.h"

namespace rage
{
	class fwEntity;
}

namespace WorldProbe
{
	class CShapeTestHit
	{
	public:
		rage::Vec3V m_Position;
		rage::Vec3V m_SurfaceNormal;
		rage::Vec3V m_20;
		uint64_t m_Material;
		uint16_t m_InstIndex;
		uint8_t padding_3A[2];
		uint32_t m_3C;
	};
	static_assert(sizeof(CShapeTestHit) == 0x40);

	class CShapeTestResults
	{
	public:
		uint8_t m_MaxHits;
		uint8_t m_HitCount;
		uint8_t m_2;
		uint8_t m_3;
		uint32_t m_State;
		CShapeTestHit* m_Hits;

		CShapeTestResults(uint8_t maxHits);

		void AbortTest();
	};
	static_assert(sizeof(CShapeTestResults) == 0x10);

	enum class eShapeTestType : uint32_t
	{
		Invalid = -1,
		Probe = 0,
	};

	class CShapeTestDesc
	{
	public:
		eShapeTestType m_Type;
		uint8_t padding_C[0x4];
		CShapeTestResults* m_Results;
		CShapeTestHit* m_ResultHits;
		struct
		{
			void* Items[128]; // phInst*
			uint32_t Count;
			uint32_t padding;
		} m_ExcludeEntitiesInsts;
		struct
		{
			void* Items[128];
			uint32_t Count;
			uint32_t padding;
		} m_Unk;
		uint32_t m_Flags1;
		uint32_t m_Flags2;
		uint32_t m_838;
		uint8_t m_83C;
		uint8_t padding_83D[0x3];
		uint32_t m_MaxResultHits;
		uint32_t m_844;
		uint32_t m_848;
		uint32_t m_84C;
		uint8_t m_850;
		uint8_t padding_851[0x3];
		uint32_t m_854;
		uint32_t m_858;
		uint32_t m_85C;

		CShapeTestDesc();

		virtual ~CShapeTestDesc();

		void SetResultsStructure(CShapeTestResults* results);
		void SetExcludeEntities(const rage::fwEntity* const* entities, int entityCount, uint8_t flags); // actually accepts CEntity
	};
	static_assert(sizeof(CShapeTestDesc) == 0x860);

	class CShapeTestProbeDesc : public CShapeTestDesc
	{
	public:
		rage::Vec3V m_Start;
		rage::Vec3V m_End;

		CShapeTestProbeDesc();
	};
	static_assert(sizeof(CShapeTestProbeDesc) == 0x880);

	class CShapeTestManager
	{
	public:
		bool SubmitTest(CShapeTestDesc& desc, bool a3);
	};

	CShapeTestManager* GetShapeTestManager();
}
