import abc
import datetime
from typing import List, Optional, Union, Literal, Any

from pydantic import BaseModel, conint, Field

import database.models
from database.models import UploadProberLogPolicy, ProberRunnerType

__all__ = [
    "Cluster", "ClusterVariable", "ClusterRecipe", "ClusterRecipeFile",
    "Prober", "ProberFile", "ProberConfig", "ProberVariable", "ProberMatrixVariable",
    "BashProberRunner", "ProberRunner",
    "ManualClusterDeployPolicy", "RegularClusterDeployPolicy", "ClusterDeployPolicy"
]


class ClusterRecipeFile(BaseModel):
    id: int
    relative_file_path: str

    class Config:
        orm_mode = True


class ClusterRecipe(BaseModel):
    id: int

    manually_created: bool
    arcadia_path: str
    name: str
    description: str
    files: List[ClusterRecipeFile]

    class Config:
        orm_mode = True


class ClusterVariable(BaseModel):
    id: int

    name: str
    value: Any

    class Config:
        orm_mode = True


class BaseClusterDeployPolicy(BaseModel):
    parallelism: conint(gt=0) = 10
    plan_timeout: Optional[conint(gt=0)] = None
    apply_timeout: Optional[conint(gt=0)] = None

    class Config:
        orm_mode = True


class ManualClusterDeployPolicy(BaseClusterDeployPolicy):
    type: Literal[database.models.ClusterDeployPolicyType.MANUAL] = database.models.ClusterDeployPolicyType.MANUAL
    ship: bool = False

    class Config:
        orm_mode = True

    def create_database_object(self) -> database.models.Base:
        return database.models.ManualClusterDeployPolicy(
            parallelism=self.parallelism,
            type=self.type,
            ship=self.ship,
            plan_timeout=self.plan_timeout,
            apply_timeout=self.apply_timeout,
        )


class RegularClusterDeployPolicy(BaseClusterDeployPolicy):
    type: Literal[database.models.ClusterDeployPolicyType.REGULAR] = database.models.ClusterDeployPolicyType.REGULAR
    sleep_interval: int = 60  # seconds

    class Config:
        orm_mode = True

    def create_database_object(self) -> database.models.Base:
        return database.models.RegularClusterDeployPolicy(
            parallelism=self.parallelism,
            type=self.type,
            sleep_interval=self.sleep_interval,
            plan_timeout=self.plan_timeout,
            apply_timeout=self.apply_timeout,
        )


ClusterDeployPolicy = Union[ManualClusterDeployPolicy, RegularClusterDeployPolicy]


class Cluster(BaseModel):
    id: int

    name: str
    slug: str

    recipe: ClusterRecipe
    manually_created: bool
    arcadia_path: str
    variables: List[ClusterVariable]
    deploy_policy: Optional[ClusterDeployPolicy] = None
    # We add underscore to this field and display it as _last_deployment_finish_time because
    # we plan on supporting Cluster Deployment API and remove this field in the near future.
    last_deploy_attempt_finish_time: Optional[datetime.datetime] = Field(alias="_last_deployment_finish_time")

    class Config:
        orm_mode = True
        allow_population_by_field_name = True


class ProberVariable(BaseModel):
    id: int

    name: str
    value: Any

    class Config:
        orm_mode = True


class ProberMatrixVariable(BaseModel):
    id: int

    name: str
    values: List[Any]

    class Config:
        orm_mode = True


class ProberConfig(BaseModel):
    id: int

    manually_created: bool
    is_prober_enabled: Optional[bool]
    interval_seconds: Optional[int]
    timeout_seconds: Optional[int]
    s3_logs_policy: Optional[UploadProberLogPolicy]
    default_routing_interface: Optional[str]
    dns_resolving_interface: Optional[str]

    matrix_variables: Optional[List[ProberMatrixVariable]]
    variables: Optional[List[ProberVariable]]

    cluster_id: Optional[int]
    hosts_re: Optional[str]

    class Config:
        orm_mode = True


class ProberFile(BaseModel):
    id: int

    relative_file_path: str
    is_executable: bool = False
    md5_hexdigest: Optional[str] = None

    class Config:
        orm_mode = True


class AbstractProberRunner(BaseModel, abc.ABC):
    @abc.abstractmethod
    def create_database_object(self) -> database.models.Base:
        """
        Creates a new database object for this kind of ProberRunner
        """
        pass


class BashProberRunner(AbstractProberRunner):
    type: Literal[ProberRunnerType.BASH] = ProberRunnerType.BASH

    command: Optional[str] = None

    class Config:
        orm_mode = True

    def create_database_object(self) -> database.models.Base:
        return database.models.BashProberRunner(
            command=self.command,
        )

    def __str__(self):
        return self.command


ProberRunner = Union[BashProberRunner]


class Prober(BaseModel):
    id: int

    name: str
    slug: str
    description: str
    manually_created: bool = True
    arcadia_path: str
    runner: ProberRunner

    files: Optional[List[ProberFile]] = None
    configs: Optional[List[ProberConfig]] = None

    class Config:
        orm_mode = True
