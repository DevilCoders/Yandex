#!/usr/bin/env python

import argparse

import requests


def _main():
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument(
        '-u',
        '--url',
        type=str,
        help='mdb-health url')

    args_parser.add_argument(
        '-v',
        '--verify',
        type=str,
        help='url TLS ceritificate (do not set for default)')

    args = args_parser.parse_args()
    url = args.url
    verify = args.verify

    try:
        resp = requests.get('{url}/v1/stats'.format(url=url), verify=verify)
        if resp.status_code != 200:
            print('1;code: %d, msg: %s' % (resp.status_code, resp.reason))
            return

        # TODO do something about stats
        print('0;OK')
    except Exception as exc:
        print('1;%s' % exc)


if __name__ == '__main__':
    _main()
