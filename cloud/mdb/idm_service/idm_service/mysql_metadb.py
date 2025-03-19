import json
import uuid
from functools import partial

from .tasks_version import TASKS_VERSION

json_dump = partial(json.dumps, ensure_ascii=False)


def get_mysql_user_modify_task_params(cid, pillar, folder_id, rev, login=None):
    task_id = uuid.uuid4().hex
    params = {
        'task_type': 'mysql_user_modify',
        'operation_type': 'mysql_user_modify',
        'metadata': json_dump({'user_name': login or 'IDM'}),
        'task_args': json_dump({'target-user': login, 'zk_hosts': pillar.zk_hosts}),
        'created_by': 'idm_service',
        'task_id': task_id,
        'cid': cid,
        'rev': rev,
        'folder_id': folder_id,
        'version': TASKS_VERSION,
    }

    return task_id, params
