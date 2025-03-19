import re
from collections import OrderedDict
from typing import List

from cloud.mdb.clickhouse.versions.config_generator.utils import Config, dump_yaml, load_file, load_yaml_file
from cloud.mdb.clickhouse.versions.lib.version import Version, VersionsConfig

PY_INT_API_CONFIG_PATH = '../../dbaas_infra_tests/images/internal-api/config/config.py'
GO_INT_API_CONFIG_PATH = '../../dbaas_infra_tests/images/go-internal-api/config/mdb-internal-api.yaml'


def infra_tests_config_generator(versions_config: VersionsConfig) -> List[Config]:
    # Infra tests support up to 3 ClickHouse versions for now.
    last_versions = versions_config.versions[-3:]

    return [
        generate_py_int_api_config(last_versions),
        generate_go_int_api_config(last_versions),
    ]


def generate_go_int_api_config(last_versions: List[Version]) -> Config:
    file_path = GO_INT_API_CONFIG_PATH
    config = load_yaml_file(file_path)

    config_versions = []
    last_version_ids = [version.version for version in last_versions]
    for version in last_versions:
        config_versions.append(
            OrderedDict(
                (
                    ('version', version.version),
                    ('name', version.name),
                    ('deprecated', version.deprecated('porto')),
                    ('default', version.default('porto')),
                    ('updatable_to', [ver for ver in version.updatable_to('porto') if ver in last_version_ids]),
                )
            )
        )

    config['logic']['clickhouse']['versions'] = config_versions

    return Config(file_path, dump_yaml(config))


def generate_py_int_api_config(last_versions: List[Version]) -> Config:
    file_path = PY_INT_API_CONFIG_PATH
    config = load_file(file_path)

    config_versions = [{'version': version.version} for version in last_versions]

    config = re.sub(
        pattern=r"('clickhouse_cluster':)\s*\[[^\]]+\]",
        repl=rf"\1 {config_versions}",
        string=config,
        flags=re.MULTILINE | re.DOTALL,
    )

    return Config(file_path, config)
