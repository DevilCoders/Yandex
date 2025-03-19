# -*- coding: utf-8 -*-
"""
Config-related functions
"""

import json
from types import SimpleNamespace

import jsonschema
import os

CONFIG_SCHEMA = {
    'description': 'JSON Schema for DBaaS-compute e2e tests',
    'type': 'object',
    'properties': {
        'log_level': {
            'enum': ['CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG'],
        },
        'log_format': {
            'type': 'string',
        },
        'log_file': {
            'type': 'string',
        },
        'log_size': {
            'type': 'integer',
        },
        'log_enable_stdout': {'type': 'boolean'},
        'interval': {
            'type': 'integer',
            'minimum': 1,
        },
        'timeout': {
            'type': 'integer',
            'minimum': 1,
        },
        'environment': {
            'enum': ['PRESTABLE', 'PRODUCTION'],
        },
        'request_timeout': {
            'type': 'integer',
            'minimum': 1,
        },
        'precheck_conn_timeout': {
            'type': 'integer',
            'minimum': 1,
        },
        'replication_timeout': {
            'type': 'integer',
            'minimum': 0,
        },
        'conn_ca_path': {
            'type': ['string', 'boolean'],
        },
        'dbname': {
            'type': 'string',
        },
        'dbuser': {
            'type': 'string',
        },
        'dbpassword': {
            'type': 'string',
        },
        'assign_public_ip': {
            'type': 'boolean',
        },
        'api_url': {
            'type': 'string',
        },
        'api_client': {
            'type': 'string',
        },
        'ca_path': {
            'type': ['string', 'boolean'],
        },
        'sa_creds': {
            'description': 'JSON Schema for service account',
            'type': 'object',
            'properties': {
                'service_account_id': {'type': 'string'},
                'id': {'type': 'string'},
                'private_key': {'type': 'string'},
            },
            'required': [
                'service_account_id',
                'id',
                'private_key',
            ],
        },
        'iam': {
            'description': 'JSON Schema for iam client',
            'type': 'object',
            'properties': {
                'host': {'type': 'string'},
                'cert_file': {'type': 'string'},
            },
            'required': [
                'host',
                'cert_file',
            ],
        },
        'folder_id': {
            'type': 'string',
        },
        'network_id': {
            'type': ['null', 'string'],
        },
        'subnet_id': {
            'type': ['null', 'string'],
        },
        'dualstack_network_id': {
            'type': ['null', 'string'],
        },
        'dualstack_subnet_id': {
            'type': ['null', 'string'],
        },
        'geo_map': {
            'type': 'object',
        },
        'flavor': {
            'type': 'string',
        },
        'disk_type': {
            'type': 'string',
        },
        'redis_flavor': {
            'type': 'string',
        },
        'sqlserver_flavor': {
            'type': 'string',
        },
        'elasticsearch_flavor': {
            'type': 'string',
        },
        'ssh_public_key': {
            'type': 'string',
        },
        'ssh_private_key_path': {
            'type': 'string',
        },
        's3_bucket': {
            'type': 'string',
        },
        'odbc_check_script': {
            'type': 'string',
        },
        'hadoop_flavor': {
            'type': 'string',
        },
        'hadoop_log_group_id': {
            'type': 'string',
        },
        'greenplum_master_flavor': {
            'type': 'string',
        },
        'greenplum_segment_flavor': {
            'type': 'string',
        },
        'project_id': {
            'type': 'string',
        },
        'cloud_type': {
            'type': 'string',
        },
        'region_id': {
            'type': 'string',
        },
        'name': {
            'type': 'string',
        },
        'resource_preset_id': {
            'type': 'string',
        },
        'description': {
            'type': 'string',
        },
        'version': {
            'type': 'string',
        },
        'encryption': {
            'type': 'boolean',
        },
        'disk_size': {
            'type': 'integer',
        },
        'replica_count': {
            'type': 'integer',
        },
        'shard_count': {
            'type': 'integer',
        },
    },
    'required': [
        'log_level',
        'log_format',
        'log_file',
        'log_size',
        'interval',
        'timeout',
        'request_timeout',
        'ca_path',
        'dbname',
        'dbuser',
        'dbpassword',
        'environment',
        'precheck_conn_timeout',
        'replication_timeout',
        'conn_ca_path',
        'iam',
        'sa_creds',
        'api_url',
        'api_client',
        'folder_id',
        'geo_map',
        'flavor',
        'redis_flavor',
        'sqlserver_flavor',
        'assign_public_ip',
        'disk_type',
        'ssh_public_key',
        'ssh_private_key_path',
        'odbc_check_script',
        'hadoop_flavor',
    ],
}

DEFAULT_CONFIG = {
    'log_level': 'INFO',
    'log_format': '%(asctime)s [%(levelname)s] %(name)s:\t%(message)s',
    'log_enable_stdout': False,
    'interval': 10,
    'timeout': 600,
    'request_timeout': 60,
    'precheck_conn_timeout': 300,
    'replication_timeout': 60,
    'conn_ca_path': True,
    'dbname': 'dbaas_e2e',
    'dbuser': 'alice',
    'dbpassword': 'rdGOSgsrSyC51wcXtRMnEK1iI',
    'api_client': 'internal',
    'flavor': 'db1.nano',
    'redis_flavor': 'm1.nano',
    'sqlserver_flavor': 's1.micro',
    'elasticsearch_flavor': 's2.micro',
    'assign_public_ip': True,
    'disk_type': 'network-nvme',
    'environment': 'PRESTABLE',
    'ssh_public_key': 'ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ',
    'ssh_private_key_path': '/home/monitor/.ssh/dataproc_ed25519',
    'odbc_check_script': '/bin/false',
    'hadoop_flavor': 'b2.small',
    'greenplum_master_flavor': 's2.micro',
    'greenplum_segment_flavor': 's2.micro',
    'greenplum_segment_in_host': 1,
    'greenplum_master_host_count': 2,
    'greenplum_segment_host_count': 2,
    'project_id': 'mdb-junk',
    'cloud_type': 'aws',
    'region_id': 'eu-central-1',
    'name': 'test',
    'resource_preset_id': 's1-c2-m8',
    'arm_resource_preset_id': 'a1-c2-m8',
    'encryption': True,
    'description': 'test cluster',
    'disk_size': 68719476736,
    'replica_count': 1,
    'shard_count': 1,
}


class ConfigValidationError(RuntimeError):
    """
    Common validation error
    """


def get_config(config_path):
    """
    Load config, validate and return namespaced dict
    """
    config = DEFAULT_CONFIG.copy()
    with open(config_path) as config_file:
        config.update(json.loads(config_file.read()))

    if key := os.getenv("E2E_TESTS_SA_KEY"):
        config['sa_creds']['private_key'] = key

    validator = jsonschema.Draft4Validator(CONFIG_SCHEMA)
    config_errors = validator.iter_errors(config)
    report_error = jsonschema.exceptions.best_match(config_errors)
    if report_error is not None:
        raise ConfigValidationError('Malformed config: {error}'.format(error=report_error.message))

    return SimpleNamespace(**config)
