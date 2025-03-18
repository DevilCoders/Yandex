#!/skynet/python/bin/python
"""
    Script to manipulate HBF aliases (NOCDEV-479). Can perform following actions:
        - show current HBF state and compare with gencfg state
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import urllib2
from collections import defaultdict
import json
import re
import time

import gencfg
import config
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.settings import SETTINGS
from gaux.aux_utils import retry_urlopen, retry_requests_get
from gaux.aux_colortext import red_text
from gaux.aux_hbf import hbf_group_macro_name
from gaux.aux_mongo import get_next_hbf_project_id


MACROS_PATTERN = '^_GENCFG_([A-Z0-9_]+)_$'


def IS_TEST_RX_475(name_group_or_macro):
    return ('TEST_RX_475' in name_group_or_macro) or ('_GENCFG_MONITORING' in name_group_or_macro) or ('_GENCFG_NOTIFY_BOT' in name_group_or_macro)


def __get_macros_for_update(macros_name):
    """Auxiliarily function to get macros from macros name"""
    if not macros_name:
        raise Exception('You must specify <--macros> param in action {}'.format(EActions.UPDATEMACROS))
    if not CURDB.hbfmacroses.has_hbf_macros(macros_name):
        raise Exception('Macros <{}> does not exists'.format(macros_name))

    macros = CURDB.hbfmacroses.get_hbf_macros(macros_name)

    if macros.external or macros.group_macro:
        raise Exception('Can not update external/group macros <{}>'.format(macros.name))

    return macros


def __request_hbf(path, data=None, raise_failed=True):
    """Send request to hbf"""

    headers = dict(Authorization='OAuth {}'.format(config.get_default_oauth()))
    url = '{}?{}'.format(SETTINGS.services.racktables.hbf.rest.url, path)

    print 'Executing {} with data {}'.format(url, data)

    req = urllib2.Request(url, data, headers)
    try:
        retry_urlopen(3, req)
    except Exception, e:
        if raise_failed:
            raise
        else:
            print red_text(e)


class EActions(object):
    DUMP = "dump"  # dump current state of hbf

    # macros manipulation actions
    SHOWMACROS = "showmacros" # show macros info
    UPDATEMACROS = "updatemacros" # update macros info
    ADDMACROS = "addmacros" # add new macros
    REMOVEMACROS = "removemacros" # remove macros

    # sync macroses
    SYNCMACROSES = "syncmacroses" # sync macroses with hbf

    SHOW = "show"
    # push groups/macroses info into racktables
    UPDATE = "update"

    FIXDB = "fixdb"  # fix gencfg db, e. g. change hbf id for groups with different hbf id in hbf

    ALL = [DUMP, SHOW, SHOWMACROS, UPDATEMACROS, ADDMACROS, REMOVEMACROS, SYNCMACROSES, UPDATE, FIXDB]


class THbfGroupInfo(object):
    """
        Class, representing current state for group in hbf
    """

    class EStatus:
        OK = 0 # set when group in hbf already and project id is ok
        NOT_IN_HBF = 1 # set when group not in hbf and project id is not found in hbf as well
        WRONG_PRJ = 2 # group in hbf, but its project id differs from gencfg one
        USED_PRJ = 3 # group not in hbf, but its prj corresponds to some other entity

    __slots__ = ['group', 'status', 'msg', 'hbf_info']

    def __init__(self, group, hbf_groups, hbf_prjs):
        """
            Initialize Group state, based on current hbf state

            :param group: gencfg group
            :param hbf_groups: dict with mapping (groupname -> hbf_project_id)
            :param hbf_prjs: dict with mapping (hbf_project_id -> groupname)
        """

        self.group = group
        self.hbf_info = dict()

        stripped_name = self.group.card.name.strip('_')
        if stripped_name in hbf_groups:
            self.hbf_info = hbf_groups[stripped_name]
            if hbf_groups[stripped_name]["id"] == self.group.card.properties.hbf_project_id:
                self.status = THbfGroupInfo.EStatus.OK
                self.msg = "Group <%s> is already in hbf"
            else:
                self.status = THbfGroupInfo.EStatus.WRONG_PRJ
                self.msg = "Group <%s> has prj <%s> in card and <%s> in hbf" % (
                    self.group.card.name, self.group.card.properties.hbf_project_id,
                    hbf_groups[stripped_name]['id']
                )
        elif self.group.card.properties.hbf_project_id in hbf_prjs:
            self.status = THbfGroupInfo.EStatus.USED_PRJ
            self.msg = "Group <%s> has prj <%s>, corresponding to <%s> in hbf" % (
                self.group.card.name,
                self.group.card.properties.hbf_project_id,
                hbf_prjs[self.group.card.properties.hbf_project_id]
            )
        else:
            self.status = THbfGroupInfo.EStatus.NOT_IN_HBF
            self.msg = "Group <%s> not in hbf yet" % self.group.card.name


def get_parser():
    parser = ArgumentParserExt(description="Perform various actions with hbf")
    parser.add_argument("-a", "--action", type=str, default = EActions.SHOW,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("--db", type=argparse_types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, default="ALL",
                        help="Optional. List of groups to process (all groups are processed by default)")
    parser.add_argument("-m", "--macros", type=str, default=None,
                        help="Optional. Macros name (for actions {})".format(','.join([EActions.SHOWMACROS, EActions.UPDATEMACROS, EActions.ADDMACROS, EActions.REMOVEMACROS])))
    parser.add_argument("-p", "--parent-macros", type=argparse_types.gencfg_hbf_macros, default=None,
                        help="Optional. Parent macros (for actions {})".format(','.join([EActions.ADDMACROS, EActions.UPDATEMACROS])))
    parser.add_argument('-d', '--descr', type=str, default=None,
                        help='Optional. Macros description')
    parser.add_argument("-o", "--owners", type=argparse_types.comma_list, default=None,
                        help="Optional. Comma-separated list of owners to set (for actions {}, {})".format(EActions.UPDATEMACROS, EActions.ADDMACROS))
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum 1)")

    return parser


def get_hbf_group_list(if_modified_since=None):
    """Get hbf group list

    :param if_modified_since: timestamp for If-Modified-Since header (NOCDEV-952)"""

    url = "%s?op=list" % SETTINGS.services.racktables.hbf.rest.url

    headers = dict()
    headers["Authorization"] = "OAuth %s" % config.get_default_oauth()
    if if_modified_since is not None:
        headers['If-Modified-Since'] = time.strftime('%a, %d %b %Y %H:%M:%S GMT', time.gmtime(if_modified_since))

    req = urllib2.Request(url, None, headers)

    try:
        data = urllib2.urlopen(req).read()
    except urllib2.HTTPError as e:
        if if_modified_since and e.code == 304:
            return None
        else:
            raise

    result = json.loads(data)

    # ================================== GENCFG-2390 START =======================================
    all_macroses_data = retry_requests_get(3, SETTINGS.services.racktables.hbf.rest.all_macroses_url).json()
    for name, data in all_macroses_data.iteritems():
        if name in result:
            continue

        owners = [elem['name'] for elem in data['owners'] if elem['type'] in ('user', 'department')]

        result[name] = dict(
            id=None,
            name=name,
            section=name,
            owners=owners,
            parent=None,
            description='',
            internet=False,
        )
    # ================================== GENCFG-2390 FINISH ======================================

    return result


def load_hbf_info():
    hbf_groups = dict()
    hbf_prjs = dict()

    jsoned = get_hbf_group_list()
    all_macroses = jsoned.keys()

    jsoned = {x: y for x, y in jsoned.iteritems() if x.startswith('_GENCFG_')}
    for macro_name in jsoned:
        m = re.match("^_GENCFG_(.*)_$", str(macro_name))
        if not m:
            raise Exception("Macro <%s> does not satisfy gencfg marco regex" % macro_name)

        group_name = m.group(1)
        if group_name in hbf_groups:
            raise Exception("Group <%s> found at least twice in hbf project list" % group_name)

        hbf_prj = int(jsoned[macro_name]["id"], 16)
        if hbf_prj in hbf_prjs:
            raise Exception,("Hbf prj <%s> found at least twice in hbf project list" % hbf_prj)

        hbf_prjs[hbf_prj] = group_name
        hbf_groups[group_name] = jsoned[macro_name]
        hbf_groups[group_name]["id"] = hbf_prj

    return hbf_groups, hbf_prjs, all_macroses


def main_show(hbf_groups_info, options):
    by_statuses = defaultdict(list)

    for hbf_group_info in hbf_groups_info:
        by_statuses[hbf_group_info.status].append(hbf_group_info)

    print "Total groups: %s" % len(hbf_groups_info)

    print "    Groups already in hbf: %s" % len(by_statuses[THbfGroupInfo.EStatus.OK])

    print "    Groups not in hbf yet: %s" % len(by_statuses[THbfGroupInfo.EStatus.NOT_IN_HBF])
    if options.verbose > 0:
        for elem in by_statuses[THbfGroupInfo.EStatus.NOT_IN_HBF]:
            print '        {}'.format(elem.msg)

    print "    Groups with wrong hbf prj: %s" % len(by_statuses[THbfGroupInfo.EStatus.WRONG_PRJ])
    if options.verbose > 0:
        for elem in by_statuses[THbfGroupInfo.EStatus.WRONG_PRJ]:
            print '        {}'.format(elem.msg)

    print "    Groups with prj used by another group: %s" % len(by_statuses[THbfGroupInfo.EStatus.USED_PRJ])
    if options.verbose > 0:
        for elem in by_statuses[THbfGroupInfo.EStatus.USED_PRJ]:
            print '        {}'.format(elem.msg)


def main_update(hbf_groups_info, all_macroses, options):
    # add new macroses
    for macros in CURDB.hbfmacroses.get_hbf_macroses():
        if macros.external or macros.group_macro:
            continue
        if macros.name not in all_macroses and not IS_TEST_RX_475(macros.name):
            print 'Adding macros {}'.format(macros.name)
            path = 'op=create&name=%s&id=%x' % (macros.name, macros.hbf_project_id)
            __request_hbf(path, data='', raise_failed=False)

    # update macroses owners, parent
    """
    for macros in CURDB.hbfmacroses.get_hbf_macroses():
        if macros.external or macros.group_macro:
            continue
        if macros.name != '_GENCFG_SEARCHPRODNETS_ROOT_' and not IS_TEST_RX_475(macros.name):
            print 'Updating macros {} parent/owners'.format(macros.name)
            path = 'op=set_owners&name={}&owners={}'.format(macros.name, ','.join(macros.owners))
            __request_hbf(path, data='', raise_failed=False)

            if macros.parent_macros is not None:
                path = 'op=set_parent&name={}&parent={}'.format(macros.name, macros.parent_macros)
                __request_hbf(path, data='', raise_failed=False)
    """

    # update info on all groups
    for hbf_group_info in hbf_groups_info:
        if IS_TEST_RX_475(hbf_group_info.group.card.name):
            continue
        if hbf_group_info.status == THbfGroupInfo.EStatus.USED_PRJ:
            print red_text(hbf_group_info.msg)
            continue
        if hbf_group_info.status in (THbfGroupInfo.EStatus.OK, THbfGroupInfo.EStatus.WRONG_PRJ, THbfGroupInfo.EStatus.USED_PRJ):
            # update owners list
            converted_owners = sorted([x.replace('abc:', 'svc_') for x in hbf_group_info.group.card.owners])
            if set(converted_owners) != set(hbf_group_info.hbf_info.get('owners', [])):
                # update owners
                print 'Updating owners for {}'.format(hbf_group_info.group.card.name)
                path = 'op=set_owners&name={}&owners={}'.format(
                    hbf_group_macro_name(hbf_group_info.group),
                    ",".join(converted_owners),
                )
                __request_hbf(path, data='', raise_failed=False)

            # update parents
            if hbf_group_info.group.card.properties.hbf_parent_macros != hbf_group_info.hbf_info.get('parent', None):
                print 'Updating parent for {}'.format(hbf_group_info.group.card.name)
                path = 'op=set_parent&name={}&parent={}'.format(
                    hbf_group_macro_name(hbf_group_info.group),
                    hbf_group_info.group.card.properties.hbf_parent_macros,
                )
                __request_hbf(path, data='', raise_failed=False)

            # update internet property
            if int(hbf_group_info.group.card.properties.internet_tunnel) != hbf_group_info.hbf_info.get('internet', None):
                print 'Updating internet for group {} to {}'.format(hbf_group_info.group.card.name, hbf_group_info.group.card.properties.internet_tunnel)
                path = 'op=set_internet&name={}&internet={}'.format(
                    hbf_group_macro_name(hbf_group_info.group),
                    int(hbf_group_info.group.card.properties.internet_tunnel),
                )
                __request_hbf(path, data='', raise_failed=False)

            # print warning if needed
            if hbf_group_info.status in (THbfGroupInfo.EStatus.WRONG_PRJ, THbfGroupInfo.EStatus.USED_PRJ):
                print red_text(hbf_group_info.msg)

            continue

        print "Adding group %s" % hbf_group_info.group.card.name

        path = "op=create&name=%s&id=%x&owners=%s&internet=%d" % (
            hbf_group_macro_name(hbf_group_info.group),
            hbf_group_info.group.card.properties.hbf_project_id,
            ",".join(hbf_group_info.group.card.owners),
            int(hbf_group_info.group.card.properties.internet_tunnel),
        )
        if hbf_group_info.group.card.properties.hbf_parent_macros is not None:
             path = '{}&parent={}'.format(path, hbf_group_info.group.card.properties.hbf_parent_macros)

        __request_hbf(path, data='', raise_failed=False)


def main_fixdb(hbf_groups_info, options):
    current_hbf_ids = {x.card.properties.hbf_project_id for x in CURDB.groups.get_groups()}

    for hbf_group_info in hbf_groups_info:
        if IS_TEST_RX_475(hbf_group_info.group.card.name):
            continue
        if hbf_group_info.status == THbfGroupInfo.EStatus.WRONG_PRJ:  # try to change hbf project id in gencfg
            hbf_info = hbf_group_info.hbf_info
            group = hbf_group_info.group
            if hbf_info["id"] not in current_hbf_ids:
                print 'Group <{}>: changing hbf project id from <{}> to <{}>'.format(group.card.name, group.card.properties.hbf_project_id, hbf_info["id"])

                group.card.properties.hbf_project_id = hbf_info["id"]
                group.card.properties.mtn_fqdn_version = CURDB.groups.SCHEME.get_cached().resolve_scheme_path(['properties', 'mtn_fqdn_version']).default
                group.mark_as_modified()

                current_hbf_ids.add(hbf_info["id"])
            else:
                print red_text('Group <{}>: can not changed hbf project id to <{}> (already in base)'.format(group.card.name, hbf_info["id"]))
        elif hbf_group_info.status == THbfGroupInfo.EStatus.USED_PRJ:  # change hbf project id to new one
            hbf_group_info.group.card.properties.hbf_project_id = get_next_hbf_project_id(hbf_group_info.group.card.properties.hbf_range)
            hbf_group_info.group.mark_as_modified()

    CURDB.groups.update(smart=True)


def main_showmacros(options):
    if not options.macros:
        raise Exception('You must specify <--macros> param in action {}'.format(EActions.SHOWMACROS))

    hbf_group_list = get_hbf_group_list()

    if options.macros not in hbf_group_list:
        raise Exception('Macros <{}> not found in hbf group list'.format(options.macros))

    macros_info = hbf_group_list[options.macros]
    print 'Macros {}: id={} owners={}'.format(macros_info['name'], macros_info['id'], ','.join(macros_info['owners']))


def main_updatemacros(options):
    macros = __get_macros_for_update(options.macros)

    if options.owners is not None:
        macros.owners = options.owners

    if options.parent_macros is not None:
        if options.parent_macros in macros.recurse_child_macroses:
            raise Exception('Trying to set macro <{}> parent to one if its children <{}> , thus making cycle'.format(macros.name, options.parent_macros.name))
        else:
            macros.parent_macros = options.parent_macros.name

    CURDB.hbfmacroses.mark_as_modified()

    CURDB.update(smart=True)


def main_addmacros(options):
    if not options.macros:
        raise Exception('You must specify <--macros> param in action'.format(EActions.ADDMACROS))
    if CURDB.hbfmacroses.has_hbf_macros(options.macros):
        raise Exception('Macros <{}> already exists'.format(options.macros))
    if not re.match(MACROS_PATTERN, options.macros):
        raise Exception('Macros <{}> does not satisfy regex <{}>'.format(options.macros, MACROS_PATTERN))

    macros = CURDB.hbfmacroses.add_hbf_macros(options.macros, options.descr, options.owners, parent_macros=options.parent_macros)
    CURDB.update(smart=True)


def main_removemacros(options):
    if not options.macros:
        raise Exception('You must specify <--macros> param in action'.format(options.action))
    if not CURDB.hbfmacroses.has_hbf_macros(options.macros):
        raise Exception('Macros <{}> does not exists'.format(options.macros))

    macros = CURDB.hbfmacroses.get_hbf_macros(options.macros)
    if macros.removed:
        raise Exception('Macros <{}> is already removed'.format(macros.name))

    groups_with_hbf_macros = filter(lambda x: x.card.properties.hbf_parent_macros == macros.name, CURDB.groups.get_groups())
    if len(groups_with_hbf_macros) > 0:
        raise Exception("Can not remove hbf macros <%s>, which is used by <%s>" % (
            macros.name, ",".join(map(lambda x: x.card.name, groups_with_hbf_macros))))

    child_macroses = macros.child_macroses
    if child_macroses:
        raise Exception('Can not remove macros {}: macros has children {}'.format(macros.name, ' '.join(x.name for x in child_macroses)))

    macros.removed = True

    CURDB.hbfmacroses.mark_as_modified()
    CURDB.update(smart=True)


def main_syncmacroses(options):
    """Sync hbf macroses into gencfg database"""

    gencfg_group_macroses = {hbf_group_macro_name(x) for x in CURDB.groups.get_groups()}

    # all external macroses
    external_macroses = dict()
    for macros_name, macros_data in get_hbf_group_list().iteritems():
        if IS_TEST_RX_475(macros_name):
            continue
        name = macros_name.encode('utf8')
        description = macros_data['description'].encode('utf8')
        description = description.replace('\r\n', '\n')
        owners = [x.encode('utf8') for x in macros_data['owners']]
        parent = macros_data['parent'].encode('utf8') if macros_data['parent'] is not None else None
        if macros_data['id'] is None:
            id_ = None
        else:
            id_ = int(macros_data['id'], 16)
        external_macroses[name] = dict(name=name, description=description, owners=owners, parent=parent, id=id_)
    external_macroses = {x: y for x, y in external_macroses.iteritems() if x not in gencfg_group_macroses}
    # all gencfg
    gencfg_macroses = {x.name: x for x in CURDB.hbfmacroses.get_hbf_macroses() if not IS_TEST_RX_475(x.name)}

    # find extra gencfg/extra
    common_macroses = set(external_macroses.keys()) & set(gencfg_macroses.keys())
    extra_external_macroses = set(external_macroses.keys()) - set(gencfg_macroses.keys())
    extra_gencfg_macroses = set(gencfg_macroses.keys()) - set(external_macroses.keys())

    # process new external macroses
    for k in extra_external_macroses:
        print 'Adding new external macros <{}>'.format(k)
        macros_info = external_macroses[k]
        new_macros = CURDB.hbfmacroses.add_hbf_macros(macros_info['name'], macros_info['description'], macros_info['owners'], parent_macros=macros_info['parent'],
                                                      hbf_project_id=macros_info['id'])
        new_macros.external = True

    # process extra gencfg macroses
    for k in extra_gencfg_macroses:
        macros = gencfg_macroses[k]
        if macros.external:
            print 'Removing old external macros <{}>'.format(k)
            CURDB.hbfmacroses.remove_hbf_macros(macros.name)

    # process common macroses
    for k in common_macroses:
        external_macros = external_macroses[k]
        gencfg_macros = gencfg_macroses[k]
        # modify only external macroses
        if gencfg_macros.external:
            if (gencfg_macros.owners != external_macros['owners']) or \
               (gencfg_macros.parent_macros != external_macros['parent']) or \
               (gencfg_macros.description.strip() != external_macros['description'].strip()):
                    print 'Updating properties of external macros <{}>'.format(k)
                    gencfg_macros.owners = external_macros['owners']
                    gencfg_macros.parent_macros = external_macros['parent']
                    gencfg_macros.description = external_macros['description']

    CURDB.hbfmacroses.mark_as_modified()
    CURDB.update(smart=True)


def main(options):
    if options.action == EActions.DUMP:
        print json.dumps(get_hbf_group_list(), indent=4)
    elif options.action == EActions.SHOWMACROS:
        main_showmacros(options)
    elif options.action == EActions.UPDATEMACROS:
        main_updatemacros(options)
    elif options.action == EActions.ADDMACROS:
        main_addmacros(options)
    elif options.action == EActions.REMOVEMACROS:
        main_removemacros(options)
    elif options.action == EActions.SYNCMACROSES:
        return
        # main_syncmacroses(options)
    else:
        hbf_groups, hbf_prjs, all_macroses = load_hbf_info()
        hbf_groups_info = []
        for group in options.groups:
            hbf_groups_info.append(THbfGroupInfo(group, hbf_groups, hbf_prjs))

        if options.action == EActions.SHOW:
            main_show(hbf_groups_info, options)
        elif options.action == EActions.UPDATE:
            if options.verbose > 0:
                main_show(hbf_groups_info, options)
            return
            # main_update(hbf_groups_info, all_macroses, options)
        elif options.action == EActions.FIXDB:
            return
            # main_fixdb(hbf_groups_info, options)
        else:
            raise Exception, "Unknown action <%s>" % (options.action)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)
