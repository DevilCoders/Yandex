#!/skynet/python/bin/python
"""Compare disk size on machines and in reqs.volumes section"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import copy
from collections import defaultdict

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
import gaux.aux_decorators
import gaux.aux_volumes

GB = 1024 * 1024 * 1024


def get_parser():
    parser = ArgumentParserExt(description='Check disk volumes for hosts')
    parser.add_argument('-g', '--groups', type=argparse_types.groups, default=[],
                        help='Optional. List of groups to process')
    parser.add_argument('-x', '--exclude', type=argparse_types.groups, dest='exclude_groups', default=[],
                        help='Optional. List of groups to exclude')
    parser.add_argument('-s', '--hosts', type=argparse_types.hosts, default=[],
                        help='Optional. List of hosts to process')
    parser.add_argument('-v', '--verbose', action='count', default=0,
                        help='Optional. Increase output verbosity (maximum 2)')

    return parser


@gaux.aux_decorators.memoize
def calculate_usage(groupname):
    result = defaultdict(int)

    group = CURDB.groups.get_group(groupname)
    for volume_info in gaux.aux_volumes.volumes_as_objects(group):
        result[volume_info.host_mp_root] += volume_info.quota

    assert set(result.keys()) <= set(['/place', '/ssd'])

    return result


def failed_as_str(host, have, used, verbose):
    result = ['Host {}:'.format(host.name)]

    if verbose >= 1:
        for k in used.iterkeys():
            if (used[k] > have[k]) or (verbose >= 3):
                result.append('    Mp <{}> have only <{}> while need <{}>'.format(k, have[k] / float(GB), used[k] / float(GB)))

    if verbose >= 2:
        result.append('    Instances info:')

    for instance in CURDB.groups.get_host_instances(host):
        result.append('        Instance {}:'.format(instance.full_name()))

        instance_group = CURDB.groups.get_group(instance.type)
        volumes = gaux.aux_volumes.volumes_as_objects(instance_group)
        for volume in volumes:
            result.append('            Host mp <{}> guest mp <{}> needed: <{}>'.format(volume.host_mp_root, volume.guest_mp, volume.quota / float(GB)))

    return '\n'.join(result)


def main(options):
    exclude_hosts = set(sum((group.getHosts() for group in options.exclude_groups), []))
    hosts = set(sum((group.getHosts() for group in options.groups), []))
    hosts = list((hosts | set(options.hosts)) - exclude_hosts)


    if len(hosts) == 0:
        raise Exception('Found zero hosts to process')


    status = 0
    for host in hosts:
        have = {
            '/place': host.disk*GB,
            '/ssd': host.ssd*GB
        }

        used = defaultdict(int)
        for instance in CURDB.groups.get_host_instances(host):
            instance_used = calculate_usage(instance.type)
            for k, v in instance_used.iteritems():
                used[k] += v

        for k in used:
            if used[k] > have[k]:
                status = 1
                print failed_as_str(host, have, used, options.verbose)
                break
        else:
            if options.verbose >= 3:
                print failed_as_str(host, have, used, options.verbose)

    return status


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    status = main(options)

    sys.exit(status)
