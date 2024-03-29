import numpy as np
from typing import List, Tuple
from unidecode import unidecode
from urllib.parse import unquote

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
        repository_path = root_layer.realPath
        repository_path_unquoted = unquote(repository_path)
        dir_index = repository_path_unquoted.rfind("/")
        return repository_path_unquoted[:dir_index + 1]

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

    def get_default_prim(stage: Usd.Stage) -> Usd.Prim:
        return stage.GetDefaultPrim()

    def add_reference(stage: Usd.Stage, ref_path_relative: str, stage_path: str, stage_subpath: str) -> Tuple[
            UsdGeom.Xform, UsdGeom.Xform]:
        xform_parent: UsdGeom.Xform = UsdGeom.Xform.Define(stage, stage_path)
        xform_ref: UsdGeom.Xform = UsdGeom.Xform.Define(stage, stage_path + stage_subpath)
        xform_ref_prim: Usd.Prim = xform_ref.GetPrim()
        path_unquoted = unquote(ref_path_relative)
        references: Usd.References = xform_ref_prim.GetReferences()
        references.AddReference(path_unquoted)
        return xform_parent, xform_ref

    def get_applied_scale(stage: Usd.Stage, scale_factor: float):
        stage_scale = UsdGeom.GetStageMetersPerUnit(stage)
        return scale_factor / stage_scale

    def apply_scale_xform_op(xform: UsdGeom.Xform, scale: float):
        scale_value = Gf.Vec3d(scale, scale, scale)
        xform_ordered_ops: List[UsdGeom.XformOp] = xform.GetOrderedXformOps()
        found_op = False
        for xform_op in xform_ordered_ops:
            if xform_op.GetOpType() == UsdGeom.XformOp.TypeScale:
                xform_op.Set(scale_value)
                found_op = True

        if not found_op:
            xform.AddScaleOp().Set(scale_value)

    def np_matrix_from_gdtf(value: str) -> np.matrix:
        # GDTF Matrix is: 4x4, row-major, Right-Handed, Z-up (Distance Unit not specified, but mm implied)
        # expect form like "{x,y,z,w}{x,y,z,w}{x,y,z,w}{x,y,z,w}" where "x","y","z", "w" is similar to 1.000000
        # make source compatible with np.matrix constructor: "x y z; x y z; x y z; x y z"
        value_alt = value[1:]  # Removes "{" prefix
        value_alt = value_alt[:-1]  # Removes "}" suffix
        value_alt = value_alt.replace("}{", "; ")
        value_alt = value_alt.replace(",", " ")
        np_matrix: np.matrix = np.matrix(value_alt)
        return np_matrix

    def gf_matrix_from_gdtf(np_matrix: np.matrix, scale: float) -> Gf.Matrix4d:
        # Row major matrix
        gf_matrix = Gf.Matrix4d(
            np_matrix.item((0, 0)), np_matrix.item((1, 0)), np_matrix.item((2, 0)), np_matrix.item((3, 0)),
            np_matrix.item((0, 1)), np_matrix.item((1, 1)), np_matrix.item((2, 1)), np_matrix.item((3, 1)),
            np_matrix.item((0, 2)), np_matrix.item((1, 2)), np_matrix.item((2, 2)), np_matrix.item((3, 2)),
            np_matrix.item((0, 3)), np_matrix.item((1, 3)), np_matrix.item((2, 3)), np_matrix.item((3, 3))
        )
        return gf_matrix

    def add_beam(stage: Usd.Stage, path: str, position_matrix: str, radius: float) -> UsdLux:
        applied_scale = USDTools.compute_applied_scale(stage)
        axis_matrix = USDTools.get_axis_rotation_matrix()
        light: UsdLux.DiskLight = UsdLux.DiskLight.Define(stage, path)
        translation, rotation = USDTools.compute_xform_values(position_matrix, applied_scale, axis_matrix)
        rotation += Gf.Vec3d(-90, 0, 0)
        scale = Gf.Vec3d(radius * 2, radius * 2, 1)
        USDTools._set_light_xform(light, translation, rotation, scale)
        USDTools._additional_default_attributes(light)
        return light

    def add_light_default(stage: Usd.Stage, path: str, height: float, diameter: float):
        light: UsdLux.DiskLight = UsdLux.DiskLight.Define(stage, path)
        translation = Gf.Vec3d(0, -height * 0.5, 0)
        rotation = Gf.Vec3d(-90, 0, 0)
        scale = Gf.Vec3d(diameter, diameter, 1)
        USDTools._set_light_xform(light, translation, rotation, scale)
        USDTools._additional_default_attributes(light)

    def _additional_default_attributes(light: UsdLux):
        prim = light.GetPrim()
        prim.CreateAttribute("visibleInPrimaryRay", Sdf.ValueTypeNames.Bool).Set(True)
        light.CreateIntensityAttr().Set(60_000)
        # if UsdLux.ShapingAPI.CanApply(prim):
        UsdLux.ShapingAPI.Apply(prim)

    def _set_light_xform(light: UsdLux.DiskLight, translation: Gf.Vec3d, rotation: Gf.Vec3d, scale: Gf.Vec3d):
        light.ClearXformOpOrder()  # Prevent error when overwritting
        light.AddTranslateOp().Set(translation)
        light.AddRotateZYXOp().Set(rotation)
        light.AddScaleOp().Set(scale)

    def set_light_attributes(light: UsdLux.DiskLight, beamAngle: float, intensity: float, colorTemp: float):
        if colorTemp is not None:
            light.GetEnableColorTemperatureAttr().Set(True)
            light.GetColorTemperatureAttr().Set(colorTemp)
        else:
            light.GetEnableColorTemperatureAttr().Set(False)
            light.GetColorTemperatureAttr().Set(6500)  # default value

        if intensity is not None:
            light.GetIntensityAttr().Set(intensity)

        if beamAngle is not None:
            prim: Usd.Prim = light.GetPrim()
            shapingAPI = UsdLux.ShapingAPI(prim)
            shapingAPI.GetShapingConeAngleAttr().Set(beamAngle)

    def compute_applied_scale(stage: Usd.Stage) -> float:
        gdtf_scale = 1  # GDTF dimensions are in meters
        applied_scale = USDTools.get_applied_scale(stage, gdtf_scale)
        return applied_scale

    def get_axis_rotation_matrix() -> Gf.Matrix3d:
        rotate_minus90deg_xaxis = Gf.Matrix3d(1, 0, 0,
                                              0, 0, 1,
                                              0, -1, 0)
        return rotate_minus90deg_xaxis

    def compute_xform_values(position_matrix: str, scale: float, axis_matrix: Gf.Matrix3d) -> (Gf.Vec3d, Gf.Vec3d):
        np_matrix: np.matrix = USDTools.np_matrix_from_gdtf(position_matrix)
        gf_matrix: Gf.Matrix4d = USDTools.gf_matrix_from_gdtf(np_matrix, scale)

        rotation: Gf.Rotation = gf_matrix.GetTranspose().ExtractRotation()
        euler: Gf.Vec3d = rotation.Decompose(Gf.Vec3d.XAxis(), Gf.Vec3d.YAxis(), Gf.Vec3d.ZAxis())

        translation_value = axis_matrix * gf_matrix.ExtractTranslation()
        rotation_value = axis_matrix * euler

        return translation_value, rotation_value

    def set_prim_attribute(prim: Usd.Prim, attribute_name: str, attribute_type: Sdf.ValueTypeNames, attribute_value):
        prim.CreateAttribute(f"mf:gdtf:{attribute_name}", attribute_type).Set(attribute_value)
