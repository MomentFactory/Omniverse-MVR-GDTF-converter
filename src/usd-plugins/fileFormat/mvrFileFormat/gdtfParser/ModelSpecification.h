#include <string>
#include <vector>

namespace GDTF {

    struct ModelSpecification
    {
        std::string Name;
        std::string Id;
        std::string File;
        std::string ConvertedFilePath;
    };

    struct GDTFSpecification
    {
        std::string Name;
        std::vector<ModelSpecification> Models;
    };

}