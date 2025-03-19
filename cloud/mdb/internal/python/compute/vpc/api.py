"""
VPC provider
"""

from typing import Iterable, NamedTuple, Callable, Union, List
import logging
import uuid

import grpc

from google.protobuf import field_mask_pb2

from cloud.mdb.internal.python import grpcutil
from dbaas_common import retry, tracing
from yandex.cloud.priv.vpc.v1 import (
    network_service_pb2,
    network_service_pb2_grpc,
    operation_service_pb2,
    operation_service_pb2_grpc,
    security_group_pb2,
    security_group_service_pb2,
    security_group_service_pb2_grpc,
    subnet_service_pb2,
    subnet_service_pb2_grpc,
)
from yandex.cloud.priv.vpc.v1.inner import (
    feature_flag_service_pb2,
    feature_flag_service_pb2_grpc,
)
from yandex.cloud.priv.servicecontrol.v1.resource_pb2 import Resource

from .models import Network, SecurityGroup, SecurityGroupRule, Subnet


class Config(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 5.0
    page_size: int = 1000


class VPCError(RuntimeError):
    """
    Base VPC error
    """


class VPCWaitRepeat(VPCError):
    """
    Retry waiting operation
    """


class NetworkNotFoundError(VPCError):
    """
    Network not found
    """


class SubnetNotFoundError(VPCError):
    """
    Subnet not found
    """


class CreateSecurityGroupError(VPCError):
    """
    Create Security Group
    """


class DeleteSecurityGroupError(VPCError):
    """
    Delete Security Group
    """


class UpdateSecurityGroupError(VPCError):
    """
    Update Security Group
    """


class SecurityGroupNotFoundError(VPCError):
    """
    SG not found
    """


class FeatureFlagNotFoundError(VPCError):
    """
    Feature flag not found
    """


class SetFeatureFlagError(VPCError):
    """
    Set Feature Flag
    """


_retry_vpc = retry.on_exception(
    (
        grpc.RpcError,
        VPCWaitRepeat,
    ),
    factor=10,
    max_wait=60,
    max_tries=6,
)


def _subnet_from_grpc(subnet) -> Subnet:
    return Subnet(
        subnet_id=subnet.id,
        network_id=subnet.network_id,
        folder_id=subnet.folder_id,
        zone_id=subnet.zone_id,
        v4_cidr_blocks=subnet.v4_cidr_blocks,
        v6_cidr_blocks=subnet.v6_cidr_blocks,
        egress_nat_enable=subnet.egress_nat_enable,
    )


def _sg_rules_from_grpc(sgrules) -> List[SecurityGroupRule]:
    res = []
    for rule in sgrules:
        ports_from, ports_to = rule.ports.from_port, rule.ports.to_port
        if rule.ports == security_group_pb2.PortRange():
            # Do not set zero-values if PortRange is empty
            ports_from, ports_to = None, None
        target = rule.WhichOneof('target')
        res.append(
            SecurityGroupRule(
                id=rule.id,
                description=rule.description,
                direction=rule.direction,
                ports_from=ports_from,
                ports_to=ports_to,
                protocol_name=rule.protocol_name,
                protocol_number=rule.protocol_number,
                v4_cidr_blocks=rule.cidr_blocks.v4_cidr_blocks,
                v6_cidr_blocks=rule.cidr_blocks.v6_cidr_blocks,
                predefined_target=rule.predefined_target if target == 'predefined_target' else None,
                security_group_id=rule.security_group_id if target == 'security_group_id' else None,
            )
        )
    return res


def _sg_from_grpc(sg) -> SecurityGroup:
    return SecurityGroup(
        id=sg.id,
        network_id=sg.network_id,
        folder_id=sg.folder_id,
        name=sg.name,
        default_for_network=sg.default_for_network,
        rules=_sg_rules_from_grpc(sg.rules),
    )


def _is_already_exist(rpc_error) -> bool:
    if hasattr(rpc_error, 'code') and callable(rpc_error.code):
        return rpc_error.code() == grpc.StatusCode.ALREADY_EXISTS
    return False


def _is_not_found(rpc_error) -> bool:
    if hasattr(rpc_error, 'code') and callable(rpc_error.code):
        return rpc_error.code() == grpc.StatusCode.NOT_FOUND
    return False


class VPC:
    """
    VPC provider
    """

    def __init__(
        self, config: Config, logger: Union[logging.LoggerAdapter, logging.Logger], token_getter: Callable[[], str]
    ) -> None:
        self.config = config
        self.logger = logger
        self.token_getter = token_getter

        self.logger.info('init VPC internal provider channel with transport: %s', repr(config.transport))
        channel = grpcutil.new_grpc_channel(config.transport)
        self.network_stub = network_service_pb2_grpc.NetworkServiceStub(channel)
        self.subnet_stub = subnet_service_pb2_grpc.SubnetServiceStub(channel)
        self.security_group_stub = security_group_service_pb2_grpc.SecurityGroupServiceStub(channel)
        self.operation_stub = operation_service_pb2_grpc.OperationServiceStub(channel)
        self.feature_flag_stub = feature_flag_service_pb2_grpc.FeatureFlagServiceStub(channel)

    def _get_metadata(self, request_id):
        return [
            ('x-request-id', request_id),
            ('authorization', 'Bearer %s' % self.token_getter()),
        ]

    @_retry_vpc
    @tracing.trace('VPC Get Subnets page')
    def _get_subnets_page(self, network_id: str, page_token: str):
        request_id = str(uuid.uuid4())
        tracing.set_tag('vpc.network.id', network_id)
        tracing.set_tag('request_id', request_id)
        request = network_service_pb2.ListNetworkSubnetsRequest(
            network_id=network_id,
            page_size=self.config.page_size,
        )
        if page_token:
            request.page_token = page_token

        self.logger.info('Requesting %s subnets [page: %r] in %s', network_id, page_token, self.config.transport.url)
        try:
            return self.network_stub.ListSubnets(
                request, metadata=self._get_metadata(request_id), timeout=self.config.timeout
            )
        except grpc.RpcError as rpc_error:
            if _is_not_found(rpc_error):
                self.logger.info('Network %s not found', network_id)
                raise NetworkNotFoundError(f'Network {network_id} not found')
            self.logger.exception('Failed to list network subnets. x-request-id: %s', request_id)
            raise

    def get_subnets(self, network_id: str) -> Iterable[Subnet]:
        """
        Get network subnets
        """
        page_token = ''
        while True:
            resp = self._get_subnets_page(network_id, page_token)
            page_token = resp.next_page_token
            for subnet in resp.subnets:
                yield _subnet_from_grpc(subnet)
            if not page_token:
                break

    @_retry_vpc
    @tracing.trace('VPC Get Network')
    def get_network(self, network_id: str) -> Network:
        """
        Get network by its id
        """
        request_id = str(uuid.uuid4())
        tracing.set_tag('vpc.network.id', network_id)
        tracing.set_tag('request_id', request_id)
        self.logger.info('Requesting %s network in %s', network_id, self.config.transport.url)
        try:
            res = self.network_stub.Get(
                network_service_pb2.GetNetworkRequest(network_id=network_id),
                metadata=self._get_metadata(request_id),
                timeout=self.config.timeout,
            )
        except grpc.RpcError as rpc_error:
            if _is_not_found(rpc_error):
                self.logger.info('Network %s not found', network_id)
                raise NetworkNotFoundError(f'Network {network_id} not found')
            self.logger.exception('Failed to get network. x-request-id: %s', request_id)
            raise

        return Network(
            network_id=res.id,
            folder_id=res.folder_id,
            default_security_group_id=res.default_security_group_id,
        )

    @_retry_vpc
    @tracing.trace('VPC Get Networks page')
    def _get_networks_page(self, folder_id: str, page_token: str):
        """
        Paginate networks
        """
        request_id = str(uuid.uuid4())
        tracing.set_tag('vpc.folder_id', folder_id)
        tracing.set_tag('request_id', request_id)

        request = network_service_pb2.ListNetworksRequest(folder_id=folder_id, page_size=self.config.page_size)
        if page_token:
            request.page_token = page_token

        self.logger.info(
            'Requesting networks in folder %s [page: %r] in %s', folder_id, page_token, self.config.transport.url
        )
        try:
            return self.network_stub.List(request, metadata=self._get_metadata(request_id), timeout=self.config.timeout)
        except grpc.RpcError as rpc_error:
            if _is_not_found(rpc_error):
                self.logger.info('Networks in folder %s are not found', folder_id)
                raise NetworkNotFoundError(f'Networks in folder {folder_id} are not found')
            self.logger.exception('Failed to get network. x-request-id: %s', request_id)
            raise

    @_retry_vpc
    @tracing.trace('VPC Get Networks')
    def get_networks(self, folder_id: str) -> List[Network]:
        """
        Get networks by folder id
        """
        page_token = ''
        while True:
            resp = self._get_networks_page(folder_id, page_token)
            page_token = resp.next_page_token
            for network in resp.networks:
                yield Network(
                    network_id=network.id,
                    folder_id=network.folder_id,
                    default_security_group_id=network.default_security_group_id,
                )
            if not page_token:
                break

    @_retry_vpc
    @tracing.trace('VPC Get Subnet')
    def get_subnet(self, subnet_id: str) -> Subnet:
        """
        Get subnet by its id
        """
        request_id = str(uuid.uuid4())
        tracing.set_tag('vpc.subnet.id', subnet_id)
        tracing.set_tag('request_id', request_id)
        self.logger.info('Requesting %s subnet in %s', subnet_id, self.config.transport.url)
        try:
            return _subnet_from_grpc(
                self.subnet_stub.Get(
                    subnet_service_pb2.GetSubnetRequest(subnet_id=subnet_id),
                    metadata=self._get_metadata(request_id),
                    timeout=self.config.timeout,
                )
            )
        except grpc.RpcError as rpc_error:
            if _is_not_found(rpc_error):
                raise SubnetNotFoundError(f'Subnet {subnet_id} not found')
            self.logger.exception('Failed to get subnet. x-request-id: %s', request_id)
            raise

    @_retry_vpc
    @tracing.trace('VPC Get Security Group')
    def get_security_group(self, security_group_id: str) -> SecurityGroup:
        """
        Get security group by its id
        """
        request_id = str(uuid.uuid4())
        tracing.set_tag('vpc.security_group.id', security_group_id)
        tracing.set_tag('request_id', request_id)
        self.logger.info('Requesting %s security group in %s', security_group_id, self.config.transport.url)
        try:
            return _sg_from_grpc(
                self.security_group_stub.Get(
                    security_group_service_pb2.GetSecurityGroupRequest(security_group_id=security_group_id),
                    metadata=self._get_metadata(request_id),
                    timeout=self.config.timeout,
                )
            )
        except grpc.RpcError as rpc_error:
            if _is_not_found(rpc_error):
                raise SecurityGroupNotFoundError(f'Security group {security_group_id} not found')
            self.logger.exception('Failed to get security group. x-request-id: %s', request_id)
            raise

    @_retry_vpc
    @tracing.trace('VPC Find Security Group')
    def find_security_group(self, name: str, folder_id: str) -> SecurityGroup:
        """
        VPC Find Security Group by name and network
        """
        request_id = str(uuid.uuid4())
        req = security_group_service_pb2.ListSecurityGroupsRequest(
            folder_id=folder_id,
            filter=f'name="{name}"',
            page_size=100,
        )
        self.logger.info(f'{request_id} find security group name {name}, folder_id {folder_id}')
        resp = self.security_group_stub.List(
            req,
            metadata=self._get_metadata(request_id),
            timeout=self.config.timeout,
        )
        for sg in resp.security_groups:
            if sg.name != name:
                raise VPCError(
                    f'unexpect api result, request {request_id} filter security group {name} but got {sg.name}'
                )
            return _sg_from_grpc(sg)
        raise SecurityGroupNotFoundError(f'request {request_id} security group {name} not found in folder {folder_id}')

    @_retry_vpc
    @tracing.trace('VPC Wait Operation')
    def _wait_operation(self, request_id: str, operation_id: str):
        operation = self.operation_stub.Get(
            operation_service_pb2.GetOperationRequest(operation_id=operation_id),
            metadata=self._get_metadata(request_id),
            timeout=self.config.timeout,
        )
        if not operation.done:
            raise VPCWaitRepeat(f'operation not done, request {request_id} operation id {operation_id}')
        return operation

    def _convert_to_grpc_rule_specs(self, rules: List[SecurityGroupRule]):
        rule_specs = []
        for rule in rules:
            target = {}
            if rule.predefined_target:
                target['predefined_target'] = rule.predefined_target
            elif rule.security_group_id:
                target['security_group_id'] = rule.security_group_id
            else:
                target['cidr_blocks'] = security_group_pb2.CidrBlocks(
                    v4_cidr_blocks=rule.v4_cidr_blocks,
                    v6_cidr_blocks=rule.v6_cidr_blocks,
                )
            rule_spec = security_group_service_pb2.SecurityGroupRuleSpec(
                description=rule.description,
                direction=rule.direction.value,
                protocol_name=rule.protocol_name,
                protocol_number=rule.protocol_number,
                ports=security_group_pb2.PortRange(
                    from_port=rule.ports_from,
                    to_port=rule.ports_to,
                ),
                **target,
            )
            rule_specs.append(rule_spec)
        return rule_specs

    @_retry_vpc
    @tracing.trace('VPC Create Security Group')
    def create_security_group(
        self, name: str, folder_id: str, network_id: str, rules: List[SecurityGroupRule]
    ) -> SecurityGroup:
        """
        VPC Create Security Group
        """
        rule_specs = self._convert_to_grpc_rule_specs(rules)
        request_id = str(uuid.uuid4())
        try:
            operation = self.security_group_stub.Create(
                security_group_service_pb2.CreateSecurityGroupRequest(
                    folder_id=folder_id,
                    name=name,
                    network_id=network_id,
                    rule_specs=rule_specs,
                ),
                metadata=self._get_metadata(request_id),
                timeout=self.config.timeout,
            )
        except grpc.RpcError as rpc_error:
            if _is_already_exist(rpc_error):
                return self.find_security_group(name, folder_id)
            self.logger.exception('Failed to create security group %s for network id %s', name, network_id)
            raise
        self.logger.info('create security group operation %s, operation obj: "%r"', operation.id, operation)
        if not operation.done:
            operation = self._wait_operation(request_id, operation.id)
        if operation.WhichOneof('result') == 'error':
            raise CreateSecurityGroupError(f'Failed to create security group: {operation.error}')
        sg = security_group_pb2.SecurityGroup()
        operation.response.Unpack(sg)
        return _sg_from_grpc(sg)

    @_retry_vpc
    @tracing.trace('VPC Delete Security Group')
    def delete_security_group(self, sg_id: str) -> None:
        """
        VPC Delete Security Group
        """
        request_id = str(uuid.uuid4())
        try:
            operation = self.security_group_stub.Delete(
                security_group_service_pb2.DeleteSecurityGroupRequest(security_group_id=sg_id),
                metadata=self._get_metadata(request_id),
                timeout=self.config.timeout,
            )
        except grpc.RpcError as rpc_error:
            if _is_not_found(rpc_error):
                return
            self.logger.exception('Failed to delete security group %s: %s', sg_id, repr(rpc_error))
            raise
        if not operation.done:
            operation = self._wait_operation(request_id, operation.id)
        if operation.WhichOneof('result') == 'error':
            raise DeleteSecurityGroupError(f'Failed to delete security group: {operation.error}')
        self.logger.info('deleted security group %s', sg_id)

    @_retry_vpc
    @tracing.trace('VPC Update Security Group Rules')
    def set_security_group_rules(self, sg_id: str, rules: List[SecurityGroupRule]) -> None:
        """
        VPC Update Security Group Rules
        """
        rule_specs = self._convert_to_grpc_rule_specs(rules)
        request_id = str(uuid.uuid4())
        try:
            operation = self.security_group_stub.Update(
                security_group_service_pb2.UpdateSecurityGroupRequest(
                    security_group_id=sg_id,
                    update_mask=field_mask_pb2.FieldMask(paths=['rule_specs']),
                    rule_specs=rule_specs,
                ),
                metadata=self._get_metadata(request_id),
                timeout=self.config.timeout,
            )
        except grpc.RpcError as rpc_error:
            if _is_not_found(rpc_error):
                raise SecurityGroupNotFoundError(f'Security group {sg_id} not found')
            self.logger.exception('Failed to update security group %s rules', sg_id)
            raise
        self.logger.info('update security group rules operation %s, operation obj: "%r"', operation.id, operation)
        if not operation.done:
            operation = self._wait_operation(request_id, operation.id)
        if operation.WhichOneof('result') == 'error':
            raise UpdateSecurityGroupError(f'Failed to update security group rules: {operation.error}')

    @_retry_vpc
    @tracing.trace('VPC Set Superflow v2.2 Flag On Instance')
    def set_superflow_v22_flag_on_instance(self, instance_id: str, feature_flag_id: str) -> None:
        request_id = str(uuid.uuid4())
        tracing.set_tag('instance.id', instance_id)
        tracing.set_tag('request_id', request_id)
        try:
            operation = self.feature_flag_stub.AddToWhiteList(
                feature_flag_service_pb2.AddToFeatureFlagWhiteListRequest(
                    feature_flag_id=feature_flag_id,
                    scopes=[Resource(id=instance_id, type='compute.instance')],
                ),
                metadata=self._get_metadata(request_id),
                timeout=self.config.timeout,
            )
        except grpc.RpcError as rpc_error:
            if _is_not_found(rpc_error):
                raise FeatureFlagNotFoundError(f'Feature flag {feature_flag_id} not found')
            if _is_already_exist(rpc_error):
                self.logger.info('Feature flag %s is already exists on instance %s', feature_flag_id, instance_id)
                return
            self.logger.exception(
                'Failed to set feature flag %s on instance %s, error %s', feature_flag_id, instance_id, repr(rpc_error)
            )
            raise
        if not operation.done:
            operation = self._wait_operation(request_id, operation.id)
        if operation.WhichOneof('result') == 'error':
            raise SetFeatureFlagError(f'Failed to set feature flag {feature_flag_id}: {operation.error}')
        self.logger.info('Set feature flag %s success on instance %s', feature_flag_id, instance_id)
