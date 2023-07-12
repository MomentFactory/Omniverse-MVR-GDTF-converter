# import numpy as np
import xml.etree.ElementTree as ET

from .filepathUtility import Filepath
from .USDTools import USDTools


def get_attrib_if_exists(node: ET.Element, attr: str):
    return node.attrib[attr] if attr in node.attrib else None


class Model:
    def __init__(self, node: ET.Element):
        self._name = node.attrib["Name"]
        self._name_usd = USDTools.make_name_valid(self._name)
        self._file = get_attrib_if_exists(node, "File")
        self._primitive_type = node.attrib["PrimitiveType"]
        self._height = node.attrib["Height"]
        self._length = node.attrib["Length"]
        self._width = node.attrib["Width"]

    def get_name(self) -> str:
        return self._name

    def get_name_usd(self) -> str:
        return self._name_usd

    def has_file(self) -> bool:
        return self._file is not None and self._file != ""

    def get_file(self) -> str:
        return self._file

    def set_tmpdir_filepath(self, path: Filepath):
        self._tmpdir_filepath = path

    def get_tmpdir_filepath(self) -> Filepath:
        return self._tmpdir_filepath

    def set_converted_filepath(self, path: Filepath):
        self._converted_filepath = path

    def get_converted_filepath(self) -> Filepath:
        return self._converted_filepath


class GeometryAxis:
    def __init__(self, node: ET.Element):
        self._name: str = node.attrib["Name"]
        self._model_id: str = get_attrib_if_exists(node, "Model")
        self._position = node.attrib["Position"]

    def get_name(self) -> str:
        return self._name

    def get_model_id(self) -> str:
        if self._model_id is not None:
            return self._model_id
        return self._name

    def get_position(self) -> str:
        return self._position

    def set_model(self, model: Model):
        self._model = model

    def get_model(self) -> Model:
        return self._model

    def set_stage_path(self, path: str):
        self._stage_path = path

    def get_stage_path(self) -> str:
        return self._stage_path

    def set_depth(self, depth: int):
        self._depth = depth

    def get_depth(self) -> int:
        return self._depth
