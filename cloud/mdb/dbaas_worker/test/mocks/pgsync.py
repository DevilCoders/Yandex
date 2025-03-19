"""
Simple pgsync util mock (to simplify switchover tests)
"""

import re
from .zookeeper import _rec_set, _rec_create


def pgsync_react_switchover(root_dict, operation, path, data):
    """
    Fake pgsync daemon reactions to switchover commands
    """
    if re.match(r'/?pgsync/[^/]+/switchover/state', path):
        if operation == 'set':
            if data == 'scheduled'.encode('utf-8'):
                data = 'finished'.encode('utf-8')
                _rec_set(root_dict, path, data)


def pgsync_react_delete_timeline(root_dict, operation, path, _):
    """
    Recreate timelien during postgres upgrade
    """
    if re.match(r'/?pgsync/[^/]+/timeline', path):
        if operation == 'delete':
            _rec_create(root_dict, path, '1'.encode('utf-8'))
