import logging

from .filepathUtility import Filepath


class MVRImporter:
    async def convert(file: Filepath, mvr_output_dir: str, output_ext: str = ".usd") -> bool:
        logger = logging.getLogger(__name__)
        logger.error(f"Converting file at {file.fullpath} to {mvr_output_dir} with ext {output_ext}")

        return True
