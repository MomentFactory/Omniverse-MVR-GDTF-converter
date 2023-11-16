#include "GdtfParser.h"

#include <iostream>

namespace GDTF {

// std::vector<ModelSpecification> GDTFParser::ParseGDTFParserFile(const std::shared_ptr<ZipFile>& zipFile)
// {
// 	for (const ZipInfo& info : zipFile->infolist())
// 	{
// 		const std::string& fileContent = zipFile->read(info);

// 		File file = { info.filename, fileContent };

// 		const FileType fileType = GetFileTypeFromExtension(GetFileExtension(info.filename));
// 		switch (fileType)
// 		{
// 		case FileType::MODEL:
// 			HandleModel(file);
// 			break;
// 		case FileType::XML:
// 			HandleXML(file);
// 			break;
// 		default:
// 			break; // Skip unknown file format.
// 		}
// 	}

// 	std::cout << "DONE" << std::endl;

// 	return m_Models;
// }

    void GDTFParser::HandleModel(const File& fileName)
    {

    }
}