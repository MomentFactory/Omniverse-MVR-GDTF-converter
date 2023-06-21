import asyncio
import os
from typing import List

import omni.client
import omni.ext
import omni.ui as ui
import omni.usd
from pxr import Gf, Usd, UsdGeom, Sdf

from .gltfImporter import GLTFImporter
from .USDTools import USDTools


class MfOvGdtfExtension(omni.ext.IExt):
    def on_startup(self, _):
        self._window = ui.Window("GDTF Importer TMP", width=300, height=100)
        with self._window.frame:
            with ui.VStack():
                def on_gltf_import():
                    asyncio.ensure_future(_gltf_import_and_reference(self._input.model.get_value_as_string()))

                self._input = ui.StringField()
                ui.Button("Import GLTF", clicked_fn=on_gltf_import)

    def on_shutdown(self):
        pass


async def _gltf_import_and_reference(input_dir: str):
    filenames: List[str] = ["base", "yoke", "head"]
    input_ext = "glb"

    output_dir = USDTools.get_stage_directory() + "gltf/"

    output_files: List[str] = await GLTFImporter.convert(filenames, input_dir, input_ext, output_dir)
    _gltf_reference(output_files)


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
