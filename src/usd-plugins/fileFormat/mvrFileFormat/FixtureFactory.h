#pragma once

namespace MVR {

	namespace tinyxml2 {
		class XMLNode;

	}

	struct FixtureSpecification;

	class FixtureFactory
	{
	public:
		FixtureFactory() = default;
		~FixtureFactory() = default;

		FixtureSpecification CreateFromXML(tinyxml2::XMLNode* node);
	};
}