import urllib2
import json
import logging
import socket

log = logging.getLogger(__name__)

def _query(url):
    for attempt in xrange(3):
        try:
            log.debug('fetching {0}'.format(url))
            response = urllib2.urlopen(url, timeout = 5).read()
            log.debug('response {0}'.format(response))
            return response
        except:
            log.debug('Failed fetch info from {0}'.format(url))
            return None

def _get_json(data):
    try:
        result = json.loads(data)
    except Exception as e:
        log.error('Failed to parse result "{0}": {1}'.format(data, e))
        result = {}
    return result

def _get_conductor_data(url):
    data = _query(url)
    if data is None:
        return None
    else:
        return _get_json(data)

def conductor_info():
    hostinfo = _get_conductor_data('http://c.yandex-team.ru/api-cached/hosts/{fqdn}/?format={format}'.format(format='json', fqdn=socket.getfqdn()))
    if hostinfo is None:
        log.warning('Host is not configured in conductor.')
        return

    info = hostinfo[0]
    tags = _get_conductor_data('http://c.yandex-team.ru/api-cached/get_host_tags/{fqdn}/?format={format}'.format(format='json', fqdn=socket.getfqdn()))
    groups = _get_conductor_data('http://c.yandex-team.ru/api-cached/hosts2groups/{fqdn}/?format={format}'.format(format='json', fqdn=socket.getfqdn()))
    project = _get_conductor_data('http://c.yandex-team.ru/api-cached/hosts2projects/{fqdn}/?format={format}'.format(format='json', fqdn=socket.getfqdn()))

    if tags:
        info['tags'] = tags
    if groups:
        info['groups'] = [x['name'] for x in groups]
    if project:
        info['project'] = project[0]['name']

    c = []
    for group in groups:
        c.append(format(group['name']))
        c.append("{0}@{1}".format(group['name'], info['root_datacenter']))

    result = {'conductor': info, 'c': c }
    return result
