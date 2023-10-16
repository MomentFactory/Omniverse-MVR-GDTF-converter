#pragma once

namespace tinyxml2 {

	class XMLElement;

}

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