from types import SimpleNamespace
from typing import Any, TypedDict, Optional, NamedTuple


class Task(TypedDict):
    """
    Task dict that acquire_task query returns
    """

    task_id: str
    cid: str
    timeout: float
    task_type: str
    task_args: dict[str, Any]
    context: dict[str, Any]
    created_by: Optional[str]
    folder_id: str
    feature_flags: list[str]
    tracing: str
    cluster_status: str
    network_id: Optional[str]


class GenericHost(TypedDict):
    """
    Host dict that generic_resolve query returns
    """

    fqdn: str
    geo: str
    region_name: str
    space_limit: int
    subnet_id: Optional[str]
    platform_id: str
    cpu_guarantee: float
    cpu_limit: float
    gpu_limit: int
    memory_guarantee: int
    memory_limit: int
    network_guarantee: int
    network_limit: int
    io_limit: int
    io_cores_limit: int
    flavor: str
    cloud_provider_flavor_name: Optional[str]
    arch: str
    disk_type_id: str
    vtype: str
    vtype_id: Optional[str]
    assign_public_ip: bool
    subcid: str
    shard_id: Optional[str]
    shard_name: Optional[str]
    roles: list[str]
    environment: str
    host_group_ids: list[str]
    cloud_provider_disk_type: Optional[str]
    cluster_type: str
    instance_role_id: str


class HostGroup(NamedTuple):
    properties: Optional[SimpleNamespace]
    hosts: dict[str, GenericHost]
