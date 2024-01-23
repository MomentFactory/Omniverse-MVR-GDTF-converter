#pragma once

#include "tinyxml2.h"

namespace MVR {

	struct FixtureSpecification;

	class FixtureFactory
	{
	public:
		FixtureFactory() = default;
		~FixtureFactory() = default;

		FixtureSpecification CreateFromXML(tinyxml2::XMLElement* node);
	};

}