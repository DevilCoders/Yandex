#!./venv/venv/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
sys.path.append('/skynet')

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from utils.pregen import update_hosts
from gaux.aux_mongo import get_mongo_collection
from gaux.aux_reserve import get_reserve_group, move_transfered_hosts, redistribute_hosts


def parse_cmd():
    parser = ArgumentParser("Update hosts info and add hosts from prenalivka to reserved")
    parser.add_argument("-g", "--group", type=argparse_types.group, required=True,
                        help="Obligatory. Prenalivka group (use ALL_PRENALIVKA)")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum is 1)")
    parser.add_argument("--use-heartbeat-db", action="store_true", default=False,
                        help="Optional. Use heartbeat cache instead of checking via skynet")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Apply changes")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    hosts = map(lambda x: x.name, options.group.getHosts())

    update_hosts_action = 'update'
    if options.use_heartbeat_db:
        update_hosts_action = 'updatehb'
    update_hosts_options = {
        'action': update_hosts_action,
        'hosts': hosts,
        'ignore_detect_fail': True,
        'verbose': options.verbose
    }

    succ_hosts = []
    fail_hosts = []

    all_invnums = set(map(lambda x: x.invnum, CURDB.hosts.get_hosts()))
    all_invnums -= set(map(lambda x: x.invnum, options.group.getHosts()))
    if len(hosts) > 0:
        update_hosts_result = update_hosts.jsmain(update_hosts_options, False)
        succ_hosts = filter(lambda x: update_hosts_result[x]['status'] == update_hosts.EStatuses.SUCCESS,
                            update_hosts_result.iterkeys())
        succ_hosts = CURDB.hosts.get_hosts_by_name(succ_hosts)
        for hostname in update_hosts_result:
            if update_hosts_result[hostname]['status'] != update_hosts.EStatuses.SUCCESS:
                fail_hosts.append({'hostname': hostname, 'status': update_hosts_result[hostname]['status']})

    ready_to_use_hosts = filter(
        lambda x: x.invnum != 'unknown' and x.invnum not in all_invnums and x.model != 'unknown', succ_hosts)

    print "Updated group %s: %s failed, %s succ, %s ready to use" % (
    options.group.card.name, len(fail_hosts), len(succ_hosts), len(ready_to_use_hosts))
    print "Move %d hosts to reserved: %s" % (
    len(ready_to_use_hosts), ', '.join(map(lambda x: x.name, ready_to_use_hosts)))

    for host in ready_to_use_hosts:
        reserved_group_name = get_reserve_group(host)
        CURDB.groups.move_host(host, reserved_group_name)

    redistribute_hosts(CURDB)
    move_transfered_hosts(CURDB)

    if options.apply:
        CURDB.update(smart=True)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
