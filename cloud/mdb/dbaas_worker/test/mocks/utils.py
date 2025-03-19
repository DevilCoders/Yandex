"""
Mocks helpers
"""

import json

from cloud.mdb.dbaas_worker.internal.exceptions import Interrupted


def http_return(code=200, body=''):
    """
    Http return helper to reduce mocks boilerplate
    """
    if isinstance(body, str):
        return {
            'status_code': code,
            'content': body,
        }

    return {
        'status_code': code,
        'content': json.dumps(body),
        'headers': {'content-type': 'application/json'},
    }


def handle_http_action(state, action_id):
    """
    Handle actions for http mocks
    """
    if action_id in state['fail_actions']:
        return http_return(code=500, body=f'Fail action {action_id}')
    if action_id in state['cancel_actions']:
        raise Interrupted(f'Cancel action {action_id}')
    if action_id not in state['set_actions']:
        state['actions'].append(action_id)
        state['set_actions'].add(action_id)

    return None


def handle_action(state, action_id):
    """
    Handle actions for non-http mocks
    """
    # if action_id == 'mlock-release-dbaas-worker-cid-test':
    #     raise Exception('here')
    if action_id in state['fail_actions']:
        raise RuntimeError(f'Fail action {action_id}')
    if action_id in state['cancel_actions']:
        raise Interrupted(f'Cancel action {action_id}')
    if action_id not in state['set_actions']:
        state['actions'].append(action_id)
        state['set_actions'].add(action_id)
