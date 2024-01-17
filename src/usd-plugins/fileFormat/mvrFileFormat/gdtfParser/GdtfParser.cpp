#include "GdtfParser.h"

#ifdef GDTF_FILEFORMAT
#include "../mvrFileFormat/mvrParser/zip_file2.hpp"
#endif

#include "../mvrFileFormat/mvrParser/tinyxml2.h"

#include "../mvrFileFormat/assimp/include/assimp/scene.h"
#include "../mvrFileFormat/assimp/include/assimp/postprocess.h"
#include "../mvrFileFormat/assimp/include/assimp/Importer.hpp"
#include "../mvrFileFormat/assimp/include/assimp/Exporter.hpp"

#include <iostream>
#include <map>
#include <sstream>
#include <fstream>

#define MINIZ_HEADER_FILE_ONLY
#include "../mvrFileFormat/mvrParser/zip_file2.hpp"
#undef MINIZ_HEADER_FILE_ONLY

using ZipInfo = miniz_cpp2::zip_info;
using ZipInfoList = std::vector<ZipInfo>;
using ZipFile = miniz_cpp2::zip_file;

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

namespace GDTF {

    std::vector<std::string> GDTFParser::StringSplit(const std::string& input, const char delimiter)
	{
		std::vector<std::string> result;
		std::stringstream ss(input);
		std::string item;

		while (getline(ss, item, delimiter))
		{
			result.push_back(item);
		}

		return result;
	}

    GDTF::GDTFMatrix StringToMatrix(const std::string& inputString)
	{
		std::string input = inputString;
		size_t pos;
		while ((pos = input.find("}{")) != std::string::npos) 
		{
			input.replace(pos, 2, " ");
		}

		for (char& c : input) 
		{
			if (c == ',' || c == ';' || c == '{' || c == '}') 
			{
				c = ' ';
			}
		}

		GDTF::GDTFMatrix output;
		std::istringstream iss(input);
		for (int i = 0; i < 4; ++i) 
		{
			for (int j = 0; j < 4; ++j) 
			{
				if (!(iss >> output[i][j])) 
				{
				}
			}
		}

		return output;
	}

	bool GDTFParser::FileExists(const std::string& path) const
	{
		const std::ifstream filePath(path);
		return filePath.good();
	}
	
    GDTF::GDTFSpecification GDTFParser::ParseGDTFFile(const std::string& filePath)
    {
        if (!FileExists(filePath))
		{
			m_Errors.push("Failed to parse GDTF file: file doesn't exists - " + filePath);
			return {};
		}

        m_TargetPath = std::experimental::filesystem::temp_directory_path().string() + "/";

		auto zipFile = std::make_shared<ZipFile>(filePath);

		return HandleGDTF(zipFile);
    }

    GDTF::GDTFSpecification GDTFParser::ParseCompressed(std::shared_ptr<ZipFile> file)
    {
        m_TargetPath = std::experimental::filesystem::temp_directory_path().string() + "/";
		return HandleGDTF(file);
    }

    void GDTFParser::HandleModel(const File& file, const std::string& fixtureName)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFileFromMemory(file.content.data(), file.content.size() , aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices,"EXTENTION");

		Assimp::Exporter exporter;

		std::experimental::filesystem::path targetPath = m_TargetPath;
		std::experimental::filesystem::path destination = targetPath.parent_path().append(fixtureName);
		std::experimental::filesystem::create_directory(destination);

		std::experimental::filesystem::path convertedFileName = destination.append(std::experimental::filesystem::path(file.name).stem().concat(".gltf").c_str());

		exporter.Export(scene, "gltf2", convertedFileName.string());

		m_GDTFAssets[fixtureName][file.name] = convertedFileName.string();
		std::cout << "inserted:" << fixtureName << " input:" << file.name << " output:" << convertedFileName << std::endl;
	}

    std::string GDTFParser::GetFileExtension(const std::string& fileName)
	{
		const auto& fileNameSplits = StringSplit(fileName, '.');
		const std::string fileExtension = fileNameSplits[fileNameSplits.size() - 1];
		return fileExtension;
	}

	FileType GDTFParser::GetFileTypeFromExtension(const std::string& fileExtension)
	{
		if (fileExtension == "xml")
		{
			return FileType::XML;
		}
		else if (fileExtension == "3ds")
		{
			return FileType::MODEL;
		}
		else if(fileExtension == "gdtf")
		{
			return FileType::GDTF;
		}
		else if(fileExtension == "3ds")
		{
			return FileType::MODEL;
		}

		return FileType::UNKNOWN;
	}

    GDTF::GDTFSpecification GDTFParser::HandleGDTF(std::shared_ptr<ZipFile>& zipFile)
    {
		std::map<std::string, std::vector<File>> fixtures;
		std::vector<File> assetFiles;

		GDTF::GDTFSpecification spec{};
		
		for (const ZipInfo& info : zipFile->infolist())
		{
			std::cout << info.filename << std::endl;
			const std::string& fileContent = zipFile->read(info);
			File file = { info.filename, fileContent };

			const FileType fileType = GetFileTypeFromExtension(GetFileExtension(info.filename));
			switch (fileType)
			{
				case FileType::XML:
				{
					tinyxml2::XMLDocument doc;
					if (doc.Parse(file.content.c_str()) != tinyxml2::XML_SUCCESS)
					{
						m_Errors.push("Failed to parse XML file: " + file.name);
						return {};
					}

					tinyxml2::XMLElement* root = doc.RootElement();

					auto fixtureType = root->FirstChildElement("FixtureType");
					std::string name = (fixtureType->FindAttribute("Name"))->Value();
					spec.Name = name;
					auto models = fixtureType->FirstChildElement("Models");

					for(auto* model = models->FirstChildElement("Model"); model; model = model->NextSiblingElement())
					{
						GDTF::ModelSpecification modelSpec;
						modelSpec.Name = model->FindAttribute("Name")->Value();
						modelSpec.File = model->FindAttribute("File")->Value();

						spec.Models.push_back(modelSpec);
					}

					auto geometries = fixtureType->FirstChildElement("Geometries");

					auto axisBase = geometries->FirstChildElement("Axis");
					auto axisBasePosition = axisBase->FindAttribute("Position")->Value();
					
					auto axisYoke = axisBase->FirstChildElement("Axis");
					auto axisYokePosition = axisYoke->FindAttribute("Position")->Value();

					auto axisBody = axisYoke->FirstChildElement("Axis");
					auto axisBodyPosition = axisBody->FindAttribute("Position")->Value();

					auto baseMatrix = StringToMatrix(axisBasePosition);
					auto yokeMatrix = StringToMatrix(axisYokePosition);
					auto bodyMatrix = StringToMatrix(axisBodyPosition);

					spec.BaseMatrix = baseMatrix;
					spec.BodyMatrix = bodyMatrix;
					spec.YokeMatrix = yokeMatrix;
					break;
				}
				case FileType::MODEL:
				{
					assetFiles.push_back(file);
					break;
				}
				default:
					break; // Skip unknown file format.
			}
		}

        for(auto& f : assetFiles)
		{
			HandleModel(f, spec.Name);
		}

        return spec;
    }
}