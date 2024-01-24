#pragma once

#include "tinyxml2.h"

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