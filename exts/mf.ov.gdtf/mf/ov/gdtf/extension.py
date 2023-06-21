import asyncio
import logging

import omni.client
import omni.ext
import omni.ui as ui
import omni.usd

from .filepathUtility import Filepath
from .gdtfImporter import GDTFImporter
from .USDTools import USDTools


class MfOvGdtfExtension(omni.ext.IExt):
    def on_startup(self, _):
        self._window = ui.Window("GDTF Importer TMP", width=300, height=100)
        with self._window.frame:
            with ui.VStack():
                def on_gdtf_import():
                    _import_gdtf(self._input.model.get_value_as_string())

                self._input = ui.StringField()
                ui.Button("Import GDTF", clicked_fn=on_gdtf_import)

    def on_shutdown(self):
        pass


def _import_gdtf(filepath: str):
    file: Filepath = Filepath(filepath)

    if file.is_nucleus_path():
        # TODO: Cannot Unzip directly from omniverse, might have to download the file locally as tmp
        logger = logging.getLogger(__name__)
        logger.error("Cannot import directly from Omniverse")
        return

    stage_dir = USDTools.get_stage_directory()
    if stage_dir == "":
        output_dir = file.directory
        logger = logging.getLogger(__name__)
        logger.warn(f"Current stage is anonymous, output files in filesystem at {output_dir} instead of in nucleus")
    else:
        output_dir = USDTools.get_stage_directory()

    output_dir += "gdtf/"
    asyncio.ensure_future(GDTFImporter.convert(filepath, output_dir))


"""
def _gltf_reference(files: List[str]):
    stage: Usd.Stage = USDTools.get_stage()
    default_prim: Usd.Prim = stage.GetDefaultPrim()
    default_path: Sdf.Path = default_prim.GetPath()

    stage_metersPerUnit = UsdGeom.GetStageMetersPerUnit(stage)
    scale_offset = UsdGeom.LinearUnits.millimeters
    scale = scale_offset / stage_metersPerUnit

    # TODO: Replace with Geometry hierarchy
    for file in files:
        filename = os.path.splitext(file)[0]
        xform: UsdGeom.Xform = UsdGeom.Xform.Define(stage, default_path.AppendPath(Sdf.Path(filename)))
        xform_prim: Usd.Prim = xform.GetPrim()
        references: Usd.References = xform_prim.GetReferences()
        references.AddReference(f"./gltf/{file}")

        base_xform_ordered_ops: List[UsdGeom.XformOp] = xform.GetOrderedXformOps()
        for xform_op in base_xform_ordered_ops:
            if xform_op.GetOpType() == UsdGeom.XformOp.TypeScale:
                xform_op.Set(Gf.Vec3f(scale, scale, scale))
"""
