#pragma once

#include <vector>
#include <string>

namespace miniz_cpp {

	class zip_file;
}

namespace MVR {

	class Layer;
	class Fixture;

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

		std::vector<Layer> ParseMVRFile(const std::string& path);

	private:
		const std::string m_SceneDescriptionFileName = "GeneralSceneDescription.xml";

		// File handling
		void HandleZipFile(ZipFile& zipFile);
		void HandleXML(const File& fileName);
		void HandleModel(const File& fileName);
		void HandleGDTF(const File& fileName);

		// Utilities
		bool FileExists(const std::string& path) const;

		std::string GetFileExtension(const std::string& path);
		FileType GetFileTypeFromExtension(const std::string& extension);

		std::vector<std::string> StringSplit(const std::string& input, const char delimiter);
	};

}