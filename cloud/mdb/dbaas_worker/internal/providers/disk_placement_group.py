"""
YC.Disk Placement Groups interaction module
"""
import uuid
from typing import Callable, List

import time
from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.disk_placement_groups import (
    DiskPlacementGroupClient,
    DiskPlacementGroupClientConfig,
    DiskPlacementGroupModel,
)
from cloud.mdb.internal.python.compute.disk_placement_groups.errors import DuplicateDiskPlacementGroupNameError
from cloud.mdb.internal.python.compute.disks import DiskType
from cloud.mdb.internal.python.compute.models import OperationModel
from cloud.mdb.internal.python.compute.operations import (
    OperationsClient,
    OperationsClientConfig,
)
from dataclasses import dataclass
from dbaas_common import tracing

from .common import Change
from .http import BaseProvider
from .iam_jwt import IamJwt
from ..exceptions import UserExposedException, ExposedException


@dataclass
class DiskPlacementInfo:
    disk_placement_group_id: str


@dataclass
class NoopDiskPlacementInfo(DiskPlacementInfo):
    """
    Default placement is no placement at all.
    """

    disk_placement_group_id: None  # type: ignore

    def __init__(self, *args, **kwargs):
        super().__init__(disk_placement_group_id=None)


@dataclass
class AllocatedDiskPlacementGroupInfo(DiskPlacementInfo):
    pass


def gen_config(ca_path: str) -> Callable:
    def from_url(url: str) -> grpcutil.Config:
        return grpcutil.Config(
            url=url,
            cert_file=ca_path,
        )

    return from_url


# constraint from NBS
MAX_DISKS_IN_GROUP = 5


def name_for_dpg(cid: str, num: int, geo: str, subcid: str = None, shard_id: str = None):
    if shard_id is not None:
        return f'{cid}-{shard_id}-{geo}-{num}'
    else:
        return f'{cid}-{subcid}-{geo}-{num}'


class DiskPlacementGroupsError(ExposedException):
    """
    Base disk placement groups error unexposed error.
    """


class NoSuitableDiskPlacementGroupError(UserExposedException):
    """
    No group for specified conditions can be found.
    """


class OperationTimedOutError(UserExposedException):
    """
    Operation did not finish within timeout. Showing this to user.
    """


class TooManyDisksInGroup(UserExposedException):
    """
    A group cannot hold so many disks.
    """


class DiskPlacementGroupProvider(BaseProvider):
    """
    Disk Placement Group provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self._idempotence_ids = dict()
        iam_jwt = IamJwt(
            config,
            task,
            queue,
            service_account_id=self.config.compute.service_account_id,
            key_id=self.config.compute.key_id,
            private_key=self.config.compute.private_key,
        )
        transport_config = gen_config(self.config.compute.ca_path)
        error_handlers = {}
        self.__disk_placement_group_client = DiskPlacementGroupClient(
            config=DiskPlacementGroupClientConfig(
                transport=transport_config(self.config.compute.url),
            ),
            logger=self.logger,
            token_getter=iam_jwt.get_token,
            error_handlers=error_handlers,
        )
        self.__operations_client = OperationsClient(
            config=OperationsClientConfig(
                transport=transport_config(self.config.compute.url),
            ),
            logger=self.logger,
            token_getter=iam_jwt.get_token,
            error_handlers=error_handlers,
        )

    def _get_idempotence_id(self, key):
        """
        Get local idempotence id by key
        """
        if key not in self._idempotence_ids:
            self._idempotence_ids[key] = str(uuid.uuid4())
        return self._idempotence_ids[key]

    def _disk_placement_group_exists(
        self,
        folder_id: str,
        cid: str,
        az: str,
        subcid: str = None,
        shard_id: str = None,
        pg_num: int = None,
        max_disks_at_once: int = 1,
    ) -> DiskPlacementGroupModel:
        """
        Get or create a disk placement group with an empty slot for new disk.
        """
        if max_disks_at_once > MAX_DISKS_IN_GROUP:
            raise TooManyDisksInGroup(
                message=f'A disk placement group cannot hold {max_disks_at_once} disks, which you asked'
            )
        start_from = pg_num or 0
        max_elem = pg_num or 100
        for num in range(start_from, max_elem + 1):
            try:
                return self._disk_placement_group_created(
                    num=num, folder_id=folder_id, cid=cid, az=az, subcid=subcid, shard_id=shard_id
                )
            except DuplicateDiskPlacementGroupNameError:
                group = self.__disk_placement_group_client.get_by_name(
                    folder_id=folder_id,
                    name=name_for_dpg(cid, num, az, subcid=subcid, shard_id=shard_id),
                )[0]
                disk_ids = self.ids_of_disks_in_group(group.id)
                if len(disk_ids) + max_disks_at_once <= MAX_DISKS_IN_GROUP:
                    self.logger.debug('Found suitable existing disk placement group id=%s', group.id)
                    return group
        raise NoSuitableDiskPlacementGroupError(
            message='could not find suitable disk placement group',
        )

    def _disk_placement_group_created(
        self, num: int, folder_id: str, cid: str, az: str, subcid: str = None, shard_id: str = None
    ) -> DiskPlacementGroupModel:
        name = name_for_dpg(cid, num, az, subcid=subcid, shard_id=shard_id)
        context_key = f'disk_placement_group.{name}.create'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(context_key, 'create disk placement group initiated', rollback=Change.noop_rollback))
            operation_id = operation_from_context
        else:
            operation = self.__disk_placement_group_client.create_disk_placement_group(
                self._get_idempotence_id(f'create.disk_placement_group.{name}'),
                name,
                folder_id=folder_id,
                zone_id=az,
            )
            operation_id = operation.operation_id
            self.add_change(
                Change(
                    context_key,
                    'create disk placement group initiated',
                    context={context_key: operation_id},
                    rollback=Change.noop_rollback,
                )
            )
        return self.operation_wait_disk_placement_group(operation_id)

    def operation_wait_disk_placement_group(self, operation_id: str) -> DiskPlacementGroupModel:
        return self.operation_wait(operation_id, timeout=60).parse_response(DiskPlacementGroupModel).response

    @tracing.trace('Disk Placement Group Operation Wait')
    def operation_wait(self, operation_id, timeout=600, stop_time=None) -> OperationModel:
        """
        Wait while operation finishes.
        """
        tracing.set_tag('compute.operation.id', operation_id)
        if stop_time is None:
            stop_time = time.time() + timeout
        with self.interruptable:
            while time.time() < stop_time:
                operation = self._get_operation(operation_id)
                if not operation.done:
                    self.logger.info('Waiting for operation %s', operation_id)
                    time.sleep(1)
                    continue
                if not operation.error:
                    return operation
                raise DiskPlacementGroupsError(
                    message=operation.error.message, err_type=operation.error.err_type, code=operation.error.code
                )

            msg = '{timeout}s passed. Compute operation {id} is still running'
            raise OperationTimedOutError(msg.format(timeout=timeout, id=operation_id))

    @tracing.trace('Compute Operation Get', ignore_active_span=True)
    def _get_operation(self, operation_id) -> OperationModel:
        """
        Get operation by id
        """
        return self.__operations_client.get_operation(operation_id)

    def ids_of_disks_in_group(self, group_id: str) -> List[str]:
        disks = self.__disk_placement_group_client.list_disks(group_id)
        return [disk.id for disk in disks]

    def absent(self, group_id: str):
        """
        Ensure Disk Placement Group is deleted.
        """
        context_key = f'disk_placement_group.{group_id}.delete'
        created_context = self.context_get(context_key)
        if created_context:
            self.add_change(
                Change(
                    context_key,
                    'delete disk placement group initiated',
                    rollback=Change.noop_rollback,
                ),
            )
            operation_id = created_context
        else:
            operation = self.__disk_placement_group_client.delete_disk_placement_group(
                self._get_idempotence_id(f'delete.disk_placement_group.{group_id}'),
                group_id,
            )
            operation_id = operation.operation_id
            self.add_change(
                Change(
                    context_key,
                    'delete disk placement group initiated',
                    context={context_key: operation_id},
                    rollback=Change.noop_rollback,
                )
            )
        self.operation_wait(operation_id)

    def get_placement_info(
        self,
        disk_type: DiskType,
        folder_id: str,
        geo: str,
        cid: str,
        subcid: str = None,
        shard_id: str = None,
        pg_num: int = None,
    ) -> DiskPlacementInfo:
        """
        Prepare a placement group for next disk.
        """
        if disk_type == DiskType.network_ssd_nonreplicated:
            group = self._disk_placement_group_exists(
                folder_id=folder_id,
                cid=cid,
                az=geo,
                shard_id=shard_id,
                subcid=subcid,
                pg_num=pg_num,
            )
            placement_info = AllocatedDiskPlacementGroupInfo(disk_placement_group_id=group.id)
            return placement_info
        return NoopDiskPlacementInfo()
