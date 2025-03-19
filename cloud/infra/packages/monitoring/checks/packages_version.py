#!/usr/bin/env python3
"""
This script is designed to check whether the versions installed in the release of a component match.
The script accepts the name of the component as input.

The script searches for the list of packages in the predefined directory 'COMPONENT_PACKAGES_DIR'.
The filename must match the name of the component with the 'yaml' extension.
For example, if you run the script with the 'secret-agent' argument,
the list of packages will be displayed must be located in the 'secret-agent.yaml' file.

The file content example with the package list:

'
packages:
  package1-name: "pkg-version1"
  package2-name: "pkg-version2"
'
"""

import apt
import os
import sys
import time
import timeout_decorator
from typing import Any, Callable, Generator, Iterable, Tuple
import yaml
from yc_monitoring import JugglerPassiveCheck, CacheFile

COMPONENT_PACKAGES_DIR = "/var/cache/monitoring_packages"
MAX_ITEMS_IN_DESCRIPTION = 5


def all_installed_packages() -> Generator[Tuple[str], None, None]:
    for pkg in apt.Cache():
        if pkg.is_installed:
            yield pkg.name, pkg.installed.version


def format_with_limit(
    queue: Iterable[Any],
    formatter: Callable[[Any], str] = lambda x: x,
    limit: int = MAX_ITEMS_IN_DESCRIPTION
) -> Generator[str, None, None]:
    cnt = 0
    for item in queue:
        if cnt >= limit:
            yield "and more"
            return
        yield formatter(item)
        cnt += 1


@timeout_decorator.timeout(15, use_signals=False)
def check_versions(check: JugglerPassiveCheck, component_name: str) -> JugglerPassiveCheck:
    """
    Timeout decorator used Multiprocessing module, so
    we must return check object from function to save changes.
    """
    packages_path = os.path.join(
        COMPONENT_PACKAGES_DIR, component_name + ".yaml")

    if not os.path.exists(packages_path):
        check.crit("File '{}' with packages not found".format(packages_path))
        return check

    try:
        component_pkgs = yaml.load(open(packages_path), Loader=yaml.SafeLoader)["packages"]
    except KeyError as ex:
        check.crit("Wrong format: not found key '{}'".format(ex))
        return check
    except Exception as ex:
        check.crit("Wrong format: {}".format(ex))
        return check

    if not isinstance(component_pkgs, dict):
        check.crit("Wrong format: type '{}', must be dict".format(component_pkgs.__class__.__name__))
        return check

    if not component_pkgs:
        check.ok("Packages list is empty")
        return check

    must_be_deleted = set()
    wrong_versions = {}

    for pkg, installed in all_installed_packages():
        try:
            pinned = component_pkgs.pop(pkg)
        except KeyError:
            # pinned package not related to current component, skip it
            continue

        if pinned.lower().find("purge") != -1:
            # the 'purge' version is pinned, but the package is installed
            must_be_deleted.add(pkg)
            continue

        if pinned != installed:
            wrong_versions[pkg] = {"pinned": pinned, "installed": installed}

    component_pkgs = {pkg: ver for pkg, ver in component_pkgs.items() if ver.lower().find("purge") == -1}

    if wrong_versions:
        check.crit("{} wrong versions found: {}".format(
            len(wrong_versions),
            ", ".join(format_with_limit(
                sorted(wrong_versions.items()),
                formatter=lambda x: "for {pkg}={inst} must be {pin}".format(
                    pkg=x[0], inst=x[1]["installed"], pin=x[1]["pinned"]),
            ))
        ))

    if must_be_deleted:
        check.crit("Next {} packages must be deleted: {}".format(
            len(must_be_deleted),
            ", ".join(format_with_limit(sorted(must_be_deleted)))
        ))

    if component_pkgs:
        check.warn("{} packages pinned, but not installed: {}".format(
            len(component_pkgs),
            ", ".join(format_with_limit(
                sorted(component_pkgs.items()),
                formatter=lambda x: "{pkg}={ver}".format(pkg=x[0], ver=x[1]),
            ))
        ))

    return check


def main():
    try:
        check_name = "{}_packages".format(sys.argv[1])
    except IndexError:
        raise IndexError("you must specify a component")

    check = JugglerPassiveCheck(check_name)
    cache_filename = "{}_cache.yaml".format(check_name)

    try:
        cache = CacheFile(cache_filename)
        try:
            check = check_versions(check, sys.argv[1])
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
