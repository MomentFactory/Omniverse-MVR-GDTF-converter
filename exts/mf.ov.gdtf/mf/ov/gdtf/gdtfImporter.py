import logging
import tempfile
from typing import List
import xml.etree.ElementTree as ET
from zipfile import ZipFile

from .filepathUtility import Filepath
from .gdtfUtil import Model
from .gltfImporter import GLTFImporter
from .USDTools import USDTools


class GDTFImporter:
    TMP_ARCHIVE_EXTRACT_DIR = f"{tempfile.gettempdir()}/MF.OV.GDTF/"

    async def convert(file: Filepath, output_dir: str, output_ext: str = ".usd") -> bool:
        try:
            with ZipFile(file.fullpath, 'r') as archive:
                data = archive.read("description.xml")
                root = ET.fromstring(data)
                await GDTFImporter._find_and_convert_gltf(root, archive, output_dir)
                GDTFImporter._convert_gdtf_usd(output_dir, file.filename, output_ext)

        except Exception as e:
            logger = logging.getLogger(__name__)
            logger.error(f"Failed to parse gdtf file at {file.fullpath}. Make sure it is not corrupt. {e}")
            return False

        return True

    #region convert gltf
    async def _find_and_convert_gltf(root: ET.Element, archive: ZipFile, output_dir: str):
        models: List[Model] = GDTFImporter._get_model_nodes(root)
        gltf_filenames: List[str] = GDTFImporter._get_valid_gltf_filenames(models)
        gltf_extract_path: List[str] = GDTFImporter._extract_gltf_tmp(gltf_filenames, archive)
        converted_gltf_files: List[str] = await GDTFImporter._convert_gltf(gltf_extract_path, output_dir)
        print(converted_gltf_files)

    def _get_model_nodes(root: ET.Element) -> List[Model]:
        node_fixture: ET.Element = root.find("FixtureType")
        node_models: ET.Element = node_fixture.find("Models")
        nodes_model = node_models.findall("Model")
        models: List[Model] = []
        for node_model in nodes_model:
            models.append(Model(node_model))
        return models

    def _get_valid_gltf_filenames(models: List[Model]) -> List[str]:
        gltf_filenames: List[str] = []
        for model in models:
            if model.has_file():
                gltf_filenames.append(model.get_file())
            else:
                logger = logging.getLogger(__name__)
                logger.warn(f"File attribute empty for model node {model.get_name()}, skipping.")
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
    #endregion

    #region make gdtf
    def _convert_gdtf_usd(output_dir: str, filename: str, ext: str):
        url: str = output_dir + filename + ext
        GDTFImporter._get_or_create_gdtf_usd(url)

    def _get_or_create_gdtf_usd(url: str):
        _ = USDTools.get_or_create_stage(url)

    def _add_gltf_payload_to_gdtf():
        pass
    #endregion
