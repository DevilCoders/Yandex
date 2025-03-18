# coding: utf-8
from __future__ import unicode_literals

import os

from fabric.api import local, task, env, settings, hide
from fabric.utils import abort

from robe import venv as robe_venv

env.robe.projects = {'ids': {'path': '.'}}
env.robe.venv.dev_requirements_file = 'requirements.test.txt'
env.robe.venv.quiet_install = False

TEAMCITY_ENV_PROJECT_NAME = "TEAMCITY_PROJECT_NAME"

services = [
    'abc',
    'at',
    'calendar',
    'formatter',
    'gap',
    'jira',
    'orange',
    'plan',
    'staff',
    'startrek',
    'startrek2',
]


def _is_running_under_teamcity():
    return os.getenv(TEAMCITY_ENV_PROJECT_NAME) is not None


@task()
def enter(cmd):
    with robe_venv.activate():
        local(cmd)


@task()
def venv(*args, **kwargs):
    robe_venv.create(*args, **kwargs)
    with robe_venv.activate():
        with hide('running'):
            wheels_dir = local(
                'python -c "from pip.utils import appdirs;'
                'print appdirs.user_cache_dir(\'wheels\')";',
                capture=True
            ).strip()
        local(' '.join([
            'pip wheel',
            '-e '
            '".[%s]"' % ','.join(services),
            '-i ' + env.robe.venv.pypi_url,
            '-w "%s"' % wheels_dir,
            '-f "%s"' % wheels_dir,
        ]))
        local(' '.join([
            'pip',
            'install',
            '--disable-pip-version-check',
            '-e '
            '".[%s]"' % ','.join(services),
            '-i ' + env.robe.venv.pypi_url,
            '--use-wheel',
            '--no-index',
            '-f "%s"' % wheels_dir,
        ]))


@task()
def test(scope=None, mode=None):
    """
    Run tests.

    mode is one of:
      unit: run only unit tests (no integration tests)
      jira: run JIRA integration tests
      startrek: run startrek integration tests
      all: run all tests
    """

    opts = ' --ignore *.txt'
    is_teamcity = _is_running_under_teamcity()

    if not mode:
        if is_teamcity:
            mode = 'unit'
        else:
            mode = 'all'

        if scope == 'jira':
            mode = 'integration'
        elif scope == 'startrek':
            mode = 'integration'

    if mode == 'integration':
        opts += ' -m "integration"'
    if mode == 'unit':
        opts += ' -m "not integration"'

    if is_teamcity:
        opts += " --teamcity --verbose"

    with robe_venv.activate():
        with settings(warn_only=True):
            succeeded = local('py.test' + opts).succeeded

    if not succeeded:
        abort('Tests failed')
