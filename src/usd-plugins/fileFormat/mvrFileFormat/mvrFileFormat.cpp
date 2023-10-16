// Copyright 2023 NVIDIA CORPORATION
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mvrFileFormat.h"
#include "tinyxml2.h"

#include <pxr/pxr.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/usdaFileFormat.h>

#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdGeom/xform.h>

#include <pxr/usd/usdLux/rectLight.h>

#include <pxr/base/gf/matrix3f.h>
#include <pxr/base/gf/vec3f.h>

#include <fstream>
#include <cmath>

// Parsing Utilities

#include "MVRParser.h"

#include <FixtureFactory.h>
#include "Fixture.h"

// ZIP



PXR_NAMESPACE_OPEN_SCOPE

MvrFileFormat::MvrFileFormat() : SdfFileFormat(
										MvrFileFormatTokens->Id,
										MvrFileFormatTokens->Version,
										MvrFileFormatTokens->Target,
										MvrFileFormatTokens->Extension)
{
}

MvrFileFormat::~MvrFileFormat()
{
}

// Exemple of arguments passed to payload
static const double defaultSideLengthValue = 1.0;

static double _ExtractSideLengthFromContext(const PcpDynamicFileFormatContext& context)
{
    // Default sideLength.
    double sideLength = defaultSideLengthValue;

    VtValue value;
    if (!context.ComposeValue(MvrFileFormatTokens->SideLength,
                              &value) ||
        value.IsEmpty()) {
        return sideLength;
    }

    if (!value.IsHolding<double>()) {
       
        return sideLength;
    }

    return value.UncheckedGet<double>();
}

static double
_ExtractSideLengthFromArgs(const SdfFileFormat::FileFormatArguments& args)
{
    // Default sideLength.
    double sideLength = defaultSideLengthValue;

    // Find "sideLength" file format argument.
    auto it = args.find(MvrFileFormatTokens->SideLength);
    if (it == args.end()) {
        return sideLength;
    }

    // Try to convert the string value to the actual output value type.
    double extractVal;
    bool success = true;
    extractVal = TfUnstringify<double>(it->second, &success);
    if (!success) {
        
        return sideLength;
    }

    sideLength = extractVal;
    return sideLength;
}

bool MvrFileFormat::CanRead(const std::string& filePath) const
{
	return true;
}

static std::string CleanNameForUSD(const std::string& name)
{
	std::string cleanedName = name;
	if(cleanedName.size() == 0)
	{
		return "Default";
	}

	if(cleanedName.size() == 1 && !TfIsValidIdentifier(cleanedName))
	{
		// If we have an index as a name, we only need to add _ beforehand.
		return CleanNameForUSD("_" + cleanedName);
	}

	return TfMakeValidIdentifier(cleanedName);
}




bool MvrFileFormat::Read(SdfLayer* layer, const std::string& resolvedPath, bool metadataOnly) const
{
	MVR::MVRParser mvrParser;

	auto result = mvrParser.ParseMVRFile(resolvedPath);


	
	
	// Open the MVR Zip archive and parse the relevent files
	// 1) GDTF - Open the GDTF Zip archive and handle the models
	// 2) XML  - Create relevant fixture and layers from the XML file contained
	// -------------------------------------------------------------------------------
    

    // GeneralSceneDescription.xml
    // ---------------------------------------------
	tinyxml2::XMLDocument doc;
	const tinyxml2::XMLError xmlReadSuccess = doc.Parse(resolvedPath.c_str());
	if (xmlReadSuccess != 0)
	{
		TF_CODING_ERROR("Failed to load xml file: " + resolvedPath);
		return false;
	}

	tinyxml2::XMLElement* rootNode = doc.RootElement();
	if (rootNode == nullptr)
	{
		TF_CODING_ERROR("XML Root node is null: " + resolvedPath);
		return false;
	}


    // TODO: Valide Version of MVR file > 1.5 in XML
    MVR::FixtureFactory fixtureFactory;
    MVR::FixtureSpecification fixtureSpec = fixtureFactory.CreateFromXML(rootNode);


    auto fixture = MVR::Fixture(fixtureSpec);

    MVR::Layer mvrLayer;
    mvrLayer.PushFixture(fixture);

    // TODO: Parse Scene in XML
    // TODO: Parse Layers
    // TODO: Parse fixtures
    


    // Create USD Stage
    // -----------------------------------
	SdfLayerRefPtr newLayer = SdfLayer::CreateAnonymous(".usd");
	UsdStageRefPtr stage = UsdStage::Open(newLayer);

	const auto& xformPath = SdfPath("/mvr_payload");
	auto defaultPrim = UsdGeomXform::Define(stage, xformPath);
	stage->SetDefaultPrim(defaultPrim.GetPrim());

	// TODO: Create usd scene.
	
    // Copy contents into output layer.
    layer->TransferContent(newLayer);

	return true;
}

bool MvrFileFormat::WriteToString(const SdfLayer& layer, std::string* str, const std::string& comment) const
{
	// this POC doesn't support writing
	return false;
}

bool MvrFileFormat::WriteToStream(const SdfSpecHandle& spec, std::ostream& out, size_t indent) const
{
	// this POC doesn't support writing
	return false;
}

/*
void MvrFileFormat::ComposeFieldsForFileFormatArguments(const std::string& assetPath, const PcpDynamicFileFormatContext& context, FileFormatArguments* args, VtValue* contextDependencyData) const
{
	 // Default sideLength.
    double sideLength = 1.0;

    VtValue value;
    if (!context.ComposeValue(MvrFileFormatTokens->SideLength,
                              &value) ||
        value.IsEmpty()) {

    }

    if (!value.IsHolding<double>()) {
        // error;
    }

    double length;

    (*args)[MvrFileFormatTokens->SideLength] = TfStringify(sideLength);
}

bool MvrFileFormat::CanFieldChangeAffectFileFormatArguments(const TfToken& field, const VtValue& oldValue, const VtValue& newValue, const VtValue& contextDependencyData) const
{
	// Check if the "sideLength" argument changed.
    double oldLength = oldValue.IsHolding<double>()
                           ? oldValue.UncheckedGet<double>()
                           : 1.0;
    double newLength = newValue.IsHolding<double>()
                           ? newValue.UncheckedGet<double>()
                           : 1.0;

    return oldLength != newLength;

}
*/
// these macros emit methods defined in the Pixar namespace
// but not properly scoped, so we have to use the namespace
// locally here
TF_DEFINE_PUBLIC_TOKENS(
	MvrFileFormatTokens,
	((Id, "mvrFileFormat"))
	((Version, "1.0"))
	((Target, "usd"))
	((Extension, "xml"))
	((SideLength, "Usd_Triangle_SideLength"))
);

TF_REGISTRY_FUNCTION(TfType)
{
	SDF_DEFINE_FILE_FORMAT(MvrFileFormat, SdfFileFormat);
}

PXR_NAMESPACE_CLOSE_SCOPE