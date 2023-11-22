#include "LayerFactory.h"
#include "MVRParser.h"
#include "../gdtfParser/ModelSpecification.h"

#include "zip_file2.hpp"

using ZipInfo = miniz_cpp2::zip_info;
using ZipInfoList = std::vector<ZipInfo>;

#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "tinyxml2.h"

#include <sstream>
#include <fstream>
#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

namespace MVR {

	std::vector<LayerSpecification> MVRParser::ParseMVRFile(const std::string& path)
	{
		if (!FileExists(path))
		{
			m_Errors.push("Failed to parse MVR file: file doesn't exists - " + path);
			return {};
		}

		m_TargetPath = path + "/../";

		// We open the .mvr archive and parse the file tree and handle files
		// by their file extension. XML, gltf, 3ds.
		// ---------------------------------------------------------------------------
		auto filePath = std::string(path);
		auto zipFile = std::make_shared<ZipFile>(filePath);
		HandleZipFile(zipFile);

		std::cout << "DONE" << std::endl;

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
				//HandleModel(file);
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
		auto ss = std::istringstream(file.content);
		auto zipFile = std::make_shared<ZipFile>(ss);

		std::map<std::string, std::vector<File>> fixtures;
		std::vector<File> assetFiles;

		GDTF::GDTFSpecification spec;
		
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
						return;
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

		// TODO: Read zip content and unzip.
		//HandleZipFile(ZipFile(std::istringstream(fileContent)));
	}

	void MVRParser::HandleModel(const File& file, const std::string& fixtureName)
	{
		std::cout << "Found 3D model: " << file.name << std::endl;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFileFromMemory(file.content.data(), file.content.size() , aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices,"EXTENTION");

		Assimp::Exporter exporter;

		std::experimental::filesystem::path targetPath = m_TargetPath;

		std::experimental::filesystem::path destination = targetPath.parent_path().append(fixtureName);
		std::experimental::filesystem::create_directory(destination);

		std::experimental::filesystem::path convertedFileName = destination.append((std::experimental::filesystem::path(file.name).stem().concat(".gltf").c_str()));

		exporter.Export(scene, "gltf2", convertedFileName.string());

		m_GDTFAssets[fixtureName][file.name] = convertedFileName.string();
		std::cout << "inserted:" << fixtureName << " input:" << file.name << " output:" << convertedFileName << std::endl;
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
				layers.push_back(layerFactory.CreateSpecificationFromXML(layer));
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