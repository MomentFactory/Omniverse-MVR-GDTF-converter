import logging
import shutil
from urllib.parse import unquote

import omni.kit.window.content_browser

from .filepathUtility import Filepath
from .gdtfImporter import GDTFImporter
from .gltfImporter import GLTFImporter


class ConverterHelper:
    def _create_import_task(self, absolute_path, export_folder, _):
        absolute_path_unquoted = unquote(absolute_path)
        if absolute_path_unquoted.startswith("file:/"):
            path = absolute_path_unquoted[6:]
        else:
            path = absolute_path_unquoted

        current_nucleus_dir = omni.kit.window.content_browser.get_content_window().get_current_directory()

        file: Filepath = Filepath(path)
        output_dir = current_nucleus_dir if export_folder is None else export_folder
        if export_folder is not None and export_folder != "":
            output_dir = export_folder

        # Cannot Unzip directly from Nucleus, must download file beforehand
        if file.is_nucleus_path():
            tmp_path = GLTFImporter.TMP_ARCHIVE_EXTRACT_DIR + file.basename
            result = omni.client.copy(file.fullpath, tmp_path, omni.client.CopyBehavior.OVERWRITE)
            if result == omni.client.Result.OK:
                file = Filepath(tmp_path)
            else:
                logger = logging.getLogger(__name__)
                logger.error(f"Could not import {file.fullpath} directly from Omniverse, try downloading the file instead")
                return

        url: str = GDTFImporter.convert(file, output_dir)
        return url

    async def create_import_task(self, absolute_paths, export_folder, hoops_context):
        converted_assets = {}
        for i in range(len(absolute_paths)):
            converted_assets[absolute_paths[i]] = self._create_import_task(absolute_paths[i], export_folder,
                                                                           hoops_context)
        shutil.rmtree(GLTFImporter.TMP_ARCHIVE_EXTRACT_DIR)
        return converted_assets
