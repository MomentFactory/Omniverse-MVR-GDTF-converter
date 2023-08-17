import omni.ext
import omni.kit.tool.asset_importer as ai

from .converterDelegate import ConverterDelegate

class MfOvMvrExtension(omni.ext.IExt):
    def on_startup(self, _):
        self._delegate_mvr = ConverterDelegate(
            "MVR Converter",
            ["(.*\\.mvr$)"],
            ["MVR Files (*.mvr)"]
        )
        ai.register_importer(self._delegate_mvr)

    def on_shutdown(self):
        ai.remove_importer(self._delegate_mvr)
        self._delegate_mvr.destroy()
        self._delegate_mvr = None
