#!/usr/bin/python3

import os
import re
import sys
import signal
import requests
import argparse
from urllib.parse import urljoin
from pprint import pprint


class ApiKeyException(Exception):
    pass


class PingdomRequestException(Exception):
    pass


class PingdomConf(object):
    """
    Config class with needed properties.
    """
    def __init__(self):
        self._apikey = os.environ.get('PINGDOM_APIKEY', None)
        self._apiurl = 'https://api.pingdom.com/api/3.1/'

    @property
    def apikey(self) -> str:
        if self._apikey is None:
            raise ApiKeyException('No api token found in PINGDOM_APIKEY environment variable')
        return self._apikey

    @property
    def apiurl(self) -> str:
        return self._apiurl


class Printer(object):
    """
    Colorful printer class.
    Call method before printing desired text to print it in color.
    After, call reset.
    """
    @staticmethod
    def green():
        print('\033[0;32m', end='')

    @staticmethod
    def red():
        print('\033[0;31m', end='')

    @staticmethod
    def yellow():
        print('\033[0;33m', end='')

    @staticmethod
    def reset():
        print('\033[0m', end='')


class PingdomApi(object):
    """
    Wrapper class for Pingdom API functions.
    """
    def __init__(self, pc: PingdomConf):
        self.url = pc.apiurl
        self.apikey = pc.apikey

    @staticmethod
    def quiet_print_check(check: dict) -> None:
        """
        Pretty and colorfully print one check.
        Output format: `CheckName`: `status`
        """
        status_colors = {
            'up': 'green',
            'down': 'red',
            'paused': 'yellow',
            'unknown': 'reset'
            }
        print(check['name'], end=': ')
        getattr(Printer, status_colors[check['status']])()
        print(check['status'])
        Printer.reset()

    def _request_(self, uri: str, method: str = 'GET', data: dict = {}, headers: dict = {}, params: dict = {}) -> dict:
        """
        Common request method.
        """
        headers.update({'Authorization': 'Bearer {}'.format(self.apikey)})
        request_url = urljoin(self.url, uri)
        response = getattr(requests, method.lower())(request_url, headers=headers, data=data, params=params)
        if response.status_code == 401:
            raise ApiKeyException('Bad key in PINGDOM_APIKEY environment variable;\n'
                                  'Reason: got 401 from Pingdom API.')
        elif response.status_code != 200:
            if response.json()['error']['errormessage']:
                msg = response.json()['error']['errormessage']
                raise PingdomRequestException('Error {} while requesting Pingdom API.\nMessage: {}'.format(response.status_code, msg))
            raise PingdomRequestException('Error {} while requesting Pingdom API.'.format(response.status_code))
        return response.json()

    def list_all_checks(self) -> dict:
        """
        List all checks for current account.
        """
        res = self._request_('checks', params={'include_tags': True})
        return res

    def check_by_name(self, name: str) -> list:
        """
        Get check[s] by name. Returns list, because there can be multiple checks
        with the same name, but different `id`.
        """
        res = self._request_('checks', params={'include_tags': True})
        checks = [check for check in res['checks'] if check['name'] == name]
        return checks

    def check_by_tags(self, tags: str) -> list:
        """
        Get check[s] by tags. Returns list, because there can be multiple checks
        with the same tag.
        """
        res = self._request_('checks', params={'include_tags': True, 'tags': tags})
        return res

    def check_status(self, name: str) -> list:
        """
        Get checks' status by name.
        """
        checks = self.check_by_name(name)
        res = []
        for check in checks:
            res.append('{} check is {}'.format(check['name'], check['status']))
        return res

    def _modify_check_(self, name: str, data: dict = {}) -> (str, bool):
        """
        Modifies checks by name using PUT method.
        """
        checks = self.check_by_name(name)
        if not checks:
            return 'No checks for {}'.format(name), False
        for check in checks:
            _id_ = check['id']
            res = self._request_('checks/{}/'.format(_id_), method='PUT', data=data)
            msg = str(res['message'])
            success = 'success' in msg
            return msg, success

    def pause_check(self, name) -> bool:
        """
        Pause check by name.
        """
        return self._modify_check_(name, data={'paused': True})[1]

    def unpause_check(self, name) -> bool:
        """
        Unpause check by name.
        """
        return self._modify_check_(name, data={'paused': False})[1]

    def pause_all_checks(self) -> dict:
        """
        Pause all checks for current account.
        """
        res = self._request_('checks', method='PUT', data={'paused': True})
        return res

    def unpause_all_checks(self) -> dict:
        """
        Unpause all checks for current account.
        """
        res = self._request_('checks', method='PUT', data={'paused': False})
        return res


def validate_id(ident):
    ident = str(ident)
    matchObj = re.match(r'^\d+$', ident)

    if matchObj:
        return True
    else:
        print('Invalid id: %s' % ident)
        return False


def parse_args():
    """
    Parsing command line arguments.
    3 arguments: list, pause, unpause
    4 positional arguments: --all, --by-tag[s], --by-name[s], --by-status
    """
    parser = argparse.ArgumentParser(description='Pingdom command line interface script. Using api key from ENV variable PINGDOM_APIKEY')
    parser.add_argument('-v', '--verbose', action='store_true', dest='verbose_output')

    sub_parsers = parser.add_subparsers(dest='action')
    list_parser = sub_parsers.add_parser('list')
    pause_parser = sub_parsers.add_parser('pause')
    unpause_parser = sub_parsers.add_parser('unpause')

    list_parser.add_argument(
        '--all', action='store_true', dest='all',
        help='List all checks (eg. `list --all`)'
    )
    list_parser.add_argument(
        '--by-tags', '--by-tag', action='store', type=str, dest='tags', nargs='+',
        help='List checks by TAG[s] (eg. `list --by-tags TAG1[ TAG2 TAG3]`)'
    )
    list_parser.add_argument(
        '--by-names', '--by-name', action='store', type=str, dest='name', nargs='+',
        help='List check[s] with specific NAME[s] (eg. `list --by-name NAME1[ NAME2 NAME3]`)'
    )
    list_parser.add_argument(
        '--by-status', action='store', type=str, dest='status', nargs='+',
        help='List check[s] with specific STATUS[es] (eg. `list --by-status down[ paused]`)'
    )

    pause_parser.add_argument(
        '--all', action='store_true', dest='all',
        help='Pause all checks (eg. `pause --all`)'
    )
    pause_parser.add_argument(
        '--by-tags', '--by-tag', action='store', type=str, dest='tags', nargs='+',
        help='Pause checks by TAG[s] (eg. `pause --by-tags TAG1[ TAG2 TAG3]`)'
    )
    pause_parser.add_argument(
        '--by-names', '--by-name', action='store', type=str, dest='name', nargs='+',
        help='Pause check[s] with specific NAME[s] (eg. `pause --by-name NAME1[ NAME2 NAME3]`)'
    )
    pause_parser.add_argument(
        '--by-status', action='store', type=str, dest='status', nargs='+',
        help='Pause check[s] with specific STATUS[es] (eg. `pause --by-status down`)'
    )

    unpause_parser.add_argument(
        '--all', action='store_true', dest='all',
        help='Unpause all checks (eg. `unpause --all`)'
    )
    unpause_parser.add_argument(
        '--by-tags', '--by-tag', action='store', type=str, dest='tags', nargs='+',
        help='Unpause checks by TAG[s] (eg. `unpause --by-tags TAG1[ TAG2 TAG3]`)'
    )
    unpause_parser.add_argument(
        '--by-names', '--by-name', action='store', type=str, dest='name', nargs='+',
        help='Unpause check[s] with specific NAME[s] (eg. `unpause --by-name NAME1[ NAME2 NAME3]`)'
    )
    unpause_parser.add_argument(
        '--by-status', action='store', type=str, dest='status', nargs='+',
        help='Unpause check[s] with specific STATUS[es] (eg. `unpause --by-status paused`)'
    )

    return parser, parser.parse_args()


# Helper methods
#

def _print_list_of_checks_(all_checks: dict, verbose: bool = False) -> None:
    """
    Print any list of checks.
    The checks will be printed in sorted order by status:
    order: up, unknown, paused, down -- for better human visual understanding.
    After list of checks, prints counts by statuses and total.

    :param all_checks: dict : Dict, needed keys:
        1. checks -- list of dicts with checks
        2. filtered -- int, total count of checks
    :param verbose: bool : Pretty print quietly all checks if verbose is False.
    Else just print raw json.
    """
    total_count = all_checks['counts']['filtered']
    all_checks = sorted(all_checks['checks'], key=lambda k: k['status'], reverse=True)
    if verbose:
        pprint(all_checks)
    else:
        for check in all_checks:
            PingdomApi.quiet_print_check(check)
    paused, up, down, unknown = 0, 0, 0, 0
    for check in all_checks:
        if check['status'] == 'up':
            up += 1
        elif check['status'] == 'down':
            down += 1
        elif check['status'] == 'unknown':
            unknown += 1
        elif check['status'] == 'paused':
            paused += 1
    print('\r\nUP: {}\r\nDOWN: {}\r\nPAUSED: {}\r\nUNKNOWN (checking in progress): {}\r\nTOTAL: {}'.format(up,
                                                                                                           down,
                                                                                                           paused,
                                                                                                           unknown,
                                                                                                           total_count))


def _get_checks_by_names_(p: PingdomApi, names: list) -> dict:
    checks = []
    for name in names:
        checks += p.check_by_name(name)
    return {'checks': checks, 'counts': {'filtered': len(checks)}}


def _get_checks_by_status_(p: PingdomApi, status: list) -> dict:
    all_checks = p.list_all_checks()['checks']
    checks = []
    for check in all_checks:
        if check['status'] in status:
            checks.append(check)
    return {'checks': checks, 'counts': {'filtered': len(checks)}}


def _pause_list_of_checks_(p: PingdomApi, all_checks: dict, verbose: bool = False):
    count, total = 0, all_checks['counts']['filtered']
    for check in all_checks['checks']:
        res = p.pause_check(check['name'])
        if res:
            count += 1
    print('Successfully paused {} of {} checks:\n'.format(count, total))


def _unpause_list_of_checks__(p: PingdomApi, all_checks: dict, verbose: bool = False):
    count, total = 0, all_checks['counts']['filtered']
    for check in all_checks['checks']:
        res = p.unpause_check(check['name'])
        if res:
            count += 1
    print('Successfully unpaused {} of {} checks:\n'.format(count, total))


# Main API wrappers methods
#

def list_checks(p: PingdomApi, verbose: bool = False) -> None:
    all_checks = p.list_all_checks()
    _print_list_of_checks_(all_checks, verbose)


def list_by_tags(p: PingdomApi, tags: list, verbose: bool = False) -> None:
    all_checks = p.check_by_tags(','.join(tags))
    _print_list_of_checks_(all_checks, verbose)


def list_by_names(p: PingdomApi, names: list, verbose: bool = False) -> None:
    _print_list_of_checks_(_get_checks_by_names_(p, names), verbose)


def list_by_status(p: PingdomApi, status: list, verbose: bool = False) -> None:
    _print_list_of_checks_(_get_checks_by_status_(p, status), verbose)


def pause_all_checks(p: PingdomApi) -> None:
    print(p.pause_all_checks())


def pause_by_tags(p: PingdomApi, tags: list, verbose: bool = False) -> None:
    _pause_list_of_checks_(p, p.check_by_tags(','.join(tags)), verbose)
    list_by_tags(p, tags, verbose)


def pause_by_name(p: PingdomApi, name: list, verbose: bool = False) -> None:
    _pause_list_of_checks_(p, _get_checks_by_names_(p, name), verbose)
    list_by_names(p, name, verbose)


def pause_by_status(p: PingdomApi, status: list, verbose: bool = False) -> None:
    _pause_list_of_checks_(p, _get_checks_by_status_(p, status), verbose)
    list_by_status(p, status, verbose)


def unpause_all_checks(p: PingdomApi) -> None:
    print(p.unpause_all_checks())


def unpause_by_tags(p: PingdomApi, tags: list, verbose: bool = False) -> None:
    _unpause_list_of_checks__(p, p.check_by_tags(','.join(tags)), verbose)
    list_by_tags(p, tags, verbose)


def unpause_by_name(p: PingdomApi, name: list, verbose: bool = False) -> None:
    _unpause_list_of_checks__(p, _get_checks_by_names_(p, name), verbose)
    list_by_names(p, name, verbose)


def unpause_by_status(p: PingdomApi, status: list, verbose: bool = False) -> None:
    _unpause_list_of_checks__(p, _get_checks_by_status_(p, status), verbose)
    list_by_status(p, status, verbose)


def check_args(args: argparse.Namespace) -> bool:
    # Must be set exactly one parameter
    args_list = [args.all, args.name, args.status, args.tags]
    if args_list.count(None) + args_list.count(False) != len(args_list) - 1:
        return False
    return True


def sigint_handler(signal: int, frame: object) -> None:
    """
    Handler for sudden SIGINT.
    """
    Printer.yellow()
    print('Cached SIGINT signal (or you pressed Ctrl+C), therefore NOTHING is guaranteed!\n'
          'Use `list` argument to view what happened to your checks.')
    Printer.reset()
    exit(0)


def main():
    signal.signal(signal.SIGINT, sigint_handler)
    parser, args = parse_args()

    # Print error if no apikey fount in ENV.
    #
    try:
        PingdomConf().apikey
    except ApiKeyException as e:
        Printer.red()
        print('{}:'.format(e.__class__.__name__))
        print('Need to have environment variable PINGDOM_APIKEY.\n'
              'Get API_downtimer_key at https://yav.yandex-team.ru/secret/sec-01d80gt7p3n0pgh7p5pszq662h (for kinopoisk)\n'
              'Or ask your Pingdom administrator for a key.')
        Printer.reset()
        sys.exit(1)

    p = PingdomApi(PingdomConf())

    # Check if only one optional argument is set.
    #
    if not check_args(args):
        print('Invalid list of arguments. Make sure you use only one optional argument from given list.\n'
              'View ``your positional argument` --help` (e.g. list --help)')
        sys.exit(1)

    verbose = args.verbose_output

    try:
        if args.action == 'list':
            if args.all:
                list_checks(p, verbose)
            if args.tags:
                list_by_tags(p, args.tags, verbose)
            if args.name:
                list_by_names(p, args.name, verbose)
            if args.status:
                list_by_status(p, args.status, verbose)
        if args.action == 'pause':
            if args.all:
                pause_all_checks(p, verbose)
            if args.tags:
                pause_by_tags(p, args.tags, verbose)
            if args.name:
                pause_by_name(p, args.name, verbose)
            if args.status:
                pause_by_status(p, args.status, verbose)
        if args.action == 'unpause':
            if args.all:
                unpause_all_checks(p, verbose)
            if args.tags:
                unpause_by_tags(p, args.tags, verbose)
            if args.name:
                unpause_by_name(p, args.name, verbose)
            if args.status:
                unpause_by_status(p, args.status, verbose)
    except Exception as e:
        Printer.red()
        print('{}:'.format(e.__class__.__name__))
        print(str(e))
        Printer.reset()
