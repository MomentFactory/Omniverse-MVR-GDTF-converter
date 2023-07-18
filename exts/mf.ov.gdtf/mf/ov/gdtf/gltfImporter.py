import logging
from typing import List

import omni.client

from .filepathUtility import Filepath
from .gdtfUtil import Model


class GLTFImporter:
    def convert(models: List[Model], output_dir: str) -> List[Model]:
        _, files_in_output_dir = omni.client.list(output_dir)  # Ignoring omni.client.Result
        relative_paths_in_output_dir = [x.relative_path for x in files_in_output_dir]

        converted_models: List[Model] = []

        for model in models:
            file: Filepath = model.get_tmpdir_filepath()
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
