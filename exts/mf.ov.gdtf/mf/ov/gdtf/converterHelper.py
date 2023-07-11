import asyncio
import logging
from typing import Tuple

import omni.kit.window.content_browser

from .filepathUtility import Filepath
from .gdtfImporter import GDTFImporter


class ConverterHelper:
    def split_abspath(self, absolute_path: str) -> Tuple[str, str]:
        split_index = absolute_path.rfind(":")
        left_partial = absolute_path[:split_index]
        right_partial = absolute_path[split_index:]
        source_index = left_partial.rfind("/")
        right_source = left_partial[source_index + 1:]

        left = left_partial[:source_index] + "/"
        right = right_source + right_partial

        return left, right

    def _create_import_task(self, absolute_path, export_folder, _):
        current_nucleus_dir = omni.kit.window.content_browser.get_content_window().get_current_directory()

        file: Filepath = Filepath(absolute_path)
        output_dir = current_nucleus_dir if export_folder is None else export_folder
        if export_folder is not None and export_folder != "":
            output_dir = export_folder

        if file.is_nucleus_path():
            # TODO: Cannot Unzip directly from omniverse, might have to download the file locally as tmp
            logger = logging.getLogger(__name__)
            logger.error("Cannot import directly from Omniverse")
            return

        url: str = asyncio.ensure_future(GDTFImporter.convert(file, output_dir))
        return url

    async def create_import_task(self, absolute_paths, export_folder, hoops_context):
        converted_assets = {}
        for i in range(len(absolute_paths)):
            converted_assets[absolute_paths[i]] = self._create_import_task(absolute_paths[i], export_folder,
                                                                           hoops_context)
        return converted_assets
