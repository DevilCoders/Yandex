"""
Functions to build Contrail object names (last part of fq_name).
"""
# noinspection PyPep8Naming
from typing import List, Tuple

from yc_common.clients.network.models.virtual_network import VirtualDnsRecordType as vrt

# Note: extract prefix to const if used more than once.
FIP_PREFIX = "fip-"
FIP_POOL_PREFIX = "fip-pool-"


def vnet(compute_network_id):
    return "vnet-{}".format(compute_network_id)


def ipam(compute_network_id):
    return "ipam-{}".format(compute_network_id)


def vdns(compute_network_id):
    return "vdns-{}".format(compute_network_id)


# NOTE(andgein): It's important to keep naming correspondent with contrail-dns logic. See CLOUD-35099 for details.
# Basically, proxy vDNS name should differ from vDNS name by adding "proxy-" substring.
def proxy_vdns(compute_network_id):
    return "proxy-vdns-{}".format(compute_network_id)


def vm(compute_instance_id):
    return "vm-{}".format(compute_instance_id)


def port(compute_instance_id, port_uuid):
    return "port-{}-{}".format(compute_instance_id, port_uuid)


def ip(compute_instance_id, port_uuid, ip_address):
    return "ip-{}-{}-{}".format(compute_instance_id, port_uuid, _ip_to_name(ip_address))


def dns_record(record_type, name, value, compute_instance_id=None):
    if compute_instance_id is None or compute_instance_id == "":
        # Extra DNS records do not belong to any instance. They are created per network. CLOUD-15348
        compute_instance_id = "extra"

    if record_type == vrt.RecordType.PTR_TYPE:
        return "vdns-rec-{}-{}-{}-{}".format(compute_instance_id, record_type, value, _ip_to_name(name))
    elif record_type in (vrt.RecordType.A_TYPE, vrt.RecordType.AAAA_TYPE):
        return "vdns-rec-{}-{}-{}-{}".format(compute_instance_id, record_type, name, _ip_to_name(value))
    elif record_type == vrt.RecordType.CNAME_TYPE:
        return "vdns-rec-{}-{}-{}".format(compute_instance_id, record_type, name)
    else:
        raise NotImplementedError("record_type {!r} is not supported".format(record_type))


def fip(compute_instance_id, port_uuid, ip_address):
    return "{}{}-{}-{}".format(FIP_PREFIX, compute_instance_id, port_uuid, _ip_to_name(ip_address))


def parse_fip(fq_name: str) -> Tuple[str, str, str]:
    """
    Parses Contrail's FIP fq_name to (compute_instance_id, port_uuid, ip_address)
    """
    if not fq_name.startswith(FIP_PREFIX):
        raise ValueError("Invalid prefix (expected: 'fip-', actual: {!r})".format(fq_name))

    tokens = fq_name.split("-")

    return tokens[1], "-".join(tokens[2:7]), _name_to_ip(tokens[7:])


def fip_pool(fip_bucket_name):
    return "{}{}".format(FIP_POOL_PREFIX, fip_bucket_name)


def parse_fip_pool(fq_name: str):
    """
    Extracts FIP bucket name from Contrail's FIP pool fq_name.
    """
    if not fq_name.startswith(FIP_POOL_PREFIX):
        raise ValueError("Invalid prefix (expected: {!r}, actual: {!r})".format(FIP_POOL_PREFIX, fq_name))

    return fq_name[len(FIP_POOL_PREFIX):]


def fip_vnet(fip_bucket_name):
    return "fip-vnet-{}".format(fip_bucket_name)


def fip_ipam(fip_bucket_name):
    return "fip-ipam-{}".format(fip_bucket_name)


def _ip_to_name(ip_address):
    return str(ip_address).translate(str.maketrans(":./", "---"))


def _is_valid_ipv4_octet(value):
    try:
        return 0 <= int(value) <= 255
    except ValueError:
        return False


def _name_to_ip(name_tokens):
    if len(name_tokens) == 4 and all(_is_valid_ipv4_octet(t) for t in name_tokens):
        return ".".join(name_tokens)
    return ":".join(name_tokens)


def acl(name, subnet_id):
    return "acl-{}-{}".format(name, subnet_id)


def routing_instance(name):
    return "ri-{}".format(name)


def route_target(rt):
    return "target:{}".format(rt)


def route_table(compute_route_table_id):
    return "rtable-{}".format(compute_route_table_id)


def route_table_id_from_name(contrail_route_table_name: str) -> str:
    return contrail_route_table_name.replace("rtable-", '', 1)


def route_table_id_from_fq_name(contrail_route_table_fq_name: List[str]) -> str:
    return route_table_id_from_name(contrail_route_table_fq_name[-1])


def routing_policy_type_id(type_prefix: str, policy_key: str) -> str:
    return "{}-{}".format(type_prefix, policy_key)


def routing_policy_accept_all_type(policy_key: str) -> str:
    return routing_policy_type_id("accept-all", policy_key)


def routing_policy_reject_static_routes_type(policy_key: str) -> str:
    return routing_policy_type_id("reject-static-routes", policy_key)


def routing_policy(policy_type_id: str, policy_action: str) -> str:
    return "rpolicy-{}-{}".format(policy_type_id, policy_action)
