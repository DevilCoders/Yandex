# -*- coding: utf-8 -*-
"""
State for declarative managing *-site.xml properties
"""

import logging


def __virtual__():
    return 'zipfile'


def objects_present(name, archive_path, content):
    """
    Ensures that archive has objects with expected content.
    """
    ret = {
        'name': name,
        'changes': {},
        'result': True,
        'comment': 'Nothing to change in arhive {path}'.format(path=archive_path)
    }
    dry_run = __opts__['test']
    has_changes, changes = \
        __salt__['zipfile.objects_present'](
            archive_path,
            content,
            dry_run=dry_run)
    if has_changes:
        if dry_run:
            ret['result'] = None
        ret['changes']['diff'] = changes
        ret['comment'] = \
            'The archive {path} is set to be changed'.format(path=archive_path)
    return ret
