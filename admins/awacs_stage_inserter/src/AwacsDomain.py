#!/usr/bin/env python

import time
import nanny_rpc_client

from infra.awacs.proto import modules_pb2, model_pb2, api_stub, api_pb2
from logging_config import LoggingConfig
# from awacs.model.errors import ConflictError


class AwacsDomain(object):
    def __init__(self, config):
        self.config = config
        logging_config = LoggingConfig(self.__class__.__name__)
        self.logger = logging_config.get_logger()
        self.nanny_client = nanny_rpc_client.RequestsRpcClient(self.config.url,
                                                               request_timeout=10,
                                                               oauth_token=self.config.token)
        self.domain_stub = api_stub.DomainServiceStub(self.nanny_client)

    def insert_upstream_in_domain(self):
        result = None
        while result is None:
            try:
                current_pb = self.get_domain().domain
                upstream_list = current_pb.spec.yandex_balancer.config.include_upstreams.ids
                if self.config.upstream_id in upstream_list:
                    self.logger.info('Upstream `{}` is already present in domain `{}`'.format(self.config.upstream_id, self.config.domain))
                    self.logger.info('Not inserting this upstream in domain...')
                    return False
                comment = 'Updated from awacs_stage_inserter'
                upstream_list.append(self.config.upstream_id)
                req_pb = api_pb2.CreateDomainOperationRequest()
                req_pb.meta.id = self.config.domain
                req_pb.meta.namespace_id = self.config.namespace
                req_pb.meta.comment = comment
                req_pb.order.set_upstreams.include_upstreams.type = modules_pb2.BY_ID
                req_pb.order.set_upstreams.include_upstreams.ids.extend(upstream_list)

                result = self.domain_stub.create_domain_operation(req_pb)
            except:  # ConflictError as e: # noqa: E722
                self.logger.error('Other blocking domain operation is still in progress')
                self.logger.error('Need to wait a little bit. Waiting...')
                time.sleep(1)

        self.logger.info('Domain `{}` updated:\n{}'.format(self.config.domain, result))
        return True

    def remove_upstream_from_domain(self):
        result = None
        while result is None:
            try:
                current_pb = self.get_domain().domain
                upstream_list = current_pb.spec.yandex_balancer.config.include_upstreams.ids

                if self.config.upstream_id not in upstream_list:
                    self.logger.info('Upstream `{}` is not present within domain `{}`'.format(self.config.upstream_id, self.config.domain))
                    self.logger.info('No need in removing this upstream in domain...')
                    return False
                comment = 'Updated from awacs_stage_inserter'

                upstream_list.remove(self.config.upstream_id)

                req_pb = api_pb2.CreateDomainOperationRequest()
                req_pb.meta.id = self.config.domain
                req_pb.meta.namespace_id = self.config.namespace
                req_pb.meta.comment = comment
                req_pb.order.set_upstreams.include_upstreams.type = modules_pb2.BY_ID
                del req_pb.order.set_upstreams.include_upstreams.ids[:]
                req_pb.order.set_upstreams.include_upstreams.ids.extend(upstream_list)

                result = self.domain_stub.create_domain_operation(req_pb)
            except:  # ConflictError as e: # noqa: E722
                self.logger.error('Other blocking domain operation is still in progress')
                self.logger.error('Need to wait a little bit. Waiting...')
                time.sleep(1)

        self.logger.info('Domain `{}` updated:\n{}'.format(self.config.domain, result))
        return True

    def create_domain(self):
        req_pb = api_pb2.CreateDomainRequest()
        req_pb.meta.id = self.config.domain
        req_pb.meta.namespace_id = self.config.namespace
        req_pb.meta.comment = 'Create domain from awacs_stage_inserter'
        req_pb.order.protocol = model_pb2.DomainSpec.Config.Protocol.HTTP_AND_HTTPS
        req_pb.order.fqdns.extend(self.config.fqdns)
        include_upstreams = req_pb.order.include_upstreams
        include_upstreams.type = modules_pb2.IncludeUpstreamsType.BY_ID
        include_upstreams.ids.extend([self.config.upstream_id])
        cert_order_content = req_pb.order.cert_order.content
        cert_order_content.abc_service_id = int(self.config.abc_id)
        cert_order_content.ca_name = 'InternalCA'
        cert_order_content.common_name = self.config.fqdns[0]
        if len(self.config.fqdns) > 1:
            cert_order_content.subject_alternative_names.extend(self.config.fqdns[1:])
        cert_order_content.public_key_algorithm_id = 'rsa'
        try:
            resp_pb = self.domain_stub.create_domain(req_pb)
            self.logger.debug('Start create_domain method request')
        except Exception as exc:
            self.logger.error('Got exception {}'.format(exc))
            return
        self.logger.debug('Started creation of domain and cert {}'.format(resp_pb))

    def get_domain(self):
        get_req_pb = api_pb2.GetDomainRequest(
            id=self.config.domain,
            namespace_id=self.config.namespace
        )
        resp_pb = self.domain_stub.get_domain(get_req_pb)
        return resp_pb
