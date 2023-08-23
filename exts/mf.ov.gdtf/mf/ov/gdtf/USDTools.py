import numpy as np
from typing import List, Tuple
from unidecode import unidecode

import omni.usd
from pxr import Gf, Tf, Sdf, UsdLux, Usd, UsdGeom


class USDTools:
    def make_name_valid(name: str) -> str:
        if name[:1].isdigit():
            name = "_" + name
        return Tf.MakeValidIdentifier(unidecode(name))

    def get_context():
        return omni.usd.get_context()

    def get_stage() -> Usd.Stage:
        context = USDTools.get_context()
        return context.get_stage()

    def get_stage_directory(stage: Usd.Stage = None) -> str:
        if stage is None:
            stage = USDTools.get_stage()
        root_layer = stage.GetRootLayer()
        repository_path = root_layer.repositoryPath
        repository_path_spaces = repository_path.replace("%20", " ")
        dir_index = repository_path_spaces.rfind("/")
        return repository_path_spaces[:dir_index + 1]

    def get_or_create_stage(url: str) -> Usd.Stage:
        try:  # TODO: Better way to check if stage exists?
            return Usd.Stage.Open(url)
        except:
            stage = Usd.Stage.CreateNew(url)
            UsdGeom.SetStageMetersPerUnit(stage, UsdGeom.LinearUnits.centimeters)  # TODO get user defaults
            UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)  # TODO get user defaults
            default_prim = stage.DefinePrim("/World", "Xform")
            stage.SetDefaultPrim(default_prim)
            stage.Save()
            return stage

    def add_reference(stage: Usd.Stage, ref_path_relative: str, stage_path: str, stage_subpath: str) -> Tuple[
            UsdGeom.Xform, UsdGeom.Xform]:
        xform_parent: UsdGeom.Xform = UsdGeom.Xform.Define(stage, stage_path)
        xform_ref: UsdGeom.Xform = UsdGeom.Xform.Define(stage, stage_path + stage_subpath)
        xform_ref_prim: Usd.Prim = xform_ref.GetPrim()
        unescaped_path: str = ref_path_relative.replace("%20", " ")
        references: Usd.References = xform_ref_prim.GetReferences()
        references.AddReference(unescaped_path)
        return xform_parent, xform_ref

    def get_applied_scale(stage: Usd.Stage, scale_factor: float):
        stage_scale = UsdGeom.GetStageMetersPerUnit(stage)
        return scale_factor / stage_scale

    def apply_scale_xform_op(xform: UsdGeom.Xform, value: Gf.Vec3f):
        xform_ordered_ops: List[UsdGeom.XformOp] = xform.GetOrderedXformOps()
        found_op = False
        for xform_op in xform_ordered_ops:
            if xform_op.GetOpType() == UsdGeom.XformOp.TypeScale:
                xform_op.Set(value)
                found_op = True

        if not found_op:
            xform.AddScaleOp().Set(value)

    def np_matrix_from_gdtf(value: str) -> np.matrix:
        # GDTF Matrix is: 4x4, row-major, Right-Handed, Z-up (Distance Unit not specified, but mm implied)
        # expect form like "{x,y,z,w}{x,y,z,w}{x,y,z,w}{x,y,z,w}" where "x","y","z", "w" is similar to 1.000000
        # make source compatible with np.matrix constructor: "x y z; x y z; x y z; x y z"
        value_alt = value[1:]  # Removes "{" prefix
        value_alt = value_alt[:-1]  # Removes "}" suffix
        value_alt = value_alt.replace("}{", "; ")
        value_alt = value_alt.replace(",", " ")

        np_matrix: np.matrix = np.matrix(value_alt)
        np_matrix.transpose()
        return np_matrix

    def gf_matrix_from_gdtf(np_matrix: np.matrix, scale: float) -> Gf.Matrix4d:
        gf_matrix = Gf.Matrix4d(
            np_matrix.item((0, 0)), np_matrix.item((0, 1)), np_matrix.item((0, 2)), np_matrix.item((0, 3)) * scale,
            np_matrix.item((1, 0)), np_matrix.item((1, 1)), np_matrix.item((1, 2)), np_matrix.item((1, 3)) * scale,
            np_matrix.item((2, 0)), np_matrix.item((2, 1)), np_matrix.item((2, 2)), np_matrix.item((2, 3)) * scale,
            np_matrix.item((3, 0)), np_matrix.item((3, 1)), np_matrix.item((3, 2)), np_matrix.item((3, 3))
        )

        # Uses transpose because gdtf is row-major and faster to write than to rewrite the whole matrix constructor
        return gf_matrix.GetTranspose()

    def add_light(stage: Usd.Stage, path: str, position: str, radius: float):
        scale = USDTools.get_applied_scale(stage, 1)
        light: UsdLux.DiskLight = UsdLux.DiskLight.Define(stage, path)

        np_matrix: np.matrix = USDTools.np_matrix_from_gdtf(position)
        gf_matrix: Gf.Matrix4d = USDTools.gf_matrix_from_gdtf(np_matrix, scale)
        rotation: Gf.Rotation = gf_matrix.ExtractRotation()
        euler: Gf.Vec3d = rotation.Decompose(Gf.Vec3d.XAxis(), Gf.Vec3d.YAxis(), Gf.Vec3d.ZAxis())
        rotate_minus90deg_xaxis = Gf.Matrix3d(1, 0, 0, 0, 0, 1, 0, -1, 0)
        translation = rotate_minus90deg_xaxis * gf_matrix.ExtractTranslation()
        rotate = euler + Gf.Vec3d(-90, 0, 0)

        light.ClearXformOpOrder()  # Prevent error when overwritting
        light.AddTranslateOp().Set(translation)
        light.AddRotateXYZOp().Set(rotate)
        light.AddScaleOp().Set(Gf.Vec3d(radius * 2 * scale, radius * 2 * scale, 1))
        light.CreateIntensityAttr().Set(60_000)
        light.GetPrim().CreateAttribute("visibleInPrimaryRay", Sdf.ValueTypeNames.Bool).Set(True)
