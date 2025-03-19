from yandex.cloud.priv.loadbalancer.v1alpha.inner import loadbalancer_pb2 as lb

from schematics.exceptions import ConversionError
from schematics.types import StringType, ListType, ModelType, BaseType
from yc_common.clients.models.grpc.common import GrpcModel, QuasiIpAddress, BaseLoadBalancerGrpcModel
from yc_common.clients.models.network_load_balancers import ListenerProtocol
from yc_common.validation import LBPortType
from yc_common.exceptions import Error
from yc_common.models import IPAddressType


class ForwardingGroupTarget(GrpcModel):
    zone_id = StringType()
    address = ModelType(QuasiIpAddress)

    @classmethod
    def get_grpc_class(cls):
        return lb.ForwardingGroup.Target


class ProtocolType(BaseType):
    primitive_type = int
    native_type = str

    MESSAGES = {
        'convert': "Couldn't convert '{0}' value to protocol type",
    }

    def __init__(self, *args, **kwargs):
        super().__init__(*args, choices=ListenerProtocol.ALL, **kwargs)

    def to_native(self, value, context=None):
        if isinstance(value, str):
            return value

        try:
            return ListenerProtocol.from_iana(value)
        except Error:
            raise ConversionError(self.messages['convert'].format(value))

    def to_primitive(self, value, context=None):
        return ListenerProtocol.to_iana(value)


class ForwardingGroupListener(GrpcModel):
    address = StringType()
    port = LBPortType()
    protocol = ProtocolType()

    @classmethod
    def get_grpc_class(cls):
        return lb.ForwardingGroup.Listener


class ForwardingGroupLbOptions(GrpcModel):
    health_check_group_id = StringType()

    @classmethod
    def get_grpc_class(cls):
        return lb.ForwardingGroup.LbOptions


class ForwardingGroupGrpc(GrpcModel):
    id = StringType(required=True)
    listeners = ListType(ModelType(ForwardingGroupListener), default=list)
    targets = ListType(ModelType(ForwardingGroupTarget), default=list)
    lb_options = ModelType(ForwardingGroupLbOptions)

    @classmethod
    def get_grpc_class(cls):
        return lb.ForwardingGroup


class ForwardingRule(GrpcModel):
    class Address(GrpcModel):
        address = IPAddressType(required=True)
        port = LBPortType()

        @classmethod
        def get_grpc_class(cls):
            return lb.ForwardingRule.Address

        @property
        def frozen(self):
            return tuple(self.values())

    zone_id = StringType(required=True)
    protocol = ProtocolType(required=True)
    source = ModelType(Address, required=True)
    destination = ModelType(Address, required=True)

    @classmethod
    def get_grpc_class(cls):
        return lb.ForwardingRule

    @property
    def frozen(self):
        return (self.zone_id, self.protocol, self.source.frozen, self.destination.frozen)


class ForwardingGroup(BaseLoadBalancerGrpcModel):
    # TODO(yesworld): Sync forwarding group model with grpc message.
    id = StringType(required=True)
    health_check_group_id = StringType(required=True)
    forwarding_rules = ListType(ModelType(ForwardingRule), required=True)

    @classmethod
    def get_grpc_class(cls):
        return lb.ForwardingGroup

    @property
    def frozen(self):
        return (
            self.id,
            self.health_check_group_id,
            frozenset(r.frozen for r in self.forwarding_rules),
        )
