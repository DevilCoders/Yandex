# -*- coding: utf-8 -*-
"""
Config-related functions
"""

from dbaas_common.config import parse_config
from dbaas_worker.config import get_config as parse_worker_config

__all__ = [
    'get_config',
]

DEFINITIONS = {
    'host': {
        'type': 'object',
        'properties': {
            'fqdn': {
                'type': 'string',
            },
            'geo': {
                'type': 'string',
            },
        },
        'required': [
            'fqdn',
            'geo',
        ],
    },
    'host_group': {
        'type':
            'object',
        'properties': {
            'hosts': {
                'type': 'array',
                'items': {
                    '$ref': '#/definitions/host',
                },
            },
            'conductor_group': {
                'type': 'string',
            },
            'cores': {
                'type': 'integer',
            },
            'memory': {
                'type': 'integer',
            },
            'platform_id': {
                'type': 'string',
            },
            'disk_type_id': {
                'type': 'string',
            },
            'image_type': {
                'type': 'string',
            },
            'deploy_type': {
                'type': 'string',
            },
            'root_size': {
                'type': 'integer',
            },
            'data_size': {
                'type': 'integer',
            },
            'networks': {
                'type': 'array',
                'items': {
                    'oneOf': [{
                        'type': 'object',
                        'properties': {
                            'network_id': {
                                'type': 'string',
                            },
                            'folder_id': {
                                'type': 'string',
                            },
                        },
                        'required': [
                            'network_id',
                            'folder_id',
                        ],
                    }, {
                        'type': 'string',
                    }],
                },
            },
            'balancer': {
                'type': 'string',
            },
        },
        'required': [
            'hosts',
            'conductor_group',
            'cores',
            'memory',
            'platform_id',
            'disk_type_id',
            'image_type',
            'deploy_type',
            'root_size',
            'networks',
        ],
    },
}

CONFIG_SCHEMA = {
    'description':
        'JSON Schema for DBaaS Bootstrap config',
    'type':
        'object',
    'definitions':
        DEFINITIONS,
    'properties': {
        'selfdns_token': {
            'type': 'string',
        },
        'worker_config': {
            'type': 'string',
        },
        'external_salt': {
            'type': 'string',
        },
        'internal_salt': {
            'type': 'string',
        },
        'conductor_group': {
            'type': 'string',
        },
        'api_access_id': {
            'type': 'string',
        },
        'api_encrypted_access_secret': {
            'type': 'string',
        },
        'configuration': {
            'type':
                'object',
            'properties': {
                'salt': {
                    '$ref': '#/definitions/host_group',
                },
                'zk': {
                    '$ref': '#/definitions/host_group',
                },
                'metadb': {
                    '$ref': '#/definitions/host_group',
                },
                'mdb-health': {
                    '$ref': '#/definitions/host_group',
                },
                'api': {
                    '$ref': '#/definitions/host_group',
                },
                'worker': {
                    '$ref': '#/definitions/host_group',
                },
                'mdb-internal-api': {
                    '$ref': '#/definitions/host_group',
                },
                'e2e': {
                    '$ref': '#/definitions/host_group',
                },
                'mdb-dns': {
                    '$ref': '#/definitions/host_group',
                },
                'mdb-secrets-db': {
                    '$ref': '#/definitions/host_group',
                },
                'mdb-secrets-api': {
                    '$ref': '#/definitions/host_group',
                },
            },
            'required': [
                'salt',
                'zk',
                'metadb',
                'mdb-health',
                'api',
                'worker',
                'mdb-internal-api',
                'e2e',
                'mdb-dns',
                'mdb-secrets-db',
                'mdb-secrets-api',
            ],
        },
    },
    'required': [
        'selfdns_token',
        'worker_config',
        'conductor_group',
        'external_salt',
        'internal_salt',
        'configuration',
        'api_access_id',
        'api_encrypted_access_secret',
    ],
}

DEFAULT_CONFIG = {}


def get_config(conffile):
    """
    Parses and returns config
    """
    config = parse_config(conffile, CONFIG_SCHEMA, default=DEFAULT_CONFIG)
    worker_config = parse_worker_config(config.worker_config)
    return config, worker_config
