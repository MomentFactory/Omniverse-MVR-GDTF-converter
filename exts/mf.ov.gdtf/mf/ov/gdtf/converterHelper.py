class ConverterHelper:
    def _create_import_task(self, absolute_path, filename_with_ext, export_folder, _):
        print("Create import task")

    async def create_import_task(self, absolute_paths, relative_paths, export_folder, hoops_context):
        converted_assets = {}
        for i in range(len(absolute_paths)):
            converted_assets[absolute_paths[i]] = self._create_import_task(absolute_paths[i], relative_paths[i],
                                                                           export_folder, hoops_context)
        return converted_assets
