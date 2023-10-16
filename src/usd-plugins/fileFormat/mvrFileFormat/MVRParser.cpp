#include "MVRParser.h"

#include "zip_file.hpp"

using ZipFile = miniz_cpp::zip_file;
using ZipInfo = miniz_cpp::zip_info;
using ZipInfoList = std::vector<ZipInfo>;

#include "tinyxml2.h"

#include <sstream>
#include <fstream>
#include <LayerFactory.h>

namespace MVR {

	std::vector<Layer> MVRParser::ParseMVRFile(const std::string& path)
	{
		if (!FileExists(path))
		{
			// TODO: Print exception, TF_ERROR?
			return;
		}

		// We open the .mvr archive and parse the file tree and handle files
		// by their file extension. XML, gltf, 3ds.
		// ---------------------------------------------------------------------------
		auto mvrArchive = ZipFile(path);
		HandleZipFile(mvrArchive);
	}

	void MVRParser::HandleZipFile(ZipFile& zipFile)
	{
		for (const ZipInfo& info : zipFile.infolist())
		{
			const std::string& fileContent = zipFile.read(info);

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
				// Report error.
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

				// TODO: Warn message.
			}

			// Parse Scene in XML
			// -------------------------------
			auto scene = root->FirstChildElement("Scene");
			auto layers = scene->FirstChildElement("Layers");
			for (auto* layer = root->FirstChildElement("Layer"); layer; layer = layer->NextSiblingElement()) 
			{
				// Create layer
				LayerFactory layerFactory;

			}
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