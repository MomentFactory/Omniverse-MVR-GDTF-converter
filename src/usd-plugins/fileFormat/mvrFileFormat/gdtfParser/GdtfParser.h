#pragma once

#include "ModelSpecification.h"

#include <vector>
#include <stack>
#include <string>
#include <memory>



namespace GDTF {

	enum class FileType
	{
		MODEL,
        XML
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

		//std::vector<ModelSpecification> ParseGDTFParserFicle(const std::shared_ptr<ZipFile>& filepath);

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
		std::vector<ModelSpecification> m_Models;

		// File handling
		void HandleXML(const File& fileName);
		void HandleModel(const File& fileName);

		// Utilities
		bool FileExists(const std::string& path) const;

		std::string GetFileExtension(const std::string& path);
		FileType GetFileTypeFromExtension(const std::string& extension);

		std::vector<std::string> StringSplit(const std::string& input, const char delimiter);

		
	};

}