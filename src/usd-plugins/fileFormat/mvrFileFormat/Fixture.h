#pragma once
#include <string>
#include <vector>

namespace MVR {

	struct FixtureSpecification
	{
		std::string Name;
		uint32_t UUID;
		uint32_t FixtureId;
		uint32_t CustomId;
		uint32_t UnitNumber;
		bool CastShadows;
	};

	class Fixture
	{
	public:
		Fixture(const FixtureSpecification& spec);
		~Fixture() = default;

		inline const std::string& GetName() const { return m_Name; }
	private:
		std::string m_Name;
		uint32_t m_UUID;
	};

	struct LayerSpecification
	{

	};

	class Layer
	{
	public:
		Layer(const LayerSpecification& spec);
		~Layer() = default;

		void PushFixture(const Fixture& fixture);
		inline std::vector<Fixture>& GetFixtures() { return m_Fixtures; }

	private:
		std::string m_Name;
		uint32_t m_UUID;

		std::vector<Fixture> m_Fixtures;
	};

}