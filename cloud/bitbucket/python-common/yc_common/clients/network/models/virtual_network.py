from schematics.types import StringType, ListType, BooleanType, IntType, ModelType

from yc_common.models import Model


# Virtual Network related models
class VirtualNetworkFeatureFlags(Model):
    """
    VN feature flags
    See: https://wiki.yandex-team.ru/cloud/devel/sdn/vrouter-dp-featureflags/#spisokrealizovannyxficheflagov
    """
    class FeatureFlags:
        HARDENED_PUBLIC_IP = "hardened-public-ip"
        BLACKHOLE = "blackhole"
        STATIC_ROUTE_OVERRIDE_FIP = "static-route-override-fip"
        ALL = [HARDENED_PUBLIC_IP, BLACKHOLE, STATIC_ROUTE_OVERRIDE_FIP]

    flag = ListType(StringType, default=list, choices=FeatureFlags.ALL)


class VirtualNetworkType(Model):
    class ForwardingMode:
        L2L3_MODE = "l2_l3"
        L3_MODE = "l3"
        ALL = [L2L3_MODE, L3_MODE]

    allow_transit = StringType()
    forwarding_mode = StringType(required=True, choices=ForwardingMode.ALL)
    mirror_destination = BooleanType(default=False)
    network_id = StringType()
    rpf = BooleanType()
    vxlan_network_identifier = StringType()
    feature_flag_list = ModelType(VirtualNetworkFeatureFlags)


# Virtual Network <-> Network Ipam reference related models
class SubnetType(Model):
    ip_prefix = StringType()
    ip_prefix_len = IntType()


class IpamSubnetType(Model):
    addr_from_start = BooleanType(default=True)
    dhcp_option_list = StringType()
    host_routes = StringType()
    subnet_name = StringType()
    allocation_pools = ListType(StringType, default=list)
    dns_nameservers = ListType(StringType, default=list)
    enable_dhcp = BooleanType(default=True)
    subnet = ModelType(SubnetType)
    dns_server_address = StringType(serialize_when_none=False)
    default_gateway = StringType(serialize_when_none=False)


class VnSubnetsType(Model):
    ipam_subnets = ListType(ModelType(IpamSubnetType))
    host_routes = StringType()


# Network Ipam related models
class IpamDnsAddressType(Model):
    tenant_dns_server_address = StringType(required=True)
    virtual_dns_server_name = StringType()


class IpamType(Model):
    cidr_block = StringType()
    dhcp_option_list = StringType()
    host_routes = StringType()
    ipam_dns_method = StringType(default="virtual-dns-server")
    ipam_dns_server = ModelType(IpamDnsAddressType)
    ipam_method = StringType()


# Virtual DNS Record related models
class VirtualDnsRecordType(Model):
    class RecordClass:
        IN_CLASS = "IN"
        ALL = [IN_CLASS]

    class RecordType:
        A_TYPE = "A"
        AAAA_TYPE = "AAAA"
        CNAME_TYPE = "CNAME"
        PTR_TYPE = "PTR"
        NS_TYPE = "NS"
        MX_TYPE = "MX"
        ALL = [A_TYPE, AAAA_TYPE, CNAME_TYPE, PTR_TYPE, NS_TYPE, MX_TYPE]

    record_type = StringType(choices=RecordType.ALL, required=True)
    record_ttl_seconds = IntType(required=True)
    record_class = StringType(choices=RecordClass.ALL, required=True)
    record_data = StringType(required=True)
    record_name = StringType(required=True)
    record_mx_preference = StringType()


# Virtual Machine Interface related models
class PortType(Model):
    start_port = IntType(min_value=0, default=0)
    end_port = IntType(max_value=65535, default=65535)


class EphemeralPortType(Model):
    start_port = IntType(min_value=0, default=1024)
    end_port = IntType(max_value=65535, default=65535)


class VirtualMachineInterfaceFeatureFlags(Model):
    """
    VMI feature flags
    See: https://wiki.yandex-team.ru/cloud/devel/sdn/vrouter-dp-featureflags/#spisokrealizovannyxficheflagov
    """
    class FeatureFlags:
        SUPER_FLOW_V1 = "super-flow-v1"
        SUPER_FLOW_V2 = "super-flow-v2"
        ECMP_HASH_EACH_PACKET = "ecmp-hash-each-packet"
        VMI_METADATA_CONNECTION_LIMITER = "vmi-metadata-connection-limiter"
        VMI_REDUCE_FLOW_LOG_EVENT_SIZE = "vmi-reduce-flow-log-event-size"
        VMI_DROP_UNKNOWN_MULTICAST = "vmi-drop-unknown-multicast"
        VMI_DROP_L2_ADVERTISEMENTS = "vmi-drop-l2-advertisements"
        VMI_DONT_APPEND_SEARCH_DOMAIN_FOR_DNS_REQUESTS = "vmi-dont-append-search-domain-for-dns-requests"
        ALL = [
            SUPER_FLOW_V1,
            SUPER_FLOW_V2,
            ECMP_HASH_EACH_PACKET,
            VMI_METADATA_CONNECTION_LIMITER,
            VMI_REDUCE_FLOW_LOG_EVENT_SIZE,
            VMI_DROP_UNKNOWN_MULTICAST,
            VMI_DROP_L2_ADVERTISEMENTS,
            VMI_DONT_APPEND_SEARCH_DOMAIN_FOR_DNS_REQUESTS,
        ]

    flag = ListType(StringType, default=list, choices=FeatureFlags.ALL)


# TODO: support all VirtualMachineInterfacePropertiesType fields from vnc_cfg.xsd
class VirtualMachineInterfacePropertiesType(Model):
    feature_flag_list = ModelType(VirtualMachineInterfaceFeatureFlags)
    super_flow_v1_ports = ModelType(EphemeralPortType)


class DhcpOptionType(Model):
    dhcp_option_name = StringType(required=True)
    dhcp_option_value = StringType(required=True)
    dhcp_option_value_bytes = StringType()


class DhcpOptionListType(Model):
    dhcp_option = ListType(ModelType(DhcpOptionType), default=list)


# Floating IP related models
class FloatingIpDescriptor(Model):
    fip_bucket_name = StringType(required=True)
    floating_ip_address = StringType(required=True)


class FloatingIpBinding(Model):
    fixed_ip_address = StringType(required=True)
    floating_ip_descriptors = ListType(ModelType(FloatingIpDescriptor))
