#include "LayerFactory.h"
#include "FixtureFactory.h"

#include "Fixture.h"
#include <iostream>

namespace MVR {

	LayerSpecification LayerFactory::CreateSpecificationFromXML(tinyxml2::XMLElement* element)
	{
		std::cout << "CreateSpecificationFromXML" << std::endl;
		std::string name = std::string(element->FindAttribute("name")->Value());
		std::string uuid = std::string(element->FindAttribute("uuid")->Value());
		
		// Parse fixtures in the layer
		std::vector<FixtureSpecification> fixtureSpecs;

		FixtureFactory fixtureFactory;
		auto childList = element->FirstChildElement("ChildList");
		for (auto it = childList->FirstChildElement("Fixture"); it; it = it->NextSiblingElement())
		{
			FixtureSpecification fixture = fixtureFactory.CreateFromXML(it);
			fixtureSpecs.push_back(std::move(fixture));
		}

		LayerSpecification spec
		{
			std::move(name),
			std::move(uuid),
			std::move(fixtureSpecs)
		};

		return spec;
	}
}