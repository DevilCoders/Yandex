#!/usr/bin/env python3
"""
This script do next checks:
 - sources.list.d contains only valid files - extension must be '.list'
 - repositories list from config is equal repositories list on the host
"""

import os
import re
from typing import Dict, Generator, Tuple, Union
import yaml

from yc_monitoring import (
    JugglerPassiveCheck,
    JugglerPassiveCheckException,
)

CONFIG_PATH = "repositories_list.yaml"
SOURCES_LISTS_FILE = "/etc/apt/sources.list"
SOURCES_LISTS_DIR = "/etc/apt/sources.list.d/"
STAND_NAME_FILE = "/etc/debian_chroot"


class Repository:
    def __init__(self, type_: str, uri: str, suite: str, components: Tuple[str] = None):
        self.type = type_
        self.uri = uri
        self.suite = suite
        self.components = components or ()

    def __str__(self):
        s = " ".join((self.type, self.uri, self.suite))
        if self.components:
            s += " " + " ".join(self.components)
        return s

    def __repr__(self):
        return self.__str__()


# deb http://yandex-cloud-secure.dist.yandex.ru/yandex-cloud-secure stable/all/
# deb [arch=amd64] http://ru.archive.ubuntu.com/ubuntu/ focal main restricted
REPO_LINE_PATTERN = re.compile(
    r'^(?P<type>[\w-]+)\s*(\[[\w=]+])?\s+(?P<uri>[\w:/.-]+)\s+(?P<suite>[\w/-]+)(?P<components>(\s+([\w.-]+))*)$')


def get_stand_name(file_path: str):
    try:
        with open(file_path) as fd:
            return fd.readline().strip().lower()
    except FileNotFoundError:
        return None


def parse_source_line(line: str) -> Union[Repository, None]:
    match = REPO_LINE_PATTERN.match(line)
    if not match:
        return
    return Repository(
        type_=match.groupdict()["type"],
        uri=match.groupdict()["uri"],
        suite=match.groupdict()["suite"],
        components=tuple((el for el in match.groupdict()["components"].split(" ") if el)) or (),
    )


def iterate_lines_from_sources_files(check: JugglerPassiveCheck) -> Generator[str, None, None]:
    files_list = []
    if os.path.exists(SOURCES_LISTS_FILE):
        files_list.append(SOURCES_LISTS_FILE)
    if os.path.exists(SOURCES_LISTS_DIR):
        files_list.extend((os.path.join(SOURCES_LISTS_DIR, file_name) for file_name in os.listdir(SOURCES_LISTS_DIR)))
    unexpected_files = set()
    for file_name in files_list:
        if not file_name.endswith(".list"):
            unexpected_files.add(os.path.basename(file_name))
            continue
        with open(file_name) as fd:
            for line in fd.readlines():
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                yield line
    if unexpected_files:
        check.warn("unexpected files in sources.list.d: '{}'".format(", ".join(sorted(unexpected_files))))


def iterate_existing_repositories(check: JugglerPassiveCheck) -> Generator[Repository, None, None]:
    unparsed_lines = set()
    for line in iterate_lines_from_sources_files(check):
        source = parse_source_line(line)
        if not source:
            unparsed_lines.add(line)
            continue
        yield source
    if unparsed_lines:
        check.crit("unparsed lines: '{}'".format("; ".join(unparsed_lines)))


def iterate_config_repositories(config: Dict, check: JugglerPassiveCheck) -> Generator[Repository, None, None]:
    if not isinstance(config, dict):
        check.ok("no any repository are specified in the config")
        return
    for type_, uri_list in config.get("repositories", {}).items():
        if not isinstance(uri_list, dict):
            check.warn("unexpected data type in the config. Check key 'repositories.{}'".format(type_))
            continue
        for uri, suites in uri_list.items():
            if not isinstance(suites, list):
                check.warn("unexpected data type in the config. Check key 'repositories.{}.{}'".format(type_, uri))
                continue
            for suite in suites:
                yield Repository(type_=type_, uri=uri, suite=suite)


def load_config(check: JugglerPassiveCheck, config_path, stand: str) -> Union[Dict, None]:
    try:
        with open(config_path) as fd:
            data = yaml.load(fd, Loader=yaml.SafeLoader)
            if not isinstance(data, dict):
                raise JugglerPassiveCheckException("config must be dict, now '{}'".format(data.__class__.__name__))
            return data.get(stand, {})
    except FileNotFoundError:
        check.ok("the config not found")
        return None


def check_repositories_list(check: JugglerPassiveCheck, config: Dict):
    config_repositories_set = set(map(str, iterate_config_repositories(config, check)))
    if not config_repositories_set:
        check.ok("config for current stand is empty")
        return
    existing_repositories_set = set(map(str, iterate_existing_repositories(check)))
    absent_on_host = sorted(config_repositories_set.difference(existing_repositories_set))
    if absent_on_host:
        check.warn("These repositories are not present on the host: '{}'".format("; ".join(absent_on_host)))
    unexpected_repositories = sorted(existing_repositories_set.difference(config_repositories_set))
    if unexpected_repositories:
        check.crit("These repositories should not be on the host: '{}'".format("; ".join(unexpected_repositories)))


def main():
    check = JugglerPassiveCheck("repo")
    try:
        stand_name = get_stand_name(STAND_NAME_FILE)
        if stand_name:
            config = load_config(config_path=CONFIG_PATH, stand=stand_name, check=check)
            check_repositories_list(check, config)
            # TODO(nuraev): add check that apt checks the deb sign
        else:
            check.crit("can't get stand name")
    except JugglerPassiveCheckException as ex:
        check.crit(ex.description)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == '__main__':
    main()
