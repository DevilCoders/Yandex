# -*- coding: utf-8 -*-
"""
State for declarative managing conda packages
"""

import logging


def __virtual__():
    return 'conda'

def env_present(name, packages, cleanup=True):
    """
    Ensure that conda environment is present and packages installed.
    """
    ret = {
        'name': name,
        'changes': {},
        'result': True,
        'comment': 'Nothing to do'
    }
    dry_run = __opts__['test']
    has_changes, changes = \
        __salt__['conda.environment_present'](
            name,
            packages,
            cleanup=cleanup,
            dry_run=dry_run)
    if has_changes:
        if dry_run:
            ret['result'] = None
        ret['changes']['diff'] = changes
        ret['comment'] = \
            'Conda packages is set to be installed'
    return ret

def pkg_present(name, packages, cleanup=True, env=None):
    """
    Ensures that packages are present and have right versions.
    """
    ret = {
        'name': name,
        'changes': {},
        'result': True,
        'comment': 'Nothing to install'
    }
    dry_run = __opts__['test']
    has_changes, changes = \
        __salt__['conda.packages_present'](
            packages,
            env,
            cleanup=cleanup,
            dry_run=dry_run)
    if has_changes:
        if dry_run:
            ret['result'] = None
        ret['changes']['diff'] = changes
        ret['comment'] = \
            'Conda packages is set to be installed'
    return ret
    
def pkg_absent(name, packages, env=None):
    """
    Ensures that packages are absent.
    """
    ret = {
        'name': name,
        'changes': {},
        'result': True,
        'comment': 'Nothing to uninstall'
    }
    dry_run = __opts__['test']
    has_changes, changes = \
        __salt__['conda.packages_absent'](
            packages,
            env,
            dry_run=dry_run)
    if has_changes:
        if dry_run:
            ret['result'] = None
        ret['changes']['diff'] = changes
        ret['comment'] = \
            'Conda packages is set to be uninstalled'
    return ret
