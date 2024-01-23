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

#include "gdtfFileFormat.h"

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
#include <pxr/base/gf/rotation.h>
#include <pxr/usd/usd/payloads.h>

#include "../mvrFileFormat/gdtfParser/GdtfParser.h"
#include "gdtfUsdConverter.h"

#include <fstream>
#include <cmath>
#include <iostream>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

PXR_NAMESPACE_OPEN_SCOPE

GdtfFileFormat::GdtfFileFormat() : SdfFileFormat(
										GdtfFileFormatTokens->Id,
										GdtfFileFormatTokens->Version,
										GdtfFileFormatTokens->Target,
										GdtfFileFormatTokens->Extension)
{
}

GdtfFileFormat::~GdtfFileFormat()
{
}

bool GdtfFileFormat::CanRead(const std::string& filePath) const
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

bool GdtfFileFormat::Read(SdfLayer* layer, const std::string& resolvedPath, bool metadataOnly) const
{
	// Do parsing here...
	// TF_CODING_ERROR to throw errors
	// Create a new anonymous layer and wrap a stage around it.
    PXR_NAMESPACE_USING_DIRECTIVE
	if (!TF_VERIFY(layer))
	{
		return false;
	}

    SdfLayerRefPtr newLayer = SdfLayer::CreateAnonymous(".usd");
	UsdStageRefPtr stage = UsdStage::Open(newLayer);

	// Parse GDTF file
    auto parser = GDTF::GDTFParser();
    GDTF::GDTFSpecification device = parser.ParseGDTFFile(resolvedPath);
	
    // Write to stage
    GDTF::ConvertToUsd(device, stage);

    // Copy contents into output layer.
    layer->TransferContent(newLayer);

	return true;
}

bool GdtfFileFormat::WriteToString(const SdfLayer& layer, std::string* str, const std::string& comment) const
{
	// Implementation for two-way writting potentially
	return false;
}

bool GdtfFileFormat::WriteToStream(const SdfSpecHandle& spec, std::ostream& out, size_t indent) const
{
	// this POC doesn't support writing
	return false;
}

bool GdtfFileFormat::_ShouldSkipAnonymousReload() const
{
	return false;
}

bool GdtfFileFormat::_ShouldReadAnonymousLayers() const
{
	return true;
}

void GdtfFileFormat::ComposeFieldsForFileFormatArguments(const std::string& assetPath, const PcpDynamicFileFormatContext& context, FileFormatArguments* args, VtValue* contextDependencyData) const
{
}

bool GdtfFileFormat::CanFieldChangeAffectFileFormatArguments(const TfToken& field, const VtValue& oldValue, const VtValue& newValue, const VtValue& contextDependencyData) const
{
	return true;
}


// these macros emit methods defined in the Pixar namespace
// but not properly scoped, so we have to use the namespace
// locally here
TF_DEFINE_PUBLIC_TOKENS(
	GdtfFileFormatTokens,
	((Id, "gdtfFileFormat"))
	((Version, "1.0"))
	((Target, "usd"))
	((Extension, "gdtf"))
);

TF_REGISTRY_FUNCTION(TfType)
{
	SDF_DEFINE_FILE_FORMAT(GdtfFileFormat, SdfFileFormat);
}

PXR_NAMESPACE_CLOSE_SCOPE