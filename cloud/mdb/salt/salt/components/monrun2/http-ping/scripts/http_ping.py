#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import sys

import requests
from requests.adapters import HTTPAdapter

URL = 'https://localhost/ping'
TIMEOUT = 5.0


class ParseRanges(argparse.Action):
    """
    Parse ranges
    """

    def __init__(self, option_strings, dest, nargs=None, **kwargs):
        super(ParseRanges, self).__init__(option_strings, dest, **kwargs)

    def __call__(self, parser, namespace, value, option_string=None):
        try:
            getattr(namespace, self.dest).append(_parse_range(value))
        except AttributeError:
            setattr(namespace, self.dest, [_parse_range(value)])


class PingError(RuntimeError):
    pass


def _parse_range(value, sep='-'):
    if sep in value:
        start, end = value.split(sep)
        return range(int(start), int(end))
    return range(int(value), int(value) + 1)


def _create_http_adapter():
    """
    Return adapter instance -- required so that we dont retry things
    """
    # http://docs.python-requests.org/en/master/api/#requests.adapters.HTTPAdapter
    return HTTPAdapter(
        max_retries=1,
        pool_connections=1,
        pool_maxsize=1,
    )


def _ping(url, host=None, cert=None, ca_bundle=None, timeout=None):
    """
    Execute ping
    """
    headers = {}

    if not isinstance(cert, tuple):
        raise TypeError('cert must be a tuple of cert, key file paths')
    if timeout is None:
        timeout = TIMEOUT
    if host is not None:
        headers['Host'] = host
    if ca_bundle is None:
        ca_bundle = False
    ses = requests.Session()
    ses.mount(url, _create_http_adapter())
    try:
        # Python3 does not provide this.
        requests.packages.urllib3.disable_warnings()
    except AttributeError:
        pass
    try:
        # http://docs.python-requests.org/en/master/api/#requests.Session.request
        result = ses.get(
            url,
            cert=cert,
            verify=ca_bundle,
            timeout=timeout,
            headers=headers,
        )
        return result.status_code
    except requests.ConnectionError:
        raise PingError('unable to connect')
    except requests.Timeout:
        raise PingError('read timed out ({0})'.format(TIMEOUT))
    except Exception:
        raise PingError('unable to read response')


def _check(args, code_map):
    """ Check result against map"""
    http_code = _ping(
        args.url,
        host=args.host,
        cert=(args.cert, args.key),
        ca_bundle=args.ca,
        timeout=args.timeout,
    )
    for result_code, msg, code_set in code_map:
        for codes in code_set:
            if http_code in codes:
                return (result_code, msg.format(http_code))
    raise ValueError('response code didnt match any filters')


def _result(code=0, msg='OK'):
    """ Output the result and exit"""
    print('{c};{m}'.format(c=code, m=msg))
    sys.exit(0)


def _parse_args():
    arg = argparse.ArgumentParser(description='API pinger')

    arg.add_argument(
        '--ca',
        metavar='<path>',
        required=False,
        default=None,
        help='path to CA bundle')
    arg.add_argument(
        '--key',
        metavar='<path>',
        required=False,
        default=None,
        help='path to client certificate key')
    arg.add_argument(
        '--cert',
        metavar='<path>',
        required=False,
        default=None,
        help='path to client certificate')
    arg.add_argument(
        '--host',
        metavar='<hostname>',
        required=False,
        default=None,
        help='hostname to use in Host header')
    arg.add_argument(
        '--url',
        metavar='<url>',
        required=False,
        default=URL,
        help='URL to use')
    arg.add_argument(
        '-t',
        '--timeout',
        type=float,
        required=False,
        default=TIMEOUT,
        help='timeout',
    )
    arg.add_argument(
        '-o',
        '--ok',
        metavar='<int or range>',
        nargs='+',
        required=True,
        action=ParseRanges,
        dest='ok',
        help='integer or range, e.g. 400-500',
    )
    arg.add_argument(
        '-w',
        '--warn',
        nargs='+',
        required=True,
        action=ParseRanges,
        dest='warn',
    )
    arg.add_argument(
        '-c',
        '--crit',
        nargs='+',
        required=True,
        action=ParseRanges,
        dest='crit',
    )

    args = arg.parse_args()
    if args.cert is not None and args.key is None:
        raise ValueError('cert is provided but key is absent')
    return args


def _main():
    try:
        args = _parse_args()
        code_map = (
            # Order matters
            (0, 'OK', args.ok),
            (1, 'warn: got {0}', args.warn),
            (2, 'crit: got {0}', args.crit),
        )
        _result(*_check(args, code_map))
    except PingError as err:
        _result(2, 'crit: {0}'.format(err))
    except Exception as exc:
        _result(2, 'crit: {0}'.format(exc))


if __name__ == '__main__':
    _main()
