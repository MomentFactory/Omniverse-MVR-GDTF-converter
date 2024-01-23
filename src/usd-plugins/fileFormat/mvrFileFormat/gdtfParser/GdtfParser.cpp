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
		m_SpecName = std::experimental::filesystem::path(filePath).filename().string();

		auto zipFile = std::make_shared<ZipFile>(filePath);
		auto spec = HandleGDTF(zipFile);
		spec.SpecName = std::experimental::filesystem::path(zipFile->get_filename()).filename().string();
		return spec;
    }

    GDTF::GDTFSpecification GDTFParser::ParseCompressed(std::shared_ptr<ZipFile> file, const std::string& zipFileName)
    {
        m_TargetPath = std::experimental::filesystem::temp_directory_path().string() + "/";
		m_SpecName = std::experimental::filesystem::path(zipFileName).filename().string();

		auto spec = HandleGDTF(file);
		spec.SpecName = zipFileName;
		return spec;
    }


	bool StringEndsWith(const std::string& input, const std::string& compare)
	{
		if(input.size() >= compare.size())
		{
			return (input.compare(input.length() - compare.length(), compare.length(), compare) == 0);
		}

		return false;
	}

    void GDTFParser::HandleModel(const File& file, const std::string& fixtureName)
	{
		Assimp::Importer importer;

		bool from3ds = StringEndsWith(file.name, "3ds");

		const aiScene* scene = importer.ReadFileFromMemory(file.content.data(), file.content.size() , aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices,"EXTENTION");
		Assimp::Exporter exporter;

		std::experimental::filesystem::path targetPath = m_TargetPath;
		std::experimental::filesystem::path destination = targetPath.parent_path().append(fixtureName);
		std::experimental::filesystem::create_directory(destination);

		std::experimental::filesystem::path convertedFileName = destination.append(std::experimental::filesystem::path(file.name).stem().concat(".gltf").c_str());

		exporter.Export(scene, "gltf2", convertedFileName.string());

		m_GDTFAssets[fixtureName][file.name] = convertedFileName.string();
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
		else if(fileExtension == "3ds" || fileExtension == "glb" || fileExtension == "gltf")
		{
			return FileType::MODEL;
		}

		return FileType::UNKNOWN;
	}

	void GDTFParser::HandleGDTFRecursive(tinyxml2::XMLElement* element, GDTF::GDTFSpecification& spec, int depth)
	{
		if(depth >= 4)
		{
			return; // Avoid stack overflow
		}

		int itCount = 0;
		for(auto* geometry = element->FirstChildElement(); geometry != nullptr; geometry = element->NextSiblingElement())
		{
			itCount++;

			if(itCount > 8)
			{
				return;
			}

			const std::string elementName =  geometry->Name();
			bool isBeam = elementName == "Beam";
			bool isGeometry = elementName == "Geometry";
			bool isAxis = elementName == "Axis";
			bool isInventory = elementName == "Inventory";
			bool isValid = isBeam || isGeometry || isAxis;
			bool isModel = geometry->FindAttribute("Model") != nullptr;

			if(!isValid || !isModel)
				continue;

			auto positionString = geometry->FindAttribute("Position")->Value();
			auto position = StringToMatrix(positionString);

			std::string name = geometry->FindAttribute("Name")->Value();
			std::string model = geometry->FindAttribute("Model")->Value();

			std::replace(name.begin(), name.end(), ' ', '_');

			if(name == "Pigtail" || name == "pigtail" || model == "pigtail" || model == "Pigtail")
			{
				continue;
			}

			Geometry geometrySpec = {};
			geometrySpec.Name = name;
			geometrySpec.Model = model;
			geometrySpec.Transform = position;
			geometrySpec.Depth = depth;

			if(isBeam)
			{
				geometrySpec.isBeam = true;
				
				auto beamPosition = geometry->FindAttribute("Position")->Value();
				float beamRadius = 0.0f;
				if(!geometry->QueryFloatAttribute("BeamRadius", &beamRadius))
				{
					// Failed to find beamRadius.
				}

				geometrySpec.beamRadius = beamRadius;

				spec.BeamRadius = beamRadius;
				spec.BeamMatrix = StringToMatrix(beamPosition);
				spec.HasBeam = true;
			}

			spec.Geometries.push_back(geometrySpec);

			HandleGDTFRecursive(geometry, spec, depth + 1);
		}
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

					if(name.empty())
					{
						name = fixtureType->FindAttribute("LongName")->Value();
					}

					auto physicalDescription = fixtureType->FirstChildElement("PhysicalDescriptions");
					if(physicalDescription)
					{
						auto pdProp = physicalDescription->FirstChildElement("Properties");
						if(pdProp)
						{
							auto temp = pdProp->FirstChildElement("OperatingTemperature");
							auto highXml = temp->FindAttribute("High");
							if(highXml)
							{
								spec.HighTemperature = std::atof(highXml->Value());
							}

							auto lowXml = temp->FindAttribute("Low");
							if(lowXml)
							{
								spec.LowTemperature = std::atof(lowXml->Value());
							}

							auto weightXml = pdProp->FirstChildElement("Weight");
							if(weightXml)
							{
								auto weightValueXml = weightXml->FindAttribute("Value");
								if(weightValueXml)
								{
									spec.Weight = std::atof(weightValueXml->Value());
								}
							}

							auto legHeightXml = pdProp->FirstChildElement("LegHeight");
							if(legHeightXml)
							{
								auto legHeightValueXml = legHeightXml->FindAttribute("Value");
								if(legHeightValueXml)
								{
									spec.LegHeight = std::atof(legHeightValueXml->Value());
								}
							}
						}
					}

					spec.Name = std::string(name);

					auto models = fixtureType->FirstChildElement("Models");

					for(auto* model = models->FirstChildElement("Model"); model; model = model->NextSiblingElement())
					{
						GDTF::ModelSpecification modelSpec;
						modelSpec.Name = model->FindAttribute("Name")->Value();
						modelSpec.File = model->FindAttribute("File")->Value();

						// Fallback if the XML doesnt contain a name
						if(modelSpec.Name.empty())
						{
							modelSpec.Name = modelSpec.File;
						}

						model->QueryFloatAttribute("Length", &modelSpec.Length);
						model->QueryFloatAttribute("Height", &modelSpec.Height);

						spec.Models.push_back(modelSpec);
					}

					int depth = 0;
					
					auto geometries = fixtureType->FirstChildElement("Geometries");

					HandleGDTFRecursive(geometries, spec, 0);

					break;
				}
				case FileType::MODEL:
				{
					if(StringEndsWith(file.name, "3ds"))
					{
						spec.ConvertedFrom3ds = true;
					}

					assetFiles.push_back(file);
					break;
				}
				default:
					break; // Skip unknown file format.
			}
		}

        for(auto& f : assetFiles)
		{
			HandleModel(f, m_SpecName);
		}

        return spec;
    }
}