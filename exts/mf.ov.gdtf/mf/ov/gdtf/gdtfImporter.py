import logging
import tempfile
from typing import List
import xml.etree.ElementTree as ET
from zipfile import ZipFile

from .filepathUtility import Filepath
from .gltfImporter import GLTFImporter


class GDTFImporter:
    TMP_ARCHIVE_EXTRACT_DIR = f"{tempfile.gettempdir()}/MF.OV.GDTF/"

    async def convert(file: Filepath, output_dir: str, output_ext: str = ".usd") -> bool:
        try:
            with ZipFile(file.fullpath, 'r') as archive:
                data = archive.read("description.xml")
                root = ET.fromstring(data)
                await GDTFImporter._find_and_convert_gltf(root, archive, output_dir)
                GDTFImporter._create_gdtf_usd(output_dir, file.filename, output_ext)
        except Exception as e:
            logger = logging.getLogger(__name__)
            logger.error(f"Failed to parse gdtf file at {file.fullpath}. Make sure it is not corrupt. {e}")
            return False

        return True

    async def _find_and_convert_gltf(root: ET.Element, archive: ZipFile, output_dir: str):
        nodes_model: List[ET.Element] = GDTFImporter._get_model_nodes(root)
        gltf_filenames: List[str] = GDTFImporter._get_valid_gltf_filenames(nodes_model)
        gltf_extract_path: List[str] = GDTFImporter._extract_gltf_tmp(gltf_filenames, archive)
        converted_gltf_files: List[str] = await GDTFImporter._convert_gltf(gltf_extract_path, output_dir)
        print(converted_gltf_files)

    def _get_model_nodes(root: ET.Element) -> List[ET.Element]:
        node_fixture: ET.Element = root.find("FixtureType")
        node_models: ET.Element = node_fixture.find("Models")
        return node_models.findall("Model")

    def _get_valid_gltf_filenames(nodes_model: List[ET.Element]) -> List[str]:
        gltf_filenames: List[str] = []
        for node_model in nodes_model:
            if "File" in node_model.attrib:
                if node_model.attrib["File"] != "":
                    gltf_filenames.append(node_model.attrib["File"])
                else:
                    logger = logging.getLogger(__name__)
                    logger.warn(f"File attribute empty for model node {node_model.attrib['Name']}, skipping.")
            else:
                logger = logging.getLogger(__name__)
                logger.warn(f"No File attribute for model node {node_model.attrib['Name']}, skipping.")
        return gltf_filenames

    def _extract_gltf_tmp(filenames: List[str], gdtf_archive: ZipFile) -> List[str]:
        namelist = gdtf_archive.namelist()
        extracted_filepaths: List[str] = []

        for filename in filenames:
            filepath_glb = f"models/gltf/{filename}.glb"
            filepath_gltf = f"models/gltf/{filename}.gltf"
            filepath_3ds = f"models/3ds/{filename}.3ds"

            if filepath_glb in namelist:
                tmp_export_path = gdtf_archive.extract(filepath_glb, GDTFImporter.TMP_ARCHIVE_EXTRACT_DIR)
                extracted_filepaths.append(tmp_export_path)
            elif filepath_gltf in namelist:
                tmp_export_path = gdtf_archive.extract(filepath_gltf, GDTFImporter.TMP_ARCHIVE_EXTRACT_DIR)
                extracted_filepaths.append(tmp_export_path)
            elif filepath_3ds:
                logger = logging.getLogger(__name__)
                logger.warn(f"Found unsupported 3ds file for {filename}, skipping.")
            else:
                logger = logging.getLogger(__name__)
                logger.warn(f"No file found for {filename}, skipping.")

        return extracted_filepaths

    async def _convert_gltf(filepaths: List[str], gdtf_output_dir):
        gltf_output_dir = gdtf_output_dir + "gltf/"
        return await GLTFImporter.convert(filepaths, gltf_output_dir)
