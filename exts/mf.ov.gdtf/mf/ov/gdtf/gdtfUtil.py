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

class GeometryAxis:
    def __init__(self, node: ET.Element):
        self._name = node.attrib["Name"]
        self._model = get_attrib_if_exists(node, "Model")
        # self._xform = node.attrib["Position"]
