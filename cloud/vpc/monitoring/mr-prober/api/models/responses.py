from typing import List, Dict, Any, Optional, Literal

from pydantic import BaseModel

from .common import Cluster, ClusterVariable, ClusterRecipe, ClusterRecipeFile, \
    Prober, ProberFile, ProberConfig


class SuccessResponse(BaseModel):
    status: Literal["ok"] = "ok"


class ErrorResponse(BaseModel):
    status: Literal["error"] = "error"
    message: str
    details: Optional[Dict[str, Any]] = None


class ClusterListResponse(SuccessResponse):
    clusters: List[Cluster]


class ClusterResponse(SuccessResponse):
    cluster: Cluster


class CreateClusterResponse(SuccessResponse):
    cluster: Cluster


class ClusterRecipeListResponse(SuccessResponse):
    recipes: List[ClusterRecipe]


class ClusterRecipeResponse(SuccessResponse):
    recipe: ClusterRecipe


class CreateClusterRecipeResponse(SuccessResponse):
    recipe: ClusterRecipe


class UpdateClusterRecipeResponse(SuccessResponse):
    recipe: ClusterRecipe


class UpdateClusterResponse(SuccessResponse):
    cluster: Cluster


class ClusterRecipeFileResponse(SuccessResponse):
    file: ClusterRecipeFile


class CreateClusterRecipeFileResponse(SuccessResponse):
    recipe: ClusterRecipe
    file: ClusterRecipeFile


class UpdateClusterRecipeFileResponse(SuccessResponse):
    recipe: ClusterRecipe
    file: ClusterRecipeFile


class ProberListResponse(SuccessResponse):
    probers: List[Prober]


class ProberResponse(SuccessResponse):
    prober: Prober


class CreateProberResponse(SuccessResponse):
    prober: Prober


class UpdateProberResponse(SuccessResponse):
    prober: Prober


class DeleteProberResponse(SuccessResponse):
    pass


class CreateProberConfigResponse(SuccessResponse):
    prober: Prober
    config: ProberConfig


class DeleteProberConfigResponse(SuccessResponse):
    pass


class UpdateProberConfigResponse(SuccessResponse):
    prober: Prober
    config: ProberConfig


class CreateProberFileResponse(SuccessResponse):
    prober: Prober
    file: ProberFile


class UpdateProberFileResponse(SuccessResponse):
    prober: Prober
    file: ProberFile


class ProberFileResponse(SuccessResponse):
    file: ProberFile


class CreateClusterVariableResponse(SuccessResponse):
    variable: ClusterVariable
    cluster: Cluster


class UpdateClusterVariableResponse(SuccessResponse):
    variable: ClusterVariable


class SyncDiffFile(BaseModel):
    old_content: bytes
    new_content: bytes


class AgentsConfigurationSyncDiff(SuccessResponse):
    unchanged_files: List[str]
    changed_files: Dict[str, SyncDiffFile]
