import asyncio
import logging
import os
from typing import List

import omni.client
import omni.kit.asset_converter as converter


class GLTFImporter:
    async def convert(filepaths: List[str], output_dir: str, output_ext: str = "usd", timeout: int = 10) -> List[str]:
        _, files_in_output_dir = omni.client.list(output_dir)  # Ignoring omni.client.Result
        relative_paths_in_output_dir = [x.relative_path for x in files_in_output_dir]
        filenames: List[str] = [os.path.splitext(os.path.basename(x))[0] for x in filepaths]

        imported: List[str] = []
        for i, filename in enumerate(filenames):
            output_file = filename + "." + output_ext
            if output_file not in relative_paths_in_output_dir:
                input_path = filepaths[i]
                output_path = output_dir + output_file
                try:
                    success = await asyncio.wait_for(GLTFImporter._convert(input_path, output_path), timeout=timeout)
                except asyncio.TimeoutError:
                    success = False
                if success:
                    imported.append(output_file)
            else:
                imported.append(output_file)
        return imported

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
