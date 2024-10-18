#include "LayerFactory.h"
#include "MVRParser.h"

#include "../gdtfParser/GdtfParser.h"

#include "assimp/Exporter.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "zip_file2.hpp"
#include "tinyxml2.h"

#include <sstream>
#include <fstream>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

using ZipInfo = miniz_cpp2::zip_info;
using ZipInfoList = std::vector<ZipInfo>;

namespace MVR {

	std::vector<LayerSpecification> MVRParser::ParseMVRFile(const std::string& path)
	{
		if (!FileExists(path))
		{
			m_Errors.push("Failed to parse MVR file: file doesn't exists - " + path);
			return {};
		}

		m_TargetPath = std::experimental::filesystem::temp_directory_path().string() + "/";

		// We open the .mvr archive and parse the file tree and handle files
		// by their file extension. XML, gltf, 3ds.
		// ---------------------------------------------------------------------------
		auto filePath = std::string(path);
		auto zipFile = std::make_shared<ZipFile>(filePath);
		HandleZipFile(zipFile);

		return m_Layers;
	}

	void MVRParser::HandleZipFile(std::shared_ptr<ZipFile> zipFile)
	{
		for (const ZipInfo& info : zipFile->infolist())
		{
			const std::string& fileContent = zipFile->read(info);

			File file = { info.filename, fileContent };

			const FileType fileType = GetFileTypeFromExtension(GetFileExtension(info.filename));
			switch (fileType)
			{
			case FileType::GDTF:
				{
					auto parser = GDTF::GDTFParser();

					auto zipFileReader = std::istringstream(file.content);
					auto zipFile = std::make_shared<ZipFile>(zipFileReader);
					auto spec = parser.ParseCompressed(zipFile, file.name);

					m_GDTFSpecifications[file.name] = spec;
				}
				break;
			case FileType::XML:
				HandleXML(file);
				break;
			default:
				break; // Skip unknown file format.
			}
		}
	}

	bool MVRParser::HasGDTFSpecification(const std::string& name) const
	{
		return m_GDTFSpecifications.find(name) != m_GDTFSpecifications.end();
	}

	bool StringEndsWith(const std::string& input, const std::string& compare)
	{
		if(input.size() >= compare.size())
		{
			return (input.compare(input.length() - compare.length(), compare.length(), compare) == 0);
		}

		return false;
	}

	GDTF::GDTFSpecification MVRParser::GetGDTFSpecification(const std::string& name)
	{
		auto fullName = name;
		if(!StringEndsWith(fullName, ".gdtf"))
		{
			fullName += ".gdtf";
		}

		for(auto& s : m_GDTFSpecifications)
		{
			std::cout << s.first << std::endl;
		}

		if(!HasGDTFSpecification(fullName))
		{
			return {};
		}

		return m_GDTFSpecifications[fullName];
	}

	void MVRParser::HandleXML(const File& file)
	{
		if (file.name == m_SceneDescriptionFileName)
		{
			tinyxml2::XMLDocument doc;
			if (doc.Parse(file.content.c_str()) != tinyxml2::XML_SUCCESS)
			{
				m_Errors.push("Failed to parse XML file: " + file.name);
				return;
			}

			tinyxml2::XMLElement* root = doc.RootElement();

			// Verify version of MVR file, we support >1.5
			// -------------------------------
			auto major = root->FindAttribute("verMajor")->IntValue();
			auto minor = root->FindAttribute("verMinor")->IntValue();
			if (major != 1 || minor != 5)
			{
				// Warn version
				std::string warnMsg = "This extension is tested with mvr v1.5, this file version is"; 
				warnMsg += std::to_string(major) + "." + std::to_string(minor);

				m_Errors.push(warnMsg);
			}

			// Parse Scene in XML
			// -------------------------------
			LayerFactory layerFactory;

			std::vector<LayerSpecification> layers;
			auto scene = root->FirstChildElement("Scene");
			auto layersXml = scene->FirstChildElement("Layers");
			for (auto* layer = layersXml->FirstChildElement("Layer"); layer; layer = layer->NextSiblingElement())
			{
				auto layerSpec = layerFactory.CreateSpecificationFromXML(layer);
				if (layerSpec.fixtures.size() > 0)
				{
					layers.push_back(layerFactory.CreateSpecificationFromXML(layer));
				}
			}

			m_Layers = layers;
		}
	}

	bool MVRParser::FileExists(const std::string& path) const
	{
		const std::ifstream filePath(path);
		return filePath.good();
	}

	std::vector<std::string> MVRParser::StringSplit(const std::string& input, const char delimiter)
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

	std::string MVRParser::GetFileExtension(const std::string& fileName)
	{
		const auto& fileNameSplits = StringSplit(fileName, '.');
		const std::string fileExtension = fileNameSplits[fileNameSplits.size() - 1];
		return fileExtension;
	}

	FileType MVRParser::GetFileTypeFromExtension(const std::string& fileExtension)
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
}