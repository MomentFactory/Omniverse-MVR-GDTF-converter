# import numpy as np
import xml.etree.ElementTree as ET

def get_attrib_if_exists(node: ET.Element, attr: str):
    return node.attrib[attr] if attr in node.attrib else None

class Model:
    def __init__(self, node: ET.Element):
        self._name = node.attrib["Name"]
        self._file = get_attrib_if_exists(node, "File")
        self._primitive_type = node.attrib["PrimitiveType"]
        self._height = node.attrib["Height"]
        self._length = node.attrib["Length"]
        self._width = node.attrib["Width"]

    def get_name(self) -> str:
        return self._name

    def has_file(self) -> bool:
        return self._file is not None and self._file != ""

    def get_file(self) -> str:
        return self._file

    def set_tmpdir_filepath(self, path: str):
        self._tmpdir_filepath = path

    def get_tmpdir_filepath(self) -> str:
        return self._tmpdir_filepath

    def set_converted_filepath(self, path: str):
        self._converted_filepath = path

    def get_converted_filepath(self) -> str:
        self._converted_filepath

class GeometryAxis:
    def __init__(self, node: ET.Element):
        self._name = node.attrib["Name"]
        self._model = get_attrib_if_exists(node, "Model")
        # self._xform = node.attrib["Position"]

    def get_name(self) -> str:
        return self._name

    def get_model_id(self) -> str:
        if self._model is not None:
            return self._model
        return self._name
