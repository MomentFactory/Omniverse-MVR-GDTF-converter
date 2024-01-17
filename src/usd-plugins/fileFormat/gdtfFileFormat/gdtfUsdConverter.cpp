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

#include <pxr/usd/usdLux/rectLight.h>

#include <pxr/base/gf/matrix3f.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/usd/usd/payloads.h>

#include <iostream>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

namespace GDTF 
{
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
        baseModelXform.GetPrim().GetPayloads().AddPayload(SdfPayload((parentPath + "/" + spec.Name + "/" + "Base.gltf")));
        // BASE
        GfMatrix4d baseTransform = GfMatrix4d(
            spec.BaseMatrix[0][0], spec.BaseMatrix[1][0], spec.BaseMatrix[2][0], 0,
            spec.BaseMatrix[0][1], spec.BaseMatrix[1][1], spec.BaseMatrix[2][1], 0,
            spec.BaseMatrix[0][2], spec.BaseMatrix[1][2], spec.BaseMatrix[2][2], 0,
            spec.BaseMatrix[0][3], spec.BaseMatrix[1][3], spec.BaseMatrix[2][3], 1
        );

        GfVec3d baseTranslation = rotateMinus90deg * baseTransform.ExtractTranslation();
        GfRotation baseRotation = baseTransform.ExtractRotation();
        GfVec3d baseEuler = baseRotation.Decompose(GfVec3f::XAxis(), GfVec3f::YAxis(), GfVec3f::ZAxis());
        GfVec3d baseRotate = baseEuler;

        // Set transform
        baseXform.ClearXformOpOrder();
        baseXform.AddTranslateOp().Set(baseTranslation * 1000.0);
        baseXform.AddRotateYZXOp(UsdGeomXformOp::PrecisionDouble).Set(baseRotate);
        baseXform.AddScaleOp().Set(GfVec3f(1, 1, 1));

        const auto& yokePath = basePath.AppendChild(TfToken("Yoke"));
        if(spec.TreeDepth > 1)
        {
            const auto& yokeModelPath = yokePath.AppendChild(TfToken("model"));
            const auto& yokeXform = UsdGeomXform::Define(stage, yokePath);
            const auto& yokeModelXform = UsdGeomXform::Define(stage, yokeModelPath);
            yokeModelXform.GetPrim().GetPayloads().AddPayload(SdfPayload((parentPath + "/" + spec.Name + "/" + "Yoke.gltf")));

            GfMatrix4d yokeTransform = GfMatrix4d(
                spec.YokeMatrix[0][0], spec.YokeMatrix[1][0], spec.YokeMatrix[2][0], 0,
                spec.YokeMatrix[0][1], spec.YokeMatrix[1][1], spec.YokeMatrix[2][1], 0,
                spec.YokeMatrix[0][2], spec.YokeMatrix[1][2], spec.YokeMatrix[2][2], 0,
                spec.YokeMatrix[0][3], spec.YokeMatrix[1][3], spec.YokeMatrix[2][3], 1
            );

            GfVec3d yokeTranslation = rotateMinus90deg * yokeTransform.ExtractTranslation();
            GfRotation yokeRotation = yokeTransform.ExtractRotation();
            GfVec3d yokeEuler = yokeRotation.Decompose(GfVec3f::XAxis(), GfVec3f::YAxis(), GfVec3f::ZAxis());
            GfVec3d yokeRotate = yokeEuler;

            // Set transform
            yokeXform.ClearXformOpOrder();
            yokeXform.AddTranslateOp().Set(yokeTranslation * 1000.0);
            yokeXform.AddRotateYZXOp(UsdGeomXformOp::PrecisionDouble).Set(yokeRotate);
            yokeXform.AddScaleOp().Set(GfVec3f(1, 1, 1));

            if(spec.TreeDepth == 2 && spec.HasBeam)
            {
                SdfPath lightPath = yokeModelPath.AppendChild(TfToken("Yoke"));
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
                lightXform.AddTranslateOp().Set(GfVec3d(0, -heightOffset * 0.5, 0) * 1000.0);
                lightXform.AddRotateYZXOp(UsdGeomXformOp::PrecisionDouble).Set(GfVec3d(-90, 0, 0));
                lightXform.AddScaleOp().Set(GfVec3f(spec.BeamRadius * 2.0, spec.BeamRadius * 2.0, 1));
                diskLight.GetPrim().CreateAttribute(
                    TfToken("intensity"), 
                    SdfValueTypeNames->Float
                ).Set(60000.0f);

                diskLight.GetPrim().CreateAttribute(
                    TfToken("visibleInPrimaryRay"), 
                    SdfValueTypeNames->Bool
                ).Set(true);
            }
        }

        if(spec.TreeDepth > 2)
        {
            const auto& bodyPath = yokePath.AppendChild(TfToken("Body"));
            const auto& bodyModelPath = bodyPath.AppendChild(TfToken("model"));
            const auto& bodyXform = UsdGeomXform::Define(stage, bodyPath);
            const auto& bodyModelXform = UsdGeomXform::Define(stage, bodyModelPath);
            bodyModelXform.GetPrim().GetPayloads().AddPayload(SdfPayload((parentPath + "/" + spec.Name + "/" + "Body.gltf")));

            // BODY
            GfMatrix4d bodyTransform = GfMatrix4d(
                spec.BodyMatrix[0][0], spec.BodyMatrix[1][0], spec.BodyMatrix[2][0], 0,
                spec.BodyMatrix[0][1], spec.BodyMatrix[1][1], spec.BodyMatrix[2][1], 0,
                spec.BodyMatrix[0][2], spec.BodyMatrix[1][2], spec.BodyMatrix[2][2], 0,
                spec.BodyMatrix[0][3], spec.BodyMatrix[1][3], spec.BodyMatrix[2][3], 1
            );

            GfVec3d bodyTranslation = rotateMinus90deg * bodyTransform.ExtractTranslation();

            GfRotation bodyRotation = bodyTransform.ExtractRotation();
            GfVec3d bodyEuler = bodyRotation.Decompose(GfVec3f::XAxis(), GfVec3f::YAxis(), GfVec3f::ZAxis());
            GfVec3d bodyRotate = bodyEuler;

            bodyXform.ClearXformOpOrder();
            bodyXform.AddTranslateOp().Set(bodyTranslation * 1000.0);
            bodyXform.AddRotateYZXOp(UsdGeomXformOp::PrecisionDouble).Set(bodyRotate);
            bodyXform.AddScaleOp().Set(GfVec3f(1, 1, 1));

            if(spec.TreeDepth == 3 && spec.HasBeam)
            {
                SdfPath lightPath = bodyPath.AppendChild(TfToken("Body"));
                auto diskLight = UsdLuxDiskLight::Define(stage, lightPath);
                auto lightXform = UsdGeomXformable(diskLight);

                float heightOffset = 0.0f;
                const auto modelSpecIt = std::find_if(spec.Models.begin(), spec.Models.end(), 
                    [](const ModelSpecification& model) { 
                        return model.Name == std::string("Body"); 
                    }
                );

                if(modelSpecIt != spec.Models.end())
                {
                    const ModelSpecification& modelSpec = *modelSpecIt;
                    heightOffset = modelSpec.Height;
                }

                std::cout << "found offset:" << std::to_string(heightOffset) << std::endl;

                lightXform.ClearXformOpOrder();
                lightXform.AddTranslateOp().Set(GfVec3d(0, -heightOffset * 0.5, 0) * 1000.0);
                lightXform.AddRotateYZXOp(UsdGeomXformOp::PrecisionDouble).Set(GfVec3d(-90, 0, 0));
                lightXform.AddScaleOp().Set(GfVec3f(spec.BeamRadius * 2.0, spec.BeamRadius * 2.0, 1));
                diskLight.GetPrim().CreateAttribute(
                    TfToken("intensity"), 
                    SdfValueTypeNames->Float
                ).Set(60000.0f);

                diskLight.GetPrim().CreateAttribute(
                    TfToken("visibleInPrimaryRay"), 
                    SdfValueTypeNames->Bool
                ).Set(true);

            }
        }



    }
}