import logging
import omni.client
import os
import subprocess
import sys
import tempfile
from typing import List
import xml.etree.ElementTree as ET
from zipfile import ZipFile

from .filepathUtility import Filepath
from .gdtfUtil import Model


class GLTFImporter:
    TMP_ARCHIVE_EXTRACT_DIR = f"{tempfile.gettempdir()}/MF.OV.GDTF/"

    def convert(root: ET.Element, archive: ZipFile, output_dir: str) -> List[Model]:
        models: List[Model] = GLTFImporter._get_model_nodes(root)
        models_filtered: List[Model] = GLTFImporter._filter_models(models)
        GLTFImporter._extract_gltf_to_tmp(models_filtered, archive)
        GLTFImporter._convert_gltf(models_filtered, output_dir)
        return models

    def _get_model_nodes(root: ET.Element) -> List[Model]:
        node_fixture: ET.Element = root.find("FixtureType")
        node_models: ET.Element = node_fixture.find("Models")
        nodes_model = node_models.findall("Model")
        models: List[Model] = []
        for node_model in nodes_model:
            models.append(Model(node_model))
        return models

    def _filter_models(models: List[Model]) -> List[Model]:
        filters: List[str] = ['pigtail', 'beam']
        filtered_models: List[Model] = []
        for model in models:
            if model.has_file():
                filtered_models.append(model)
            elif model.get_name().lower() not in filters:
                logger = logging.getLogger(__name__)
                logger.info(f"File attribute empty for model node {model.get_name()}, skipping.")
        return filtered_models

    def _extract_gltf_to_tmp(models: List[Model], gdtf_archive: ZipFile):
        namelist = gdtf_archive.namelist()
        to_remove: List[Model] = []

        for model in models:
            filename = model.get_file()
            filepath_glb = f"models/gltf/{filename}.glb"
            filepath_gltf = f"models/gltf/{filename}.gltf"
            filepath_3ds = f"models/3ds/{filename}.3ds"

            if filepath_glb in namelist:
                tmp_export_path = gdtf_archive.extract(filepath_glb, GLTFImporter.TMP_ARCHIVE_EXTRACT_DIR)
                model.set_tmpdir_filepath(Filepath(tmp_export_path))
            elif filepath_gltf in namelist:
                tmp_export_path = gdtf_archive.extract(filepath_gltf, GLTFImporter.TMP_ARCHIVE_EXTRACT_DIR)
                for filepath in namelist:  # Also import .bin, textures, etc.
                    if filepath.startswith(f"models/gltf/{filename}") and filepath != filepath_gltf:
                        gdtf_archive.extract(filepath, GLTFImporter.TMP_ARCHIVE_EXTRACT_DIR)
                model.set_tmpdir_filepath(Filepath(tmp_export_path))
            elif filepath_3ds in namelist:
                tmp_export_path = gdtf_archive.extract(filepath_3ds, GLTFImporter.TMP_ARCHIVE_EXTRACT_DIR)
                temp_export_path_gltf = tmp_export_path[:-4] + ".gltf"
                GLTFImporter._convert_3ds_to_gltf(tmp_export_path, temp_export_path_gltf)
                model.set_tmpdir_filepath(Filepath(temp_export_path_gltf))
                model.set_converted_from_3ds()
                os.remove(tmp_export_path)
            else:
                logger = logging.getLogger(__name__)
                logger.warn(f"No file found for {filename}, skipping.")
                to_remove.append(model)

        for model in to_remove:
            models.remove(model)

    def _convert_3ds_to_gltf(input, output):
        path = __file__
        my_env = os.environ.copy()
        my_env["PATH"] = path + '\\..\\' + os.pathsep + my_env['PATH']
        scriptPath = path + "\\..\\3dsConverterScript.py"
        try:
            result = subprocess.run(["py", "-3.10", scriptPath, input, output], capture_output=True, env=my_env)
            if result.returncode != 0:
                logger = logging.getLogger(__name__)
                logger.error(f"Failed to convert 3ds file to gltf: {input}\nerror (Requires python 3.10): {result.stderr.decode('utf-8')}\nerror message: {result.stdout.decode('utf-8')}")
        except Exception as e:
            logger = logging.getLogger(__name__)
            logger.error(f"Failed to convert 3ds file to gltf: {input}\n{e}")

    def _convert_gltf(models: List[Model], gdtf_output_dir):
        output_dir = gdtf_output_dir + "gltf/"
        _, files_in_output_dir = omni.client.list(output_dir)  # Ignoring omni.client.Result
        relative_paths_in_output_dir = [x.relative_path for x in files_in_output_dir]

        converted_models: List[Model] = []

        for model in models:
            file: Filepath = model.get_tmpdir_filepath()
            if model.get_converted_from_3ds():
                bin_file = file.basename[:-5] + ".bin"
                bin_path = output_dir + bin_file
                if bin_file not in relative_paths_in_output_dir:
                    input_path = file.fullpath[:-5] + ".bin"
                    result = result = omni.client.copy(input_path, bin_path, omni.client.CopyBehavior.OVERWRITE)

            output_file = file.basename
            output_path = output_dir + output_file
            if output_file not in relative_paths_in_output_dir:
                input_path = file.fullpath
                result = omni.client.copy(input_path, output_path, omni.client.CopyBehavior.OVERWRITE)
                if result == omni.client.Result.OK:
                    model.set_converted_filepath(Filepath(output_path))
                    converted_models.append(model)
                else:
                    logger = logging.getLogger(__name__)
                    logger.error(f"Failure to convert file {input_path}: {result}")
            else:
                model.set_converted_filepath(Filepath(output_path))
                converted_models.append(model)
        return converted_models
