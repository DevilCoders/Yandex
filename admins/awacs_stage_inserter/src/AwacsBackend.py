#!/usr/bin/env python

import nanny_rpc_client

from infra.awacs.proto import api_stub, api_pb2, model_pb2
from logging_config import LoggingConfig


class AwacsBackend(object):
    def __init__(self, config, backend_id, dc):
        self.config = config
        self.backend_id = backend_id
        self.dc = dc
        logging_config = LoggingConfig(self.__class__.__name__)
        self.logger = logging_config.get_logger()
        self.nanny_client = nanny_rpc_client.RequestsRpcClient(self.config.url,
                                                               request_timeout=10,
                                                               oauth_token=self.config.token)
        self.backends_stub = api_stub.BackendServiceStub(self.nanny_client)
        self.endpoint_set_stub = api_stub.EndpointSetServiceStub(self.nanny_client)

    def create_backend(self):
        self.logger.error('Backend {} not found. Going to create it.'.format(self.backend_id))
        selector = model_pb2.BackendSelector(
            type=model_pb2.BackendSelector.YP_ENDPOINT_SETS_SD,
        )
        selector.yp_endpoint_sets.add(cluster=self.dc, endpoint_set_id=self.config.endpoint_set)
        spec_pb = model_pb2.BackendSpec(selector=selector)
        req_pb = api_pb2.CreateBackendRequest(spec=spec_pb)
        req_pb.meta.id = self.backend_id
        req_pb.meta.namespace_id = self.config.namespace
        req_pb.meta.comment = 'Created from awacs_stage_inserter'
        req_pb.meta.auth.type = req_pb.meta.auth.STAFF
        req_pb.meta.auth.staff.owners.logins.extend(self.config.users + ['nanny-robot'])
        req_pb.meta.auth.staff.owners.group_ids.extend(list(map(str, self.config.guids)))

        result = self.backends_stub.create_backend(req_pb)
        self.logger.info('Backend `{}` created:\n{}'.format(self.backend_id, result))

    def remove_backend(self):
        req_pb = api_pb2.RemoveBackendRequest()
        req_pb.id = self.backend_id
        req_pb.namespace_id = self.config.namespace
        req_pb.version = self.get_backend().backend.meta.version

        result = self.backends_stub.remove_backend(req_pb)
        self.logger.info('Backend `{}` removed:\n{}'.format(self.backend_id, result))

    def get_backend(self):
        get_req_pb = api_pb2.GetBackendRequest(
            id=self.backend_id,
            namespace_id=self.config.namespace
        )
        resp_pb = self.backends_stub.get_backend(get_req_pb)
        return resp_pb
