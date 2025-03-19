"""
Contains values of dbaas:default_versions.
"""

import yaml

FILE_PATH = '../salt/pillar/metadb_default_versions.sls'


def read_default_versions():
    """
    Read yaml file and load DEFAULT_VERSIONS for dbaas.default_versions
    """

    with open(FILE_PATH, 'r') as conf_file:
        return yaml.load(conf_file)['data']['dbaas_metadb']['default_versions']


DEFAULT_VERSIONS = read_default_versions()
