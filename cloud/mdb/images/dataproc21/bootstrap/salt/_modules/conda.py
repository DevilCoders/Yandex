# -*- coding: utf-8 -*-
"""
Module to provide methods for working with pillar through metadata
:platform: all
"""

from __future__ import absolute_import

import os
import json
import logging

log = logging.getLogger(__name__)

__virtualname__ = 'conda'

CONDA_PATH = '/opt/conda'
CONDA_BIN = '{path}/bin/conda'.format(path=CONDA_PATH)


def __virtual__():
    """
    Determine whether or not to load this module
    """
    if not os.path.exists(CONDA_BIN):
        return False
    return __virtualname__


class CondaException(Exception):
    """
    Conda Exception
    """


def execute(cmd):
    return __salt__['cmd.run'](cmd)


def package_list(env=None):
    command = '{bin} list'.format(bin=CONDA_BIN)
    if env:
        command += ' -n {env}'.format(env=env)
    command += ' --json'
    return json.loads(execute(command))


def conda_info():
    command = '{bin} info --json'.format(bin=CONDA_BIN)
    return json.loads(execute(command))


def conda_clean():
    command = '{bin} clean --all --yes --json'.format(bin=CONDA_BIN)
    ret = json.loads(execute(command))
    if 'success' not in ret or not ret['success']:
        raise CondaException(ret['error'])


def packages_uninstall(packages, prune=False, env=None):
    packs = []
    if not packages:
        return
    for package, version in packages.items():
        if version == 'any':
            packs.append(package)
        else:
            packs.append('{}={}'.format(package, version))
    command = '{bin} uninstall'.format(bin=CONDA_BIN)
    if prune:
        command += ' --prune'
    if env:
        command += ' -n {env}'.format(env=env)
    command += ' ' + ' '.join(packs)
    command += ' --yes --quiet --json'
    ret = json.loads(execute(command))
    if 'success' not in ret or not ret['success']:
        raise CondaException(ret['error'])


def conda_package_version(package, version):
    """
    Return a string of pacakge with version for conda install command
    https://docs.conda.io/projects/conda/en/latest/user-guide/concepts/pkg-specs.html#package-match-specifications
    """
    if version in ('any', '', None):
        # package without version
        # conda install numpy
        return str(package)
    if not {'=', '|', '>', '<', '!', ','} & set(version):
        # Version without expression, use fuzzy
        # The fuzzy constraint numpy=1.11 matches 1.11, 1.11.0, 1.11.1, 1.11.2, 1.11.18, and so on.
        return f'{package}={version}'
    # Otherwise use complex version condition
    # conda install "numpy>1.11"
    # conda install "numpy=1.11.1|1.11.3"
    # conda install "numpy>=1.8,<2"
    return f'"{package}{version}"'


def packages_install(packages, env=None):
    packs = []
    if not packages:
        return
    packs = [conda_package_version(package, version) for package, version in packages.items()]
    command = '{bin} install'.format(bin=CONDA_BIN)
    if env:
        command += ' -n {env}'.format(env=env)
    command += ' ' + ' '.join(packs)
    command += ' --yes --quiet --json'
    ret = json.loads(execute(command))
    if 'success' not in ret or not ret['success']:
        raise CondaException(ret['error'])
    return ret


def package_is_present(installed, package, version):
    for pack in installed:
        if package == pack['name']:
            if version == 'any' or str(version) == str(pack['version']):
                return True
    return False


def packages_present(packages, env=None, cleanup=True, dry_run=False):
    """
    Function ensure that required packages installed
    """
    has_changes = False
    changes = '\n'
    if not packages:
        return (has_changes, changes)

    installed_packages = package_list(env)
    packages_to_install = {}
    for package, version in packages.items():
        if not package_is_present(installed_packages, package, version):
            packages_to_install[package] = version
            has_changes = True
    if packages_to_install:
        changes = 'Packages to install:\n'
    changes += '\n'.join(['+ {} == {}'.format(k, v) for k, v in packages_to_install.items()])
    if not dry_run:
        ret = packages_install(packages_to_install, env)
        if ret is None or ret.get('message') == 'All requested packages already installed.':
            has_changes = False
            changes = '\n'
        else:
            if cleanup:
                conda_clean()
    return (has_changes, changes)


def packages_absent(packages, env=None, prune=False, dry_run=False):
    """
    Function ensure that specified packages is absent
    """
    has_changes = False
    changes = '\n'
    if not packages:
        return (has_changes, changes)

    installed_packages = package_list(env)
    packages_to_uninstall = {}
    for package, version in packages.items():
        if package_is_present(installed_packages, package, version):
            packages_to_uninstall[package] = version
            has_changes = True
    if packages_to_uninstall:
        changes = 'Packages to uninstall:\n'
    changes += '\n'.join(['- {} == {}'.format(k, v) for k, v in packages_to_uninstall.items()])
    if not dry_run:
        packages_uninstall(packages_to_uninstall, prune, env)
    return (has_changes, changes)


def environment_create(environment):
    command = '{bin} create -n {env} --yes --json'.format(bin=CONDA_BIN, env=environment)
    ret = json.loads(execute(command))
    if 'success' not in ret or not ret['success']:
        raise CondaException(ret['error'])
    return ret


def environment_present(env, packages={}, cleanup=True, dry_run=False):
    """
    Ensure that environment exists with right packages
    """
    has_changes = False
    changes = '\n'
    if not packages:
        return (has_changes, changes)

    # Check existing of environment
    info = conda_info()
    envs = [env_path.split('/')[-1] for env_path in info['envs']]
    env_exists = True
    if env not in envs:
        # create environment
        has_changes = True
        changes = 'New environment: {}\n'.format(env)
        if not dry_run:
            environment_create(env)
        else:
            env_exists = False
    installed_packages = []
    if env_exists:
        installed_packages = package_list(env)
    if packages:
        packets_has_changes, packages_changes = packages_present(packages, env)
        has_changes = has_changes or packets_has_changes
        if packets_has_changes:
            changes += packages_changes
    return (has_changes, changes)
