import omni.usd
from pxr import Usd


class USDTools:
    def get_context():
        return omni.usd.get_context()

    def get_stage() -> Usd.Stage:
        context = USDTools.get_context()
        return context.get_stage()

    def get_stage_directory() -> str:
        stage: Usd.Stage = USDTools.get_stage()
        root_layer = stage.GetRootLayer()
        repository_path = root_layer.repositoryPath
        dir_index = repository_path.rfind("/")
        return repository_path[:dir_index + 1]
