#!/usr/bin/env python
from six.moves import http_client as httplib
import json
import os
import socket
import time

_FALLBACK_DC_MAP = {
    'i': 'man',
    'e': 'iva',
    'f': 'myt',
    'h': 'sas',
    'k': 'vla',
}


def _conductor_host():
    return __opts__.get('conductor_host', 'c.yandex-team.ru')


def ya():
    try:
        hostname = socket.getfqdn()
        data = {
            'conductor': [],
            'groups': [],
        }
        cache_time = 0
        cache_data = {}
        if os.path.exists('/tmp/.grains_conductor.cache'):
            try:
                with open('/tmp/.grains_conductor.cache') as cache:
                    cache_data = json.load(cache)
                cache_time = os.path.getmtime('/tmp/.grains_conductor.cache')
            except Exception:
                pass

        if time.time() - cache_time < 30 or (time.time() - cache_time < 600 and cache_data.get('conductor')):
            return {'ya': cache_data}
        try:
            conn = httplib.HTTPConnection(_conductor_host(), timeout=1)
            conn.request('GET', '/api/hosts2groups/{host}'.format(host=hostname))
            resp = conn.getresponse()
            if resp.status == 200:
                for i in resp.read().split():
                    i = i.decode()
                    data['conductor'].append(i)
                    data['groups'].append('conductor.' + i)
            conn.close()

            conn = httplib.HTTPConnection(_conductor_host(), timeout=1)
            conn.request('GET', '/api/hosts/{host}?format=json'.format(host=hostname))
            resp = conn.getresponse()
            if resp.status == 200:
                resp_json = json.loads(resp.read())
                data['group'] = resp_json[0]['group']
                data['short_dc'] = resp_json[0]['root_datacenter']
            conn.close()
        except Exception:
            pass

        for i in data:
            if cache_data.get(i) and not data.get(i):
                data[i] = cache_data[i]
        if not data.get('short_dc'):
            if hostname[:3] in _FALLBACK_DC_MAP.values():
                data['short_dc'] = hostname[:3]
            elif not hostname.startswith('rc1'):
                last_letter = hostname.split('.')[0][-1]
                if last_letter in _FALLBACK_DC_MAP:
                    data['short_dc'] = _FALLBACK_DC_MAP[last_letter]
        if not data.get('group') and data['conductor']:
            data['group'] = data['conductor'][-1]
        try:
            with open('/tmp/.grains_conductor.cache', 'w') as cache:
                cache.write(json.dumps(data))
        except Exception:
            pass

        return {'ya': data}
    except Exception:
        return {}


if __name__ == '__main__':
    from pprint import pprint
    pprint(ya())
