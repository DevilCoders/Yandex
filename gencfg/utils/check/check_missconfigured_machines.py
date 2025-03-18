#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
from collections import defaultdict

import gencfg
import core.argparse.types as argparse_types
from gaux.aux_colortext import red_text
from gaux.aux_decorators import gcdisable

def lowmemhosts(hosts):
    return filter(lambda x: x.memory < 12, hosts)


def mirrorssdhosts(hosts):
    return filter(lambda x: x.ssd > x.disk * 2, hosts)


def repeatedinvnumhosts(hosts):
    hosts_by_invnum = defaultdict(list)

    for host in hosts:
        if host.invnum in ['unknown', '']:
            continue

        hosts_by_invnum[host.invnum].append(host)

    return sum(filter(lambda x: len(x) > 1, hosts_by_invnum.itervalues()), [])


def unknowninvnums(hosts):
    return filter(lambda x: x.invnum in ['unknown', ''], hosts)


def noiphosts(check_hosts):
    filtered = filter(lambda x: x.ipv4addr == 'unknown' and x.ipv6addr == 'unknown', check_hosts)
    if len(filtered) > 10:
        return filtered
    else:
        return []


def nosameips(hosts):
    hosts_by_ip = defaultdict(list)

    for host in hosts:
        hosts_by_ip[host.ipv4addr].append(host)
        hosts_by_ip[host.ipv6addr].append(host)

    hosts_by_ip.pop('unknown', None)

    return list(set(sum(filter(lambda x: len(x) > 1, hosts_by_ip.itervalues()), [])))


def unknownmodel(hosts):
    return filter(lambda x: x.is_vm_guest() == False and x.model == "unknown", hosts)


ISSUES = {
    'lowmemhosts': ("Hosts with low memory", lowmemhosts),
    'mirrorssdhosts': ("Hosts with probably mirror on ssd", mirrorssdhosts),
    'repeatedinvnumhosts': ("Hosts with invnum found twice or more times in gencfg db", repeatedinvnumhosts),
    'noiphosts': ("Hosts without ip IPv4 and IPv6 address", noiphosts),
    'nosameips': ("Multiple hosts with same IPv4/IPv6 addr", nosameips),
    'unknowninvnums': ("Hosts with unknown invnum", unknowninvnums),
    'unknownmodel': ("Hosts with unknown model", unknownmodel),
}


def parse_cmd():
    parser = ArgumentParser(
        description="Check missconfigured machines. Like machines with too low memory, with mirrored ssd, etc.")
    parser.add_argument("-i", "--issue", type=str, required=True,
                        choices=ISSUES.keys(),
                        help="Obligatory. Issue to deted: %s" % ','.join(ISSUES.keys()))
    parser.add_argument("-g", "--groups", type=argparse_types.groups, required=True,
                        help="Obligatory. List of groups to process")
    parser.add_argument("-f", "--group-filter", dest="group_filter", type=argparse_types.pythonlambda, default=None,
                        help="Optional. Filter on groups")
    parser.add_argument("--host-filter", type=argparse_types.pythonlambda, default=None,
                        help="Optional. Filter on hosts")
    parser.add_argument("-c", "--show-configured", dest="show_configured", action="store_true", default=False,
                        help="Optional. Show only configured machines")
    parser.add_argument("--fail-on-error", action="store_true", default=False,
                        help="Optional. Fail (return 1) when found missconfigured host")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    options.groups = filter(lambda x: x.card.on_update_trigger is None, options.groups)
    if options.group_filter:
        options.groups = filter(lambda x: options.group_filter(x), options.groups)

    return options

@gcdisable
def main(options):
    all_hosts = list(set(sum(map(lambda x: x.getHosts(), options.groups), [])))
    if options.host_filter is not None:
        all_hosts = filter(options.host_filter, all_hosts)
    missconfigured_hosts = ISSUES[options.issue][1](all_hosts)

    return all_hosts, missconfigured_hosts, ISSUES[options.issue][0]


if __name__ == '__main__':
    options = parse_cmd()
    all_hosts, missconfigured_hosts, descr = main(options)

    if options.show_configured:
        print "Good machines: %s" % ','.join(map(lambda x: x.name, set(all_hosts) - set(missconfigured_hosts)))
    else:
        if len(missconfigured_hosts):
            print red_text("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
            print red_text("%s: %s" % (descr, ','.join(sorted(map(lambda x: "{}({})".format(x.name, x.invnum), missconfigured_hosts)))))
            print red_text("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")

    if options.fail_on_error and len(missconfigured_hosts) > 0:
        sys.exit(1)
