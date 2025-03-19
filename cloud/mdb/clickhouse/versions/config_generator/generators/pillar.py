from collections import OrderedDict
from typing import List

from cloud.mdb.clickhouse.versions.config_generator.utils import Config, dump_yaml
from cloud.mdb.clickhouse.versions.lib.version import VersionsConfig


def pillar_generator(versions_config: VersionsConfig) -> List[Config]:
    pillars = {
        'internal_api_clickhouse_versions.sls': generate_internal_api_pillar,
        'mdb_internal_api_clickhouse_versions.sls': generate_mdb_internal_api_pillar,
        'ch_template_version.sls': generate_template_pillar,
    }

    configs = []
    for filename, generator in pillars.items():
        for environment in ('porto', 'compute'):
            pillar = generator(versions_config, environment)
            file_path = f'../../salt/pillar/generated/{environment}/{filename}'
            configs.append(Config(file_path, dump_yaml(pillar, generation_notice=True)))

    return configs


def generate_internal_api_pillar(versions_config: VersionsConfig, environment: str) -> dict:
    versions = []
    for version_config in versions_config.versions:
        if not version_config.available(environment):
            continue

        version = OrderedDict()
        version['version'] = version_config.version
        version['name'] = version_config.name
        version['default'] = version_config.default(environment)
        version['deprecated'] = version_config.deprecated(environment)
        version['downgradable'] = version_config.downgradable
        versions.append(version)

    return {
        'data': {
            'internal_api': {
                'config': {
                    'versions': {
                        'clickhouse_cluster': versions,
                    }
                }
            }
        }
    }


def generate_mdb_internal_api_pillar(versions_config: VersionsConfig, environment: str) -> dict:
    versions = []
    for version_config in versions_config.versions:
        if not version_config.available(environment):
            continue

        version = OrderedDict()
        version['version'] = version_config.version
        version['name'] = version_config.name
        version['default'] = version_config.default(environment)
        version['deprecated'] = version_config.deprecated(environment)
        version['updatable_to'] = version_config.updatable_to(environment)
        versions.append(version)

    return {
        'data': {
            'mdb-internal-api': {
                'config': {
                    'logic': {
                        'clickhouse': {
                            'versions': versions,
                        }
                    }
                }
            }
        }
    }


def generate_template_pillar(versions_config: VersionsConfig, environment: str) -> dict:
    return {
        'data': {
            'clickhouse': {
                'ch_version': versions_config.default_version(environment).version,
            }
        }
    }
