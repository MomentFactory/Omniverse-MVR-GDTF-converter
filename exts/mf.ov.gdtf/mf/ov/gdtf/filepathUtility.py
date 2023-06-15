import os

class Filepath:
    def __init__(self, filepath: str):
        self.directory = os.path.dirname(filepath) + "/"
        self.basename = os.path.basename(filepath)
        self.filename, self.ext = os.path.splitext(self.basename)

    def is_nucleus_path(self) -> bool:
        return self.directory[:12] == "omniverse://"
