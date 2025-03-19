import abc
import json
import logging
import os
import pathlib
import subprocess
from typing import List, Optional, TypeVar, Literal, Union, Any, Dict, Tuple

from pydantic import BaseModel

from agent.process_cgroup import CgroupManager, Cgroup
from agent.process_namespace import NamespaceManager, MountNamespace
from database.models import UploadProberLogPolicy, ProberRunnerType
import settings

__all__ = ["Prober", "ProberConfig", "ProberFile", "Cluster", "ProberWithConfig", "AgentConfig",
           "ClusterMetadata", "BashProberRunner", "make_default_prober_config"]


class ProberFile(BaseModel):
    id: int
    relative_file_path: str
    is_executable: bool = False
    md5_hexdigest: Optional[str] = None


class AbstractProberRunner(BaseModel, abc.ABC):
    @abc.abstractmethod
    def create_process(self, prober: "Prober", config: "ProberConfig") -> subprocess.Popen:
        """
        Starts a new process for prober
        """
        pass


cgroup_manager = CgroupManager()
namespace_manager = NamespaceManager()


class BashProberRunner(AbstractProberRunner):
    type: Literal[ProberRunnerType.BASH] = ProberRunnerType.BASH

    command: Optional[str] = None

    def __str__(self):
        return self.command

    def create_process(self, prober: "Prober", config: "ProberConfig") -> subprocess.Popen:
        """
        Set the variable start_new_session to True so that the os.setsid() system call will be made
        in the child process prior to the execution of the subprocess.
        """
        cgroup = cgroup_manager.find_cgroup_by_prober_config(config)
        namespace = namespace_manager.find_namespace_by_prober_config(config)
        return subprocess.Popen(
            self.command,
            cwd=prober.files_location.as_posix(),
            shell=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            env=self._serialize_variables(config.variables or {}),
            executable="/bin/bash",
            start_new_session=True,
            preexec_fn=lambda: (self._put_process_into_cgroup(cgroup), self._put_process_into_namespace(namespace))
        )

    @staticmethod
    def _serialize_variables(variables: Dict[str, Any]) -> Dict[str, str]:
        env_variables = {}
        for name, value in variables.items():
            # this case need if we want to pass the string argument '-4' without quotation marks
            # in any case values without spaces we can pass without quotation marks
            if isinstance(value, str) and " " not in value:
                env_variables[f"VAR_{name}"] = value
            else:
                env_variables[f"VAR_{name}"] = json.dumps(value)
        return env_variables

    @staticmethod
    def _put_process_into_cgroup(cgroup: Optional[Cgroup]):
        """
        In some cases we have to run a new process in a cgroup.
        """
        if not cgroup:
            return

        try:
            pid = os.getpid()
            cgroup.add(pid)
        except Exception as e:
            logging.error(f"Error occurred on putting prober into cgroup: {e}", exc_info=e)
            raise

    @staticmethod
    def _put_process_into_namespace(namespace: Optional[MountNamespace]):
        if not namespace:
            return

        try:
            namespace.enter()
        except OSError as e:
            logging.error(f"Error occurred on entering into mount namespace: {e}", exc_info=e)
            raise


ProberRunner = Union[BashProberRunner]


class Prober(BaseModel):
    id: int
    name: str
    slug: str
    runner: ProberRunner

    files: List[ProberFile]

    files_location: Optional[pathlib.Path] = pathlib.Path(".")


T = TypeVar("T")


class ProberConfig(BaseModel):
    prober_id: int

    hosts_re: Optional[str]

    is_prober_enabled: Optional[bool]
    interval_seconds: Optional[int]
    timeout_seconds: Optional[int]
    s3_logs_policy: Optional[UploadProberLogPolicy]
    matrix_variables: Optional[Dict[str, List[Any]]]
    variables: Optional[Dict[str, Any]]
    default_routing_interface: Optional[str]
    dns_resolving_interface: Optional[str]

    additional_metric_labels: Optional[Dict[str, str]] = {}

    def update_with(self, other: "ProberConfig"):
        if self.prober_id != other.prober_id:
            raise ValueError("ProberConfig.update_with() works only with two configs for one prober.")

        self.is_prober_enabled = self._update_field(self.is_prober_enabled, other.is_prober_enabled)
        self.interval_seconds = self._update_field(self.interval_seconds, other.interval_seconds)
        self.timeout_seconds = self._update_field(self.timeout_seconds, other.timeout_seconds)
        self.s3_logs_policy = self._update_field(self.s3_logs_policy, other.s3_logs_policy)
        self.hosts_re = self._update_field(self.hosts_re, other.hosts_re)
        self.matrix_variables = self._update_field(self.matrix_variables, other.matrix_variables)
        self.variables = self._update_field(self.variables, other.variables, merge_dicts=True)
        self.default_routing_interface = self._update_field(self.default_routing_interface,
                                                            other.default_routing_interface)
        self.dns_resolving_interface = self._update_field(self.dns_resolving_interface,
                                                          other.dns_resolving_interface)

    @staticmethod
    def _update_field(field: Optional[T], other_field: Optional[T], merge_dicts: bool = False) -> Optional[T]:
        """
        Merges two optional values with priorities (other's is more priority).
        (10, 20) → 20
        (None, 20) → 20
        (10, None) → 10
        (None, None) → None

        If merge_dicts = True, merges two dicts also:
        ({"a": "a", "b": None}, {"b":"b", "c": "c"}) -> {"a": "a", "b": "b", "c": "c"}

        """
        if other_field is None:
            return field
        if merge_dicts and isinstance(other_field, dict) and isinstance(field, dict):
            return {**field, **other_field}
        return other_field


def make_default_prober_config(prober_id: int) -> ProberConfig:
    return ProberConfig(
        prober_id=prober_id,
        is_prober_enabled=False,
        interval_seconds=settings.DEFAULT_PROBER_TIMEOUT_SECONDS,
        timeout_seconds=settings.DEFAULT_PROBER_TIMEOUT_SECONDS,
        s3_logs_policy=UploadProberLogPolicy.FAIL,
    )


class Cluster(BaseModel):
    id: int
    name: str
    slug: str
    variables: Dict[str, Any] = {}
    prober_configs: List[ProberConfig]


class ClusterMetadata(BaseModel):
    id: int
    name: str
    slug: str

    def __init__(self, cluster: Cluster):
        super().__init__(id=cluster.id, name=cluster.name, slug=cluster.slug)


class ProberWithConfig(BaseModel):
    unique_key: Tuple
    prober: Prober
    config: ProberConfig


class AgentConfig(BaseModel):
    cluster: ClusterMetadata
    probers: List[ProberWithConfig]
