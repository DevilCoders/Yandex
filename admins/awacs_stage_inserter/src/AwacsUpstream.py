#!/usr/bin/env python

import os
import re
import yaml
import nanny_rpc_client

from infra.awacs.proto import model_pb2, api_stub, api_pb2
from logging_config import LoggingConfig


class AwacsUpstream(object):
    def __init__(self, config):
        self.config = config
        logging_config = LoggingConfig(self.__class__.__name__)
        self.logger = logging_config.get_logger()
        self.nanny_client = nanny_rpc_client.RequestsRpcClient(self.config.url,
                                                               request_timeout=10,
                                                               oauth_token=self.config.token)
        self.upstream_stub = api_stub.UpstreamServiceStub(self.nanny_client)

    def fill_defaults(self, yaml_loaded):
        dcs = []
        for k, v in self.config.backends_by_dc.items():
            dcs.append({'name': k, 'backend_ids': [v]})

        path_re = ')|('.join(self.config.fqdns)
        path_re = re.sub(r'\.', '\\.', path_re)
        path_re = '({})'.format(path_re)
        self.logger.info('Using host_re in matcher: {}'.format(path_re))

        if not yaml_loaded['l7_upstream_macro']['id']:
            yaml_loaded['l7_upstream_macro']['id'] = self.config.upstream_id
        if not yaml_loaded['l7_upstream_macro']['monitoring']['uuid']:
            yaml_loaded['l7_upstream_macro']['monitoring']['uuid'] = self.config.upstream_id
        if not yaml_loaded['l7_upstream_macro']['matcher']['host_re']:
            yaml_loaded['l7_upstream_macro']['matcher']['host_re'] = path_re
        if not yaml_loaded['l7_upstream_macro']['by_dc_scheme']['dcs']:
            yaml_loaded['l7_upstream_macro']['by_dc_scheme']['dcs'] = dcs
        if not yaml_loaded['l7_upstream_macro']['by_dc_scheme']['dc_balancer']['attempts']:
            yaml_loaded['l7_upstream_macro']['by_dc_scheme']['dc_balancer']['attempts'] = min(len(dcs), 2)

        return yaml_loaded

    def read_upstream_config_from_file(self, upstream_config):
        upstream_yaml = ''
        with open(upstream_config, 'r') as __fd__:
            upstream_yaml = __fd__.read()
        try:
            yaml_loaded = yaml.safe_load(upstream_yaml)
        except Exception as _:
            self.logger.error('Parse error: invalid yaml config format')
        return yaml.safe_dump(self.fill_defaults(yaml_loaded))

    def make_yaml_upstream(self):
        yaml_loaded = {
            'l7_upstream_macro': {
                'version': '0.2.0',
                'id': [],
                'matcher': {
                    'host_re': []
                },
                'monitoring': {
                    'uuid': []
                },
                'by_dc_scheme': {
                    'dc_balancer': {
                        'weights_section_id': 'bygeo',
                        'method': 'LOCAL_THEN_BY_DC_WEIGHT',
                        'attempts': []
                    },
                    'balancer': {
                        'attempts': 1,
                        'max_reattempts_share': 0.15,
                        'max_pessimized_endpoints_share': 0.2,
                        'fast_attempts': 2,
                        'retry_http_responses': {
                            'codes': [
                                '5xx'
                            ]
                        },
                        'retry_non_idempotent': False,
                        'connect_timeout': '70ms',
                        'backend_timeout': '10s'
                    },
                    'dcs': [],
                    'on_error': {
                        'static': {
                            'status': 504,
                            'content': 'Service unavailable'
                        }
                    }
                }
            }
        }

        return yaml.safe_dump(self.fill_defaults(yaml_loaded))

    def create_upstream(self):
        if getattr(self.config, 'upstream_config', None) and os.path.isfile(self.config.upstream_config):
            upstream_yaml = self.read_upstream_config_from_file(self.config.upstream_config)
        else:
            upstream_yaml = self.make_yaml_upstream()
        self.logger.error('Upstream {} not found. Going to create it.'.format(self.config.upstream_id))
        req_pb = api_pb2.CreateUpstreamRequest()
        req_pb.meta.id = self.config.upstream_id
        req_pb.meta.namespace_id = self.config.namespace
        req_pb.meta.auth.type = req_pb.meta.auth.STAFF
        req_pb.meta.auth.staff.owners.logins.extend(self.config.users + ['nanny-robot'])
        req_pb.meta.auth.staff.owners.group_ids.extend(list(map(str, self.config.guids)))
        req_pb.meta.comment = 'Created from awacs_stage_inserter'
        req_pb.spec.type = model_pb2.YANDEX_BALANCER
        req_pb.spec.yandex_balancer.mode = model_pb2.YandexBalancerUpstreamSpec.EASY_MODE2
        req_pb.spec.yandex_balancer.yaml = upstream_yaml
        req_pb.spec.labels['order'] = '10000000'

        result = self.upstream_stub.create_upstream(req_pb)
        self.logger.info('Upstream `{}` created:\n{}'.format(self.config.upstream_id, result))

    def remove_upstream(self):
        req_pb = api_pb2.RemoveUpstreamRequest()
        req_pb.id = self.config.upstream_id
        req_pb.namespace_id = self.config.namespace
        req_pb.version = self.get_upstream().upstream.meta.version

        result = self.upstream_stub.remove_upstream(req_pb)
        self.logger.info('Upstream `{}` removed:\n{}'.format(self.config.upstream_id, result))

    def get_upstream(self):
        get_req_pb = api_pb2.GetUpstreamRequest(
            id=self.config.upstream_id,
            namespace_id=self.config.namespace
        )
        resp_pb = self.upstream_stub.get_upstream(get_req_pb)
        return resp_pb
