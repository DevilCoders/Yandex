import abc
import collections.abc
import ipaddress
import socket
import struct
from typing import Tuple, Union

import google.protobuf.message
import google.protobuf.timestamp_pb2
from google.protobuf import any_pb2
from schematics.exceptions import ValidationError
from schematics.types import IntType, ListType, ModelType, StringType
from schematics.transforms import Converter, import_loop
from schematics.undefined import Undefined
from schematics import common as schematics_common

from yc_common.clients.models.operations import OperationListV1Beta1, OperationV1Beta1
from yc_common.models import ModelAbcMeta, Model, IPAddressType, IsoTimestampType


def _is_grpc_model_type(field):
    return isinstance(field, ModelType) and issubclass(field.model_class, GrpcModelMixin)


def _unpack_any(msg):
    from google.protobuf import symbol_database
    factory = symbol_database.Default()

    type_url = msg.type_url

    if not type_url:
        return None

    type_name = type_url.split('/')[-1]
    descriptor = factory.pool.FindMessageTypeByName(type_name)

    if descriptor is None:
        return None

    message_class = factory.GetPrototype(descriptor)
    message = message_class()

    message.ParseFromString(msg.value)
    return message


def _unwrap_grpc(x):
    skip = object()

    def convert_field(f, v):
        if f.enum_type is None:
            return v
        if v == 0:
            return skip
        return f.enum_type.values_by_number[v].name

    if isinstance(x, any_pb2.Any):
        x = _unpack_any(x)
    if isinstance(x, google.protobuf.message.Message):
        if not x.ByteSize():
            return None
        return {
            k: v for k, v in (
                (f.name, convert_field(f, getattr(x, f.name, None)))
                for f in x.DESCRIPTOR.fields
            ) if v is not skip
        }
    elif isinstance(x, collections.abc.Mapping) and not x:
        return None
    return x


class GrpcModelMixin:
    @classmethod
    @abc.abstractmethod
    def get_grpc_class(cls):
        return None

    @classmethod
    def from_grpc(cls, grpc_model):
        native = cls.convert_grpc(_unwrap_grpc(grpc_model))
        return cls.from_api(native)

    def to_grpc(self):
        values = {}
        for field_name, field_model in self.fields.items():
            value = getattr(self, field_name, None)
            if value is None:
                continue

            if field_model.export_level == schematics_common.DROP:
                continue

            if isinstance(field_model, ListType) and _is_grpc_model_type(field_model.field):
                value = [obj.to_grpc() for obj in value]
            elif _is_grpc_model_type(field_model):
                value = value.to_grpc()
            else:
                value = field_model.to_primitive(value)

            values[field_name] = value

        grpc_class = self.get_grpc_class()
        return grpc_class(**values)

    @classmethod
    def convert_grpc(cls, grpc_model, context=None):
        return import_loop(cls._schema, grpc_model, field_converter=GrpcImporter(), context=context)


class GrpcTypeMixin:
    def convert_grpc(self, value, context=None):
        return self.convert(value, context=None)

    def to_grpc(self, value, context=None):
        return self.to_primitive(value, context=context)


class GrpcImporter(Converter):
    def __call__(self, field, value, context):
        value = _unwrap_grpc(value)
        field.check_required(value, context)
        if value is None or value is Undefined:
            return value
        if isinstance(field, GrpcTypeMixin):
            return field.convert_grpc(value, context=context)
        if _is_grpc_model_type(field):
            return field.model_class.convert_grpc(value, context=context)
        if isinstance(field, IsoTimestampType):
            return value.get('seconds') or 0
        if isinstance(field, StringType) and not value:
            return None
        return field.convert(value, context=context)


class GrpcModel(Model, GrpcModelMixin, metaclass=ModelAbcMeta):
    pass


class QuasiIpAddress(GrpcModel):
    """ https://wiki.yandex-team.ru/cloud/devel/loadbalancing/#slovar """

    _padding = b'\xfc' + b'\x00' * 7

    ip = IPAddressType(required=True)
    vrf = IntType(required=False)

    def validate_vrf(self, data, value):
        if data.get("ip"):
            ip_version = None
            try:
                ip_version = ipaddress.ip_address(data["ip"]).version
            except ValueError as e:
                raise ValidationError(str(e))
            if ip_version == 4 and not data.get("vrf"):
                raise ValidationError("ipv4 requires vrf")
        return value

    @property
    def ipv6(self) -> str:
        return str(self.to_quasi_ipv6(self.ip, self.vrf))

    @classmethod
    def to_quasi_ipv6(cls, ip: Union[str, ipaddress.IPv4Address, ipaddress.IPv6Address], vrf: int) -> str:
        if isinstance(ip, str):
            ip = ipaddress.ip_address(ip)
        if ip.version == 6:
            return ip
        b = cls._padding + struct.pack("<I", vrf) + socket.inet_aton(str(ip))
        return ipaddress.IPv6Address(b)

    @classmethod
    def from_quasi_ipv6(cls, ipv6: str) -> Tuple[Union[ipaddress.IPv4Address, ipaddress.IPv6Address], int]:
        b = ipaddress.IPv6Address(ipv6).packed
        ip = ipv6
        vrf = None
        if b[:len(cls._padding)] == cls._padding:
            ip = str(ipaddress.IPv4Address(b[-4:]))
            vrf = struct.unpack("<I", b[len(cls._padding):len(b)-4])[0]
        return ipaddress.ip_address(ip), vrf

    @classmethod
    def get_grpc_class(cls):
        return None

    def to_grpc(self, context=None):
        return self.ipv6

    @classmethod
    def convert_grpc(cls, grpc_model, context=None):
        ip, vrf = cls.from_quasi_ipv6(grpc_model)
        return cls.new(ip=ip, vrf=vrf)


class OperationListGrpc(OperationListV1Beta1, GrpcModelMixin):
    pass


class OperationGrpc(OperationV1Beta1, GrpcModelMixin):
    pass


class LoadBalancerGrpcMetadataModel(Model):
    network_load_balancer_id = StringType()
    target_group_id = StringType()

    def to_metadata(self):
        result = []
        for field_name, field_model in self.fields.items():
            value = getattr(self, field_name, None)
            if value is None:
                continue

            if field_model.export_level == schematics_common.DROP:
                continue

            result.append((field_name, value))
        return result


class BaseLoadBalancerGrpcModel(GrpcModel):
    metadata = ModelType(LoadBalancerGrpcMetadataModel, export_level=schematics_common.DROP)
