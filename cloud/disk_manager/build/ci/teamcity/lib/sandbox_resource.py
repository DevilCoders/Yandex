#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

from teamcity_core.core.sandbox import Sandbox
from teamcity_core.core.launcher import TeamCityStdOutWriter


def get_task_resources(task_id):
    server = 'https://sandbox.yandex-team.ru/'
    sandbox = Sandbox(server, os.environ.get('sandbox_oauth', None))
    resources = sandbox.GetTaskResources(task_id)
    for r in resources:
        print("Found sandbox task resource: {}".format(r))
    return resources


def get_packages(task_id):
    packages = {}
    resources = get_task_resources(task_id)
    for r in resources:
        if r.get('type_name', '') == 'YA_PACKAGE':
            attrs = r.get('attrs', dict())
            name = attrs.get('resource_name', None)
            ver = attrs.get('resource_version', None)
            if name is None or ver is None:
                continue
            packages[name] = ver
    return packages


def get_package_version(task_id):
    reporter = TeamCityStdOutWriter("Sandbox Gateway")
    resources = get_task_resources(task_id)
    version = None
    for r in resources:
        if r.get('type_name', '') == 'YA_PACKAGE':
            attrs = r.get('attrs', dict())
            ver = attrs.get('resource_version', None)
            if ver is None:
                continue
            if version is not None and version != ver:
                reporter.Error("resources with different versions: ({},{})".format(version, ver))
                raise Exception("Found resources with different versions ({},{})".format(version, ver))
            version = ver
    if version is None:
        raise Exception("No resource versions found for task {}".format(task_id))
    return version
