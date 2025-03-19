from .errors import Error
from .helpers import create_ycp, wait_for_block_device_to_appear

from cloud.blockstore.pylibs.ycp import Ycp

from contextlib import contextmanager
import logging


class YcpNewInstancePolicy:
    def __init__(self,
                 cluster_name: str,
                 zone_id: str,
                 ipc_type: str,
                 logger: logging.Logger,
                 name: str,
                 core_count: int,
                 ram_count: int,
                 compute_node: str,
                 placement_group: str,
                 platform_ids: list[str],
                 auto_delete: bool) -> None:
        self._cluster_name = cluster_name
        self._zone_id = zone_id
        self._ipc_type = ipc_type
        self._logger = logger
        self._name = name
        self._core_count = core_count
        self._ram_count = ram_count
        self._compute_node = compute_node
        self._placement_group = placement_group
        self._platform_ids = platform_ids
        self._auto_delete = auto_delete

    @contextmanager
    def obtain(self) -> Ycp.Instance:
        self._instance = None
        for platform_id in self._platform_ids:
            try:
                with create_ycp(
                    self._cluster_name,
                    self._zone_id,
                    self._ipc_type,
                    self._logger).create_instance(
                        name=self._name,
                        cores=self._core_count,
                        memory=self._ram_count,
                        compute_node=self._compute_node,
                        placement_group_name=self._placement_group,
                        platform_id=platform_id,
                        auto_delete=False) as instance:
                    self._instance = instance
                break
            except Exception as e:
                self._logger.info(f'Cannot create VM on platform'
                                  f' {platform_id}')
                self._logger.info(f'Error: {e}')
        if not self._instance:
            raise Error(f'Cannot create VM on any platform in'
                        f' {self._platform_ids}')
        try:
            yield self._instance
        finally:
            if self._auto_delete:
                create_ycp(
                    self._cluster_name,
                    self._zone_id,
                    self._ipc_type,
                    self._logger).delete_instance(self._instance)

    @contextmanager
    def attach_disk(self, disk: Ycp.Disk, block_device: str) -> None:
        with create_ycp(self._cluster_name,
                        self._zone_id,
                        self._ipc_type,
                        self._logger).attach_disk(self._instance, disk):
            wait_for_block_device_to_appear(self._instance.ip, block_device)
            yield
