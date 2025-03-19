import socket

from schematics.types import StringType, ListType, BooleanType, IntType, ModelType, DateTimeType

from yc_common.models import Model
from yc_common.clients.network.models.virtual_network import SubnetType, PortType


# Network Acl related models
class AddressType(Model):
    virtual_network = StringType()
    security_group = StringType()
    network_policy = StringType()
    subnet = ModelType(SubnetType)
    subnet_list = ListType(ModelType(SubnetType))


class EtherType:
    IPv4 = "IPv4"
    IPv6 = "IPv6"

    ALL = [IPv4, IPv6]


class MatchConditionType(Model):
    protocol = StringType(required=True)
    src_address = ModelType(AddressType, required=True)
    src_port = ModelType(PortType, required=True)
    dst_address = ModelType(AddressType, required=True)
    dst_port = ModelType(PortType, required=True)
    ethertype = StringType(choices=EtherType.ALL, required=True)


class StaticMirrorNhType(Model):
    vtep_dst_ip_address = StringType(required=True)
    vtep_dst_mac_address = StringType()
    vni = IntType(required=True)


class MirrorActionType(Model):
    class NHModeType:
        DYNAMIC = "dynamic"
        STATIC = "static"

        ALL = [DYNAMIC, STATIC]

    analyzer_name = StringType()
    encapsulation = StringType()
    analyzer_ip_address = StringType()
    analyzer_mac_address = StringType()
    routing_instance = StringType(required=True)
    udp_port = IntType()
    juniper_header = BooleanType(default=True)
    nh_mode = StringType(choices=NHModeType.ALL)
    static_nh_header = ModelType(StaticMirrorNhType)


class SimpleActionType:
    DENY = "deny"
    PASS = "pass"

    ALL = [DENY, PASS]


class ActionListType(Model):
    simple_action = StringType(choices=SimpleActionType.ALL, required=True)
    gateway_name = StringType()
    apply_service = ListType(StringType)
    mirror_to = ModelType(MirrorActionType)
    assign_routing_instance = StringType()
    log = BooleanType(default=False)
    alert = BooleanType(default=False)
    qos_action = StringType()


class AclRuleType(Model):
    match_condition = ModelType(MatchConditionType, required=True)
    action_list = ModelType(ActionListType, required=True)
    rule_uuid = StringType()


class AclEntriesType(Model):
    dynamic = BooleanType()
    acl_rule = ListType(ModelType(AclRuleType), required=True)


# Security Groups related models
class SequenceType(Model):
    major = IntType(required=True)
    minor = IntType(required=True)


class DirectionType:
    GT = ">"
    LTGT = "<>"

    ALL = [GT, LTGT]


class PolicyRuleType(Model):
    rule_sequence = ModelType(SequenceType)
    rule_uuid = StringType()
    direction = StringType(choices=DirectionType.ALL)
    protocol = StringType(required=True)
    src_addresses = ListType(ModelType(AddressType), required=True)
    src_ports = ListType(ModelType(PortType), required=True)
    application = ListType(StringType)
    dst_addresses = ListType(ModelType(AddressType), required=True)
    dst_ports = ListType(ModelType(PortType), required=True)
    action_list = ModelType(ActionListType, required=True)
    ethertype = StringType(choices=EtherType.ALL)


class PolicyEntriesType(Model):
    policy_rule = ListType(ModelType(PolicyRuleType), required=True)


class TimerType(Model):
    # xsd:dateTime + xsd:time: https://www.w3schools.com/xml/schema_dtypes_date.asp
    # ISO-8601

    start_time = DateTimeType(required=True)
    on_interval = DateTimeType(required=True, serialized_format="%H:%M:%S")
    off_interval = DateTimeType(required=True, serialized_format="%H:%M:%S")
    end_time = DateTimeType(required=True)


class VirtualNetworkPolicyType(Model):
    sequence = ModelType(SequenceType)
    timer = ModelType(TimerType)


# Iana protocol numbers
# https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
class Protocol:
    TCP = socket.IPPROTO_TCP
    UDP = socket.IPPROTO_UDP

    ANY = "any"


class InstanceTargetType(Model):
    class ImportExport:
        EXPORT = "export"
        IMPORT = "import"

        ALL = [EXPORT, IMPORT]

    import_export = StringType(choices=ImportExport.ALL)


class PolicyBasedForwardingRuleType(Model):
    class TrafficDirection:
        INGRESS = "ingress"
        EGRESS = "egress"
        BOTH = "both"

        ALL = [INGRESS, EGRESS, BOTH]

    direction = StringType(choices=TrafficDirection.ALL)
    vlan_tag = IntType()
    src_mac = StringType()
    dst_mac = StringType()
    mpls_label = IntType()
    service_chain_address = StringType()
    ipv6_service_chain_address = StringType()
    protocol = StringType()
