import logging
import tempfile
from typing import List
import xml.etree.ElementTree as ET
from zipfile import ZipFile

from pxr import Usd

from .filepathUtility import Filepath
from .gdtfUtil import Model, GeometryAxis
from .gltfImporter import GLTFImporter
from .USDTools import USDTools


class GDTFImporter:
    TMP_ARCHIVE_EXTRACT_DIR = f"{tempfile.gettempdir()}/MF.OV.GDTF/"

    async def convert(file: Filepath, output_dir: str, output_ext: str = ".usd") -> bool:
        try:
            with ZipFile(file.fullpath, 'r') as archive:
                data = archive.read("description.xml")
                root = ET.fromstring(data)
                converted_models: List[Model] = await GDTFImporter._find_and_convert_gltf(root, archive, output_dir)
                GDTFImporter._convert_gdtf_usd(output_dir, file.filename, output_ext, root, converted_models)

        except Exception as e:
            logger = logging.getLogger(__name__)
            logger.error(f"Failed to parse gdtf file at {file.fullpath}. Make sure it is not corrupt. {e}")
            return False

        return True

    # region convert gltf
    async def _find_and_convert_gltf(root: ET.Element, archive: ZipFile, output_dir: str) -> List[Model]:
        models: List[Model] = GDTFImporter._get_model_nodes(root)
        models_filtered: List[Model] = GDTFImporter._filter_models(models)
        GDTFImporter._extract_gltf_to_tmp(models_filtered, archive)
        await GDTFImporter._convert_gltf(models_filtered, output_dir)
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
            else:
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
                model.set_tmpdir_filepath(Filepath(tmp_export_path))
            elif filepath_3ds:
                logger = logging.getLogger(__name__)
                logger.warn(f"Found unsupported 3ds file for {filename}, skipping.")
            else:
                logger = logging.getLogger(__name__)
                logger.warn(f"No file found for {filename}, skipping.")

    async def _convert_gltf(models: List[Model], gdtf_output_dir):
        gltf_output_dir = gdtf_output_dir + "gltf/"
        return await GLTFImporter.convert(models, gltf_output_dir)
    # endregion

    # region make gdtf
    def _convert_gdtf_usd(output_dir: str, filename: str, ext: str, root: ET.Element, models: List[Model]):
        url: str = output_dir + filename + ext
        stage: Usd.Stage = GDTFImporter._get_or_create_gdtf_usd(url)
        geometries: List[GeometryAxis] = GDTFImporter._get_geometry_hierarchy(root, models, stage)
        GDTFImporter._add_gltf_payload(stage, geometries)

    def _get_or_create_gdtf_usd(url: str) -> Usd.Stage:
        return USDTools.get_or_create_stage(url)

    def _get_geometry_hierarchy(root: ET.Element, models: List[Model], stage: Usd.Stage) -> List[GeometryAxis]:
        node_fixture: ET.Element = root.find("FixtureType")
        node_geometries = node_fixture.find("Geometries")
        default_prim_path = stage.GetDefaultPrim().GetPath()
        geometries: List[GeometryAxis] = []
        GDTFImporter._get_geometry_hierarchy_recursive(node_geometries, models, geometries, default_prim_path, 0)
        return geometries

    def _get_geometry_hierarchy_recursive(parent_node: ET.Element, models: List[Model], geometries: List[GeometryAxis],
                                          path: str, depth: int):
        child_nodes_geometry = parent_node.findall("Geometry")
        child_nodes_axis = parent_node.findall("Axis")
        child_nodes = child_nodes_geometry + child_nodes_axis
        for child_node in child_nodes:
            geometry: GeometryAxis = GeometryAxis(child_node)
            model_id: str = geometry.get_model_id()
            model: Model = next((model for model in models if model.get_name() == model_id), None)
            if model is not None:
                geometry.set_model = model
                stage_path = f"{path}/{model.get_name_usd()}"
                geometry.set_stage_path(stage_path)
                geometry.set_depth(depth)
                geometries.append(geometry)
                GDTFImporter._get_geometry_hierarchy_recursive(child_node, models, geometries, stage_path, depth + 1)

    def _add_gltf_payload(stage: Usd.Stage, geometries: List[GeometryAxis]):
        for geometry in geometries:
            print(geometry.get_stage_path())
    # endregion
