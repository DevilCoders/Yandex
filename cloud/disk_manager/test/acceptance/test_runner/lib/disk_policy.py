from .errors import Error
from .helpers import create_ycp

from cloud.blockstore.pylibs.ycp import Ycp

from abc import ABC, abstractmethod
from contextlib import contextmanager
import logging
import re


class DiskPolicy(ABC):

    @abstractmethod
    def obtain():
        '''Obtain (get or create) disk'''


class YcpNewDiskPolicy(DiskPolicy):
    def __init__(self,
                 cluster_name: str,
                 zone_id: str,
                 ipc_type: str,
                 logger: logging.Logger,
                 name: str,
                 size: str,
                 blocksize: str,
                 type_id: str,
                 auto_delete: str) -> None:
        self._cluster_name = cluster_name
        self._zone_id = zone_id
        self._ipc_type = ipc_type
        self._logger = logger
        self._name = name
        self._size = size
        self._blocksize = blocksize
        self._type_id = type_id
        self._auto_delete = auto_delete

    @contextmanager
    def obtain(self) -> Ycp.Disk:
        with create_ycp(
                self._cluster_name,
                self._zone_id,
                self._ipc_type,
                self._logger).create_disk(
                    name=self._name,
                    type_id=self._type_id,
                    bs=self._blocksize,
                    size=self._size,
                    auto_delete=self._auto_delete) as disk:
            yield disk


class YcpFindDiskPolicy(DiskPolicy):
    def __init__(self,
                 cluster_name: str,
                 zone_id: str,
                 ipc_type: str,
                 logger: str,
                 name_regex: str) -> None:
        self._cluster_name = cluster_name
        self._zone_id = zone_id
        self._ipc_type = ipc_type
        self._logger = logger
        self._name_regex = name_regex

    def obtain(self) -> Ycp.Disk:
        disks = create_ycp(
            self._cluster_name,
            self._zone_id,
            self._ipc_type,
            self._logger).list_disks()
        for disk in disks:
            if re.match(self._name_regex, disk.name):
                return disk
        raise Error(f'Failed to find disk with name regex {self._name_regex}')
