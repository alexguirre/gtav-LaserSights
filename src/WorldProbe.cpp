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
		reinterpret_cast<Fn>(Addresses::WorldProbe_CShapeTestResults_AbortTest)(this);
	}

	CShapeTestDesc::CShapeTestDesc()
	{
		m_Type = eShapeTestType::Invalid;
		m_Results = nullptr;
		m_ResultHits = nullptr;
		m_ExcludeEntitiesInsts.Count = 0;
		m_Unk.Count = 0;
		m_Flags2 = 0xFFFFFFFF;
		m_Flags1 = 0;
		m_MaxResultHits = 0;
		m_844 = 0;
		m_848 = 0;
		m_850 = 0;
		m_838 = 0x3E00000;
		m_83C = 7;
		m_84C = 4;
	}

	CShapeTestDesc::~CShapeTestDesc()
	{
	}

	void CShapeTestDesc::SetResultsStructure(CShapeTestResults* results)
	{
		using Fn = void(*)(CShapeTestDesc*, CShapeTestResults*);
		reinterpret_cast<Fn>(Addresses::WorldProbe_CShapeTestDesc_SetResultsStructure)(this, results);
	}

	CShapeTestProbeDesc::CShapeTestProbeDesc()
	{
		m_Type = eShapeTestType::Probe;
	}

	bool CShapeTestManager::SubmitTest(CShapeTestDesc& desc, bool unk)
	{
		using Fn = bool(*)(CShapeTestManager*, CShapeTestDesc&, bool);
		return reinterpret_cast<Fn>(Addresses::WorldProbe_CShapeTestManager_SubmitTest)(this, desc, unk);
	}

	CShapeTestManager* GetShapeTestManager()
	{
		using Fn = decltype(&GetShapeTestManager);
		return reinterpret_cast<Fn>(Addresses::WorldProbe_GetShapeTestManager)();
	}
}
