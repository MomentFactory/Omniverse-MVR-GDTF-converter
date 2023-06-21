import omni.ext
import omni.kit.tool.asset_importer as ai

from .converterDelegate import ConverterDelegate


class MfOvGdtfExtension(omni.ext.IExt):
    def on_startup(self, _):
        self._delegate_gdtf = ConverterDelegate(
            "GDTF Converter",
            ["(.*\\.gdtf$)"],
            ["GDTF Files (*.gdtf)"]
        )
        ai.register_importer(self._delegate_gdtf)

    def on_shutdown(self):
        ai.remove_mporter(self._delegate_gdtf)
        self._delegate_gdtf.destroy()
        self._delegate_gdtf = None


"""
def _import_gdtf(filepath: str):
    file: Filepath = Filepath(filepath)

    if file.is_nucleus_path():
        # TODO: Cannot Unzip directly from omniverse, might have to download the file locally as tmp
        logger = logging.getLogger(__name__)
        logger.error("Cannot import directly from Omniverse")
        return

    stage_dir = USDTools.get_stage_directory()
    if stage_dir == "":
        output_dir = file.directory
        logger = logging.getLogger(__name__)
        logger.warn(f"Current stage is anonymous, output files in filesystem at {output_dir} instead of in nucleus")
    else:
        output_dir = USDTools.get_stage_directory()

    output_dir += "gdtf/"
    asyncio.ensure_future(GDTFImporter.convert(file, output_dir))
"""
