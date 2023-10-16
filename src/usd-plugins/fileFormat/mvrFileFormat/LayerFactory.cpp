#include "LayerFactory.h"
#include "FixtureFactory.h"

#include "Fixture.h"

#include "tinyxml2.h"

namespace MVR {

	LayerSpecification LayerFactory::CreateSpecificationFromXML(tinyxml2::XMLElement* element)
	{
		std::string name = std::string(element->FindAttribute("name")->Value());
		std::string uuid = std::string(element->FindAttribute("uuid")->Value());

		LayerSpecification spec
		{
			std::move(name),
			std::move(uuid)
		};

		return std::move(spec);
	}
}