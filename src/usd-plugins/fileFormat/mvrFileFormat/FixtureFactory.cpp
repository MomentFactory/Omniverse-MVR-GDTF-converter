#include "FixtureFactory.h"
#include "Fixture.h"

#include <tinyxml2.h>

#define GetAttribAsString(name) std::string(node->FindAttribute(name)->Value());
#define TryGetAttribAsString(name) node->FindAttribute(name) ? std::string(node->FindAttribute(name)->Value()) : "";

#define TryGetAttribAsInt(name) node->FindAttribute(name) ? std::string(node->FindAttribute(name)->Value()) : "";

#define TryGetAttribAsBool(name) node->FindAttribute(name) ? node->FindAttribute(name)->BoolValue() : true;

namespace MVR {

	template<typename T>
	T GetAttribute(tinyxml2::XMLElement* element, const std::string& name)
	{
		if constexpr (std::is_same_v<std::string, T>)
		{
			return element->FindAttribute(name) ? std::string(element->FindAttribute(name)->Value()) : "";
		}
		else if constexpr (std::is_same_v<uint32_t, T>)
		{
			return element->FindAttribute(name) ? element->FindAttribute(name)->IntValue() : 0;
		}
		else if constexpr (std::is_same_v<bool, T>)
		{
			return element->FindAttribute(name) ? element->FindAttribute(name)->BoolValue() : false;
		}

		static_assert(true && "Attribute type not implemented.");
	}

	FixtureSpecification FixtureFactory::CreateFromXML(tinyxml2::XMLElement* node)
	{
		FixtureSpecification spec;
		spec.Name = GetAttribute<std::string>(node, "name");
		spec.UUID = GetAttribute<std::string>(node, "uuid");
		spec.Matrix = GetAttribute<std::string>(node, "Matrix");
		spec.GDTFSpec = GetAttribute<std::string>(node, "GDTFSpec");
		spec.GDTFMode = GetAttribute<std::string>(node, "GDTFMode");

		// Custom commands
		auto customCommands = node->FirstChildElement("CustomCommands");
		if (customCommands)
		{
			for (auto it = customCommands->FirstChildElement("CustomCommand"); it; it = it->NextSiblingElement())
			{
				spec.CustomCommands.push_back(it->GetText());
			}
		}

		spec.Classing = GetAttribute<std::string>(node, "Classing");;

		// Addresses
		auto addresses = node->FirstChildElement("Addresses");
		if (addresses)
		{
			for (auto it = addresses->FirstChildElement("Address"); it; it = it->NextSiblingElement())
			{
				spec.Addresses.push_back(it->GetText());
			}
		}

		spec.FixtureID = GetAttribute<uint32_t>(node, "fixtureId");
		spec.UnitNumber = GetAttribute<uint32_t>(node, "UnitNumber");
		spec.FixtureID = GetAttribute<uint32_t>(node, "FixtureID");
		spec.CustomId = GetAttribute<uint32_t>(node, "CustomId");
		// spec.CieColor = TryGetAttribAsString("CieColor"); TODO: Color
		spec.CastShadows = GetAttribute<bool>(node, "CastShadow");
	}
}