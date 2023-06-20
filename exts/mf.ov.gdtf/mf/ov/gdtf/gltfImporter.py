import asyncio
import logging
from typing import List

import omni.client
import omni.kit.asset_converter as converter

from .filepathUtility import Filepath
from .gdtfUtil import Model


class GLTFImporter:
    async def convert(models: List[Model], output_dir: str, output_ext: str = "usd", timeout: int = 10) -> List[Model]:
        _, files_in_output_dir = omni.client.list(output_dir)  # Ignoring omni.client.Result
        relative_paths_in_output_dir = [x.relative_path for x in files_in_output_dir]

        converted_models: List[Model] = []

        for model in models:
            file: Filepath = Filepath(model.get_tmpdir_filepath())
            filename = file.filename
            output_file = filename + "." + output_ext
            if output_file not in relative_paths_in_output_dir:
                input_path = file.fullpath
                output_path = output_dir + output_file
                try:
                    success = await asyncio.wait_for(GLTFImporter._convert(input_path, output_path), timeout=timeout)
                except asyncio.TimeoutError:
                    success = False
                if success:
                    model.set_converted_filepath(output_file)
                    converted_models.append(model)
            else:
                model.set_converted_filepath(output_file)
                converted_models.append(model)
        return converted_models

    async def _convert(input_path: str, output_path: str) -> bool:
        converter_context = converter.AssetConverterContext()
        # https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/Specification.adoc#coordinate-system-and-units
        converter_context.use_meter_as_world_unit = True  # GLTF spec: The units for all linear distances are meters.
        task_manager = converter.get_instance()
        task = task_manager.create_converter_task(input_path, output_path, asset_converter_context=converter_context)

        success = await task.wait_until_finished()
        if not success:
            status_code = task.get_status()
            status_error_string = task.get_error_message()
            logger = logging.getLogger(__name__)
            logger.error(f"Failure to convert file {input_path}: ({status_code}) {status_error_string}")

        return success
