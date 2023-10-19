#include "LayerFactory.h"
#include "MVRParser.h"

#include "tinyxml2.h"

#include "zip_file.hpp"
using ZipFile = miniz_cpp::zip_file;
using ZipInfo = miniz_cpp::zip_info;
using ZipInfoList = std::vector<ZipInfo>;

#include <sstream>
#include <fstream>


namespace MVR {

	std::vector<LayerSpecification> MVRParser::ParseMVRFile(const std::string& path)
	{
		if (!FileExists(path))
		{
			m_Errors.push("Failed to parse MVR file: file doesn't exists - " + path);
			return {};
		}

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
				HandleGDTF(file);
				break;
			case FileType::MODEL:
				HandleModel(file);
				break;
			case FileType::XML:
				HandleXML(file);
				break;
			default:
				break; // Skip unknown file format.
			}
		}
	}

	void MVRParser::HandleGDTF(const File& file)
	{
		std::cout << "Found GDTF archive." << std::endl;
		// TODO: Read zip content and unzip.
		// 
		//HandleZipFile(ZipFile(std::istringstream(fileContent)));
	}

	void MVRParser::HandleModel(const File& file)
	{
		std::cout << "Found 3D model: " << file.name << std::endl;
	}

	void MVRParser::HandleXML(const File& file)
	{
		if (file.name == m_SceneDescriptionFileName)
		{
			tinyxml2::XMLDocument doc;
			if (doc.Parse(file.content.c_str()) != tinyxml2::XML_SUCCESS)
			{
				// TODO: Warn message.
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
				layers.push_back(layerFactory.CreateSpecificationFromXML(layer));
			}

			m_Layers = std::move(layers);
		}

		std::cout << "Found XML file: " << file.name << std::endl;
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
		else if (fileExtension == "gdtf")
		{
			return FileType::GDTF;
		}

		return FileType::UNKNOWN;
	}
}