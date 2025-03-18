#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
import requests

from core.db import CURDB
from core.settings import SETTINGS
from argparse import ArgumentParser


class EActions(object):
    SHOW_EXTRA = 'show_extra'
    SHOW_MAPPING = 'show_mapping'

    ALL = [SHOW_EXTRA, SHOW_MAPPING]


def get_bot_hosts():
    fqdn_to_invnum = {}

    data = requests.get(SETTINGS.services.oops.rest.hosts.url).text
    for elem in data.strip().split('\n'):
        parts = elem.strip().split('\t')

        hostname = parts[1].lower().strip()
        if not hostname:
            continue

        invnum = str(parts[0])

        if hostname not in fqdn_to_invnum:
            fqdn_to_invnum[hostname] = invnum
        else:
            raise ValueError('Bot return {} twice or more times'.format(hostname))

    return fqdn_to_invnum


def get_gencfg_hosts():
    fqdn_to_invnum = {}

    for host in CURDB.hosts.get_hosts():
        if host.name not in fqdn_to_invnum:
            fqdn_to_invnum[host.name] = host.invnum
        else:
            raise ValueError('GenCfg has {} twice or more times'.format(hostname))

    return fqdn_to_invnum


def get_common(left, right):
    common = set(left) & set(right)
    return common


def get_extra(subtrahend, subtractor):
    extra = set(subtrahend) - set(subtractor)
    return list(extra)


def get_extra_symetric(left, right):
    extra_left = get_extra(left, right)
    extra_right = get_extra(right, left)
    return extra_left + extra_right


def show_extra_hostnames(subtrahend_fqdn_to_invnum, subtractor_fqdn_to_invnum, output_offset=0):
    subtractor_invnum_to_fqdn = {v: k for k, v in subtractor_fqdn_to_invnum.items()}
    extra_subtrahend_hostnames = get_extra(subtrahend_fqdn_to_invnum.keys(), subtractor_fqdn_to_invnum.keys())
    for hostname in extra_subtrahend_hostnames:
        invnum = subtrahend_fqdn_to_invnum[hostname]
        if invnum == 'unknown':
            continue

        if invnum in subtractor_invnum_to_fqdn:
            subtractor_hostname = subtractor_invnum_to_fqdn[invnum]
            if subtractor_hostname in subtrahend_fqdn_to_invnum:
                subtrahend_invnum = subtrahend_fqdn_to_invnum[subtractor_hostname]
                print('{}{}(l) -> {}(lr) -> {}(r) -> {}(l)'.format(' ' * output_offset, hostname, invnum, subtractor_hostname, subtrahend_invnum))
            else:
                print('{}{}(l) -> {}(lr) -> {}(r)'.format(' ' * output_offset, hostname, invnum, subtractor_hostname))
        else:
            print('{}{}: {}'.format(' ' * output_offset, hostname, invnum))


def show_mapping_hostnames(fqdn_to_invnum, filter_func=lambda h, i: True, output_offset=0):
    for hostname, invnum in fqdn_to_invnum.items():
        if not filter_func(hostname, invnum):
            continue
        print('{}{}: {}'.format(' ' * output_offset, hostname, invnum))


def parser_cmd():
    def python_lambda(value):
        if not value.startswith('lambda'):
            raise ValueError('Argumnet must be python lambda')
        return eval(value)

    parser = ArgumentParser(description='Script to show mapping hostname to invnum.')
    parser.add_argument('-a', '--action', type=str, required=True, choices=EActions.ALL,
                        help='Action to execute')
    parser.add_argument('-f', '--filter', type=python_lambda, default='lambda h, i: bool(h) and i != "unknown"',
                        help='Python lambda expression to filter mapping output.')

    return parser.parse_args()


def main():
    options = parser_cmd()

    bot_fqdn_to_invnum = get_bot_hosts()
    gencfg_fqdn_to_invnum = get_gencfg_hosts()

    if options.action == EActions.SHOW_EXTRA:
        print('Extra BOT hostnames')
        show_extra_hostnames(bot_fqdn_to_invnum, gencfg_fqdn_to_invnum, output_offset=4)

        print('Extra GenCfg hostnames')
        show_extra_hostnames(gencfg_fqdn_to_invnum, bot_fqdn_to_invnum, output_offset=4)

    elif options.action == EActions.SHOW_MAPPING:
        print('BOT mapping')
        show_mapping_hostnames(bot_fqdn_to_invnum, options.filter, output_offset=4)

        print('GenCfg mapping')
        show_mapping_hostnames(gencfg_fqdn_to_invnum, options.filter, output_offset=4)


if __name__ == '__main__':
    main()
