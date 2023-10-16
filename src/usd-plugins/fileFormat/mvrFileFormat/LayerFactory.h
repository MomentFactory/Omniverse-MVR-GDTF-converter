#pragma once

namespace MVR {

	namespace tinyxml2 {
		class XMLElement;

	}

	class LayerSpecification;

	class LayerFactory
	{
	public:
		LayerFactory() = default;
		~LayerFactory() = default;

		LayerSpecification CreateSpecificationFromXML(tinyxml2::XMLElement* element);
	};
}