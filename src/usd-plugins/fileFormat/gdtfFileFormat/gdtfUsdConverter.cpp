#include "gdtfUsdConverter.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usdLux/diskLight.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <pxr/usd/usdLux/rectLight.h>

#include <pxr/base/gf/matrix3f.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/pxr.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>

#include <iostream>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>


namespace GDTF 
{
    std::string CleanNameForUSD(const std::string& name)
    {
        std::string cleanedName = name;
        if(cleanedName.size() == 0)
        {
            return "Default";
        }

        if(cleanedName.size() == 1 && pxr::TfIsValidIdentifier(cleanedName))
        {
            // If we have an index as a name, we only need to add _ beforehand.
            return CleanNameForUSD("_" + cleanedName);
        }

        return pxr::TfMakeValidIdentifier(cleanedName);
    }

    void ConvertToUsd(const GDTFSpecification& spec, pxr::UsdStageRefPtr stage, const std::string& targetPrimPath)
    {
        PXR_NAMESPACE_USING_DIRECTIVE

        SdfPath xformPath;
        if(targetPrimPath.empty())
        {
            xformPath = SdfPath("/default_prim");
            auto defaultPrim = UsdGeomXform::Define(stage, xformPath);
            stage->SetDefaultPrim(defaultPrim.GetPrim());
        }
        else
        {
            xformPath = SdfPath(targetPrimPath);
        }

        GfMatrix3d rotateMinus90deg = GfMatrix3d(1,  0,  0, 
                                                0,  0,  1, 
                                                0,  -1, 0);

        const std::string& parentPath = std::experimental::filesystem::temp_directory_path().string();

        const auto& basePath = xformPath.AppendChild(TfToken("Base"));
        const auto& baseModelPath = basePath.AppendChild(TfToken("model"));
        const auto& baseXform = UsdGeomXform::Define(stage, basePath);
        const auto& baseModelXform = UsdGeomXform::Define(stage, baseModelPath);
 
        // Add GDTF custom properties to parent prim
        auto fixturePrim = stage->GetPrimAtPath(xformPath);
        fixturePrim.GetPrim().CreateAttribute(TfToken("mf:gdtf:LegHeight"), pxr::SdfValueTypeNames->Float).Set(spec.LegHeight);
        fixturePrim.GetPrim().CreateAttribute(TfToken("mf:gdtf:OperatingTemperature:High"), pxr::SdfValueTypeNames->Float).Set(spec.HighTemperature);
        fixturePrim.GetPrim().CreateAttribute(TfToken("mf:gdtf:OperatingTemperature:Low"), pxr::SdfValueTypeNames->Float).Set(spec.LowTemperature);
        fixturePrim.GetPrim().CreateAttribute(TfToken("mf:gdtf:Weight"), pxr::SdfValueTypeNames->Float).Set(spec.Weight);

        bool from3ds = spec.ConvertedFrom3ds;
        float modelScaleFactory = spec.ConvertedFrom3ds ? 0.001f : 1.0f;
        float modelBaseRotateAngle = from3ds ? -90.0f : 0.0f;

        if(spec.Name.empty())
        {
            std::cout << "spec name is empty! " << std::endl;
        }

        SdfPath geoPath = xformPath;
        for(auto& geometry : spec.Geometries)
        {
            if(geometry.Name.empty())
            {
                continue;
            }

            geoPath = geoPath.AppendChild(TfToken(CleanNameForUSD(geometry.Name)));

            const auto& xform = UsdGeomXform::Define(stage, geoPath);

            GfMatrix4d transform = GfMatrix4d(
                geometry.Transform[0][0], geometry.Transform[1][0], geometry.Transform[2][0], 0,
                geometry.Transform[0][1], geometry.Transform[1][1], geometry.Transform[2][1], 0,
                geometry.Transform[0][2], geometry.Transform[1][2], geometry.Transform[2][2], 0,
                geometry.Transform[0][3], geometry.Transform[1][3], geometry.Transform[2][3], 1
            );

            GfVec3d translation = rotateMinus90deg * transform.ExtractTranslation();
            GfRotation rotation = transform.GetTranspose().ExtractRotation();
            GfVec3d euler = rotation.Decompose(GfVec3f::XAxis(), GfVec3f::YAxis(), GfVec3f::ZAxis());
            GfVec3d rotate = rotateMinus90deg * euler;

            // Set transform
            xform.ClearXformOpOrder();
            xform.AddTranslateOp().Set(translation);
            xform.AddRotateYZXOp(UsdGeomXformOp::PrecisionDouble).Set(rotate);
            xform.AddScaleOp().Set(GfVec3f(1.0));

            if(!geometry.isBeam)
            {
                const auto& modelPath = geoPath.AppendChild(TfToken("model"));
                const auto& modelXform = UsdGeomXform::Define(stage, modelPath);
                modelXform.AddTranslateOp().Set(GfVec3d(0));
                modelXform.AddRotateYZXOp(UsdGeomXformOp::PrecisionDouble).Set(GfVec3d(modelBaseRotateAngle, 0, 0));

                auto scaleOp = modelXform.AddScaleOp();
                if(from3ds)
                {
                    scaleOp.Set(GfVec3f(modelScaleFactory));
                }   
                
                std::string fileName = "";
                for(auto m : spec.Models)
                {
                    if(m.Name == geometry.Model)
                    {
                        fileName = m.File;
                    }
                }

                std::string payloadPath = parentPath + "/" + spec.SpecName + "/" + fileName + ".gltf";
                modelXform.GetPrim().GetPayloads().AddPayload(SdfPayload(payloadPath));
            }
            else
            {
                SdfPath lightPath = geoPath.AppendChild(TfToken("Beam"));
                auto diskLight = UsdLuxDiskLight::Define(stage, lightPath);
                auto lightXform = UsdGeomXformable(diskLight);
                float heightOffset = 0.0f;

                const auto modelSpecIt = std::find_if(spec.Models.begin(), spec.Models.end(), 
                    [](const ModelSpecification& model) { 
                        return model.Name == std::string("Beam"); 
                    }
                );

                if(modelSpecIt != spec.Models.end())
                {
                    const ModelSpecification& modelSpec = *modelSpecIt;
                    heightOffset = modelSpec.Height;
                }

                lightXform.ClearXformOpOrder();
                lightXform.AddTranslateOp().Set(GfVec3d(0, -heightOffset * 0.5, 0));
                lightXform.AddRotateYXZOp(UsdGeomXformOp::PrecisionDouble).Set(GfVec3d(-90, 0, 0));
                lightXform.AddScaleOp().Set(GfVec3f(spec.BeamRadius * 2.0, spec.BeamRadius * 2.0, 1));
                diskLight.GetPrim().CreateAttribute(
                    TfToken("intensity"), 
                    SdfValueTypeNames->Float
                ).Set(60000.0f);

                diskLight.GetPrim().CreateAttribute(
                    TfToken("visibleInPrimaryRay"), 
                    SdfValueTypeNames->Bool
                ).Set(true);

                diskLight.GetPrim().CreateAttribute(TfToken("mf:gdtf:BeamAngle"), pxr::SdfValueTypeNames->Float).Set(spec.BeamAngle);
                diskLight.GetPrim().CreateAttribute(TfToken("mf:gdtf:BeamType"), pxr::SdfValueTypeNames->String).Set(spec.BeamType);
                diskLight.GetPrim().CreateAttribute(TfToken("mf:gdtf:ColorRenderingIndex"), pxr::SdfValueTypeNames->Int).Set(spec.ColorRenderingIndex);
                diskLight.GetPrim().CreateAttribute(TfToken("mf:gdtf:ColorTemperature"), pxr::SdfValueTypeNames->Float).Set(spec.ColorTemperature);
                diskLight.GetPrim().CreateAttribute(TfToken("mf:gdtf:FieldAngle"), pxr::SdfValueTypeNames->Float).Set(spec.FieldAngle);
                diskLight.GetPrim().CreateAttribute(TfToken("mf:gdtf:LampType"), pxr::SdfValueTypeNames->String).Set(spec.LampType);
                diskLight.GetPrim().CreateAttribute(TfToken("mf:gdtf:PowerConsumption"), pxr::SdfValueTypeNames->Float).Set(spec.PowerConsumption);
                diskLight.GetPrim().CreateAttribute(TfToken("mf:gdtf:LuminousFlux"), pxr::SdfValueTypeNames->Float).Set(spec.LuminousFlux);
            }
        }
    }
}