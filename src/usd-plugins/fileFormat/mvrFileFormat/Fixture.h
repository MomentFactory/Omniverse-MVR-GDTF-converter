#pragma once
#include <string>
#include <vector>

namespace MVR {

	struct FixtureSpecification
	{
		std::string Name;
		std::string UUID;
		std::string Matrix;
		std::string GDTFSpec;
		std::string GDTFMode;
		std::vector<std::string> CustomCommands;
		std::string Classing;
		std::vector<std::string> Addresses;
		uint32_t FixtureID;
		uint32_t UnitNumber;
		uint32_t FixtureTypeID;
		uint32_t CustomId;
		std::vector<float> CieColor;
		bool CastShadows;
	};

	class Fixture
	{
	public:
		Fixture(const FixtureSpecification& fixture);
		~Fixture() = default;

		inline const std::string& GetName() const { return m_Name; }
	private:
		std::string m_Name;
		std::string m_UUID;
		std::string m_Matrix;
		std::string m_GDTFSpec;
		std::string m_GDTFMode;
		std::vector<std::string> m_CustomCommands;
		std::string m_Classing;
		std::vector<std::string> m_Addresses;
		uint32_t m_FixtureID;
		uint32_t m_UnitNumber;
		uint32_t m_FixtureTypeID;
		uint32_t m_CustomId;
		std::vector<float> m_CieColor;
		bool m_CastShadows;
	};

	struct LayerSpecification
	{
		std::string name;
		std::string uuid;

		std::vector<FixtureSpecification> fixtures;
	};

	class Layer
	{
	public:
		Layer(const LayerSpecification& spec);
		~Layer() = default;

		inline std::vector<Fixture>& GetFixtures() { return m_Fixtures; }

	private:
		std::string m_Name;
		std::string m_UUID;

		std::vector<Fixture> m_Fixtures;
	};

}