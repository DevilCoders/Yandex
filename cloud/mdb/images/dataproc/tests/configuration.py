"""
Configuration for dataproc-image tests
"""
import os
import yaml
import secrets
import collections.abc

GiB = 2**30


def default_configuration():
    """
    Return default configuration
    """
    return {
        'user': 'ubuntu',
        'compute': {
            'platform_id': 'standard-v2',
            'memory': 8 * GiB,
            'cores': 2,
            'core_fraction': 20,
            'root_disk_type': 'network-ssd',
            'root_disk_size': 20 * GiB,
            'image_id': None,
        },
        'vpc': {
            'dhcp_options': {},
            # v4_cidr_block will be ignored if environment.subnet_id has own v4_cidr_block
            'v4_cidr_block': '192.168.0.0/24',
        },
        'mdb': {
            'clickhouse': {
                'environment': 'PRODUCTION',
                'db': 'test_db',
                'user': 'test_user',
                'password': secrets.token_urlsafe(),
            },
        },
        'ssh_key_yav_id': 'sec-01f2vn3vntm6y7rzhzrryr2xcj',
        's3': {
            'region': 'ru-central1',
            'endpoint': 'storage.cloud-preprod.yandex.net',
        },
        'sa_key_secret_id': 'sec-01f21mtzjv44s5sv54whxc3gfw',
        'environment': {
            'region': 'ru-central1',
            'folder-id': 'aoe9deomk5fikb8a0f5g',
            'endpoint': 'api.cloud-preprod.yandex.net',
            'network_id': 'c64hq73umfj803fn8img',
            'subnet_id': 'blt7bkf0oegob5crc69e',
            'log_group_id': 'af31g5nmuesg9am02cci',
            'zone': 'ru-central1-b',
            'logging_endpoint_url': 'api.cloud-preprod.yandex.net:443',
        },
    }


def get_user_configuration():
    """
    Load user overrided options
    """
    user_configuration_path = os.path.expanduser('~/.dataproc-image-test.yaml')
    if os.path.exists(user_configuration_path):
        with open(user_configuration_path) as stream:
            try:
                return yaml.safe_load(stream)
            except yaml.YAMLError:
                raise
    return {}


def get_env_configuration():
    """
    Load options from environment variables
    """
    conf = {}
    if os.environ.get('TOKEN'):
        conf['environment'] = {'token': os.environ.get('TOKEN')}
    return conf


def validate_configuration(conf):
    """
    Check required fields of configuration
    """
    pass


def merge(original, update):
    """
    Recursively merge update dict into original.
    """
    for key in update:
        recurse_conditions = [
            # Does update have same key?
            key in original,
            # Do both the update and original have dicts at this key?
            isinstance(original.get(key), dict),
            isinstance(update.get(key), collections.abc.Mapping),
        ]
        if all(recurse_conditions):
            merge(original[key], update[key])
        else:
            original[key] = update[key]
    return original


def get_configuration():
    """
    Build configuration for test
    """
    conf = default_configuration()
    merge(conf, get_user_configuration())
    merge(conf, get_env_configuration())
    validate_configuration(conf)
    return conf
