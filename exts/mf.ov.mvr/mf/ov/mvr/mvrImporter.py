import logging
import numpy as np
from typing import List
import xml.etree.ElementTree as ET
from zipfile import ZipFile

from pxr import Gf, Usd, UsdGeom

from .filepathUtility import Filepath
from .mvrUtil import Layer, Fixture
from .USDTools import USDTools


class MVRImporter:
    async def convert(file: Filepath, mvr_output_dir: str, output_ext: str = ".usd") -> str:
        try:
            with ZipFile(file.fullpath, 'r') as archive:
                output_dir = mvr_output_dir + file.filename + ".mvr/"
                data = archive.read("GeneralSceneDescription.xml")
                root = ET.fromstring(data)
                MVRImporter._warn_for_version(root)
                url: str = MVRImporter.convert_mvr_usd(output_dir, file.filename, output_ext, root)
                return url
        except Exception as e:
            logger = logging.getLogger(__name__)
            logger.error(f"Failed to parse mvr file at {file.fullpath}. Make sure it is not corrupt. {e}")
            return None

    def _warn_for_version(root):
        v_major = root.attrib["verMajor"]
        v_minor = root.attrib["verMinor"]
        if v_major != "1" or v_minor != "5":
            logger = logging.getLogger(__name__)
            logger.warn(f"This extension is tested with mvr v1.5, this file version is {v_major}.{v_minor}")

    def convert_mvr_usd(output_dir: str, filename: str, ext: str, root: ET.Element) -> str:
        url: str = output_dir + filename + ext
        scene: ET.Element = root.find("Scene")
        stage: Usd.Stage = USDTools.get_or_create_stage(url)
        layer: Layer = MVRImporter._get_layer(scene)
        fixtures: List[Fixture] = MVRImporter._get_fixtures(layer)
        MVRImporter._add_fixture_xform(stage, fixtures)

        fixture_names: List[str] = [x.get_unique_name_usd() for x in fixtures]
        print(url)
        print(fixture_names)
        return url

    def _get_layer(scene: ET.Element) -> ET.Element:
        # According to spec, must contain exactly 1 layer
        layers: ET.Element = scene.find("Layers")
        layer: Layer = Layer(layers.find("Layer"))
        return layer

    def _get_fixtures(layer: Layer) -> List[Fixture]:
        childlist = layer.get_node().find("ChildList")
        fixtures = childlist.findall("Fixture")
        return [Fixture(x) for x in fixtures]

    def _add_fixture_xform(stage: Usd.Stage, fixtures: List[Fixture]):
        rotate_minus90deg_xaxis = Gf.Matrix3d(1, 0, 0, 0, 0, 1, 0, -1, 0)
        mvr_scale = 0.001  # MVR dimensions are in milimeters
        applied_scale: float = USDTools.get_applied_scale(stage, mvr_scale)

        for fixture in fixtures:
            xform: UsdGeom.Xform = USDTools.add_fixture_xform(stage, fixture.get_unique_name_usd())
            np_matrix: np.matrix = USDTools.np_matrix_from_mvr(fixture.get_matrix())
            gf_matrix: Gf.Matrix4d = USDTools.gf_matrix_from_mvr(np_matrix, applied_scale)

            rotation: Gf.Rotation = gf_matrix.ExtractRotation()
            euler: Gf.Vec3d = rotation.Decompose(Gf.Vec3d.XAxis(), Gf.Vec3d.YAxis(), Gf.Vec3d.ZAxis())

            # Z-up to Y-up
            # TODO: Validate with stage up axis
            translation = rotate_minus90deg_xaxis * gf_matrix.ExtractTranslation()
            rotate = rotate_minus90deg_xaxis * euler

            xform.ClearXformOpOrder()  # Prevent error when overwritting
            xform.AddTranslateOp().Set(translation)
            xform.AddRotateXYZOp().Set(rotate)

            fixture.apply_attributes_to_prim(xform.GetPrim())
        stage.Save()
