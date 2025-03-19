#!/usr/bin/env python3
"""
This script shows packages that are not related to any component in bootstrap, i.e. they are ownerless.

In addition to the packages pinned in bootstrap, there are packages that are installed on the host differently.
All these packages are listed below in the 'PACKAGES_OWNED_BY_OTHER_SYSTEMS' variable.

The script searches for the list of packages in the files with 'yaml' extension
in the predefined directory 'COMPONENT_PACKAGES_DIR'.

The file content example with the package list:

'
packages:
  package1-name: "pkg-version1"
  package2-name: "pkg-version2"
'
"""

import apt
import os
import time
import timeout_decorator
from typing import Dict, Generator, List, Set
import yaml
from yc_monitoring import JugglerPassiveCheck, CacheFile

# packages listed in config not managed via salt-formula, or waiting any actions
CONFIG_PATH = "ownerless_packages.yaml"
COMPONENT_PACKAGES_DIR = "/var/cache/monitoring_packages"
MAX_ITEMS_IN_DESCRIPTION = 40


def all_installed_packages(apt_cache: apt.Cache) -> Generator[str, None, None]:
    for pkg in apt_cache:
        if pkg.is_installed:
            yield pkg.name


def get_kernel_packages(apt_cache: apt.Cache) -> Generator[str, None, None]:
    for pkg in apt_cache:
        if pkg.is_installed:
            if pkg.name.startswith("linux-headers-"):
                yield pkg.name
            elif pkg.name.startswith("linux-image-"):
                yield pkg.name


def mark_warn_packages(
    cur_check: JugglerPassiveCheck,
    ownerless_pkgs: Set[str],
    service_packages: Dict[str, List[str]],
    awaiting_actions_packages: Dict[str, Set[str]],
) -> Set[str]:
    warn_packages = set()
    warn_components = set()
    for service, pkgs in service_packages.items():
        for pkg_name in pkgs:
            if pkg_name in ownerless_pkgs:
                warn_packages.add(pkg_name)
                warn_components.add(service)
    if warn_components:
        cur_check.warn("{} packages waiting for the next components release: {}".format(
            len(warn_packages), ", ".join(sorted(warn_components))))
    for ticket_id in sorted(awaiting_actions_packages):
        pkgs = set()
        for pkg_name in awaiting_actions_packages[ticket_id]:
            if pkg_name in ownerless_pkgs:
                warn_packages.add(pkg_name)
                pkgs.add(pkg_name)
        if pkgs:
            cur_check.warn("awaiting https://st.yandex-team.ru/{} for: {}".format(ticket_id, ", ".join(sorted(pkgs))))
    # remove this hardcode after contrail-vrouter-dkms will be containerized
    pkgs = set()
    for pkg_name in ownerless_pkgs:
        if pkg_name.startswith("contrail-vrouter-dkms-"):
            pkgs.add(pkg_name)
            warn_packages.add(pkg_name)
    if pkgs:
        cur_check.warn("awaiting https://st.yandex-team.ru/CLOUD-65682 for: contrail-vrouter-dkms-*")
    return warn_packages


@timeout_decorator.timeout(15, use_signals=False)
def check_ownerless_packages(check: JugglerPassiveCheck, config_path: str) -> JugglerPassiveCheck:
    """
    Timeout decorator used Multiprocessing module, so
    we must return check object from function to save changes.
    """
    try:
        with open(config_path) as fd:
            config = yaml.load(fd, Loader=yaml.SafeLoader)
    except IOError:
        config = {}

    packages_owned_by_other_systems = set(config.get("packages_owned_by_other_system", []))
    packages_fixed_with_next_release = config.get("packages_fixed_with_next_release", {})
    packages_awaiting_action = config.get("packages_awaiting_action", {})

    apt_cache = apt.Cache()
    owned_packages = set(packages_owned_by_other_systems)
    owned_packages.update(get_kernel_packages(apt_cache))
    for root, _, files in os.walk(COMPONENT_PACKAGES_DIR):
        if root != COMPONENT_PACKAGES_DIR:
            continue
        for file_name in files:
            if not file_name.endswith(".yaml"):
                continue
            file_path = os.path.join(COMPONENT_PACKAGES_DIR, file_name)
            try:
                component_pkgs = yaml.load(open(file_path), Loader=yaml.SafeLoader)["packages"]
            except KeyError as ex:
                check.crit("Wrong format in file '{}': not found key '{}'".format(file_path, ex))
                return check
            except Exception as ex:
                check.crit("Wrong format in file '{}': {}".format(file_name, ex))
                return check
            else:
                owned_packages.update(set(component_pkgs.keys()))

    ownerless_packages = set(all_installed_packages(apt_cache)).difference(owned_packages)
    warn_packages = mark_warn_packages(
        check, ownerless_packages, packages_fixed_with_next_release, packages_awaiting_action)
    crit_packages = ownerless_packages.difference(warn_packages)
    if crit_packages:
        msg = []
        for pkg in sorted(crit_packages):
            if len(msg) >= MAX_ITEMS_IN_DESCRIPTION:
                msg.append("and more {}".format(
                    len(crit_packages) - MAX_ITEMS_IN_DESCRIPTION))
                break
            msg.append(pkg)
        check.crit("{:d} packages not related to any component: {}".format(len(crit_packages), ", ".join(msg)))
    return check


def main():
    check = JugglerPassiveCheck("ownerless_packages")
    try:
        cache = CacheFile("ownerless_packages_cache.yaml")
        try:
            check = check_ownerless_packages(check, CONFIG_PATH)
            data = {
                "status": check.status,
                "messages": check.messages,
                "timestamp": int(time.time()),
                "count": 0,
            }
            cache.store("data", data)
        except timeout_decorator.TimeoutError:
            data = cache.load("data")
            data["count"] += 1
            cache.store("data", data)
            timeout_duration = int(time.time()) - data["timestamp"]
            if timeout_duration > 1800:
                check.crit("The check was timed out {} times in {} seconds".format(data["count"], timeout_duration))
            elif timeout_duration > 900:
                check.warn("The check was timed out {} times in {} seconds".format(data["count"], timeout_duration))
            else:
                check.restore_state(data["status"], data["messages"])
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == "__main__":
    main()
