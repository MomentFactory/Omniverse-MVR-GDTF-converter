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
        ai.remove_importer(self._delegate_gdtf)
        self._delegate_gdtf.destroy()
        self._delegate_gdtf = None
