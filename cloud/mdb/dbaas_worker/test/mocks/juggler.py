"""
Simple juggler mock
"""

import json
import time

from httmock import urlmatch

from .utils import handle_http_action, http_return


def juggler(state):
    """
    Setup mock with state
    """

    @urlmatch(netloc='juggler.test', method='post', path='/v2/downtimes/set_downtimes')
    def set_downtime(_, request):
        data = json.loads(request.body)
        fqdn = data['filters'][0]['host']
        ret = handle_http_action(state, f'juggler-set-downtime-{fqdn}')
        if ret:
            return ret

        if 'downtime_id' in data and fqdn not in state['juggler']:
            return http_return(code=404, body={'error': 'No such downtime'})

        data['downtime_id'] = f'downtime-{fqdn}'
        data['filters'][0].update(
            {
                'instance': '',
                'namespace': '',
                'service': '',
                'tags': [],
            }
        )
        state['juggler'][fqdn] = data

        return http_return()

    @urlmatch(netloc='juggler.test', method='post', path='/v2/downtimes/remove_downtimes')
    def remove_downtime(_, request):
        data = json.loads(request.body)
        ids = data['downtime_ids']

        fqdns = []
        for fqdn in state['juggler']:
            if state['juggler'][fqdn]['downtime_id'] in ids:
                fqdns.append(fqdn)
                ret = handle_http_action(state, f'juggler-remove-downtime-{fqdn}')
                if ret:
                    return ret

        if not fqdns:
            return http_return(code=404, body={'error': 'No such downtime'})

        for fqdn in fqdns:
            del state['juggler'][fqdn]

        return http_return()

    @urlmatch(netloc='juggler.test', method='post', path='/v2/downtimes/get_downtimes')
    def get_downtime(_, request):
        data = json.loads(request.body)
        fqdn = data['filters'][0]['host']
        ret = handle_http_action(state, f'juggler-get-downtime-{fqdn}')
        if ret:
            return ret

        body = {'items': []}

        if fqdn in state['juggler']:
            body['items'].append(state['juggler'][fqdn])

        return http_return(body=body)

    @urlmatch(netloc='juggler.test', method='post', path='/v2/events/get_raw_events')
    def get_raw_events(_, request):
        data = json.loads(request.body)
        fqdn = data['filters'][0]['host']
        service_name = data['filters'][0]['service']
        ret = handle_http_action(state, f'juggler-get-raw-event-{fqdn}')
        if ret:
            return ret

        status = state['juggler'].get(fqdn, {}).get('status', 'OK')

        return http_return(
            body={
                'items': [
                    {
                        'description': 'mocked',
                        'host': fqdn,
                        'instance': '',
                        'received_time': time.time(),
                        'service': service_name,
                        'status': status,
                        'tags': [],
                    }
                ],
            }
        )

    return (set_downtime, get_downtime, get_raw_events, remove_downtime)
