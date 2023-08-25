from io import BytesIO
import logging
import numpy as np
import os
import shutil
import subprocess
import tempfile
from typing import List
import xml.etree.ElementTree as ET
from zipfile import ZipFile

from pxr import Gf, Usd, UsdGeom

from .filepathUtility import Filepath
from .gdtfUtil import Model, Geometry, Beam
from .gltfImporter import GLTFImporter
from .USDTools import USDTools


def convert_3ds_to_gltf(input, output):
    path = __file__
    my_env = os.environ.copy()
    my_env["PATH"] = path + '\\..\\' + os.pathsep + my_env['PATH']
    scriptPath = path + "\\..\\gltf-exporter.py"
    return subprocess.run(["py", scriptPath, input, output], capture_output=True, env=my_env)


class GDTFImporter:
    TMP_ARCHIVE_EXTRACT_DIR = f"{tempfile.gettempdir()}/MF.OV.GDTF/"

    def convert(file: Filepath, output_dir: str, output_ext: str = ".usd") -> str:
        try:
            with ZipFile(file.fullpath, 'r') as archive:
                gdtf_output_dir = output_dir + file.filename + ".gdtf/"
                url: str = GDTFImporter._convert(archive, gdtf_output_dir, file.filename, output_ext)
                return url

        except Exception as e:
            logger = logging.getLogger(__name__)
            logger.error(f"Failed to parse gdtf file at {file.fullpath}. Make sure it is not corrupt. {e}")
            return None

    def convert_from_mvr(spec_name: str, output_dir: str, mvr_archive: ZipFile, output_ext: str = ".usd") -> bool:
        spec_name_with_ext = spec_name + ".gdtf"
        if spec_name_with_ext in mvr_archive.namelist():
            gdtf_data = BytesIO(mvr_archive.read(spec_name_with_ext))
            gdtf_output_dir = output_dir + spec_name_with_ext + "/"
            with ZipFile(gdtf_data) as gdtf_archive:
                GDTFImporter._convert(gdtf_archive, gdtf_output_dir, spec_name, output_ext)
                return True
        else:
            return False

    def _convert(archive: ZipFile, output_dir: str, name: str, output_ext: str) -> str:
        data = archive.read("description.xml")
        root = ET.fromstring(data)
        converted_models: List[Model] = GDTFImporter._find_and_convert_gltf(root, archive, output_dir)
        url: str = GDTFImporter._convert_gdtf_usd(output_dir, name, output_ext, root, converted_models)
        return url

    # region convert gltf
    def _find_and_convert_gltf(root: ET.Element, archive: ZipFile, output_dir: str) -> List[Model]:
        models: List[Model] = GDTFImporter._get_model_nodes(root)
        models_filtered: List[Model] = GDTFImporter._filter_models(models)
        GDTFImporter._extract_gltf_to_tmp(models_filtered, archive)
        GDTFImporter._convert_gltf(models_filtered, output_dir)
        shutil.rmtree(GDTFImporter.TMP_ARCHIVE_EXTRACT_DIR)
        return models_filtered

    def _get_model_nodes(root: ET.Element) -> List[Model]:
        node_fixture: ET.Element = root.find("FixtureType")
        node_models: ET.Element = node_fixture.find("Models")
        nodes_model = node_models.findall("Model")
        models: List[Model] = []
        for node_model in nodes_model:
            models.append(Model(node_model))
        return models

    def _filter_models(models: List[Model]) -> List[Model]:
        filtered_models: List[Model] = []
        for model in models:
            if model.has_file():
                filtered_models.append(model)
            elif model.get_name().lower() != "pigtail" and model.get_name().lower() != "beam":
                logger = logging.getLogger(__name__)
                logger.warn(f"File attribute empty for model node {model.get_name()}, skipping.")
        return filtered_models

    def _extract_gltf_to_tmp(models: List[Model], gdtf_archive: ZipFile):
        namelist = gdtf_archive.namelist()

        for model in models:
            filename = model.get_file()
            filepath_glb = f"models/gltf/{filename}.glb"
            filepath_gltf = f"models/gltf/{filename}.gltf"
            filepath_3ds = f"models/3ds/{filename}.3ds"

            if filepath_glb in namelist:
                tmp_export_path = gdtf_archive.extract(filepath_glb, GDTFImporter.TMP_ARCHIVE_EXTRACT_DIR)
                model.set_tmpdir_filepath(Filepath(tmp_export_path))
            elif filepath_gltf in namelist:
                tmp_export_path = gdtf_archive.extract(filepath_gltf, GDTFImporter.TMP_ARCHIVE_EXTRACT_DIR)
                for filepath in namelist:  # Also import .bin, textures, etc.
                    if filepath.startswith(f"models/gltf/{filename}") and filepath != filepath_gltf:
                        gdtf_archive.extract(filepath, GDTFImporter.TMP_ARCHIVE_EXTRACT_DIR)
                model.set_tmpdir_filepath(Filepath(tmp_export_path))
            elif filepath_3ds in namelist:
                tmp_export_path = gdtf_archive.extract(filepath_3ds, GDTFImporter.TMP_ARCHIVE_EXTRACT_DIR)
                temp_export_path_gltf = tmp_export_path[:-4] + ".gltf"
                convert_3ds_to_gltf(tmp_export_path, temp_export_path_gltf)
                model.set_tmpdir_filepath(Filepath(temp_export_path_gltf))
                model.set_converted_from_3ds()
                os.remove(tmp_export_path)
            else:
                logger = logging.getLogger(__name__)
                logger.warn(f"No file found for {filename}, skipping.")

    def _convert_gltf(models: List[Model], gdtf_output_dir):
        gltf_output_dir = gdtf_output_dir + "gltf/"
        return GLTFImporter.convert(models, gltf_output_dir)
    # endregion

    # region make gdtf
    def _convert_gdtf_usd(output_dir: str, filename: str, ext: str, root: ET.Element, models: List[Model]) -> str:
        url: str = output_dir + filename + ext
        stage: Usd.Stage = GDTFImporter._get_or_create_gdtf_usd(url)
        geometries, beams = GDTFImporter._get_stage_hierarchy(root, models, stage)
        GDTFImporter._add_gltf_reference(stage, geometries)
        GDTFImporter._apply_gltf_scale(stage, geometries)
        GDTFImporter._apply_gdtf_matrix(stage, geometries)
        GDTFImporter._add_light_to_hierarchy(stage, beams, geometries)

        return url

    def _get_or_create_gdtf_usd(url: str) -> Usd.Stage:
        return USDTools.get_or_create_stage(url)

    def _get_stage_hierarchy(root: ET.Element, models: List[Model], stage: Usd.Stage) -> (List[Geometry], List[Beam]):
        node_fixture: ET.Element = root.find("FixtureType")
        node_geometries = node_fixture.find("Geometries")
        default_prim_path = stage.GetDefaultPrim().GetPath()
        geometries: List[Geometry] = []
        beams: List[Beam] = []
        GDTFImporter._get_stage_hierarchy_recursive(node_geometries, models, geometries, beams, default_prim_path, 0)
        return geometries, beams

    def _get_stage_hierarchy_recursive(parent_node: ET.Element, models: List[Model], geometries: List[Geometry],
                                       beams: List[Beam], path: str, depth: int):
        geometry_filter: List[str] = ['Geometry', 'Axis', 'Beam', 'Inventory']
        for child_node in list(parent_node):
            if 'Model' in child_node.attrib:
                if child_node.tag not in geometry_filter:
                    # Pass through (might want to add an xform)
                    GDTFImporter._get_stage_hierarchy_recursive(child_node, models, geometries, beams, path, depth + 1)
                else:
                    geometry: Geometry = Geometry(child_node)
                    model_id: str = geometry.get_model_id()
                    model: Model = next((model for model in models if model.get_name() == model_id), None)
                    if model is not None and model.has_file():
                        geometry.set_model(model)
                        stage_path = f"{path}/{model.get_name_usd()}"
                        geometry.set_stage_path(stage_path)
                        geometry.set_depth(depth)
                        geometries.append(geometry)
                        GDTFImporter._get_stage_hierarchy_recursive(child_node, models, geometries, beams, stage_path, depth + 1)
                    else:
                        if model_id.lower() == "pigtail":
                            pass  # Skip pigtail geometry
                        elif model_id.lower() == "beam":
                            stage_path = f"{path}/beam"
                            geometry.set_stage_path(stage_path)
                            beam: Beam = Beam(geometry, child_node)
                            beams.append(beam)
                        elif model is not None and not model.has_file():
                            logger = logging.getLogger(__name__)
                            logger.warn(f"No file found for {model_id}, skipping.")
                        else:
                            # Probably could just be a transform
                            pass
            else:
                # Probably could just be a transform
                pass

    def _add_gltf_reference(stage: Usd.Stage, geometries: List[Geometry]):
        stage_path = Filepath(USDTools.get_stage_directory(stage))
        for geometry in geometries:
            model: Model = geometry.get_model()
            relative_path: str = stage_path.get_relative_from(model.get_converted_filepath())
            xform_parent, xform_model = USDTools.add_reference(stage, relative_path, geometry.get_stage_path(),
                                                               "/model")
            geometry.set_xform_parent(xform_parent)
            geometry.set_xform_model(xform_model)
        stage.Save()

    def _apply_gltf_scale(stage: Usd.Stage, geometries: List[Geometry]):
        stage_metersPerUnit = UsdGeom.GetStageMetersPerUnit(stage)
        scale_offset = UsdGeom.LinearUnits.millimeters
        scale = scale_offset / stage_metersPerUnit
        scaleValue = Gf.Vec3f(scale, scale, scale)
        # TODO: Some conversion add a xform over the mesh with a scale on it, we should prevent that,
        # or include the reverse operation into scaleValue: scaleValue = scaleValue * (1 / xform.scale)

        for geometry in geometries:
            xform = geometry.get_xform_model()
            USDTools.apply_scale_xform_op(xform, scaleValue)

        stage.Save()

    def _apply_gdtf_matrix(stage: Usd.Stage, geometries: List[Geometry]):
        applied_scale = USDTools.compute_applied_scale(stage)
        axis_matrix = USDTools.get_axis_rotation_matrix()

        for geometry in geometries:
            translation, rotation = USDTools.compute_xform_values(geometry.get_position_matrix(), applied_scale, axis_matrix)
            xform: UsdGeom.Xform = geometry.get_xform_parent()
            xform.ClearXformOpOrder()  # Prevent error when overwritting
            xform.AddTranslateOp().Set(translation)
            xform.AddRotateXYZOp().Set(rotation)

        stage.Save()

    def _add_light_to_hierarchy(stage: Usd.Stage, beams: List[Beam], geometries: List[Geometry]):
        if len(beams) > 0:
            GDTFImporter._add_beam_to_hierarchy(stage, beams)
        else:
            # Some gdtf files only represents brackets and such. They contain only "Inventory" geometry.
            # We don't want to add a light source to those.
            has_not_inventory_geometry = False
            for geometry in geometries:
                if geometry.get_tag() != 'Inventory':
                    has_not_inventory_geometry = True
            if has_not_inventory_geometry:
                GDTFImporter._add_default_light_to_hierarchy(stage, geometries)

    def _add_beam_to_hierarchy(stage: Usd.Stage, beams: List[Beam]):
        for beam in beams:
            USDTools.add_beam(stage, beam.get_stage_path(), beam.get_position_matrix(), beam.get_radius())
        stage.Save()

    def _add_default_light_to_hierarchy(stage: Usd.Stage, geometries: List[Geometry]):
        deepest_geom = geometries[-1]
        max_depth = deepest_geom.get_depth()
        for geom in reversed(geometries):
            depth = geom.get_depth()
            if (depth > max_depth):
                deepest_geom = geom
                max_depth = depth
        light_stage_path = deepest_geom.get_stage_path() + "/Beam"
        model = deepest_geom.get_model()
        USDTools.add_light_default(stage, light_stage_path, model.get_height(), model.get_width())
        stage.Save()
    # endregion
