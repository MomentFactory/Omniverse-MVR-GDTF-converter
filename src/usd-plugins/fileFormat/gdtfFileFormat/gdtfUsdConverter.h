#pragma once
#include "../mvrFileFormat/gdtfParser/ModelSpecification.h"
#include <pxr/usd/usd/stage.h>

namespace GDTF
{
    void ConvertToUsd(const GDTFSpecification& spec, pxr::UsdStageRefPtr stage, const std::string& targetPrimPath = "");
}