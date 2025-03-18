#!./venv/venv/bin/python

import os
import sys
import json

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
sys.path.append('/skynet')

import re
import gencfg
import core.argparse.types as argparse_types

from argparse import ArgumentParser
from core.db import CURDB
from gaux.aux_utils import retry_urlopen
from utils.pregen import update_hosts
from gaux.aux_reserve import ALL_PRENALIVKA_TAGS, PRENALIVKA_GROUP, UNSORTED_GROUP
from core.settings import SETTINGS

FILTERS = [
    (lambda host: ALL_PRENALIVKA_TAGS & set(host.walle_tags), PRENALIVKA_GROUP),
]


class TBotHost(object):
    __slots__ = ['name', 'invnum']

    def __init__(self, line):
        self.name = line.split('\t')[1].lower()
        self.invnum = line.split('\t')[0]


class EActions(object):
    ADDNEW = 'addnew'
    UPDATE_HOSTS_INFO = 'update_hosts_info'
    SHOWDIFF = 'showdiff'
    REMOVEOLD = 'removeold'
    SHOWHOSTS = 'showhosts'
    ALL = [ADDNEW, UPDATE_HOSTS_INFO, SHOWDIFF, REMOVEOLD, SHOWHOSTS]


def parse_cmd():
    parser = ArgumentParser("Synchronize generator hosts with bot hosts (add extra bot hosts)")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("-s", "--hosts", type=str, default=None,
                        help="Optional. List of hosts to check if they found in bot (for action showhosts)")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, default=[],
                        help="Optional. List of groups to remove old stuff (for action removeold)")
    parser.add_argument("-f", "--group-filter", type=argparse_types.pythonlambda, default=None,
                        help="Optional. Filter on groups to remove old stuff (for action removeold). Added to <--groups> group list")
    parser.add_argument("-v", "--verbose", action="store_true", default=False,
                        help="Optional. Add verbose output")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Apply changes")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    if options.hosts is None and options.action == EActions.SHOWHOSTS:
        raise Exception("You must specify --hosts option with %s action" % EActions.SHOWHOSTS)

    if options.hosts is not None:
        options.hosts = options.hosts.split(',')

    if options.action != EActions.SHOWHOSTS and options.hosts is not None:
        raise Exception("You can not specify <--hosts> option only with <--action %s> option" % EActions.SHOWHOSTS)

    if options.group_filter is not None:
        options.groups.extend(filter(options.group_filter, CURDB.groups.get_groups()))

    return options


def get_bot_hosts():
    result = []

    data = retry_urlopen(3, SETTINGS.services.oops.rest.hosts.url)
    for line in data.strip().split('\n'):
        bothost = TBotHost(line)
        if bothost.name != '':
            result.append(bothost)
    return result


def main(options, from_cmd=True):
    del from_cmd
    bot_hosts = set(get_bot_hosts())

    if options.action in [EActions.ADDNEW, EActions.SHOWDIFF, EActions.REMOVEOLD]:
        bot_hostnames = set(map(lambda x: x.name, bot_hosts))

        generator_hostnames = set(map(lambda x: x.name, CURDB.hosts.get_all_hosts()))
        generator_invnums = set(map(lambda x: x.invnum, CURDB.hosts.get_all_hosts()))

        if options.groups is not None:
            my_hosts = map(lambda x: x.getHosts(), options.groups)
        else:
            if options.action == EActions.SHOWDIFF:
                my_hosts = [filter(lambda x: not x.is_vm_guest(), CURDB.hosts.get_hosts())]
            else:
                my_hosts = map(lambda (flt, groupname): CURDB.groups.get_group(groupname).getHosts(), FILTERS)
        my_hosts = sum(my_hosts, [])

        my_hosts = filter(lambda x: not x.is_vm_guest(), my_hosts)
        my_hostnames = set(map(lambda x: x.name, my_hosts))

        extra_my_hostnames = sorted(list(my_hostnames - bot_hostnames))
        extra_bot_hosts = filter(lambda x: (x.name not in generator_hostnames) and (x.invnum not in generator_invnums), bot_hosts)
        extra_bot_hostnames = sorted(map(lambda x: x.name, extra_bot_hosts))

        print "Hosts count: in bot - %d, in gencfg - %d" % (len(bot_hostnames), len(CURDB.hosts.get_hosts()))
        print "Extra bot hosts: (%d): %s" % (len(extra_bot_hostnames), ','.join(extra_bot_hostnames[:200]))
        print "Extra my hosts (%d): %s" % (len(extra_my_hostnames), ','.join(extra_my_hostnames[:200]))

        if options.action == EActions.ADDNEW:
            # add new hosts and update hwdata
            update_hosts_options = {
                "action": "updateoops",
                "hosts": extra_bot_hostnames,
                "ignore_detect_fail": True,
                "verbose": options.verbose,
            }
            update_hosts.jsmain(update_hosts_options, False)

            update_hosts_options = {
                "action": "updatelocal",
                "hosts": extra_bot_hostnames,
                "ignore_detect_fail": True,
                "verbose": options.verbose,
            }
            update_hosts.jsmain(update_hosts_options, False)

            # Fix invalid hostanmes
            all_hostnames = set(map(lambda x: x.name, CURDB.hosts.get_all_hosts()))
            invalid_extra_hostnames = filter(lambda hostname: hostname not in all_hostnames, extra_bot_hostnames)
            extra_bot_hosts = filter(lambda x: x.name not in invalid_extra_hostnames, extra_bot_hosts)
            extra_bot_hostnames = sorted(map(lambda x: x.name, extra_bot_hosts))

            for hostname in extra_bot_hostnames:
                host = CURDB.hosts.get_host_by_name(hostname)
                host.memory = max(10, host.botmem)

            extra_bot_hosts = CURDB.hosts.get_hosts_by_name(extra_bot_hostnames)

            for flt, groupname in FILTERS:
                filtered = set(filter(flt, extra_bot_hosts))
                extra_bot_hosts = set(extra_bot_hosts) - set(filtered)
                group = CURDB.groups.get_group(groupname)
                for host in filtered:
                    group.addHost(host)
                print "Added %d hosts to %s: %s" % (
                len(filtered), groupname, ','.join(map(lambda x: x.name, list(filtered)[:200])))

                """
                    Some machines in unsorted can be renamed already to valid names.
                """
                filtered_unsorted = filter(flt, CURDB.groups.get_group(UNSORTED_GROUP).getHosts())
                if len(filtered_unsorted) > 0:
                    CURDB.groups.move_hosts(filtered_unsorted, group)
                    print "Added %d to %s from UNSORTED: %s" % (
                    len(filtered_unsorted), groupname, ','.join(map(lambda x: x.name, filtered_unsorted[:200])))

            """
                All hosts, which were not moved to specific group by pattern should go to unsorted group
            """
            if len(extra_bot_hosts) > 0:
                for host in extra_bot_hosts:
                    CURDB.groups.get_group(UNSORTED_GROUP).addHost(host)

            if options.apply:
                CURDB.hosts.mark_as_modified()
                CURDB.update(smart=True)
        elif options.action == EActions.REMOVEOLD:
            if options.groups is not None:
                processed_groups = options.groups
            else:
                processed_groups = map(lambda (_, groupname): CURDB.groups.get_group(groupname), FILTERS)

            for processed_group in processed_groups:
                group_hosts = set(map(lambda x: x.name, processed_group.getHosts()))
                filtered = list(group_hosts & set(extra_my_hostnames))
                if filtered:
                    print "Removed %d hosts from %s: %s" % (len(filtered), processed_group.card.name, ','.join(filtered[:200]))

            update_hosts_options = {
                "action": "remove",
                "hosts": extra_my_hostnames,
                "verbose": options.verbose,
            }
            update_hosts.jsmain(update_hosts_options, False)

            if options.apply:
                CURDB.update()
    elif options.action == EActions.SHOWHOSTS:
        my_hosts = set(options.hosts)

        not_found_in_bot = list(my_hosts - bot_hosts)
        found_in_bot = list(my_hosts & bot_hosts)

        print "Hosts found in bot (%d): %s" % (len(found_in_bot), ','.join(found_in_bot[:200]))
        print "Hosts not found in bot (%d): %s" % (len(not_found_in_bot), ','.join(not_found_in_bot[:200]))
    elif options.action == EActions.UPDATE_HOSTS_INFO:
        hosts_to_update = {host.name for host in CURDB.hosts.get_hosts() if host.model == 'unknown'}
        hosts_to_update |= {host.name for host in CURDB.hosts.get_hosts() if host.switch == 'unknown'}
        hosts_to_update = list(hosts_to_update)
        update_hosts_options = {
            "action": "updateoops",
            "hosts": hosts_to_update,
            "ignore_detect_fail": True,
            "verbose": options.verbose,
        }
        update_hosts.jsmain(update_hosts_options, False)

        update_hosts_options = {
            "action": "updatelocal",
            "hosts": hosts_to_update,
            "ignore_detect_fail": True,
            "verbose": options.verbose,
        }
        update_hosts.jsmain(update_hosts_options, False)

        update_hosts_options = {
            "action": "updatehb",
            "hosts": hosts_to_update,
            "ignore_detect_fail": True,
            "verbose": options.verbose
        }
        update_hosts.jsmain(update_hosts_options, False)

        if options.apply:
            CURDB.hosts.mark_as_modified()
            CURDB.update(smart=True)
    else:
        raise Exception("Unknown action %s" % options.action)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
