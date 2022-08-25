#include "WorldProbe.h"
#include "Addresses.h"

namespace WorldProbe
{
	CShapeTestResults::CShapeTestResults(uint8_t maxHits)
	{
		m_State = 0;
		m_Hits = nullptr;
		m_MaxHits = maxHits;
		m_HitCount = 0;
		m_2 = 0;
	}

	void CShapeTestResults::AbortTest()
	{
		using Fn = void(*)(CShapeTestResults*);
		reinterpret_cast<Fn>(Addresses.WorldProbe_CShapeTestResults_AbortTest)(this);
	}

	CShapeTestDesc::CShapeTestDesc()
	{
		m_Type = eShapeTestType::Invalid;
		m_Results = nullptr;
		m_ResultHits = nullptr;
		m_ExcludeEntitiesInsts.Count = 0;
		m_Unk.Count = 0;
		m_TypeFlags = 0xFFFFFFFF;
		m_Flags1 = 0;
		m_MaxResultHits = 0;
		m_844 = 0;
		m_848 = 0;
		m_850 = 0;
		m_IncludeFlags = 0x3E00000;
		m_83C = 7;
		m_84C = 4;
	}

	CShapeTestDesc::~CShapeTestDesc()
	{
	}

	void CShapeTestDesc::SetResultsStructure(CShapeTestResults* results)
	{
		using Fn = void(*)(CShapeTestDesc*, CShapeTestResults*);
		reinterpret_cast<Fn>(Addresses.WorldProbe_CShapeTestDesc_SetResultsStructure)(this, results);
	}

	void CShapeTestDesc::SetExcludeEntities(const rage::fwEntity* const* entities, int entityCount, uint8_t flags)
	{
		using Fn = void(*)(CShapeTestDesc*, const rage::fwEntity* const*, int, uint8_t);
		reinterpret_cast<Fn>(Addresses.WorldProbe_CShapeTestDesc_SetExcludeEntities)(this, entities, entityCount, flags);
	}

	CShapeTestProbeDesc::CShapeTestProbeDesc()
	{
		m_Type = eShapeTestType::Probe;
	}

	bool CShapeTestManager::SubmitTest(CShapeTestDesc& desc, bool async)
	{
		using Fn = bool(*)(CShapeTestManager*, CShapeTestDesc&, bool);
		return reinterpret_cast<Fn>(Addresses.WorldProbe_CShapeTestManager_SubmitTest)(this, desc, async);
	}

	CShapeTestManager* GetShapeTestManager()
	{
		using Fn = decltype(&GetShapeTestManager);
		return reinterpret_cast<Fn>(Addresses.WorldProbe_GetShapeTestManager)();
	}
}
