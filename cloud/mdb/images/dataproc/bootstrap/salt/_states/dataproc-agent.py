# -*- coding: utf-8 -*-
"""
States for working with dataproc
"""

import time
import logging

LOG = logging.getLogger(__name__)


def __virtual__():
    return 'dataproc-agent'


def updated(name):
    """
    Ensure that dataproc-agent has actual version
    """
    ret = {
        'name': name,
        'changes': {},
        'result': True,
        'comment': 'Agent already have recent verion'
    }
    dry_run = __opts__['test']
    has_changes, changes = __salt__['dataproc-agent.update'](dry_run=dry_run)
    if has_changes:
        ret['changes'] = {'diff': changes}
        if dry_run:
            ret['result'] = None
        ret['comment'] = changes
    return ret
