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
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/usd/usd/payloads.h>

#include "mvrParser/MVRParser.h"
#include "../gdtfFileFormat/gdtfUsdConverter.h"

#include <iostream>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

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
	// these macros emit methods defined in the Pixar namespace
	// but not properly scoped, so we have to use the namespace
	// locally here - note this isn't strictly true since we had to open
	// the namespace scope anyway because the macros won't allow non-Pixar namespaces
	// to be used because of some auto-generated content
	PXR_NAMESPACE_USING_DIRECTIVE
	if (!TF_VERIFY(layer))
	{
		return false;
	}

	// Parse MVR file
	// ---------------------
	using namespace MVR;
	auto parser = MVRParser();
	auto layers = parser.ParseMVRFile(resolvedPath);

	// Create USD Schema
	// ------------------------
	SdfLayerRefPtr newLayer = SdfLayer::CreateAnonymous(".usd");
	UsdStageRefPtr stage = UsdStage::Open(newLayer);
	auto xformPath = SdfPath("/mvr_payload");
	auto defaultPrim = UsdGeomXform::Define(stage, xformPath);
	stage->SetDefaultPrim(defaultPrim.GetPrim());

	for(const auto& layer : layers)
	{
		const std::string cleanName = CleanNameForUSD(layer.name);

		const auto& layerPath = xformPath.AppendChild(TfToken(CleanNameForUSD(layer.name)));
		auto layerUsd = UsdGeomScope::Define(stage, layerPath);

		for(const auto& fixture : layer.fixtures)
		{
			const std::string cleanFixtureName = CleanNameForUSD(fixture.Name + fixture.UUID);
			const auto& fixturePath = layerPath.AppendChild(TfToken(cleanFixtureName));
			const auto& fixtureUsd = UsdGeomXform::Define(stage, fixturePath);

			GDTF::GDTFSpecification gdtfSpec = parser.GetGDTFSpecification(fixture.GDTFSpec);

			GDTF::ConvertToUsd(gdtfSpec, stage, fixturePath.GetAsString());

			GfMatrix4d transform = GfMatrix4d(
				fixture.Matrix[0][0], fixture.Matrix[0][1], fixture.Matrix[0][2], 0,
				fixture.Matrix[1][0], fixture.Matrix[1][1], fixture.Matrix[1][2], 0,
				fixture.Matrix[2][0], fixture.Matrix[2][1], fixture.Matrix[2][2], 0,
				fixture.Matrix[3][0], fixture.Matrix[3][1], fixture.Matrix[3][2], 1
			);

			// Offset matrix
			GfMatrix3d rotateMinus90deg = GfMatrix3d(1,  0,  0, 
													 0,  0,  1, 
													 0,  -1, 0);

			// Translation
			//transform = transform.GetTranspose();
			GfVec3d translation = rotateMinus90deg * transform.ExtractTranslation() * 0.1;

			// Rotation
			GfRotation rotation = transform.ExtractRotation();
			GfVec3d euler = rotation.Decompose(GfVec3f::XAxis(), GfVec3f::YAxis(), GfVec3f::ZAxis());
			GfVec3d rotate = rotateMinus90deg * euler; // we somehow have a complete 180 offset here.

			// Set transform
			auto fixtureXform = UsdGeomXformable(fixtureUsd);
			fixtureXform.ClearXformOpOrder();
			fixtureXform.AddTranslateOp().Set(translation);

			fixtureXform.AddScaleOp().Set(GfVec3f(100, 100, 100));
			fixtureXform.AddRotateYXZOp(UsdGeomXformOp::PrecisionDouble).Set(rotate);

			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:name"), pxr::SdfValueTypeNames->String).Set(fixture.Name);
			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:uuid"), pxr::SdfValueTypeNames->String).Set(fixture.UUID);	
			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:GDTFSpec"), pxr::SdfValueTypeNames->String).Set(fixture.GDTFSpec);
			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:GDTFMode"), pxr::SdfValueTypeNames->String).Set(fixture.GDTFMode);

			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:Classing"), pxr::SdfValueTypeNames->String).Set(fixture.Classing);

			int i = 0;
			std::string allCommands = "[";
			for(auto cc : fixture.CustomCommands)
			{
				allCommands += cc + ",";
				
				i++;
			}
			allCommands += "]";

			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:CustomCommands"), pxr::SdfValueTypeNames->String).Set(allCommands);

			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:FixtureID"), pxr::SdfValueTypeNames->UInt).Set(fixture.FixtureID);
			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:UnitNumber"), pxr::SdfValueTypeNames->UInt).Set(fixture.UnitNumber);
			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:FixtureTypeId"), pxr::SdfValueTypeNames->UInt).Set(fixture.FixtureTypeID);
			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:CustomId"), pxr::SdfValueTypeNames->UInt).Set(fixture.CustomId);

			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:CastShadow"), pxr::SdfValueTypeNames->Bool).Set(fixture.CastShadows);
		}
	}
	
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

bool MvrFileFormat::_ShouldSkipAnonymousReload() const
{
	return false;
}

bool MvrFileFormat::_ShouldReadAnonymousLayers() const
{
	return true;
}

void MvrFileFormat::ComposeFieldsForFileFormatArguments(const std::string& assetPath, const PcpDynamicFileFormatContext& context, FileFormatArguments* args, VtValue* contextDependencyData) const
{
	return;
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