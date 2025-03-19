"""
Default config and parser
"""

import collections.abc
import getpass
import json
import os
from types import SimpleNamespace

import jsonschema
from dbaas_common.config import ConfigValidationError
from dbaas_common.dict import combine_dict

CONFIG_SCHEMA = {
    'description': 'JSON Schema for DBaaS Internal API functional tests suite',
    'type': 'object',
    'properties': {
        'main': {
            'type': 'object',
            'properties': {
                'disable_cleanup': {
                    'type': 'boolean',
                },
                'start_timeout': {
                    'type': 'integer',
                    'minimum': 1,
                },
            },
            'required': ['disable_cleanup', 'start_timeout'],
        },
        'metadb': {
            'type': 'object',
            'properties': {
                'repo_url': {
                    'type': 'string',
                },
                'dsn_base': {
                    'type': 'string',
                },
                'migrations_timeout': {
                    'type': 'integer',
                    'minimum': 1,
                },
            },
            'required': ['repo_url', 'dsn_base', 'migrations_timeout'],
        },
    },
    'required': ['main', 'metadb'],
}

DEFAULT_CONFIG = {
    'main': {
        'disable_cleanup': bool(os.environ.get('DISABLE_CLEANUP', False)),
        'start_timeout': 5,
    },
    'metadb': {
        'repo_url': 'ssh://{name}@review.db.yandex-team.ru:9440/mdb/dbaas-metadb'.format(name=getpass.getuser()),
        'dsn_base': 'dbname=dbaas_metadb user=dbaas_api host=localhost',
        'migrations_timeout': 60,
    },
}


def parse_config(path, schema, default=None):
    """
    Parse config from path
    """
    if default is None:
        default = dict()
    default_config = default.copy()
    if os.path.exists(path):
        with open(path) as config_file:
            config = combine_dict(default_config, json.loads(config_file.read()))
    else:
        config = combine_dict(default_config, dict())

    validator = jsonschema.Draft4Validator(schema)
    config_errors = validator.iter_errors(config)
    report_error = jsonschema.exceptions.best_match(config_errors)
    if report_error is not None:
        raise ConfigValidationError('Malformed config: {error}'.format(error=report_error.message))

    namespaces = {}
    for section, options in config.items():
        if isinstance(options, collections.abc.Mapping):
            namespaces[section] = SimpleNamespace(**options)
        else:
            namespaces[section] = options
    return SimpleNamespace(**namespaces)


def get_config():
    """
    Get config from
    """
    path = os.path.expanduser('~/.dbaas-internal-api-test.conf')
    return parse_config(path, CONFIG_SCHEMA, DEFAULT_CONFIG)
