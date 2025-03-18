#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import random
import string
import copy
import re

import gencfg
import gaux.aux_utils
from core.argparse.parser import ArgumentParserExt
from core.db import CURDB
from collections import defaultdict
import core.argparse.types
from core.card.updater import CardUpdater
import core.card.types as card_types
from utils.common import find_most_unused_port
from core.exceptions import UtilNormalizeException
from gaux.aux_utils import correct_pfname
from core.settings import SETTINGS
from config import MAIN_DIR
from core.igroup_triggers import TGroupTriggers

RESERVED_GROUPS = ['MSK_RESERVED', 'SAS_RESERVED', 'MAN_RESERVED', 'UNKNOWN_RESERVED']


def empty_group(db, group, remove_from_master=False, acceptor=None):
    if group.card.tags.itype not in ('int', 'intl2'):
        for intlookup_name in group.card.intlookups:
            db.intlookups.remove_intlookup(intlookup_name)

    # cleanup donees
    for sibling in group.card.master.slaves:
        if sibling.card.host_donor == group.card.name:
            for intlookup_name in sibling.card.intlookups:
                print "Remove donee's intlookup: {}".format(intlookup_name)
                db.intlookups.remove_intlookup(intlookup_name)

    if group.card.master is None and group.card.properties.background_group:
        db.groups.remove_hosts(group.getHosts(), group)
    else:
        if remove_from_master or not group.card.master:
            if acceptor:
                db.groups.move_hosts(group.getHosts(), acceptor)
            else:
                db.groups.move_hosts_to_reserved(group.getHosts())
        elif not group.card.host_donor:
            db.groups.remove_slave_hosts(group.getHosts(), group)

    db.update(smart=True)


def get_parser():
    parser = ArgumentParserExt(description="Script to add/remove/change groups", prog='PROG', usage="""
usage: %(prog)s -a listall
             -a list -g <group>
             -a addgroup -g <group> [--template-group <templatename> [--slaves-mapping <slaves_mapping>]] [--parent-group <parentname> [--donor-group <donorname>]] [-d <descr>] [-o <owners>] [-w <watchers>] [-x expires] [--instance-count-func cfun] [--instance-power-func pfun] [--instance-port-func ifun] [-t tags] [-p properties] [-s hosts]
             -a addintsgroup -g <group> [-f funcs]
             -a movehosts -g <group> -s <hosts>
             -a movetoreserved -s <hosts>
             -a removehosts -g <group> -s <hosts>
             -a freehosts -g <group>
             -a emptygroup -g <group> [-c <acceptor_group>] [--remove-from-master]
             -a touch [-g <group>]
             -a touchall
             -a checkcards
             -a remove -g <group> [-c <acceptor_group>] [-r]
             -a rename -g <old_name> -n <new_name>
             -a showreadytomove -g <srcgroup> -s <comma-separated hosts list>
             -a manipulatewithslave -g <srcgroup> [--parent-group <parentname>] [--donor-group <donorname>] [--keep-hosts] [--instance-port-func <port func>]""")

    parser.add_argument("-a", "--action", dest="action", type=str, required=True,
                        choices=["listall", "list", "addgroup", "addintsgroup",
                                 "movehosts", "removehosts", "freehosts", "addslavehosts", "removeslavehosts",
                                 "touch", "touchall", "remove", "rename", "emptygroup", "copyasslave", "moveslave",
                                 "showreadytomove", "manipulatewithslave", "compactinstances", "checkcards",
                                 "movetoreserved"],
                        help="action to execute")
    parser.add_argument("--db", type=core.argparse.types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument("-g", "--group", dest="group", type=str, default=None,
                        help="group to perform action on")
    parser.add_argument("-n", "--new_group", dest="new_group", type=str, default=None,
                        help="new group (for action rename)")
    parser.add_argument("-d", "--description", dest="description", type=str, default=None,
                        help="String with description")
    parser.add_argument("-f", "--funcs", dest="funcs", type=str, default=None,
                        help="Comma-separated group funcs: <instanceCount>,<instancePower>,<instancePort>. Use 'default' for "
                             "generic function (DEPRECATED)")
    parser.add_argument("--instance-count-func", type=str, default=None,
                        help="Number of instances per host (use 'default' if not sure)")
    parser.add_argument("--instance-power-func", type=str, default=None,
                        help="Instance power func (use 'default' if not sure)")
    parser.add_argument("--instance-port-func", type=str, default="auto",
                        help="Starting port (use 'auto' to get best suitable port)")
    parser.add_argument("-o", "--owners", dest="owners", type=core.argparse.types.comma_list, default=None,
                        help="Comma-separated list of owners for new group")
    parser.add_argument("-w", "--watchers", dest="watchers", type=str, default=None,
                        help="Comma-sepearated list of watchers for new group")
    parser.add_argument("-s", "--hosts", dest="hosts", type=core.argparse.types.hosts, default=None,
                        help="comma-separated list of hosts")
    parser.add_argument("-y", "--yr", action="store_true", dest="yr_format", default=False,
                        help="List hosts in yr-format")
    parser.add_argument("-c", "--acceptor_group", dest="acceptor", type=str, default=None,
                        help="group that will accept hosts.")
    parser.add_argument("-r", "--recursive", action="store_true", dest="recursive", default=False,
                        help="recursive group removal.")
    parser.add_argument("-D", "--donor-group", type=str, default=None,
                        help="Optional. Param for actions 'addgroup' and some others. Name of donor group. Applied only if parent-group is not None")
    parser.add_argument("-t", "--tags", dest="tags", type=core.argparse.types.jsondict, default=None,
                        help="""group tags as json dict, e. g. '{ "ctype" : "prod", "itype" : "balancer", "prj" : ["gencfg-group-service"], "metaprj" : "web"}'""")
    parser.add_argument("-x", "--expires", dest="expires", type=int, default=None,
                        help="Optional. Expiration time (in days since now)")
    parser.add_argument("-p", "--properties", dest="properties", type=str, default=None,
                        help="Optional. Comma-separated list of additional properties, e. g. <properties.nonsearch=True,reqs.instances.memory_guarantee=12Gb>")
    parser.add_argument("--slaves-mapping", dest="slaves_mapping", type=str, default=None,
                        help="Optional. Comma-separated pairs <src slave name>:<dst slave name>")
    parser.add_argument("-m", "--parent-group", dest="parent_group", type=str, default=None,
                        help="Optional. Param for action 'addgroup' 'moveslave'. Name of parent group")
    parser.add_argument("-e", "--template-group", dest="template_group", type=str, default=None,
                        help="Optional. Param for action 'addgroup'. Name of template group")
    parser.add_argument("--keep-hosts", action="store_true", default=False,
                        help="Optional. Param for action 'manipulatewithslave'. Keep hosts when moving group in or out")
    parser.add_argument('--allow-change-port', action='store_true', default=False,
                        help='Optional. Allow change of port when adding hosts to group (RX-594)')
    parser.add_argument("--remove-from-master", action="store_true", default=False,
                        help="Optional. Param for action 'emptygroup'. Remove hosts from master group also (for slave groups only)")
    parser.add_argument("--move-to-group", type=core.argparse.types.group, default=None,
                        help="Optional. Param for action 'showreadytomove'. Move hosts to specified group")

    return parser


def parse_cmd():
    return get_parser().parse_cmd()


def parse_json(request):
    parser = get_parser()
    return parser.parse_json(request)


def gen_commit_msg(request):
    if request.action == 'addgroup':
        common_description = "Created group %s" % request.group

        if request.template_group is not None:
            template_description = "template of %s" % request.template_group
        else:
            template_description = ""

        if request.parent_group is not None:
            slave_description = "slave of %s" % request.parent_group
        else:
            slave_description = ""

        if request.donor_group is not None:
            donor_description = "with donor %s" % request.donor_group
        else:
            donor_description = ""

        owners_description = "owned by %s" % request.owners

        description = request.description
        if description is None:
            description = ""
        if len(description) > 40:
            description = "description: %s" % (description[:37] + "...")

        return ", ".join(x for x in [common_description, template_description, slave_description, donor_description,
                                     owners_description, description] if x)

    if request.action == "remove":
        return "Removed group %s" % request.group

    if request.action == "rename":
        return "Renamed group %s to %s" % (request.group, request.new_group)

    if request.action == "addslavehosts":
        return "To slave group %s added hosts %s" % (request.group, ", ".join(map(lambda x: x.name, request.hosts)))

    if request.action == "removeslavehosts":
        return "From slave group %s removed hosts %s" % (request.group, ", ".join(map(lambda x: x.name, request.hosts)))

    raise Exception("gen_commit_msg not implemented for this action %s" % request.action)


def normalize(options, args=None):
    required_opts = {
        'list': ['group'],
        'listall': [],
        'addgroup': ['group'],
        'addintsgroup': ['group'],
        'movehosts': ['group', 'hosts'],
        'movetoreserved': ['hosts'],
        'removehosts': ['group', 'hosts'],
        'freehosts': ['group'],
        'addslavehosts': ['group', 'hosts'],
        'removeslavehosts': ['group', 'hosts'],
        'remove': ['group'],
        'rename': ['group', 'new_group'],
        'touch': [],
        'touchall': [],
        'emptygroup': ['group'],
        'moveslave': ['group', 'parent_group'],
        'showreadytomove': ['group', 'hosts'],
        'manipulatewithslave': ['group'],
        'compactinstances': ['group'],
        'checkcards': [],
    }

    if options.action is None:
        raise Exception("You must specify action")
    missing_opts = []
    for required_opt in required_opts[options.action]:
        if getattr(options, required_opt, None) is None:
            missing_opts.append(required_opt)
    if missing_opts:
        raise UtilNormalizeException(correct_pfname(__file__), missing_opts,
                                     "Options <%s> are obligatory" % ', '.join(missing_opts))

    if options.hosts is None:
        options.hosts = []

    if options.action == 'rename':
        options.old_name, options.new_name = options.group, options.new_group
    if options.action == 'remove':
        options.groups = options.group.split(',')
        can_not_remove_groups = set(options.groups) & set(RESERVED_GROUPS)
        if len(can_not_remove_groups) > 0:
            raise UtilNormalizeException(correct_pfname(__file__), ["group"],
                                         "Groups %s can not be removed" % ",".join(can_not_remove_groups))

    if options.funcs is not None:
        if len(options.funcs.split(',')) != 3:
            raise Exception("You must define all funcs")
        if options.instance_count_func is not None or options.instance_power_func is not None or options.instance_port_func is not None:
            raise Exception("Can not use --funcs with any of --instance-count-func --instance-power-func --instance-port-func")

        options.instance_count_func, options.instance_power_func, options.instance_port_func = options.funcs.split(',')
    if options.action in ('addgroup',) and options.instance_port_func == 'auto':
        subutil_params = {'port_range': 8}
        if options.parent_group is not None:
            subutil_params['hosts'] = options.db.groups.get_group(options.parent_group).getHosts()
        subresult = find_most_unused_port.jsmain(subutil_params)
        options.instance_port_func = 'new%s' % subresult.port

    if options.action in ["addgroup"] and options.owners in [None, []]:
        if options.template_group is not None:
            options.owners = copy.copy(options.db.groups.get_group(options.template_group).card.owners)
        elif options.parent_group is not None:
            options.owners = copy.copy(options.db.groups.get_group(options.parent_group).card.owners)
        else:
            options.owners = [gaux.aux_utils.getlogin()]

    if options.action == "showreadytomove" and len(options.hosts) == 0:
        options.hosts = list(options.db.groups.get_group(options.group).getHosts())

    if options.watchers is not None:
        options.watchers = core.argparse.types.comma_list(options.watchers)

    if options.tags:
        allowed_keys = {
            'ctype': str,
            'itype': str,
            'prj': list,
            'metaprj': str,
            'itag': list,
        }
        for k in options.tags:
            if k not in allowed_keys:
                raise Exception("Found tagname <%s> which is not one of <%s>" % (
                                k, ",".join(sorted(allowed_keys.keys()))))
            if not isinstance(options.tags[k], allowed_keys[k]):
                raise Exception("Wrong type <%s> for key <%s> (should be <%s>)" % (
                                type(options.tags[k]), k, allowed_keys[k]))

    if options.properties:
        options.properties = options.properties.split(',')
        options.properties = map(lambda x: (tuple(x.partition('=')[0].split('.')), x.partition('=')[2]),
                                 options.properties)
        options.properties = dict(options.properties)

        # FIXME: very ugly
        for propname in options.properties.keys():
            propvalue = options.properties[propname]
            if propvalue == 'True':
                propvalue = True
            elif propvalue == 'False':
                propvalue = False
            else:
                try:
                    propvalue = int(propvalue)
                except:
                    try:
                        propvalue = float(propvalue)
                    except:
                        pass
            options.properties[propname] = propvalue

    if options.action == 'addgroup':
        if options.parent_group is not None and options.db.groups.get_group(
                options.parent_group).card.master is not None:
            raise UtilNormalizeException(correct_pfname(__file__), ["parent_group"],
                                         "Non-master parent group <%s> is forbidden" % options.parent_group)
        if options.parent_group is not None and options.donor_group is not None:
            parent_with_slaves = map(lambda x: x.card.name,
                                     options.db.groups.get_group(options.parent_group).card.slaves) + [
                                     options.parent_group]
            if options.donor_group not in parent_with_slaves:
                raise UtilNormalizeException(correct_pfname(__file__), ["donor_group"],
                                             "Donor group <%s> is not a slave of <%s>" % (
                                             options.donor_group, options.parent_group))

        if options.donor_group is not None and options.parent_group is None:
            donor = options.db.groups.get_group(options.donor_group)
            if donor.card.master is None:
                options.parent_group = options.donor_group
            else:
                options.parent_group = donor.card.master.card.name

        # check some obviously wrong combination
        incompatable_params = False

        if options.instance_port_func is not None:
            port_func = options.instance_port_func
        elif options.template_group is not None:
            port_func = options.db.groups.get_group(options.template_group).card.legacy.funcs.instancePort
        else:
            subutil_params = {'port_range': 1}
            subresult = find_most_unused_port.jsmain(subutil_params)
            port_func = 'new%s' % subresult.port

        if options.parent_group is not None and options.db.groups.get_group(
                options.parent_group).card.legacy.funcs.instancePort == port_func:
            if not options.db.groups.get_group(options.template_group).card.properties.share_master_ports:
                incompatable_params = True
        if options.donor_group is not None and options.db.groups.get_group(
                options.donor_group).card.legacy.funcs.instancePort == port_func:
            incompatable_params = True

        if incompatable_params:
            raise UtilNormalizeException(correct_pfname(__file__), ["instance_port_func"],
                                         "Created group instances intersects with one of template/parent/donor group")

        if options.hosts != [] and options.donor_group is not None:
            raise UtilNormalizeException(correct_pfname(__file__), ["hosts", "donor_group"],
                                         "Can not copy slave group with hosts")

    if options.action == "manipulatewithslave":
        # check compatibilyt of params
        srcgroup = options.db.groups.get_group(options.group)
        mastergroup = options.db.groups.get_group(options.parent_group) if options.parent_group is not None else None
        donorgroup = options.db.groups.get_group(options.donor_group) if options.donor_group is not None else None

        # check mastergroup
        if (mastergroup is not None) and (mastergroup.card.master is not None):
            raise UtilNormalizeException(correct_pfname(__file__), ["parent_group"],
                "Parent group <%s> is not a master group (has master <%s>)" % (mastergroup.card.name, mastergroup.card.master.card.name))

        # check compatibily of mastergroup/donorgroup
        if (mastergroup is not None) and (donorgroup is not None):
            if donorgroup not in mastergroup.card.slaves + [mastergroup]:
                raise UtilNormalizeException(correct_pfname(__file__), ["parent_group", "donor_group"],
                    "Donor group <%s> is not a slave of <%s>" % (donorgroup.card.name, mastergroup.card.name))

    if options.action == "compactinstances":
        options.group = options.db.groups.get_group(options.group)
        if len(options.group.card.intlookups) == 0:
            raise UtilNormalizeException(correct_pfname(__file__), ["group"],
                "Group <%s> does not have intlookups" % options.group.card.name)
        if not re.match("^exactly\d+$", options.group.card.legacy.funcs.instanceCount):
            raise UtilNormalizeException(correct_pfname(__file__), ["group"],
                "Group <%s> has instanceCount func <%s> which does not satisfy regex <exactly\\d+>" % options.group.card.name)


    return options


def commit_message(options):
    if options.action == "addgroup":
        general_msg = "Created group %s" % options.group
        slave_msg = "slave of %s" % options.parent_group if options.parent_group else ""
        msgs = [general_msg, slave_msg]
        msg = "; ".join(x for x in msgs if x)
    elif options.action == "remove":
        msg = "Removed group %s" % options.group
    elif options.action == "rename":
        msg = "Renamed group %s to %s" % (options.group, options.new_group)
    else:
        raise Exception("Log message for action %s is not implemented" % options.action)

    return msg


def get_yr_include(group, instances):
    hosts_by_queue = {}
    for instance in instances:
        if not instance.host.queue in hosts_by_queue:
            hosts_by_queue[instance.host.queue] = set()
        hosts_by_queue[instance.host.queue].add(instance.host.name)

    result = []
    aggregate_line = "$macro{'%s'} = [" % group
    queue_id = 0
    for queue in sorted(hosts_by_queue.keys()):
        queue_id += 1
        yr_group = '%s%d' % (group, queue_id)
        if (queue_id - 1) % 3 == 0:
            aggregate_line += '\n    '
        aggregate_line += "@{$macro{'%s'}}, " % yr_group
        result += ["    '%s' => [ # %s" % (yr_group, queue)]
        shift = ' ' * 7
        hosts = sorted(hosts_by_queue[queue])
        line = shift
        for host in hosts:
            append_line = " '%s'," % host
            if len(line) + len(append_line) > 80:
                result += [line]
                line = shift
            line += append_line
        result += [line, '    ],']

    aggregate_line += '\n];'
    result += [aggregate_line]
    return '\n'.join(result)


def get_dispenser_prj_key(group):
    if not hasattr(group.card, 'dispenser'):
        return None
    return group.card.dispenser.project_key


def main(options):
    options = normalize(options)

    if options.action == "listall":
        for group in options.db.groups.get_groups():
            print "%s: %s" % (group.card.name, " ".join(group.getHostNames()))
    if options.action == "list":
        igroup = options.db.groups.get_group(options.group)
        if not options.yr_format:
            print "%s: %s" % (igroup.name, " ".join(igroup.getHostNames()))
        else:
            print get_yr_include(igroup.name, igroup.instances)
    if options.action == "addgroup":
        if options.template_group is None:
            new_group = options.db.groups.add_group(options.group,
                                                    description=options.description,
                                                    owners=options.owners,
                                                    watchers=options.watchers,
                                                    instance_count_func=options.instance_count_func,
                                                    instance_power_func=options.instance_power_func,
                                                    instance_port_func=options.instance_port_func,
                                                    master=options.parent_group,
                                                    donor=options.donor_group,
                                                    tags=options.tags,
                                                    expires=options.expires)
        else:
            new_group = options.db.groups.copy_group(options.template_group,
                                                     options.group,
                                                     parent_name=options.parent_group,
                                                     donor_name=options.donor_group,
                                                     slaves_mapping=options.slaves_mapping,
                                                     description=options.description,
                                                     owners=options.owners,
                                                     watchers=options.watchers,
                                                     instance_count_func=options.instance_count_func,
                                                     instance_power_func=options.instance_power_func,
                                                     instance_port_func=options.instance_port_func,
                                                     tags=options.tags,
                                                     expires=options.expires)

        try:
            if options.properties is not None:
                has_updates, is_ok, failed_properties = CardUpdater().update_group_card(new_group, options.properties,
                                                                                        mode="util")
                if not is_ok:
                    lst = ["Failed to set properties:"]
                    for prop in sorted(failed_properties.keys()):
                        reason = failed_properties[prop]
                        prop = ".".join(prop)
                        lst.append("    <%s> -> <%s>" % (prop, reason))
                    raise Exception("\n".join(lst))
            if options.hosts is not None:
                for host in options.hosts:
                    if options.parent_group is None:
                        options.db.groups.move_host(host, options.db.groups.get_group(options.group))
                    else:
                        options.db.groups.add_slave_host(host, new_group)

            # ================================= GENCFG-1536 START =============================================
            if (new_group.card.host_donor is not None) and (new_group.card.tags.itype in ('psi', 'portovm')):
                for host in new_group.getHosts():
                    TGroupTriggers.TOnAddHostsTriggers.run(new_group, host)
            # ================================= GENCFG-1536 FINISHE =============================================

            if not get_dispenser_prj_key(new_group):
                if options.parent_group:
                    parent_prj_key = get_dispenser_prj_key(options.db.groups.get_group(options.parent_group))
                    if parent_prj_key:
                        new_group.card.set_card_value(('dispenser', 'project_key'), parent_prj_key, new_group.parent.SCHEME)
                    else:
                        raise Exception('dispenser.project_key is not set in new group')
                else:
                    raise Exception('dispenser.project_key is not set in new group')

            options.db.update(smart=True)
        except Exception, e: # we have to remove group
            try:
                options.db.groups.remove_group(new_group)
            except:
                pass
            raise

    if options.action == "addintsgroup":
        description = 'Ints group for %s' % options.group
        base_group = options.db.groups.get_group(options.group)
        owners = base_group.card.owners[:]
        name = options.db.groups.get_group(options.group).get_int_group_name()
        master = base_group.name if base_group.master is None else base_group.card.master.card.name
        new_group = options.db.groups.add_group(name,
                                    description=description,
                                    owners=owners,
                                    instance_count_func=options.instance_count_func,
                                    instance_power_func=options.instance_power_func,
                                    instance_port_func=options.instance_port_func,
                                    master=master,
                                    donor=options.group)

        if not is_dispenser_prj_key_present(new_group):
            try:
                options.db.groups.remove_group(new_group)
            except:
                pass
	    raise Exception('dispenser.project_key is not set in new group')

        options.db.groups.update()

    # TODO: for all move hosts operations we have to check that intlookup is not broken

    if options.action == "movehosts":
        options.group = options.db.groups.get_group(options.group)

        # =============================== RX-594 START ========================================
        ignore_groups = []
        if (options.group.card.master is None) and (options.group.card.properties.background_group == False) and (len(options.hosts) > 0):
            host_master_group = options.db.groups.get_host_master_group(options.hosts[0])
            ignore_groups.append(host_master_group)

        subutil_params = dict(port_range=8, hosts=options.hosts, preferred_port=options.group.get_default_port(), ignore_groups=ignore_groups)

        subresult = find_most_unused_port.jsmain(subutil_params)
        if (subresult.port != options.group.get_default_port()) and (not options.allow_change_port):
            raise Exception('Have to change port in group <{}> after adding hosts, but not allowed to (option <--allow-change-port> is not set)'.format(options.group.card.name))
        options.group.card.legacy.funcs.instancePort = 'new%s' % subresult.port
        options.group.refresh_after_card_update()
        # =============================== RX-594 START ========================================

        if options.group.card.master:  # slave
            options.db.groups.move_hosts(options.hosts, options.group.card.master)
            options.db.groups.add_slave_hosts(options.hosts, options.group)
        else:  # not slave
            if options.group.card.properties.background_group: # background group (just adding host not removing from everywhere)
                options.db.groups.add_hosts(options.hosts, options.group)
            else:
                options.db.groups.move_hosts(options.hosts, options.group)

        options.db.hosts.update(smart=True)
        options.db.intlookups.update(smart=True)
        options.db.ipv4tunnels.update(smart=True)
        options.db.groups.update(smart=True)
    if options.action == 'movetoreserved':
        options.db.groups.move_hosts_to_reserved(options.hosts)
        options.db.update(smart=True)
    if options.action == "emptygroup":
        empty_group(options.db, options.db.groups.get_group(options.group), options.remove_from_master, options.acceptor)
    if options.action == "removehosts":
        # To completely remove hosts
        options.db.groups.get_group(options.group)
        options.db.groups.move_hosts(options.hosts, None)
        options.db.update(smart=True)
    if options.action == "addslavehosts":
        group = options.db.groups.get_group(options.group)
        options.db.groups.add_slave_hosts(options.hosts, group)
        options.db.update(smart=True)
    if options.action == "removeslavehosts":
        group = options.db.groups.get_group(options.group)
        options.db.groups.remove_slave_hosts(options.hosts, group)
        options.db.update(smart=True)
    if options.action == "touch":
        if options.group is None:
            options.db.groups.update()
        else:
            group = options.db.groups.get_group(options.group)
            group.mark_as_modified()
            options.db.update(smart=True)
    if options.action == "touchall":
        options.db.intlookups.get_intlookups()
        options.db.update()
    if options.action == "remove":
        for groupname in options.groups:
            group = options.db.groups.get_group(groupname)
            options.db.groups.remove_group(group.card.name, acceptor=options.acceptor, recursive=options.recursive)
        options.db.update(smart=True)
    if options.action == "rename":
        group = options.db.groups.get_group(options.old_name)

        # load intlookups
        # for intlookup_file in group.card.intlookups:
        #    intlookup = options.db.intlookups.get_intlookup(intlookup_file)
        #    if intlookup.base_type == group.card.name:
        #        intlookup.base_type = options.new_name

        # this will rename instances including intlookup instances
        options.db.groups.rename_group(group.card.name, options.new_name)

        options.db.update(smart=True)

        # make tricky and non-safe thing: replace old name by new one in all configs (vis string.replace)
        # make sure do it after all updates
        from utils.common import find_replace_configs
        foptions = {
            'old': options.old_name,
            'new': options.new_name,
            'directory_or_file': [options.db.CONFIG_DIR, os.path.join(MAIN_DIR, "recluster2")],
            'filter': lambda x: True,
        }

        find_replace_configs.main(gaux.aux_utils.dict_to_options(foptions))
    if options.action == "moveslave":
        if options.db.groups.get_group(options.group).master is None:
            raise Exception("Can not move non-slave group %s" % options.group)
        if options.db.groups.get_group(options.parent_group).master is not None:
            raise Exception("Can not copy group to non-master group %s" % options.parent_group)
        if options.db.groups.get_group(options.group).master.name == options.parent_group:
            raise Exception("Group %s already slave of %s" % (options.group, options.parent_group))

        # copy as slave with temporary name
        temp_group_name = ''.join(random.choice(string.ascii_uppercase) for _ in range(20))
        options.db.groups.copy_as_slave(options.group, temp_group_name, options.parent_group)
        options.db.groups.remove_group(options.group)
        options.db.groups.rename_group(temp_group_name, options.group)
        options.db.update(smart=True)

    if options.action == "showreadytomove":
        mygroup = options.db.groups.get_group(options.group)
        allhosts = set(mygroup.getHosts())

        busyhosts = set(mygroup.get_busy_hosts())
        for slave in mygroup.slaves:
            if (slave.card.host_donor is not None) and (slave.card.host_donor == mygroup.card.name):
                busyhosts |= set(slave.get_busy_hosts())
            else:
                busyhosts |= set(slave.getHosts())

        ready_to_move = allhosts - busyhosts
        ready_to_move &= set(options.hosts)
        print "Ready to move (%d hosts): %s" % (len(ready_to_move), ",".join(map(lambda x: x.name, ready_to_move)))

        if options.move_to_group is not None:
            options.db.groups.move_hosts(ready_to_move, options.move_to_group)
            options.db.groups.update(smart=True)
            options.db.intlookups.update(smart=True)

    if options.action == "manipulatewithslave":
        srcgroup = options.db.groups.get_group(options.group)
        mastergroup = options.db.groups.get_group(options.parent_group) if options.parent_group is not None else None
        donorgroup = options.db.groups.get_group(options.donor_group) if options.donor_group is not None else None

        options.db.groups.manipulate_with_slave(srcgroup, mastergroup, donorgroup, options.keep_hosts,
                                                srcgroup.card.legacy.funcs.instancePort)

        # change properties. FIXME: code dublication
        if options.properties is not None:
            has_updates, is_ok, failed_properties = CardUpdater().update_group_card(srcgroup, options.properties, mode="util")
            if not is_ok:
                lst = ["Failed to set properties:"]
                for prop in sorted(failed_properties.keys()):
                    reason = failed_properties[prop]
                    prop = ".".join(prop)
                    lst.append("    <%s> -> <%s>" % (prop, reason))
                raise Exception("\n".join(lst))


        options.db.update(smart=True)

    if options.action == "movetoslave":
        srcgroup = options.db.groups.get_group(options.group)
        mastergroup = options.db.groups.get_group(options.parent_group)
        donorgroup = options.db.groups.get_group(options.donor_group) if options.donor_group is not None else None

        options.db.groups.move_to_slave(srcgroup, mastergroup, donorgroup)
        options.db.update(smart=True)

    if options.action == "movefromslave":
        srcgroup = options.db.groups.get_group(options.group)

        options.db.groups.move_from_slave(srcgroup)
        options.db.update(smart=True)

    if options.action == "compactinstances":
        options.db.update(smart=True)

        host_used_instances = defaultdict(list)
        for instance in options.group.get_kinda_busy_instances():
            host_used_instances[instance.host].append(instance)

        # calculate maximal number of instances per host
        max_per_host_new = max(map(lambda x: len(x), host_used_instances.itervalues()))
        max_per_host_old = int(re.match("^exactly(\d+)$", options.group.card.legacy.funcs.instanceCount).group(1))
        if max_per_host_new >= max_per_host_old:
            return

        print "Changing instance count in group <%s> from <%d> to <%d>" % (options.group.card.name, max_per_host_old, max_per_host_new)

        # shift instances
        for host in host_used_instances:
            instances = host_used_instances[host]
            for instance in instances:
                if instance.N < max_per_host_new:
                    continue
                first_unused_N = min(set(range(max_per_host_new)) - set(map(lambda x: x.N, instances)))
                instance.N = first_unused_N
                instance.port = options.group.funcs.instancePort(first_unused_N)

        # change instance func
        options.group.card.legacy.funcs.instanceCount = "exactly%d" % max_per_host_new

        options.group.mark_as_modified()
        for intlookup_name in options.group.card.intlookups:
            options.db.intlookups.get_intlookup(intlookup_name).mark_as_modified()

        options.group.refresh_after_card_update()

        options.db.intlookups.update(smart=True)
        options.db.groups.update(smart=True)

    if options.action == "checkcards":
        options.db.groups.check_group_cards_on_update(
            False,
            skip_leaves=[('reqs', 'instances', 'memory_guarantee')],
            skip_check_dispenser=True,
        )


def jsmain(d):
    options = get_parser().parse_json(d)
    return main(options)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
