"""
Placement Groups interaction module
"""
import uuid
import time
from typing import Callable, List
from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.grpcutil import exceptions as grpcutil_errors
from cloud.mdb.internal.python.compute.placement_groups import (
    PlacementGroupClient,
    PlacementGroupClientConfig,
    PlacementGroupModel,
)
from cloud.mdb.internal.python.compute.placement_groups.errors import DuplicatePlacementGroupNameError
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
from .metadb_placement_group import MetadbPlacementGroup


MAX_VMS_IN_PG = 100


@dataclass
class PlacementInfo:
    placement_group_id: str


@dataclass
class AllocatedPlacementGroupInfo(PlacementInfo):
    pass


def gen_config(ca_path: str) -> Callable:
    def from_url(url: str) -> grpcutil.Config:
        return grpcutil.Config(
            url=url,
            cert_file=ca_path,
        )

    return from_url


def name_for_pg(cid: str, shard_id: str = None, subcid: str = None, local_id: int = None):
    if local_id is not None:
        return f'{cid}-{local_id}'
    elif shard_id is not None:
        return f'{cid}-{shard_id}'
    else:
        return f'{cid}'


class PlacementGroupsError(ExposedException):
    """
    Base placement groups error unexposed error.
    """


class OperationTimedOutError(UserExposedException):
    """
    Operation did not finish within timeout. Showing this to user.
    """


class NoSuitablePlacementGroupError(UserExposedException):
    """
    No group for specified conditions can be found.
    """


class TooManyVmsInGroup(UserExposedException):
    """
    A group cannot hold so many vms.
    """


@dataclass
class NoopPlacementInfo(PlacementInfo):
    """
    Default placement is no placement at all.
    """

    placement_group_id: None  # type: ignore

    def __init__(self, *args, **kwargs):
        super().__init__(placement_group_id=None)


class PlacementGroupProvider(BaseProvider):
    """
    Placement Group provider
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
        self.__placement_group_client = PlacementGroupClient(
            config=PlacementGroupClientConfig(
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
        self.metadb_placement_group = MetadbPlacementGroup(config, task, queue)

    def _get_idempotence_id(self, key):
        """
        Get local idempotence id by key
        """
        if key not in self._idempotence_ids:
            self._idempotence_ids[key] = str(uuid.uuid4())
        return self._idempotence_ids[key]

    def _placement_group_exists(
        self,
        folder_id: str,
        cid: str,
        subcid: str = None,
        shard_id: str = None,
        max_vms_at_once: int = 1,
        best_effort: bool = False,
        local_id: int = None,
    ) -> PlacementGroupModel:
        """
        Get or create a placement group.
        """
        if max_vms_at_once > MAX_VMS_IN_PG:
            raise TooManyVmsInGroup(message=f'A placement group cannot hold {max_vms_at_once} vms, which you asked')

        try:
            return self._placement_group_created(
                folder_id=folder_id,
                cid=cid,
                subcid=subcid,
                shard_id=shard_id,
                best_effort=best_effort,
                local_id=local_id,
            )
        except DuplicatePlacementGroupNameError:
            group = self.__placement_group_client.get_by_name(
                folder_id=folder_id, name=name_for_pg(cid, subcid=subcid, shard_id=shard_id, local_id=local_id)
            )[0]
            vm_ids = self.ids_of_vms_in_group(group.id)
            if len(vm_ids) + max_vms_at_once <= MAX_VMS_IN_PG:
                self.logger.debug('Found suitable existing placement group id=%s', group.id)
                return group

        raise NoSuitablePlacementGroupError(
            message='Could not find suitable placement group',
        )

    def _placement_group_created(
        self,
        folder_id: str,
        cid: str,
        subcid: str = None,
        shard_id: str = None,
        best_effort: bool = False,
        local_id: int = None,
    ) -> PlacementGroupModel:
        name = name_for_pg(cid, subcid=subcid, shard_id=shard_id, local_id=local_id)
        context_key = f'placement_group.{name}.create'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(context_key, 'create placement group initiated', rollback=Change.noop_rollback))
            operation_id = operation_from_context
        else:
            operation = self.__placement_group_client.create_placement_group(
                self._get_idempotence_id(f'create.placement_group.{name}'),
                name,
                folder_id=folder_id,
                best_effort=best_effort,
            )
            operation_id = operation.operation_id
            self.add_change(
                Change(
                    context_key,
                    'create placement group initiated',
                    context={context_key: operation_id},
                    rollback=Change.noop_rollback,
                )
            )
        return self.operation_wait_placement_group(operation_id)

    def operation_wait_placement_group(self, operation_id: str) -> PlacementGroupModel:
        return self.operation_wait(operation_id, timeout=60).parse_response(PlacementGroupModel).response

    @tracing.trace('Placement Group Operation Wait')
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
                raise PlacementGroupsError(
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

    def ids_of_vms_in_group(self, group_id: str) -> List[str]:
        vms = self.__placement_group_client.list_vms(group_id)
        return [vm.id for vm in vms]

    def absent(self, group_id: str):
        """
        Ensure Placement Group is deleted.
        """
        context_key = f'placement_group.{group_id}.delete'
        created_context = self.context_get(context_key)
        if created_context:
            self.add_change(
                Change(
                    context_key,
                    'delete placement group initiated',
                    rollback=Change.noop_rollback,
                ),
            )
            operation_id = created_context
        else:
            try:
                placement_group = self.__placement_group_client.get_placement_group(group_id)
            except grpcutil_errors.NotFoundError as e:
                self.logger.info('Unable to get placement group by id %s: %s', group_id, repr(e))
                return
            if placement_group:
                operation = self.__placement_group_client.delete_placement_group(
                    self._get_idempotence_id(f'delete.placement_group.{group_id}'),
                    group_id,
                )
                operation_id = operation.operation_id
                self.add_change(
                    Change(
                        context_key,
                        'delete placement group initiated',
                        context={context_key: operation_id},
                        rollback=Change.noop_rollback,
                    )
                )
        self.operation_wait(operation_id)

    def get_placement_group_info(
        self,
        folder_id: str,
        cid: str,
        subcid: str = None,
        shard_id: str = None,
        local_id: int = None,
        best_effort: bool = False,
    ) -> PlacementInfo:
        """
        Prepare a placement group for next vm.
        """
        if local_id is not None:
            group = self._placement_group_exists(
                folder_id=folder_id,
                cid=cid,
                local_id=local_id,
                shard_id=shard_id,
                subcid=subcid,
                best_effort=best_effort,
            )
            placement_info = AllocatedPlacementGroupInfo(placement_group_id=group.id)
            return placement_info
        return NoopPlacementInfo()

    def remove_placement_group(self, host_pgid):
        """
        Remove placement group
        """
        for host, placement_group_id in host_pgid.items():
            tracing.set_tag('cluster.cid.placement_group_id', placement_group_id)
            tracing.set_tag('cluster.cid.host', host)
            self.absent(placement_group_id)
            self.metadb_placement_group.update(placement_group_id, 'DELETED', host)
