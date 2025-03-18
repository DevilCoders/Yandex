

import urllib2
import json
import time
from copy import deepcopy


CENV_PRODUCTION = "production"
CENV_TEST = "test"
CENV_DEVELOPMENT = "development"
CACHE_EXPIRATION_TIME = 120  # seconds

CONDUCTOR_URL = dict(
    production='http://c.yandex-team.ru',
    test='http://c.test.yandex-team.ru',
    development='http://markov.dev.yandex.net:3000'
)


class Conductor(object):
    def __init__(self, env, host_data):
        self.url = CONDUCTOR_URL[env]
        self._cache = {}
        if host_data:
            if isinstance(host_data, str):
                self._host_data = json.loads(open(host_data).read())
            else:
                self._host_data = host_data
        else:
            self._host_data = None

    def groups2hosts(self, groups):
        if type(groups) == str:
            groups = [groups]
        grouplist = ','.join(groups)

        cached = self.__getfromcache('groups2hosts', grouplist)
        if cached is not None:
            return cached

        path = '/api/groups2hosts/' + grouplist
        response = self.__getresponse(path)
        if response is None:
            return None
        data = response.split("\n")[:-1]
        self.__puttocache('groups2hosts', grouplist, data)
        return data

    def host2group(self, host):
        path = '/api/hosts/' + host + '?format=json'
        response = self.__getresponse(path)
        if response is None:
            return None
        data = response
        try:
            data = json.loads(data)
        except:
            return None
        return str(data[0]['group'])

    def host_info(self, fqdn):
        if not type(fqdn) == str:
            raise ValueError(
                'host_info handler accepts only string fqdn parameter')

        cached = self.__getfromcache('host_info', fqdn)
        if cached is not None:
            return cached

        path = '/api/host_info/' + fqdn
        if self._host_data:
            response = deepcopy(self._host_data[fqdn])
        else:
            response = self.__getresponse(path)
        if response is None:
            return None
        data = response
        if not self._host_data:
            try:
                data = json.loads(data)
            except:
                return None

        self.__puttocache('host_info', fqdn, data)
        if 'ip4' in data['addresses']:
            self.__puttocache(
                'ip_info', data['addresses']['ip4']['address'], data['addresses']['ip4'])
        if 'ip6' in data['addresses']:
            self.__puttocache(
                'ip_info', data['addresses']['ip6']['address'], data['addresses']['ip6'])
        return data

    def ip_info(self, ip):
        cached = self.__getfromcache('ip_info', ip)
        if cached:
            return cached

        path = '/api/ip_info/' + ip
        response = self.__getresponse(path)
        if not response:
            return None
        data = response
        try:
            data = json.loads(data)
        except ValueError:
            return None

        if 'ip4' in data['addresses']:
            self.__puttocache(
                'ip_info', data['addresses']['ip4']['address'], data['addresses']['ip4'])
            return data['addresses']['ip4']
        if 'ip6' in data['addresses']:
            self.__puttocache(
                'ip_info', data['addresses']['ip6']['address'], data['addresses']['ip6'])
            return data['addresses']['ip6']
        return None

    def __getfromcache(self, handler, key):
        key = handler + ':' + key
        if key in self._cache.keys():
            cache = self._cache[key]
            if time.time() - cache['timestamp'] < CACHE_EXPIRATION_TIME:
                return cache['data']
        return None

    def __puttocache(self, handler, key, value):
        key = handler + ':' + key
        self._cache[key] = dict(timestamp=time.time(), data=value)

    def __getresponse(self, path):
        try:
            return urllib2.urlopen(self.url + path).read()
        except urllib2.HTTPError as e:
            if e.code == 424:
                raise Exception(json.loads(e.read())['error'])
            return None
        except:
            return None
