import json
import uuid
from functools import partial

from .tasks_version import TASKS_VERSION

json_dump = partial(json.dumps, ensure_ascii=False)


def get_pg_user_modify_task_params(cid, folder_id, rev, login=None):
    task_id = uuid.uuid4().hex
    params = {
        'task_type': 'postgresql_user_modify',
        'operation_type': 'postgresql_user_modify',
        'metadata': json_dump({'user_name': login or 'IDM'}),
        'task_args': json_dump({'target-user': login}),
        'created_by': 'idm_service',
        'task_id': task_id,
        'cid': cid,
        'rev': rev,
        'folder_id': folder_id,
        'version': TASKS_VERSION,
    }

    return task_id, params
