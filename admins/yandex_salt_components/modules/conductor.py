#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Conductor module, provides minimal interface to conductor
"""

import logging
from typing import Any
import requests

__author__ = "d3rp"

log = logging.getLogger(__name__)

# Define the module's virtual name
__virtualname__ = "conductor"

__grains__: dict[str, Any] = {}


def __virtual__():
    return __virtualname__


def _query(query, cached=False):
    """
    Query to conductor with 3 retry
    """
    url = f"http://c.yandex-team.ru/api{'-lightcached' if cached else ''}/{query}"
    resp = requests.get(url)
    resp.raise_for_status()
    return resp.json()


def _get_pkgs_last_version(pkg, env, cached=False):
    """
    Return version "1.2.3-0ubuntu0"
    """

    log.info(f"Pkgname: {pkg}, environment: {env}, use cache: {cached}")
    assert pkg, "package name requeired"
    version_list = _query(f"package_version/{pkg}", cached)
    log.info(f"Conductor response: {version_list}")

    envs = ("unstable", "testing", "prestable", "stable")
    for env_to, env_from in zip(envs[:-1], envs[1:]):
        # dev <= testing <= prestable <= stable
        if not version_list[pkg][env_to]["version"]:
            ver = version_list[pkg][env_from]["version"]
            version_list[pkg][env_to]["version"] = ver

    return version_list[pkg][env]["version"]


def _get_pkgs_list(cached=False):
    """
    Return list of packages [{"foo", "1.2.3-0ubuntu0"}, {"bar": "1.2.3-0ubuntu0"}]
    """
    packages = _query(f"packages_on_host/{__grains__['fqdn']}?format=json", cached)
    pkgs_list = []
    for pkg in packages:
        pkgs_list.append({str(pkg["name"]): str(pkg["version"])})
    return pkgs_list


def package(name=None, cached=False):
    """
    Return versions of specified packages
    """
    pkg = _get_pkgs_list(cached)
    if name:
        name = (name,) if isinstance(name, str) else name
        pkg = [p for p in pkg if list(p.keys())[0] in name]
    return pkg


def last_version(pkg, env="stable", cached=False):
    """
    Query for last version of specified package
    """
    return _get_pkgs_last_version(pkg, env, cached)


def groups2hosts(groups, datacenter=None, cached=True):
    """
    Expands group/groups name in to list of hosts belong this group/groups
    If datacenter defined - return hosts with the same datacenter
    """

    groups = (groups,) if isinstance(groups, str) else groups
    result = []
    for group in groups:
        hosts = _query(
            f"groups2hosts/{group}?format=json&"
            "fields=fqdn,root_datacenter_name,datacenter_name",
            cached,
        )
        for host in hosts:
            if (
                not datacenter
                or datacenter == host["datacenter_name"]
                or datacenter == host["root_datacenter_name"]
            ):
                result.append(str(host["fqdn"]))
    log.info(f"Got {len(result)} hosts in conductor's {groups=}")
    return result


def group2children(group, cached=True):
    """
    Return all children of group
    """
    result = []
    groups = _query("groups/{0}?format=json".format(group), cached)
    for group in groups[0]["children"]:
        result.append(str(group))
    log.info(f"Got {len(result)} children in conductor's {group=}")
    return result
