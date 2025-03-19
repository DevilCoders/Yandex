import ipaddress
import socket

from schematics.exceptions import ValidationError

from yc_common.clients.models import base as base_models
from yc_common.clients.models.grpc.common import GrpcModelMixin
from yc_common.clients.models.networks import AddressType, DDoSProvider, IpVersion, IpVersionType as _IpVersionType
from yc_common.exceptions import Error
from yc_common.models import BooleanType, IPAddressType, StringType, ListType, ModelType, StringEnumType
from yc_common.validation import LBPortType, ResourceIdType, ResourceNameType
from . import operations, target_groups


class SessionAffinity:
    CLIENT_IP_PORT_PROTO = "client_ip_port_proto"

    DEFAULT = CLIENT_IP_PORT_PROTO
    ALL = [CLIENT_IP_PORT_PROTO]


class NetworkLoadBalancerType:
    EXTERNAL = "external"
    INTERNAL = "internal"
    ALL = [EXTERNAL, INTERNAL]


class ListenerProtocol:
    TCP = "tcp"
    UDP = "udp"
    ALL = [TCP, UDP]

    @classmethod
    def to_iana(cls, protocol: str) -> int:
        if protocol == cls.TCP:
            return socket.IPPROTO_TCP
        if protocol == cls.UDP:
            return socket.IPPROTO_UDP

        raise Error("Undefined IP protocol name.")

    @classmethod
    def from_iana(cls, protocol: int) -> str:
        if protocol == socket.IPPROTO_TCP:
            return cls.TCP
        if protocol == socket.IPPROTO_UDP:
            return cls.UDP

        raise Error("Undefined IP protocol number.")


def _get_address_version(address):
    return {4: IpVersion.IPV4, 6: IpVersion.IPV6}[address.version]


def _validate_ip_version(data, ip_version):
    address = data.get("address")
    if address is not None and ip_version != _get_address_version(address):
        raise ValidationError("Invalid IP version for address")


class IpVersionType(_IpVersionType):
    def convert(self, value, context=None):
        value = super().convert(value, context=context)
        if value in IpVersion.ALL:
            return value
        try:
            address = ipaddress.ip_address(value)
        except ValueError:
            return value
        return _get_address_version(address)


class ExternalAddressSpecWithoutAntiDdos(base_models.BasePublicModel, GrpcModelMixin):
    address = IPAddressType()
    ip_version = IpVersionType(default=IpVersion.IPV4, deserialize_from="address", required=True)
    yandex_only = BooleanType()

    def validate_ip_version(self, data, value):
        _validate_ip_version(data, value)

    def validate_yandex_only(self, data, value):
        if value and data["ip_version"] != IpVersion.IPV6:
            raise ValidationError("Invalid IP version for address")

    @property
    def allocating(self):
        return self.address is None

    @property
    def type(self):
        return AddressType.EXTERNAL

    def __hash__(self):
        return hash(tuple(self.values()))

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.loadbalancer.v1.network_load_balancer_service_pb2 import ExternalAddressSpec
        return ExternalAddressSpec


class ExternalAddressSpec(ExternalAddressSpecWithoutAntiDdos):
    ddos_protection_provider = StringType()

    def validate_ddos_protection_provider(self, data, ddos_protection_provider):
        if ddos_protection_provider is not None and ddos_protection_provider not in DDoSProvider.ALL:
            raise ValidationError("Invalid ddos_protection_provider value")

    @classmethod
    def from_spec_without_antiddos(cls, spec_without_ddos):
        return cls.new_from_model(spec_without_ddos, aliases={"ddos_protection_provider": None})


class InternalAddressSpec(base_models.BasePublicModel, GrpcModelMixin):
    INTERNAL_IPV6_LISTENER_NOT_ALLOWED = "The internal network load balancer can't have ipv6 listener."

    address = IPAddressType()
    subnet_id = ResourceIdType(required=True)
    ip_version = IpVersionType(default=IpVersion.IPV4, deserialize_from="address", required=True)

    def validate_ip_version(self, data, value):
        _validate_ip_version(data, value)
        if value == IpVersion.IPV6:
            raise ValidationError(self.INTERNAL_IPV6_LISTENER_NOT_ALLOWED)

    @property
    def allocating(self):
        return self.address is None

    @property
    def type(self):
        return AddressType.INTERNAL

    def __hash__(self):
        return hash(tuple(self.values()))

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.loadbalancer.v1.network_load_balancer_service_pb2 import InternalAddressSpec
        return InternalAddressSpec


class ListenerPublicV1Alpha1(base_models.BasePublicModel, GrpcModelMixin):
    subnet_id = ResourceIdType()
    address = IPAddressType()
    port = LBPortType()
    # target_port will be deserialized from port field if target_port is not presented in input data
    target_port = LBPortType(deserialize_from='port')
    protocol = StringEnumType()


class ListenerPublic(ListenerPublicV1Alpha1):
    name = ResourceNameType()


class BaseListenerSpec(base_models.BasePublicModel, GrpcModelMixin):
    port = LBPortType(required=True)
    # target_port will be deserialized from port field if target_port is not presented in input data
    target_port = LBPortType(required=True, deserialize_from='port')
    protocol = StringEnumType(required=True, choices=ListenerProtocol.ALL)

    internal_address_spec = ModelType(InternalAddressSpec)

    def validate_internal_address_spec(self, data, internal_spec):
        external_spec = data["external_address_spec"]

        not_none_address_specs = [spec for spec in [external_spec, internal_spec] if spec is not None]

        if len(not_none_address_specs) != 1:
            raise ValidationError("Only one address field is allowed.")

    @property
    def address_spec(self):
        return self.external_address_spec or self.internal_address_spec

    @property
    def ip_version(self):
        return self.address_spec.ip_version

    @property
    def address_type(self):
        if self.external_address_spec is not None:
            return NetworkLoadBalancerType.EXTERNAL
        return NetworkLoadBalancerType.INTERNAL

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.loadbalancer.v1.network_load_balancer_service_pb2 import ListenerSpec
        return ListenerSpec


class ListenerSpecExternalAddressSpecWithoutAntiDDosMixin(base_models.BasePublicModel):
    external_address_spec = ModelType(ExternalAddressSpecWithoutAntiDdos)


class ListenerSpecExternalAddressSpecMixin(base_models.BasePublicModel):
    external_address_spec = ModelType(ExternalAddressSpec)

    @classmethod
    def from_spec_without_antiddos(cls, spec_without_ddos):
        spec = cls.new_from_model(spec_without_ddos, aliases={"external_address_spec": None})
        if spec_without_ddos.external_address_spec is not None:
            spec.external_address_spec = ExternalAddressSpec.from_spec_without_antiddos(spec_without_ddos.external_address_spec)
        return spec


class ListenerSpecNameMixin(base_models.BasePublicModel):
    name = ResourceNameType(required=True)

    @classmethod
    def generate_name(cls, protocol, port):
        return "-".join(map(str, ["auto", protocol, port]))

    @classmethod
    def from_spec_without_name(cls, spec_without_name):
        spec = cls.new_from_model(spec_without_name, aliases={"name": None})
        spec.name = cls.generate_name(spec_without_name.protocol, spec_without_name.port)
        return spec


class ListenerSpec(BaseListenerSpec, ListenerSpecNameMixin, ListenerSpecExternalAddressSpecMixin):
    pass


class ListenerSpecWithoutAntiDdos(BaseListenerSpec, ListenerSpecNameMixin, ListenerSpecExternalAddressSpecWithoutAntiDDosMixin):
    pass


class ListenerSpecWithoutName(BaseListenerSpec, ListenerSpecExternalAddressSpecMixin):
    pass


class ListenerSpecWithoutAntiDdosAndName(BaseListenerSpec, ListenerSpecExternalAddressSpecWithoutAntiDDosMixin):
    pass


def ln_spec_cls(with_antiddos, with_name):
    # {(with_antiddos, with_name): cls}
    return {
        (False, False): ListenerSpecWithoutAntiDdosAndName,
        (False, True): ListenerSpecWithoutAntiDdos,
        (True, False): ListenerSpecWithoutName,
        (True, True): ListenerSpec,
    }[(with_antiddos, with_name)]


class NetworkLoadBalancerStatus:
    CREATING = "creating"
    STARTING = "starting"
    ACTIVE = "active"
    STOPPING = "stopping"
    STOPPED = "stopped"
    DELETING = "deleting"
    INACTIVE = "inactive"

    ALL_IN_PROGRESS = [CREATING, STARTING, STOPPING, DELETING]
    ALL_STARTED = [CREATING, STARTING, ACTIVE]
    ALL_READY = [ACTIVE, STOPPED, INACTIVE]
    ALL_STOPPED = [STOPPED, INACTIVE]
    ALL = [CREATING, STARTING, ACTIVE, STOPPING, STOPPED, DELETING, INACTIVE]


class TargetStateStatus:
    INITIAL = "initial"
    HEALTHY = "healthy"
    UNHEALTHY = "unhealthy"
    DRAINING = "draining"
    INACTIVE = "inactive"

    ALL = [INITIAL, HEALTHY, UNHEALTHY, DRAINING, INACTIVE]


class TargetStatePublic(base_models.BasePublicModel):
    subnet_id = ResourceIdType(required=True)
    address = IPAddressType(required=True)
    status = StringEnumType()


class NetworkLoadBalancerTargetStates(base_models.BasePublicModel, GrpcModelMixin):
    target_states = ListType(ModelType(TargetStatePublic), default=list)


class BaseNetworkLoadBalancerPublic(base_models.RegionalPublicObjectModelV1Beta1, GrpcModelMixin):
    type = StringEnumType()
    attached_target_groups = ListType(ModelType(target_groups.AttachedTargetGroupPublic))
    session_affinity = StringEnumType()
    status = StringEnumType()


class NetworkLoadBalancerPublicV1Alpha1(BaseNetworkLoadBalancerPublic):
    listeners = ListType(ModelType(ListenerPublicV1Alpha1))


class NetworkLoadBalancerPublic(BaseNetworkLoadBalancerPublic):
    listeners = ListType(ModelType(ListenerPublic))


class NetworkLoadBalancerListV1Alpha1(base_models.BaseListModel, GrpcModelMixin):
    network_load_balancers = ListType(ModelType(NetworkLoadBalancerPublicV1Alpha1), required=True, default=list)


class NetworkLoadBalancerList(base_models.BaseListModel, GrpcModelMixin):
    network_load_balancers = ListType(ModelType(NetworkLoadBalancerPublic), required=True, default=list)


class NetworkLoadBalancerMetadata(operations.OperationMetadataV1Beta1, GrpcModelMixin):
    network_load_balancer_id = StringType(required=True)


class NetworkLoadBalancerOperation(operations.OperationV1Beta1, GrpcModelMixin):
    metadata = ModelType(NetworkLoadBalancerMetadata)


class NetworkLoadBalancerAttachmentMetadata(operations.OperationMetadataV1Beta1, GrpcModelMixin):
    network_load_balancer_id = StringType(required=True)
    target_group_id = StringType(required=True)


class NetworkLoadBalancerAttachmentOperation(operations.OperationV1Beta1, GrpcModelMixin):
    metadata = ModelType(NetworkLoadBalancerAttachmentMetadata)
