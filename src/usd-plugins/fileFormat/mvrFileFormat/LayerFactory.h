#pragma once

namespace tinyxml2 {

	class XMLElement;

}

namespace MVR {

	class LayerSpecification;

	class LayerFactory
	{
	public:
		LayerFactory() = default;
		~LayerFactory() = default;

		LayerSpecification CreateSpecificationFromXML(tinyxml2::XMLElement* element);
	};

}