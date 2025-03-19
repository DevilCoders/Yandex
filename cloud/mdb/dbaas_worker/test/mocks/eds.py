from collections import namedtuple
from typing import Dict

from yandex.cloud.priv.dns.v1.dns_zone_pb2 import DnsZone

from google.protobuf import any_pb2
from envoy.config.endpoint.v3.endpoint_pb2 import ClusterLoadAssignment
from envoy.config.endpoint.v3.endpoint_components_pb2 import LocalityLbEndpoints, LbEndpoint, Endpoint
from envoy.service.discovery.v3.discovery_pb2 import DiscoveryResponse


TEST_DNS_ZONE = 'test-zone-name.'


DnsRecord = namedtuple('DnsRecord', 'name, type, ttl, data')


def eds(mocker, state):
    cnames = state['eds']['dns_cname_records']
    cname: Dict[str, DnsRecord]

    def dns_zone_get(request, **kwargs):
        return DnsZone(
            zone=TEST_DNS_ZONE,
        )

    def dns_record_upsert(request, **kwargs):
        for record in request.replacements:
            cnames[record.name] = DnsRecord(
                name=record.name,
                type=record.type,
                ttl=record.ttl,
                data=record.data,
            )
        for record in request.deletions:
            if record.name in cnames:
                del cnames[record.name]
        return

    def match(name, mask):
        nparts = name.split('.')
        mparts = mask.split('.')
        if len(nparts) != len(mparts):
            return False
        for npart, mpart in zip(nparts, mparts):
            if mpart != npart and mpart != '*':
                return False
        return True

    def eds_regular_search(mask):
        messages = []
        for name, record in cnames.items():
            fullname = f'{name}.{TEST_DNS_ZONE}'.rstrip('.')
            if match(fullname, mask):
                for fqdn in record.data:
                    msg = any_pb2.Any()
                    submessage = ClusterLoadAssignment(
                        endpoints=[LocalityLbEndpoints(lb_endpoints=[LbEndpoint(endpoint=Endpoint(hostname=fqdn))])]
                    )
                    msg.Pack(submessage)
                    messages.append(msg)
        return messages

    def eds_reverse_search(mask):
        messages = []
        for name, record in cnames.items():
            if mask in record.data:
                msg = any_pb2.Any()
                full_cname = f'{name}.{TEST_DNS_ZONE}'
                submessage = ClusterLoadAssignment(
                    endpoints=[LocalityLbEndpoints(lb_endpoints=[LbEndpoint(endpoint=Endpoint(hostname=full_cname))])]
                )
                msg.Pack(submessage)
                messages.append(msg)
        return messages

    def eds_discovery(request, **kwargs):
        resources = []
        for rname in request.resource_names:
            if rname.endswith('?reverse-search=true&type=CNAME'):
                messages = eds_reverse_search(rname.removesuffix('?reverse-search=true&type=CNAME'))
            else:
                messages = eds_regular_search(rname)
            resources.extend(messages)

        return DiscoveryResponse(
            resources=resources,
        )

    eds_stub = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.eds.EndpointDiscoveryServiceStub',
    )
    eds_stub.return_value.FetchEndpoints = eds_discovery

    dns_service = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.eds.DnsZoneServiceStub',
    )
    dns_service.return_value.Get = dns_zone_get
    dns_service.return_value.UpsertRecordSets = dns_record_upsert

    jwt = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.eds.IamJwt').return_value
    jwt.get_token.side_effect = lambda: 'some-jwt-token'
