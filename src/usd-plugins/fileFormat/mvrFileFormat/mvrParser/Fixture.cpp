

#include "Fixture.h"


namespace MVR {

	Fixture::Fixture(const FixtureSpecification& spec) :
		m_Name(spec.Name),
		m_UUID(spec.UUID),
		m_Matrix(spec.Matrix),
		m_GDTFSpec(spec.GDTFSpec),
		m_GDTFMode(spec.GDTFMode),
		m_CustomCommands(spec.CustomCommands),
		m_Classing(spec.Classing),
		m_Addresses(spec.Addresses),
		m_FixtureID(spec.FixtureID),
		m_UnitNumber(spec.UnitNumber),
		m_FixtureTypeID(spec.FixtureTypeID),
		m_CustomId(spec.CustomId),
		m_CastShadows(spec.CastShadows)
	{
	}

	Layer::Layer(const LayerSpecification& spec) :
		m_Name(spec.name),
		m_UUID(spec.uuid)
	{
		m_Fixtures.reserve(spec.fixtures.size());

		for (const auto& layerSpec : spec.fixtures)
		{
			m_Fixtures.push_back(Fixture(layerSpec));
		}
	}
}
