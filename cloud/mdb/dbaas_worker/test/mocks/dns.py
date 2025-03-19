"""
Simple dns-api mock
"""

import json

from httmock import urlmatch

from .utils import handle_http_action, http_return


def dns(state):
    """
    Setup mock with state
    """

    @urlmatch(netloc='dns.test', method='put', path='/v2.3/robot-dnsapi-mdb/primitives')
    def primitives(_, request):
        data = json.loads(request.body)['primitives'][0]
        ret = handle_http_action(state, f'dns-{data["operation"]}-{data["name"]}-{data["type"]}')
        if ret:
            return ret

        key = f'{data["name"]}-{data["type"]}'

        if data['operation'] == 'add':
            state['dns'][key] = data['data']
        elif data['operation'] == 'delete':
            if key in state['dns']:
                del state['dns'][key]

        return http_return()

    return (primitives,)
