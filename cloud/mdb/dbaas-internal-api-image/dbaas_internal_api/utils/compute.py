# -*- coding: utf-8 -*-
"""
Compute API
"""
# magic import, that allows forward references in class definitions
# it should be safe to remove this import in Python 3.11
from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Any, Dict, List, NamedTuple, Optional, Generator

from flask import current_app, g

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute import host_groups, host_type
from cloud.mdb.internal.python.compute import instances, images
from cloud.mdb.internal.python.grpcutil.exceptions import NotFoundError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from .logs import get_logger
from ..core.auth import check_action
from ..core.exceptions import DbaasClientError, PreconditionFailedError
from ..utils import iam_jwt
from ..utils.types import RequestedHostResources
from ..utils.request_context import get_x_request_id


class HostType(NamedTuple):
    id: str
    cores: int
    memory: int
    disks: int
    disk_size: int


class HostGroup(NamedTuple):
    id: str
    name: str
    folder_id: str
    zone_id: str
    host_type_id: str
    host_type: Optional[HostType]

    def with_host_type(self, ht: HostType) -> HostGroup:
        return HostGroup(
            id=self.id,
            name=self.name,
            folder_id=self.folder_id,
            zone_id=self.zone_id,
            host_type_id=self.host_type_id,
            host_type=ht,
        )


class ComputeProvider(ABC):
    """
    Abstract Compute Provider
    """

    @abstractmethod
    def get_host_group(self, host_group_id: str) -> HostGroup:
        """
        Get host group by id
        """

    @abstractmethod
    def get_host_type(self, host_type_id: str) -> HostType:
        """
        Get host type by id
        """

    @abstractmethod
    def get_instance(self, instance_id: str) -> instances.InstanceModel:
        """
        Get instance by id
        """

    @abstractmethod
    def get_image(self, image_id: str) -> images.ImageModel:
        """
        Get image by id
        """

    @abstractmethod
    def list_images(self, folder_id: str) -> Generator[images.ImageModel, None, None]:
        """
        List images by folder id
        """


class YCComputeProvider(ComputeProvider):
    """
    YC Compute Provider
    """

    def __init__(self, config) -> None:
        self.iam_jwt = None
        if not config.get('token'):
            self.iam_jwt = iam_jwt.get_provider()

        logger = MdbLoggerAdapter(
            get_logger(),
            extra={
                'request_id': get_x_request_id(),
            },
        )
        self._host_groups = host_groups.HostGroupsClient(
            config=host_groups.HostGroupsClientConfig(
                transport=grpcutil.Config(
                    url=config['url'],
                    cert_file=config['ca_path'],
                ),
                timeout=config.get('timeout', 5.0),
            ),
            logger=logger,
            token_getter=lambda: self.iam_jwt.get_iam_token() if self.iam_jwt is not None else config['token'],
            error_handlers={},
        )
        self._host_type = host_type.HostTypeClient(
            config=host_type.HostTypeClientConfig(
                transport=grpcutil.Config(
                    url=config['url'],
                    cert_file=config['ca_path'],
                ),
                timeout=config.get('timeout', 5.0),
            ),
            logger=logger,
            token_getter=lambda: self.iam_jwt.get_iam_token() if self.iam_jwt is not None else config['token'],
            error_handlers={},
        )
        self._instance_service = instances.InstancesClient(
            config=instances.InstancesClientConfig(
                transport=grpcutil.Config(
                    url=config['url'],
                    cert_file=config['ca_path'],
                ),
                timeout=config.get('timeout', 5.0),
            ),
            logger=logger,
            token_getter=lambda: self.iam_jwt.get_iam_token() if self.iam_jwt is not None else config['token'],
            error_handlers={},
        )
        self._image_service = images.ImagesClient(
            config=images.ImagesClientConfig(
                transport=grpcutil.Config(
                    url=config['url'],
                    cert_file=config['ca_path'],
                ),
                timeout=config.get('timeout', 5.0),
            ),
            logger=logger,
            token_getter=lambda: self.iam_jwt.get_iam_token() if self.iam_jwt is not None else config['token'],
            error_handlers={},
        )

    def get_host_group(self, host_group_id: str) -> HostGroup:
        """
        Get host group by id
        """
        hg = self._host_groups.get_host_group(host_group_id)
        return HostGroup(
            id=hg.id, name=hg.name, folder_id=hg.folder_id, zone_id=hg.zone_id, host_type_id=hg.type_id, host_type=None
        )

    def get_host_type(self, host_type_id: str) -> HostType:
        """
        Get host type by id
        """
        ht = self._host_type.get_host_type(host_type_id)
        return HostType(id=ht.id, cores=ht.cores, memory=ht.memory, disks=ht.disks, disk_size=ht.disk_size)

    def get_instance(self, instance_id: str) -> instances.InstanceModel:
        """
        Get instance by id
        """
        instance = self._instance_service.get_instance(instance_id)
        return instance

    def get_image(self, image_id: str) -> images.ImageModel:
        """
        Get image by id
        """
        image = self._image_service.get_image(image_id)
        return image

    def list_images(self, folder_id: str) -> Generator[images.ImageModel, None, None]:
        """
        List images by folder id
        """
        images = self._image_service.list_images(folder_id)
        return images


def get_provider() -> Optional[ComputeProvider]:
    """
    Get Compute provider according to config and flags
    """
    config = current_app.config['COMPUTE_GRPC']
    if not config.get('url'):
        return None
    return current_app.config['COMPUTE_PROVIDER'](config)


def validate_host_groups(
    host_group_ids: List[str],
    cluster_zone_ids: List[str],
    flavor: Optional[Dict[str, Any]] = None,
    resources: Optional[RequestedHostResources] = None,
) -> None:
    if not host_group_ids:
        return

    provider = get_provider()
    if not provider:
        raise DbaasClientError('This installation does not support host groups')

    groups = {}  # type: Dict[str, HostGroup]
    for host_group_id in host_group_ids:
        try:
            host_group = provider.get_host_group(host_group_id)
            try:
                ht = provider.get_host_type(host_group.host_type_id)
                host_group = host_group.with_host_type(ht)
            except NotFoundError:
                raise RuntimeError(f"Host type {host_group.host_type_id} not found")
            groups[host_group_id] = host_group
        except NotFoundError:
            raise PreconditionFailedError(f"Host group {host_group_id} not found")

    for host_group_id, host_group in groups.items():
        if host_group.folder_id != g.folder['folder_ext_id']:
            identity_provider = current_app.config['IDENTITY_PROVIDER'](current_app.config)
            cloud_ext_id = identity_provider.get_cloud_by_folder_ext_id(host_group.folder_id)

            if cloud_ext_id != g.cloud['cloud_ext_id']:
                raise PreconditionFailedError(
                    f"Host group {host_group_id} belongs to the cloud distinct from cluster's cloud"
                )
        check_action(action='compute.hostGroups.use', folder_ext_id=host_group.folder_id)

    host_group_zones = set()
    for host_group_id, host_group in groups.items():
        host_group_zones.add(host_group.zone_id)
    zones_without_host_group = set(cluster_zone_ids) - host_group_zones
    if zones_without_host_group:
        raise DbaasClientError(
            "The list of host groups must contain at least one host group for each of the availability zones"
            " where the cluster is hosted. The provided list of host groups does not contain host groups"
            f" in the following availability zones: {', '.join(zones_without_host_group)}"
        )

    for host_group_id, host_group in groups.items():
        assert host_group.host_type is not None
        if flavor is not None and host_group.host_type.cores < flavor['cpu_limit']:
            raise PreconditionFailedError(
                f"Cannot use host group {host_group_id} with this resource preset. "
                f"There are up to {host_group.host_type.cores} cores in host group {host_group_id}. "
                f"Required cores are {flavor['cpu_limit']}"
            )
        if flavor is not None and host_group.host_type.memory < flavor['memory_limit']:
            raise PreconditionFailedError(
                f"Cannot use host group {host_group_id} with this resource preset. "
                f"There are up to {host_group.host_type.memory} bytes of memory in host group {host_group_id}. "
                f"Required memory size is {flavor['memory_limit']}"
            )
        if (
            resources is not None
            and resources.disk_size is not None
            and host_group.host_type.disk_size < resources.disk_size
        ):
            raise PreconditionFailedError(
                f"Cannot use host group {host_group_id} with this resource preset. "
                f"There ate up to {host_group.host_type.disk_size} bytes of disk in host group {host_group_id}. "
                f"Required disk size is {resources.disk_size}"
            )
