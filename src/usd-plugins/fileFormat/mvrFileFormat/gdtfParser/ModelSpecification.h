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
        int Depth = 0;
        float Height = 0.0f;
        float Length = 0.0f;
    };

    struct Geometry
    {
        int Depth = 0.0f;
        GDTFMatrix Transform;
        std::string Name;
        std::string Model;
        bool isBeam = false;
        float beamRadius = 0.0f;
    };

    struct GDTFSpecification
    {
        std::string Name;
        std::string SpecName;
        
        bool ConvertedFrom3ds = false;

        std::vector<ModelSpecification> Models;

        std::vector<Geometry> Geometries;

        GDTFMatrix BaseMatrix = GDTFMatrix();
        GDTFMatrix BodyMatrix = GDTFMatrix();
        GDTFMatrix YokeMatrix = GDTFMatrix();

        int TreeDepth = 0;
        bool HasBeam = false;
        float BeamRadius = 0.0f;
        GDTFMatrix BeamMatrix = GDTFMatrix();
    };

}