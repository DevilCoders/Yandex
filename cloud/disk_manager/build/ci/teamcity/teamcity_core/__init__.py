#!/usr/bin/env python
# -*- coding: utf-8 -*-

import shutil
from os.path import abspath, join, dirname
from os import environ
from subprocess import check_call


def teamcity_core_svn():
    revision = environ.get("BUILD_VCS_NUMBER", None)
    branch_path = environ.get("branch_path", None)
    assert revision is not None
    assert branch_path is not None

    return "svn+ssh://arcadia.yandex.ru/arc/{branch_path}/arcadia/kikimr/library/ci/teamcity_core/core@{revision}".format(
        branch_path=branch_path, revision=revision
    )


def setup():
    teamcity_core_svn_path = teamcity_core_svn()
    teamcity_core_local_path = abspath(join(dirname(__file__), "core"))

    shutil.rmtree(teamcity_core_local_path, ignore_errors=True)

    check_call([
        "svn", "export", "-q", teamcity_core_svn_path, teamcity_core_local_path, "--force", "--non-interactive"
    ])


setup()
