import numpy as np
from unidecode import unidecode

from pxr import Gf, Tf, Sdf, Usd, UsdGeom


class USDTools:
    def make_name_valid(name: str) -> str:
        return Tf.MakeValidIdentifier(unidecode(name))

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

    def add_scope(stage: Usd.Stage, name: str) -> UsdGeom.Scope:
        default_prim_path: Sdf.Path = stage.GetDefaultPrim().GetPrimPath()
        scope_path: Sdf.Path = default_prim_path.AppendPath(name)
        scope: UsdGeom.Scope = UsdGeom.Scope.Define(stage, scope_path)
        return scope

    def add_fixture_xform(stage: Usd.Stage, scope: UsdGeom.Scope, name: str) -> UsdGeom.Xform:
        path = scope.GetPath().AppendPath(name)
        xform: UsdGeom.Xform = UsdGeom.Xform.Define(stage, path)
        return xform

    def get_applied_scale(stage: Usd.Stage, scale_factor: float) -> float:
        stage_scale = UsdGeom.GetStageMetersPerUnit(stage)
        return scale_factor / stage_scale

    def np_matrix_from_mvr(value: str) -> np.matrix:
        # MVR Matrix is: 4x3, Right-handed, Z-up, 1 Distance Unit equals 1mm
        # expect form like "<Matrix>{x,y,z}{x,y,z}{x,y,z}{x,y,z}</Matrix>" where "x","y","z" is similar to 1.000000
        # make source compatible with np.matrix constructor: "x y z; x y z; x y z; x y z"
        value_alt = value[1:]  # Removes "{" prefix
        value_alt = value_alt[:-1]  # Removes "}" suffix
        value_alt = value_alt.replace("}{", "; ")
        value_alt = value_alt.replace(",", " ")
        np_matrix: np.matrix = np.matrix(value_alt)
        return np_matrix

    def gf_matrix_from_mvr(np_matrix: np.matrix, scale: float) -> Gf.Matrix4d:
        # Column major matrix
        gf_matrix = Gf.Matrix4d(
            np_matrix.item((0, 0)), np_matrix.item((0, 1)), np_matrix.item((0, 2)), 0,
            np_matrix.item((1, 0)), np_matrix.item((1, 1)), np_matrix.item((1, 2)), 0,
            np_matrix.item((2, 0)), np_matrix.item((2, 1)), np_matrix.item((2, 2)), 0,
            np_matrix.item((3, 0)) * scale, np_matrix.item((3, 1)) * scale, np_matrix.item((3, 2)) * scale, 1
        )

        return gf_matrix

    def set_fixture_attribute(prim: Usd.Prim, attribute_name: str, attribute_type: Sdf.ValueTypeNames, attribute_value):
        prim.CreateAttribute(f"mf:mvr:{attribute_name}", attribute_type).Set(attribute_value)

    def add_reference(stage: Usd.Stage, ref_path_relative: str, stage_path: str):
        xform_ref: UsdGeom.Xform = stage.GetPrimAtPath(stage_path)
        unescaped_path: str = ref_path_relative.replace("%20", " ")
        references: Usd.References = xform_ref.GetReferences()
        references.AddReference(unescaped_path)
        stage.Save()
