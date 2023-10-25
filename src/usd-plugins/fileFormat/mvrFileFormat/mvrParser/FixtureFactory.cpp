#include "FixtureFactory.h"
#include "Fixture.h"

#include <string>
#include <sstream>
#include <vector>
#include <iostream>


#define GetAttribAsString(name) std::string(node->FindAttribute(name)->Value());
#define TryGetAttribAsString(name) node->FindAttribute(name) ? std::string(node->FindAttribute(name)->Value()) : "";

#define TryGetAttribAsInt(name) node->FindAttribute(name) ? std::string(node->FindAttribute(name)->Value()) : "";

#define TryGetAttribAsBool(name) node->FindAttribute(name) ? node->FindAttribute(name)->BoolValue() : true;

namespace MVR {

	template<typename T>
	T GetAttribute(tinyxml2::XMLElement* element, const std::string& name)
	{
		static_assert(true && "Attribute type not implemented.");
	}

	template<>
	std::string GetAttribute<std::string>(tinyxml2::XMLElement* element, const std::string& name)
	{
		return element->FindAttribute(name.c_str()) ? std::string(element->FindAttribute(name.c_str())->Value()) : std::string();
	}

	template<>
	uint32_t GetAttribute<uint32_t>(tinyxml2::XMLElement* element, const std::string& name)
	{
		return element->FindAttribute(name.c_str()) ? element->FindAttribute(name.c_str())->IntValue() : 0;
	}

	template<>
	bool GetAttribute<bool>(tinyxml2::XMLElement* element, const std::string& name)
	{
		return element->FindAttribute(name.c_str()) ? element->FindAttribute(name.c_str())->BoolValue() : false;
	}

	FixtureSpecification FixtureFactory::CreateFromXML(tinyxml2::XMLElement* node)
	{
		FixtureSpecification spec;
		spec.Name = GetAttribute<std::string>(node, "name");
		spec.UUID = GetAttribute<std::string>(node, "uuid");
		
		auto inputString = std::string(node->FirstChildElement("Matrix")->GetText());
		inputString = inputString.substr(inputString.find("{") + 1, inputString.rfind("}") - inputString.find("{") - 1);

		// Replace "},{" with ";"
		size_t pos;
		while ((pos = inputString.find("}{")) != std::string::npos) {
			inputString.replace(pos, 2, " ");
		}

		// Replace "," with space
		for (char& c : inputString) 
		{
			if (c == ',' || c == ';') 
			{
				c = ' ';
			}
		}

		MVRMatrix output;
		std::istringstream iss(inputString);

		for (int i = 0; i < 4; ++i) 
		{
			for (int j = 0; j < 3; ++j) 
			{
				if (!(iss >> output[i][j])) 
				{
					// Handle any parsing error here if needed
				}
			}
		}

		spec.Matrix = output;
		
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
		spec.CastShadows = GetAttribute<bool>(node, "CastShadow");

		return spec;
	}
}