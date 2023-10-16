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
		std::string Addresses;
		std::string FixtureID;
		std::string UnitNumber;
		std::string FixtureTypeID;
		std::string CustomId;
		std::string CieColor;
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
		std::string m_CustomCommands;
		std::string m_Classing;
		std::string m_Addresses;
		std::string m_FixtureID;
		std::string m_UnitNumber;
		std::string m_FixtureTypeID;
		std::string m_CustomId;
		std::string m_CieColor;
		bool m_CastShadows;
	};

	struct LayerSpecification
	{
		std::string name;
		std::string uuid;

		std::vector<FixtureSpecification> layers;
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