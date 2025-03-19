from copy import deepcopy
from collections import OrderedDict
from typing import List

from cloud.mdb.clickhouse.versions.config_generator.utils import Config, dump_yaml
from cloud.mdb.clickhouse.versions.lib.version import VersionsConfig


def helm_config_generator(versions_config: VersionsConfig) -> List[Config]:
    configs = {
        '../../datacloud/generated/clickhouse_versions.yaml': {
            'environment': 'datacloud',
            'config_paths': [['internal_api', 'clickhouse', 'versions']],
        },
        '../../salt/pillar/generated/compute/clickhouse_versions.yaml': {
            'environment': 'compute',
            'config_paths': [
                ['go-api', 'app_config', 'logic', 'clickhouse', 'versions'],
                ['py-api', 'app_config', 'versions', 'clickhouse_cluster'],
            ],
        },
    }

    return [
        Config(path, dump_yaml(generate_helm_config(versions_config, **conf), generation_notice=True))
        for path, conf in configs.items()
    ]


def generate_helm_config(versions_config: VersionsConfig, environment: str, config_paths: List[List[str]]) -> dict:
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

    result = {}
    for path in config_paths:
        conf = deepcopy(versions)
        for key in reversed(path):
            conf = {key: conf}
        result.update(conf)

    return result
