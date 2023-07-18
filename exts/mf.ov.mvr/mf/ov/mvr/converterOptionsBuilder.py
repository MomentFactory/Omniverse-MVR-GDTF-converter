from omni.kit.menu import utils
from omni.kit.tool.asset_importer.file_picker import FilePicker
from omni.kit.tool.asset_importer.filebrowser import FileBrowserMode, FileBrowserSelectionType
import omni.kit.window.content_browser as content

from .converterOptions import ConverterOptions

class ConverterOptionsBuilder:
    def __init__(self):
        self._file_picker = None
        self._export_content = ConverterOptions()
        self._folder_button = None
        self._refresh_default_folder = False
        self._default_folder = None
        self._clear()

    def destroy(self):
        self._clear()
        if self._file_picker:
            self._file_picker.destroy()

    def _clear(self):
        self._built = False
        self._export_folder_field = None
        if self._folder_button:
            self._folder_button.set_clicked_fn(None)
            self._folder_button = None

    def set_default_target_folder(self, folder: str):
        self._default_folder = folder
        self._refresh_default_folder = True

    def _select_picked_folder_callback(self, paths):
        if paths:
            self._export_folder_field.model.set_value(paths[0])

    def _cancel_picked_folder_callback(self):
        pass

    def _show_file_picker(self):
        if not self._file_picker:
            mode = FileBrowserMode.OPEN
            file_type = FileBrowserSelectionType.DIRECTORY_ONLY
            filters = [(".*", "All Files (*.*)")]
            self._file_picker = FilePicker("Select Folder", mode=mode, file_type=file_type, filter_options=filters)
            self._file_picker.set_file_selected_fn(self._select_picked_folder_callback)
            self._file_picker.set_cancel_fn(self._cancel_picked_folder_callback)

        folder = self._export_folder_field.model.get_value_as_string()
        if utils.is_folder(folder):
            self._file_picker.show(folder)
        else:
            self._file_picker.show(self._get_current_dir_in_content_window())

    def _get_current_dir_in_content_window(self):
        content_window = content.get_content_window()
        return content_window.get_current_directory()

    def get_import_options(self):
        return ConverterOptions()
