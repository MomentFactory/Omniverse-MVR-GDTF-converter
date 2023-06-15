import logging
import xml.etree.ElementTree as ET
from zipfile import ZipFile


class GDTFImporter:
    def convert(filename: str, input_dir: str, output_dir: str, output_ext: str = "usd") -> bool:
        if input_dir[:12] == "omniverse://":
            # TODO: Could download for extraction in tmp
            logger = logging.getLogger(__name__)
            logger.error("Cannot import directly from Omniverse")
            return False

        file_path = input_dir + filename
        try:
            with ZipFile(file_path, 'r') as archive:
                data = archive.read("description.xml")
                root = ET.fromstring(data)
                GDTFImporter._convert_gltf(root)
        except Exception as e:
            logger = logging.getLogger(__name__)
            logger.error(f"Failed to parse gdtf file at {file_path}. Make sure it is not corrupt. {e}")
            return False

        return True

    def _convert_gltf(root: ET):
        print(root)
