import omni.usd
from pxr import Usd, UsdGeom
from unidecode import unidecode

from pxr import Tf


class USDTools:
    def make_name_valid(name: str) -> str:
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
        dir_index = repository_path.rfind("/")
        return repository_path[:dir_index + 1]

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

    def add_reference(stage: Usd.Stage, ref_path_relative: str, stage_path: str, stage_subpath: str) -> UsdGeom.Xform:
        _: UsdGeom.Xform = UsdGeom.Xform.Define(stage, stage_path)
        print(stage_path + stage_subpath)
        xform_ref: UsdGeom.Xform = UsdGeom.Xform.Define(stage, stage_path + stage_subpath)
        xform_ref_prim: Usd.Prim = xform_ref.GetPrim()
        stage.Save()
        references: Usd.References = xform_ref_prim.GetReferences()
        references.AddReference(ref_path_relative)
        stage.Save()
        return xform_ref
