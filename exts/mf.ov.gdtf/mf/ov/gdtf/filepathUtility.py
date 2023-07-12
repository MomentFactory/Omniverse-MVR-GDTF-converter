import os


class Filepath:
    def __init__(self, filepath: str):
        self._is_none = filepath == ""

        self.fullpath = filepath
        self.directory = os.path.dirname(filepath) + "/"
        self.basename = os.path.basename(filepath)
        self.filename, self.ext = os.path.splitext(self.basename)

    def is_nucleus_path(self) -> bool:
        # TODO: Replace with omni utility method
        return self.directory[:12] == "omniverse://"

    def get_relative_from(self, other) -> str:
        if self._is_none:
            return other.fullpath
        else:
            return "./" + other.fullpath[len(self.directory):]
