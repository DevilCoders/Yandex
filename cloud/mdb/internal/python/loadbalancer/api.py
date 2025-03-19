import functools
import time

from cloud.mdb.internal.python import grpcutil
from typing import Callable, Dict, NamedTuple, Optional, List, Tuple, Iterable
from .models import (
    NetworkLoadBalancer,
    TargetGroup,
    Target,
    Type,
    AttachedTargetGroup,
    ListenerSpec,
)

from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from dbaas_common import tracing
from yandex.cloud.priv.loadbalancer.v1 import (
    network_load_balancer_service_pb2,
    network_load_balancer_service_pb2_grpc,
    target_group_service_pb2,
    target_group_service_pb2_grpc,
    operation_service_pb2,
    operation_service_pb2_grpc,
)

from ..compute.models import OperationModel
from ..compute.pagination import ComputeResponse, paginate
from ..grpcutil import WrappedGRPCService


class LoadBalancerClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0
    page_size: int = 1000


class LoadBalancerClient:
    def __init__(
        self,
        config: LoadBalancerClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='LoadBalancerClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    @functools.cached_property
    def target_group_service(self):
        return ComputeGRPCService(
            self.logger,
            grpcutil.new_grpc_channel(self.config.transport),
            target_group_service_pb2_grpc.TargetGroupServiceStub,
            timeout=self.config.timeout,
            get_token=self.token_getter,
            error_handlers=self.error_handlers,
        )

    @functools.cached_property
    def network_load_balancer_service(self):
        return ComputeGRPCService(
            self.logger,
            grpcutil.new_grpc_channel(self.config.transport),
            network_load_balancer_service_pb2_grpc.NetworkLoadBalancerServiceStub,
            timeout=self.config.timeout,
            get_token=self.token_getter,
            error_handlers=self.error_handlers,
        )

    @functools.cached_property
    def operation_service(self):
        return WrappedGRPCService(
            self.logger,
            grpcutil.new_grpc_channel(self.config.transport),
            operation_service_pb2_grpc.OperationServiceStub,
            timeout=self.config.timeout,
            get_token=self.token_getter,
            error_handlers=self.error_handlers,
        )

    @client_retry
    @tracing.trace('Get network load balancer')
    def get_network_load_balancer(self, network_load_balancer_id: str) -> NetworkLoadBalancer:
        tracing.set_tag('network_load_balancer_id', network_load_balancer_id)
        request = network_load_balancer_service_pb2.GetNetworkLoadBalancerRequest(
            network_load_balancer_id=network_load_balancer_id,
        )
        return NetworkLoadBalancer.from_api(self.network_load_balancer_service.Get(request))

    @client_retry
    @tracing.trace('Get target group')
    def get_target_group(self, target_group_id: str) -> TargetGroup:
        tracing.set_tag('network_load_balancer_id', target_group_id)
        request = target_group_service_pb2.GetTargetGroupRequest(
            target_group_id=target_group_id,
        )
        return TargetGroup.from_api(self.target_group_service.Get(request))

    @tracing.trace('Wait operation')
    def wait_operation(self, operation_id: str, timeout: int = 900) -> OperationModel:
        deadline = time.time() + timeout
        while time.time() < deadline:
            operation = self.operation_service.Get(
                operation_service_pb2.GetOperationRequest(operation_id=operation_id),
                timeout=self.config.timeout,
            )
            if operation.done:
                return operation
            time.sleep(1)

    @client_retry
    @tracing.trace('List target groups clusters')
    def _list_target_groups(self, request: target_group_service_pb2.ListTargetGroupsRequest) -> Iterable:
        tracing.set_tag('folder_id', request.folder_id)
        response = self.target_group_service.List(request)
        return ComputeResponse(
            resources=map(TargetGroup.from_api, response.target_groups),
            next_page_token=response.next_page_token,
        )

    def list_target_groups(self, folder_id: str) -> Iterable[TargetGroup]:
        request = target_group_service_pb2.ListTargetGroupsRequest()
        request.folder_id = folder_id
        request.page_size = self.config.page_size
        return paginate(self._list_target_groups, request)

    @client_retry
    @tracing.trace('List network load balancers clusters')
    def _list_network_load_balancers(
        self, request: network_load_balancer_service_pb2.ListNetworkLoadBalancersRequest
    ) -> Iterable:
        tracing.set_tag('folder_id', request.folder_id)
        response = self.network_load_balancer_service.List(request)
        return ComputeResponse(
            resources=map(NetworkLoadBalancer.from_api, response.network_load_balancers),
            next_page_token=response.next_page_token,
        )

    def list_network_load_balancers(self, folder_id: str) -> Iterable[NetworkLoadBalancer]:
        request = network_load_balancer_service_pb2.ListNetworkLoadBalancersRequest()
        request.folder_id = folder_id
        request.page_size = self.config.page_size
        return paginate(self._list_network_load_balancers, request)

    @client_retry
    @tracing.trace('Update target group')
    def _target_group(
        self,
        target_group_id: str,
        name: Optional[str] = None,
        description: Optional[str] = None,
        targets: Optional[List[Target]] = None,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('target_group_id', target_group_id)

        request = target_group_service_pb2.UpdateTargetGroupRequest(
            target_group_id=target_group_id,
            name=name,
            description=description,
            targets=[Target.to_api(target) for target in targets],
        )

        response = self.target_group_service.Create(request, idempotency_key=idempotency_key)
        response_metadata = target_group_service_pb2.UpdateTargetGroupMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.target_group_id

    @client_retry
    @tracing.trace('Create network load balancer')
    def create_network_load_balancer(
        self,
        folder_id: str,
        region_id: str,
        type: Type,
        name: Optional[str] = None,
        description: Optional[str] = None,
        labels: Optional[Dict[str, str]] = None,
        listener_specs: Optional[List[ListenerSpec]] = (),
        attached_target_groups: Optional[List[AttachedTargetGroup]] = (),
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('folder_id', folder_id)

        request = network_load_balancer_service_pb2.CreateNetworkLoadBalancerRequest(
            folder_id=folder_id,
            name=name,
            labels=labels,
            region_id=region_id,
            description=description,
            type=Type.to_api(type),
            listener_specs=[ListenerSpec.to_api(listener_spec) for listener_spec in listener_specs],
            attached_target_groups=[
                AttachedTargetGroup.to_api(attached_target_group) for attached_target_group in attached_target_groups
            ],
        )

        response = self.network_load_balancer_service.Create(request, idempotency_key=idempotency_key)
        response_metadata = network_load_balancer_service_pb2.CreateNetworkLoadBalancerMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.network_load_balancer_id

    @client_retry
    @tracing.trace('Attach target group')
    def attach_target_group(
        self,
        network_load_balancer_id: str,
        attached_target_group: AttachedTargetGroup,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('network_load_balancer_id', network_load_balancer_id)

        request = network_load_balancer_service_pb2.AttachNetworkLoadBalancerTargetGroupRequest(
            network_load_balancer_id=network_load_balancer_id,
            attached_target_group=AttachedTargetGroup.to_api(attached_target_group),
        )

        response = self.network_load_balancer_service.AttachTargetGroup(request, idempotency_key=idempotency_key)
        response_metadata = network_load_balancer_service_pb2.AttachNetworkLoadBalancerTargetGroupMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.network_load_balancer_id

    @client_retry
    @tracing.trace('Delete network load balancer')
    def delete_network_load_balancer(
        self,
        network_load_balancer_id: str,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('network_load_balancer_id', network_load_balancer_id)

        request = network_load_balancer_service_pb2.DeleteNetworkLoadBalancerRequest(
            network_load_balancer_id=network_load_balancer_id,
        )

        response = self.network_load_balancer_service.Delete(request, idempotency_key=idempotency_key)
        response_metadata = network_load_balancer_service_pb2.DeleteNetworkLoadBalancerMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.network_load_balancer_id

    @client_retry
    @tracing.trace('Create target group')
    def create_target_group(
        self,
        folder_id: str,
        name: Optional[str] = None,
        description: Optional[str] = None,
        labels: Optional[Dict[str, str]] = None,
        region_id: Optional[str] = None,
        targets: Optional[List[Target]] = None,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('folder_id', folder_id)

        request = target_group_service_pb2.CreateTargetGroupRequest(
            folder_id=folder_id,
            name=name,
            labels=labels,
            region_id=region_id,
            description=description,
            targets=[Target.to_api(target) for target in targets],
        )

        response = self.target_group_service.Create(request, idempotency_key=idempotency_key)
        response_metadata = target_group_service_pb2.CreateTargetGroupMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.target_group_id

    @client_retry
    @tracing.trace('Delete target group')
    def delete_target_group(
        self,
        target_group_id: str,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('target_group_id', target_group_id)

        request = target_group_service_pb2.DeleteTargetGroupRequest(
            target_group_id=target_group_id,
        )

        response = self.target_group_service.Delete(request, idempotency_key=idempotency_key)
        response_metadata = target_group_service_pb2.DeleteTargetGroupMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.target_group_id
