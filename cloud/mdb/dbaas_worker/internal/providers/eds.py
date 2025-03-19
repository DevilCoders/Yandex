import grpc

from typing import NamedTuple, Optional

from google.protobuf.json_format import MessageToDict

from envoy.config.core.v3 import base_pb2
from envoy.config.endpoint.v3 import endpoint_pb2
from envoy.service.discovery.v3 import discovery_pb2
from envoy.service.endpoint.v3.eds_pb2_grpc import EndpointDiscoveryServiceStub

from yandex.cloud.priv.dns.v1.dns_zone_pb2 import RecordSet
from yandex.cloud.priv.dns.v1.dns_zone_service_pb2_grpc import DnsZoneServiceStub
from yandex.cloud.priv.dns.v1.dns_zone_service_pb2 import GetDnsZoneRequest, UpsertRecordSetsRequest

from cloud.mdb.internal.python import grpcutil

from dbaas_common import retry, tracing
from ..exceptions import ExposedException
from .common import BaseProvider, Change
from .iam_jwt import IamJwt


class EdsAlias(NamedTuple):
    host: str
    cid: str
    root: str
    zone: str

    @staticmethod
    def from_full_cname(cname: str) -> 'EdsAlias':
        parts = cname.split('.')
        if not len(parts) >= 4:
            raise ExposedException(
                f"Wrong format for alias cname {cname}: expecting 4+ point-separated parts, got {len(parts)}"
            )
        alias = EdsAlias(host=parts[0], cid=parts[1], root=parts[2], zone='.'.join(parts[3:]))
        return alias

    @property
    def discovery_mask(self) -> str:
        """Make cname alias with zone suffix. This value is used as EDS discovery mask."""
        return f'{self.host}.{self.cid}.{self.root}.{self.zone}'

    @property
    def dns_name(self) -> str:
        """Make cname alias without zone suffix. This value is used as 'name' in upsert/delete DNS record request."""
        return f'{self.host}.{self.cid}.{self.root}'


class EdsError(ExposedException):
    """
    Base eds error
    """


class EdsApi(BaseProvider):
    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        if self._disabled:
            return

        self.iam_jwt = IamJwt(config, task, queue)

        eds_channel = grpc.insecure_channel(self.config.eds.eds_endpoint)
        eds_channel = tracing.grpc_channel_tracing_interceptor(eds_channel)
        self.eds_stub = EndpointDiscoveryServiceStub(eds_channel)

        dns_transport = grpcutil.Config(
            url=self.config.eds.dns_endpoint,
            cert_file=self.config.eds.cert_file,
        )
        dns_channel = grpcutil.new_grpc_channel(dns_transport)
        dns_channel = tracing.grpc_channel_tracing_interceptor(dns_channel)
        self.dns_service = grpcutil.WrappedGRPCService(
            self.logger,
            dns_channel,
            DnsZoneServiceStub,
            self.config.eds.dns_api_timeout,
            self.iam_jwt.get_token,
            {},
        )

        self.zone_id = config.eds.zone_id
        self.zone = self._get_zone_name(self.zone_id)

    def _get_zone_name(self, zone_id):
        request = GetDnsZoneRequest(dns_zone_id=zone_id)
        response = self.dns_service.Get(request)
        name = response.zone.rstrip('.')
        return name

    @property
    def _disabled(self) -> bool:
        return not self.config.eds.enabled

    @retry.on_exception(grpc.RpcError, factor=1, max_wait=5, max_tries=24)
    def _eds_request(self, resource_names: list[str]):
        request = discovery_pb2.DiscoveryRequest(
            node=base_pb2.Node(
                id="mdb_worker",
                user_agent_name="mdb_worker",
            ),
            resource_names=resource_names,
        )
        try:
            response = self.eds_stub.FetchEndpoints(request)
        except grpc._channel._InactiveRpcError as err:
            if err.code() == grpc.StatusCode.NOT_FOUND:
                return []
            else:
                raise err

        results = []
        for resource in response.resources:
            if resource.Is(endpoint_pb2.ClusterLoadAssignment.DESCRIPTOR):  # type: ignore
                msg = MessageToDict(resource)
                for item in msg['endpoints']:
                    for lb_endpoint in item['lbEndpoints']:
                        fqdn = lb_endpoint['endpoint']['hostname'].rstrip('.')
                        results.append(fqdn)
            else:
                self.logger.warning(f'Unknown resource type {resource.TypeName()} in discovery response')
        return results

    def _eds_discovery_request(self, mask: str):
        resource_names = [mask]
        hosts = self._eds_request(resource_names)
        return hosts

    def _eds_reverse_search_request(self, fqdn: str) -> list[EdsAlias]:
        resource_names = [f"{fqdn}?reverse-search=true&type=CNAME"]
        aliases = [EdsAlias.from_full_cname(cname) for cname in self._eds_request(resource_names)]
        return aliases

    @retry.on_exception(grpc.RpcError, factor=1, max_wait=5, max_tries=24)
    def _upsert_dns_record(self, name: str, value: str):
        replacement = RecordSet(
            name=name,
            type='CNAME',
            ttl=self.config.eds.dns_cname_ttl,
            data=[value],
        )
        request = UpsertRecordSetsRequest(
            dns_zone_id=self.zone_id,
            replacements=[replacement],
        )
        self.dns_service.UpsertRecordSets(request)

    @retry.on_exception(grpc.RpcError, factor=1, max_wait=5, max_tries=24)
    def _delete_dns_record(self, name, value):
        deletion = RecordSet(
            name=name,
            type='CNAME',
            ttl=0,
            data=[value],
        )
        request = UpsertRecordSetsRequest(
            dns_zone_id=self.zone_id,
            deletions=[deletion],
        )
        self.dns_service.UpsertRecordSets(request)

    @retry.on_exception(EdsError, factor=10, max_wait=60, max_tries=6)
    def _registered_check(self, alias: EdsAlias):
        """
        Ensure that cname is successfully sprouted into the EDS
        """
        mask = alias.discovery_mask
        result = self._eds_discovery_request(mask)
        if not result:
            raise EdsError(f'Empty discovery result for {mask}')
        if len(result) > 1:
            raise ExposedException(f'Multiple results when discovery {mask}')

    @retry.on_exception(EdsError, factor=10, max_wait=60, max_tries=6)
    def _unregistered_check(self, alias: EdsAlias):
        """
        Ensure that deleted cname is successfully un-sprouted from the EDS
        """
        mask = alias.discovery_mask
        result = self._eds_discovery_request(mask)
        if len(result) > 1:
            raise ExposedException(f'Multiple results when discovery {mask}')
        if result:
            raise EdsError(f'The unregistered host is still present in EDS: {mask}')

    # pylint: disable=no-self-use
    def _hostname_from_fqdn(self, fqdn):
        hostname = fqdn.split('.')[0]
        return hostname

    def _make_alias(self, fqdn, group, root) -> EdsAlias:
        host = self._hostname_from_fqdn(fqdn)
        alias = EdsAlias(host, group, root, self.zone)
        return alias

    # pylint: disable=no-self-use
    def eds_group_name_from_id(self, group_id):
        return 'db_{0}'.format(group_id.replace('-', '_'))

    @tracing.trace('Eds Host Register')
    def _host_registered(self, fqdn, group, root):
        tracing.set_tag('eds.host.fqdn', fqdn)
        tracing.set_tag('eds.group.name', group)
        tracing.set_tag('eds.group.root', root)

        alias = self._make_alias(fqdn, group, root)
        self._upsert_dns_record(name=alias.dns_name, value=fqdn)
        self._registered_check(alias)

    def _get_alias(self, fqdn) -> Optional[EdsAlias]:
        aliases = self._eds_reverse_search_request(fqdn)
        if len(aliases) > 1:
            raise ExposedException(f'Multiple aliases for host {fqdn}')
        if len(aliases) == 1:
            return aliases[0]
        return None

    @tracing.trace('Eds Host Unregister')
    def _host_unregistered(self, fqdn):
        tracing.set_tag('eds.host.fqdn', fqdn)

        alias = self._get_alias(fqdn)
        if alias:
            self._delete_dns_record(name=alias.dns_name, value=fqdn)
            self._unregistered_check(alias)

    def host_register(self, fqdn, group_id, root):

        if self._disabled:
            return

        group = self.eds_group_name_from_id(group_id)
        self.add_change(
            Change(
                f'eds_host.{fqdn}',
                'registered',
                rollback=lambda task, safe_revision: self._host_unregistered(fqdn),
            )
        )
        self._host_registered(fqdn, group, root)

    def host_unregister(self, fqdn):

        if self._disabled:
            return

        self.add_change(Change(f'eds_host.{fqdn}', 'unregistered'))
        self._host_unregistered(fqdn)

    def _host_has_root(self, host, root):
        alias = self._get_alias(host)
        if not alias:
            raise ExposedException(f'No alias for host {host}')
        actual_root = alias.root
        if actual_root != root:
            group = alias.cid
            self._host_unregistered(host)
            self._host_registered(host, group, root)

    def group_has_root(self, group, root):
        if self._disabled:
            return
        group_name = self.eds_group_name_from_id(group)
        mask = f'*.{group_name}.*.{self.zone}'
        hosts = self._eds_discovery_request(mask)
        for host in hosts:
            self._host_has_root(host, root)
