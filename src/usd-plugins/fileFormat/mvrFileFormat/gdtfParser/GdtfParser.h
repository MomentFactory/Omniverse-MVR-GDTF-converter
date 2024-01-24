#pragma once

#include "ModelSpecification.h"

#include <vector>
#include <stack>
#include <string>
#include <memory>
#include <map>

namespace tinyxml2
{
	class XMLElement;
}

namespace miniz_cpp2
{
	class zip_file;
}

using ZipFile = miniz_cpp2::zip_file;

namespace GDTF {

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

	class GDTFParser
	{
	public:
		GDTFParser() = default;
		~GDTFParser() = default;

		GDTF::GDTFSpecification ParseGDTFFile(const std::string& path);
		GDTF::GDTFSpecification ParseCompressed(std::shared_ptr<ZipFile> file, const std::string& zipFileName);

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

		std::stack<std::string> m_Errors;

		// File handling
		void HandleXML(const File& fileName);
		void HandleModel(const File& file, const std::string& fixtureName);
		GDTF::GDTFSpecification HandleGDTF(std::shared_ptr<ZipFile>& file);

		void HandleGDTFRecursive(tinyxml2::XMLElement* element, GDTF::GDTFSpecification& spec, int depth);

		// Utilities
		bool FileExists(const std::string& path) const;

		std::string GetFileExtension(const std::string& path);
		FileType GetFileTypeFromExtension(const std::string& extension);

		std::vector<std::string> StringSplit(const std::string& input, const char delimiter);
		std::map<std::string, std::map<std::string, std::string>> m_GDTFAssets;

		std::string m_TargetPath;
		std::string m_SpecName;
	};
}