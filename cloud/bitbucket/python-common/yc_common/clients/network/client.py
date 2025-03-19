import functools
import ipaddress
import random
import time

from copy import deepcopy
from http import HTTPStatus
from itertools import chain
from typing import Iterable, List, Optional, Union, Tuple, Dict, TypeVar

from yc_common import metrics, constants
from yc_common.misc import timeout_iter
from yc_common.clients.contrail import client, context, exceptions, schema, resource
from yc_common.clients.contrail.resource import Resource
from yc_common.clients.models.networks import ContrailNetwork as Network, ContrailPort, DnsRecord
from yc_common.clients.network import naming
from yc_common.clients.network.models.route_table import (
    RouteType, RouteTableType, TermActionListType, CommunityAttributes, SequenceType,
    PolicyStatementType, PolicyTermType, TermMatchConditionType, PrefixMatchType,
)
from yc_common.clients.network.models.virtual_network import (
    SubnetType,
    VirtualNetworkFeatureFlags,
    VirtualNetworkType,
    FloatingIpBinding,
    DhcpOptionType,
    DhcpOptionListType,
    VnSubnetsType,
    IpamType,
    IpamDnsAddressType,
    IpamSubnetType,
    PortType,
    VirtualMachineInterfaceFeatureFlags,
    VirtualMachineInterfacePropertiesType,
    VirtualDnsRecordType,
)
from yc_common.clients.network.models.network_acl import (
    ActionListType, MatchConditionType, AclRuleType, AclEntriesType,
    PolicyBasedForwardingRuleType, InstanceTargetType, AddressType,
)
from yc_common.exceptions import Error
from yc_common.logging import get_logger


# Service networks, that we don't show to users
SERVICE_NETWORKS = [
    '__link_local__',
    'ip-fabric',
    'default-virtual-network',
]

_DEFAULT_DOMAIN = "default-domain"
_DEFAULT_PROJECT = _DEFAULT_DOMAIN + ":default-project"
_DEFAULT_IPAM = _DEFAULT_PROJECT + ":default-network-ipam"
_EXTERNAL_NETWORK = _DEFAULT_PROJECT + ":__external__"

# TODO: CLOUD-15739: Make this config value in compute or stay here
_STATIC_ROUTE_BGP_COMMUNITY = "63333:1"

_BGP_RTGT_MIN_ID = 8000000
_WAIT_ROUTING_INSTANCE_TIMEOUT = 60

T = TypeVar("T")


log = get_logger(__name__)

network_methods_latency = metrics.Metric(
    metrics.MetricTypes.HISTOGRAM,
    "network_method_latency",
    ["method", "status"],
    "Network functions calls latency histogram. ms",
    buckets=metrics.time_buckets_ms)


def measure_latency(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        start = time.monotonic()

        def measure(status):
            duration_ms = (time.monotonic() - start) * constants.SECOND_MILLISECONDS
            network_methods_latency.labels(func.__name__, status).observe(duration_ms)
        try:
            result = func(*args, **kwargs)
        except Exception as e:
            measure("exception_raised: " + type(e).__name__)
            raise
        measure("complete")
        return result
    return wrapper


def _make_fqname(parent_fqname, name):
    return "{}:{}".format(parent_fqname, name)


def _subnet_to_ip_network(contrail_subnet):
    return ipaddress.ip_network("{}/{}".format(contrail_subnet["ip_prefix"], contrail_subnet["ip_prefix_len"]))


def _is_ip_address(s):
    try:
        ipaddress.ip_address(s)
        return True
    except ValueError:
        return False


def _make_ipam_subnet(cidr: str, has_default_gateway=True):
    ip_network = ipaddress.ip_network(cidr)

    prefix = str(ip_network.network_address)
    prefix_len = ip_network.prefixlen

    subnet = SubnetType.new(ip_prefix=prefix, ip_prefix_len=prefix_len)

    ipam_subnet = IpamSubnetType.new(subnet=subnet)
    if has_default_gateway:
        ipam_subnet.default_gateway = str(ip_network[1])
        ipam_subnet.dns_server_address = str(ip_network[2])

    return ipam_subnet, prefix, prefix_len


class NativeNetworkClient:
    def __init__(self,
                 host: str,
                 port: int,
                 protocol: str,
                 schema_version: str,
                 request_id: str = None,
                 operation_id: str = None,
                 ):
        self.context = context.Context()
        self.context.session = client.ContrailAPISession(
            host, port, protocol, request_id, operation_id
        )
        if schema_version:
            self.context.schema = schema.create_schema_from_version(
                schema_version)
        else:
            self.context.schema = schema.DummySchema(self.context)

    def resource(self, type, *args, **kwargs) -> Resource:
        """Create new or fetch object from contrail.

        :param type: type of the resource
        :type type: str
        :param fetch: immediately fetch resource from the server
        :type fetch: bool
        :param uuid: uuid of the resource
        :type uuid: v4UUID str
        :param fq_name: fq name of the resource
        :type fq_name: str (domain:project:identifier)
                   or list ['domain', 'project', 'identifier']
        :param check: check that the resource exists
        :type check: bool
        :param parent: parent resource
        :type parent: Resource
        :param recursive: level of recursion
        :type recursive: int

        :raises ResourceNotFound: bad uuid or fq_name is given
        :raises HttpError: when save(), fetch() or delete() fail
        """
        return Resource(self.context, type, *args, **kwargs)

    def collection(self, type, *args, **kwargs):
        """Fetch a bunch of resources from contrail.

        :param type: name of the collection
        :type type: str
        :param fetch: immediately fetch collection from the server
        :type fetch: bool
        :param recursive: level of recursion
        :type recursive: int
        :param fields: list of field names to fetch
        :type fields: [str]
        :param filters: list of filters
        :type filters: [(name, value), ...]
        :param parent_uuid: filter by parent_uuid
        :type parent_uuid: v4UUID str or list of v4UUID str
        :param back_ref_uuid: filter by back_ref_uuid
        :type back_ref_uuid: v4UUID str or list of v4UUID str
        :param data: initial resources of the collection
        :type data: [Resource]

        """
        return resource.Collection(self.context, type, *args, **kwargs)

    def _is_service_network(self, name):
        return name in SERVICE_NETWORKS

    def _format_subnet(self, subnet):
        return subnet['ip_prefix'] + '/' + str(subnet['ip_prefix_len'])

    def iter_networks(self) -> Iterable[Network]:
        vnets = self.collection('virtual-network', fetch=True, recursive=2)
        for vnet in vnets:
            if self._is_service_network(vnet['name']):
                continue

            yield self._make_network(vnet)

    def _make_network(self, vnet):
        subnet_refs = vnet.get('network_ipam_refs', [])
        subnets = []
        for ref in subnet_refs:
            ipam_subnets = ref['attr']['ipam_subnets']
            subnets.extend([self._format_subnet(subnet['subnet'])
                            for subnet in ipam_subnets])
        return Network.from_api({
            'id': vnet['uuid'],
            'name': vnet['name'],
            'project_id': vnet['parent_uuid'],
            'subnets': subnets,
        })

    def _get_project(self, project_fqname=_DEFAULT_PROJECT):
        return self.resource("project", fq_name=project_fqname)

    def _get_domain(self, domain_fqname=_DEFAULT_DOMAIN):
        return self.resource("domain", fq_name=domain_fqname)

    def _get_referred_ipam(self, network):
        net_ipam_refs = network.refs.network_ipam
        if not net_ipam_refs:
            raise Error("Failed to get IPAM for network {!r}: no IPAMs referred.", network.uuid)

        net_ipam = net_ipam_refs[0].fetch()
        # This fetch is here because network resource
        # after fetching its refs looses ref attributes
        # but we may need it later
        network.fetch()
        return net_ipam

    def _try_get_ipam_vdns(self, ipam):
        vdns_ref = ipam.refs.virtual_DNS
        if vdns_ref:
            return vdns_ref[0].fetch()
        else:
            return None

    def _build_vdns_data(self, reverse_resolution: bool, domain: str=None, next_vdns: str=None):
        default_ipam = self.resource("network-ipam", fq_name=_DEFAULT_IPAM, fetch=True)
        try:
            default_vdns = self._try_get_ipam_vdns(default_ipam)
            if not default_vdns:
                raise Error("IPAM {!r} has no vDNS", _DEFAULT_IPAM)

            data = deepcopy(default_vdns.virtual_DNS_data)

            data["external_visible"] = False
            data["reverse_resolution"] = reverse_resolution
            if next_vdns is not None:
                data["next_virtual_DNS"] = next_vdns
            if domain is not None:
                data["domain_name"] = domain

            return data
        except Error:
            # TODO: Add correct exception to raise
            raise Error("Failed to get virtual DNS: no default vdns provided.")

    def _create_raw_vnet(self,
                         network_uuid: str,
                         name: str,
                         display_name: str,
                         forwarding_mode: str,
                         rpf: str,
                         feature_flags: List[str] = None,
                         ) -> Resource:
        """Create raw virtual network object in contrail."""

        project = self._get_project()
        net_properties = VirtualNetworkType.new(
            forwarding_mode=forwarding_mode,
            rpf=rpf,
            feature_flag_list=VirtualNetworkFeatureFlags.new(flag=feature_flags),
        )
        network = self.resource(
            "virtual-network", uuid=network_uuid, fq_name=list(project.fq_name) + [name], display_name=display_name,
            parent=project, virtual_network_properties=net_properties.to_primitive(),
            port_security_enabled=True, flood_unknown_unicast=False,
        )
        try:
            network.create()
        except exceptions.HttpError as e:
            if e.http_status == HTTPStatus.CONFLICT:
                network = self.resource("virtual-network", uuid=network_uuid, fetch=True)
            else:
                # FIXME: Add wrap exception to raise
                raise e

        return network

    def _create_raw_ipam(self, ipam_name: str, vdns_name: Optional[str]):
        """Create raw network ipam object in contrail."""

        project = self._get_project()

        if vdns_name is not None:
            ipam_dns = IpamDnsAddressType.new(virtual_dns_server_name=_make_fqname(_DEFAULT_DOMAIN, vdns_name))
            net_ipam_mgmt = IpamType.new(ipam_dns_server=ipam_dns)
        else:
            net_ipam_mgmt = IpamType.new(ipam_dns_method="none")

        ipam_fq_name = list(project.fq_name) + [ipam_name]

        net_ipam = self.resource(
            "network-ipam", fq_name=ipam_fq_name,
            parent=project, network_ipam_mgmt=net_ipam_mgmt.to_primitive()
        )
        try:
            net_ipam.create()
        except exceptions.HttpError as e:
            if e.http_status == HTTPStatus.CONFLICT:
                net_ipam = self.resource("network-ipam", fq_name=ipam_fq_name, fetch=True)
            else:
                # FIXME: Add wrap exception to raise
                raise e

        return net_ipam

    @measure_latency
    def create_subnet(self,
                      network_uuid: str,
                      compute_network_id: str,
                      display_name: str,
                      cidrs: List[str] = None,
                      forwarding_mode: str = VirtualNetworkType.ForwardingMode.L3_MODE,
                      export_rts: List[str] = None,
                      import_rts: List[str] = None,
                      next_vdns: str = None,
                      proxy_vdns_domain: str = None,
                      rpf_enabled: bool = False,
                      feature_flags: List[str] = None,
                      ) -> Resource:
        """Create subnet for user network in contrail."""

        log.debug("Creating subnet: network_uuid=%r, compute_network_id=%r, display_name=%r, cidrs=%r, " +
                  "forwarding_mode=%r, export_rts=%r, import_rts=%r, next_vdns=%r, proxy_vdns_domain=%r, " +
                  "feature_flags=%r, ...",
                  network_uuid, compute_network_id, display_name, cidrs,
                  forwarding_mode, export_rts, import_rts, next_vdns,
                  proxy_vdns_domain, feature_flags)

        vnet_name = naming.vnet(compute_network_id)
        ipam_name = naming.ipam(compute_network_id)
        vdns_name = naming.vdns(compute_network_id)

        network = self._create_vnet_with_ipam(
            network_uuid, vnet_name, ipam_name, vdns_name, display_name,
            cidrs, forwarding_mode,
            has_default_gateway=True,
            rpf_enabled=rpf_enabled,
            feature_flags=feature_flags,
        )

        if proxy_vdns_domain:
            proxy_vdns = self._create_proxy_vdns(compute_network_id, proxy_vdns_domain, next_vdns)
            next_vdns = ":".join(proxy_vdns.fq_name)

        self._create_vdns(compute_network_id, network, next_vdns)

        if export_rts or import_rts:
            self._set_network_route_targets(network, import_rts or [], export_rts or [])

        routing_policies = self._get_or_create_network_default_routing_policies(policy_key=network_uuid)
        self.set_network_routing_policies(network_uuid, routing_policies)

        return network

    def _create_vnet_with_ipam(self,
                               network_uuid: str,
                               vnet_name: str,
                               ipam_name: str,
                               vdns_name: Optional[str],
                               vnet_display_name: str,
                               cidrs: List[str],
                               forwarding_mode: str,
                               has_default_gateway: bool = True,
                               rpf_enabled: bool = False,
                               feature_flags: List[str] = None,
                               ) -> Resource:
        """Create network with ipam reference in contrail."""

        rpf = 'enable' if rpf_enabled else 'disable'
        network = self._create_raw_vnet(network_uuid, vnet_name, vnet_display_name, forwarding_mode, rpf,
                                        feature_flags=feature_flags)
        net_ipam = self._create_raw_ipam(ipam_name, vdns_name)

        if network.refs.network_ipam and network.refs.network_ipam[0].fq_name == net_ipam.fq_name:
            return network

        ipam_subnets = []
        for cidr in cidrs or []:
            ipam_subnet, _, _ = _make_ipam_subnet(cidr, has_default_gateway)
            ipam_subnets.append(ipam_subnet)

        reference_attributes = VnSubnetsType.new(ipam_subnets=ipam_subnets)
        try:
            network.add_ref(net_ipam, attr=reference_attributes.to_primitive())
        except exceptions.HttpError:
            # TODO: Add wrap exception to raise
            raise

        return network

    @measure_latency
    def create_fip_network(self, network_uuid, fip_bucket_name, cidrs, export_rts=None, import_rts=None):
        log.debug("Creating Floating IP network: network_uuid=%r, fip_bucket_name=%r, cidrs=%r, "
                  "export_rts=%r, import_rts=%r ...", network_uuid, fip_bucket_name, cidrs, export_rts, import_rts)

        vnet_name = naming.fip_vnet(fip_bucket_name)
        ipam_name = naming.fip_ipam(fip_bucket_name)

        network = self._create_vnet_with_ipam(network_uuid, vnet_name, ipam_name, vdns_name=None,
                                              vnet_display_name=vnet_name, cidrs=cidrs,
                                              forwarding_mode=VirtualNetworkType.ForwardingMode.L3_MODE,
                                              rpf_enabled=False, has_default_gateway=False)
        self._create_fip_pool(network, fip_bucket_name)

        if export_rts or import_rts:
            self._set_network_route_targets(network, import_rts or [], export_rts or [])

        return network

    def _create_fip_pool(self, network, fip_bucket_name):
        log.debug("Adding Floating IP pool %r for network %r ...", fip_bucket_name, network.uuid)

        fip_pool_name = naming.fip_pool(fip_bucket_name)
        fip_pool = self.resource("floating-ip-pool", fq_name=list(network.fq_name) + [fip_pool_name], parent=network)

        return self._create_or_get(fip_pool)

    @measure_latency
    def add_cidrs(self, network_id: str, cidrs: List[str]):
        """Add CIDRs to Contrail network."""

        log.debug("Adding CIDRs %r to network %r ...", cidrs, network_id)

        network = self.resource("virtual-network", uuid=network_id, fetch=True)
        ipam_subnets = network["network_ipam_refs"][0]["attr"]["ipam_subnets"]

        existing_pairs = {(s["subnet"]["ip_prefix"], s["subnet"]["ip_prefix_len"]) for s in ipam_subnets}

        modified = False
        for cidr in cidrs:
            ipam_subnet, prefix, prefix_len = _make_ipam_subnet(cidr)
            if (prefix, prefix_len) not in existing_pairs:
                ipam_subnets.append(ipam_subnet.to_primitive())
                modified = True

        if modified:
            network.save()
        else:
            log.debug("Nothing to add to network %r: all CIDRs already exist.", network_id)

    @measure_latency
    def delete_cidrs(self, network_id: str, cidrs: List[str]):
        """Delete CIDRs from Contrail network."""

        log.debug("Deleting CIDRs %r from network %r ...", cidrs, network_id)

        try:
            network = self.resource("virtual-network", uuid=network_id, fetch=True)
        except exceptions.ResourceNotFoundError:
            log.warning("No CIDRs to delete: network %r does not exist", network_id)
            return

        ipam_refs = network.get("network_ipam_refs", [])
        if not ipam_refs:
            log.warning("Network %r doesn't have IPAM, skipping CIDR deletion", network_id)
            return

        ipam_subnets = ipam_refs[0]["attr"]["ipam_subnets"]

        pairs_to_delete = set()
        for cidr in cidrs:
            _, prefix, prefix_len = _make_ipam_subnet(cidr)
            pairs_to_delete.add((prefix, prefix_len))

        new_ipam_subnets = [
            s for s in ipam_subnets if (s["subnet"]["ip_prefix"], s["subnet"]["ip_prefix_len"]) not in pairs_to_delete
        ]

        if len(new_ipam_subnets) < len(ipam_subnets):
            ipam_refs[0]["attr"]["ipam_subnets"] = new_ipam_subnets
            network.save()
        else:
            log.debug("Nothing to delete from network %r: all CIDRs already deleted", network_id)

    @measure_latency
    def set_cidrs(self, network_id: str, cidrs: List[str]):
        """Set CIDRs to Contrail network."""

        log.debug("Setting CIDRs %r to network %r ...", cidrs, network_id)

        # MAY raise ResourceNotFoundError, must be caught by caller
        network = self.resource("virtual-network", uuid=network_id, fetch=True)

        ipam_refs = network.get("network_ipam_refs", [])
        if not ipam_refs:
            log.warning("Network %r doesn't have IPAM, skipping CIDR synchronization...", network_id)
            raise exceptions.ResourceNotFoundError(resource=network)

        current_subnets = network["network_ipam_refs"][0]["attr"]["ipam_subnets"]
        new_subnets = []

        existing_pairs = {(s["subnet"]["ip_prefix"], s["subnet"]["ip_prefix_len"]) for s in current_subnets}
        new_pairs = set()

        for cidr in cidrs:
            new_subnet, prefix, prefix_len = _make_ipam_subnet(cidr)
            new_pairs.add((prefix, prefix_len))
            new_subnets.append(new_subnet.to_primitive())

        if existing_pairs != new_pairs:
            ipam_refs[0]["attr"]["ipam_subnets"] = new_subnets
            network.save()
        else:
            log.debug("Nothing to sync in network %r: all CIDRs are in place.", network_id)

    def _create_raw_vdns(self, vdns_name, vdns_data):
        domain = self._get_domain()

        try:
            vdns = self.resource(
                "virtual-DNS", fq_name=_make_fqname(_DEFAULT_DOMAIN, vdns_name),
                virtual_DNS_data=vdns_data, parent=domain
            )
            vdns.save()
        except exceptions.HttpError as e:
            if e.http_status == HTTPStatus.CONFLICT:
                vdns = self.resource(
                    "virtual-DNS", fq_name=_make_fqname(_DEFAULT_DOMAIN, vdns_name),
                    fetch=True
                )
            else:
                raise e

        return vdns

    def _create_vdns(self, compute_network_id, network, next_vdns):
        """Create virtual DNS for network in contrail."""

        vdns_data = self._build_vdns_data(reverse_resolution=True, next_vdns=next_vdns)
        vdns_name = naming.vdns(compute_network_id)
        vdns = self._create_raw_vdns(vdns_name, vdns_data)

        net_ipam = self._get_referred_ipam(network)
        try:
            net_ipam.add_ref(vdns)
        except exceptions.HttpError:
            # TODO: Add wrap exceptions to raise
            raise

        return vdns

    def _create_proxy_vdns(self, compute_network_id, domain, next_vdns):
        vdns_data = self._build_vdns_data(reverse_resolution=False, domain=domain, next_vdns=next_vdns)
        vdns_name = naming.proxy_vdns(compute_network_id)
        return self._create_raw_vdns(vdns_name, vdns_data)

    def _set_network_route_targets(self, network, import_rts, export_rts):
        """Set route targets for virtual network in contrail."""

        import_rts = [naming.route_target(str(target)) for target in import_rts]
        export_rts = [naming.route_target(str(target)) for target in export_rts]

        for route_target in set(import_rts + export_rts):
            try:
                contrail_rt = self.resource("route-target", fq_name=[route_target])
                contrail_rt.save()
            except exceptions.HttpError as e:
                if e.http_status == HTTPStatus.CONFLICT:
                    log.warning("Route target creation failed with conflict: %s.", e)
                else:
                    raise e

        network.data["import_route_target_list"] = {"route_target": import_rts}
        network.data["export_route_target_list"] = {"route_target": export_rts}
        network.save()

    @measure_latency
    def update_subnet_route_targets(self, network_uuid, export_rts=None, import_rts=None):
        """Update subnet's route targets on user network in contrail."""

        log.debug("Updating subnet's route targets: network_uuid=%r ...", network_uuid)

        try:
            network = self.resource("virtual-network", uuid=network_uuid, fetch=True)
        except exceptions.ResourceNotFoundError:
            log.warning("Tried to update nonexistent subnet %r.", network_uuid)
            return

        if export_rts is not None or import_rts is not None:
            self._set_network_route_targets(network, import_rts or [], export_rts or [])

    @measure_latency
    def delete_subnet(self, network_uuid, compute_network_id, recursive=False):
        """Delete subnet from user network in contrail."""

        log.debug("Deleting subnet: network_uuid=%r, compute_network_id=%r, recursive=%r ...",
                  network_uuid, compute_network_id, recursive)

        try:
            network = self.resource("virtual-network", uuid=network_uuid, fetch=True)
        except exceptions.ResourceNotFoundError:
            log.warning("Tried to delete nonexistent subnet %r.", network_uuid)
            return

        if recursive:
            for ip in network.back_refs.instance_ip:
                self._delete(ip)
            vms_to_check = dict()
            for port in network.back_refs.virtual_machine_interface:
                port.fetch()
                for vm in port.refs.virtual_machine:
                    vms_to_check[vm.uuid] = vm
                for fip in port.back_refs.floating_ip:
                    self._delete(fip)
                self._delete(port)
            # NOTE (simonov-d): possible VM leak if interrupted here (no way to find VM, because ports are deleted)
            for vm in vms_to_check.values():
                vm.fetch()
                if len(vm.back_refs.virtual_machine_interface) == 0:
                    self._delete(vm)
            network.fetch()  # Refresh refs/back_refs

        if network.back_refs.virtual_machine_interface:
            log.error("Cannot delete virtual-network %r (%r), because it has ports: %r.",
                      network.display_name, network.uuid, [i.uuid for i in network.back_refs.virtual_machine_interface])
            # FIXME: Use correct error
            raise Error("Subnet not empty")

        # Delete possible associated route_tables
        for route_table in network.refs.route_table:
            network.remove_ref(route_table)

        self.set_network_routing_policies(network_uuid, [], False)
        self._delete_network_default_routing_policies(network_uuid)

        # Use fq_name ("weak" reference) to avoid leaks when Contrail's
        # "ref" is already removed, but referred object still exists.
        ipam = self._try_get("network-ipam", _make_fqname(_DEFAULT_PROJECT, naming.ipam(compute_network_id)))
        vdns = self._try_get("virtual-DNS", _make_fqname(_DEFAULT_DOMAIN, naming.vdns(compute_network_id)))
        proxy_vdns = self._try_get("virtual-DNS", _make_fqname(_DEFAULT_DOMAIN, naming.proxy_vdns(compute_network_id)))

        if ipam:
            network.remove_ref(ipam)
            self._delete(ipam)

        if vdns:
            for record in vdns.get("virtual_DNS_records", []):
                # Speedup mass-deletion of records by not re-fetching records
                self._delete(record, fetch=False)
            self._delete(vdns)
        # vdns must be removed before proxy_vdns, because vdns
        # references proxy_vdns by fq_name via 'next_virtual_DNS'
        if proxy_vdns:
            for record in proxy_vdns.get("virtual_DNS_records", []):
                # Speedup mass-deletion of records by not re-fetching records
                self._delete(record, fetch=False)
            self._delete(proxy_vdns)

        # Remove all vnet access-control-lists
        for acl in network.get("access_control_lists", []):
            self._delete(acl)

        for ri in network.get("routing_instances", []):
            self._delete(ri)

        self._delete(network)

    @measure_latency
    def delete_fip_network(self, network_uuid, fip_bucket_name, recursive=False):
        log.debug("Deleting FIP network: network_uuid=%r, fip_bucket_name=%r, recursive=%r ...",
                  network_uuid, fip_bucket_name, recursive)
        try:
            network = self.resource("virtual-network", uuid=network_uuid, fetch=True)
        except exceptions.ResourceNotFoundError:
            log.warning("Tried to delete nonexistent FIP network %r.", network_uuid)
            return

        # Use fq_name ("weak" reference) to avoid leaks when Contrail's
        # "ref" is already removed, but referred object still exists.
        ipam_name = naming.fip_ipam(fip_bucket_name)
        ipam = self._try_get("network-ipam", _make_fqname(_DEFAULT_PROJECT, ipam_name))

        fip_pool_name = naming.fip_pool(fip_bucket_name)
        fip_pool = self._try_get("floating-ip-pool", _make_fqname(network.fq_name, fip_pool_name))

        if recursive and fip_pool:
            for fip in fip_pool.children.floating_ip:
                fip.delete()

        if ipam:
            network.remove_ref(ipam)
            ipam.delete()

        if fip_pool:
            fip_pool.delete()

        network.delete()

    def _to_ref(self, resource):
        return {
            "to": resource.fq_name,
            "uuid": resource.uuid,
        }

    def _create_or_get(self, obj: T) -> T:
        try:
            res = obj.create()
            log.debug("Created %s with fq_name %r, uuid %r.", obj.type, obj.fq_name, obj.uuid)
            return res
        except exceptions.HttpError as e:
            if e.http_status == HTTPStatus.CONFLICT:
                res = self.resource(obj.type, fq_name=obj.fq_name, fetch=True,)
                log.debug("Tried to create %s, but it already exists (fq_name %r uuid %r).",
                          res.type, res.fq_name, res.uuid)
                return res
            raise e

    def _try_get(self, resource_type: str, fq_name: Union[str, List[str]]):
        try:
            return self.resource(resource_type, fq_name=fq_name, fetch=True)
        except exceptions.ResourceNotFoundError:
            log.warning("%s with fq_name %r not found.", resource_type, fq_name)
            return None

    def _resource_exists(self, resource_type: str, resource_uuid: str):
        try:
            self.resource(resource_type, uuid=resource_uuid, check=True)
            return True
        except exceptions.ResourceNotFoundError:
            return False

    def network_exists(self, network_uuid: str):
        return self._resource_exists("virtual-network", network_uuid)

    def _delete(self, obj, fetch=False):
        try:
            log.debug("Deleting %s (fq_name %r, uuid %r)...", obj.type, obj.fq_name, obj.uuid)
            if fetch:
                obj.fetch()
            obj.delete()
        except exceptions.ResourceNotFoundError:
            log.warning("Tried to delete nonexistent %s (fq_name %r, uuid %r).", obj.type, obj.fq_name, obj.uuid)

    @measure_latency
    def set_dns_records(self, network_uuid: str, records: List[DnsRecord], instance_id: Optional[str]=None):
        log.debug("Setting DNS records %r in network %r for instance %r...", records, network_uuid, instance_id)

        manager = VirtualDNSManager(self, network_uuid)
        for record in records:
            manager.save_dns_record(
                resource_name=naming.dns_record(record.type, record.name, record.value, instance_id),
                record_type=record.type,
                record_name=record.name,
                record_value=record.value,
                record_ttl=record.ttl
            )

    @measure_latency
    def delete_dns_records(self, network_uuid: str, records: List[DnsRecord], instance_id: Optional[str]=None):
        log.debug("Deleting DNS records %r from network %r for instance %r...", records, network_uuid, instance_id)

        manager = VirtualDNSManager(self, network_uuid)
        for record in records:
            manager.delete_dns_record(
                resource_name=naming.dns_record(record.type, record.name, record.value, instance_id),
                record_type=record.type,
                record_name=record.name
            )

    @measure_latency
    def sync_dns_records(self, network_uuid: str, all_records: Dict[Optional[str], List[DnsRecord]]):
        manager = VirtualDNSManager(self, network_uuid)

        records_map = {}
        for instance_id, records in all_records.items():
            for record in records:
                records_map[naming.dns_record(record.type, record.name, record.value, instance_id)] = record

        manager.sync_dns_records(records_map)

    # This method may fail during creation of large number of DNS records.
    # Caller method should gracefully handle this situation.
    def random_faulty_sync_dns_records(self, network_uuid: str, all_records: Dict[Optional[str], List[DnsRecord]]):
        manager = VirtualDNSManager(self, network_uuid)

        records_map = {}
        for instance_id, records in all_records.items():
            for record in records:
                records_map[naming.dns_record(record.type, record.name, record.value, instance_id)] = record

        manager.random_faulty_sync_dns_records(records_map)

    def ensure_virtual_machine(self, instance_uuid, compute_instance_id, fqdn):
        vm_name = naming.vm(compute_instance_id)
        vm = self.resource("virtual-machine", uuid=instance_uuid, fq_name=[vm_name], display_name=fqdn)
        vm = self._create_or_get(vm)
        if fqdn and fqdn != vm.display_name:
            raise Error("Requested FQDN ({!r}) for instance {!r} differs from instance's FQDN in Contrail DB ({!r}).",
                        fqdn, instance_uuid, vm.display_name)  # Instance's FQDN is used for DNS record deletion
        return vm

    @measure_latency
    def create_fip(self, port_uuid: str, compute_instance_id: str, fip_bucket_name: str,
                   floating_ip_address: str, fixed_ip_address: str):
        try:
            port = self.resource("virtual-machine-interface", uuid=port_uuid, fetch=True)
        except exceptions.ResourceNotFoundError:
            raise Error("Port {} not found.", port_uuid)

        return self._create_fip(port, compute_instance_id, fip_bucket_name, floating_ip_address, fixed_ip_address)

    def _create_fip(self, port: Resource, compute_instance_id: str, fip_bucket_name: str,
                    floating_ip_address: str, fixed_ip_address: str):
        project = self._get_project()

        fip_network_name = naming.fip_vnet(fip_bucket_name)
        fip_network = self.resource("virtual-network", fq_name=list(project.fq_name) + [fip_network_name])

        fip_pool_name = naming.fip_pool(fip_bucket_name)
        fip_pool = self.resource("floating-ip-pool", fq_name=list(fip_network.fq_name) + [fip_pool_name])

        name = naming.fip(compute_instance_id, port.uuid, floating_ip_address)
        fip = self.resource("floating-ip",
                            fq_name=list(fip_pool.fq_name) + [name],
                            parent=fip_pool,
                            project_refs=[self._to_ref(project)],
                            virtual_machine_interface_refs=[self._to_ref(port)],
                            floating_ip_address=floating_ip_address,
                            floating_ip_fixed_ip_address=fixed_ip_address)
        return self._create_or_get(fip)

    @measure_latency
    def delete_fip(self, port_uuid: str, compute_instance_id: str, fip_bucket_name: str,
                   floating_ip_address: str):
        project = self._get_project()

        fip_network_name = naming.fip_vnet(fip_bucket_name)
        fip_network = self.resource("virtual-network", fq_name=list(project.fq_name) + [fip_network_name])

        fip_pool_name = naming.fip_pool(fip_bucket_name)
        fip_pool = self.resource("floating-ip-pool", fq_name=list(fip_network.fq_name) + [fip_pool_name])

        name = naming.fip(compute_instance_id, port_uuid, floating_ip_address)
        fip = self.resource("floating-ip",
                            fq_name=list(fip_pool.fq_name) + [name])
        self._delete(fip, fetch=True)

    def _build_dhcp_options(self, instance_name: str, search_domains: List[str]=None):
        dhcp_option_list = DhcpOptionListType.new()

        if instance_name:
            hostname_option = DhcpOptionType.new(
                dhcp_option_name="host-name",
                dhcp_option_value=instance_name
            )
            dhcp_option_list.dhcp_option.append(hostname_option)

            domain_split = instance_name.split(".", 1)
            if len(domain_split) > 1:
                domain_option = DhcpOptionType.new(
                    dhcp_option_name="domain-name",
                    dhcp_option_value=domain_split[1]
                )
                dhcp_option_list.dhcp_option.append(domain_option)

        if search_domains:
            search_option = DhcpOptionType.new(
                dhcp_option_name="domain-search",
                dhcp_option_value=" ".join(search_domains)
            )
            v6_search_option = DhcpOptionType.new(
                dhcp_option_name="v6-domain-search",
                dhcp_option_value=" ".join(search_domains)
            )

            dhcp_option_list.dhcp_option.append(search_option)
            dhcp_option_list.dhcp_option.append(v6_search_option)

        if len(dhcp_option_list.dhcp_option) > 0:
            return dhcp_option_list.to_primitive()
        else:
            return None

    @measure_latency
    def create_port(self,
                    port_uuid: str,
                    network_uuid: str,
                    instance_uuid: str,
                    compute_instance_id: str,
                    instance_fqdn: str,
                    ip_addresses: List[str],
                    port_name: str = None,
                    mac_address: str = None,
                    search_domains: List[str] = None,
                    floating_ip_bindings: List[FloatingIpBinding] = None,
                    feature_flags: List[str] = None,
                    ) -> ContrailPort:
        """Create a virtual-machine-interface in contrail

        port_name is optional. If it is not passed, it would be generated
        as {compute_instance_id}-{port_uuid}
        mac_address is optional, would be auto-generated if not passed.
        """
        log.debug("Creating port %r for instance %r (%r) to network %r "
                  "(ip_addresses=%r, floating_ip_bindings=%r, feature_flags=%r) ...",
                  port_uuid, compute_instance_id, instance_fqdn, network_uuid,
                  ip_addresses, floating_ip_bindings, feature_flags)

        vm = self.ensure_virtual_machine(instance_uuid, compute_instance_id, instance_fqdn)

        try:
            network = self.resource("virtual-network", uuid=network_uuid, fetch=True)
        except exceptions.ResourceNotFoundError:
            raise Error("Network {} not found", network_uuid)

        # Overriding port_name is useful for Contrail port creation test (active probe)
        if not port_name:
            port_name = naming.port(compute_instance_id, port_uuid)

        allocation_plan = {4: [], 6: []}  # Map from ip_version to list of subnet_uuid
        all_network_cidrs = []  # To make clear error message

        for ipam in network.refs.network_ipam:
            ipam_subnets = ipam["attr"]["ipam_subnets"]
            for ipam_subnet in ipam_subnets:
                ip_network = _subnet_to_ip_network(ipam_subnet["subnet"])
                all_network_cidrs.append(str(ip_network))  # To make clear error message
                for ip_address in ip_addresses:
                    ip_address = ipaddress.ip_address(ip_address)
                    if ip_address in ip_network:
                        allocation_plan[ip_network.version].append((str(ip_address), ipam_subnet["subnet_uuid"]))

        if len(allocation_plan[4]) + len(allocation_plan[6]) < len(ip_addresses):
            raise Error("Some of requested addresses ({!r}) were not found in network {!r} CIDRs ({!r})",
                        ip_addresses, network_uuid, all_network_cidrs)

        # NOTE(k-zaitsev): we can't just pass None for mac address, for then cloudwatt client
        # would attempt to treat literally treat it as mac address. Threrefore using kwargs here
        vm_interface_kwargs = {}
        if mac_address:
            vm_interface_kwargs["virtual_machine_interface_mac_addresses"] = {"mac_address": [mac_address]}

        vmi_properties = VirtualMachineInterfacePropertiesType.new(
            feature_flag_list=VirtualMachineInterfaceFeatureFlags.new(flag=feature_flags),
        )

        project = self._get_project()
        port = self.resource(
            "virtual-machine-interface",
            uuid=port_uuid,
            fq_name=list(project.fq_name) + [port_name],
            parent=project,
            virtual_network_refs=[self._to_ref(network)],
            virtual_machine_interface_disable_policy=False,
            virtual_machine_interface_dhcp_option_list=self._build_dhcp_options(instance_fqdn, search_domains),
            port_security_enabled=False,
            virtual_machine_interface_device_owner="compute",  # TODO(k-zaitsev): parametrize?,
            virtual_machine_interface_properties=vmi_properties.to_primitive(),
            **vm_interface_kwargs)
        port = self._create_or_get(port)

        log.debug("IP address allocation plan for port %r: %r.", port.uuid, allocation_plan)

        for ip_version, address_subnets in allocation_plan.items():
            for ip_address, subnet_uuid in address_subnets:
                ip_name = naming.ip(compute_instance_id, port_uuid, ip_address)
                ip = self.resource(
                    "instance-ip",
                    fq_name=[ip_name],
                    subnet_uuid=subnet_uuid,
                    instance_ip_address=ip_address,
                    instance_ip_family="v{}".format(ip_version),
                    virtual_machine_interface_refs=[self._to_ref(port)],
                    virtual_network_refs=[self._to_ref(network)],
                )
                self._create_or_get(ip)

        for binding in floating_ip_bindings or []:
            for descriptor in binding.floating_ip_descriptors:
                self._create_fip(port, compute_instance_id, descriptor.fip_bucket_name, descriptor.floating_ip_address,
                                 binding.fixed_ip_address)

        port = port.add_ref(vm)

        contrail_port = ContrailPort.new(
            id=port.uuid,
            name=port.display_name,
            network_id=network.display_name,
            contrail_network_id=network.uuid,
            vm_id=instance_uuid,
            vm_name=instance_fqdn,
            project_id=project.uuid,
            mac_address=port.virtual_machine_interface_mac_addresses["mac_address"][0]
        )
        return contrail_port

    def _set_port_properties(self, port_uuid: str, **properties) -> Resource:
        port = self.resource("virtual-machine-interface", uuid=port_uuid, fetch=True)

        if port.virtual_machine_interface_properties is not None:
            port.virtual_machine_interface_properties.update(properties)
        else:
            vmi_properties = VirtualMachineInterfacePropertiesType.new(**properties)
            port["virtual_machine_interface_properties"] = vmi_properties.to_primitive()
        return port

    @measure_latency
    def set_port_feature_flags(self, port_uuid: str, feature_flags: List[str]) -> Resource:
        """Sets vm interface feature-flag list"""
        log.debug("Update feature_flags vm interface port: %r, feature_flags: %r",
                  port_uuid, feature_flags)
        feature_flag_list = VirtualMachineInterfaceFeatureFlags.new(flag=feature_flags).to_primitive()
        port = self._set_port_properties(port_uuid, feature_flag_list=feature_flag_list)
        return port.save()

    @measure_latency
    def get_port_feature_flags(self, port_uuid: str) -> Optional[List[str]]:
        """Returns vm interface feature-flag list"""
        log.debug("Get feature_flags for vm interface port: %r", port_uuid)
        port = self.resource("virtual-machine-interface", uuid=port_uuid, fetch=True)
        if port.virtual_machine_interface_properties is not None and \
                port.virtual_machine_interface_properties.get('feature_flag_list') is not None:
            return port.virtual_machine_interface_properties['feature_flag_list']['flag']

    @measure_latency
    def delete_port(self, port_uuid: str, network_uuid: str, compute_instance_id: str):
        """Delete contrail port

        Removes associated vm if it was the last port on that VM,
        Clears dns records for said port.
        Removes any ip and fip objects.
        Finally removes the port object itself.
        Ignores any ResourceNotFoundErrors raised in the process.
        """

        log.debug("Deleting port: network_id=%r, port_id=%r ...", network_uuid, port_uuid)

        try:
            port = self.resource("virtual-machine-interface", uuid=port_uuid, fetch=True)
        except exceptions.ResourceNotFoundError:
            log.warning("Tried to delete nonexistent port %r.", port_uuid)
            return

        network = self.resource("virtual-network", uuid=network_uuid, fetch=True)

        # Use fq_name ("weak" reference) to avoid leaks when Contrail's
        # "ref" is already removed, but referred object still exists.
        vm = self._try_get("virtual-machine", naming.vm(compute_instance_id))
        if vm:
            port.remove_ref(vm)  # If reference doesn't exist, it doesn't fail
            vm.fetch()  # Refresh vm.back_refs

            if len(vm.back_refs.virtual_machine_interface) > 0:
                log.warning("Not deleting contrail-vm object fq_name %r, uuid %r. "
                            "It still has port back references: %r.",
                            vm.fq_name, vm.uuid, [i.uuid for i in vm.back_refs.virtual_machine_interface])
            else:
                self._delete(vm)

        for instance_ip in port.back_refs.instance_ip:
            self._delete(instance_ip)
        for floating_ip in port.back_refs.floating_ip:
            self._delete(floating_ip)
        self._delete(port)

    def _set_network_properties(self, network_uuid: str, **properties) -> Resource:
        network = self.resource("virtual-network", uuid=network_uuid, fetch=True)

        if network.virtual_network_properties is not None:
            network.virtual_network_properties.update(properties)
        else:
            net_properties = VirtualNetworkType.new(**properties)
            network["virtual_network_properties"] = net_properties.to_primitive()
        return network

    @measure_latency
    def update_network_rpf(self, network_uuid: str, rpf: str):
        log.debug("Update rpf: network=%r, rpf=%r", network_uuid, rpf)

        network = self._set_network_properties(network_uuid, rpf=rpf)
        network.save()

    def enable_network_rpf(self, network_uuid: str):
        self.update_network_rpf(network_uuid, "enable")

    def disable_network_rpf(self, network_uuid: str):
        self.update_network_rpf(network_uuid, "disable")

    @measure_latency
    def set_network_feature_flags(self, network_uuid: str, feature_flags: List[str]) -> Resource:
        """Sets virtual network feature-flag list"""
        log.debug("Update feature_flags for network: %r, feature_flags: %r",
                  network_uuid, feature_flags)
        feature_flag_list = VirtualNetworkFeatureFlags.new(flag=feature_flags).to_primitive()
        network = self._set_network_properties(network_uuid, feature_flag_list=feature_flag_list)
        return network.save()

    @measure_latency
    def get_network_feature_flags(self, network_uuid: str) -> Optional[List[str]]:
        """Returns virtual network feature-flag list"""
        log.debug("Get feature_flags for network: %r", network_uuid)
        network = self.resource("virtual-network", uuid=network_uuid, fetch=True)
        if network.virtual_network_properties is not None and \
                network.virtual_network_properties.get('feature_flag_list') is not None:
            return network.virtual_network_properties['feature_flag_list']['flag']

    # route target

    def create_route_target(self, route_target):
        log.debug("Create route_target=%r", route_target)

        name = naming.route_target(route_target)
        contrail_rt = self.resource("route-target", fq_name=[name])
        return self._create_or_get(contrail_rt)

    def get_route_target(self, rt, fetch=True):
        rt_name = naming.route_target(rt)
        return self.resource("route-target", fq_name=[rt_name], fetch=fetch)

    def delete_route_target(self, route_target, safe=False):
        """
        :param safe: if True then route target will be removed only if there is no refs to it
        """
        log.debug("Delete route_target=%r", route_target)

        try:
            rt = self.get_route_target(route_target)
        except exceptions.ResourceNotFoundError as e:
            log.warning("Failed to delete route-target: route target %r does not exist", route_target)
            return

        if safe and list(rt.back_refs):
            return None

        return self._delete(rt)

    # routing instance

    @measure_latency
    def get_routing_instance(self, uuid: str):
        return self.resource("routing-instance", uuid=uuid, fetch=True)

    @measure_latency
    def get_network_routing_instances(self, network_uuid: str):
        network = self.resource("virtual-network", uuid=network_uuid, fetch=True)
        return network.get("routing_instances")

    @measure_latency
    def get_network_default_routing_instance(self, network_uuid: str) -> Resource:
        network = self.resource("virtual-network", uuid=network_uuid, fetch=True)
        return self.resource(
            "routing-instance",
            fq_name=list(network.fq_name) + [network.fq_name[-1]],
            fetch=True,
        )

    @measure_latency
    def find_routing_instance(self, name: str, network_uuid: str):
        ris = self.get_network_routing_instances(network_uuid)
        for ri in ris:
            if naming.routing_instance(name) == ri.fq_name[-1]:
                return self.get_routing_instance(ri.uuid)
        return None

    @measure_latency
    def create_routing_instance(self, name: str, network_uuid: str,
                                route_targets=None,
                                rt_type=InstanceTargetType.ImportExport.IMPORT,
                                wait_timeout=_WAIT_ROUTING_INSTANCE_TIMEOUT):
        """
        :param name: The name of routing instance
        :param network_uuid: Network uuid
        :param route_targets: List of route target names
        """

        log.debug("Create routing_instance: name=%r, network_uuid=%r, route_targets=%r", name, network_uuid, route_targets)

        network = self.resource("virtual-network", uuid=network_uuid, fetch=True)

        name = naming.routing_instance(name)

        ri = self.resource(
            "routing-instance",
            fq_name=list(network.fq_name) + [name],
            display_name=name,
            parent_type="virtual-network",
            route_target_refs=[],
            parent=network,
            routing_instance_is_default=False,
            routing_instance_has_pnf=False,
        )

        ri = self._create_or_get(ri)

        # CLOUD-35541: There is a race beetwen schema transformer and manual ref updates.
        # Before any manual ref updates, client should wait for schema-transformer synchronization.
        if wait_timeout:
            self.__wait_routing_instance_is_ready(ri, wait_timeout)

        if route_targets:
            for rt in route_targets:
                rt = self.get_route_target(rt)
                self.add_route_target_to_routing_instance(ri, rt, rt_type)

        return ri

    def __wait_routing_instance_is_ready(self, ri, timeout):
        log.debug("Waiting for routing-instance is ready fq_name=%r", ri.fq_name)

        for _ in timeout_iter(Error("Waiting for routing instance {!r} to become ready has timed out after {} seconds", ri.fq_name, timeout), timeout=timeout):
            ri.fetch()

            for rt in ri.refs.route_target:
                rt_name = rt.fq_name[-1]
                rt_id = int(rt_name.split(":")[-1])
                if rt_id >= _BGP_RTGT_MIN_ID:
                    return True

    def add_route_target_to_routing_instance(self, ri, rt, rt_type=InstanceTargetType.ImportExport.IMPORT):
        attr = InstanceTargetType.new(import_export=rt_type).to_primitive()
        ri.add_ref(rt, attr)

    @measure_latency
    def delete_routing_instance(self, name: str, network_uuid: str):
        log.debug("Delete routing_instance: name=%r, network_uuid=%r", name, network_uuid)

        try:
            network = self.resource("virtual-network", uuid=network_uuid, fetch=True)
        except exceptions.ResourceNotFoundError as e:
            log.warning("Failed to delete routing-instance (%s): virtual-network %r does not exist",
                        name, network_uuid)
            return

        name = naming.routing_instance(name)
        ri = self.resource("routing-instance", fq_name=list(network.fq_name) + [name], parent=network)
        self._delete(ri, fetch=True)

    @measure_latency
    def attach_route_instance_to_port(self, port_uuid: str, network_uuid: str, ri_name: str):
        log.debug(
            "Attach routing instance to port: port_uuid=%r, network_uuid=%r, ri_name=%r",
            port_uuid, network_uuid, ri_name,
        )

        port = self.resource("virtual-machine-interface", uuid=port_uuid, fetch=True)
        network = self.resource("virtual-network", uuid=network_uuid, fetch=True)

        ri_name = naming.routing_instance(ri_name)
        ri = self.resource("routing-instance", fq_name=list(network.fq_name) + [ri_name], fetch=True)

        attr = PolicyBasedForwardingRuleType.new(direction=None)
        port.add_ref(ri, attr.to_primitive())

        return port

    @measure_latency
    def detach_routing_instance_from_port(self, port_uuid: str, network_uuid: str, ri_name: str):
        log.debug(
            "Detach routing instance from port: port_uuid=%r, network_uuid=%r, ri_name=%r",
            port_uuid, network_uuid, ri_name,
        )

        port = self.resource("virtual-machine-interface", uuid=port_uuid, fetch=True)
        network = self.resource("virtual-network", uuid=network_uuid, fetch=True)

        ri_name = naming.routing_instance(ri_name)
        ri = self.resource("routing-instance", fq_name=list(network.fq_name) + [ri_name], fetch=True)

        port.remove_ref(ri)

        return port

    # access-control-list

    @measure_latency
    def create_acl(self, acl_name: str, network_uuid: str, rules=None):
        """Create empty acl control list"""

        log.debug("Create acl: name=%r, network=%r", acl_name, network_uuid)

        network = self.resource("virtual-network", uuid=network_uuid, fetch=True)

        if rules is None:
            rules = []
        entries = AclEntriesType.new(acl_rule=rules)

        acl_name = naming.acl(acl_name, network.uuid)
        acl = self.resource("access-control-list",
                            fq_name=list(network.fq_name) + [acl_name],
                            name=acl_name,
                            parent=network,
                            access_control_list_entries=entries.to_primitive())
        return self._create_or_get(acl)

    @measure_latency
    def delete_acl(self, acl_name: str, network_uuid: str):
        log.debug("Delete acl: name=%r, network=%r", acl_name, network_uuid)

        try:
            network = self.resource("virtual-network", uuid=network_uuid, fetch=True)
        except exceptions.ResourceNotFoundError as e:
            log.warning("Failed to delete acl: virtual-network %r does not exist", network_uuid)
            return

        acl_name = naming.acl(acl_name, network.uuid)
        acl = self.resource("access-control-list",
                            fq_name=list(network.fq_name) + [acl_name],
                            parent=network)

        self._delete(acl, fetch=True)

    @measure_latency
    def add_subnets_acl_rule(self, name: str, src_subnet_uuid: str,
                             src_network: str=None, src_subnets: List[str]=None, src_port: List[int]=None,
                             dst_network: str=None, dst_subnets: List[str]=None, dst_port: List[int]=None,
                             protocol: int=None, assign_ri: str=None, action: str=None, rule_uuid: str=None):
        log.debug(
            "Add acl rule: src_subnet_uuid=%r, src_port=%r, "
            "dst_port=%r, protocol=%r, assign_ri=%r, action=%r",
            src_subnet_uuid, src_port, dst_port, protocol, assign_ri, action,
        )

        rule = self.make_acl_rule(
            src_network=src_network, src_subnets=src_subnets, src_port=src_port,
            dst_network=dst_network, dst_subnets=dst_subnets, dst_port=dst_port,
            protocol=protocol, assign_ri=assign_ri, action=action, rule_uuid=rule_uuid,
        ).to_primitive()

        src_vnet = self.resource("virtual-network", uuid=src_subnet_uuid, fetch=True)
        acl_name = naming.acl(name, src_vnet.uuid)
        acl = self.resource("access-control-list", fq_name=list(src_vnet.fq_name) + [acl_name], fetch=True)
        acl.access_control_list_entries["acl_rule"].append(rule)
        acl.save()

        return acl

    @measure_latency
    def replace_subnet_acl_rules(self, name: str, src_subnet_uuid: str, rules: List[AclRuleType]):
        src_vnet = self.resource("virtual-network", uuid=src_subnet_uuid, fetch=True)
        acl_name = naming.acl(name, src_vnet.uuid)
        acl = self.resource("access-control-list", fq_name=list(src_vnet.fq_name) + [acl_name], fetch=True)
        acl.access_control_list_entries["acl_rule"] = rules
        acl.save()

        return acl

    @measure_latency
    def list_acl_rules(self, name, src_subnet_uuid):
        acl = self._get_acl(name, src_subnet_uuid)
        return acl.access_control_list_entries["acl_rule"]

    @measure_latency
    def add_acl_rule(self, name: str, rule_uuid: str,
                     src_subnet_uuid: str, src_port: List[int],
                     dst_subnet_uuid: str, dst_port: List[int],
                     protocol: int, assign_ri: str=None, action: str=None):
        src_vnet = self.resource("virtual-network", uuid=src_subnet_uuid, fetch=True)
        dst_vnet = self.resource("virtual-network", uuid=dst_subnet_uuid, fetch=True)

        src_network = self._make_network(src_vnet)
        dst_network = self._make_network(dst_vnet)

        return self.add_subnets_acl_rule(name, src_subnet_uuid,
                                         src_subnets=src_network.subnets, src_port=src_port,
                                         dst_subnets=dst_network.subnets, dst_port=dst_port,
                                         protocol=protocol, assign_ri=assign_ri, action=action, rule_uuid=rule_uuid)

    @measure_latency
    def remove_acl_rule(self, name, src_subnet_uuid, rule_uuid):
        acl = self._get_acl(name, src_subnet_uuid)
        rule = self.find_acl_rule(name, src_subnet_uuid, rule_uuid)

        if rule is not None:
            acl.access_control_list_entries["acl_rule"] = list(filter(lambda r: r["rule_uuid"] != rule_uuid,
                                                               acl.access_control_list_entries["acl_rule"]))
            acl.save()
        else:
            log.warn("Trying to remove absent acl rule: acl_name=%r, network_uuid=%r, rule_uuid=%r",
                     name, src_subnet_uuid, rule_uuid)

        return acl

    @measure_latency
    def find_acl_rule(self, name, subnet_uuid, rule_uuid):
        all_rules = self.list_acl_rules(name, subnet_uuid)
        rules = list(filter(lambda r: r["rule_uuid"] == rule_uuid, all_rules))
        if rules:
            return rules.pop()
        return None

    @measure_latency
    def sort_acl_rules(self, name, src_subnet_uuid, key_fn):
        src_vnet = self.resource("virtual-network", uuid=src_subnet_uuid, fetch=True)
        acl_name = naming.acl(name, src_vnet.uuid)
        acl = self.resource("access-control-list", fq_name=list(src_vnet.fq_name) + [acl_name], fetch=True)
        rules = acl.access_control_list_entries["acl_rule"]
        rules.sort(key=key_fn)
        acl.save()

    def _get_acl(self, name, subnet_uuid):
        src_vnet = self.resource("virtual-network", uuid=subnet_uuid, fetch=True)
        acl_name = naming.acl(name, src_vnet.uuid)
        return self.resource("access-control-list", fq_name=list(src_vnet.fq_name) + [acl_name],
                             parent=src_vnet, fetch=True)

    def make_acl_rule(self, src_network: str=None, src_subnets: List[str]=None, src_port: List[int]=None,
                       dst_network: str = None, dst_subnets: List[str]=None, dst_port: List[int]=None,
                       protocol: int=None, assign_ri: str=None, action: str=None, rule_uuid: str=None) -> AclRuleType:
        return AclRuleType.new(
            match_condition=MatchConditionType.new(
                src_address=self._make_address(subnets=src_subnets, network=src_network).to_primitive(),
                src_port=self._make_port(src_port).to_primitive(),
                dst_address=self._make_address(subnets=dst_subnets, network=dst_network).to_primitive(),
                dst_port=self._make_port(dst_port).to_primitive(),
                protocol=str(protocol) if protocol else None,
            ),
            action_list=ActionListType.new(
                simple_action=action,
                assign_routing_instance=assign_ri,
            ).to_primitive(),
            rule_uuid=rule_uuid,
        )

    def _make_address(self, subnets: List[str]=None, network: str=None, security_group=None, network_policy=None):
        subnet_list = []
        if subnets is not None:
            for subnet in subnets:
                ip_network = ipaddress.ip_network(subnet)

                prefix = str(ip_network.network_address)
                prefix_len = ip_network.prefixlen

                subnet_list.append(
                    SubnetType.new(
                        ip_prefix=prefix,
                        ip_prefix_len=prefix_len,
                    ),
                )

        return AddressType.new(
            virtual_network=network,
            subnet_list=subnet_list,
            security_group=security_group,
            network_policy=network_policy,
        )

    def _make_port(self, ports=None):
        start_port, end_port = -1, -1
        if ports:
            start_port, end_port = ports
        return PortType.new(start_port=int(start_port), end_port=int(end_port))

    # route tables
    @staticmethod
    def _make_route(route, rt_community, static_route_community=_STATIC_ROUTE_BGP_COMMUNITY):
        return RouteType.new(
            prefix=str(route.destination_prefix),
            next_hop=str(route.next_hop_address),
            community_attributes=CommunityAttributes.new(
                community_attribute=[rt_community, static_route_community]
            )
        )

    def _get_route_table(self, route_table_id):
        project = self._get_project()
        name = naming.route_table(route_table_id)
        return self.resource("route-table", fq_name=list(project.fq_name) + [name], fetch=True)

    @measure_latency
    def create_route_table(self, route_table_id, rt_community, routes):
        """Creates route table object in Contrail."""

        log.debug("Creating route table %r...", route_table_id)

        project = self._get_project()
        name = naming.route_table(route_table_id)

        oct_routes = []
        for route in routes:
            oct_route = self._make_route(route, rt_community)
            oct_routes.append(oct_route.to_primitive())

        raw_routes = RouteTableType.new(route=oct_routes).to_primitive()
        route_table = self.resource(
            "route-table",
            fq_name=list(project.fq_name) + [name],
            parent=project,
            routes=raw_routes,
        )

        # Create routing policy that accepts all routes in route table
        self._create_route_table_routing_policy(route_table_id, rt_community)

        route_table = self._create_or_get(route_table)
        if route_table.routes != raw_routes:
            # _create_or_get may fetch existing object with outdated routes
            route_table['routes'] = raw_routes
            route_table.save()

        return route_table

    @measure_latency
    def add_routes(self, route_table_id, routes, rt_community):
        """Add routes to Contrail route table."""

        route_table = self._get_route_table(route_table_id)

        log.debug("Adding routes %r to route table %r ...", routes, route_table_id)

        oct_routes = [(route["prefix"], route["next_hop"]) for route in route_table["routes"]["route"]]
        current_routes = route_table["routes"]["route"]

        modified = False
        for route in routes:
            oct_route = self._make_route(route, rt_community)
            if (oct_route.prefix, oct_route.next_hop) not in oct_routes:
                current_routes.append(oct_route.to_primitive())
                modified = True

        if modified:
            route_table.save()
        else:
            log.debug("Nothing to add to route table %r: all routes already exist.", route_table_id)

    @measure_latency
    def delete_routes(self, route_table_id, routes):
        """Delete routes from Contrail route table."""

        log.debug("Deleting routes %r from route table %r ...", routes, route_table_id)

        try:
            route_table = self._get_route_table(route_table_id)
        except exceptions.ResourceNotFoundError:
            log.warning("No routes to delete: route table %r does not exist", route_table_id)
            return

        oct_routes_to_delete = set()
        for route in routes:
            oct_routes_to_delete.add((str(route["destination_prefix"]), str(route["next_hop_address"])))

        new_routes = [
            s for s in route_table["routes"]["route"] if (s["prefix"], s["next_hop"]) not in oct_routes_to_delete
        ]

        if len(new_routes) < len(route_table["routes"]["route"]):
            route_table["routes"]["route"] = new_routes
            route_table.save()
        else:
            log.debug("Nothing to delete from route_table %r: all routes already deleted", route_table_id)

    @measure_latency
    def replace_routes(self, route_table_id, routes, rt_community):
        """Add routes to Contrail route table."""

        log.debug("Setting routes %r to route table %r ...", routes, route_table_id)

        try:
            route_table = self._get_route_table(route_table_id)
        except exceptions.ResourceNotFoundError:
            log.warning("Route table %r does not exist, skip setting routes", route_table_id)
            return

        oct_routes = [self._make_route(route, rt_community) for route in routes]
        route_table['routes'] = RouteTableType.new(route=oct_routes).to_primitive()

        route_table.save()

    @measure_latency
    def delete_route_table(self, route_table_id):
        """Deletes route table from Contrail."""

        log.debug("Deleting route table %r...", route_table_id)

        try:
            route_table = self._get_route_table(route_table_id)
        except exceptions.ResourceNotFoundError:
            log.warning("Route table %r does not exist", route_table_id)
            return

        try:
            routing_policy = self._get_route_table_routing_policy(route_table_id)
        except exceptions.ResourceNotFoundError:
            # Idempotency
            routing_policy = None
            log.info("Route table's %r routing policy does not exists", route_table_id)

        if routing_policy:
            if routing_policy.refs.routing_instance:
                log.error(
                    "Cannot delete route-table %r (%r), because it's routing-policy %r has referred routing-instance: %r",
                    route_table.display_name, route_table.uuid, routing_policy.uuid,
                    [i.uuid for i in routing_policy.refs.routing_instance]
                )
                # FIXME: Use correct error
                raise Error("Route table not empty")
            self._delete(routing_policy)

        if route_table.back_refs.virtual_network:
            log.error(
                "Cannot delete route-table %r (%r), because it has referred virtual-network: %r",
                route_table.display_name, route_table.uuid,
                [i.uuid for i in route_table.back_refs.virtual_network]
            )
            # FIXME: Use correct error
            raise Error("Route table not empty")

        self._delete(route_table)

    def _create_community_based_routing_policy(self,
                                               community_attribute: str,
                                               policy_type_id: str,
                                               policy_action: str) -> Resource:
        term_match_condition = TermMatchConditionType.new(community=community_attribute)

        return self.__create_routing_policy(term_match_condition, policy_type_id, policy_action)

    def _create_prefixes_based_routing_policy(self,
                                              prefixes: List[str],
                                              prefix_type: str,
                                              policy_type_id: str,
                                              policy_action: str) -> Resource:
        policy_prefixes = []
        for prefix in prefixes:
            policy_prefixes.append(PrefixMatchType.new(prefix=str(prefix), prefix_type=prefix_type))

        term_match_condition = TermMatchConditionType.new(
            prefix=[policy_prefixes]
        )
        return self.__create_routing_policy(term_match_condition, policy_type_id, policy_action)

    def __create_routing_policy(self,
                                term_match_condition: TermMatchConditionType,
                                policy_type_id: str,
                                policy_action: str) -> Resource:
        log.debug("Creating routing policy with type %r and action %r", policy_type_id, policy_action)

        project = self._get_project()
        name = naming.routing_policy(policy_type_id, policy_action)

        term_action_list = TermActionListType.new(action=policy_action)

        policy_entry = PolicyTermType.new(
            term_match_condition=term_match_condition,
            term_action_list=term_action_list
        )

        routing_policy = self.resource(
            "routing-policy",
            fq_name=list(project.fq_name) + [name],
            parent=project,
            routing_policy_entries=PolicyStatementType.new(term=[policy_entry]).to_primitive()
        )
        return self._create_or_get(routing_policy)

    def _get_routing_policy(self, policy_type_id: str, policy_action: str) -> Resource:
        project = self._get_project()
        name = naming.routing_policy(policy_type_id, policy_action)
        return self.resource("routing-policy", fq_name=list(project.fq_name) + [name], fetch=True)

    def _get_accept_all_routing_policy(self, policy_key: str) -> Resource:
        return self._get_routing_policy(
            policy_type_id=naming.routing_policy_accept_all_type(policy_key),
            policy_action=TermActionListType.ActionType.ACCEPT,
        )

    def _get_or_create_accept_all_routing_policy(self, policy_key: str) -> Resource:
        try:
            policy = self._get_accept_all_routing_policy(policy_key)
        except exceptions.ResourceNotFoundError:
            policy = self._create_prefixes_based_routing_policy(
                prefixes=["0.0.0.0/0"],
                prefix_type=PrefixMatchType.PrefixType.LONGER,
                policy_type_id=naming.routing_policy_accept_all_type(policy_key),
                policy_action=TermActionListType.ActionType.ACCEPT
            )
        return policy

    def _get_reject_all_static_routes_routing_policy(self, policy_key: str) -> Resource:
        return self._get_routing_policy(
            policy_type_id=naming.routing_policy_reject_static_routes_type(policy_key),
            policy_action=TermActionListType.ActionType.REJECT
        )

    def _get_or_create_reject_all_static_routes_routing_policy(self, policy_key: str) -> Resource:
        try:
            policy = self._get_reject_all_static_routes_routing_policy(policy_key)
        except exceptions.ResourceNotFoundError:
            policy = self._create_community_based_routing_policy(
                community_attribute=_STATIC_ROUTE_BGP_COMMUNITY,
                policy_type_id=naming.routing_policy_reject_static_routes_type(policy_key),
                policy_action=TermActionListType.ActionType.REJECT
            )
        return policy

    def _get_or_create_network_default_routing_policies(self, policy_key: str) -> Tuple[Resource, ...]:
        """Creates or gets default routing policies that are needed for every network."""
        return (
            self._get_or_create_reject_all_static_routes_routing_policy(policy_key),
            self._get_or_create_accept_all_routing_policy(policy_key),
        )

    def _delete_network_default_routing_policies(self, policy_key: str):
        """Deletes default network routing policies."""
        try:
            policy = self._get_accept_all_routing_policy(policy_key)
            self._delete(policy)
        except exceptions.ResourceNotFoundError:
            pass
        try:
            policy = self._get_reject_all_static_routes_routing_policy(policy_key)
            self._delete(policy)
        except exceptions.ResourceNotFoundError:
            pass

    def _create_route_table_routing_policy(self, route_table_id, route_table_community):
        return self._create_community_based_routing_policy(
            community_attribute=route_table_community,
            policy_type_id=route_table_id,
            policy_action=TermActionListType.ActionType.ACCEPT
        )

    def _get_route_table_routing_policy(self, route_table_id):
        project = self._get_project()
        name = naming.routing_policy(route_table_id, TermActionListType.ActionType.ACCEPT)
        return self.resource("routing-policy", fq_name=list(project.fq_name) + [name], fetch=True)

    def set_network_routing_policies(self, network_uuid: str, routing_policies: Iterable[Resource], strict=True) -> Optional[Resource]:
        try:
            routing_instance = self.get_network_default_routing_instance(network_uuid)
        except exceptions.ResourceNotFoundError as e:
            log.error("default routing-instance for virtual-network %r not found.", network_uuid)

            if strict:
                raise e
            return

        # Clean-up old policies
        # Refer routing instance with routing policies in correct sequence
        for policy in routing_instance.back_refs.routing_policy:
            routing_instance.remove_back_ref(policy)

        # Add new policies
        for i, policy in enumerate(routing_policies):
            routing_instance.add_back_ref(policy, attr=SequenceType.new(sequence=str(i)).to_primitive())

        routing_instance.save()
        return routing_instance

    # FIXME: switch to one stateless association method instead of two statefull https://st.yandex-team.ru/CLOUD-28010
    @measure_latency
    def associate_route_table_with_subnet(self, route_table_id: str, network_uuid: str):
        """
            Refers route table with virtual network in contrail
            and creates routing policies to manage this reference.
        """
        log.debug("Associating route-table %r with virtual network %r ...", route_table_id, network_uuid)

        route_table = self._get_route_table(route_table_id)
        network = self.resource("virtual-network", uuid=network_uuid, fetch=True)

        # Keep previous associated route tables
        route_tables_routing_policies = []
        is_new_association = True
        for old_route_table in network.refs.route_table:
            old_route_table_id = naming.route_table_id_from_fq_name(old_route_table.fq_name)
            if old_route_table_id == route_table_id:
                log.info("Route table %r is already associated with %r", route_table_id, network_uuid)
                is_new_association = False
            route_tables_routing_policies.append(self._get_route_table_routing_policy(old_route_table_id))

        # Add new route table
        if is_new_association:
            route_tables_routing_policies.append(self._get_route_table_routing_policy(route_table_id))

        # Combine with default routing policies
        routing_policies = chain(
            route_tables_routing_policies,
            self._get_or_create_network_default_routing_policies(policy_key=network_uuid)
        )

        self.set_network_routing_policies(network_uuid, routing_policies)

        # Refer route table with subnet
        network.add_ref(route_table)
        log.debug("Associated route-table %r with virtual network %r", route_table_id, network_uuid)

        # FIXME: remove this logging after https://st.yandex-team.ru/CLOUD-20918 is resolved
        route_table.fetch()
        log.debug("Network refs; %r, route-table back_refs: %r", list(network.refs), list(route_table.back_refs))

    # FIXME: switch to one stateless association method instead of two statefull https://st.yandex-team.ru/CLOUD-28010
    @measure_latency
    def disassociate_route_table_from_subnet(self, route_table_id: str, network_uuid: str):
        """
            Deletes reference of route table from virtual network
            in contrail and deletes it's routing policies.
        """
        log.debug("Disassociating route-table %r from virtual network %r ...", route_table_id, network_uuid)

        try:
            route_table = self._get_route_table(route_table_id)
        except exceptions.ResourceNotFoundError as e:
            log.info("Route table %r does not exists; skipping", route_table_id)
            return

        network = self.resource("virtual-network", uuid=network_uuid, fetch=True)

        # Keep other associated route tables
        route_tables_routing_policies = []
        for old_route_table in network.refs.route_table:
            old_route_table_id = naming.route_table_id_from_fq_name(old_route_table.fq_name)
            if old_route_table_id == route_table_id:
                continue
            route_tables_routing_policies.append(self._get_route_table_routing_policy(old_route_table_id))

        routing_policies = chain(
            route_tables_routing_policies,
            self._get_or_create_network_default_routing_policies(policy_key=network_uuid)
        )
        self.set_network_routing_policies(network_uuid, routing_policies)

        network.remove_ref(route_table)
        log.debug("Disassociated route-table %r from virtual network %r", route_table_id, network_uuid)

        # FIXME: remove this logging after https://st.yandex-team.ru/CLOUD-20918 is resolved
        route_table.fetch()
        log.debug("Network refs; %r, route-table back_refs: %r", list(network.refs), list(route_table.back_refs))

        # FIXME: remove this logging after the https://st.yandex-team.ru/CLOUD-20918 investigation
        if network.refs.route_table:
            log.error(
                "Route-table %r (%r) dissassociated from virtual-network %r, but found in network refs",
                route_table.display_name, route_table.uuid, network_uuid
            )


class VirtualDNSManager:
    MAX_VDNS_CHAIN_LENGTH = 10

    def __init__(self, client: NativeNetworkClient, network_uuid: str):
        self._client = client
        self._network_uuid = network_uuid
        self._vdns_list = None

    def _fetch_dns_list(self, strict=True):
        log.debug("Fetching virtual-DNS chain from Contrail for virtual-network %r...", self._network_uuid)
        try:
            network = self._client.resource("virtual-network", uuid=self._network_uuid, fetch=True)
        except exceptions.ResourceNotFoundError as e:
            if strict:
                log.error("virtual-network %r not found.", self._network_uuid)
                raise e
            log.warning("virtual-network %r not found.", self._network_uuid)
            self._vdns_list = []
            return

        ipam = self._client._get_referred_ipam(network)
        vdns = self._client._try_get_ipam_vdns(ipam)
        self._vdns_list = []

        if not vdns:
            log.warning("No vDNS attached to network-ipam of virtual-network %r.", self._network_uuid)
            return

        log.debug("Fetched vDNS %r with domain-name %r.", vdns.uuid, vdns.virtual_DNS_data["domain_name"])
        self._vdns_list.append(vdns)

        for _ in range(self.MAX_VDNS_CHAIN_LENGTH):
            next_vdns_name = vdns.virtual_DNS_data.get("next_virtual_DNS")
            log.debug("Next vDNS for %r is %r.", vdns.uuid, next_vdns_name)
            if not next_vdns_name or _is_ip_address(next_vdns_name):
                break
            vdns = self._client.resource("virtual-DNS", fq_name=next_vdns_name, fetch=True)
            log.debug("Fetched vDNS %r with domain name %r.", vdns.uuid, vdns.virtual_DNS_data["domain_name"])
            self._vdns_list.append(vdns)

    def _find_vdns(self, record_type, record_name, strict=True):
        log.debug("Trying to find virtual-DNS for %r %r in virtual-network %r...",
                  record_type, record_name, self._network_uuid)

        if self._vdns_list is None:
            self._fetch_dns_list(strict)

        if len(self._vdns_list) == 0:
            message = "No vDNS found for virtual-network {!r}.".format(self._network_uuid)
            if strict:
                raise Error(message)
            else:
                log.warning(message)
                return None

        # CLOUD-8974: Always create PTR records in IPAM's vDNS
        if record_type == "PTR":
            vdns = self._vdns_list[0]
            log.debug("Found vDNS %r.", vdns.uuid)
            return vdns

        for vdns in self._vdns_list:
            if record_name.endswith("." + vdns.virtual_DNS_data["domain_name"]):
                log.debug("Found vDNS %r.", vdns.uuid)
                return vdns

        message = "No vDNS found for {!r} {!r} in virtual-network {!r}.".format(
            record_type, record_name, self._network_uuid)
        if strict:
            raise Error(message)
        else:
            log.warning(message)
            return None

    def save_dns_record(self, resource_name, record_type, record_name, record_value, record_ttl=None):
        log.debug("Saving vDNS record (%r, %r, %r, %r) with name %r to network %r...",
                  record_type, record_name, record_value, record_ttl, resource_name, self._network_uuid)

        vdns = self._find_vdns(record_type, record_name, strict=False)
        if vdns is None:
            log.warning("No vDNS found, skipping vDNS record saving.")
            return

        resource_fq_name = list(vdns.fq_name) + [resource_name]
        dns_rec = self._client._try_get("virtual-DNS-record", resource_fq_name)

        dns_rec_data = {
            "record_type": record_type,
            "record_name": record_name,
            "record_data": record_value,
            "record_class": "IN",
            "record_ttl_seconds": record_ttl or vdns.virtual_DNS_data["default_ttl_seconds"],
        }

        if dns_rec:
            dns_rec.virtual_DNS_record_data.update(dns_rec_data)
            dns_rec.save()
            log.debug("Updated vDNS record %r with uuid %r", dns_rec.fq_name, dns_rec.uuid)
        else:
            dns_rec = self._client.resource(
                "virtual-DNS-record",
                fq_name=resource_fq_name,
                virtual_DNS_record_data=dns_rec_data,
                parent=vdns)
            dns_rec.create()
            log.debug("Created vDNS record %r with uuid %r", dns_rec.fq_name, dns_rec.uuid)

    def delete_dns_record(self, resource_name, record_type, record_name):
        log.debug("Deleting DNS record (%r, %r) with name %r from network %r...",
                  record_type, record_name, resource_name, self._network_uuid)

        vdns = self._find_vdns(record_type, record_name, strict=False)
        if vdns is None:
            log.warning("No vDNS found, skipping vDNS record deletion.")
            return

        vdns_rec = self._client.resource("virtual-DNS-record", fq_name=list(vdns.fq_name) + [resource_name])
        self._client._delete(vdns_rec, fetch=True)

    def _delete_raw_dns_records(self, records: List[Resource]):
        for record in records:
            if record.type != "virtual-DNS-record":
                log.warning("Trying to delete not a virtual-DNS-record %s (fq_name %r, uuid %r).", record.type, record.fq_name, record.uuid)
                continue

            # Don't fetch
            self._client._delete(record, fetch=False)

    def get_all_dns_records(self) -> Dict[str, Resource]:
        records = {}

        if self._vdns_list is None:
            self._fetch_dns_list()

        for vdns in self._vdns_list:
            if "virtual_DNS_records" not in vdns.data:
                continue
            for record in vdns["virtual_DNS_records"]:
                # Not a full record, just a reference to one
                records[record.fq_name[-1]] = record

        return records

    def plan_dns_record_sync(self, new_records: Dict[str, DnsRecord]) -> Tuple[Dict[str, DnsRecord], List[Resource]]:
        current_records = self.get_all_dns_records()

        to_add = {}
        to_delete = [current_records[resource_name] for resource_name in current_records.keys() - new_records.keys()]

        for resource_name, record in new_records.items():
            if resource_name not in current_records:
                to_add[resource_name] = record
            elif record.ttl is not None or record.type == VirtualDnsRecordType.RecordType.CNAME_TYPE:
                # Are we updating existing record's TTL or CNAME target?
                try:
                    current_records[resource_name].fetch()
                    if (current_records[resource_name].virtual_DNS_record_data["record_ttl_seconds"] != record.ttl or
                        current_records[resource_name].virtual_DNS_record_data["record_data"] != record.value):
                        to_add[resource_name] = record
                except exceptions.ResourceNotFoundError:
                    # Existing record was gone for some reason. We better re-create it now.
                    to_add[resource_name] = record

        return to_add, to_delete

    def sync_dns_records(self, new_records: Dict[str, DnsRecord]):
        to_add, to_delete = self.plan_dns_record_sync(new_records)

        log.debug("Syncing DNS records: +%s-%s...", len(to_add), len(to_delete))

        self._delete_raw_dns_records(to_delete)

        for resource_name, record in to_add.items():
            self.save_dns_record(resource_name, record.type, record.name, record.value, record.ttl)

    def random_faulty_sync_dns_records(self, new_records: Dict[str, DnsRecord]):
        to_add, to_delete = self.plan_dns_record_sync(new_records)

        log.debug("Syncing DNS records: +%s-%s...", len(to_add), len(to_delete))

        current = 0
        total = len(to_add)
        for resource_name, record in to_add.items():
            if current > random.randint(0, total):
                # Fail randomly when creating more that one record. If called several times with
                # same new_records, worst case scenario is when this exception is rased after every
                # first record created.
                raise exceptions.HttpError(message="Fault Injection", http_status=666)
            self.save_dns_record(resource_name, record.type, record.name, record.value, record.ttl)
            current += 1

        self._delete_raw_dns_records(to_delete)
