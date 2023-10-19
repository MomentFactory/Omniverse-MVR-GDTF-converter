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

#ifndef OMNI_MVR_MVRFILEFORMAT_H_
#define OMNI_MVR_MVRFILEFORMAT_H_

#define NOMINMAX

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>

#include "api.h"


PXR_NAMESPACE_OPEN_SCOPE

/// \class MvrFileFormat
///
/// Represents a generic dynamic file format for external data.
/// Actual acquisition of the external data is done via a set
/// of plug-ins to various back-end external data systems.
///
class MVR_API MvrFileFormat : public SdfFileFormat
{
public:
	// SdfFileFormat overrides
	bool CanRead(const std::string& filePath) const override;
	bool Read(SdfLayer* layer, const std::string& resolvedPath, bool metadataOnly) const override;
	bool WriteToString(const SdfLayer& layer, std::string* str, const std::string& comment = std::string()) const override;
	bool WriteToStream(const SdfSpecHandle& spec, std::ostream& out, size_t indent) const override;

protected:

	SDF_FILE_FORMAT_FACTORY_ACCESS;

	virtual ~MvrFileFormat();
	MvrFileFormat();
};

TF_DECLARE_PUBLIC_TOKENS(
	MvrFileFormatTokens,
	((Id, "mvrFileFormat")) 
	((Version, "1.0")) 
	((Target, "usd")) 
	((Extension, "mvr"))
	);

TF_DECLARE_WEAK_AND_REF_PTRS(MvrFileFormat);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
