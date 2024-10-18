from typing import List
import xml.etree.ElementTree as ET

from pxr import Usd, Sdf

from .USDTools import USDTools


class Fixture:
    def __init__(self, node: ET.Element):
        self._root = node
        self._name = node.attrib["name"]
        self._uuid = node.attrib["uuid"]
        self._matrix = self._get_value_text_if_exists("Matrix")
        self._gdtf_spec = self._get_value_text_if_exists("GDTFSpec")
        self._gdtf_mode = self._get_value_text_if_exists("GDTFMode")
        self._custom_commands = self._get_custom_commands_values()
        self._classing = self._get_value_text_if_exists("Classing")
        self._addresses = self._get_addresses_values()
        self._fixture_id = self._get_value_int_if_exists("fixtureID")
        self._unit_number = self._get_value_int_if_exists("UnitNumber")
        self._fixture_type_id = self._get_value_int_if_exists("FixtureTypeId")
        self._custom_id = self._get_value_int_if_exists("CustomId")
        self._cie_color = self._get_color_values()
        self._cast_shadow = self._get_value_bool_if_exists("CastShadow")

    def get_unique_name_usd(self) -> str:
        return USDTools.make_name_valid(self._name + "_" + self._uuid)

    def get_matrix(self) -> str:
        return self._matrix

    def set_stage_path(self, path: str):
        self._stage_path = path

    def get_stage_path(self) -> str:
        return self._stage_path

    def get_spec_name(self) -> str:
        spec_name = self._gdtf_spec
        if self._gdtf_spec[-5:] == ".gdtf":
            spec_name = self._gdtf_spec[:-5]
        return spec_name

    def _get_value_text_if_exists(self, name: str) -> str:
        node = self._get_child_node(name)
        if node is not None:
            text = node.text
            if text is not None:
                return node.text
        return None

    def _get_value_int_if_exists(self, name: str) -> int:
        txt = self._get_value_text_if_exists(name)
        if txt is None:
            return None
        return int(txt)

    def _get_value_bool_if_exists(self, name: str) -> bool:
        txt = self._get_value_text_if_exists(name)
        if txt is None:
            return None
        return bool(txt)

    def _get_child_node(self, node: str):
        return self._root.find(node)

    def _get_custom_commands_values(self) -> List[str]:
        values: List[str] = []
        node = self._get_child_node("CustomCommands")
        if node is not None:
            subnodes = node.findall("CustomCommand")
            if subnodes is not None and len(subnodes) > 0:
                values = [x.text for x in subnodes]
        return values

    def _get_addresses_values(self) -> List[str]:
        values: List[str] = []
        node = self._get_child_node("Addresses")
        if node is not None:
            subnodes = node.findall("Address")
            if subnodes is not None and len(subnodes):
                values = [int(x.text) for x in subnodes]
        return values

    def _get_color_values(self) -> List[float]:
        colors: List[float] = []
        node = self._get_child_node("Color")
        if node is not None:
            colors = [float(x) for x in node.text.split(",")]
        return colors

    def apply_attributes_to_prim(self, prim: Usd.Prim):
        self._set_attribute_text_if_valid(prim, "name", self._name)
        self._set_attribute_text_if_valid(prim, "uuid", self._uuid)
        self._set_attribute_text_if_valid(prim, "GDTFSpec", self._gdtf_spec)
        self._set_attribute_text_if_valid(prim, "GDTFMode", self._gdtf_mode)
        self._set_attribute_textarray_if_valid(prim, "CustomCommands", self._custom_commands)
        self._set_attribute_text_if_valid(prim, "Classing", self._classing)
        self._set_attribute_intarray_if_valid(prim, "Addresses", self._addresses)
        self._set_attribute_int_if_valid(prim, "FixtureID", self._fixture_id)
        self._set_attribute_int_if_valid(prim, "UnitNumber", self._unit_number)
        self._set_attribute_int_if_valid(prim, "FixtureTypeId", self._fixture_type_id)
        self._set_attribute_int_if_valid(prim, "CustomId", self._custom_id)
        self._set_attribute_floatarray_if_valid(prim, "CIEColor", self._cie_color)
        self._set_attribute_bool_if_value(prim, "CastShadow", self._cast_shadow)

    def _set_attribute_text_if_valid(self, prim: Usd.Prim, name: str, value: str):
        if value is not None:
            USDTools.set_fixture_attribute(prim, name, Sdf.ValueTypeNames.String, value)

    def _set_attribute_int_if_valid(self, prim: Usd.Prim, name: str, value: int):
        if value is not None:
            USDTools.set_fixture_attribute(prim, name, Sdf.ValueTypeNames.Int, value)

    def _set_attribute_bool_if_value(self, prim: Usd.Prim, name: str, value: bool):
        if value is not None:
            USDTools.set_fixture_attribute(prim, name, Sdf.ValueTypeNames.Bool, value)

    def _set_attribute_textarray_if_valid(self, prim: Usd.Prim, name: str, value: List[str]):
        if value is not None and len(value) > 0:
            USDTools.set_fixture_attribute(prim, name, Sdf.ValueTypeNames.StringArray, value)

    def _set_attribute_intarray_if_valid(self, prim: Usd.Prim, name: str, value: List[int]):
        if value is not None and len(value) > 0:
            USDTools.set_fixture_attribute(prim, name, Sdf.ValueTypeNames.IntArray, value)

    def _set_attribute_floatarray_if_valid(self, prim: Usd.Prim, name: str, value: List[float]):
        if value is not None and len(value) > 0:
            USDTools.set_fixture_attribute(prim, name, Sdf.ValueTypeNames.FloatArray, value)


class Layer:
    def __init__(self, node: ET.Element):
        self._name = node.attrib["name"]
        self._uuid = node.attrib["uuid"]
        self._node = node
        self._fixtures = []

    def get_name_usd(self) -> str:
        return USDTools.make_name_valid(self._name)

    def find_fixtures(self):
        childlist = self._node.find("ChildList")
        if childlist is not None:
            fixtures = childlist.findall("Fixture")
            self._fixtures = [Fixture(x) for x in fixtures]

    def fixtures_len(self) -> int:
        return len(self._fixtures)

    def get_fixtures(self) -> List[Fixture]:
        return self._fixtures
