import xml.etree.ElementTree as ET

from pxr import Usd, UsdGeom, UsdLux, Sdf

from .filepathUtility import Filepath
from .USDTools import USDTools


def get_attrib_if_exists(node: ET.Element, attr: str):
    return node.attrib[attr] if attr in node.attrib else None


def get_attrib_text_if_exists(node: ET.Element, attr: str):
    return get_attrib_if_exists(node, attr)


def get_attrib_int_if_exists(node: ET.Element, attr: str):
    str_value = get_attrib_if_exists(node, attr)
    if str_value is not None:
        return int(str_value)
    return None


def get_attrib_float_if_exists(node: ET.Element, attr: str):
    str_value = get_attrib_if_exists(node, attr)
    if str_value is not None:
        return float(str_value)
    return None


def set_attribute_text_if_valid(prim: Usd.Prim, name: str, value: str):
    if value is not None:
        USDTools.set_prim_attribute(prim, name, Sdf.ValueTypeNames.String, value)


def set_attribute_int_if_valid(prim: Usd.Prim, name: str, value: str):
    if value is not None:
        USDTools.set_prim_attribute(prim, name, Sdf.ValueTypeNames.Int, value)


def set_attribute_float_if_valid(prim: Usd.Prim, name: str, value: str):
    if value is not None:
        USDTools.set_prim_attribute(prim, name, Sdf.ValueTypeNames.Float, value)


class Model:
    def __init__(self, node: ET.Element):
        self._name = node.attrib["Name"]
        self._name_usd = USDTools.make_name_valid(self._name)
        self._file = get_attrib_if_exists(node, "File")
        self._primitive_type = node.attrib["PrimitiveType"]
        self._height = float(node.attrib["Height"])
        self._length = float(node.attrib["Length"])
        self._width = float(node.attrib["Width"])
        self._converted_from_3ds = False

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

    def set_converted_from_3ds(self):
        self._converted_from_3ds = True

    def get_converted_from_3ds(self):
        return self._converted_from_3ds

    def set_converted_filepath(self, path: Filepath):
        self._converted_filepath = path

    def get_converted_filepath(self) -> Filepath:
        return self._converted_filepath

    def get_height(self) -> float:
        return self._height

    def get_width(self) -> float:
        return self._width


class Geometry:
    def __init__(self, node: ET.Element):
        self._name: str = node.attrib["Name"]
        self._model_id: str = get_attrib_if_exists(node, "Model")
        self._position_matrix = node.attrib["Position"]
        self._tag = node.tag

    def get_tag(self) -> str:
        return self._tag

    def get_name(self) -> str:
        return self._name

    def get_model_id(self) -> str:
        if self._model_id is not None:
            return self._model_id
        return self._name

    def get_position_matrix(self) -> str:
        return self._position_matrix

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

    def set_xform_model(self, xform: UsdGeom.Xform):
        self._xform_model = xform

    def get_xform_model(self) -> UsdGeom.Xform:
        return self._xform_model

    def set_xform_parent(self, xform: UsdGeom.Xform):
        self._xform_parent = xform

    def get_xform_parent(self) -> UsdGeom.Xform:
        return self._xform_parent


class Beam:
    def __init__(self, geometry: Geometry, node: ET.Element):
        self._radius = float(node.attrib["BeamRadius"])
        self._position_matrix = geometry.get_position_matrix()
        self._stage_path = geometry.get_stage_path()

        # The attributes should always exists as per standard definition
        self._beam_angle = get_attrib_float_if_exists(node, "BeamAngle")
        self._beam_type = get_attrib_text_if_exists(node, "BeamType")
        self._color_rendering_index = get_attrib_int_if_exists(node, "ColorRenderingIndex")
        self._color_temperature = get_attrib_float_if_exists(node, "ColorTemperature")
        self._field_angle = get_attrib_float_if_exists(node, "FieldAngle")
        self._lamp_type = get_attrib_text_if_exists(node, "LampType")
        self._luminous_flux = get_attrib_float_if_exists(node, "LuminousFlux")
        self._power_consumption = get_attrib_float_if_exists(node, "PowerConsumption")

    def get_radius(self) -> float:
        return self._radius

    def get_position_matrix(self) -> str:
        return self._position_matrix

    def get_stage_path(self) -> str:
        return self._stage_path

    def apply_attributes_to_prim(self, light: UsdLux):
        prim: Usd.Prim = light.GetPrim()
        set_attribute_float_if_valid(prim, "BeamAngle", self._beam_angle)
        set_attribute_text_if_valid(prim, "BeamType", self._beam_type)
        set_attribute_int_if_valid(prim, "ColorRenderingIndex", self._color_rendering_index)
        set_attribute_float_if_valid(prim, "ColorTemperature", self._color_temperature)
        set_attribute_float_if_valid(prim, "FieldAngle", self._field_angle)
        set_attribute_text_if_valid(prim, "LampType", self._lamp_type)
        set_attribute_float_if_valid(prim, "LuminousFlux", self._luminous_flux)
        set_attribute_float_if_valid(prim, "PowerConsumption", self._power_consumption)
        USDTools.set_light_attributes(light, self._beam_angle, self._luminous_flux, self._color_temperature)


class FixtureAttributes:
    def __init__(self, root: ET.Element):
        self._operating_temperature_high = None
        self._operating_temperature_low = None
        self._weight = None
        self._leg_height = None

        node_fixture: ET.Element = root.find("FixtureType")
        node_physdesc: ET.Element = node_fixture.find("PhysicalDescriptions")
        if node_physdesc is not None:
            node_properties: ET.Element = node_physdesc.find("Properties")
            if node_properties is not None:
                node_operatingtemp: ET.Element = node_properties.find("OperatingTemperature")
                if node_operatingtemp is not None:
                    self._operating_temperature_high = get_attrib_float_if_exists(node_operatingtemp, "High")
                    self._operating_temperature_low = get_attrib_float_if_exists(node_operatingtemp, "Low")
                node_weight: ET.Element = node_properties.find("Weight")
                if node_weight is not None:
                    self._weight = get_attrib_float_if_exists(node_weight, "Value")
                node_legheight: ET.Element = node_properties.find("LegHeight")
                if node_legheight is not None:
                    self._leg_height = get_attrib_float_if_exists(node_legheight, "Value")

    def apply_attributes_to_prim(self, prim: Usd.Prim):
        set_attribute_float_if_valid(prim, "OperatingTemperature:High", self._operating_temperature_high)
        set_attribute_float_if_valid(prim, "OperatingTemperature:Low", self._operating_temperature_low)
        set_attribute_float_if_valid(prim, "Weight", self._weight)
        set_attribute_float_if_valid(prim, "LegHeight", self._leg_height)
