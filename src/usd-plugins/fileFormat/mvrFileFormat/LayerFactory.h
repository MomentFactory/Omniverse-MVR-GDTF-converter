#pragma once

namespace tinyxml2 {
	class XMLElement;
}

namespace MVR {

	struct LayerSpecification;

	class LayerFactory
	{
	public:
		LayerFactory() = default;
		~LayerFactory() = default;

		LayerSpecification CreateSpecificationFromXML(tinyxml2::XMLElement* element);
	};

}