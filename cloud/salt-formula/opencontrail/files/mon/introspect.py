#!/usr/bin/env python
# -*-coding: utf-8 -*-
# vim: sw=4 ts=4 expandtab ai

"""Monitoring for services in contrail"""

from __future__ import print_function
import argparse
import fnmatch
import os
import requests
import sys
import yaml
from lxml import etree

from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

DEFAULT_HOST = '127.0.0.1'
DEFAULT_MAX_RETRIES = 3
DEFAULT_TIMEOUT = 0.5  # wait 0.5 1.0 2.0 seconds
DEFAULT_REMOTE_TIMEOUT = 3.0  # wait 3.0 6.0 12.0 seconds
CONFIG_DIRS = '/home/monitor/agents/etc/'

# Introspection HTTP API ports, taken from:
# https://github.yandex-team.ru/YandexCloud/contrail-yandex/blob/da1422459284ac4064b7e2aa5fc45907e8be3efd/src/controller/src/sandesh/common/vns.sandesh#L110

SUPPORTED_SERVICES = {
    "contrail-api": 8084,
    "contrail-collector": 8089,
    "contrail-control": 8083,
    "contrail-discovery": 5997,
    "contrail-schema": 8087,
    "contrail-vrouter-agent": 8085,
}

# Status codes for monitoring (monrun)

STATUS_OK = 0
STATUS_WARN = 1
STATUS_CRIT = 2

ESC_RED = '\033[91m'
ESC_GREEN = '\033[92m'
ESC_YELLOW = '\033[93m'
ESC_MAGENTA = '\033[95m'
ESC_END = '\033[0m'


def _colorize(color, message):
    if not sys.stdout.isatty():
        return message
    else:
        return '{}{}{}'.format(color, message, ESC_END)


def red(message):
    return _colorize(ESC_RED, message)


def green(message):
    return _colorize(ESC_GREEN, message)


def yellow(message):
    return _colorize(ESC_YELLOW, message)


def magenta(message):
    return _colorize(ESC_MAGENTA, message)


# Note: this code region was intentionally copied from /usr/bin/contrail-status
#       ('contrail-utils' package 3.1.1.13~20170720104400-0)
#
# Reason: we need this code to speak Sandesh protocol, but we don't want to be
#       broken if 'contrail-status' script changes in next versions or isn't
#       installed on a machine

class EtreeToDict(object):
    """Converts the xml etree to dictionary/list of dictionary."""

    def __init__(self, xpath):
        self.xpath = xpath

    def _handle_list(self, elems):
        """Handles the list object in etree."""
        a_list = []
        for elem in elems.getchildren():
            rval = self._get_one(elem, a_list)
            if 'element' in rval.keys():
                a_list.append(rval['element'])
            elif 'list' in rval.keys():
                a_list.append(rval['list'])
            else:
                a_list.append(rval)

        if not a_list:
            return None
        return a_list

    def _get_one(self, xp, a_list=None):
        """Recrusively looks for the entry in etree and converts to dictionary.

        Returns a dictionary.
        """
        val = {}

        child = xp.getchildren()
        if not child:
            val.update({xp.tag: xp.text})
            return val

        for elem in child:
            if elem.tag == 'list':
                val.update({xp.tag: self._handle_list(elem)})
            else:
                rval = self._get_one(elem, a_list)
                if elem.tag in rval.keys():
                    val.update({elem.tag: rval[elem.tag]})
                else:
                    val.update({elem.tag: rval})
        return val

    def get_all_entry(self, path):
        """All entries in the etree is converted to the dictionary

        Returns the list of dictionary/didctionary.
        """
        xps = path.xpath(self.xpath)

        if type(xps) is not list:
            return self._get_one(xps)

        val = []
        for xp in xps:
            val.append(self._get_one(xp))
        return val

    def find_entry(self, path, match):
        """Looks for a particular entry in the etree.
        Returns the element looked for/None.
        """
        xp = path.xpath(self.xpath)
        f = filter(lambda x: x.text == match, xp)
        if len(f):
            return f[0].text  # pylint: disable=E1136
        return None


# End of verbatim copied code from /usr/bin/contrail-status

class IntrospectUtil(object):
    def __init__(self, ip, port, debug, timeout, max_retries=DEFAULT_MAX_RETRIES):
        self._ip = ip
        self._port = port
        self._debug = debug
        self._session = self._build_session(timeout, max_retries)

    def _mk_url_str(self, path):
        return "http://%s:%d/%s" % (self._ip, self._port, path)

    def _load(self, path):
        url = self._mk_url_str(path)
        resp = self._session.get(url)
        if resp.status_code == requests.codes.ok:
            return etree.fromstring(resp.text)
        else:
            if self._debug:
                print('URL: %s : HTTP error: %s' % (url, str(resp.status_code)))
            return None

    @staticmethod
    def _build_session(timeout, max_retries):
        session = requests.Session()
        adapter = HTTPAdapter(max_retries=Retry(
            total=max_retries,
            read=max_retries,
            connect=0,  # fail fast for contrail-schema
            backoff_factor=timeout))
        session.mount('http://', adapter)
        return session

    def get_uve(self, tname):
        path = 'Snh_SandeshUVECacheReq?x=%s' % (tname)
        xpath = './/' + tname
        p = self._load(path)
        if p is not None:
            return EtreeToDict(xpath).get_all_entry(p)
        else:
            if self._debug:
                print('UVE: %s : not found' % (path))
            return None

    def get(self, path):
        response = self._load(path)
        return None if response is None else EtreeToDict("./*").get_all_entry(response)


def is_ignored(name, names_to_ignore):
    for pattern in names_to_ignore:
        if fnmatch.fnmatch(name, pattern):
            return True
    else:
        return False


def is_non_functional(proc_status):
    # CLOUD-13348: contrail-collector connection is optional, everything else is not
    return (proc_status['state'].lower() != 'functional' and
            proc_status.get('description', '').lower() != 'collector connection down')


def get_procstatus(host, port, debug=False, timeout=None, max_retries=DEFAULT_MAX_RETRIES):
    """Get introspection of one service"""
    proc_status = None
    node_status = None
    if timeout is None:
        timeout = DEFAULT_TIMEOUT if host == DEFAULT_HOST \
            else DEFAULT_REMOTE_TIMEOUT
    svc_introspect = IntrospectUtil(host, port, debug, timeout, max_retries)
    try:
        node_status = svc_introspect.get_uve('NodeStatus')
    except requests.exceptions.RequestException as err:
        if debug:
            print('Error getting NodeStatus for {}:{}: {}'.format(host, port, str(err)))
            return None

    if not node_status or not len(node_status):
        if debug:
            print('Error getting NodeStatus for {}:{}'.format(host, port))
    else:
        proc_status = node_status[0]['process_status'][0]
    return proc_status


def get_contrail_service_status(proc_status, ignored_connections, host, port):
    status = STATUS_OK
    messages = []
    if not proc_status:
        status = STATUS_CRIT
        messages.append('Can\'t get status for service ({}:{})'.format(host, port))
    elif not len(proc_status['connection_infos']):
        status = STATUS_CRIT
        messages.append('No connections data for service ({}:{})'.format(host, port))
    else:
        for connection in proc_status['connection_infos']:
            ignored = is_ignored(connection['type'].lower(), ignored_connections)
            if connection['status'].lower() != 'up' and not ignored:
                desc = connection.get('description')

                msg = "{c[type]}/{c[name]}({c[server_addrs]}) is {c[status]} ({desc})".format(
                    c=connection,
                    desc=desc
                )
                messages.append(msg)
                status = STATUS_WARN
        if is_non_functional(proc_status):
            status = STATUS_CRIT
    return status, messages


def print_for_human(check_name, proc_status, ignored_connections):
    """Print service status in human-readable format"""
    details = []
    if not proc_status:
        print(red('{} is Down (can\'t get status)'.format(check_name)))
    else:
        # Print each connection status
        is_functional = not is_non_functional(proc_status)
        warn, crit = 0, 0 if is_functional else 1

        for connection in proc_status['connection_infos']:
            if connection['status'].lower() == 'up':
                color = green
            else:
                ignored = is_ignored(connection['type'].lower(), ignored_connections)
                # We don't care or the connection doesn't render the service non-functional
                if ignored or is_functional:
                    color = yellow
                    warn += 1
                else:
                    color = red
                    crit += 1

            type_ = connection['type']
            name = connection['name']
            desc = connection.get('description', '')

            full_type = '{}/{}'.format(type_, name) if name and name != type_ else type_
            full_desc = ' ({})'.format(desc) if desc else ''

            details.append(color('{:30} is {:4}{}'.format(full_type,
                                                          connection['status'],
                                                          full_desc)))

        # Print service summary status
        if crit:
            color = red
        elif warn:
            color = yellow
        else:
            color = magenta

        print(color('{} is {} ({})'.format(check_name,
                                           proc_status['state'],
                                           proc_status['description'])))

        for line in details:
            print('\t{}'.format(line))


def print_for_monitoring(check_name, status, desc):
    """Print service status in juggler-friendly format"""

    messages = ';'.join(desc) if isinstance(desc, list) else desc
    if not messages:
        messages = 'OK' if (status == STATUS_OK) else 'No info'

    print('PASSIVE-CHECK:{};{};{}'.format(check_name, status, messages))


class LoadArgsFromFile(argparse.Action):
    """Load arguments from config file"""
    def __call__(self, parser, namespace, values, option_string=None):
        with values as f:
            parser.parse_args(f.read().split(), namespace)


def parse_args():
    """Parse, normalize and validate command-line arguments"""
    all_checks = sorted(SUPPORTED_SERVICES.keys())

    parser = argparse.ArgumentParser(
        description='Checks for contrail (via Introspect API)',
        epilog='supported checks: ' + ' '.join(all_checks),
        formatter_class=lambda prog: argparse.HelpFormatter(prog, max_help_position=35))

    parser.add_argument('-t', '--test', help='output for tests', action='store_true')

    parser.add_argument('-o', '--host',
                        default=DEFAULT_HOST,
                        help='host to connect to (default: {})'.format(DEFAULT_HOST))

    parser.add_argument('-c', '--check',
                        dest='checks',
                        metavar='CHECK',
                        nargs='*',
                        default=all_checks,
                        help='checks to run (default: all checks)')

    parser.add_argument('-i', '--ignore',
                        dest='ignores',
                        metavar='IGNORE',
                        nargs='*',
                        default=[],
                        help='components to ignore connection problems')

    parser.add_argument('-f', '--config',
                        type=open,
                        action=LoadArgsFromFile,
                        metavar='FILE',
                        help='file name to load arguments from')

    args = parser.parse_args()

    # Names are case-insensitive (more user-friendly)

    args.checks = [x.lower() for x in args.checks]
    args.ignores = [x.lower() for x in args.ignores]

    for check in args.checks:
        if check not in all_checks:
            raise ValueError('Unsupported check: "{}". Run with -h to see supported checks.'.format(check))

    return args


def print_contrail_service_status(service_name, ignored_connections=None, host=DEFAULT_HOST):
    port = SUPPORTED_SERVICES[service_name]
    proc_status = get_procstatus(host, port)
    status, messages = get_contrail_service_status(proc_status, ignored_connections or [], host, port)
    return print_for_monitoring(service_name, status, messages)


def load_config(conf_name):
    conf_dirname = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), CONFIG_DIRS))
    with open(os.path.join(conf_dirname, conf_name)) as stream:
        return yaml.load(stream)
