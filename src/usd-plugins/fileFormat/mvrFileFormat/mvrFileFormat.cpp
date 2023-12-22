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

			GfMatrix4d transform = GfMatrix4d(
				fixture.Matrix[0][0], fixture.Matrix[0][1], fixture.Matrix[0][2], 0,
				fixture.Matrix[1][0], fixture.Matrix[1][1], fixture.Matrix[1][2], 0,
				fixture.Matrix[2][0], fixture.Matrix[2][1], fixture.Matrix[2][2], 0,
				fixture.Matrix[3][0], fixture.Matrix[3][1], fixture.Matrix[3][2], 1
			);

			// Offset matrix
			GfMatrix3d rotateMinus90deg = GfMatrix3d(1,  0, 0, 
													 0,  0, 1, 
													 0, -1, 0);

			// Translation
			GfVec3d translation = rotateMinus90deg * transform.ExtractTranslation();

			// Rotation
			GfRotation rotation = transform.ExtractRotation();
			GfVec3d euler = rotation.Decompose(GfVec3f::XAxis(), GfVec3f::YAxis(), GfVec3f::ZAxis());
			GfVec3d rotate = euler;

			// Set transform
			auto fixtureXform = UsdGeomXformable(fixtureUsd);
			fixtureXform.ClearXformOpOrder();
			fixtureXform.AddTranslateOp().Set(translation * 0.1f);
			fixtureXform.AddRotateZYXOp(UsdGeomXformOp::PrecisionDouble).Set(rotate);

			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:name"), pxr::SdfValueTypeNames->String).Set(fixture.Name);
			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:uuid"), pxr::SdfValueTypeNames->String).Set(fixture.UUID);	
			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:GDTFSpec"), pxr::SdfValueTypeNames->String).Set(fixture.GDTFSpec);
			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:GDTFMode"), pxr::SdfValueTypeNames->String).Set(fixture.GDTFMode);

			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:Classing"), pxr::SdfValueTypeNames->String).Set(fixture.Classing);

			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:FixtureID"), pxr::SdfValueTypeNames->UInt).Set(fixture.FixtureID);
			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:UnitNumber"), pxr::SdfValueTypeNames->UInt).Set(fixture.UnitNumber);
			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:FixtureTypeId"), pxr::SdfValueTypeNames->UInt).Set(fixture.FixtureTypeID);
			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:CustomId"), pxr::SdfValueTypeNames->UInt).Set(fixture.CustomId);

			fixtureUsd.GetPrim().CreateAttribute(TfToken("mf:mvr:CastShadow"), pxr::SdfValueTypeNames->Bool).Set(fixture.CastShadows);

			const auto& basePath = fixturePath.AppendChild(TfToken("Base"));
			const auto& baseModelPath = basePath.AppendChild(TfToken("model"));
			const auto& baseXform = UsdGeomXform::Define(stage, basePath);
			const auto& baseModelXform = UsdGeomXform::Define(stage, baseModelPath);

			const auto& yokePath = basePath.AppendChild(TfToken("Yoke"));
			const auto& yokeModelPath = yokePath.AppendChild(TfToken("model"));
			const auto& yokeXform = UsdGeomXform::Define(stage, yokePath);
			const auto& yokeModelXform = UsdGeomXform::Define(stage, yokeModelPath);

			const auto& bodyPath = yokePath.AppendChild(TfToken("Body"));
			const auto& bodyModelPath = bodyPath.AppendChild(TfToken("model"));
			const auto& bodyXform = UsdGeomXform::Define(stage, bodyPath);
			const auto& bodyModelXform = UsdGeomXform::Define(stage, bodyModelPath);

			std::cout << "fixturename sname:" << fixture.Name << std::endl;
			GDTF::GDTFSpecification gdtfSpec = parser.GetGDTFSpecification(fixture.Name);

			const std::string& parentPath = std::experimental::filesystem::temp_directory_path().string();
			bodyModelXform.GetPrim().GetPayloads().AddPayload(SdfPayload((parentPath + "/" + fixture.Name + "/" + "Body.gltf")));
			yokeModelXform.GetPrim().GetPayloads().AddPayload(SdfPayload((parentPath + "/" + fixture.Name + "/" + "Yoke.gltf")));
			baseModelXform.GetPrim().GetPayloads().AddPayload(SdfPayload((parentPath + "/" + fixture.Name + "/" + "Base.gltf")));

			if(parser.HasGDTFSpecification(fixture.Name))
			{
				std::cout << " HAS SPECIFIACTION!!" << std::endl;
			}
			// BODY
			GfMatrix4d bodyTransform = GfMatrix4d(
				gdtfSpec.BodyMatrix[0][0], gdtfSpec.BodyMatrix[1][0], gdtfSpec.BodyMatrix[2][0], 0,
				gdtfSpec.BodyMatrix[0][1], gdtfSpec.BodyMatrix[1][1], gdtfSpec.BodyMatrix[2][1], 0,
				gdtfSpec.BodyMatrix[0][2], gdtfSpec.BodyMatrix[1][2], gdtfSpec.BodyMatrix[2][2], 0,
				gdtfSpec.BodyMatrix[0][3], gdtfSpec.BodyMatrix[1][3], gdtfSpec.BodyMatrix[2][3], 1
			);


			for(int x = 0; x < 4; x++)
			{
				for(int y = 0; y < 4; y++)
				{
					std::cout << bodyTransform[x][y] << ", ";
				}

				std::cout << std::endl;
			}


			GfVec3d bodyTranslation = rotateMinus90deg * bodyTransform.ExtractTranslation();

			std::cout << "body Translation: " << bodyTranslation << std::endl;
			GfRotation bodyRotation = bodyTransform.ExtractRotation();
			GfVec3d bodyEuler = bodyRotation.Decompose(GfVec3f::XAxis(), GfVec3f::YAxis(), GfVec3f::ZAxis());
			GfVec3d bodyRotate = bodyEuler;

			bodyXform.ClearXformOpOrder();
			bodyXform.AddTranslateOp().Set(bodyTranslation);
			bodyXform.AddRotateZYXOp(UsdGeomXformOp::PrecisionDouble).Set(bodyRotate);
			bodyXform.AddScaleOp().Set(GfVec3f(1, 1, 1));

			// YOKE
			GfMatrix4d yokeTransform = GfMatrix4d(
				gdtfSpec.YokeMatrix[0][0], gdtfSpec.YokeMatrix[1][0], gdtfSpec.YokeMatrix[2][0], gdtfSpec.YokeMatrix[3][0],
				gdtfSpec.YokeMatrix[0][1], gdtfSpec.YokeMatrix[1][1], gdtfSpec.YokeMatrix[2][1], gdtfSpec.YokeMatrix[3][1],
				gdtfSpec.YokeMatrix[0][2], gdtfSpec.YokeMatrix[1][2], gdtfSpec.YokeMatrix[2][2], gdtfSpec.YokeMatrix[3][2],
				gdtfSpec.YokeMatrix[0][3], gdtfSpec.YokeMatrix[1][3], gdtfSpec.YokeMatrix[2][3], gdtfSpec.YokeMatrix[3][3]
			);

			for(int x = 0; x < 4; x++)
			{
				for(int y = 0; y < 4; y++)
				{
					std::cout << yokeTransform[x][y] << ", ";
				}

				std::cout << std::endl;
			}

			GfVec3d yokeTranslation = yokeTransform.ExtractTranslation();
			std::cout << "yoke Translation: " << yokeTranslation << std::endl;
			GfRotation yokeRotation = yokeTransform.ExtractRotation();
			GfVec3d yokeEuler = yokeRotation.Decompose(GfVec3f::XAxis(), GfVec3f::YAxis(), GfVec3f::ZAxis());
			GfVec3d yokeRotate = yokeEuler;

			// Set transform
			yokeXform.ClearXformOpOrder();
			yokeXform.AddTranslateOp().Set(yokeTranslation);
			yokeXform.AddRotateZYXOp(UsdGeomXformOp::PrecisionDouble).Set(yokeRotate);
			yokeXform.AddScaleOp().Set(GfVec3f(1, 1, 1));

			// BASE
			GfMatrix4d baseTransform = GfMatrix4d(
				gdtfSpec.BaseMatrix[0][0], gdtfSpec.BaseMatrix[1][0], gdtfSpec.BaseMatrix[2][0], gdtfSpec.BaseMatrix[3][0],
				gdtfSpec.BaseMatrix[0][1], gdtfSpec.BaseMatrix[1][1], gdtfSpec.BaseMatrix[2][1], gdtfSpec.BaseMatrix[3][1],
				gdtfSpec.BaseMatrix[0][2], gdtfSpec.BaseMatrix[1][2], gdtfSpec.BaseMatrix[2][2], gdtfSpec.BaseMatrix[3][2],
				gdtfSpec.BaseMatrix[0][3], gdtfSpec.BaseMatrix[1][3], gdtfSpec.BaseMatrix[2][3], gdtfSpec.BaseMatrix[3][3]
			);

			for(int x = 0; x < 4; x++)
			{
				for(int y = 0; y < 4; y++)
				{
					std::cout << baseTransform[x][y] << ", ";
				}

				std::cout << std::endl;
			}

			GfVec3d baseTranslation = rotateMinus90deg * baseTransform.ExtractTranslation();
			std::cout << "base Translation: " << baseTranslation << std::endl;
			GfRotation baseRotation = baseTransform.ExtractRotation();
			GfVec3d baseEuler = baseRotation.Decompose(GfVec3f::XAxis(), GfVec3f::YAxis(), GfVec3f::ZAxis());
			GfVec3d baseRotate = baseEuler;

			// Set transform
			baseXform.ClearXformOpOrder();
			baseXform.AddTranslateOp().Set(baseTranslation);
			baseXform.AddRotateZYXOp(UsdGeomXformOp::PrecisionDouble).Set(baseRotate);
			baseXform.AddScaleOp().Set(GfVec3f(1, 1, 1));
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