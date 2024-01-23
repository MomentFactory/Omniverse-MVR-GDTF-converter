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
			if (c == ',' || c == ';' || c == '{' || c == '}') 
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

		for (int i = 0; i < 4; ++i) 
		{
			for (int j = 0; j < 3; ++j) 
			{
				std::cout << std::to_string(output[i][j]) << " ";
			}

			std::cout << std::endl;
		}

		spec.Matrix = output;
		
		spec.GDTFSpec =	node->FirstChildElement("GDTFSpec")->GetText();
		spec.GDTFMode = node->FirstChildElement("GDTFMode")->GetText();

		// Custom commands
		auto customCommands = node->FirstChildElement("CustomCommands");
		if (customCommands)
		{
			for (auto it = customCommands->FirstChildElement("CustomCommand"); it; it = it->NextSiblingElement())
			{
				spec.CustomCommands.push_back(it->GetText());
			}
		}

		spec.Classing = std::string(node->FirstChildElement("Classing")->GetText());
		
		auto addresses = node->FirstChildElement("Addresses");
		if (addresses)
		{
			for (auto it = addresses->FirstChildElement("Address"); it; it = it->NextSiblingElement())
			{
				spec.Addresses.push_back(it->GetText());
			}
		}

		auto fixtureIdXml = node->FirstChildElement("FixtureID");
		if(fixtureIdXml && fixtureIdXml->GetText())
		{
			const std::string content = std::string(fixtureIdXml->GetText());
			if(!content.empty())
			{
				spec.FixtureID = static_cast<uint32_t>(std::stoul(content));
			}
		}
		else
		{
			fixtureIdXml = node->FirstChildElement("fixtureId");
			if(fixtureIdXml && fixtureIdXml->GetText())
			{
				const std::string content = fixtureIdXml->GetText();
				if(!content.empty())
				{
					spec.FixtureID = static_cast<uint32_t>(std::stoul(content));
				}
			}
		}

		auto unitNumberXml = node->FirstChildElement("UnitNumber");
		if(unitNumberXml && unitNumberXml->GetText())
		{
			spec.UnitNumber = std::stoul(unitNumberXml->GetText());
		}

		auto customIdXml = node->FirstChildElement("CustomId");
		if(customIdXml && customIdXml->GetText())
		{
			spec.CustomId = std::stoul(customIdXml->GetText());
		}

		auto castShadowXml = node->FirstChildElement("CastShadow");
		if(castShadowXml && castShadowXml->GetText())
		{
			spec.CastShadows = castShadowXml->GetText() == "true" ? true : false;
		}

		return spec;
	}
}