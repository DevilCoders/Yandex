import re

from schematics import common as schematics_common
from schematics import types as schematics_types
from schematics.datastructures import Context
from schematics.exceptions import ValidationError

from yc_common import fields
from yc_common import models as common_models
from yc_common import validation
from yc_common.clients.models import base as base_models
from yc_common.clients.models import operations as operations_models
from yc_common.clients.models.grpc.common import GrpcModelMixin, GrpcTypeMixin


class IpVersion:
    IPV4 = "ipv4"
    IPV6 = "ipv6"

    ALL = [IPV4, IPV6]


class AddressType:
    INTERNAL = "internal"
    EXTERNAL = "external"

    ALL = [INTERNAL, EXTERNAL]


class DDoSProvider:
    QRATOR = "qrator"

    ALL = [QRATOR]


class SMTPCapability:
    DIRECT = "direct"
    RELAY = "relay"

    ALL = [DIRECT, RELAY]


class IpVersionType(common_models.StringEnumType, GrpcTypeMixin):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, choices=IpVersion.ALL, **kwargs)

    def to_grpc(self, value, context=None):
        return None if value is None else value.upper()


class NetworkInterfaceAddress(base_models.BasePublicModel):
    type = common_models.StringEnumType(required=True)
    ip_version = IpVersionType()
    address = schematics_types.StringType(required=True)


class OneToOneNatSchema(base_models.BasePublicModel, GrpcModelMixin):
    address_id = validation.ResourceIdType()
    address = common_models.IPAddressType()
    ip_version = IpVersionType()
    ddos_protection_provider = common_models.StringType()
    outgoing_smtp_capability = common_models.StringType()

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.compute.v1.instance_service_pb2 import OneToOneNatSpec
        return OneToOneNatSpec


class OneToOneNat(base_models.BasePublicModel):
    address = schematics_types.StringType()
    address_id = schematics_types.StringType()
    ip_version = IpVersionType()


class PrimaryAddressSchema(base_models.BasePublicModel, GrpcModelMixin):
    address = schematics_types.StringType()
    one_to_one_nat_spec = schematics_types.ModelType(OneToOneNatSchema, deserialize_from=["nat_spec", "natSpec"])
    additional_fqdns = schematics_types.ListType(validation.HostnameType, max_size=32)

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.compute.v1.instance_service_pb2 import PrimaryAddressSpec
        return PrimaryAddressSpec


class PrimaryAddress(base_models.BasePublicModel):
    address = schematics_types.StringType(required=True)
    one_to_one_nat = schematics_types.ModelType(OneToOneNat)
    additional_fqdns = schematics_types.ListType(schematics_types.StringType)


class NetworkInterfaceSchemaV1Alpha1(base_models.BasePublicModel):
    network_id = schematics_types.StringType(required=True, max_length=256, metadata={"description": "Network ID"})


class NetworkInterfaceSchema(base_models.BasePublicModel, GrpcModelMixin):
    # We can't make it required in the model because the model is used in billing metrics simulation Console API where
    # subnet ID may be not known if user has no subnets yet.
    subnet_id = validation.ResourceIdType()

    primary_v4_address_spec = schematics_types.ModelType(PrimaryAddressSchema)
    primary_v6_address_spec = schematics_types.ModelType(PrimaryAddressSchema)

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.compute.v1.instance_service_pb2 import NetworkInterfaceSpec
        return NetworkInterfaceSpec


class NetworkInterfaceV1Alpha1(base_models.BasePublicModel):
    port_id = schematics_types.StringType(required=True)
    network_id = schematics_types.StringType(required=True)
    mac_address = schematics_types.StringType(required=True)
    primary_address = schematics_types.ModelType(NetworkInterfaceAddress)
    secondary_addresses = schematics_types.ListType(schematics_types.ModelType(NetworkInterfaceAddress))


class NetworkInterface(base_models.BasePublicModel):
    index = schematics_types.IntType(required=True)
    mac_address = schematics_types.StringType(required=True)

    # Optional, only valid when interface attached to some network
    subnet_id = schematics_types.StringType()
    primary_v4_address = schematics_types.ModelType(PrimaryAddress)
    primary_v6_address = schematics_types.ModelType(PrimaryAddress)


# FIXME: Mustn't be available to public users
class UnderlayNetworkSchema(base_models.BasePublicModel):
    network_name = schematics_types.StringType(required=True)


# FIXME: Mustn't be available to public users
class UnderlayNetwork(base_models.BasePublicModel):
    network_name = schematics_types.StringType(required=True)
    mac_address = schematics_types.StringType()
    primary_address = schematics_types.ModelType(PrimaryAddress)


class NetworkType:
    STANDARD = "standard"
    SOFTWARE_ACCELERATED = "software-accelerated"
    ALL = [STANDARD, SOFTWARE_ACCELERATED]


class NetworkSettingsUpdate(base_models.BasePublicModel):
    type = common_models.StringEnumType(choices=NetworkType.ALL)


class NetworkSettingsSpec(NetworkSettingsUpdate):
    pass


class NetworkSettings(base_models.BasePublicModel):
    type = common_models.StringEnumType(required=True)


# API 2.0


class NetworkStatus:
    CREATING = "creating"
    ACTIVE = "active"
    DELETING = "deleting"

    ALL = [CREATING, ACTIVE, DELETING]
    ALL_ACTIVE = [ACTIVE]


class Network(base_models.BasePublicObjectModelV1Beta1, GrpcModelMixin):
    status = common_models.StringEnumType()

    system_route_table_id = common_models.StringType()  # NB: this is only for private api, see: CLOUD-25999


class NetworkList(base_models.BaseListModel, GrpcModelMixin):
    networks = schematics_types.ListType(schematics_types.ModelType(Network), required=True, default=list)


class SubnetStatus:
    CREATING = "creating"
    ACTIVE = "active"
    DELETING = "deleting"

    ALL = [CREATING, ACTIVE, DELETING]
    ALL_ACTIVE = [ACTIVE]


class RouteTargetType(schematics_types.StringType):
    ROUTE_TARGET_RE = re.compile(r"^(\d{1,5}):(\d{1,10})$")

    def validate_route_target(self, value, context=None):
        match = self.ROUTE_TARGET_RE.match(value)
        if match is None:
            raise ValidationError("Invalid route target format.")

        asn, number = map(int, match.groups())
        good = (0 <= asn < 2**16) and (0 <= number < 2**32)
        if not good:
            raise ValidationError("Route target is out of range.")


class SubnetExtraParamsSchema(base_models.BasePublicModel, GrpcModelMixin):
    import_rts = fields.JsonListType(RouteTargetType, default=list)
    export_rts = fields.JsonListType(RouteTargetType, default=list)
    next_vdns = schematics_types.StringType(max_length=256)
    hbf_enabled = schematics_types.BooleanType(default=False)
    rpf_enabled = schematics_types.BooleanType(default=False)
    feature_flags = schematics_types.ListType(schematics_types.StringType, default=list)

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.vpc.v1.subnet_service_pb2 import SubnetExtraParams
        return SubnetExtraParams


class Subnet(base_models.ZonalPublicObjectModelV1Beta1, GrpcModelMixin):
    status = common_models.StringEnumType()

    network_id = schematics_types.StringType(required=True)
    route_table_id = schematics_types.StringType()
    egress_nat_enable = schematics_types.BooleanType()

    v4_cidr_block = schematics_types.ListType(schematics_types.StringType)
    v6_cidr_block = schematics_types.ListType(schematics_types.StringType)

    @classmethod
    def convert_grpc(cls, grpc_model, context=None):
        context = Context(context or {}, mapping={k: k+'s' for k in ['v4_cidr_block', 'v6_cidr_block']})
        return super().convert_grpc(grpc_model, context=context)


class SubnetList(base_models.BaseListModel, GrpcModelMixin):
    subnets = schematics_types.ListType(schematics_types.ModelType(Subnet), required=True, default=list)


# FIXME: PublicModel?
class ContrailNetwork(common_models.Model):
    id = schematics_types.StringType(required=True)
    name = schematics_types.StringType(required=True)
    project_id = schematics_types.StringType(required=True)
    subnets = schematics_types.ListType(schematics_types.StringType)


# FIXME: PublicModel?
class ContrailPort(common_models.Model):
    id = schematics_types.StringType(required=True)
    name = schematics_types.StringType(required=True)
    # Name of network in contrail(to show it in API)
    network_id = schematics_types.StringType(required=True)
    # Uuid of network in contrail(to make interface in compute)
    contrail_network_id = schematics_types.StringType(required=True)
    vm_id = schematics_types.StringType()
    vm_name = schematics_types.StringType()
    project_id = schematics_types.StringType(required=True)
    mac_address = schematics_types.StringType(required=True)


class NetworkMetadata(operations_models.OperationMetadataV1Beta1, GrpcModelMixin):
    network_id = schematics_types.StringType()


class NetworkOperation(operations_models.OperationV1Beta1, GrpcModelMixin):
    metadata = schematics_types.ModelType(NetworkMetadata)
    response = schematics_types.ModelType(Network)


class SubnetMetadata(operations_models.OperationMetadataV1Beta1):
    subnet_id = schematics_types.StringType()


class SubnetOperation(operations_models.OperationV1Beta1, GrpcModelMixin):
    metadata = schematics_types.ModelType(SubnetMetadata)
    response = schematics_types.ModelType(Subnet)


class CreateDefaultNetworkMetadata(operations_models.OperationMetadataV1Beta1, GrpcModelMixin):
    name = schematics_types.StringType()
    create_network_operation_id = schematics_types.StringType()
    create_subnet_operation_ids = schematics_types.DictType(schematics_types.StringType)

    @classmethod
    def convert_grpc(cls, grpc_model, context=None):
        return super().convert_grpc(dict(grpc_model, create_subnet_operation_ids=None), context)


class CreateDefaultNetworkOperation(operations_models.OperationV1Beta1, GrpcModelMixin):
    metadata = schematics_types.ModelType(CreateDefaultNetworkMetadata)


class Address(base_models.BasePublicObjectModelV1Beta1, GrpcModelMixin):
    address = schematics_types.StringType()
    ip_version = IpVersionType()

    type = common_models.StringEnumType()

    subnet_id = schematics_types.StringType()
    zone_id = schematics_types.StringType()
    region_id = schematics_types.StringType()

    ephemeral = schematics_types.BooleanType()
    used = schematics_types.BooleanType()

    ddos_protection_provider = common_models.StringType()
    outgoing_smtp_capability = common_models.StringType()

    instance_id = schematics_types.StringType()
    network_load_balancer_id = schematics_types.StringType()


class AddressList(base_models.BaseListModel, GrpcModelMixin):
    addresses = schematics_types.ListType(schematics_types.ModelType(Address), required=True, default=list)


class ExternalAddressSchema(base_models.BasePublicModel, GrpcModelMixin):
    address = schematics_types.StringType()
    ip_version = IpVersionType()

    zone_id = validation.ZoneIdType()
    region_id = validation.RegionIdType()

    ddos_protection_provider = common_models.StringType()
    outgoing_smtp_capability = common_models.StringType()
    yandex_only = common_models.BooleanType()

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.vpc.v1.inner.address_service_pb2 import ExternalAddressSpec
        return ExternalAddressSpec


class InternalAddressSchema(base_models.BasePublicModel, GrpcModelMixin):
    address = schematics_types.StringType()
    ip_version = IpVersionType()

    subnet_id = validation.ResourceIdType()

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.vpc.v1.inner.address_service_pb2 import InternalAddressSpec
        return InternalAddressSpec


class AddressMetadata(operations_models.OperationMetadataV1Beta1, GrpcModelMixin):
    address_id = schematics_types.StringType()


class AddressOperation(operations_models.OperationV1Beta1, GrpcModelMixin):
    metadata = schematics_types.ModelType(AddressMetadata)
    response = schematics_types.ModelType(Address)


class FipBucketStatus:
    CREATING = "creating"
    ACTIVE = "active"
    DELETING = "deleting"

    ALL = [CREATING, ACTIVE, DELETING]
    ALL_ACTIVE = [ACTIVE]


class FipBucket(base_models.BasePublicObjectModelV1Beta1, GrpcModelMixin):
    status = common_models.StringEnumType()

    name = schematics_types.StringType(required=True)

    flavor = schematics_types.StringType(required=True)
    scope = schematics_types.StringType(required=True)
    cidrs = schematics_types.ListType(schematics_types.StringType, required=True, default=list)

    import_rts = schematics_types.ListType(schematics_types.StringType, default=list)
    export_rts = schematics_types.ListType(schematics_types.StringType, default=list)

    ip_version = IpVersionType()


class FipBucketList(base_models.BaseListModel, GrpcModelMixin):
    fip_buckets = schematics_types.ListType(schematics_types.ModelType(FipBucket), required=True, default=list)


class FipBucketMetadata(operations_models.OperationMetadataV1Beta1, GrpcModelMixin):
    bucket_id = schematics_types.StringType()


class FipBucketOperation(operations_models.OperationV1Beta1, GrpcModelMixin):
    metadata = schematics_types.ModelType(FipBucketMetadata)
    response = schematics_types.ModelType(FipBucket)


class DnsRecord(base_models.BasePublicModel):
    type = schematics_types.StringType()
    name = schematics_types.StringType()
    value = schematics_types.StringType()
    ttl = schematics_types.IntType()


class NetworkDnsRecordsList(base_models.BasePublicModel):
    extra_dns_records = schematics_types.ListType(schematics_types.ModelType(DnsRecord), default=list)


class NetworkAttachmentInfo(common_models.Model, GrpcModelMixin):
    """Model for Network attachment info for billing collector."""

    instance_id = schematics_types.StringType(required=True)
    instance_cloud_id = schematics_types.StringType(required=True)
    instance_folder_id = schematics_types.StringType(required=True)
    network_cloud_id = schematics_types.StringType(required=True)  # subnet cloud ID
    network_folder_id = schematics_types.StringType(required=True)   # subnet folder ID
    network_id = schematics_types.StringType()
    subnet_id = schematics_types.StringType()
    # This fields are preparation for billing detailization
    # of vms, that are in fact parts of PAAS infrastructure
    # used by our customers (like DBAAS or maybe Marketplace or others)
    # they are disabled now
    detailization_type = schematics_types.StringType()
    detailization_id = schematics_types.StringType()
    # Instance's labels
    labels = schematics_types.DictType(schematics_types.StringType, export_level=schematics_common.NONEMPTY)
    # Instance removed from this compute node timestamp
    removed_at = schematics_types.IntType()


class StaticRouteKeySchema(base_models.BasePublicModel):
    """
    Static route key schema for update requests
    See: https://bb.yandex-team.ru/projects/CLOUD/repos/private-api/browse/yandex/cloud/priv/vpc/v1/route_table_service.proto#125
    """
    destination_prefix = common_models.IPNetworkType(required=True)
    next_hop_address = common_models.IPAddressType(required=True)


class StaticRouteSchema(base_models.BasePublicModel, GrpcModelMixin):
    """
    Static route schema for requests
    See: https://bb.yandex-team.ru/projects/CLOUD/repos/private-api/browse/yandex/cloud/priv/vpc/v1/route_table.proto#22
    """
    destination_prefix = common_models.IPNetworkType(required=True)
    next_hop_address = common_models.IPAddressType(required=True)
    labels = schematics_types.DictType(schematics_types.StringType)

    _IP_VERSION_INT_TO_STR_MAP = {
        4: IpVersion.IPV4,
        6: IpVersion.IPV6,
    }

    @property
    def ip_version(self) -> str:
        return self._IP_VERSION_INT_TO_STR_MAP[self.destination_prefix.version]

    def validate_destination_prefix(self, data, value):
        validate_destination_prefix = value
        next_hop_address = data.get('next_hop_address')
        if validate_destination_prefix and next_hop_address and validate_destination_prefix.version != next_hop_address.version:
            raise ValidationError("Static route destination_prefix and next_hop_address version mismatch")

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.vpc.v1.route_table_pb2 import StaticRoute
        return StaticRoute


class StaticRoute(base_models.BasePublicModel, GrpcModelMixin):
    """
    Static route structure
    See: https://bb.yandex-team.ru/projects/CLOUD/repos/private-api/browse/yandex/cloud/priv/vpc/v1/route_table.proto#22
    """
    destination_prefix = schematics_types.StringType(required=True)
    next_hop_address = schematics_types.StringType(required=True)
    labels = schematics_types.DictType(schematics_types.StringType)

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.vpc.v1.route_table_pb2 import StaticRoute
        return StaticRoute


class RouteTable(base_models.BasePublicObjectModelV1Beta1, GrpcModelMixin):
    """
    Route table structure. Combines several static routes in one entity.
    See: https://bb.yandex-team.ru/projects/CLOUD/repos/private-api/browse/yandex/cloud/priv/vpc/v1/route_table.proto#10
    """
    network_id = schematics_types.StringType(required=True)
    static_routes = schematics_types.ListType(schematics_types.ModelType(StaticRoute), required=True, default=list)


class RouteTableList(base_models.BaseListModel, GrpcModelMixin):
    route_tables = schematics_types.ListType(schematics_types.ModelType(RouteTable), required=True, default=list)


class RouteTableMetadata(operations_models.OperationMetadataV1Beta1):
    route_table_id = schematics_types.StringType()


class RouteTableOperation(operations_models.OperationV1Beta1, GrpcModelMixin):
    metadata = schematics_types.ModelType(RouteTableMetadata)
    response = schematics_types.ModelType(RouteTable)


# TODO(lavrukov): remove after transition period
class ConsoleVpcObjectsNetwork(Network):
    can_create_subnet = common_models.BooleanType(default=False)


# TODO(lavrukov): remove after transition period
class ConsoleVpcObjectsResponse(base_models.BasePublicModel):
    subnets = common_models.ListType(common_models.ModelType(Subnet))
    route_tables = common_models.ListType(common_models.ModelType(RouteTable))
    networks = common_models.ListType(common_models.ModelType(ConsoleVpcObjectsNetwork))

    @classmethod
    def read_mask_fields(cls):
        return [f.name for f in cls.fields.values()]


class FolderStats(common_models.Model):
    external_address_count = schematics_types.IntType()
    network_count = schematics_types.IntType()
    network_load_balancer_count = schematics_types.IntType()
    target_group_count = schematics_types.IntType()
    route_table_count = schematics_types.IntType()
    subnet_count = schematics_types.IntType()


# NetworkInterfaceAttachmentService models

class NetworkInterfaceInternalInfo(common_models.Model, GrpcModelMixin):
    vm_name = schematics_types.StringType()
    subnet_cloud_id = schematics_types.StringType()
    subnet_folder_id = schematics_types.StringType()
    feature_flags = schematics_types.ListType(schematics_types.StringType)
    hbf_enabled = schematics_types.BooleanType()
    sdn_port_id = schematics_types.StringType()
    sdn_vnet_id = schematics_types.StringType()
    sdn_vm_id = schematics_types.StringType()
    sdn_project_id = schematics_types.StringType()


class NetworkInterfaceAttachment(common_models.Model, GrpcModelMixin):
    instance_id = schematics_types.StringType()
    interface_index = schematics_types.StringType()
    subnet_id = schematics_types.StringType()
    network_id = schematics_types.StringType()
    primary_v4_address = schematics_types.ModelType(PrimaryAddress)
    primary_v6_address = schematics_types.ModelType(PrimaryAddress)
    security_group_ids = schematics_types.ListType(schematics_types.StringType)
    internal_info = schematics_types.ModelType(NetworkInterfaceInternalInfo)


class GetNetworkInterfaceAttachmentResponse(common_models.Model, GrpcModelMixin):
    network_interface_attachments = schematics_types.ListType(schematics_types.ModelType(NetworkInterfaceAttachment))


class NetworkInterfaceAttachmentMetadata(common_models.Model, GrpcModelMixin):
    network_interface_attachments = schematics_types.ListType(schematics_types.ModelType(NetworkInterfaceAttachment))


class NetworkInterfaceAttachmentOperation(operations_models.OperationV1Beta1, GrpcModelMixin):
    metadata = schematics_types.ModelType(NetworkInterfaceAttachmentMetadata)


class ComputeNetworkInterface(common_models.Model, GrpcModelMixin):
    id = schematics_types.StringType(required=True)
    mac_address = schematics_types.StringType()

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.vpc.v1.inner.network_interface_attachment_service_pb2 import ComputeNetworkInterface
        return ComputeNetworkInterface


class InstanceNetworkContext(common_models.Model, GrpcModelMixin):
    instance_id = schematics_types.StringType(required=True)
    folder_id = schematics_types.StringType(required=True)
    cloud_id = schematics_types.StringType(required=True)
    zone_id = schematics_types.StringType(required=True)

    fqdn = schematics_types.StringType()
    search_domains = schematics_types.ListType(schematics_types.StringType)

    network_interfaces = schematics_types.ListType(schematics_types.ModelType(ComputeNetworkInterface))

    @classmethod
    def get_grpc_class(cls):
        from yandex.cloud.priv.vpc.v1.inner.network_interface_attachment_service_pb2 import InstanceNetworkContext
        return InstanceNetworkContext


class ComputeAuthzRequest(common_models.Model, GrpcModelMixin):
    service_name = schematics_types.StringType()
    main_action = schematics_types.StringType()
    cloud_id = schematics_types.StringType()
    folder_id = schematics_types.StringType()
    target_id = schematics_types.StringType()


class ComputeAuthzRequestList(common_models.Model, GrpcModelMixin):
    authz_requests = schematics_types.ListType(schematics_types.ModelType(ComputeAuthzRequest))
