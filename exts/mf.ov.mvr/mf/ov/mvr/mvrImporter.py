import logging
import numpy as np
from typing import List, Tuple
import xml.etree.ElementTree as ET
from zipfile import ZipFile

from pxr import Gf, Usd, UsdGeom
from mf.ov.gdtf import gdtfImporter as gdtf

from .filepathUtility import Filepath
from .mvrUtil import Layer, Fixture
from .USDTools import USDTools


class MVRImporter:
    def convert(file: Filepath, mvr_output_dir: str, output_ext: str = ".usd") -> str:
        # TODO:  change output_ext to bool use_usda
        try:
            with ZipFile(file.fullpath, 'r') as archive:
                output_dir = mvr_output_dir + file.filename + ".mvr/"
                data = archive.read("GeneralSceneDescription.xml")
                root = ET.fromstring(data)
                MVRImporter._warn_for_version(root)
                url: str = MVRImporter.convert_mvr_usd(output_dir, file.filename, output_ext, root, archive)
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

    def convert_mvr_usd(output_dir: str, filename: str, ext: str, root: ET.Element, archive: ZipFile) -> str:
        scene: ET.Element = root.find("Scene")
        layers: List[Layer] = MVRImporter._get_layers(scene)
        for layer in layers:
            layer.find_fixtures()

        stage, url = MVRImporter._make_mvr_stage(output_dir, filename, ext, layers)
        MVRImporter._convert_gdtf(stage, layers, output_dir, archive, ext)
        stage.Save()
        return url

    def _get_layers(scene: ET.Element) -> List[Layer]:
        layersNode: ET.Element = scene.find("Layers")
        layerNodes: ET.Element = layersNode.findall("Layer")
        layers: List[Layer] = []
        for layerNode in layerNodes:
            layer: Layer = Layer(layerNode)
            layers.append(layer)
        return layers

    def _make_mvr_stage(output_dir: str, filename: str, ext: str, layers: List[Layer]) -> Tuple[Usd.Stage, str]:
        url: str = output_dir + filename + ext
        stage: Usd.Stage = USDTools.get_or_create_stage(url)
        MVRImporter._add_fixture_xform(stage, layers)

        return stage, url

    def _add_fixture_xform(stage: Usd.Stage, layers: List[Layer]):
        rotate_minus90deg_xaxis = Gf.Matrix3d(1, 0, 0, 0, 0, 1, 0, -1, 0)
        mvr_scale = UsdGeom.LinearUnits.millimeters  # MVR dimensions are in millimeters
        applied_scale: float = USDTools.get_applied_scale(stage, mvr_scale)

        for layer in layers:
            if layer.fixtures_len() > 0:
                scope: UsdGeom.Scope = USDTools.add_scope(stage, layer.get_name_usd())
                for fixture in layer.get_fixtures():
                    xform: UsdGeom.Xform = USDTools.add_fixture_xform(stage, scope, fixture.get_unique_name_usd())
                    fixture.set_stage_path(xform.GetPrim().GetPath())
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
                    # Scale Op is added in _add_gdtf_reference

                    fixture.apply_attributes_to_prim(xform.GetPrim())
        stage.Save()

    def _convert_gdtf(stage: Usd.Stage, layers: List[Layer], mvr_output_dir: str, archive: ZipFile, ext: str):
        gdtf_spec_uniq: List[str] = MVRImporter._get_gdtf_to_import(layers)
        gdtf_output_dir = mvr_output_dir
        for gdtf_spec in gdtf_spec_uniq:
            gdtf.GDTFImporter.convert_from_mvr(gdtf_spec, gdtf_output_dir, archive)
        MVRImporter._add_gdtf_reference(layers, stage, ext)

    def _get_gdtf_to_import(layers: List[Layer]) -> List[str]:
        result: List[str] = []
        for layer in layers:
            if layer.fixtures_len() > 0:
                current_fixture_names = [x.get_spec_name() for x in layer.get_fixtures()]
                current_fixture_names_set = set(current_fixture_names)
                current_fixture_names_uniq = list(current_fixture_names_set)
                for current_fixture_name_uniq in current_fixture_names_uniq:
                    result.append(current_fixture_name_uniq)
        return result

    def _add_gdtf_reference(layers: List[Layer], stage: Usd.Stage, ext: str):
        for layer in layers:
            if layer.fixtures_len() > 0:
                for fixture in layer.get_fixtures():
                    spec = fixture.get_spec_name()
                    relative_path = f"./{spec}.gdtf/{spec}{ext}"
                    stage_path = fixture.get_stage_path()
                    USDTools.add_reference(stage, relative_path, stage_path)
                    USDTools.copy_gdtf_scale(stage, stage_path, relative_path)
