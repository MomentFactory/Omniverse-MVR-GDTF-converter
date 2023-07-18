import logging
from typing import List
import xml.etree.ElementTree as ET
from zipfile import ZipFile

from .filepathUtility import Filepath
from .mvrUtil import Layer, Fixture


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
        # stage: Usd.Stage = USDTools.get_or_create_stage(url)
        layer: Layer = MVRImporter._get_layer(scene)
        fixtures: List[Fixture] = MVRImporter._get_fixtures(layer)
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
