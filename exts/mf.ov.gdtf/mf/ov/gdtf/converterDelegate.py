import os

import omni.kit.tool.asset_importer as ai

from .converterOptionsBuilder import ConverterOptionsBuilder
from .converterHelper import ConverterHelper


class ConverterDelegate(ai.AbstractImporterDelegate):
    def __init__(self, name, filters, descriptions):
        super().__init__()
        self._hoops_options_builder = ConverterOptionsBuilder()
        self._hoops_converter = ConverterHelper()
        self._name = name
        self._filters = filters
        self._descriptions = descriptions

    def destroy(self):
        if self._hoops_converter:
            # self._hoops_converter.destroy()
            self._hoops_converter = None

        if self._hoops_options_builder:
            self._hoops_options_builder.destroy()
            self._hoops_options_builder = None

    @property
    def name(self):
        return self._name

    @property
    def filter_regexes(self):
        return self._filters

    @property
    def filter_descriptions(self):
        return self._descriptions

    def build_options(self, paths):
        pass
        # TODO enable this after the filepicker bugfix: OM-47383
        # self._hoops_options_builder.build_pane(paths)

    async def convert_assets(self, paths):
        context = self._hoops_options_builder.get_import_options()
        hoops_context = context.cad_converter_context
        absolute_paths = []
        relative_paths = []

        for file_path in paths:
            if self.is_supported_format(file_path):
                absolute_paths.append(file_path)
                filename = os.path.basename(file_path)
                relative_paths.append(filename)

        converted_assets = await self._hoops_converter.create_import_task(
            absolute_paths, context.export_folder, hoops_context
        )

        return converted_assets
