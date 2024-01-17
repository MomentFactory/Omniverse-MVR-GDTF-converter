#pragma once
#include <string>
#include <vector>
#include "Device.h"

namespace GDTF {

    struct ModelSpecification
    {
        std::string Name;
        std::string Id;
        std::string File;
        std::string ConvertedFilePath;
        float Height = 0.0f;
        float Length = 0.0f;
    };

    struct GDTFSpecification
    {
        std::string Name;
        std::vector<ModelSpecification> Models;

        GDTFMatrix BaseMatrix = GDTFMatrix();
        GDTFMatrix BodyMatrix = GDTFMatrix();
        GDTFMatrix YokeMatrix = GDTFMatrix();

        int TreeDepth = 0;
        bool HasBeam = false;
        float BeamRadius = 0.0f;
        GDTFMatrix BeamMatrix = GDTFMatrix();
    };

}