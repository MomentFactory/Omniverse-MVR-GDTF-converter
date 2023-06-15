import logging
import xml.etree.ElementTree as ET
from zipfile import ZipFile


class GDTFImporter:
    def convert(filepath: str, output_dir: str, output_ext: str = "usd") -> bool:
        try:
            with ZipFile(filepath, 'r') as archive:
                data = archive.read("description.xml")
                root = ET.fromstring(data)
                GDTFImporter._convert_gltf(root)
        except Exception as e:
            logger = logging.getLogger(__name__)
            logger.error(f"Failed to parse gdtf file at {filepath}. Make sure it is not corrupt. {e}")
            return False

        return True

    def _convert_gltf(root: ET):
        print(root)
