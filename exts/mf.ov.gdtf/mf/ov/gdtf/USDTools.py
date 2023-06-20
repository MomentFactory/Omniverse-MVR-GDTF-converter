import omni.usd
from pxr import Usd, UsdGeom
from unidecode import unidecode

import omni.usd
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
