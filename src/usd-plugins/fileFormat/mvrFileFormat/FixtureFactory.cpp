#include "FixtureFactory.h"
#include "Fixture.h"

#include <tinyxml2.h>

namespace MVR {

	FixtureSpecification FixtureFactory::CreateFromXML(tinyxml2::XMLNode* node)
	{
		// TODO: Parse XML nodes

		FixtureSpecification spec;
		spec.Name = "Fixture";
		spec.CastShadows = false;
	}
}