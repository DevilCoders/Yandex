from infra.awacs.proto import api_pb2, api_stub
import requests

from nanny_rpc_client import RequestsRpcClient
from nanny_rpc_client.exceptions import NotFoundError, ConflictError

from tools.releaser.src.cli import utils


# https://a.yandex-team.ru/arc/trunk/arcadia/infra/awacs/proto/


class AwacsClient:
    def __init__(self, namespace):
        oauth_token = utils.get_oauth_token_or_panic()
        client = RequestsRpcClient('https://awacs.yandex-team.ru/api/', oauth_token)
        self.namespace = namespace
        self.balancer_service = api_stub.BalancerServiceStub(client)
        self.domain_service = api_stub.DomainServiceStub(client)
        self.upstream_service = api_stub.UpstreamServiceStub(client)
        self.backend_service = api_stub.BackendServiceStub(client)

    def get_balancers(self):
        request = api_pb2.ListBalancersRequest(namespace_id=self.namespace)
        return self.balancer_service.list_balancers(request).balancers

    def update_balancer(self, balancer):
        request = api_pb2.UpdateBalancerRequest(spec=balancer.spec, meta=balancer.meta)
        return self.balancer_service.update_balancer(request)

    def get_domain(self, name):
        request = api_pb2.GetDomainRequest(namespace_id=self.namespace, id=name)
        try:
            return self.domain_service.get_domain(request).domain
        except NotFoundError:
            return None

    def get_upstreams(self, upstream_ids):
        request = api_pb2.ListUpstreamsRequest(
            namespace_id=self.namespace,
            query=api_pb2.ListUpstreamsQuery(id_regexp=f'^({"|".join(upstream_ids)})$'),
        )
        return self.upstream_service.list_upstreams(request).upstreams

    def get_backends(self, backend_ids):
        request = api_pb2.ListBackendsRequest(
            namespace_id=self.namespace,
            query=api_pb2.ListBackendsQuery(id_regexp=f'^({"|".join(backend_ids)})$'),
        )
        return self.backend_service.list_backends(request).backends

    def copy_backend(self, backend, stage, deploy_units_with_cluster):
        endpoint_sets = [
            api_pb2.model__pb2.BackendSelector.YpEndpointSet(
                endpoint_set_id=f'{stage}.{deploy_unit}',
                cluster=cluster,
            )
            for deploy_unit, cluster in deploy_units_with_cluster
        ]

        request = api_pb2.CreateBackendRequest(
            meta=api_pb2.model__pb2.BackendMeta(),
            spec=api_pb2.model__pb2.BackendSpec(
                selector=api_pb2.model__pb2.BackendSelector(
                    type=api_pb2.model__pb2.BackendSelector.Type.YP_ENDPOINT_SETS_SD,
                    yp_endpoint_sets=endpoint_sets,
                ),
            ),
        )
        request.meta.CopyFrom(backend.meta)
        request.meta.id = stage

        try:
            return self.backend_service.create_backend(request).backend
        except ConflictError:
            request = api_pb2.UpdateBackendRequest(
                meta=request.meta,
                spec=request.spec,
            )
            return self.backend_service.update_backend(request).backend

    def copy_upstream(self, upstream, stage, backend_id):
        request = api_pb2.CreateUpstreamRequest(
            meta=api_pb2.model__pb2.UpstreamMeta(),
            spec=api_pb2.model__pb2.UpstreamSpec(),
        )
        request.meta.CopyFrom(upstream.meta)
        request.meta.id = stage
        request.spec.CopyFrom(upstream.spec)
        request.spec.yandex_balancer.config.l7_upstream_macro.id = request.meta.id
        request.spec.yandex_balancer.yaml = ''
        request.spec.yandex_balancer.config.l7_upstream_macro.flat_scheme.backend_ids[:] = [backend_id]

        try:
            return self.upstream_service.create_upstream(request).upstream
        except ConflictError:
            request = api_pb2.UpdateUpstreamRequest(
                meta=request.meta,
                spec=request.spec,
            )
            return self.upstream_service.update_upstream(request).upstream

    def copy_domain(self, domain, domain_name, upstreams):
        request = api_pb2.CreateDomainRequest(
            meta=api_pb2.model__pb2.DomainMeta(),
            order=api_pb2.model__pb2.DomainOrder.Content(),
        )
        request.meta.CopyFrom(domain.meta)
        request.meta.id = domain_name

        request.order.protocol = domain.spec.yandex_balancer.config.protocol
        request.order.include_upstreams.CopyFrom(domain.spec.yandex_balancer.config.include_upstreams)
        request.order.cert_ref.CopyFrom(domain.spec.yandex_balancer.config.cert)
        request.order.redirect_to_https.CopyFrom(domain.spec.yandex_balancer.config.redirect_to_https)
        request.order.type = domain.spec.yandex_balancer.config.type

        request.order.fqdns[:] = [domain_name]
        request.order.include_upstreams.ids[:] = [upstream.meta.id for upstream in upstreams]

        return self.domain_service.create_domain(request).domain


class AwacsHttpClient:
    def __init__(self):
        oauth_token = utils.get_oauth_token_or_panic()
        self.headers = {'Authorization': 'OAuth {}'.format(oauth_token)}
        self.host = 'https://awacs.yandex-team.ru/api'

    def enable_pushclient(self, balancer_id, namespace_id, version):
        url='{}/{}/'.format(self.host, 'EnableBalancerPushclient')
        data = {
            'id': balancer_id,
            'namespace_id': namespace_id,
            'version': version,
            'comment': "Pushclient enabled via releaser"
        }
        response = requests.post(url=url, json=data, headers=self.headers)
        if response.status_code == 400:
            json_response = response.json()
            if json_response['message'] == 'Pushclient is already enabled':
                return
        response.raise_for_status()
