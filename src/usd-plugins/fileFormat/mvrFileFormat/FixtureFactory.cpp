#include "FixtureFactory.h"
#include "Fixture.h"

#include <tinyxml2.h>

#define GetAttribAsString(name) std::string(node->FindAttribute(name)->Value());
#define TryGetAttribAsString(name) node->FindAttribute(name) ? std::string(node->FindAttribute(name)->Value()) : "";

#define TryGetAttribAsBool(name) node->FindAttribute(name) ? node->FindAttribute(name)->BoolValue() : true;

namespace MVR {

	FixtureSpecification FixtureFactory::CreateFromXML(tinyxml2::XMLElement* node)
	{
		FixtureSpecification spec;
		spec.Name = GetAttribAsString("name");
		spec.UUID = GetAttribAsString("uuid");
		spec.Matrix = TryGetAttribAsString("Matrix");
		spec.GDTFSpec = TryGetAttribAsString("GDTFSpec");
		spec.GDTFMode = TryGetAttribAsString("GDTFMode");
		// spec.CustomCommands = TryGetAttribAsString("CustomCommands"); TODO
		spec.Classing = TryGetAttribAsString("Classing");
		// spec.Addresses = TryGetAttribAsString("Classing"); TODO
		spec.FixtureID = TryGetAttribAsString("fixtureId");
		spec.UnitNumber = TryGetAttribAsString("UnitNumber");
		spec.FixtureTypeID = TryGetAttribAsString("FixtureTypeID");
		spec.CustomId = TryGetAttribAsString("CustomId");
		spec.CieColor = TryGetAttribAsString("CieColor");
		spec.CastShadows = TryGetAttribAsBool("CastShadow");
		// TODO: Antoine

		spec.Name = "Fixture";
		spec.CastShadows = false;
	}
}