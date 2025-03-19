# -*- coding: utf-8 -*-
"""
DBaaS Internal API network utils
"""
from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum
from flask import current_app, g
from typing import Collection, Dict, Optional, Iterable, TypedDict, NamedTuple, List

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute import vpc
from cloud.mdb.internal.python.compute.vpc import Network
from . import metadb
from .config import vtypes_with_networks
from .logs import log_warn, get_logger
from ..core.auth import AuthError, check_action
from ..core.exceptions import (
    NonUniqueSubnetInZone,
    NoSubnetInZone,
    PreconditionFailedError,
    SubnetNotFound,
)
from ..utils import iam_jwt
from ..utils.request_context import get_x_request_id
from ..utils.types import VTYPE_PORTO

from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from cloud.mdb.internal.python.racktables import RacktablesClient, RacktablesClientConfig
from cloud.mdb.internal.python.racktables.errors import RacktablesNotFoundError
from cloud.mdb.internal.python.yandex_team.abc import ABCClient, ABCClientConfig

CONST_MAX_USER_SECURITY_GROUP = 10
ACTION_USE_SUBNET = 'vpc.subnets.use'
ACTION_CREATE_EXTERNAL_ADDRESS = 'vpc.addresses.createExternal'


@dataclass
class YCNetwork:
    """
    Wrapper around yc network
    """

    network_id: str
    folder_id: str
    default_security_group_id: str = ''

    @staticmethod
    def from_network(network: Network) -> 'YCNetwork':
        return YCNetwork(
            network_id=network.network_id,
            folder_id=network.folder_id,
            default_security_group_id=network.default_security_group_id,
        )


SubnetsIDsByZones = Dict[str, Dict[str, str]]


class SubnetInfo(TypedDict):
    id: str
    folderId: str
    zoneId: str
    networkId: str
    egressNatEnable: bool
    v4CidrBlock: List[str]


class SecurityGroupRuleDirection(Enum):
    UNSPECIFIED = 0
    INGRESS = 1
    EGRESS = 2


class SecurityGroupRule(NamedTuple):
    id: str
    description: str
    direction: SecurityGroupRuleDirection
    ports_from: Optional[int]
    ports_to: Optional[int]
    protocol_name: str
    protocol_number: int
    v4_cidr_blocks: List[str]
    v6_cidr_blocks: List[str]
    predefined_target: Optional[str]
    security_group_id: Optional[str]


class SecurityGroup(NamedTuple):
    id: str
    name: str
    folder_id: str
    network_id: str
    rules: List[SecurityGroupRule]


class NetworkOwnerServiceResolveError(PreconditionFailedError):
    def __init__(self, macro_name, owner):
        self.macro_name = macro_name
        self.owner = owner
        super().__init__('Bad service owner for macro \'{0}\': \'{1}\''.format(macro_name, owner))


class NetworkMacroIDTooLong(PreconditionFailedError):
    def __init__(self, macro_id):
        self.macro_id = macro_id
        super().__init__('Macro ID \'{0}\' is longer than 8 symbols'.format(macro_id))


class NetworkNotFound(PreconditionFailedError):
    """
    Client requested creation of host with network,
    but we were unable to find it
    """

    def __init__(self, network_id):
        self.network_id = network_id
        super().__init__('Unable to find network \'{0}\''.format(network_id))


# pylint: disable=too-few-public-methods
class NetworkProvider(ABC):
    """
    Abstract Network Provider
    """

    @abstractmethod
    def get_subnets(self, network: YCNetwork) -> SubnetsIDsByZones:
        """
        Get map geo->list of subnets for network_id in folder
        """

    @abstractmethod
    def get_network(self, network_id: str) -> YCNetwork:
        """
        Resolve network_id
        """

    @abstractmethod
    def get_subnet(self, subnet_id: str, public_ip_used: bool) -> SubnetInfo:
        """
        Get subnet info for subnet_id
        """

    @abstractmethod
    def get_security_group(self, sg_id: str) -> SecurityGroup:
        """
        Get security group for security group id
        """

    @abstractmethod
    def get_networks(self, network_id: str) -> List[YCNetwork]:
        """
        Get networks in folder_id
        """


def group_subnets_by_zones(subnets: Iterable[vpc.Subnet]) -> SubnetsIDsByZones:
    """
    group subnets by zones
    """
    ret: SubnetsIDsByZones = {}
    for subnet in subnets:
        if subnet.zone_id not in ret:
            ret[subnet.zone_id] = {}

        ret[subnet.zone_id][subnet.subnet_id] = subnet.folder_id
    return ret


class YCNetworkProvider(NetworkProvider):
    """
    YC.Network Provider
    """

    def __init__(self, config) -> None:
        self.iam_jwt = None
        if not config.get('token'):
            self.iam_jwt = iam_jwt.get_provider()

        self._vpc = vpc.VPC(
            config=vpc.Config(
                transport=grpcutil.Config(
                    url=config['url'],
                    cert_file=config['ca_path'],
                ),
                timeout=config.get('timeout', 5.0),
            ),
            logger=get_logger(),
            token_getter=lambda: self.iam_jwt.get_iam_token() if self.iam_jwt is not None else config['token'],
        )
        self._vpc_api_available = bool(config['url'])

    def _api_available(self) -> bool:
        """
        workaround for current porto installation.
        We don't have network api and don't expect it,
        but cli+gw - require it
        """
        return self._vpc_api_available

    def get_subnet(self, subnet_id, public_ip_used) -> SubnetInfo:
        """
        Get subnet info by id
        """
        try:
            subnet = self._vpc.get_subnet(subnet_id)
        except vpc.SubnetNotFoundError:
            raise AuthError(f'Unable to find subnet {subnet_id}')

        check_action(action=ACTION_USE_SUBNET, folder_ext_id=subnet.folder_id)
        if public_ip_used:
            check_action(action=ACTION_CREATE_EXTERNAL_ADDRESS, folder_ext_id=subnet.folder_id)
        # Convert to REST style dict
        return {
            'id': subnet.subnet_id,
            'folderId': subnet.folder_id,
            'zoneId': subnet.zone_id,
            'networkId': subnet.network_id,
            'egressNatEnable': subnet.egress_nat_enable,
            'v4CidrBlock': subnet.v4_cidr_blocks,
        }

    def get_subnets(self, network) -> SubnetsIDsByZones:
        if not self._api_available():
            if network.network_id:
                log_warn(
                    'Network api unavailable. Return empty subnets for network_id: %r, folder_id: %r',
                    network.network_id,
                    network.folder_id,
                )
            return {}

        return group_subnets_by_zones(self._vpc.get_subnets(network_id=network.network_id))

    def get_network(self, network_id: str) -> YCNetwork:
        if not self._api_available():
            if network_id:
                log_warn('Network api unavailable. ignore network_id: %r', network_id)
            return YCNetwork(network_id='', folder_id='')

        try:
            network = self._vpc.get_network(network_id)
        except vpc.NetworkNotFoundError:
            raise AuthError(f'Unable to find network {network_id}')
        return YCNetwork.from_network(network)

    def get_security_group(self, sg_id: str) -> SecurityGroup:
        if not self._api_available():
            if sg_id:
                log_warn('Network api unavailable. ignore security_group_id: %r', sg_id)
            return SecurityGroup(id=sg_id, network_id='', folder_id='', name='', rules=[])
        try:
            sg = self._vpc.get_security_group(sg_id)
            return SecurityGroup(
                id=sg.id, network_id=sg.network_id, folder_id=sg.folder_id, name=sg.name, rules=sg.rules
            )
        except vpc.SecurityGroupNotFoundError as exc:
            log_warn('security_group_id: %s, not found: %s', sg_id, exc)
            raise PreconditionFailedError(f'Unable to find security group {sg_id}')

    def get_networks(self, folder_id: str) -> List[YCNetwork]:
        """
        Get networks by folder id
        """
        return [YCNetwork.from_network(network) for network in self._vpc.get_networks(folder_id)]


class PortoNetworkProvider(NetworkProvider):
    """
    Porto Network Provider
    """

    def __init__(self, config) -> None:
        self.logger = MdbLoggerAdapter(
            get_logger(),
            extra={
                'request_id': get_x_request_id(),
            },
        )

        self.iam_jwt = None
        if not config['token']:
            self.iam_jwt = iam_jwt.get_provider()

        self._racktables = RacktablesClient(
            config=RacktablesClientConfig(
                base_url=current_app.config['RACKTABLES_CLIENT_CONFIG']['base_url'],
                oauth_token=current_app.config['RACKTABLES_CLIENT_CONFIG']['oauth_token'],
            )
        )

        self._abc = ABCClient(
            config=ABCClientConfig(
                transport=grpcutil.Config(
                    url=current_app.config['YANDEX_TEAM_INTEGRATION_CONFIG']['url'],
                    cert_file=current_app.config['YANDEX_TEAM_INTEGRATION_CONFIG']['cert_file'],
                )
            ),
            logger=self.logger.copy_with_ctx(client_name="ABCClient"),
            token_getter=lambda: self.iam_jwt.get_iam_token() if self.iam_jwt is not None else config['token'],
            error_handlers={},
        )

    def get_subnets(self, network: YCNetwork) -> SubnetsIDsByZones:
        """
        Get map geo->list of subnets for network_id in folder
        """

        try:
            racktables_network = self._racktables.show_macro(network.network_id)
            macro_id = racktables_network.ids[0].id
            l = len(macro_id)
            if l > 8:
                raise NetworkMacroIDTooLong(macro_id)
            elif l > 4:
                subnet_id = f'{macro_id[:l - 4]}:{macro_id[l - 4:]}'
            else:
                subnet_id = f'0:{macro_id}'

        except RacktablesNotFoundError:
            # TODO: change later to
            #  raise NetworkNotFound(network.network_id)
            self.logger.warn(f'Bad network_id: {network.network_id}')
            return {}
        except GRPCError:
            raise

        zone_ids = ["sas", "vla", "myt", "man", "iva"]
        ret: SubnetsIDsByZones = {}
        for zone_id in zone_ids:
            if zone_id not in ret:
                ret[zone_id] = {}

            ret[zone_id][subnet_id] = network.folder_id
        return ret

    def get_network(self, network_id: str) -> YCNetwork:
        """
        Resolve network_id
        """
        if network_id == "" or network_id == "0":
            return YCNetwork(network_id='', folder_id='')

        try:
            racktables_network = self._racktables.show_macro(network_id)
            service_prefix = 'svc_'
            if not racktables_network.owner_service.startswith(service_prefix):
                raise NetworkOwnerServiceResolveError(racktables_network.name, racktables_network.owner_service)
            abc_slug = racktables_network.owner_service[len(service_prefix) :]
            abc_service = self._abc.resolve(abc_slug=abc_slug)
            return YCNetwork(network_id=network_id, folder_id=abc_service.default_folder_id)
        except RacktablesNotFoundError:
            # TODO: change later to
            #  raise NetworkNotFound(network_id)
            self.logger.warn(f'Bad network_id: {network_id}')
            return YCNetwork(network_id='', folder_id='')
        except GRPCError:
            raise

    def get_subnet(self, subnet_id: str, public_ip_used: bool) -> SubnetInfo:
        """
        Get subnet info for subnet_id
        """
        return {
            'id': '',
            'folderId': '',
            'zoneId': '',
            'networkId': '',
            'egressNatEnable': False,
            'v4CidrBlock': [],
        }

    def get_networks(self, network_id: str) -> List[YCNetwork]:
        """
        Get networks in folder_id
        """
        return []

    def get_security_group(self, sg_id: str) -> SecurityGroup:
        """
        Get security group for security group id
        """
        return SecurityGroup(id=sg_id, network_id='', folder_id='', name='', rules=[])


class MetaNetworkProvider(NetworkProvider):
    """
    Meta Network Provider
    """

    def __init__(self, config) -> None:
        if 'vtype' in config:
            self.vtype = config['vtype']
        else:
            self.vtype = None

        if self.vtype == VTYPE_PORTO:
            self._porto = PortoNetworkProvider(config)
        else:
            self._vpc = YCNetworkProvider(config)

    def get_subnets(self, network: YCNetwork) -> SubnetsIDsByZones:
        """
        Get map geo->list of subnets for network_id in folder
        """
        if self.vtype == VTYPE_PORTO:
            return self._porto.get_subnets(network)
        else:
            return self._vpc.get_subnets(network)

    def get_network(self, network_id: str) -> YCNetwork:
        """
        Resolve network_id
        """
        if self.vtype == VTYPE_PORTO:
            return self._porto.get_network(network_id)
        else:
            return self._vpc.get_network(network_id)

    def get_subnet(self, subnet_id: str, public_ip_used: bool) -> SubnetInfo:
        """
        Get subnet info for subnet_id
        """
        if self.vtype == VTYPE_PORTO:
            return self._porto.get_subnet(subnet_id, public_ip_used)
        else:
            return self._vpc.get_subnet(subnet_id, public_ip_used)

    def get_networks(self, network_id: str) -> List[YCNetwork]:
        """
        Get networks in folder_id
        """
        if self.vtype == VTYPE_PORTO:
            return self._porto.get_networks(network_id)
        else:
            return self._vpc.get_networks(network_id)

    def get_security_group(self, sg_id: str) -> SecurityGroup:
        """
        Get security group for security group id
        """
        if self.vtype == VTYPE_PORTO:
            return self._porto.get_security_group(sg_id)
        else:
            return self._vpc.get_security_group(sg_id)


def get_provider() -> NetworkProvider:
    """
    Get network provider according to config and flags
    """
    return current_app.config['NETWORK_PROVIDER'](current_app.config['NETWORK'])


def get_network(network_id) -> YCNetwork:
    """
    Resolve network_id
    """
    network = get_provider().get_network(network_id)
    if network.network_id == '_PGAASINTERNALNETS_':
        return network
    if network.folder_id and network.folder_id != g.folder['folder_ext_id']:
        identity_provider = current_app.config['IDENTITY_PROVIDER'](current_app.config)

        cloud_ext_id = identity_provider.get_cloud_by_folder_ext_id(network.folder_id)

        if cloud_ext_id != g.cloud['cloud_ext_id']:
            raise AuthError(
                f'Network cloud id ({cloud_ext_id}) and ' f"cluster cloud id ({g.cloud['cloud_ext_id']}) are not equal"
            )
    return network


def get_subnets(network: YCNetwork) -> SubnetsIDsByZones:
    """
    Resolve networks using network api provider
    """
    if not network.network_id:
        return {}

    provider = get_provider()
    return provider.get_subnets(network)


def get_subnet(subnet_id, public_ip_used) -> SubnetInfo:
    """
    Get subnet info using network api provider
    """
    provider = get_provider()
    return provider.get_subnet(subnet_id, public_ip_used)


def get_host_subnet(subnets: dict, geo: str, vtype: str, public_ip_used: bool, requested_subnet: Optional[str]) -> str:
    """
    Get subnet_id for host
    """
    geo_subnets = subnets.get(geo, [])
    if requested_subnet is not None:
        if requested_subnet in geo_subnets:
            check_action(action=ACTION_USE_SUBNET, folder_ext_id=geo_subnets[requested_subnet])
            if public_ip_used:
                check_action(action=ACTION_CREATE_EXTERNAL_ADDRESS, folder_ext_id=geo_subnets[requested_subnet])
            return requested_subnet
        raise SubnetNotFound(requested_subnet, geo)

    if vtype in vtypes_with_networks():
        if not geo_subnets:
            raise NoSubnetInZone(geo)
        if len(geo_subnets) == 1:
            selected_subnet = next(iter(geo_subnets))
            check_action(action=ACTION_USE_SUBNET, folder_ext_id=geo_subnets[selected_subnet])
            if public_ip_used:
                check_action(action=ACTION_CREATE_EXTERNAL_ADDRESS, folder_ext_id=geo_subnets[selected_subnet])
            return selected_subnet
        raise NonUniqueSubnetInZone(geo)

    if vtype == VTYPE_PORTO:
        if not geo_subnets:
            return ''
        if len(geo_subnets) > 1:
            get_logger().info(geo_subnets)
            raise NonUniqueSubnetInZone(geo)

        return next(iter(geo_subnets))

    # Return default not None value
    return ''


def validate_security_groups(sg_ids: Collection[str], network_id) -> None:
    provider = get_provider()
    if not sg_ids:
        return
    if len(sg_ids) > CONST_MAX_USER_SECURITY_GROUP:
        raise PreconditionFailedError(f'too many security groups ({CONST_MAX_USER_SECURITY_GROUP} is the maximum)')
    for sg_id in sg_ids:
        sg = provider.get_security_group(sg_id)
        if network_id and sg.network_id != network_id:
            raise PreconditionFailedError(
                f'Network id ({network_id}) and security group ({sg_id}) network id ({sg.network_id}) are not equal'
            )
        check_action(action='vpc.securityGroups.use', folder_ext_id=sg.folder_id)


def validate_host_public_ip(fqdn, assign_public_ip):
    """
    Validate permissions to use public IP.
    Raises AuthError if public IP not permitted.
    Raises PreconditionFailedError if subnet if IPv6-only.
    """
    if not assign_public_ip:
        return
    host_info = metadb.get_host_info(fqdn)
    if host_info.vtype == 'porto':
        raise PreconditionFailedError('Public ip assignment is not supported in internal MDB')
    # get subnet checks permissions
    subnet = get_subnet(host_info.subnet_id, assign_public_ip)
    if not subnet.get('v4CidrBlock'):
        raise PreconditionFailedError('Public ip assignment in ipv6-only networks is not supported')
