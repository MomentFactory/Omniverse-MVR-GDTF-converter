#pragma once

#include "Fixture.h"



#include <vector>
#include <stack>
#include <string>
#include <map>
#include <memory>

namespace miniz_cpp2
{
	class zip_file;
}
using ZipFile = miniz_cpp2::zip_file;

namespace MVR {

	enum class FileType
	{
		GDTF,
		MODEL,
		XML,
		UNKNOWN
	};

	struct File
	{
		std::string name;
		std::string content;
	};

	class MVRParser
	{
	public:
		MVRParser() = default;
		~MVRParser() = default;

		std::vector<LayerSpecification> ParseMVRFile(const std::string& path);

		inline const bool HasError() const { return m_Errors.size() > 1; }
		const std::string PopError() 
		{ 
			if (!HasError())
			{
				throw std::exception("Error stack is empty.");
			}

			auto msg = m_Errors.top();
			m_Errors.pop();
			return msg;
		}

	private:
		const std::string m_SceneDescriptionFileName = "GeneralSceneDescription.xml";
		std::string m_TargetPath;

		std::stack<std::string> m_Errors;
		std::vector<LayerSpecification> m_Layers;

		// fixtureName<part, assetFile>
		std::map<std::string, std::map<std::string, std::string>> m_GDTFAssets;

		// File handling
		void HandleZipFile(std::shared_ptr<ZipFile> zipFile);
		void HandleXML(const File& fileName);
		void HandleModel(const File& fileName, const std::string& fixtureName);
		void HandleGDTF(const File& fileName);

		// Utilities
		bool FileExists(const std::string& path) const;

		std::string GetFileExtension(const std::string& path);
		FileType GetFileTypeFromExtension(const std::string& extension);

		std::vector<std::string> StringSplit(const std::string& input, const char delimiter);

		
	};

}