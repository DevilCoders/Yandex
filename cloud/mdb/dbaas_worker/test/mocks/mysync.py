"""
Simple mysync util mock (to simplify switchover tests)
"""

import re
import json
from .zookeeper import _rec_set, _rec_delete, _rec_create


def mysync(mocker, _):
    """
    Setup mysync mock
    """
    getfqdn = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.mysync.socket.getfqdn')
    getfqdn.return_value = 'test-fqdn'
    get_absolute_now = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.mysync.get_absolute_now')
    get_absolute_now.return_value.isoformat.return_value = 'test-timestamp'
    parser = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.mysync.dateutil.parser')
    parser.parse.side_effect = lambda x: x


def mysync_react_maintenance(root_dict, operation, path, data):
    """
    Fake mysync daemon reactions to maintenance commands
    """
    if re.match(r'/?mysql/[^/]+/maintenance', path):
        if operation == 'create':
            data = json.loads(data.decode('utf-8'))
            data['mysync_paused'] = True
            data = json.dumps(data).encode('utf-8')
            _rec_set(root_dict, path, data)
        elif operation == 'set':
            data = json.loads(data.decode('utf-8'))
            if data.get('should_leave'):
                _rec_delete(root_dict, path)


def mysync_react_switchover(root_dict, operation, path, data):
    """
    Fake mysync daemon reactions to switchover command
    """
    if re.match(r'/?mysql/[^/]+/switch', path):
        if operation == 'create':
            data = json.loads(data.decode('utf-8'))
            data['result'] = {'ok': True}
            data = json.dumps(data).encode('utf-8')
            _rec_create(root_dict, re.sub('/switch', '/last_switch', path), data)
            _rec_delete(root_dict, path)
