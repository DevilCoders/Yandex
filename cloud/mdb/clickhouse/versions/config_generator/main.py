#!/usr/bin/env python3
import argparse
import sys
from difflib import unified_diff
from io import StringIO

from more_itertools import flatten

from cloud.mdb.clickhouse.versions.config_generator.generators.helm import helm_config_generator
from cloud.mdb.clickhouse.versions.config_generator.generators.infra_tests import infra_tests_config_generator
from cloud.mdb.clickhouse.versions.config_generator.generators.maintenance import maintenance_config_generator
from cloud.mdb.clickhouse.versions.config_generator.generators.pillar import pillar_generator
from cloud.mdb.clickhouse.versions.config_generator.utils import Config
from cloud.mdb.clickhouse.versions.lib.version import get_versions_config


def main():
    """
    Program entry point.
    """
    generators = [
        helm_config_generator,
        infra_tests_config_generator,
        maintenance_config_generator,
        pillar_generator,
    ]

    args = parse_args()

    versions_config = get_versions_config()

    configs = flatten(generator(versions_config) for generator in generators)

    if args.check:
        exit_code = 0
        for config in configs:
            if not check_config(config):
                exit_code = 1
        exit(exit_code)

    for config in configs:
        write_config(config)


def parse_args() -> argparse.Namespace:
    """
    Parse command-line arguments.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--check', action='store_true')
    return parser.parse_args()


def write_config(config: Config) -> None:
    with open(config.file_path, 'w') as f:
        f.write(config.content.strip())
        f.write('\n')


def check_config(config: Config) -> bool:
    buffer = StringIO()
    buffer.write(config.content.strip())
    buffer.write('\n')
    expected_content = buffer.getvalue().splitlines(keepends=True)

    try:
        with open(config.file_path, 'r') as file:
            actual_content = file.readlines()
    except FileNotFoundError:
        actual_content = []

    result = unified_diff(
        actual_content, expected_content, f'{config.file_path}    (actual)', f'{config.file_path}    (expected)'
    )
    diff = ''.join(result)
    if diff:
        print(diff, file=sys.stderr)
        return False

    return True


if __name__ == '__main__':
    main()
