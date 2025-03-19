#!/usr/bin/env python
# -*- coding: utf-8 -*-

__author__ = 'd3rp'

import logging
from urllib2 import urlopen, HTTPError
from json import dumps, loads
from socket import getfqdn, timeout
from salt.exceptions import CommandExecutionError

log = logging.getLogger(__name__)

# Define the module's virtual name
__virtualname__ = 'conductor'

def __virtual__():
    '''
    Confirm this module is on a Debian based system
    '''
    if __grains__.get('os_family', False) != 'Debian':
        return False
    return __virtualname__

def _query(url):
    error = BaseException("Unknown error")
    for attempt in xrange(3):
        try:
            result = urlopen(url, timeout = 5)
            return result.read()
        except HTTPError as exc:
            error = exc
        except timeout as exc:
            error = exc
    raise CommandExecutionError(error)

def _get_pkgs_last_version(pkg, env):
    '''
    Return version "1.2.3-0ubuntu0"
    '''
    log.info('Pkgname: {0}, environment: {1}'.format(pkg, env))
    envs = ('unstable', 'testing', 'prestable', 'stable')
    data = _query('http://c.yandex-team.ru/api/package_version/{0}'.format(pkg))
    try:
        version_list = loads(data)
        for i in range(0, len(envs)-1): # dev = testing, prestable = stable
            if not version_list[pkg][envs[i]]['version']:
                version_list[pkg][envs[i]]['version'] = version_list[pkg][envs[i+1]]['version']
        log.info('Conductor response: {0}'.format(data))
        return version_list[pkg][env]['version']
    except ValueError as exc:
        raise CommandExecutionError(exc)
        return None

def _get_pkgs_list():
    '''
    Return list of packages [{"foo", "1.2.3-0ubuntu0"}, {"bar": "1.2.3-0ubuntu0"}]
    '''
    data = _query('http://c.yandex-team.ru/api-cached/packages_on_host/{0}?format=json'.format(getfqdn()))
    try:
        pkgs_list = []
        for p in loads(data):
            pkgs_list.append({str(p['name']): str(p['version'])})
    except ValueError as exc:
        raise CommandExecutionError(exc)
    return pkgs_list

def package(name=None):
    package = _get_pkgs_list()
    if name:
        if type(name) == str:
            name = (name, )
        package = [ p for p in package if p.keys()[0] in name ]
    return package

def last_version(pkg, env='stable'):
    if pkg:
        return _get_pkgs_last_version(pkg, env)
