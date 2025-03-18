#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from optparse import OptionParser

import gencfg
from core.db import CURDB


def parse_cmd():
    usage = 'Usage %prog [options]'
    parser = OptionParser(usage=usage)
    parser.add_option('-c', '--cat-hosts-data', action='store_true', dest='cat_hosts_data',
                      help='Cat hardware_data/hosts_data for all group hosts')
    parser.add_option('-i', '--show-instances', action='store_true', dest='show_instances',
                      help='Show instances data')
    parser.add_option('-x', '--show-extended-instances', action='store_true', dest='show_extended_instances',
                      help='Show instances data with power and dc')
    parser.add_option('-g', '--groups', type='str', dest='groups', default=None,
                      help='obligatory. Comma-separated list of groups to process')
    parser.add_option('-s', '--sky', action='store_true', dest='sky', default=False,
                      help='obligatory. Use sky format')
    parser.add_option('--first-n', type='int', dest='first_n', default=None,
                      help='optional. Print only first n hosts')

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()

    if options.groups is None:
        raise Exception('Missed obligator -g(--group) option')
    if (options.cat_hosts_data is not None) + (options.show_instances is not None) + (
        options.show_extended_instances is not None) > 1:
        raise Exception('At most one of <show_instances>, <show_extended_instances> and <cat_hosts_data> should be specified')

    options.groups = map(lambda x: CURDB.groups.get_group(x), options.groups.split(','))

    return options, args


hosts = None


def get_hosts(options):
    global hosts
    if hosts is None:
        hosts = list(set(sum([group.getHosts() for group in options.groups], [])))
    return hosts


instances = None


def get_instances(options):
    global instances
    if instances is None:
        instances = list(set(sum(map(lambda x: list(x.get_instances()), options.groups), [])))
    return instances


if __name__ == '__main__':
    (options, args) = parse_cmd()

    separator = ' '
    prefix = ''
    if options.cat_hosts_data:
        for host in sorted(get_hosts(options)):
            print host.tostring()
    else:

        if options.show_instances:
            elements = map(lambda x: '%s:%s' % (x.host.name, x.port), get_instances(options))
        elif options.show_extended_instances:
            elements = map(lambda x: '%s:%s:%s:%s' % (x.host.name, x.port, x.power, x.host.dc), get_instances(options))
        else:
            separator = ',' if not options.sky else ' +'
            prefix = '' if not options.sky else '+'
            elements = map(lambda x: x.name, get_hosts(options))

        elements = sorted(elements)
        if options.first_n is not None:
            elements = elements[:options.first_n]
        print prefix + separator.join(elements)
