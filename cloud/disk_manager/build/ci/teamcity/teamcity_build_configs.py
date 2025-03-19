#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json

from lib.sandbox_resource import get_packages

from teamcity_core.core.util import run_ya_package_task, teamcity_context, production_config

if __name__ == "__main__":
    with teamcity_context():
        packages = production_config()["configs"]
        task_info = run_ya_package_task(
            packages,
            repo_name=None,
            sandbox_params={
                "publish_to": "yandex-cloud",
                "debian_distribution": "stable",
            },
            validate_arcadia_url=False,
            ram_requirements_mb=10000,
            execution_space_mb=35000,
            use_arcadia_api_fuse=True
        )

        built_packages = get_packages(task_info['task_id'])
        if len(packages) != len(built_packages):
            raise Exception("Not all packages were built. Requested {}. Got {}".format(packages, built_packages))

        with open('packages.json', 'w') as out:
            json.dump(built_packages, out)
