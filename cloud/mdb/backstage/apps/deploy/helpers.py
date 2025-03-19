import jsonschema


VALID_SALT_RESULT_SCHEMA = {
    'type': 'object',
    'properties': {
        'fun': {'type': 'string'},
        'fun_args': {'type': 'array'},
        'id': {'type': 'string'},
        'jid': {'type': 'string'},
        'retcode': {'type': 'number'},
        'return': {'type': ['number', 'string', 'boolean', 'object', 'array', 'null']},
        'success': {'type': 'boolean'}
    },
    'required': [
        'fun',
        'fun_args',
        'id',
        'jid',
        'retcode',
        'return',
        'success',
    ],
    'additionalProperties': False,
}


class SaltResult:
    def __init__(self, raw, states=None, parsed=True, error=None):
        self.raw = raw
        self.states = states or []
        self.parsed = parsed
        self.error = error
        self.states_count = {
            'error': 0,
            'ok': 0,
            'changed': 0,
        }
        for state in self.states:
            if not state['result']:
                self.states_count['error'] += 1
            if state['has_changes']:
                self.states_count['changed'] += 1
            else:
                self.states_count['ok'] += 1


def parse_salt_result(result):
    if not result:
        return SaltResult(
            raw=result,
            parsed=False,
            error='there is no result given'
        )

    if result['fun'] not in {"state.highstate", "state.sls"}:
        try:
            jsonschema.validate(result, VALID_SALT_RESULT_SCHEMA)
        except jsonschema.exceptions.ValidationError as err:
            return SaltResult(
                raw=result,
                parsed=False,
                error=f'invalid salt result schema: {err}'
            )
        else:
            state = {'result': result['success'], 'has_changes': False}
            return SaltResult(
                raw=result,
                parsed=False,
                states=[state],
            )

    if 'return' not in result:
        return SaltResult(
            raw=result,
            parsed=False,
            error='No return in result',
        )

    if not isinstance(result['return'], dict):
        return SaltResult(
            raw=result,
            parsed=False,
            error='Invalid return["result"] type (dict expected)',
        )

    states = []
    for key, value in result['return'].items():
        if not isinstance(value, dict):
            return SaltResult(
                states=states,
                raw=result,
                parsed=False,
                error='Invalid type of value (dict expected): {value}',
            )

        if 'pchanges' in value:
            if value['pchanges']:
                if not value['changes']:
                    value['changes'] = value.pop('pchanges')
                else:
                    return SaltResult(
                        states=states,
                        raw=result,
                        parsed=False,
                        error='changes+pchanges? wut?: {value}',
                    )

        info = {}
        spl = key.split('_|-')
        info['state'] = f'{spl[0]}.{spl[-1]}'
        info['has_diff'] = False
        info['changes'] = None
        info['extra_diff'] = None
        info['id'] = value.pop('__id__', None)
        info['name'] = value.pop('name', None)
        info['sls'] = value.pop('__sls__', None)
        info['duration'] = value.pop('duration', None)
        info['start_time'] = value.pop('start_time', None)
        info['result'] = value.pop('result')
        info['state_ran'] = value.pop('__state_ran__', True)
        info['comment'] = value.pop('comment')
        info['run_num'] = value.pop('__run_num__')
        info['warnings'] = value.pop('warnings', [])
        info['priority'] = value.pop('priority', None)

        if not isinstance(info['comment'], str):
            info['comment'] = ''

        value.pop('link', None)
        value.pop('path', None)

        if not value['changes']:
            info['has_changes'] = False
        else:
            info['has_changes'] = True

        if info['has_changes']:
            if 'diff' in value['changes']:
                info['has_diff'] = True

            if not info['has_diff']:
                info['changes'] = value.pop('changes')
            else:
                diff = value['changes'].pop('diff')
                info['diff'] = diff
                if value['changes']:
                    info['changes'] = value.pop('changes')
                else:
                    del value['changes']

            if value:
                return SaltResult(
                    states=states,
                    raw=result,
                    parsed=False,
                    error=f'Fields left in value: {value}'
                )

        if not info['result'] and 'One or more requisite failed' in info['comment']:
            info['requisite_failed'] = True
        else:
            info['requisite_failed'] = False

        info['possible_changes'] = False
        if info['has_changes']:
            info['possible_changes'] = True

        if not info['result'] and not info['requisite_failed']:
            info['possible_changes'] = True

        if 'Traceback (most recent call last)' in info['comment']:
            info['comment_is_traceback'] = True
        else:
            info['comment_is_traceback'] = False

        states.append(info)
    return SaltResult(raw=result, states=states)
