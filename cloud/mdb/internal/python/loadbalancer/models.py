from dataclasses import dataclass
from datetime import datetime
from enum import Enum
from typing import Union, List, Dict, Optional

from yandex.cloud.priv.loadbalancer.v1 import (
    network_load_balancer_pb2,
    network_load_balancer_service_pb2,
    target_group_pb2,
    health_check_pb2,
)

from cloud.mdb.internal.python.compute.models import enum_from_api


@dataclass
class Target:
    subnet_id: str
    address: str

    @staticmethod
    def from_api(grpc_response: target_group_pb2.Target):
        return Target(
            subnet_id=grpc_response.subnet_id,
            address=grpc_response.address,
        )

    @staticmethod
    def to_api(target):
        return target_group_pb2.Target(
            subnet_id=target.subnet_id,
            address=target.address,
        )


@dataclass
class TargetGroup:
    id: str
    folder_id: str
    created_at: datetime
    name: str
    description: str
    labels: Dict[str, str]
    targets: List[Target]
    network_load_balancer_ids: List[str]

    @staticmethod
    def from_api(grpc_response: target_group_pb2.TargetGroup):
        return TargetGroup(
            id=grpc_response.id,
            folder_id=grpc_response.folder_id,
            created_at=grpc_response.created_at,
            name=grpc_response.name,
            description=grpc_response.description,
            labels=grpc_response.labels,
            targets=[Target.from_api(target) for target in grpc_response.targets],
            network_load_balancer_ids=grpc_response.network_load_balancer_ids,
        )


class IpVersion(Enum):
    IP_VERSION_UNSPECIFIED = network_load_balancer_pb2.IpVersion.IP_VERSION_UNSPECIFIED
    IPV4 = network_load_balancer_pb2.IpVersion.IPV4
    IPV6 = network_load_balancer_pb2.IpVersion.IPV6

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, IpVersion)

    @staticmethod
    def to_api(model):
        return model.value


class NetworkLoadBalancerStatus(Enum):
    STATUS_UNSPECIFIED = network_load_balancer_pb2.NetworkLoadBalancer.Status.STATUS_UNSPECIFIED
    CREATING = network_load_balancer_pb2.NetworkLoadBalancer.Status.CREATING
    STARTING = network_load_balancer_pb2.NetworkLoadBalancer.Status.STARTING
    ACTIVE = network_load_balancer_pb2.NetworkLoadBalancer.Status.ACTIVE
    STOPPING = network_load_balancer_pb2.NetworkLoadBalancer.Status.STOPPING
    STOPPED = network_load_balancer_pb2.NetworkLoadBalancer.Status.STOPPED
    DELETING = network_load_balancer_pb2.NetworkLoadBalancer.Status.DELETING
    INACTIVE = network_load_balancer_pb2.NetworkLoadBalancer.Status.INACTIVE

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, NetworkLoadBalancerStatus)

    @staticmethod
    def to_api(model):
        return model.value


class Type(Enum):
    TYPE_UNSPECIFIED = network_load_balancer_pb2.NetworkLoadBalancer.Type.TYPE_UNSPECIFIED
    EXTERNAL = network_load_balancer_pb2.NetworkLoadBalancer.Type.EXTERNAL
    INTERNAL = network_load_balancer_pb2.NetworkLoadBalancer.Type.INTERNAL

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, Type)

    @staticmethod
    def to_api(model):
        return model.value


class SessionAffinity(Enum):
    SESSION_AFFINITY_UNSPECIFIED = (
        network_load_balancer_pb2.NetworkLoadBalancer.SessionAffinity.SESSION_AFFINITY_UNSPECIFIED
    )
    CLIENT_IP_PORT_PROTO = network_load_balancer_pb2.NetworkLoadBalancer.SessionAffinity.CLIENT_IP_PORT_PROTO

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, SessionAffinity)

    @staticmethod
    def to_api(model):
        return model.value


class Protocol(Enum):
    PROTOCOL_UNSPECIFIED = network_load_balancer_pb2.Listener.Protocol.PROTOCOL_UNSPECIFIED
    TCP = network_load_balancer_pb2.Listener.Protocol.TCP
    UDP = network_load_balancer_pb2.Listener.Protocol.UDP

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, Protocol)

    @staticmethod
    def to_api(model):
        return model.value


class TargetStateStatus(Enum):
    STATUS_UNSPECIFIED = network_load_balancer_pb2.TargetState.Status.STATUS_UNSPECIFIED
    INITIAL = network_load_balancer_pb2.TargetState.Status.INITIAL
    HEALTHY = network_load_balancer_pb2.TargetState.Status.HEALTHY
    UNHEALTHY = network_load_balancer_pb2.TargetState.Status.UNHEALTHY
    INACTIVE = network_load_balancer_pb2.TargetState.Status.INACTIVE

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, TargetStateStatus)

    @staticmethod
    def to_api(model):
        return model.value


@dataclass
class TargetState:
    subnet_id: str
    address: str
    status: TargetStateStatus

    @staticmethod
    def from_api(grpc_response: network_load_balancer_pb2.TargetState):
        return TargetState(
            subnet_id=grpc_response.subnet_id,
            address=grpc_response.address,
            status=TargetStateStatus.from_api(grpc_response.status),
        )


@dataclass
class Listener:
    name: str
    address: str
    port: int
    protocol: Protocol
    ip_version: IpVersion
    target_port: int
    subnet_id: str

    @staticmethod
    def from_api(grpc_response: network_load_balancer_pb2.Listener):
        return Listener(
            name=grpc_response.name,
            address=grpc_response.address,
            port=grpc_response.port,
            protocol=Protocol.from_api(grpc_response.protocol),
            ip_version=IpVersion.from_api(grpc_response.ip_version),
            target_port=grpc_response.target_port,
            subnet_id=grpc_response.subnet_id,
        )


@dataclass
class TcpOptions:
    port: int

    @staticmethod
    def from_api(grpc_response: health_check_pb2.HealthCheck.TcpOptions):
        return TcpOptions(
            port=grpc_response.port,
        )

    @staticmethod
    def to_api(model):
        return health_check_pb2.HealthCheck.TcpOptions(
            port=model.port,
        )


@dataclass
class HttpOptions:
    port: int
    path: str
    success_codes: Optional[List[int]] = None
    headers: Optional[Dict[str, str]] = None

    @staticmethod
    def from_api(grpc_response: health_check_pb2.HealthCheck.HttpOptions):
        return HttpOptions(
            port=grpc_response.port,
            path=grpc_response.path,
            success_codes=grpc_response.success_codes,
            headers=grpc_response.headers,
        )

    @staticmethod
    def to_api(model):
        return health_check_pb2.HealthCheck.HttpOptions(
            port=model.port,
            path=model.path,
            success_codes=model.success_codes,
            headers=model.headers,
        )


@dataclass
class HttpsOptions:
    port: int
    host: str
    path: str
    success_codes: List[int]
    headers: Dict[str, str]
    verify_hostname: bool
    verify_cert_dates: bool

    @staticmethod
    def from_api(grpc_response: health_check_pb2.HealthCheck.HttpsOptions):
        return HttpsOptions(
            port=grpc_response.port,
            host=grpc_response.host,
            path=grpc_response.path,
            success_codes=grpc_response.success_codes,
            headers=grpc_response.headers,
            verify_hostname=grpc_response.verify_hostname,
            verify_cert_dates=grpc_response.verify_cert_dates,
        )


@dataclass
class Http2Options:
    port: int
    host: str
    path: str
    success_codes: List[int]
    headers: Dict[str, str]
    verify_hostname: bool
    verify_cert_dates: bool

    @staticmethod
    def from_api(grpc_response: health_check_pb2.HealthCheck.Http2Options):
        return Http2Options(
            port=grpc_response.port,
            host=grpc_response.host,
            path=grpc_response.path,
            success_codes=grpc_response.success_codes,
            headers=grpc_response.headers,
            verify_hostname=grpc_response.verify_hostname,
            verify_cert_dates=grpc_response.verify_cert_dates,
        )


@dataclass
class GrpcOptions:
    port: int
    service_name: str
    authority: str

    @staticmethod
    def from_api(grpc_response: health_check_pb2.HealthCheck.GrpcOptions):
        return GrpcOptions(
            port=grpc_response.port,
            service_name=grpc_response.service_name,
            authority=grpc_response.authority,
        )


@dataclass
class HealthCheck:
    name: str
    unhealthy_threshold: int
    healthy_threshold: int
    options: Union[TcpOptions, HttpOptions, HttpsOptions, Http2Options, GrpcOptions]

    @staticmethod
    def from_api(grpc_response: health_check_pb2.HealthCheck):
        if hasattr(grpc_response, 'tcp_options'):
            options = TcpOptions.from_api(grpc_response.tcp_options)
        elif hasattr(grpc_response, 'http_options'):
            options = HttpOptions.from_api(grpc_response.http_options)
        elif hasattr(grpc_response, 'https_options'):
            options = HttpsOptions.from_api(grpc_response.https_options)
        elif hasattr(grpc_response, 'http2_options'):
            options = Http2Options.from_api(grpc_response.http2_options)
        elif hasattr(grpc_response, 'grpc_options'):
            options = GrpcOptions.from_api(grpc_response.grpc_options)
        else:
            raise TypeError('wrong options type')

        return HealthCheck(
            name=grpc_response.name,
            unhealthy_threshold=grpc_response.unhealthy_threshold,
            healthy_threshold=grpc_response.healthy_threshold,
            options=options,
        )

    @staticmethod
    def to_api(model):
        health_check = health_check_pb2.HealthCheck(
            name=model.name,
            unhealthy_threshold=model.unhealthy_threshold,
            healthy_threshold=model.healthy_threshold,
        )
        if isinstance(model.options, TcpOptions):
            health_check.tcp_options.CopyFrom(TcpOptions.to_api(model.options))
        elif isinstance(model.options, HttpOptions):
            health_check.http_options.CopyFrom(HttpOptions.to_api(model.options))

        return health_check


@dataclass
class AttachedTargetGroup:
    target_group_id: str
    health_checks: List[HealthCheck]

    @staticmethod
    def from_api(grpc_response: network_load_balancer_pb2.AttachedTargetGroup):
        return AttachedTargetGroup(
            target_group_id=grpc_response.target_group_id,
            health_checks=[HealthCheck.from_api(health_check) for health_check in grpc_response.health_checks],
        )

    @staticmethod
    def to_api(model):
        return network_load_balancer_pb2.AttachedTargetGroup(
            target_group_id=model.target_group_id,
            health_checks=[HealthCheck.to_api(health_check) for health_check in model.health_checks],
        )


@dataclass
class NetworkLoadBalancer:
    id: str
    folder_id: str
    created_at: datetime
    name: str
    description: str
    labels: Dict[str, str]
    status: NetworkLoadBalancerStatus
    type: Type
    session_affinity: SessionAffinity
    listeners: List[Listener]
    attached_target_groups: List[AttachedTargetGroup]

    @staticmethod
    def from_api(grpc_response: network_load_balancer_pb2.NetworkLoadBalancer):
        return NetworkLoadBalancer(
            id=grpc_response.id,
            folder_id=grpc_response.folder_id,
            created_at=grpc_response.created_at,
            name=grpc_response.name,
            description=grpc_response.description,
            labels=grpc_response.labels,
            status=NetworkLoadBalancerStatus.from_api(grpc_response.status),
            type=Type.from_api(grpc_response.type),
            session_affinity=SessionAffinity.from_api(grpc_response.session_affinity),
            listeners=[Listener.from_api(listener) for listener in grpc_response.listeners],
            attached_target_groups=[
                AttachedTargetGroup.from_api(attached_target_group)
                for attached_target_group in grpc_response.attached_target_groups
            ],
        )


@dataclass
class ExternalAddressSpec:
    address: str
    ip_version: IpVersion

    @staticmethod
    def from_api(grpc_response: network_load_balancer_service_pb2.ExternalAddressSpec):
        return ExternalAddressSpec(
            address=grpc_response.address,
            ip_version=IpVersion.from_api(grpc_response.ip_version),
        )

    @staticmethod
    def to_api(model):
        return network_load_balancer_service_pb2.ExternalAddressSpec(
            address=model.address,
            ip_version=IpVersion.to_api(model.ip_version),
        )


@dataclass
class InternalAddressSpec:
    subnet_id: str
    ip_version: IpVersion
    address: Optional[str] = None

    @staticmethod
    def from_api(grpc_response: network_load_balancer_service_pb2.InternalAddressSpec):
        return InternalAddressSpec(
            address=grpc_response.address,
            subnet_id=grpc_response.subnet_id,
            ip_version=IpVersion.from_api(grpc_response.ip_version),
        )

    @staticmethod
    def to_api(model):
        return network_load_balancer_service_pb2.InternalAddressSpec(
            address=model.address,
            subnet_id=model.subnet_id,
            ip_version=IpVersion.to_api(model.ip_version),
        )


@dataclass
class ListenerSpec:
    name: str
    protocol: Protocol
    external_address_spec: Optional[ExternalAddressSpec] = None
    internal_address_spec: Optional[InternalAddressSpec] = None
    port: Optional[int] = None
    target_port: Optional[int] = None

    @staticmethod
    def from_api(grpc_response: network_load_balancer_service_pb2.ListenerSpec):
        return ListenerSpec(
            name=grpc_response.name,
            port=grpc_response.port,
            protocol=Protocol.from_api(grpc_response.protocol),
            external_address_spec=ExternalAddressSpec.from_api(grpc_response.external_address_spec)
            if grpc_response.external_address_spec
            else None,
            internal_address_spec=InternalAddressSpec.from_api(grpc_response.internal_address_spec)
            if grpc_response.internal_address_spec
            else None,
            target_port=grpc_response.target_port,
        )

    @staticmethod
    def to_api(model):
        return network_load_balancer_service_pb2.ListenerSpec(
            name=model.name,
            port=model.port,
            protocol=Protocol.to_api(model.protocol),
            external_address_spec=ExternalAddressSpec.to_api(model.external_address_spec)
            if model.external_address_spec
            else None,
            internal_address_spec=InternalAddressSpec.to_api(model.internal_address_spec)
            if model.internal_address_spec
            else None,
            target_port=model.target_port,
        )
