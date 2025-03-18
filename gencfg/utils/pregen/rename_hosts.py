#!/skynet/python/bin/python

# TODO: move to update_hosts

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import urllib2
import re
from multiprocessing import Pool
from argparse import ArgumentParser
import random
import string
import time

from multiprocessing import cpu_count

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from gaux.aux_utils import retry_urlopen
from core.settings import SETTINGS


class RenameInfo(object):
    def __init__(self, data):
        self.oldname, _, self.newname = data.partition(':')


def parse_cmd():
    parser = ArgumentParser(description="Script to change host names (by pairs oldname-newname or via bot service")

    parser.add_argument("-m", "--method", dest="method", type=str, required=True,
                        choices=["bot", "oops", "rawdata", "fake"],
                        help="Obligatory. Method for updating: <rawdata>  means, you specify comma-separated list of <oldname:newname>, <bot> means, it gets new names from bot")
    parser.add_argument("-s", "--hosts", dest="hosts", type=argparse_types.hosts,
                        help="Optional. List of hosts to check for new names")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter on hosts to rename")
    parser.add_argument("-g", "--groups", dest="groups", type=argparse_types.groups,
                        help="Optional. List of groups with hosts to check for new names")
    parser.add_argument("-r", "--rawdata", dest="rawdata", type=str,
                        help="Optional. Comma-separated pairs of hosts in format <old>:<new>")
    parser.add_argument("-i", "--ignore-rename-to-existing", action="store_true", default=False,
                        help="Optional. Ignore cases when new host name already found in DB (as different host)")
    parser.add_argument("-v", "--verbose", action="store_true", default=False,
                        help="Optional. Show verbose output")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    if options.method in ["bot", "fake"]:
        if options.hosts is None and options.groups is None:
            raise Exception("You must specify --hosts or --groups with <--method bot> or <--method fake>")
        if options.rawdata is not None:
            raise Exception("You can not specify --rawdata with <--method bot> or <--method fake>")
    elif options.method == "rawdata":
        if options.rawdata is None:
            raise Exception("You must specify --rawdata with <--method rawdata>")
        if options.hosts is not None or options.groups is not None:
            raise Exception("Yuo can not specify --hosts or --groups with <--method rawdata>")

    return options


def _get_new_host_name(host):
    try:
        if host.invnum == 'unknown':
            return None
        url = "http://bot.yandex-team.ru/api/infobyinv.php?inv=%s" % host.invnum

        COUNT_RETRY = 3
        for num_try in range(COUNT_RETRY):
            try:
                data = urllib2.urlopen(url, timeout=10).read()
                break
            except Exception as e:
                if num_try + 1 == COUNT_RETRY:
                    raise e
                print('RETRY {}: {}'.format(num_try, e))
                time.sleep(3)

        newname = re.search("\[Naimenovanie\] => (.*)", data).group(1)
        return newname.lower()
    except:
        return None


def get_oops_names(invnums):
    """Get names from oops dumps by invnums"""
    oops_data = retry_urlopen(3, SETTINGS.services.oops.rest.hosts.url)
    names_by_invnum = dict()
    for line in oops_data.strip().split('\n'):
        invnum = line.split('\t')[0]
        name = line.split('\t')[1].lower()
        if name == '':
            name = None
        names_by_invnum[invnum] = name

    return [names_by_invnum.get(x, None) for x in invnums]


def main(options):
    if options.method == "rawdata":
        rename_data = dict(map(lambda x: (x.split(':')[0], RenameInfo(x)), options.rawdata.split(',')))
    elif options.method in ["bot", "oops", "fake"]:
        hosts = []
        if options.hosts:
            hosts.extend(options.hosts)
        if options.groups:
            hosts.extend(sum(map(lambda x: x.getHosts(), options.groups), []))
        hosts = filter(options.filter, hosts)
        hosts = list(set(hosts))

        oldnames = map(lambda x: x.name, hosts)

        if options.method == "bot":
            p = Pool(cpu_count())
            newnames = p.map(_get_new_host_name, hosts)
        elif options.method == 'oops':
            newnames = get_oops_names([x.invnum for x in hosts])
        elif options.method == "fake":
            newnames = map(lambda x: "fake-%s.%s" % (
            ''.join(random.choice(string.ascii_lowercase + string.digits) for _ in range(16)), x.partition('.')[2]),
                           oldnames)

        rename_data = {}
        for oldname, newname in zip(oldnames, newnames):
            if newname is None:
                if options.verbose:
                    print "No data for host %s, skipping" % oldname
            elif newname == '':
                if options.verbose:
                    print "Empty host name for %s invnum, skipping" % oldname
            elif oldname != newname:
                rename_data[oldname] = RenameInfo('%s:%s' % (oldname, newname))
                print "Need to rename: %s -> %s" % (oldname, newname)

    CURDB.groups.get_groups()
    CURDB.intlookups.get_intlookups()

    # find affected groups/intlookups and mark them as modified
    affected_groups = map(lambda x: CURDB.groups.get_host_groups(CURDB.hosts.get_host_by_name(x)), rename_data.keys())
    affected_groups = list(set(sum(affected_groups, [])))
    for affected_group in affected_groups:
        affected_group.mark_as_modified()

    affected_intlookups = list(set(sum(map(lambda x: x.card.intlookups, affected_groups), [])))
    affected_intlookups = map(lambda x: CURDB.intlookups.get_intlookup(x), affected_intlookups)
    for affected_intlookup in affected_intlookups:
        affected_intlookup.mark_as_modified()

    # find hosts with renames (<host1> -> <host2> and <host2> -> <host1>) an proceed them first
    switched_names = set()
    for k in rename_data:
        if rename_data[k].newname in rename_data and rename_data[rename_data[k].newname] == k and k not in switched_names:
            host1 = CURDB.hosts.get_host_by_name(k)
            host2 = CURDB.hosts.get_host_by_name(rename_data[k].newname)

            CURDB.hosts.remove_host(host1)
            CURDB.hosts.remove_host(host2)
            host1.swap_data(host2)
            CURDB.hosts.add_host(host1)
            CURDB.hosts.add_host(host2)
            host1.invnum, host2.invnum = host2.invnum, host1.invnum

            CURDB.ipv4tunnels.switch_hostname(host1.name, host2.name)

            switched_names.add(k)
            switched_names.add(rename_data[k].newname)
    for k in switched_names:
        rename_data.pop(k)

    # add new hosts
    for i in xrange(10):
        if not len(rename_data):
            break
        for oldname in sorted(rename_data.keys()):
            newname = rename_data[oldname].newname
            if newname in rename_data:
                continue

            if not CURDB.hosts.has_host(oldname):
                raise Exception("Trying to rename from non-existing host %s" % oldname)
            if CURDB.hosts.has_host(newname):
                if options.ignore_rename_to_existing:
                    print "Can not rename: %s -> %s (host %s already exists)" % (oldname, newname, newname)
                    continue
                else:
                    raise Exception("Trying to rename %s into existing host %s" % (oldname, newname))

            CURDB.hosts.rename_host(CURDB.hosts.get_host_by_name(oldname), newname)
            CURDB.ipv4tunnels.rename_host(oldname, newname)
            rename_data.pop(oldname)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
    CURDB.update(smart=True)
