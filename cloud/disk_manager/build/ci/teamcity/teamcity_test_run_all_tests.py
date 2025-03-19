#!/usr/bin/env python
# -*- coding: utf-8 -*-

from teamcity_core.core.util import teamcity_context, run_ya_make_task


if __name__ == "__main__":
    with teamcity_context():
        run_ya_make_task(
            "cloud/disk_manager/internal/pkg",
            ram_requirements_mb=10000,
            execution_space_mb=35000,
            owner="STORAGE-DEV",
            sandbox_params={
                "test_tag": "-ya:force_sandbox",
                "kill_timeout": 3600 * 3,
            },
            use_arcadia_api_fuse=True,
            on_distbuild=True,
        )
