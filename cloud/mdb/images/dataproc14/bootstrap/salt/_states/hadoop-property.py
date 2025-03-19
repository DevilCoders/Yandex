# -*- coding: utf-8 -*-
"""
State for declarative managing *-site.xml properties
"""

import logging


def __virtual__():
    return 'hadoop-property'


def present(name, config_path, properties):
    """
    Ensures that properties are present and have right values.
    """
    ret = {
        'name': name,
        'changes': {},
        'result': True,
        'comment': 'Nothing to change in file {path}'.format(path=config_path)
    }
    dry_run = __opts__['test']
    has_changes, changes = \
        __salt__['hadoop-property.config_site_properties_present'](
            config_path,
            properties,
            dry_run=dry_run)
    if has_changes:
        if dry_run:
            ret['result'] = None
        ret['changes']['diff'] = changes
        ret['comment'] = \
            'The file {path} is set to be changed'.format(path=config_path)
    return ret
