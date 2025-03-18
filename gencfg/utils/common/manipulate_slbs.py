#!/skynet/python/bin/python
"""Script to maniplate racktables slbs (GENCFG-1781)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import json
import urllib2
import zlib
from xml.etree import cElementTree

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from core.settings import SETTINGS
from gaux.aux_utils import retry_urlopen


class EActions(object):
    """All action to perform on ipv4 tunnels"""
    STATS = 'stats'  # show stats of local base
    DIFF = 'diff'  # show diff with racktables
    SYNC = 'sync'  # export gencfg data to racktables
    ALL = [STATS, DIFF, SYNC]


def get_parser():
    parser = ArgumentParserExt(description='Manipulate on ipv4tunnels of on ipv6-only machines')
    parser.add_argument('-a', '--action', type=str, default = EActions.STATS,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 1.")

    return parser


def normalize(options):
    pass


def main_stats(options):
    result = []
    result.append('Found <{}> entries in local data:'.format(len(CURDB.slbs.get_all())))
    for slb in CURDB.slbs.get_all():
        result.append('    Slbs <{}> has <{}> addrs: {}'.format(slb.fqdn, len(slb.ips), ','.join(slb.ips)))

    return '\n'.join(result)


def __get_racktables_slbs():
    url = SETTINGS.services.racktables.slb.list_.url

    raw_data = retry_urlopen(3, url)
    tree = cElementTree.fromstring(raw_data)

    result = []
    for service_elem in tree.findall('virtual_service'):
        service_name = service_elem.attrib['name']
        service_ips = [x.text for x in service_elem.findall('vips/vip')]
        result.append(dict(name=service_name, scope=service_ips))

    return result


def main_diff(options):
    result = []

    racktables_data = __get_racktables_slbs()
    racktables_data_by_fqdn = {x['name']:x for x in racktables_data}
    gencfg_data_by_fqdn = {x.fqdn:x for x in CURDB.slbs.get_all()}

    # process common fqdns
    common_fqdns = set(racktables_data_by_fqdn.keys()) & (set(gencfg_data_by_fqdn.keys()))
    result.append('Found <{}> common to gencfg and racktables fqdns:'.format(len(common_fqdns)))
    for common_fqdn in common_fqdns:
        have_different_ips = (set(gencfg_data_by_fqdn[common_fqdn].ips) != set(racktables_data_by_fqdn[common_fqdn]['scope']))

        if (options.verbose_level >= 1) or have_different_ips:
            result.append('    Fqdn <{}>:'.format(common_fqdn))

        if have_different_ips:
            result.append('        Gencfg ips: <{}>, Racktables ips: <{}>'.format(sorted(gencfg_data_by_fqdn[common_fqdn].ips),
                                                                                  sorted(racktables_data_by_fqdn[common_fqdn]['scope'])))
        else:
            if options.verbose_level >= 1:
                result.append('        Gencfg and Racktables ips: <{}>'.format(sorted(gencfg_data_by_fqdn[common_fqdn].ips)))

    # process racktables extra fqdns
    racktables_extra_fqdns = set(racktables_data_by_fqdn.keys()) - (set(gencfg_data_by_fqdn.keys()))
    result.append('Found <{}> racktrables extra fqdns:'.format(len(racktables_extra_fqdns)))
    for fqdn in racktables_extra_fqdns:
        result.append('    Racktables fqdn <{}>:'.format(fqdn))
        result.append('        Racktables ips: <{}>'.format(sorted(racktables_data_by_fqdn[fqdn]['scope'])))

    # process gencfg extra fqdns
    gencfg_extra_fqdns = set(gencfg_data_by_fqdn.keys()) - (set(racktables_data_by_fqdn.keys()))
    result.append('Found <{}> gencfg extra fqdns:'.format(len(gencfg_extra_fqdns)))
    for fqdn in gencfg_extra_fqdns:
        result.append('    Gencfg fqdn <{}>:'.format(fqdn))
        result.append('        Gencfg ips: <{}>'.format(sorted(gencfg_data_by_fqdn[fqdn].ips)))

    return '\n'.join(result)


def main_sync(optoins):
    CURDB.slbs.mark_as_modified()

    racktables_data = __get_racktables_slbs()
    racktables_data_by_fqdn = {x['name']:x for x in racktables_data}
    gencfg_data_by_fqdn = {x.fqdn:x for x in CURDB.slbs.get_all()}

    # update current
    common_fqdns = set(racktables_data_by_fqdn.keys()) & (set(gencfg_data_by_fqdn.keys()))
    modified_fqdns = []
    for fqdn in common_fqdns:
        if sorted(gencfg_data_by_fqdn[fqdn].ips) != sorted(racktables_data_by_fqdn[fqdn]['scope']):
            gencfg_data_by_fqdn[fqdn].ips = sorted(racktables_data_by_fqdn[fqdn]['scope'])
            modified_fqdns.append(fqdn)

    # add new
    racktables_extra_fqdns = set(racktables_data_by_fqdn.keys()) - (set(gencfg_data_by_fqdn.keys()))
    for fqdn in racktables_extra_fqdns:
        CURDB.slbs.add(dict(fqdn=fqdn, ips=sorted(racktables_data_by_fqdn[fqdn]['scope'])))

    # remove old
    gencfg_extra_fqdns = set(gencfg_data_by_fqdn.keys()) - (set(racktables_data_by_fqdn.keys()))
    for fqdn in gencfg_extra_fqdns:
        CURDB.slbs.remove(fqdn)

    CURDB.update(smart=True)

    return 'Added {} entries, modified {} entries, removed {} entries'.format(len(racktables_extra_fqdns), len(modified_fqdns), len(gencfg_extra_fqdns))


def main(options):
    if options.action == EActions.STATS:
        return main_stats(options)
    elif options.action == EActions.DIFF:
        return main_diff(options)
    elif options.action == EActions.SYNC:
        return main_sync(options)
    else:
        raise Exception('Unknown action {}'.format(options.action))


def print_result(result, options):
    print result


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result, options)
