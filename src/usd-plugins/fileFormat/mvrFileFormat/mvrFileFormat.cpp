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

//#include "MVRParser.h"
//
//#include <FixtureFactory.h>
//#include "Fixture.h"
//#include <LayerFactory.h>


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
    PXR_NAMESPACE_USING_DIRECTIVE
	if (!TF_VERIFY(layer))
	{
		return false;
	}
	//MVR::MVRParser mvrParser;
	//auto result = mvrParser.ParseMVRFile(resolvedPath);
    //
    //while (mvrParser.HasError())
    //{
    //    TF_CODING_ERROR(mvrParser.PopError());
    //    return false;
    //}
    //
    //// Create USD Stage
    //// -----------------------------------
	SdfLayerRefPtr newLayer = SdfLayer::CreateAnonymous(".usd");
	UsdStageRefPtr stage = UsdStage::Open(newLayer);
    //
	const auto& xformPath = SdfPath("/mvr_payload");
	auto defaultPrim = UsdGeomXform::Define(stage, xformPath);
	stage->SetDefaultPrim(defaultPrim.GetPrim());
    //
	//// TODO: Create usd scene.
	//
    //// Copy contents into output layer.
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

void MvrFileFormat::ComposeFieldsForFileFormatArguments(const std::string& assetPath, const PcpDynamicFileFormatContext& context, FileFormatArguments* args, VtValue* contextDependencyData) const
{

}

bool MvrFileFormat::CanFieldChangeAffectFileFormatArguments(const TfToken& field, const VtValue& oldValue, const VtValue& newValue, const VtValue& contextDependencyData) const
{
    return true;
}

// these macros emit methods defined in the Pixar namespace
// but not properly scoped, so we have to use the namespace
// locally here
TF_DEFINE_PUBLIC_TOKENS(
	MvrFileFormatTokens,
	((Id, "mvrFileFormat"))
	((Version, "1.0"))
	((Target, "usd"))
	((Extension, "mvr"))
);

TF_REGISTRY_FUNCTION(TfType)
{
	SDF_DEFINE_FILE_FORMAT(MvrFileFormat, SdfFileFormat);
}

PXR_NAMESPACE_CLOSE_SCOPE