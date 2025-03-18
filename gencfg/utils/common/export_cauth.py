#!/skynet/python/bin/python
# coding=utf-8
"""
    Utility to import gencfg groups ownership to cauth. This utility generates pair of files:
      - group.card.owners (list of owners for every gencfg group)
      - groupы.xml (list of hosts for every group)
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import hashlib
import StringIO
import subprocess
import requests
import uuid
import itertools
import random
import time
import urllib2
import json
import zlib
import copy
import config

from collections import defaultdict
from xml.etree import ElementTree
from functools import wraps

import gencfg
from core.argparse.parser import ArgumentParserExt
from core.db import DB, CURDB
from gaux.aux_utils import retry_urlopen, get_last_sandbox_resource
import gaux.aux_mongo
from core.settings import SETTINGS
import gaux.aux_staff
import gaux.aux_portovm
import gaux.aux_abc
import gaux.aux_hbf
from gaux.aux_colortext import red_text

TRIES = 3  # number of attempts for load remote url
SEARCH_DOMAIN = 10
BSCONFIG_PATH = "/db/bin/bsconfig"
DEFAULT_HOSTS_FILE = 'group.hosts'
DEFAULT_OWNERS_FILE = 'group.card.owners'


class EActions(object):
    EXPORT = "export"
    ALL = [EXPORT]


def get_parser():
    parser = ArgumentParserExt(description="Export gencfg groups data to cauth")

    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("-b", "--bsconfig", type=str, default=BSCONFIG_PATH,
                        help="Optional. Path to bsconfig (if not specified, %s will be used)" % BSCONFIG_PATH)
    parser.add_argument("--hosts-name", type=str, default=DEFAULT_HOSTS_FILE,
                        help="Optional. Filename with xml, describing groups hosts ({} by default)".format(DEFAULT_HOSTS_FILE))
    parser.add_argument("--owners-name", type=str, default=DEFAULT_OWNERS_FILE,
                        help="Optinoal. Filename with list of owners for every group ({} by default)".format(DEFAULT_OWNERS_FILE))
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum 1)")

    return parser


def logtime(fn):
    import time
    import math

    def exec_time(start_time):
        return str(math.trunc((time.time() - start_time) * 1000) / 1000.) + 's'

    @wraps(fn)
    def inner(*args, **kwargs):
        stime = time.time()
        retval = fn(*args, **kwargs)
        print '{0}: {1}'.format(fn.__name__, exec_time(stime))
        return retval

    return inner


class UniqueIds(object):
    def __init__(self):
        self._used_ids = dict()

    def id(self, string):
        md5sum = hashlib.md5(string).hexdigest()

        id_ = int(md5sum, 16) & (2 ** 31 - 1)

        fail = True
        for i in range(3):
            if self._used_ids.get(id_ + i, string) != string:
                continue
            else:
                self._used_ids[id_ + i] = string
                fail = False
                id_ += i
                break

        if fail:
            raise RuntimeError('ID conflict for %s and %s' % (string, self._used_ids[id_]))

        return str(id_)


@logtime
def get_group_owners(data):
    real_users = set(aux_staff.get_possible_group_owners())

    def parse_line(line):
        group, owners = line.rstrip().split('\t')[:2]
        owners = set(owners.split(','))

        # ======================== TOOLSUP-20920 START ================================
        dpts = owners - real_users
        owners = owners & real_users
        for dpt in dpts:
            dpt_users = get_dpt_users(dpt)
            print 'Department <{}> users: {}'.format(dpt, ' '.join(dpt_users))
            owners |= set(dpt_users)
        # ======================== TOOLSUP-20290 FINISH ===============================

        return group, owners

    return defaultdict(set, {group: owners for group, owners in (parse_line(line) for line in data)})


@logtime
def get_tag_hosts(dump, fqdns):
    result = defaultdict(set)

    parsed_itag = None
    for line in dump:
        line = line.rstrip()
        if not line:
            continue

        if line.startswith('%instance_tag'):
            parsed_itag = line.split()[1].strip()
            if not parsed_itag.isupper():
                parsed_itag = None
            continue

        if not ':' in line:
            parsed_itag = None
            continue

        host = line[:line.find(':')].strip()

        # CAUTH-826 (big hack)
        new_host = host + fqdns.get(host, '.yandex.ru')
        if not new_host.endswith('.qloud-h.yandex.net'):
            result[parsed_itag].add(new_host)
        # result[parsed_itag].add(host + fqdns.get(host, '.yandex.ru'))

    result.pop(None, None)

    return result


def indent(elem, level=0):
    i = "\n" + level * "  "

    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "  "

        if not elem.tail or not elem.tail.strip():
            elem.tail = i

        for elem in elem:
            indent(elem, level + 1)

        if not elem.tail or not elem.tail.strip():
            elem.tail = i

    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i


@logtime
def dump_host_groups(groups, output_file):
    ui = UniqueIds()

    root = ElementTree.Element("domain", attrib=dict(name='search', id=str(SEARCH_DOMAIN)))

    for group in sorted(groups.keys()):
        element = ElementTree.SubElement(root, 'group', attrib=dict(id=ui.id(group), name=group, parent_id=""))

        for host in sorted(groups[group]):
            ElementTree.SubElement(element, 'host', attrib=dict(id=ui.id(host), name=host))
            if host.endswith('.yandex.ru'):
                host2 = host.rsplit('.yandex.ru', 1)[0] + '.search.yandex.net'
                ElementTree.SubElement(element, 'host', attrib=dict(id=ui.id(host2), name=host2))

    indent(root)
    tree = ElementTree.ElementTree(root)

    fd = open(output_file, 'w')
    tree.write(fd, encoding="UTF-8", xml_declaration=True)
    fd.close()


@logtime
def dump_group_owners(owners, output_file):
    f = open(output_file, 'w')
    for group in sorted(owners.keys()):
        lst = sorted(list(owners[group]))
        lst = [x.encode('utf8') for x in lst]
        print >> f, "%s\t%s" % (group.encode('utf8'), ','.join(lst))
    f.close()


def get_gencfg_last_released_tag():
    jsoned = json.loads(retry_urlopen(TRIES, "%s/unstable/tags" % SETTINGS.backend.api.url))
    return jsoned["tags"][0]


def get_gencfg_hosts_owners(dbname):
    print "Processing db <%s>" % dbname

    db = DB(dbname)

    groups = filter(lambda x: x.card.properties.nonsearch == False or x.card.properties.export_to_cauth == True or
                              x.card.properties.mtn.export_mtn_to_cauth == True,
                    db.groups.get_groups())
    # =========================== RX-219 START ==========================================
    groups = [x for x in groups if x.card.properties.security_segment in ('normal', 'infra')]
    # =========================== RX-219 FINISH =========================================

    # =========================== RX-141 START ==========================================
    group_owners = dict()
    group_hosts = dict()
    for group in groups:
        if group.card.properties.created_from_portovm_group:
            continue

        # process owners
        extended_owners = gaux.aux_staff.unwrap_dpts(group.card.owners)
        group_owners[group.card.name] = extended_owners
        if group.has_portovm_guest_group():
            group_owners['{}_GUEST'.format(group.card.name)] = extended_owners + gaux.aux_staff.unwrap_dpts(group.card.guest.owners)

        # process hosts
        # do not export Dom0-hosts for groups with export_mtn_to_cauth == True and nonserach == True (RX-456)
        if group.card.properties.nonsearch == False or group.card.properties.export_to_cauth == True:
            hosts = group.getHosts()
            group_hosts[group.card.name] = [x.name for x in group.getHosts()]
            if group.has_portovm_guest_group():
                group_hosts['{}_GUEST'.format(group.card.name)] = [gaux.aux_portovm.gen_vm_host_name(db, x) for x in group.get_instances()]
        else:
            group_hosts[group.card.name] = []

        # ========================================== GENCFG-1861 START ===================================================
        if hasattr(group.card.properties.mtn, 'export_mtn_to_cauth') and group.card.properties.mtn.export_mtn_to_cauth:
            instances = group.get_kinda_busy_instances()

            # ============================================== RX-336 START ================================================
            # skip hosts with mtn hostname based on mtn add and without vlan
            bad_instances = [x for x in instances if ('vlan688' not in x.host.vlans) and (x.host.mtn_fqdn_version > 0)]
            bad_instances.sort(key=lambda x: x.full_name())
            if len(bad_instances):
                print red_text('Could no generate mnt hostname for <{}>'.format(','.join(x.full_name() for x in bad_instances)))
            bad_instances = set(bad_instances)
            good_instances = [x for x in instances if x not in bad_instances]
            # ============================================== RX-336 FINISH ===============================================

            group_hosts[group.card.name].extend([gaux.aux_hbf.generate_mtn_hostname(x, group, '') for x in good_instances])
        # ========================================== GENCFG-1861 FINISH ==================================================
    # =========================== RX-141 FINISH =========================================

    return group_owners, group_hosts


def get_nanny_services():
    """Get nanny services info from last resource of GENCFG_NANNY_SERVICES (RX-384)"""


def main(options):
    # get default domains for hosts (hosts, loaded from bsconfig without domain, so we should guest correct one)
    default_fqdns = dict(map(lambda x: (x.name.partition('.')[0], x.domain), CURDB.hosts.get_hosts()))

    result_owners = defaultdict(set)
    result_hosts = defaultdict(set)

    data = [
        get_gencfg_hosts_owners("tag@%s" % get_gencfg_last_released_tag()),
        get_gencfg_hosts_owners("commit@%s" % gaux.aux_mongo.get_last_verified_commit()),
    ]
    for owners, hosts in data:
        for k, v in owners.iteritems():
            result_owners[k] |= set(v)
        for k, v in hosts.iteritems():
            result_hosts[k] |= set(v)

    # ============================================ RX-354 START ===========================================
    # ============================================ RX-384 START ===========================================
    nanny_services = get_last_sandbox_resource('GENCFG_NANNY_SERVICES')
    nanny_services = json.loads(nanny_services)
    for service in nanny_services:
        result_owners['srv_' + service['name']] = gaux.aux_staff.unwrap_dpts(service['owners'])
        result_hosts['srv_' + service['name']] = set(service['hosts'])
    # ============================================ RX-384 STOP ============================================
    # ============================================ RX-354 STOP ============================================

    dump_group_owners(result_owners, options.owners_name)
    dump_host_groups(result_hosts, options.hosts_name)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)
