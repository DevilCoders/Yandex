# -*- coding: utf-8 -*-
"""
Config-related functions
"""
# pylint: disable=too-many-lines
import argparse
from dbaas_common.config import parse_config_set

CONFIG_SCHEMA = {
    'description': 'JSON Schema for DBaaS Worker config',
    'type': 'object',
    'definitions': {
        'image_template': {
            'type': 'object',
            'properties': {
                'template': {
                    'type': 'string',
                },
                'task_arg': {
                    'type': 'string',
                },
                'whitelist': {
                    'type': 'object',
                    'additionalProperties': {'type': 'string'},
                    'minProperties': 1,
                },
            },
            # All properties defined or All properties not defined
            'dependencies': {
                'template': ['task_arg', 'whitelist'],
                'whitelist': ['template', 'task_arg'],
                'task_arg': ['template', 'whitelist'],
            },
            'additionalProperties': False,
        },
        'sg_service_rules': {
            'type': 'array',
            'items': {
                'type': 'object',
                'properties': {
                    'direction': {
                        'enum': ['INGRESS', 'EGRESS', 'BOTH'],
                    },
                    'ports_from': {
                        'type': 'integer',
                    },
                    'ports_to': {
                        'type': 'integer',
                    },
                    'protocol': {
                        'type': 'string',
                    },
                    'v4_cidr_blocks': {
                        'type': 'array',
                        'items': {
                            'type': 'string',
                        },
                    },
                    'v6_cidr_blocks': {
                        'type': 'array',
                        'items': {
                            'type': 'string',
                        },
                    },
                },
                'required': [
                    'direction',
                    'ports_from',
                    'ports_to',
                ],
                'additionalProperties': False,
            },
        },
        'conductor_alt_groups': {
            'type': 'array',
            'items': {
                'type': 'object',
                'properties': {
                    'group_name': {
                        'type': 'string',
                    },
                    'matcher': {
                        'type': 'object',
                        'properties': {
                            'key': {
                                'type': 'string',
                            },
                            'values': {
                                'type': 'array',
                                'minItems': 1,
                                'uniqueItems': True,
                                'items': {
                                    'type': 'string',
                                },
                            },
                        },
                        'required': [
                            'key',
                            'values',
                        ],
                        'additionalProperties': False,
                    },
                },
                'required': [
                    'group_name',
                    'matcher',
                ],
                'additionalProperties': False,
            },
        },
        'eds_alt_roots': {
            'type': 'array',
            'items': {
                'type': 'object',
                'properties': {
                    'root_name': {
                        'type': 'string',
                    },
                    'matcher': {
                        'type': 'object',
                        'properties': {
                            'key': {
                                'type': 'string',
                            },
                            'values': {
                                'type': 'array',
                                'minItems': 1,
                                'uniqueItems': True,
                                'items': {
                                    'type': 'string',
                                },
                            },
                        },
                        'required': [
                            'key',
                            'values',
                        ],
                        'additionalProperties': False,
                    },
                },
                'required': [
                    'root_name',
                    'matcher',
                ],
                'additionalProperties': False,
            },
        },
        'boto3_retries': {
            'type': 'object',
            'properties': {
                'mode': {
                    'enum': ['standard', 'legacy', 'adaptive'],
                },
                'max_attempts': {
                    'type': 'integer',
                },
            },
            'required': [
                'mode',
                'max_attempts',
            ],
            'additionalProperties': False,
        },
    },
    'properties': {
        'environment_name': {
            'type': 'string',
        },
        'main': {
            'description': 'Core worker config',
            'type': 'object',
            'properties': {
                'admin_api_conductor_group': {
                    'type': 'string',
                },
                'log_level': {
                    'enum': ['CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG'],
                },
                'max_tasks': {
                    'type': 'integer',
                    'minimum': 1,
                },
                'acquire_fail_limit': {
                    'type': 'integer',
                    'minimum': 1,
                },
                'poll_interval': {
                    'type': 'integer',
                    'minimum': 1,
                },
                'metadb_dsn': {
                    'type': 'string',
                },
                'metadb_hosts': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'api_sec_key': {
                    'type': 'string',
                },
                'client_pub_key': {
                    'type': 'string',
                },
                'ca_path': {
                    'type': 'string',
                },
                'sentry_dsn': {
                    'oneOf': [
                        {
                            'type': 'null',
                        },
                        {
                            'type': 'string',
                        },
                    ],
                },
                'sentry_environment': {
                    'type': 'string',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'tracing': {
                    'type': 'object',
                    'properties': {
                        'service_name': {
                            'type': 'string',
                        },
                        'disabled': {
                            'type': 'boolean',
                        },
                        'use_env_vars': {
                            'type': 'boolean',
                        },
                        'local_agent': {
                            'type': 'object',
                            'properties': {
                                'reporting_host': {
                                    'type': 'string',
                                },
                                'reporting_port': {
                                    'type': 'string',
                                },
                            },
                            'required': [
                                'reporting_host',
                                'reporting_port',
                            ],
                            'additionalProperties': False,
                        },
                        'queue_size': {
                            'type': 'integer',
                        },
                        'sampler': {
                            'type': 'object',
                            'properties': {
                                'type': {
                                    'type': 'string',
                                },
                                'param': {
                                    'type': 'integer',
                                },
                            },
                            'required': [
                                'type',
                                'param',
                            ],
                            'additionalProperties': False,
                        },
                        'logging': {
                            'type': 'boolean',
                        },
                    },
                    'required': [
                        'disabled',
                        'local_agent',
                        'logging',
                        'queue_size',
                        'sampler',
                        'service_name',
                        'use_env_vars',
                    ],
                    'additionalProperties': False,
                },
            },
            'required': [
                'admin_api_conductor_group',
                'log_level',
                'max_tasks',
                'acquire_fail_limit',
                'poll_interval',
                'metadb_dsn',
                'metadb_hosts',
                'api_sec_key',
                'client_pub_key',
                'ca_path',
                'sentry_dsn',
                'sentry_environment',
                'managed_zone',
                'public_zone',
                'private_zone',
                'tracing',
            ],
            'additionalProperties': False,
        },
        'restart': {
            'description': 'Core worker config',
            'type': 'object',
            'properties': {
                'base': {
                    'type': 'integer',
                    'minimum': 1,
                },
                'max_attempts': {
                    'type': 'integer',
                    'minimum': 0,
                },
            },
            'required': [
                'base',
                'max_attempts',
            ],
            'additionalProperties': False,
        },
        'postgresql': {
            'description': 'PostgreSQL tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_image_type_template': {
                    "$ref": "#/definitions/image_template",
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'dbm_bootstrap_cmd_template': {
                    "$ref": "#/definitions/image_template",
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'create_host_pillar',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'compute_image_type',
                'compute_image_type_template',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'dbm_bootstrap_cmd_template',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'clickhouse': {
            'description': 'ClickHouse tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'aws_image_type': {
                    'type': 'string',
                },
                'aws_root_disk_type': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'aws_root_disk_type',
                'aws_image_type',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'zookeeper': {
            'description': 'Zookeeper tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'aws_image_type': {
                    'type': 'string',
                },
                'aws_root_disk_type': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'aws_image_type',
                'aws_root_disk_type',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'mongod': {
            'description': 'MongoDB mongod tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'mongos': {
            'description': 'MongoDB mongos tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'compute_image_type',
                'sg_service_rules',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'mongocfg': {
            'description': 'MongoDB mongocfg tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'mongoinfra': {
            'description': 'MongoDB mongoinfra tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'ssh': {
            'description': 'SSH client provider config',
            'type': 'object',
            'properties': {
                'private_key': {
                    'type': 'string',
                },
                'public_key': {
                    'type': 'string',
                },
                'key_user': {
                    'type': 'string',
                },
            },
            'required': [
                'public_key',
                'private_key',
                'key_user',
            ],
            'additionalProperties': False,
        },
        'redis': {
            'description': 'Redis tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_image_type_template': {
                    "$ref": "#/definitions/image_template",
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'dbm_bootstrap_cmd_template': {
                    "$ref": "#/definitions/image_template",
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_image_type_template',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'dbm_bootstrap_cmd_template',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'mysql': {
            'description': 'MySQL tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
                'termination_grace_period': {
                    'type': 'integer',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
                'termination_grace_period',
            ],
            'additionalProperties': False,
        },
        'sqlserver': {
            'description': 'SQLServer tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_image_type_template': {
                    "$ref": "#/definitions/image_template",
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_image_type_template',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'windows_witness': {
            'description': 'Windows Witness tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_image_type_template': {
                    "$ref": "#/definitions/image_template",
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_image_type_template',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'greenplum_master': {
            'description': 'Greenplum master tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
                'use_superflow_v22': {
                    'type': 'boolean',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
                'use_superflow_v22',
            ],
            'additionalProperties': False,
        },
        'greenplum_segment': {
            'description': 'Greenplum segment tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
                'use_superflow_v22': {
                    'type': 'boolean',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
                'use_superflow_v22',
            ],
            'additionalProperties': False,
        },
        'kafka': {
            'description': 'Apache Kafka tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'aws_image_type': {
                    'type': 'string',
                },
                'aws_root_disk_type': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'aws_image_type',
                'aws_root_disk_type',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'elasticsearch_master': {
            'description': 'ElasticSearch master tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_image_type_template': {
                    "$ref": "#/definitions/image_template",
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'dbm_bootstrap_cmd_template': {
                    "$ref": "#/definitions/image_template",
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_image_type_template',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'dbm_bootstrap_cmd_template',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'elasticsearch_data': {
            'description': 'ElasticSearch datanode tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_image_type_template': {
                    "$ref": "#/definitions/image_template",
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'dbm_bootstrap_cmd_template': {
                    "$ref": "#/definitions/image_template",
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'eds_root': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_image_type_template',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'dbm_bootstrap_cmd_template',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'opensearch_master': {
            'description': 'OpenSearch master tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_image_type_template': {
                    "$ref": "#/definitions/image_template",
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'dbm_bootstrap_cmd_template': {
                    "$ref": "#/definitions/image_template",
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_root': {
                    'type': 'string',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_image_type_template',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'dbm_bootstrap_cmd_template',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'opensearch_data': {
            'description': 'OpenSearch datanode tasks config',
            'type': 'object',
            'properties': {
                'sg_service_rules': {
                    "$ref": "#/definitions/sg_service_rules",
                },
                'compute_image_type': {
                    'type': 'string',
                },
                'compute_image_type_template': {
                    "$ref": "#/definitions/image_template",
                },
                'compute_recreate_on_resize': {
                    'type': 'boolean',
                },
                'compute_root_disk_type_id': {
                    'type': 'string',
                },
                'create_host_pillar': {
                    'type': 'object',
                },
                'host_os': {
                    'type': 'string',
                },
                'dbm_data_path': {
                    'type': 'string',
                },
                'rootfs_space_limit': {
                    'type': 'integer',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'dbm_bootstrap_cmd_template': {
                    "$ref": "#/definitions/image_template",
                },
                'conductor_root_group': {
                    'type': 'string',
                },
                'eds_root': {
                    'type': 'string',
                },
                'conductor_alt_groups': {
                    '$ref': '#/definitions/conductor_alt_groups',
                },
                'eds_alt_roots': {
                    '$ref': '#/definitions/eds_alt_roots',
                },
                'juggler_checks': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'enable_oslogin': {
                    'type': 'boolean',
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'sg_service_rules',
                'compute_image_type',
                'compute_image_type_template',
                'compute_recreate_on_resize',
                'compute_root_disk_type_id',
                'create_host_pillar',
                'host_os',
                'dbm_data_path',
                'rootfs_space_limit',
                'dbm_bootstrap_cmd',
                'dbm_bootstrap_cmd_template',
                'conductor_root_group',
                'conductor_alt_groups',
                'eds_root',
                'eds_alt_roots',
                'juggler_checks',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'enable_oslogin',
                'labels',
            ],
            'additionalProperties': False,
        },
        'hadoop': {
            'description': 'Hadoop tasks config',
            'type': 'object',
            'properties': {
                'dbm_data_path': {
                    'type': 'string',
                },
                'dbm_bootstrap_cmd': {
                    'type': 'string',
                },
                'issue_tls': {
                    'type': 'boolean',
                },
                'create_gpg': {
                    'type': 'boolean',
                },
                'managed_zone': {
                    'type': 'string',
                },
                'public_zone': {
                    'type': 'string',
                },
                'private_zone': {
                    'type': 'string',
                },
                'running_operations_limit': {
                    'type': 'integer',
                    'minimum': 1,
                },
                'group_dns': {
                    'type': 'array',
                    'minItems': 0,
                    'uniqueItems': True,
                    'items': {
                        'type': 'object',
                        'properties': {
                            'id': {
                                'type': 'string',
                            },
                            'pattern': {
                                'type': 'string',
                            },
                            'disable_tls_altname': {
                                'type': 'boolean',
                            },
                        },
                        'required': [
                            'id',
                            'pattern',
                        ],
                        'additionalProperties': False,
                    },
                },
                'labels': {
                    'type': 'object',
                },
            },
            'required': [
                'dbm_data_path',
                'dbm_bootstrap_cmd',
                'issue_tls',
                'create_gpg',
                'managed_zone',
                'public_zone',
                'private_zone',
                'group_dns',
                'running_operations_limit',
                'labels',
            ],
            'additionalProperties': False,
        },
        'compute': {
            'description': 'Compute provider config',
            'type': 'object',
            'properties': {
                'managed_network_id': {
                    'type': 'string',
                },
                'managed_dns_zone_id': {
                    'type': 'string',
                },
                'managed_dns_ttl': {
                    'type': 'integer',
                },
                'managed_dns_ptr': {
                    'type': 'boolean',
                },
                'user_dns_zone_id': {
                    'type': 'string',
                },
                'user_dns_ttl': {
                    'type': 'integer',
                },
                'user_dns_ptr': {
                    'type': 'boolean',
                },
                'ca_path': {
                    'type': 'string',
                },
                'url': {
                    'type': 'string',
                },
                'service_account_id': {
                    'type': 'string',
                },
                'key_id': {
                    'type': 'string',
                },
                'private_key': {
                    'type': 'string',
                },
                'folder_id': {
                    'type': 'string',
                },
                'geo_map': {
                    'type': 'object',
                },
                'use_security_group': {
                    'type': 'boolean',
                },
                'placement_group': {
                    'type': 'object',
                    'properties': {
                        'best_effort': {
                            'type': 'boolean',
                        },
                    },
                    'required': [
                        'best_effort',
                    ],
                    'additionalProperties': False,
                },
            },
            'required': [
                'managed_network_id',
                'managed_dns_zone_id',
                'managed_dns_ttl',
                'managed_dns_ptr',
                'user_dns_zone_id',
                'user_dns_ttl',
                'user_dns_ptr',
                'ca_path',
                'url',
                'folder_id',
                'geo_map',
                'service_account_id',
                'key_id',
                'private_key',
                'use_security_group',
                'placement_group',
            ],
            'additionalProperties': False,
        },
        'vpc': {
            'description': 'VPC provider config',
            'type': 'object',
            'properties': {
                'ca_path': {
                    'type': 'string',
                },
                'url': {
                    'type': 'string',
                },
                'token': {
                    'oneOf': [
                        {
                            'type': 'null',
                        },
                        {
                            'type': 'string',
                        },
                    ],
                },
                'service_account_id': {},
                'key_id': {},
                'private_key': {},
            },
            'required': [
                'ca_path',
                'url',
                'token',
                'service_account_id',
                'key_id',
                'private_key',
            ],
            'additionalProperties': False,
            'anyOf': [
                {
                    'properties': {
                        'service_account_id': {
                            'type': 'string',
                        },
                        'key_id': {
                            'type': 'string',
                        },
                        'private_key': {
                            'type': 'string',
                        },
                    }
                },
                {
                    'properties': {
                        'service_account_id': {
                            'type': 'null',
                        },
                        'key_id': {
                            'type': 'null',
                        },
                        'private_key': {
                            'type': 'null',
                        },
                    }
                },
            ],
        },
        'user_compute': {
            'description': 'Compute provider for users folders config',
            'type': 'object',
            'properties': {
                'service_account_id': {},
                'key_id': {},
                'private_key': {},
            },
            'required': [
                'service_account_id',
                'key_id',
                'private_key',
            ],
            'additionalProperties': False,
            'anyOf': [
                {
                    'properties': {
                        'service_account_id': {
                            'type': 'string',
                        },
                        'key_id': {
                            'type': 'string',
                        },
                        'private_key': {
                            'type': 'string',
                        },
                    }
                },
                {
                    'properties': {
                        'service_account_id': {
                            'type': 'null',
                        },
                        'key_id': {
                            'type': 'null',
                        },
                        'private_key': {
                            'type': 'null',
                        },
                    }
                },
            ],
        },
        'aws': {
            'description': 'AWS providers configs',
            'type': 'object',
            'properties': {
                'region_name': {
                    'type': 'string',
                },
                'access_key_id': {
                    'type': 'string',
                },
                'secret_access_key': {
                    'type': 'string',
                },
                'labels': {
                    'type': 'object',
                    'additionalProperties': {
                        'type': 'string',
                    },
                },
                'dataplane_role_arn': {
                    'type': 'string',
                },
                'dataplane_account_id': {
                    'type': 'string',
                },
            },
            'required': [
                'region_name',
                'access_key_id',
                'secret_access_key',
                'labels',
                'dataplane_role_arn',
                'dataplane_account_id',
            ],
            'additionalProperties': False,
        },
        'ec2': {
            'description': 'EC2 provider config',
            'type': 'object',
            'properties': {
                'enabled': {
                    'type': 'boolean',
                },
                'images_prefix': {'type': 'string'},
                'supported_disk_types': {
                    'type': 'array',
                    'items': {
                        'type': 'string',
                    },
                },
                'instance_profile_arn': {
                    'type': 'string',
                },
                'retries': {
                    '$ref': '#/definitions/boto3_retries',
                },
            },
            'required': [
                'enabled',
                'images_prefix',
                'supported_disk_types',
                'instance_profile_arn',
                'retries',
            ],
            'additionalProperties': False,
        },
        'mdb_vpc': {
            'description': 'mdb-mdb_vpc provider config',
            'type': 'object',
            'properties': {
                'enabled': {
                    'type': 'boolean',
                },
                'url': {
                    'type': 'string',
                },
                'cert_file': {
                    'type': 'string',
                },
                'server_name': {
                    'type': 'string',
                },
                'insecure': {
                    'type': 'boolean',
                },
            },
            'required': [
                'enabled',
                'url',
                'cert_file',
                'server_name',
                'insecure',
            ],
            'additionalProperties': False,
        },
        'route53': {
            'description': 'Route53 provider config',
            'type': 'object',
            'properties': {
                'enabled': {
                    'type': 'boolean',
                },
                'public_hosted_zone_id': {
                    'type': 'string',
                },
                'ttl': {
                    'type': 'integer',
                },
                'retries': {
                    '$ref': '#/definitions/boto3_retries',
                },
            },
            'required': [
                'enabled',
                'public_hosted_zone_id',
                'ttl',
                'retries',
            ],
            'additionalProperties': False,
        },
        'kms': {
            'description': 'KMS provider config',
            'type': 'object',
            'properties': {
                'enabled': {
                    'type': 'boolean',
                },
                'retries': {
                    '$ref': '#/definitions/boto3_retries',
                },
            },
            'required': [
                'enabled',
                'retries',
            ],
            'additionalProperties': False,
        },
        'aws_iam': {
            'description': 'AWS IAM provider config',
            'type': 'object',
            'properties': {
                'enabled': {
                    'type': 'boolean',
                },
                'retries': {
                    '$ref': '#/definitions/boto3_retries',
                },
                'managed_dataplane_policy_arns': {
                    'type': 'array',
                    'items': {
                        'type': 'string',
                    },
                },
            },
            'required': [
                'enabled',
                'retries',
                'managed_dataplane_policy_arns',
            ],
            'additionalProperties': False,
        },
        'aws_s3': {
            'description': 'ASW S3 provider config',
            'type': 'object',
            'properties': {
                'enabled': {
                    'type': 'boolean',
                },
                'retries': {
                    '$ref': '#/definitions/boto3_retries',
                },
            },
            'required': [
                'enabled',
                'retries',
            ],
            'additionalProperties': False,
        },
        'cert_api': {
            'description': 'Certificate API provider config',
            'type': 'object',
            'properties': {
                'api': {
                    'enum': ['CERTIFICATOR', 'MDB_SECRETS', 'NOOP'],
                },
                'url': {
                    'type': 'string',
                },
                'token': {
                    'type': 'string',
                },
                'ca_name': {
                    'type': 'string',
                },
                'cert_type': {
                    'type': 'string',
                },
                'ca_path': {
                    'type': 'string',
                },
                'wildcard_cert': {
                    'type': 'boolean',
                },
            },
            'required': [
                'api',
                'url',
                'token',
                'ca_name',
                'cert_type',
                'ca_path',
                'wildcard_cert',
            ],
            'additionalProperties': False,
        },
        'dbm': {
            'description': 'DBM provider config',
            'type': 'object',
            'properties': {
                'managing_project_id': {
                    'type': 'string',
                },
                'project_id': {
                    'type': 'string',
                },
                'url': {
                    'type': 'string',
                },
                'project': {
                    'type': 'string',
                },
                'token': {
                    'type': 'string',
                },
            },
            'required': [
                'managing_project_id',
                'project_id',
                'url',
                'project',
                'token',
            ],
            'additionalProperties': False,
        },
        'yc_dns': {
            'description': 'Yandex Cloud DNS provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'ca_path': {
                    'type': 'string',
                },
                'zones': {
                    'type': 'object',
                    'additionalProperties': {'type': 'string'},
                },
                'ignored_zones': {
                    'type': 'array',
                    'items': {
                        'type': 'string',
                    },
                },
                'ttl': {
                    'type': 'integer',
                    'minimum': 30,
                },
                'record_types': {
                    'type': 'array',
                    'items': {
                        'type': 'string',
                    },
                    'minItems': 1,
                },
                'operation_wait_timeout': {
                    'type': 'integer',
                },
                'operation_wait_step': {
                    'type': 'number',
                },
            },
            'required': [
                'url',
                'ca_path',
                'zones',
                'ignored_zones',
                'ttl',
                'record_types',
                'operation_wait_timeout',
                'operation_wait_step',
            ],
            'additionalProperties': False,
        },
        'dns': {
            'description': 'Meta DNS provider config',
            'type': 'object',
            'properties': {
                'api': {
                    'enum': ['SLAYER', 'YC.DNS', 'NOOP'],
                },
            },
            'required': [
                'api',
            ],
            'additionalProperties': False,
        },
        'slayer_dns': {
            'description': 'Slayer DNS provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'account': {
                    'type': 'string',
                },
                'token': {
                    'type': 'string',
                },
                'ca_path': {
                    'type': 'string',
                },
                'ttl': {
                    'type': 'integer',
                    'minimum': 30,
                },
                'name_servers': {
                    'type': 'array',
                    'items': {
                        'type': 'string',
                    },
                    'minItems': 1,
                },
            },
            'required': [
                'url',
                'account',
                'token',
                'ca_path',
                'ttl',
                'name_servers',
            ],
            'additionalProperties': False,
        },
        'conductor': {
            'description': 'Conductor provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'token': {
                    'type': 'string',
                },
                'geo_map': {
                    'type': 'object',
                },
                'enabled': {
                    'type': 'boolean',
                },
            },
            'required': [
                'url',
                'token',
                'geo_map',
                'enabled',
            ],
            'additionalProperties': False,
        },
        'eds': {
            'description': 'Eds provider config',
            'type': 'object',
            'properties': {
                'enabled': {
                    'type': 'boolean',
                },
                'zone_id': {
                    'type': 'string',
                },
                'eds_endpoint': {
                    'type': 'string',
                },
                'dns_endpoint': {
                    'type': 'string',
                },
                'dns_cname_ttl': {
                    'type': 'integer',
                },
                'cert_file': {
                    'type': 'string',
                },
                'dns_api_timeout': {
                    'type': 'integer',
                },
            },
            'required': [
                'enabled',
                'zone_id',
                'eds_endpoint',
                'dns_endpoint',
                'dns_api_timeout',
                'dns_cname_ttl',
                'cert_file',
            ],
            'additionalProperties': False,
        },
        'juggler': {
            'description': 'Juggler provider config',
            'type': 'object',
            'properties': {
                'enabled': {
                    'type': 'boolean',
                },
                'url': {
                    'type': 'string',
                },
                'token': {
                    'type': 'string',
                },
                'downtime_hours': {
                    'type': 'integer',
                    'minimum': 1,
                    'maximum': 4320,
                },
            },
            'required': [
                'enabled',
                'url',
                'token',
                'downtime_hours',
            ],
            'additionalProperties': False,
        },
        'deploy': {
            'description': 'Deploy provider config',
            'type': 'object',
            'properties': {
                'version': {
                    'description': 'Deploy version to use for new hosts',
                    'type': 'integer',
                },
                'group': {
                    'description': 'Default deploy group for new V2 minions',
                    'type': 'string',
                },
                'regional_mapping': {
                    'type': 'object',
                    'additionalProperties': {'type': 'string'},
                },
                'url_v2': {
                    'type': 'string',
                },
                'token_v2': {
                    'type': 'string',
                },
                'dataplane_host_v2': {
                    'description': 'Passed to dataplane nodes via instance metadata',
                    'type': 'string',
                },
                'attempts': {
                    'type': 'integer',
                    'minimum': 1,
                    'maximum': 10,
                },
                'timeout': {
                    'type': 'integer',
                    'minumum': 600,
                    'maximum': 86400,
                },
                'ca_path': {
                    'type': 'string',
                },
            },
            'required': [
                'url_v2',
                'version',
                'group',
                'regional_mapping',
                'token_v2',
                'attempts',
                'timeout',
                'dataplane_host_v2',
                'ca_path',
            ],
            'additionalProperties': False,
        },
        's3': {
            'description': 'S3-related providers config',
            'type': 'object',
            'properties': {
                'backup': {
                    'type': 'object',
                    'properties': {
                        'access_key_id': {
                            'type': 'string',
                        },
                        'secret_access_key': {
                            'type': 'string',
                        },
                        'endpoint_url': {
                            'type': 'string',
                        },
                        'location_constraint': {
                            'type': 'string',
                        },
                        'allow_lifecycle_policies': {
                            'type': 'boolean',
                        },
                    },
                    'required': [
                        'access_key_id',
                        'secret_access_key',
                        'endpoint_url',
                        'location_constraint',
                        'allow_lifecycle_policies',
                    ],
                    'additionalProperties': False,
                },
                'cloud_storage': {
                    'type': 'object',
                    'properties': {
                        'access_key_id': {
                            'type': 'string',
                        },
                        'secret_access_key': {
                            'type': 'string',
                        },
                        'endpoint_url': {
                            'type': 'string',
                        },
                        'location_constraint': {
                            'type': 'string',
                        },
                        'allow_lifecycle_policies': {
                            'type': 'boolean',
                        },
                    },
                    'required': [
                        'access_key_id',
                        'secret_access_key',
                        'endpoint_url',
                        'location_constraint',
                        'allow_lifecycle_policies',
                    ],
                    'additionalProperties': False,
                },
                'secure_backups': {
                    'type': 'object',
                    'properties': {
                        'access_key_id': {
                            'type': 'string',
                        },
                        'secret_access_key': {
                            'type': 'string',
                        },
                        'endpoint_url': {
                            'type': 'string',
                        },
                        'location_constraint': {
                            'type': 'string',
                        },
                        'secured': {
                            'type': 'boolean',
                        },
                        'override_default': {
                            'type': 'boolean',
                        },
                        'folder_id': {
                            'type': 'string',
                        },
                        'iam': {
                            'type': 'object',
                            'properties': {
                                'service_account_id': {
                                    'type': 'string',
                                },
                                'key_id': {
                                    'type': 'string',
                                },
                                'private_key': {
                                    'type': 'string',
                                },
                            },
                            'required': [
                                'service_account_id',
                                'key_id',
                                'private_key',
                            ],
                            'additionalProperties': False,
                        },
                    },
                    'required': [
                        'access_key_id',
                        'secret_access_key',
                        'endpoint_url',
                        'location_constraint',
                        'secured',
                        'override_default',
                        'folder_id',
                        'iam',
                    ],
                    'additionalProperties': False,
                },
                'access_key_id': {
                    'type': 'string',
                },
                'secret_access_key': {
                    'type': 'string',
                },
                'endpoint_url': {
                    'type': 'string',
                },
                'disable_ssl_warnings': {
                    'type': 'boolean',
                },
                'addressing_style': {
                    'type': 'string',
                },
                'region_name': {
                    'type': 'string',
                },
                'ca_path': {
                    'type': 'string',
                },
                'idm_endpoint_url': {
                    'type': 'string',
                },
                'dataplane_role_arn': {'type': 'string'},
            },
            'required': [
                'backup',
                'cloud_storage',
                'secure_backups',
                'access_key_id',
                'secret_access_key',
                'endpoint_url',
                'disable_ssl_warnings',
                'addressing_style',
                'region_name',
                'ca_path',
                'idm_endpoint_url',
                'dataplane_role_arn',
            ],
            'additionalProperties': False,
        },
        'internal_api': {
            'description': 'MDB Internal API provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'ca_path': {
                    'type': 'string',
                },
                'access_id': {
                    'type': 'string',
                },
                'access_secret': {
                    'type': 'string',
                },
            },
            'required': [
                'url',
                'ca_path',
                'access_id',
                'access_secret',
            ],
            'additionalProperties': False,
        },
        'iam_token_service': {
            'description': 'IAM token service provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'expire_thresh': {
                    'type': 'integer',
                },
            },
            'required': [
                'url',
                'expire_thresh',
            ],
            'additionalProperties': False,
        },
        'iam_access_service': {
            'description': 'IAM service provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
            },
            'required': [
                'url',
            ],
            'additionalProperties': False,
        },
        'iam_service': {
            'description': 'IAM service provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
            },
            'required': [
                'url',
            ],
            'additionalProperties': False,
        },
        'iam_jwt': {
            'description': 'IAM JWT provider config',
            'type': 'object',
            'properties': {
                'audience': {
                    'type': 'string',
                },
                'url': {
                    'type': 'string',
                },
                'cert_file': {
                    'type': 'string',
                },
                'expire_thresh': {
                    'type': 'integer',
                },
                'request_expire': {
                    'type': 'integer',
                },
                'server_name': {
                    'type': 'string',
                },
                'insecure': {
                    'type': 'boolean',
                },
                'service_account_id': {
                    'type': 'string',
                },
                'key_id': {
                    'type': 'string',
                },
                'private_key': {
                    'type': 'string',
                },
            },
            'required': [
                'audience',
                'url',
                'cert_file',
                'expire_thresh',
                'request_expire',
                'server_name',
                'insecure',
                'service_account_id',
                'key_id',
                'private_key',
            ],
            'additionalProperties': False,
        },
        'solomon': {
            'description': 'Solomon provider config',
            'type': 'object',
            'properties': {
                'service_provider_id': {
                    'type': 'string',
                },
                'token': {
                    'type': 'string',
                },
                'url': {
                    'type': 'string',
                },
                'ca_path': {
                    'type': 'string',
                },
                'project': {
                    'type': 'string',
                },
                'service': {
                    'type': 'string',
                },
                'ident_template': {
                    'type': 'string',
                },
                'service_label': {
                    'type': 'string',
                },
                'enabled': {
                    'type': 'boolean',
                },
            },
            'required': [
                'service_provider_id',
                'token',
                'url',
                'ca_path',
                'project',
                'service',
                'ident_template',
                'service_label',
                'enabled',
            ],
            'additionalProperties': False,
        },
        'resource_manager': {
            'description': 'Resource manager provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'ca_path': {
                    'type': 'string',
                },
            },
            'required': ['url', 'ca_path'],
            'additionalProperties': False,
        },
        'dataproc_manager': {
            'description': 'Dataproc manager provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'cert_file': {
                    'type': 'string',
                },
                'grpc_timeout': {
                    'type': 'integer',
                },
                'server_name': {
                    'type': 'string',
                },
                'insecure': {
                    'type': 'boolean',
                },
                'sleep_time': {
                    'type': 'integer',
                },
            },
            'required': [
                'url',
                'cert_file',
                'grpc_timeout',
                'server_name',
                'insecure',
                'sleep_time',
            ],
            'additionalProperties': False,
        },
        'instance_group_service': {
            'description': 'Instance group service provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'grpc_timeout': {
                    'type': 'integer',
                },
            },
            'required': [
                'url',
                'grpc_timeout',
            ],
            'additionalProperties': False,
        },
        'managed_kubernetes_service': {
            'description': 'Managed kubernetes service provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'grpc_timeout': {
                    'type': 'integer',
                },
            },
            'required': [
                'url',
                'grpc_timeout',
            ],
            'additionalProperties': False,
        },
        'managed_postgresql_service': {
            'description': 'Managed postgresql service provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'grpc_timeout': {
                    'type': 'integer',
                },
            },
            'required': [
                'url',
                'grpc_timeout',
            ],
            'additionalProperties': False,
        },
        'loadbalancer_service': {
            'description': 'Load balancer service provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'grpc_timeout': {
                    'type': 'integer',
                },
            },
            'required': [
                'url',
                'grpc_timeout',
            ],
            'additionalProperties': False,
        },
        'lockbox_service': {
            'description': 'Lockbox service provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'grpc_timeout': {
                    'type': 'integer',
                },
            },
            'required': [
                'url',
                'grpc_timeout',
            ],
            'additionalProperties': False,
        },
        'mlock': {
            'description': 'Mlock provider config',
            'type': 'object',
            'properties': {
                'url': {
                    'type': 'string',
                },
                'cert_file': {
                    'type': 'string',
                },
                'token': {
                    'oneOf': [
                        {
                            'type': 'null',
                        },
                        {
                            'type': 'string',
                        },
                    ],
                },
                'timeout': {
                    'type': 'integer',
                },
                'keepalive_time_ms': {
                    'type': 'integer',
                },
                'server_name': {
                    'type': 'string',
                },
                'enabled': {
                    'type': 'boolean',
                },
                'insecure': {
                    'type': 'boolean',
                },
                'service_account_id': {},
                'key_id': {},
                'private_key': {},
            },
            'required': [
                'enabled',
                'url',
                'token',
                'timeout',
                'server_name',
                'cert_file',
                'keepalive_time_ms',
                'insecure',
                'service_account_id',
                'key_id',
                'private_key',
            ],
            'anyOf': [
                {
                    'properties': {
                        'service_account_id': {
                            'type': 'string',
                        },
                        'key_id': {
                            'type': 'string',
                        },
                        'private_key': {
                            'type': 'string',
                        },
                    }
                },
                {
                    'properties': {
                        'service_account_id': {
                            'type': 'null',
                        },
                        'key_id': {
                            'type': 'null',
                        },
                        'private_key': {
                            'type': 'null',
                        },
                    }
                },
            ],
            'additionalProperties': False,
        },
        'cloud_storage': {
            'description': 'Cloud storage provider config',
            'type': 'object',
            'properties': {
                'folder_id': {
                    'type': 'string',
                },
                'service_account_id': {
                    'type': 'string',
                },
                'key_id': {
                    'type': 'string',
                },
                'private_key': {
                    'type': 'string',
                },
            },
            'required': [
                'folder_id',
                'service_account_id',
                'key_id',
                'private_key',
            ],
            'additionalProperties': False,
        },
        'per_cluster_service_accounts': {
            'description': 'Configuration for per-cluster service accounts',
            'type': 'object',
            'properties': {
                'folder_id': {
                    'type': 'string',
                },
            },
            'required': [
                'folder_id',
            ],
            'additionalProperties': False,
        },
        'billingdb': {
            'description': 'Configuration for billingdb connection',
            'type': 'object',
            'properties': {
                'enabled': {
                    'type': 'boolean',
                },
                'billingdb_dsn': {
                    'type': 'string',
                },
                'billingdb_hosts': {
                    'type': 'array',
                    'minItems': 1,
                    'uniqueItems': True,
                    'items': {
                        'type': 'string',
                    },
                },
            },
            'required': ['enabled', 'billingdb_dsn', 'billingdb_hosts'],
            'additionalProperties': False,
        },
    },
    'required': [
        'environment_name',
        'main',
        'restart',
        'compute',
        'vpc',
        'user_compute',
        'aws',
        'ec2',
        'mdb_vpc',
        'route53',
        'kms',
        'aws_iam',
        'aws_s3',
        'cert_api',
        'dbm',
        'dns',
        'yc_dns',
        'slayer_dns',
        'conductor',
        'eds',
        'juggler',
        'deploy',
        'ssh',
        'resource_manager',
        's3',
        'internal_api',
        'iam_token_service',
        'iam_access_service',
        'iam_service',
        'iam_jwt',
        'solomon',
        'postgresql',
        'clickhouse',
        'zookeeper',
        'mongod',
        'mongos',
        'mongocfg',
        'mongoinfra',
        'redis',
        'mysql',
        'sqlserver',
        'windows_witness',
        'greenplum_master',
        'greenplum_segment',
        'kafka',
        'elasticsearch_data',
        'elasticsearch_master',
        'opensearch_data',
        'opensearch_master',
        'hadoop',
        'dataproc_manager',
        'instance_group_service',
        'loadbalancer_service',
        'lockbox_service',
        'managed_kubernetes_service',
        'managed_postgresql_service',
        'mlock',
        'cloud_storage',
        'per_cluster_service_accounts',
        'billingdb',
    ],
    'additionalProperties': False,
}

DEFAULT_CONFIG = {
    'main': {
        'log_level': 'INFO',
        'max_tasks': 10,
        'acquire_fail_limit': 10,
        'poll_interval': 1,
        'sentry_dsn': None,
        'sentry_environment': 'test',
        'managed_zone': 'db.yandex.net',
        'metadb_dsn': '',
        'public_zone': '',
        'private_zone': '',
        # secrets cleanup is disabled in infratests to avoid having a ssh connection
        # from infratest controlplane network to dataplane external interface network
        'tracing': {
            'service_name': 'dbaas-worker',
            'disabled': False,
            'use_env_vars': False,
            'local_agent': {
                'reporting_host': 'localhost',
                'reporting_port': '6831',
            },
            'queue_size': 10000,
            'sampler': {
                'type': 'const',
                'param': 1,
            },
            'logging': False,
        },
    },
    'restart': {
        'base': 1800,
        'max_attempts': 3,
    },
    'conductor': {
        'geo_map': {
            'ru-central1-a': 'vla',
            'ru-central1-b': 'sas',
            'ru-central1-c': 'myt',
        },
        'enabled': True,
    },
    'eds': {
        'enabled': False,
        'zone_id': '',
        'eds_endpoint': '',
        'dns_endpoint': '',
        'cert_file': '/config/CA.pem',
        'dns_api_timeout': 15,
        'dns_cname_ttl': 300,
    },
    'postgresql': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 5432,
                'ports_to': 5432,
            },
            {
                'direction': 'BOTH',
                'ports_from': 6432,
                'ports_to': 6432,
            },
        ],
        'compute_image_type': 'postgresql',
        'compute_recreate_on_resize': True,
        'compute_root_disk_type_id': 'network-hdd',
        'compute_image_type_template': {},
        'create_host_pillar': {
            'force-pooler-start': True,
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/postgresql',
        'rootfs_space_limit': 10737418240,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_pg_bionic.sh',
        'dbm_bootstrap_cmd_template': {},
        'conductor_root_group': 'mdb_other_prod',
        'conductor_alt_groups': [],
        'eds_root': 'postgresql',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'pg_ping',
            'pg_replication_alive',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [
            {
                'id': 'cid',
                'pattern': 'c-{id}.rw.db.yandex.net',
            },
            {
                'id': 'cid',
                'pattern': 'c-{id}.ro.db.yandex.net',
            },
        ],
        'enable_oslogin': False,
        'labels': {},
    },
    'clickhouse': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 2181,
                'ports_to': 2181,
            },
            {
                'direction': 'BOTH',
                'ports_from': 2281,
                'ports_to': 2281,
            },
            {
                'direction': 'BOTH',
                'ports_from': 2888,
                'ports_to': 2888,
            },
            {
                'direction': 'BOTH',
                'ports_from': 3888,
                'ports_to': 3888,
            },
            {
                'direction': 'BOTH',
                'ports_from': 8443,
                'ports_to': 8443,
            },
            {
                'direction': 'BOTH',
                'ports_from': 9009,
                'ports_to': 9010,
            },
            {
                'direction': 'BOTH',
                'ports_from': 9440,
                'ports_to': 9440,
            },
        ],
        'compute_image_type': 'clickhouse',
        'compute_recreate_on_resize': False,
        'compute_root_disk_type_id': 'network-hdd',
        'aws_root_disk_type': 'gp2',
        'aws_image_type': 'clickhouse',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/clickhouse',
        'rootfs_space_limit': 21474836480,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_ch_bionic.sh',
        'conductor_root_group': 'mdb_porto_clickhouse',
        'conductor_alt_groups': [],
        'eds_root': 'clickhouse',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'ch_ping',
        ],
        'issue_tls': True,
        'create_gpg': False,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [
            {
                'id': 'cid',
                'pattern': 'c-{id}.rw.db.yandex.net',
            },
            {
                'id': 'shard_name',
                'pattern': '{id}.c-{cid}.rw.db.yandex.net',
            },
        ],
        'enable_oslogin': True,
        'labels': {},
    },
    'zookeeper': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 2181,
                'ports_to': 2181,
            },
            {
                'direction': 'BOTH',
                'ports_from': 2281,
                'ports_to': 2281,
            },
            {
                'direction': 'BOTH',
                'ports_from': 2888,
                'ports_to': 2888,
            },
            {
                'direction': 'BOTH',
                'ports_from': 3888,
                'ports_to': 3888,
            },
        ],
        'compute_image_type': 'zookeeper',
        'compute_recreate_on_resize': True,
        'compute_root_disk_type_id': 'network-hdd',
        'aws_image_type': 'zookeeper',
        'aws_root_disk_type': 'gp2',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/zookeeper',
        'rootfs_space_limit': 10737418240,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_zk_bionic.sh',
        'conductor_root_group': 'mdb_porto_zookeeper',
        'conductor_alt_groups': [],
        'eds_root': 'zookeeper',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'zk_alive',
        ],
        'issue_tls': True,
        'create_gpg': False,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [],
        'enable_oslogin': True,
        'labels': {},
    },
    'mongod': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 27017,
                'ports_to': 27019,
            },
        ],
        'compute_image_type': 'mongodb',
        'compute_recreate_on_resize': True,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/mongodb',
        'rootfs_space_limit': 21474836480,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_mongodb_bionic.sh',
        'conductor_root_group': 'mdb_porto_mongod',
        'conductor_alt_groups': [],
        'eds_root': 'mongod',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'mongodb_ping',
            'mongodb_primary_exists',
            'mongodb_locked_queue',
            'mongodb_replication_lag',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [],
        'enable_oslogin': True,
        'labels': {},
    },
    'mongos': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 27017,
                'ports_to': 27019,
            },
        ],
        'compute_image_type': 'mongodb',
        'compute_recreate_on_resize': True,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {},
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/mongodb',
        'rootfs_space_limit': 21474836480,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_mongodb_bionic.sh',
        'conductor_root_group': 'mdb_porto_mongos',
        'conductor_alt_groups': [],
        'eds_root': 'mongos',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'mongos_ping',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [],
        'enable_oslogin': True,
        'labels': {},
    },
    'mongocfg': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 27017,
                'ports_to': 27019,
            },
        ],
        'compute_image_type': 'mongodb',
        'compute_recreate_on_resize': True,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/mongodb',
        'rootfs_space_limit': 21474836480,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_mongodb_bionic.sh',
        'conductor_root_group': 'mdb_porto_mongocfg',
        'conductor_alt_groups': [],
        'eds_root': 'mongocfg',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'mongodb_ping',
            'mongodb_primary_exists',
            'mongodb_locked_queue',
            'mongodb_replication_lag',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [],
        'enable_oslogin': True,
        'labels': {},
    },
    'mongoinfra': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 27017,
                'ports_to': 27019,
            },
        ],
        'compute_image_type': 'mongodb',
        'compute_recreate_on_resize': True,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/mongodb',
        'rootfs_space_limit': 21474836480,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_mongodb_bionic.sh',
        'conductor_root_group': 'mdb_porto_mongoinfra',
        'conductor_alt_groups': [],
        'eds_root': 'mongoinfra',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'mongodb_ping',
            'mongodb_primary_exists',
            'mongodb_locked_queue',
            'mongodb_replication_lag',
            'mongos_ping',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [],
        'enable_oslogin': True,
        'labels': {},
    },
    'redis': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 6379,
                'ports_to': 6379,
            },
            {
                'direction': 'BOTH',
                'ports_from': 16379,
                'ports_to': 16379,
            },
            {
                'direction': 'BOTH',
                'ports_from': 26379,
                'ports_to': 26379,
            },
            {
                'direction': 'BOTH',
                'ports_from': 6380,
                'ports_to': 6380,
            },
            {
                'direction': 'BOTH',
                'ports_from': 16380,
                'ports_to': 16380,
            },
            {
                'direction': 'BOTH',
                'ports_from': 26380,
                'ports_to': 26380,
            },
        ],
        'compute_image_type': 'redis',
        'compute_recreate_on_resize': True,
        'compute_root_disk_type_id': 'network-hdd',
        'compute_image_type_template': {},
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/redis',
        'rootfs_space_limit': 10737418240,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_redis_bionic.sh',
        'dbm_bootstrap_cmd_template': {},
        'conductor_root_group': 'mdb_porto_redis',
        'conductor_alt_groups': [],
        'eds_root': 'redis',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'redis_alive',
            'redis_lag',
            'redis_master',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [
            {
                'id': 'cid',
                'pattern': 'c-{id}.rw.db.yandex.net',
            },
        ],
        'enable_oslogin': True,
        'labels': {},
    },
    'mysql': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 22,
                'ports_to': 22,
            },
            {
                'direction': 'BOTH',
                'ports_from': 3306,
                'ports_to': 3307,
            },
        ],
        'compute_image_type': 'mysql',
        'compute_recreate_on_resize': True,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/mysql',
        'rootfs_space_limit': 10737418240,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_mysql.sh',
        'conductor_root_group': 'mdb_porto_mysql',
        'conductor_alt_groups': [],
        'eds_root': 'mysql',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'mysql_ping',
            'mysql_slave_running',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [
            {
                'id': 'cid',
                'pattern': 'c-{id}.rw.db.yandex.net',
            },
            {
                'id': 'cid',
                'pattern': 'c-{id}.ro.db.yandex.net',
            },
        ],
        'enable_oslogin': False,
        'labels': {},
        'termination_grace_period': 510,  # 8 minutes for DB shutdown + 30 seconds for flush & unmount
    },
    'sqlserver': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 22,
                'ports_to': 22,
            },
            {
                'direction': 'BOTH',
                'ports_from': 53,
                'ports_to': 53,
            },
            {
                'direction': 'BOTH',
                'ports_from': 135,
                'ports_to': 140,
            },
            {
                'direction': 'BOTH',
                'ports_from': 443,
                'ports_to': 445,
            },
            {
                'direction': 'BOTH',
                'ports_from': 67,
                'ports_to': 68,
            },
            {
                'direction': 'BOTH',
                'ports_from': 343,
                'ports_to': 343,
            },
            {
                'direction': 'BOTH',
                'ports_from': 547,
                'ports_to': 547,
            },
            {
                'direction': 'BOTH',
                'ports_from': 4505,
                'ports_to': 4506,
            },
            {
                'direction': 'BOTH',
                'ports_from': 30000,
                'ports_to': 65535,
            },
            {
                'direction': 'BOTH',
                'ports_from': 8443,
                'ports_to': 8443,
            },
            {
                'direction': 'BOTH',
                'ports_from': 80,
                'ports_to': 80,
            },
            {
                'direction': 'BOTH',
                'ports_from': 5022,
                'ports_to': 5022,
            },
            {
                'direction': 'BOTH',
                'ports_from': 3343,
                'ports_to': 3343,
            },
            {
                'protocol': 'UDP',
                'direction': 'BOTH',
                'ports_from': 53,
                'ports_to': 53,
            },
            {
                'protocol': 'UDP',
                'direction': 'BOTH',
                'ports_from': 67,
                'ports_to': 68,
            },
            {
                'protocol': 'UDP',
                'direction': 'BOTH',
                'ports_from': 135,
                'ports_to': 135,
            },
            {
                'protocol': 'UDP',
                'direction': 'BOTH',
                'ports_from': 547,
                'ports_to': 547,
            },
            {
                'protocol': 'UDP',
                'direction': 'BOTH',
                'ports_from': 3343,
                'ports_to': 3343,
            },
        ],
        'compute_image_type': 'sqlserver',
        'compute_image_type_template': {},
        'compute_recreate_on_resize': True,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'windows',
        'conductor_root_group': 'mdb_compute_sqlserver',
        'conductor_alt_groups': [],
        'eds_root': 'sqlserver',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'sqlserver_ping',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [
            {
                'id': 'cid',
                'pattern': 'c-{id}.rw.db.yandex.net',
            },
            {
                'id': 'cid',
                'pattern': 'c-{id}.ro.db.yandex.net',
            },
        ],
        'enable_oslogin': False,
        'labels': {},
    },
    'windows_witness': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 22,
                'ports_to': 22,
            },
            {
                'direction': 'BOTH',
                'ports_from': 53,
                'ports_to': 53,
            },
            {
                'direction': 'BOTH',
                'ports_from': 135,
                'ports_to': 140,
            },
            {
                'direction': 'BOTH',
                'ports_from': 443,
                'ports_to': 445,
            },
            {
                'direction': 'BOTH',
                'ports_from': 67,
                'ports_to': 68,
            },
            {
                'direction': 'BOTH',
                'ports_from': 343,
                'ports_to': 343,
            },
            {
                'direction': 'BOTH',
                'ports_from': 547,
                'ports_to': 547,
            },
            {
                'direction': 'BOTH',
                'ports_from': 4505,
                'ports_to': 4506,
            },
            {
                'direction': 'BOTH',
                'ports_from': 30000,
                'ports_to': 65535,
            },
            {
                'direction': 'BOTH',
                'ports_from': 8443,
                'ports_to': 8443,
            },
            {
                'direction': 'BOTH',
                'ports_from': 80,
                'ports_to': 80,
            },
            {
                'direction': 'BOTH',
                'ports_from': 5022,
                'ports_to': 5022,
            },
            {
                'direction': 'BOTH',
                'ports_from': 3343,
                'ports_to': 3343,
            },
            {
                'protocol': 'UDP',
                'direction': 'BOTH',
                'ports_from': 53,
                'ports_to': 53,
            },
            {
                'protocol': 'UDP',
                'direction': 'BOTH',
                'ports_from': 67,
                'ports_to': 68,
            },
            {
                'protocol': 'UDP',
                'direction': 'BOTH',
                'ports_from': 135,
                'ports_to': 135,
            },
            {
                'protocol': 'UDP',
                'direction': 'BOTH',
                'ports_from': 547,
                'ports_to': 547,
            },
            {
                'protocol': 'UDP',
                'direction': 'BOTH',
                'ports_from': 3343,
                'ports_to': 3343,
            },
        ],
        'compute_image_type': 'windows-witness',
        'compute_image_type_template': {},
        'compute_recreate_on_resize': True,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'windows',
        'conductor_root_group': 'mdb_compute_sqlserver_witness',
        'conductor_alt_groups': [],
        'eds_root': 'sqlserver_witness',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
        ],
        'issue_tls': False,
        'create_gpg': False,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [],
        'enable_oslogin': False,
        'labels': {},
    },
    'greenplum_master': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 22,
                'ports_to': 22,
            },
            {
                'direction': 'BOTH',
                'ports_from': 5432,
                'ports_to': 5432,
            },
            {
                'direction': 'BOTH',
                'ports_from': 6432,
                'ports_to': 6432,
            },
            {
                'protocol': 'UDP',
                'direction': 'BOTH',
                'ports_from': 1025,
                'ports_to': 65535,
            },
            {
                'protocol': 'TCP',
                'direction': 'EGRESS',
                'ports_from': 0,
                'ports_to': 0,
            },
        ],
        'compute_image_type': 'greenplum',
        'compute_recreate_on_resize': True,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {
            'segment': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/greenplum',
        'rootfs_space_limit': 21474836480,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_gpdb_bionic.sh',
        'conductor_root_group': 'mdb_porto_greenplum_master',
        'conductor_alt_groups': [],
        'eds_root': 'greenplum_master',
        'eds_alt_roots': [],
        'juggler_checks': ['META', 'gp_ping', 'segments_up'],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [
            {
                'id': 'cid',
                'pattern': 'c-{id}.rw.db.yandex.net',
            },
            {
                'id': 'cid',
                'pattern': 'c-{id}.ro.db.yandex.net',
            },
        ],
        'enable_oslogin': True,
        'labels': {},
        'use_superflow_v22': True,
    },
    'greenplum_segment': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 22,
                'ports_to': 22,
            },
            {
                'direction': 'BOTH',
                'ports_from': 5432,
                'ports_to': 5432,
            },
            {
                'direction': 'BOTH',
                'ports_from': 6000,
                'ports_to': 6100,
            },
            {
                'direction': 'BOTH',
                'ports_from': 7000,
                'ports_to': 7100,
            },
            {
                'protocol': 'UDP',
                'direction': 'BOTH',
                'ports_from': 1025,
                'ports_to': 65535,
            },
            {
                'protocol': 'TCP',
                'direction': 'EGRESS',
                'ports_from': 0,
                'ports_to': 0,
            },
        ],
        'compute_image_type': 'greenplum',
        'compute_recreate_on_resize': True,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {
            'segment': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/greenplum',
        'rootfs_space_limit': 21474836480,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_gpdb_bionic.sh',
        'conductor_root_group': 'mdb_porto_greenplum_segment',
        'conductor_alt_groups': [],
        'eds_root': 'greenplum_segment',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [
            {
                'id': 'cid',
                'pattern': 'c-{id}.rw.db.yandex.net',
            },
            {
                'id': 'cid',
                'pattern': 'c-{id}.ro.db.yandex.net',
            },
        ],
        'enable_oslogin': True,
        'labels': {},
        'use_superflow_v22': True,
    },
    'kafka': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 443,
                'ports_to': 443,
            },
            {
                'direction': 'EGRESS',
                'ports_from': 443,
                'ports_to': 443,
                'protocol': 'TCP',
                'v4_cidr_blocks': ["213.180.193.243/32", "213.180.205.156/32", "93.158.157.254/32"],
                'v6_cidr_blocks': [
                    "2a02:6b8::1d9/128",
                    "2a02:6b8:0:3400:0:587:0:68/128",
                    "2a02:6b8:0:3400:0:bd4:0:4/128",
                ],
            },
            {
                'direction': 'BOTH',
                'ports_from': 8081,
                'ports_to': 8081,
            },
            {
                'direction': 'BOTH',
                'ports_from': 2181,
                'ports_to': 2181,
            },
            {
                'direction': 'BOTH',
                'ports_from': 2888,
                'ports_to': 2888,
            },
            {
                'direction': 'BOTH',
                'ports_from': 3888,
                'ports_to': 3888,
            },
            {
                'direction': 'BOTH',
                'ports_from': 9000,
                'ports_to': 9000,
            },
            {
                'direction': 'BOTH',
                'ports_from': 9091,
                'ports_to': 9092,
            },
        ],
        'compute_image_type': 'kafka',
        'compute_recreate_on_resize': False,
        'compute_root_disk_type_id': 'network-hdd',
        'aws_image_type': 'kafka',
        'aws_root_disk_type': 'gp2',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/kafka',
        'rootfs_space_limit': 10737418240,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_kafka_bionic.sh',
        'conductor_root_group': 'mdb_porto_kafka',
        'conductor_alt_groups': [],
        'eds_root': 'kafka',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'kafka_ping',
            'kafka_cluster_availability',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [
            {
                'id': 'cid',
                'pattern': 'c-{id}.db.yandex.net',
            }
        ],
        'enable_oslogin': False,
        'labels': {},
    },
    'elasticsearch_data': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 9200,
                'ports_to': 9201,
            },
            {
                'direction': 'BOTH',
                'ports_from': 9300,
                'ports_to': 9300,
            },
        ],
        'compute_image_type': 'elasticsearch',
        'compute_image_type_template': {},
        'compute_recreate_on_resize': False,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/elasticsearch',
        'rootfs_space_limit': 21474836480,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_elasticsearch_bionic.sh',
        'dbm_bootstrap_cmd_template': {},
        'conductor_root_group': 'mdb_porto_elasticsearch_data',
        'conductor_alt_groups': [],
        'eds_root': 'elasticsearch_data',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'es_ping',
            'es_cluster_status',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [
            {
                'id': 'cid',
                'pattern': 'c-{id}.rw.db.yandex.net',
            }
        ],
        'enable_oslogin': True,
        'labels': {},
    },
    'elasticsearch_master': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 9200,
                'ports_to': 9201,
            },
            {
                'direction': 'BOTH',
                'ports_from': 9300,
                'ports_to': 9300,
            },
        ],
        'compute_image_type': 'elasticsearch',
        'compute_image_type_template': {},
        'compute_recreate_on_resize': False,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/elasticsearch',
        'rootfs_space_limit': 21474836480,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_elasticsearch_bionic.sh',
        'dbm_bootstrap_cmd_template': {},
        'conductor_root_group': 'mdb_porto_elasticsearch_master',
        'conductor_alt_groups': [],
        'eds_root': 'elasticsearch_master',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'es_ping',
            'es_cluster_status',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [
            {
                'id': 'cid',
                'pattern': 'c-{id}.rw.db.yandex.net',
            }
        ],
        'enable_oslogin': True,
        'labels': {},
    },
    'opensearch_data': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 9200,
                'ports_to': 9201,
            },
            {
                'direction': 'BOTH',
                'ports_from': 9300,
                'ports_to': 9300,
            },
        ],
        'compute_image_type': 'opensearch',
        'compute_image_type_template': {},
        'compute_recreate_on_resize': False,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/opensearch',
        'rootfs_space_limit': 21474836480,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_opensearch_bionic.sh',
        'dbm_bootstrap_cmd_template': {},
        'conductor_root_group': 'mdb_porto_opensearch_data',
        'conductor_alt_groups': [],
        'eds_root': 'opensearch_data',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'os_ping',
            'os_cluster_status',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [
            {
                'id': 'cid',
                'pattern': 'c-{id}.rw.db.yandex.net',
            }
        ],
        'enable_oslogin': True,
        'labels': {},
    },
    'opensearch_master': {
        'sg_service_rules': [
            {
                'direction': 'BOTH',
                'ports_from': 9200,
                'ports_to': 9201,
            },
            {
                'direction': 'BOTH',
                'ports_from': 9300,
                'ports_to': 9300,
            },
        ],
        'compute_image_type': 'opensearch',
        'compute_image_type_template': {},
        'compute_recreate_on_resize': False,
        'compute_root_disk_type_id': 'network-hdd',
        'create_host_pillar': {
            'replica': True,
        },
        'host_os': 'linux',
        'dbm_data_path': '/var/lib/opensearch',
        'rootfs_space_limit': 21474836480,
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_opensearch_bionic.sh',
        'dbm_bootstrap_cmd_template': {},
        'conductor_root_group': 'mdb_porto_opensearch_master',
        'conductor_alt_groups': [],
        'eds_root': 'opensearch_master',
        'eds_alt_roots': [],
        'juggler_checks': [
            'META',
            'os_ping',
            'os_cluster_status',
        ],
        'issue_tls': True,
        'create_gpg': True,
        'managed_zone': 'db.yandex.net',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [
            {
                'id': 'cid',
                'pattern': 'c-{id}.rw.db.yandex.net',
            }
        ],
        'enable_oslogin': True,
        'labels': {},
    },
    'hadoop': {
        'dbm_data_path': '/var/lib/hadoop',
        'dbm_bootstrap_cmd': '/usr/local/yandex/porto/mdb_dbaas_hadoop.sh',
        'issue_tls': False,
        'create_gpg': False,
        'managed_zone': '',
        'public_zone': '',
        'private_zone': '',
        'group_dns': [],
        'running_operations_limit': 5,
        'labels': {},
    },
    'dbm': {
        'managing_project_id': '0:1589',
        'project_id': '0:1589',
        'project': 'pgaas',
    },
    'dns': {
        'api': 'SLAYER',
    },
    'yc_dns': {
        'url': '',
        'ca_path': '',
        'zones': {},
        'ignored_zones': [],
        'ttl': 300,
        'record_types': ['A'],
        'operation_wait_timeout': 600,
        'operation_wait_step': 1.0,
    },
    'slayer_dns': {
        'account': 'robot-dnsapi-mdb',
        'ttl': 300,
        'token': '',
        'ca_path': '',
        'url': '',
        'name_servers': ['2a02:6b8::1001'],
    },
    'resource_manager': {
        'url': '',
        'ca_path': '',
    },
    's3': {
        'disable_ssl_warnings': False,
        'addressing_style': 'auto',
        'region_name': 'us-east-1',
        'ca_path': '',
        'backup': {
            'location_constraint': '',
            'allow_lifecycle_policies': False,
        },
        'cloud_storage': {
            'location_constraint': '',
            'allow_lifecycle_policies': False,
        },
        'secure_backups': {
            'access_key_id': '',
            'secret_access_key': '',
            'endpoint_url': '',
            'location_constraint': '',
            'secured': True,
            'override_default': True,
            'folder_id': '',
            'iam': {
                'service_account_id': '',
                'key_id': '',
                'private_key': '',
            },
        },
        'idm_endpoint_url': '',
        'dataplane_role_arn': '',
    },
    'compute': {
        'managed_dns_zone_id': '',
        'managed_dns_ttl': 300,
        'managed_dns_ptr': True,
        'user_dns_zone_id': '',
        'user_dns_ttl': 300,
        'user_dns_ptr': True,
        'geo_map': {
            'man': 'man',
            'iva': 'iva',
            'myt': 'myt',
            'sas': 'sas',
            'vla': 'vla',
        },
        'use_security_group': True,
        'service_account_id': '',
        'key_id': '',
        'private_key': '',
        'placement_group': {
            'best_effort': False,
        },
    },
    'vpc': {
        'token': None,
        'service_account_id': None,
        'key_id': None,
        'private_key': None,
        'url': 'network-api-internal.private-api.cloud-preprod.yandex.net:9823',
    },
    'user_compute': {
        'service_account_id': None,
        'key_id': None,
        'private_key': None,
    },
    'aws': {
        'region_name': '',
        'access_key_id': '',
        'secret_access_key': '',
        'labels': {},
        'dataplane_role_arn': '',
        'dataplane_account_id': '',
    },
    'ec2': {
        'enabled': False,
        'images_prefix': 'dc-aws',
        'supported_disk_types': ['gp2', 'gp3'],
        'instance_profile_arn': '',
        'retries': {
            'mode': 'standard',
            'max_attempts': 15,
        },
    },
    'mdb_vpc': {
        'enabled': False,
        'url': 'vpc-api:443',
        'cert_file': '/opt/yandex/allCAs.pem',
        'server_name': '',
        'insecure': False,
    },
    'route53': {
        'enabled': False,
        'public_hosted_zone_id': '',
        'ttl': 300,
        'retries': {
            'mode': 'standard',
            'max_attempts': 15,
        },
    },
    'kms': {
        'enabled': False,
        'retries': {
            'mode': 'standard',
            'max_attempts': 15,
        },
    },
    'aws_iam': {
        'enabled': False,
        'retries': {
            'mode': 'standard',
            'max_attempts': 15,
        },
        'managed_dataplane_policy_arns': [],
    },
    'aws_s3': {
        'enabled': False,
        'retries': {
            'mode': 'standard',
            'max_attempts': 15,
        },
    },
    'cert_api': {
        'api': 'CERTIFICATOR',
        'ca_name': 'InternalCA',
        'cert_type': 'mdb',
        'ca_path': '',
        'wildcard_cert': False,
    },
    'juggler': {
        'enabled': True,
        'downtime_hours': 1,
    },
    'deploy': {
        'version': 1,
        'group': '',
        'regional_mapping': {},
        'url_v2': '',
        'token_v2': '',
        'attempts': 6,
        'timeout': 3600,
        'dataplane_host_v2': '',
        'ca_path': '',
    },
    'solomon': {
        'service_provider_id': 'mdb',
        'project': 'yandexcloud',
        'service': 'yandexcloud_dbaas',
        'ident_template': 'mdb_{ident}',
        'service_label': 'yandexcloud_dbaas',
        'enabled': True,
    },
    'internal_api': {},
    'instance_group_service': {
        'url': '',
        'grpc_timeout': 30,
    },
    'managed_kubernetes_service': {
        'url': '',
        'grpc_timeout': 30,
    },
    'loadbalancer_service': {
        'url': '',
        'grpc_timeout': 30,
    },
    'lockbox_service': {
        'url': '',
        'grpc_timeout': 30,
    },
    'managed_postgresql_service': {
        'url': '',
        'grpc_timeout': 30,
    },
    'iam_token_service': {
        'url': '',
        'expire_thresh': 180,
    },
    'iam_access_service': {
        'url': '',
    },
    'iam_service': {
        'url': '',
    },
    'iam_jwt': {
        'audience': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
        'url': '',
        'cert_file': '',
        'expire_thresh': 180,
        'request_expire': 3600,
        'server_name': '',
        'insecure': False,
    },
    'ssh': {
        'key_user': 'robot-pgaas-deploy',
    },
    'dataproc_manager': {
        'url': '',
        'cert_file': '',
        'server_name': '',
        'sleep_time': 5,
        'grpc_timeout': 30,
        'insecure': False,
    },
    'mlock': {
        'enabled': False,
        'insecure': False,
        'url': '',
        'keepalive_time_ms': 300000,
        'token': None,
        'timeout': 600,
        'server_name': '',
        'cert_file': '',
        'service_account_id': None,
        'key_id': None,
        'private_key': None,
    },
    'cloud_storage': {
        'folder_id': '',
        'service_account_id': '',
        'key_id': '',
        'private_key': '',
    },
    'per_cluster_service_accounts': {
        'folder_id': '',
    },
    'billingdb': {
        'enabled': False,
        'billingdb_dsn': "",
        'billingdb_hosts': [""],
    },
}


def get_config(config_paths):
    """
    Load config, validate and return namespaced dict
    """
    return parse_config_set(config_paths, CONFIG_SCHEMA, default=DEFAULT_CONFIG)


def worker_args_parser(description: str = None) -> argparse.ArgumentParser:
    """
    Worker arguments
    """
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument(
        '-c', '--config', type=str, nargs='+', default=['/etc/dbaas-worker.conf'], help='Config file path'
    )
    return parser
