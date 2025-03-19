from typing import Dict, Optional, Any, List

from pydantic import BaseModel, constr

from .common import ProberRunner, ClusterDeployPolicy


class CreateClusterRequest(BaseModel):
    manually_created: bool = True
    arcadia_path: str = ""

    recipe_id: int
    name: str
    slug: str
    variables: Dict[str, Any]
    deploy_policy: Optional[ClusterDeployPolicy] = None


class UpdateClusterRequest(BaseModel):
    manually_created: bool = True
    arcadia_path: str

    recipe_id: int
    name: str
    slug: str
    variables: Dict[str, Any]
    deploy_policy: Optional[ClusterDeployPolicy] = None


class CreateClusterRecipeRequest(BaseModel):
    manually_created: bool = True
    arcadia_path: str

    name: str
    description: str


class CopyClusterRecipeRequest(BaseModel):
    recipe_id: int


class UpdateClusterRecipeRequest(BaseModel):
    manually_created: bool = True
    arcadia_path: str

    name: str
    description: str


class CreateClusterRecipeFileRequest(BaseModel):
    relative_file_path: str
    force: bool = False


class UpdateClusterRecipeFileRequest(BaseModel):
    relative_file_path: str
    force: bool = False


class CreateProberRequest(BaseModel):
    manually_created: bool = True
    arcadia_path: str

    name: str
    slug: str
    description: str
    runner: ProberRunner


class CopyProberRequest(BaseModel):
    prober_id: int


class UpdateProberRequest(BaseModel):
    manually_created: bool = True
    arcadia_path: str

    name: str
    slug: str
    description: str
    runner: ProberRunner


class ProberConfigRequest(BaseModel):
    manually_created: bool = True

    is_prober_enabled: Optional[bool] = None
    interval_seconds: Optional[int] = None
    timeout_seconds: Optional[int] = None
    s3_logs_policy: Optional[str] = None
    default_routing_interface: Optional[constr(max_length=50)] = None
    dns_resolving_interface: Optional[constr(max_length=50)] = None

    matrix_variables: Optional[Dict[str, List[Any]]] = None
    variables: Optional[Dict[str, Any]] = None

    cluster_id: Optional[int] = None
    hosts_re: Optional[str] = None


class CreateProberConfigRequest(ProberConfigRequest):
    pass


class CopyOrMoveProberConfigsRequest(BaseModel):
    prober_id: int


class UpdateProberConfigRequest(ProberConfigRequest):
    pass


class CreateProberFileRequest(BaseModel):
    relative_file_path: str
    force: bool = False
    is_executable: bool = False


class UpdateProberFileRequest(BaseModel):
    relative_file_path: str
    force: bool = False
    is_executable: bool = False


class CreateClusterVariableRequest(BaseModel):
    name: str
    value: Any


class UpdateClusterVariableRequest(BaseModel):
    value: Any


class UpdateClusterVariableAsListOrSetRequest(UpdateClusterVariableRequest):
    pass


class UpdateClusterVariableDeleteFromListRequest(UpdateClusterVariableRequest):
    index: int


class UpdateClusterVariableAsMapRequest(UpdateClusterVariableRequest):
    key: str
