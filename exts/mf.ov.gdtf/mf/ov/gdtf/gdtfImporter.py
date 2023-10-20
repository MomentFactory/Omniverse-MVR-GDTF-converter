from io import BytesIO
import logging
from typing import List
import xml.etree.ElementTree as ET
from zipfile import ZipFile

from pxr import Gf, Sdf, Usd, UsdGeom

from .filepathUtility import Filepath
from .gdtfUtil import Model, Geometry, Beam, FixtureAttributes
from .gltfImporter import GLTFImporter
from .USDTools import USDTools


class GDTFImporter:
    def convert(file: Filepath, output_dir: str, output_ext: str = ".usd") -> str:
        try:
            with ZipFile(file.fullpath, 'r') as archive:
                gdtf_output_dir = output_dir + file.filename + "_gdtf/"
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
            gdtf_output_dir = output_dir + spec_name + "_gdtf/"
            with ZipFile(gdtf_data) as gdtf_archive:
                GDTFImporter._convert(gdtf_archive, gdtf_output_dir, spec_name, output_ext)
                return True
        else:
            return False

    def _convert(archive: ZipFile, output_dir: str, name: str, output_ext: str) -> str:
        data = archive.read("description.xml")
        root = ET.fromstring(data)
        converted_models: List[Model] = GLTFImporter.convert(root, archive, output_dir)
        url: str = GDTFImporter._convert_gdtf_usd(output_dir, name, output_ext, root, converted_models)
        return url

    def _convert_gdtf_usd(output_dir: str, filename: str, ext: str, root: ET.Element, models: List[Model]) -> str:
        url: str = output_dir + filename + ext
        stage: Usd.Stage = GDTFImporter._get_or_create_gdtf_usd(url)
        geometries, beams = GDTFImporter._get_stage_hierarchy(root, models, stage)
        GDTFImporter._add_gltf_reference(stage, geometries)
        GDTFImporter._apply_gdtf_matrix(stage, geometries)
        GDTFImporter._add_light_to_hierarchy(stage, beams, geometries)
        GDTFImporter._apply_gltf_scale(stage, geometries)
        GDTFImporter._set_general_attributes(stage, root)

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
            xform_parent, xform_model = USDTools.add_reference(stage, relative_path, geometry.get_stage_path(), "/model")
            xform_model.GetPrim().CreateAttribute("mf:gdtf:converter_from_3ds", Sdf.ValueTypeNames.Bool).Set(model.get_converted_from_3ds())
            geometry.set_xform_parent(xform_parent)
            geometry.set_xform_model(xform_model)
        stage.Save()

    def _apply_gltf_scale(stage: Usd.Stage, geometries: List[Geometry]):
        world_xform: UsdGeom.Xform = UsdGeom.Xform(stage.GetDefaultPrim())
        stage_metersPerUnit = UsdGeom.GetStageMetersPerUnit(stage)
        scale = 1 / stage_metersPerUnit
        USDTools.apply_scale_xform_op(world_xform, scale)

        converted_3ds = False
        for geometry in geometries:
            model = geometry.get_model()
            if model.get_converted_from_3ds():
                converted_3ds = True
        if converted_3ds:
            for geometry in geometries:
                if geometry.get_tag() != 'Beam':
                    xform = geometry.get_xform_model()
                    USDTools.apply_scale_xform_op(xform, UsdGeom.LinearUnits.millimeters)  # force mm

        stage.Save()

    def _apply_gdtf_matrix(stage: Usd.Stage, geometries: List[Geometry]):
        applied_scale = USDTools.compute_applied_scale(stage)
        axis_matrix = USDTools.get_axis_rotation_matrix()

        for geometry in geometries:
            translation, rotation = USDTools.compute_xform_values(geometry.get_position_matrix(), applied_scale, axis_matrix)
            xform: UsdGeom.Xform = geometry.get_xform_parent()
            xform.ClearXformOpOrder()  # Prevent error when overwritting
            xform.AddTranslateOp().Set(translation)
            xform.AddRotateYXZOp().Set(rotation)
            xform.AddScaleOp().Set(Gf.Vec3d(1, 1, 1))

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
            light = USDTools.add_beam(stage, beam.get_stage_path(), beam.get_position_matrix(), beam.get_radius())
            beam.apply_attributes_to_prim(light.GetPrim())
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

    def _set_general_attributes(stage: Usd.Stage, root: ET.Element):
        fixtureAttr = FixtureAttributes(root)
        prim: Usd.Prim = USDTools.get_default_prim(stage)
        fixtureAttr.apply_attributes_to_prim(prim)
        stage.Save()
