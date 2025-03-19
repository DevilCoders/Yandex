from typing import List

from google.protobuf import field_mask_pb2
from yandex.cloud.priv.loadbalancer.v1alpha.inner import (
    loadbalancer_pb2 as lb_service,
    loadbalancer_pb2_grpc as lb_service_grpc,
)
from yandex.cloud.priv.loadbalancer.v1 import (
    network_load_balancer_service_pb2,
    network_load_balancer_service_pb2_grpc,
)
from yc_common import config, logging
from yc_common.clients.grpc.base import auth_metadata, BaseGrpcClient, get_client, GrpcEndpointConfig
from yc_common.clients.models.grpc.common import OperationListGrpc
from yc_common.clients.models.grpc.forwarding_group import ForwardingGroupGrpc, ForwardingGroupLbOptions, ForwardingRule
from yc_common.fields import IntType, StringType, ModelType, DictType
from yc_common.models import Model
from yc_common.clients.models import network_load_balancers as network_load_balancer_models

log = logging.get_logger(__name__)


class LoadBalancerEndpointConfig(GrpcEndpointConfig):
    pass


class EggressAclConfig(Model):
    name = StringType(required=True)
    ri_name = StringType(required=True)


class WellKnownRouteTargets(Model):
    external = StringType(required=True)
    internal = StringType(required=True)
    ipv6 = StringType(required=True)


class NetworkLoadBalancerConfig(Model):
    eggress_acl = ModelType(EggressAclConfig, required=True)
    lb_asn = IntType(required=True)
    # zone_id -> route tartets mapping.
    well_known_rts = DictType(ModelType(WellKnownRouteTargets))


class NetworkLoadBalancerClient(BaseGrpcClient):
    service_stub_cls = network_load_balancer_service_pb2_grpc.NetworkLoadBalancerServiceStub

    def get(self, network_load_balancer_id) -> network_load_balancer_models.NetworkLoadBalancerPublic:
        request = network_load_balancer_service_pb2.GetNetworkLoadBalancerRequest(network_load_balancer_id=network_load_balancer_id)
        return network_load_balancer_models.NetworkLoadBalancerPublic.from_grpc(self.reading_stub.Get(request))

    def list(self, folder_id, filter=None, page_size=None, page_token=None) -> network_load_balancer_models.NetworkLoadBalancerList:
        request = network_load_balancer_service_pb2.ListNetworkLoadBalancersRequest(
            folder_id=folder_id,
            filter=filter,
            page_size=page_size,
            page_token=page_token,
        )
        return network_load_balancer_models.NetworkLoadBalancerList.from_grpc(self.reading_stub.List(request))

    def create(
        self,
        folder_id,
        name=None,
        description=None,
        labels=None,
        region_id=None,
        type=None,
        listener_specs=None,
        attached_target_groups=None,
    ) -> network_load_balancer_models.NetworkLoadBalancerOperation:
        request = network_load_balancer_service_pb2.CreateNetworkLoadBalancerRequest(
            folder_id=folder_id,
            name=name,
            description=description,
            labels=labels,
            region_id=region_id,
            type=type,
            listener_specs=listener_specs,
            attached_target_groups=attached_target_groups,
        )
        return network_load_balancer_models.NetworkLoadBalancerOperation.from_grpc(self.writing_stub.Create(request))

    def update(
        self,
        network_load_balancer_id,
        update_mask,
        name=None,
        description=None,
        labels=None,
        listener_specs=None,
        attached_target_groups=None,
    ) -> network_load_balancer_models.NetworkLoadBalancerOperation:
        request = network_load_balancer_service_pb2.UpdateNetworkLoadBalancerRequest(
            network_load_balancer_id=network_load_balancer_id,
            update_mask=field_mask_pb2.FieldMask(paths=update_mask),
            name=name,
            description=description,
            labels=labels,
            listener_specs=listener_specs,
            attached_target_groups=attached_target_groups,
        )
        return network_load_balancer_models.NetworkLoadBalancerOperation.from_grpc(self.writing_stub.Update(request))

    def delete(self, network_load_balancer_id) -> network_load_balancer_models.NetworkLoadBalancerOperation:
        request = network_load_balancer_service_pb2.DeleteNetworkLoadBalancerRequest(network_load_balancer_id=network_load_balancer_id)
        return network_load_balancer_models.NetworkLoadBalancerOperation.from_grpc(self.writing_stub.Delete(request))

    def start(self, network_load_balancer_id) -> network_load_balancer_models.NetworkLoadBalancerOperation:
        request = network_load_balancer_service_pb2.StartNetworkLoadBalancerRequest(network_load_balancer_id=network_load_balancer_id)
        return network_load_balancer_models.NetworkLoadBalancerOperation.from_grpc(self.writing_stub.Start(request))

    def stop(self, network_load_balancer_id) -> network_load_balancer_models.NetworkLoadBalancerOperation:
        request = network_load_balancer_service_pb2.StopNetworkLoadBalancerRequest(network_load_balancer_id=network_load_balancer_id)
        return network_load_balancer_models.NetworkLoadBalancerOperation.from_grpc(self.writing_stub.Stop(request))

    def attach_target_group(self, network_load_balancer_id, attached_target_group) -> network_load_balancer_models.NetworkLoadBalancerAttachmentOperation:
        request = network_load_balancer_service_pb2.AttachTargetGroupRequest(
            network_load_balancer_id=network_load_balancer_id,
            attached_target_group=attached_target_group,
        )
        return network_load_balancer_models.NetworkLoadBalancerAttachmentOperation.from_grpc(self.writing_stub.AttachTargetGroup(request))

    def detach_target_group(self, network_load_balancer_id, target_group_id) -> network_load_balancer_models.NetworkLoadBalancerAttachmentOperation:
        request = network_load_balancer_service_pb2.DetachTargetGroupRequest(
            network_load_balancer_id=network_load_balancer_id,
            target_group_id=target_group_id,
        )
        return network_load_balancer_models.NetworkLoadBalancerAttachmentOperation.from_grpc(self.writing_stub.DetachTargetGroup(request))

    def get_target_states(self, network_load_balancer_id, target_group_id) -> network_load_balancer_models.NetworkLoadBalancerTargetStates:
        request = network_load_balancer_service_pb2.GetTargetStatesRequest(
            network_load_balancer_id=network_load_balancer_id,
            target_group_id=target_group_id,
        )
        return network_load_balancer_models.NetworkLoadBalancerTargetStates.from_grpc(self.reading_stub.GetTargetStates(request))

    def add_listener(self, network_load_balancer_id, listener_spec) -> network_load_balancer_models.NetworkLoadBalancerOperation:
        request = network_load_balancer_service_pb2.AddListenerRequest(
            network_load_balancer_id=network_load_balancer_id,
            listener_spec=listener_spec,
        )
        return network_load_balancer_models.NetworkLoadBalancerOperation.from_grpc(self.writing_stub.AddListener(request))

    def remove_listener(self, network_load_balancer_id, listener_name) -> network_load_balancer_models.NetworkLoadBalancerOperation:
        request = network_load_balancer_service_pb2.RemoveListenerRequest(
            network_load_balancer_id=network_load_balancer_id,
            listener_name=listener_name,
        )
        return network_load_balancer_models.NetworkLoadBalancerOperation.from_grpc(self.writing_stub.RemoveListener(request))

    def list_operations(self, network_load_balancer_id, page_size=None, page_token=None) -> OperationListGrpc:
        request = network_load_balancer_service_pb2.ListNetworkLoadBalancerOperationsRequest(
            network_load_balancer_id=network_load_balancer_id,
            page_size=page_size,
            page_token=page_token,
        )
        return OperationListGrpc.from_grpc(self.reading_stub.ListOperations(request))


class LoadBalancerCtrlClient(BaseGrpcClient):
    service_stub_cls = lb_service_grpc.LoadbalancerInternalServiceStub

    def get_forwarding_group(self, forwarding_group_id) -> ForwardingGroupGrpc:
        request = lb_service.GetForwardingGroupRequest(id=forwarding_group_id)
        forwarding_group_grpc = self.reading_stub.GetForwardingGroup(request)
        forwarding_group = ForwardingGroupGrpc.from_grpc(forwarding_group_grpc)
        return forwarding_group

    def get_forwarding_group_rules(self, forwarding_group_id) -> List[ForwardingRule]:
        request = lb_service.GetForwardingGroupRulesRequest(forwarding_group_id=forwarding_group_id)
        response = self.reading_stub.GetForwardingGroupRules(request)
        return [
            ForwardingRule.from_grpc(rule) for rule in response.forwarding_rules
        ]

    def replace_forwarding_group_rules(
        self, forwarding_group_id: str,
        forwarding_rules: List[ForwardingRule],
        health_check_group_id: str,
    ):
        grpc_forwarding_rules = [r.to_grpc() for r in forwarding_rules]
        request = lb_service.ReplaceForwardingGroupRulesRequest(
            forwarding_group_id=forwarding_group_id,
            forwarding_rules=grpc_forwarding_rules,
            lb_options=ForwardingGroupLbOptions.new(
                health_check_group_id=health_check_group_id,
            ).to_grpc(),
        )
        self.writing_stub.ReplaceForwardingGroupRules(request)

    def delete_forwarding_group(self, forwarding_group_id: str):
        request = lb_service.DeleteForwardingGroupRequest(id=forwarding_group_id)
        self.writing_stub.DeleteForwardingGroup(request)


def get_load_balancer_ctrl_client(metadata=None):
    return get_client(
        LoadBalancerCtrlClient,
        metadata=metadata,
        endpoint_config=config.get_value("endpoints.load_balancer_ctrl", model=LoadBalancerEndpointConfig),
    )


def get_network_load_balancer_client(iam_token) -> NetworkLoadBalancerClient:
    return get_client(
        NetworkLoadBalancerClient,
        metadata=auth_metadata(iam_token),
        endpoint_config=config.get_value("endpoints.loadbalancer", model=GrpcEndpointConfig),
    )
