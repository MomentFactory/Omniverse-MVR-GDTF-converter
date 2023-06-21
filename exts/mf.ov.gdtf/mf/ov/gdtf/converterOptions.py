from .converterContext import ConverterContext


class ConverterOptions:
    def __init__(self):
        self.cad_converter_context = ConverterContext()
        self.export_folder: str = None
