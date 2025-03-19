"""All core types"""

import dataclasses
import datetime
from enum import Enum
from typing import Dict, List, Optional, TypeVar

from bootstrap.common.rdbms.db import Db
from bootstrap.common.rdbms.exceptions import RecordNotFoundError

T = TypeVar("T")


@dataclasses.dataclass
class Lock:
    """Lock object (corresponds to record of <locks> table)"""
    id: Optional[int]
    owner: str
    description: str
    hb_timeout: int
    expired_at: datetime.datetime
    hosts: Optional[List[str]] = None

    def to_json(self) -> Dict:
        return {
            "id": self.id,
            "hosts": self.hosts,
            "owner": self.owner,
            "description": self.description,
            "hb_timeout": self.hb_timeout,
            "expired_at": self.expired_at.strftime("%Y-%m-%d %H:%M:%S"),
        }


@dataclasses.dataclass
class BaseInstance:
    """Host object (corresponds to record of <hosts> table)"""
    id: Optional[int]
    fqdn: str
    type: "EInstanceTypes"
    stand: Optional[str]

    def get_stand_id(self, db: Db) -> Optional[int]:
        """Convert stand name to stand_id"""
        # FIXME: EXTREMLY UNOPTIMAL (especially when processing a lot of instances)
        if self.stand and (not db.record_exists("stands", self.stand, column_name="name")):
            raise RecordNotFoundError("Stand <{}> is not found in database".format(self.stand))
        return db.get_id_by_name("stands", self.stand)

    def to_json(self) -> Dict:
        return {
            "id": self.id,
            "fqdn": self.fqdn,
            "type": self.type,
            "stand": self.stand,
        }


@dataclasses.dataclass
class LockedInstance:
    """LockedHost object (corresponds to record of <locked_hosts> table)"""
    instance_id: int
    lock_id: int


@dataclasses.dataclass
class Host(BaseInstance):
    """Hardware (walle) host"""
    dynamic_config: Optional[Dict] = None

    def to_json(self) -> Dict:
        result = super().to_json()
        result["dynamic_config"] = self.dynamic_config
        return result


@dataclasses.dataclass
class ServiceVm(BaseInstance):
    """Service virtual machine"""
    dynamic_config: Optional[Dict] = None

    def to_json(self) -> Dict:
        result = super().to_json()
        result["dynamic_config"] = self.dynamic_config
        return result


class InstanceType:
    def __init__(self, name: str, props_table: str, type: T):
        self.name = name
        self.props_table = props_table
        self.type = type

    @property
    def ui_name(self) -> str:
        return self.name.capitalize()

    @property
    def ui_name_plural(self) -> str:
        return "{}s".format(self.name.capitalize())

    def __str__(self) -> str:
        return self.name


class EInstanceTypes(Enum):
    HOST = InstanceType("host", "hw_props", Host)
    SVM = InstanceType("svm", "svm_props", ServiceVm)


@dataclasses.dataclass
class Stand:
    id: Optional[int]
    name: str

    def to_json(self) -> Dict:
        return {
            "id": self.id,
            "name": self.name,
        }


@dataclasses.dataclass
class InstanceGroup:
    id: Optional[int]
    name: str
    stand: str

    def get_stand_id(self, db: Db) -> Optional[int]:
        """Convert stand name to stand_id"""
        # FIXME: EXTREMLY UNOPTIMAL (especially when processing a lot of instances)
        if not db.record_exists("stands", self.stand, column_name="name"):
            raise RecordNotFoundError("Stand <{}> is not found in database".format(self.stand))
        return db.get_id_by_name("stands", self.stand)

    def to_json(self) -> Dict:
        return {
            "id": self.id,
            "name": self.name,
            "stand": self.stand,
        }


@dataclasses.dataclass
class InstanceGroupRelease:
    id: Optional[int]
    url: Optional[str]
    image_id: Optional[str]
    stand: str
    instance_group: str
    instance_group_id: int  # auxiliary attribute for updating of instance group release

    def to_json(self) -> Dict:
        return {
            "id": self.id,
            "url": self.url,
            "image_id": self.image_id,
            "stand": self.stand,
            "instance_group": self.instance_group,
        }


@dataclasses.dataclass
class StandClusterMapVersion:
    # FIXME: code duplication
    cluster_configs_version: Optional[int]
    yc_ci_version: Optional[str]
    bootstrap_templates_version: Optional[str]

    def to_json(self) -> Dict:
        return {
            "cluster_configs_version": self.cluster_configs_version,
            "yc_ci_version": self.yc_ci_version,
            "bootstrap_templates_version": self.bootstrap_templates_version,
        }


@dataclasses.dataclass
class StandClusterMap:
    id: Optional[int]
    stand: str
    grains: Optional[Dict]
    cluster_configs_version: Optional[int]
    yc_ci_version: Optional[str]
    bootstrap_templates_version: Optional[str]

    def get_stand_id(self, db: Db) -> Optional[int]:
        """Convert stand name to stand_id"""
        # FIXME: EXTREMLY UNOPTIMAL (especially when processing a lot of instances)
        if not db.record_exists("stands", self.stand, column_name="name"):
            raise RecordNotFoundError("Stand <{}> is not found in database".format(self.stand))
        return db.get_id_by_name("stands", self.stand)

    def to_json(self) -> Dict:
        return {
            "id": self.id,
            "stand": self.stand,
            "grains": self.grains,
            "cluster_configs_version": self.cluster_configs_version,
            "yc_ci_version": self.yc_ci_version,
            "bootstrap_templates_version": self.bootstrap_templates_version,
        }


@dataclasses.dataclass
class SaltRole:
    id: Optional[int]
    name: str

    def to_json(self) -> Dict:
        return {
            "id": self.id,
            "name": self.name,
        }


@dataclasses.dataclass
class InstanceSaltRolePackage:
    id: Optional[int]
    salt_role: str
    package_name: str
    target_version: str

    def to_json(self) -> Dict:
        return {
            "id": self.id,
            "salt_role": self.salt_role,
            "package_name": self.package_name,
            "target_version": self.target_version,
        }
