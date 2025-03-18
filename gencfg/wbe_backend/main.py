#!/skynet/python/bin/python

import os
import sys
from collections import defaultdict
import json
from cStringIO import StringIO
import traceback
import functools
import copy
import time
import re
import urllib
import urllib2

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__))))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
# DO NOT DELETE THIS!!!
# this will update sys.path which is required for import
import gencfg

from flask import Flask, request, Response
from flask_restful import Resource, reqparse

# TODO: move to gencfg.py
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../contrib')))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../web_shared')))

from core.db import CURDB
import service_ctl
import utils.common.update_igroups
import utils.common.show_stats
import utils.common.update_tiers
import utils.common.alloc_hosts
import utils.common.update_intlookup
import utils.common.show_histdb_events
import utils.common.show_replicas_count
from utils.mongo import show_usage as mongo_usage
from core.card.updater import CardUpdater
import core.card.types
import core.ctypes
import core.itypes
from flask_compress import Compress
from gaux.aux_utils import ids_alphabet, int_to_smart_str, dict_to_options
from config import *
from core.exceptions import *
import utils.common.change_port
from gaux.aux_staff import get_possible_group_owners
from core.card.node import TMdDoc
import web_shared.req_cacher
from web_shared.auto_updated_data import HBF_GROUPS
from core.settings import SETTINGS
from gaux.aux_hbf import hbf_group_macro_name
import gaux.aux_cpu
import gaux.aux_volumes
import gaux.aux_portovm

import wrapped_utils
import highcharts_render
import aux_render
import frontend_output

NO_JSON_IN_POST_DATA = 'No JSON in POST request'


def build_group_hosts_stats_2(hosts):
    header = [
        {'title': 'Name', 'type': 'host', 'filter': 'regexp', 'sortable': 'true'},
        {'title': 'Model', 'type': 'string', 'filter': 'regexp', 'sortable': 'true'},
        {'title': 'Power', 'type': 'integer', 'filter': 'range', 'sortable': 'true'},
        {'title': 'Memory', 'type': 'integer', 'filter': 'range', 'sortable': 'true'},
        {'title': 'Disk', 'type': 'integer', 'filter': 'range', 'sortable': 'true'},
        {'title': 'SSD', 'type': 'integer', 'filter': 'range', 'sortable': 'true'},
        {'title': 'Location', 'type': 'string', 'filter': 'regexp', 'sortable': 'true'},
    ]

    filters = {
        "regex": {
            "value": "",
            "help": "Text regexp"
        },
        "range": {
            "value": "",
            "help": "Javascript code. Examples:\n    \"x >= 7\" - value is equal or more than 7,\n    \"0 < x && x < 7\" - value in range (0, 7)\n    \"x === 15\" - value is equal to 15"
        },
    }

    hosts_info = []
    for host in hosts:
        location = ['%s' % x for x in [host.location, host.dc, host.queue, host.switch]]
        location_printable = '/'.join(location)
        location_sortable = location_printable
        host_info = [
            {
                'printable': host.name,
                'sortable': host.name,
            },
            {
                'printable': host.model,
                'sortable': host.model,
            },
            {
                'printable': str(int(host.power)),
                'sortable': int(host.power),
            },
            {
                'printable': '%s Gb' % host.memory,
                'sortable': host.memory,
            },
            {
                'printable': str(host.disk),
                'sortable': host.disk,
            },
            {
                'printable': str(host.ssd),
                'sortable': host.ssd,
            },
            {
                'printable': location_printable,
                'sortable': location_sortable,
            },
        ]
        for d in host_info:
            d['printable'] = d['printable'].replace('unknown', '???')
        hosts_info.append(host_info)
    result = {
        'filter': filters,
        'hosts': hosts_info,
        'header': header,
    }
    result = dict(hosts_info=result)
    return result


class FailPage(Resource):
    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def get(self):
        raise Exception('Something always go wrong on this page!')


class Favicon(Resource):
    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def get(self):
        icon = open(os.path.join(MAIN_DIR, 'gencfg.png')).read()
        return Response(response=icon, status=200, mimetype="image/png")


# TODO: test all utils.common.update_igroups operations

# TODO: rename get_primus_config -> get_tiers_info
# TODO???: rename hostsinfo
# TODO: replace get_all_hosts -> get_hosts

# TODO: add per request locks
# TODO: error handler

# TODO: pack request (flask-compress vs nginx)
# TODO: add loop

# TODO: create something like get_intlookup_info()

# TODO: try use decorator to throw away finalize_result

# TODO: why not singleton for WebEngine?

# TODO: fast access to intlookups

# TODO: do we need recluster next???


class BasePage(Resource):
    @classmethod
    def init(cls, api):
        cls.api = api
        cls.db = api.db
        return cls

    def finalize_result(self, result, update_actions=False):
        result['std header'] = {
            'ids_alphabet': ids_alphabet(),
            'current_branch': request.branch,
            'auth_user': request.yauth.login,
        }
        if update_actions:
            assert ('actions' in result)
            if self.db.is_readonly():
                for action in result['actions']:
                    assert ('is_changing_db' in action)
                    assert ('enabled' in action)
                    assert ('hint' in action)
                    if action['is_changing_db']:
                        action['enabled'] = False
                        action['hint'] = 'Database is readonly'

        return result

    def get_std_groups_info(self):
        result = {}
        for group in self.db.groups.get_groups():
            result[group.card.name] = dict(
                projects=group.card.tags.prj,
                master=(group.card.master.card.name if group.card.master is not None else None),
                donor=group.card.host_donor,
                owners=group.card.owners)
        return result

    def get_std_hosts_info(self, hosts=None):
        if hosts is None:
            hosts = self.db.hosts.get_hosts()

        result_hosts = {}
        for host in hosts:
            result_hosts[host.name] = dict(dc=host.dc, queue=host.queue)
        return result_hosts

    def build_group_hosts_stats(self, group, vm_hosts=False):
        max_hosts_info = 300
        hosts = group.getHosts()

        # ================================ RX-237 START =============================
        if vm_hosts:
            if group.has_portovm_guest_group():
                hosts = [gaux.aux_portovm.guest_instance(x, db=self.db).host for x in group.get_kinda_busy_instances()]
            else:
                hosts = []
        # ================================ RX-237 FINISH =============================

        hosts_info = []
        header = ['Name', 'CPU', 'Memory', 'Disk', 'SSD', 'Location', 'OS']
        for host in hosts[:max_hosts_info]:
            location = ['%s' % x for x in [host.queue, host.switch, host.rack]]
            location_printable = '/'.join(location)
            location_sortable = '.'.join(location)
            host_info = [
                {
                    'printable': host.name,
                    'sortable': host.name,
                },
                {
                    'printable': '%s/%s/%s' % (host.model, host.ncpu, int(host.power)),
                    'sortable': int(host.power),
                },
                {
                    'printable': '%s Gb' % host.memory,
                    'sortable': host.memory,
                },
                {
                    'printable': '%s/%s' % (host.disk, host.n_disks),
                    'sortable': host.disk,
                },
                {
                    'printable': str(host.ssd),
                    'sortable': host.ssd,
                },
                {
                    'printable': location_printable,
                    'sortable': location_sortable,
                },
                {
                    'printable': host.os,
                    'sortable': host.os,
                }
            ]
            for d in host_info:
                d['printable'] = d['printable'].replace('unknown', '???')
            hosts_info.append(host_info)

        result = {
            'total_hosts': len(hosts),
            'listed_hosts': len(hosts_info),
            'hosts': hosts_info,
            'header': header,
        }

        if vm_hosts:
            result = dict(vm_hosts_info=result)
        else:
            result = dict(hosts_info=result)

        return result

    def build_volumes_stats(self, group):
        volumes = gaux.aux_volumes.volumes_as_objects(group)

        result = []
        for volume in volumes:
            volume_info = [
                dict(printable=volume.guest_mp, sortable=volume.guest_mp),
                dict(printable=volume.host_mp_root, sortable=volume.host_mp_root),
                dict(printable=volume.quota.text, sortable=volume.quota.value),
                dict(printable=', '.join(volume.symlinks), sortable=', '.join(volume.symlinks)),
                dict(printable=volume.shared, sortable=volume.shared),
                dict(printable=volume.generate_deprecated_uuid, sortble=volume.generate_deprecated_uuid),
                dict(printable=volume.mount_point_workdir, sortable=volume.mount_point_workdir),
            ]
            result.append(volume_info)

        result = {
            'volumes': result,
            'header': ['Guest MP', 'Host MP', 'Size', 'Symlinks', 'IsShared', 'IsDeprecatedUuid', 'IsWorkdir'],
        }

        return result


    def subscript_exec(self, func):
        # this works well only in single thread mode
        # (for multithread mode we'll implement smart stdout object)
        # this doesn't work for non python code and for child processes

        stdstreams = StringIO()
        old_stdout, sys.stdout = sys.stdout, stdstreams
        old_stderr, sys.stderr = sys.stderr, stdstreams

        success = True
        result = None
        try:
            result = func()
            exception_info = None
        except Exception, e:
            success = False
            print >> stdstreams, traceback.format_exc()

            exception_info = {"class": e.__class__.__name__}
            if issubclass(e.__class__, IGencfgException):
                exception_info['params'] = e.dict_params()

        sys.stdout = old_stdout
        sys.stderr = old_stderr

        return success, stdstreams.getvalue(), exception_info, result

    def get_histdb_events(self, event_type, event_object_id):
        d = {
            'event_type': event_type,
            'event_object_id': event_object_id,
        }

        suboptions = utils.common.show_histdb_events.parse_json(d)

        events = utils.common.show_histdb_events.main(suboptions)

        table_data = []
        signal_value = map(lambda x: x.event_name, events)
        table_data.append(
            frontend_output.TTableColumn("Event type", "Histdb event type", "str", signal_value, fltmode="fltregex",
                                         width="20%"))

        signal_value = map(lambda x: str(x), self.db.histdb.get_smart_params(events))
        table_data.append(
            frontend_output.TTableColumn("Changed Properties", "Properties, changed at specified event", "str",
                                         signal_value, fltmode="fltregex", width="60%",
                                         css="printable__whitespace_pre-wrap"))

        signal_value = map(lambda x: {"value": x.event_date, "sortable": x.event_id}, events)
        table_data.append(frontend_output.TTableColumn("Date", "Date of event", "int", signal_value,
                                                       converter=lambda x: time.strftime("%Y-%m-%d %H:%M",
                                                                                         time.localtime(x)),
                                                       fltmode="fltregex", width="20%"))

        return frontend_output.generate_table(table_data)


class MainPage(BasePage):
    RELATIVE_URL = "/"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        result = {}

        parser = reqparse.RequestParser()
        args = parser.parse_args()

        result['groups'] = self.get_std_groups_info()

        # TODO: cache intlookups
        result_intlookups = {}
        for intlookup in self.db.intlookups.get_intlookup_names():
            result_intlookups[intlookup] = dict()
        result['intlookups'] = result_intlookups

        tiers = self.db.tiers.get_tiers()
        result_tiers = {}
        for tier in tiers:
            result_tiers[tier.name] = dict(shardsCount=tier.get_shards_count())
        result['tiers'] = result_tiers

        result_switches = {}
        for host in self.db.hosts.get_hosts():
            if host.switch not in result_switches:
                result_switches[host.switch] = dict(dc=host.dc)
        result['switches'] = result_switches

        result['contacts'] = ['hoho@yandex-team.ru', 'kimkim@yandex-team.ru']

        return self.finalize_result(result)


class SearchPage(BasePage):
    RELATIVE_URL = "/search"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        parser = reqparse.RequestParser()
        parser.add_argument('query', default='', help='Search query.')
        args = parser.parse_args()

        results = []
        query = args.query

        if self.db.groups.has_group(query):
            results.append(('group', query))
        if self.db.hosts.has_host(query):
            results.append(('host', query))
        switches = set(host.switch for host in self.db.hosts.get_hosts())
        if query in switches:
            results.append(('switch', query))
        if self.db.tiers.has_tier(query):
            results.append(('tier', query))
        if query in self.db.intlookups.get_intlookup_names():
            results.append(('intlookup', query))
        results = [dict(type=tp, id=key) for (tp, key) in results]

        result = {
            'search request': query,
            'results': results,
        }
        return self.finalize_result(result)


class GroupsPage(BasePage):
    RELATIVE_URL = "/groups"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    @web_shared.req_cacher.api_mongo_cache
    def _get(self):
        parser = reqparse.RequestParser()
        parser.add_argument('group', help='Target group.')
        parser.add_argument('action', help='Action to perform.')
        args = parser.parse_args()

        can_user_create_new_group = self.api.wbe_authorizer.can_user_create_new_group(request.yauth.login)

        result = {
            'groups': self.get_std_groups_info(),
            'group': args.group,
            'action': args.action,
            'actions': [
                self._create_create_group_action(can_user_create_new_group),
            ],
        }

        return self.finalize_result(result, update_actions=True)

    def get(self):
        return self._get(request)

    def _create_create_group_action(self, is_allowed):
        return dict(name='Create Group', enabled=is_allowed,
                    is_changing_db=True,
                    hint="You don't have permission to create group; Ask service administrators to provide you access" if not is_allowed else None)

    def gencfg_authorize_post(self, login, request):
        util_request = utils.common.update_igroups.parse_json(request.json)
        if util_request.action == 'addgroup':
            access_group = util_request.parent_group
        else:
            access_group = util_request.group

        if not access_group:
            assert (util_request.action in ['addgroup', 'addtempgroup'])
            return self.api.wbe_authorizer.can_user_create_new_group(login)

        status = self.api.wbe_authorizer.can_user_modify_group(login, access_group)
        if not status:
            if util_request.parent_group is not None:
                return status, "User <%s> tries to add group <%s> to parent <%s>, not being in parent group access list" % (login, util_request.group, util_request.parent_group)
            else:
                return status, "User <%s> tries to modify group <%s>, not being in group access list" % (login, access_group)
        else:
            return status

    def post(self):
        if not request.json:
            raise Exception(NO_JSON_IN_POST_DATA)

        util_request = copy.deepcopy(request.json)

        # check some constraints
        if util_request.get("hosts", "") != "" and util_request.get("parent_group", "") != "":
            raise Exception("You can not add \"Hosts\" when specifying \"Parent Group\"")
        if 'hosts' in util_request and util_request['hosts'] != '':
            hosts = {self.db.hosts.get_host_by_name(x) for x in util_request['hosts'].strip().split(',')}
            reserved_hosts = set(sum([x.getHosts() for x in self.db.groups.get_reserved_groups()], []))
            wrong_hosts = hosts - reserved_hosts
            if wrong_hosts:
                raise Exception('Trying to create group with hosts not in reserved groups: <{}>'.format(','.join(x.name for x in sorted(wrong_hosts))))
        # ========================================== RX-236 START =====================================================
        if util_request['action'] in ('addgroup', 'addtempgroup') and util_request.get('parent_group', '') == '':
            exception_info = ('You are not allowed to create master group from wbe interface (must specify <Parent Group>). Use \n'
                              '<https://gencfg.yandex-team.ru/unstable/utils/allocate_group_in_dynamic> to create group in dynamic \n'
                              'or create startrek task in <GENCFG> queue, if you really want separate hosts')
            result = {
                'operation log': exception_info,
                'operation success': False,
                'commit' : self.db.get_repo().get_last_commit_id(),
                'exception info': exception_info,
                'group': None
            }
            return self.finalize_result(result)
        # ========================================== RX-236 FINISH ====================================================


        if util_request.get("instance_port_func", None) == "":
            util_request["instance_port_func"] = None
        if util_request["action"] == "remove" and util_request.get("acceptor", "") == "":
            util_request["acceptor"] = None
        self.__patch_port_intersection(util_request)

        # create group by copying from template group
        if util_request["action"] == "addgroup":
            if util_request.get("parent_group", "") == "":
                util_request["parent_group"] = None
            if util_request.get("donor_group", "") == "":
                util_request["donor_group"] = None
            if util_request.get("description", "") == "":
                util_request["description"] = None
            if util_request.get("template_group", "") == "":
                util_request["template_group"] = None

        if util_request["action"] == "addtempgroup":  # with temporary group we should check expiration time and generate group name
            if util_request.get("expires", "") == "":
                raise Exception("Missing obligatory field <expires>")

            util_request["action"] = "addgroup"
            util_request["group_memory_type"] = "full_host_group"
            util_request["metaprj"] = "internal"
            if util_request.get("group", "") == "":  # we should find group
                prefix = ("%s_EXPERIMENT_" % request.yauth.login).upper()
                for index in xrange(1000):
                    if not self.api.db.groups.has_group("%s%s" % (prefix, index)):
                        util_request["group"] = "%s%s" % (prefix, index)
                        break

        # add memory requirements
        if "group_memory_type" not in util_request:
            pass # FIXME: raise exception after backend and frontend with group memory support is on production
        else:
            if util_request["group_memory_type"] == "fake_group":
                util_request["properties"] = "properties.fake_group=True"
            elif util_request["group_memory_type"] == "full_host_group":
                util_request["properties"] = "properties.full_host_group=True"
            elif util_request["group_memory_type"] == "common_group":
                # process memory guarantee reqs
                if util_request.get("instance_memory_guarantee", "") == "":
                    if util_request["template_group"] is None:
                        raise Exception, "You must specify <Instance Memory>"
                else:
                    util_request["properties"] = "reqs.instances.memory_guarantee=%s" % util_request["instance_memory_guarantee"]
                # process cpu guarantee reqs
                if util_request.get("instance_cpu_guarantee", "") == "":
                    if util_request["template_group"] is None:
                        raise Exception, "You must specify <Instance Cpu>"
                else:
                    instance_cpu_guarantee = int(util_request["instance_cpu_guarantee"])
                    util_request["instance_power_func"] = 'exactly{}'.format(instance_cpu_guarantee)
        # add metaprj requirements
        if util_request["action"] == "addgroup":
            if util_request.get("metaprj", "") == "":
                raise Exception('You must specify <Meta Project>')
            metaprj_prop_string = "tags.metaprj={}".format(util_request["metaprj"])
            if "properties" not in util_request:
                util_request["properties"] = metaprj_prop_string
            else:
                util_request["properties"] = '{},{}'.format(util_request["properties"], metaprj_prop_string)


        util_request = utils.common.update_igroups.parse_json(util_request)

        commit_msg = "[groups] [%s] %s" % (
        util_request.action, utils.common.update_igroups.gen_commit_msg(util_request))
        success, log, exception_info, _ = self.subscript_exec(
            functools.partial(utils.common.update_igroups.main, util_request))
        if success:
            self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        # we suggest a group which user would like to see after performed operation
        hint_group = util_request.group
        if not success:
            hint_group = util_request.group if self.db.groups.has_group(util_request.group) else None
        else:
            if util_request.action == 'remove':
                hint_group = None
            if util_request.action == 'rename':
                hint_group = util_request.new_group

        result = {
            "operation log": log,
            "operation success": success,
            "commit" : self.db.get_repo().get_last_commit_id(),
            "exception info": exception_info,
            "group": hint_group,
        }
        return self.finalize_result(result)

    def __patch_port_intersection(self, util_request):
        if util_request["action"] != "addgroup":
            return
        master = util_request["master"] if "master" in util_request else None
        if master is None:
            # no intersection possible
            return
        assert ("funcs" not in util_request)
        master = self.db.groups.get_group(master)
        port = master.find_free_port_for_slave()
        util_request["funcs"] = "default,default,new%s" % port


class GroupPage(BasePage):
    RELATIVE_URL = '/groups/<string:group>'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    @web_shared.req_cacher.api_mongo_cache
    def _get(self, group):
        group = str(group)  # unicode to str for compatibility

        group = self.db.groups.get_group(group)

        can_user_modify_group = self.api.wbe_authorizer.can_user_modify_group(request.yauth.login, group.card.name)

        hosts = group.getHosts()
        instances = group.get_instances()
        host_donor = group.get_group_donor()

        card_result = GroupCardPage().get(group.card.name)
        result = {
            'group': group.card.name,
            'master': group.card.master.card.name if group.card.master else None,
            'card': card_result['card'],
            'interesting_card': card_result['interesting_card'],
            'is_readonly_card': card_result['is_readonly_card'],
            'card as text': group.card.save_to_str(group.parent.SCHEME),
            'default_reserved_group': group.get_default_reserved_group(),
        }

        pretty_int = lambda x: int_to_smart_str(int(x))
        params_list = [
            dict(key='Total hosts power', value=pretty_int(sum(instance.power for instance in instances))),
            dict(key='Total hosts memory', value='%s Gb' % pretty_int(sum(host.memory for host in hosts))),
            dict(key='Number of hosts', value=pretty_int(len(hosts))),
            dict(key='Number of instances', value=pretty_int(len(instances))),
            dict(key='Number of big disks', value=pretty_int(len([host for host in hosts if host.disk > 900]))),
            dict(key='Average host power',
                 value=int(sum(instance.power for instance in instances) / len(hosts) if hosts else .0)),
            dict(key='Average host memory',
                 value='%s Gb' % pretty_int(int(sum(host.memory for host in hosts) / len(hosts) if hosts else 0))),
        ]

        if group.card.properties.fake_group and (len(group.card.slaves) == 0):
            params_list.extend(self.generate_putilin_usage_urls_for_fake_group(group))
        else:
            params_list.extend(self.generate_putilin_usage_urls_for_normal_group(group))

        hbf_macro_url = '{}&project_name={}'.format(SETTINGS.services.racktables.hbf.http.services.url, hbf_group_macro_name(group))
        params_list.append(dict(key='Hbf macro name', value = {'title': hbf_group_macro_name(group), 'href' : hbf_macro_url}))

        for i, _ in enumerate(params_list):
            if isinstance(_, dict):
                continue
            params_list[i]["value"] = str(params_list[i]["value"])
        params = {v["key"]: v["value"] for v in params_list}

        result['params'] = params  # deprecated
        result['params_list'] = params_list
        result['extra info'] = '... to be done ...'
        result['hosts'] = sorted([host.name for host in hosts])
        result['volumes_as_json'] = gaux.aux_volumes.volumes_as_json_string(group)
        result['volumes'] = self.build_volumes_stats(group)

        hosts_info = self.build_group_hosts_stats(group)
        result.update(hosts_info)

        # ==================================== RX-237 START ======================================
        vm_hosts_info = self.build_group_hosts_stats(group, vm_hosts=True)
        result.update(vm_hosts_info)
        # ==================================== RX-237 FINISH =====================================

        # TODO: add intersected groups
        related_groups = {
            'slaves': sorted([slave.card.name for slave in group.getSlaves()]),
            'host donors': [] if host_donor is None else [host_donor.card.name],
            'host acceptors': sorted([acceptor.card.name for acceptor in group.get_group_acceptors()])
        }
        result['related groups'] = related_groups

        result['intlookups'] = group.card.intlookups[:]

        # resources s
        result['resources_summary'] = self.generate_resources_summary(group)

        result['histdb_events'] = None

        if group.card.master:
            # slave cannot have a slave at the moment
            result['host_donors_for_new_slaves'] = []
        else:
            result['host_donors_for_new_slaves'] = [group.card.name] + sorted(
                [slave.card.name for slave in group.slaves if slave.card.host_donor is None])

        # actions will be added incrementally
        result['actions'] = [
            self._create_rename_group_action(group, can_user_modify_group),
            self._create_delete_group_action(group, can_user_modify_group),
            # Not working
            # self._create_utils.common.alloc_hosts_action(group, can_user_modify_group and can_user_allocate_hosts),
            self._modify_group_hosts_action(group, can_user_modify_group),
            self._create_free_hosts_action(group, can_user_modify_group),
            self._create_slave_group_action(group, can_user_modify_group),
            self._reset_intlookup_action(group, can_user_modify_group),
            self._build_simple_intlookup_action(group, can_user_modify_group),
            self._update_volumes_action(group, can_user_modify_group),
        ]

        return self.finalize_result(result, update_actions=True)

    def get(self, group):
        return self._get(request, group)

    def gencfg_authorize_post(self, login, request):
        return self.api.wbe_authorizer.can_user_modify_group(login, request.view_args['group'])

    def post(self, group):
        # TODO: this page should have address /groups/.../intlookup
        # because all actions are performed mainly with intlookup
        group = self.db.groups.get_group(group)

        if not request.json:
            raise Exception(NO_JSON_IN_POST_DATA)

        action = request.json['action']
        if action in ['remove', 'build', 'build_simple_intlookup', 'reset']:
            return self.build_intlookup(group)
        raise Exception('Unsupported action %s' % action)

    def generate_resources_summary(self, group):
        """Generate group resources info (GENCFG-969)

        :return: frontend_output.TTreeElement"""

        # calculate cpu info
        instances = group.get_kinda_busy_instances()
        total_instances = len(instances)

        if (not group.card.properties.fake_group) and (not group.card.properties.full_host_group) and (total_instances > 0):
            total_memory = total_instances * group.card.reqs.instances.memory_guarantee.value
            total_memory = '{:.2f} Gb'.format(total_memory / 1024. / 1024 / 1024)

            total_power = sum((x.power for x in instances))
            ipower = {x.power for x in instances}
            if max(ipower) - min(ipower) < 5:
                instance_power = min(ipower)
            else:
                instance_power = None
                instance_power_msg = 'Group has different instances power'
        else:
            total_memory = None
            total_power = None
            if group.card.properties.fake_group:
                total_memory_msg = 'Group is fake (no memory guarantee)'
                total_power_msg = 'Group is fake (no cpu guarantee)'
            elif group.card.properties.full_host_group:
                total_memory_msg = 'Group is full-host (no memory guarantee)'
                total_power_msg = 'Group is full-host (no cpu guarantee)'
            elif total_instances == 0:
                total_memory_msg = 'Group is empty'
                total_power_msg = 'Group is empty'
            else:
                raise Exception('Could not happen')

        total_instances_result = frontend_output.TTreeElement(
            frontend_output.TLeafElement("Total instances"),
            frontend_output.TLeafElement(str(total_instances))
        )

        # generate cpu info
        cpu_result = frontend_output.TTreeElement(frontend_output.TLeafElement("Allocated cpu"))
        if total_power is not None:
            total_power_result = frontend_output.TTreeElement(
                frontend_output.TLeafElement("Total guarantee"),
                frontend_output.TLeafElement(str(total_power))
            )
            cpu_result.children.append(total_power_result)

            if instance_power is not None:
                power_per_instance_result = frontend_output.TTreeElement(
                    frontend_output.TLeafElement("Guarantee per instance"),
                    frontend_output.TLeafElement('{:.2f} power units ({:.2f} cores)'.format(instance_power, gaux.aux_cpu.power_to_cores(instance_power)))
                )
            else:
                power_per_instance_result = frontend_output.TTreeElement(frontend_output.TLeafElement(instance_power_msg, css="printable__bold_text printable__red_text"))
            cpu_result.children.append(power_per_instance_result)
        else:
            total_power_result = frontend_output.TTreeElement(frontend_output.TLeafElement(total_power_msg, css="printable__bold_text printable__red_text"))
            cpu_result.children.append(total_power_result)

        # generate memory info
        memory_result = frontend_output.TTreeElement(frontend_output.TLeafElement("Allocated memory"))
        if total_memory:
            total_memory_result = frontend_output.TTreeElement(frontend_output.TLeafElement("Total guarantee"), frontend_output.TLeafElement(str(total_memory)))
            memory_result.children.append(total_memory_result)

            memory_per_instance_result = frontend_output.TTreeElement(
                frontend_output.TLeafElement("Memory per instance"),
                frontend_output.TLeafElement(group.card.reqs.instances.memory_guarantee.text)
            )
            memory_result.children.append(memory_per_instance_result)
        else:
            total_memory_result = frontend_output.TTreeElement(frontend_output.TLeafElement(total_memory_msg, css="printable__bold_text printable__red_text"))
            memory_result.children.append(total_memory_result)

        return [total_instances_result.to_json(), cpu_result.to_json(), memory_result.to_json()]

    def generate_putilin_usage_urls_for_fake_group(self, group):
        total_graph_url = SETTINGS.services.putilin_graphs.rest.url + "/fake_group_host_usage_from_trunk_graph/" + group.card.name
        total_graph_title = "[Trunk] Total host usage (mem and cpu)"

        distribution_graph_url = SETTINGS.services.putilin_graphs.rest.url + "/fake_group_host_usage_distribution_from_trunk_graph/" + group.card.name
        distribution_graph_title = "[Trunk] Host usage distribution (mem and cpu)"
        return [{"key" : total_graph_title, "value" : { "title" : "Click Here", "href" : total_graph_url }},
                {"key" : distribution_graph_title, "value" : { "title" : "Click Here", "href" : distribution_graph_url}}]

    def generate_putilin_usage_urls_for_normal_group(self, group):
        result = list()

        url = SETTINGS.services.putilin_graphs.rest.url

        end_ts = int(time.time())
        start_ts = SETTINGS.services.putilin_graphs.rest.startts

        # couple of aux functions
        def _get_last_histogram_timestamp():
            url = "%s/last_recent_timestamp" % SETTINGS.services.putilin_graphs.rest.url
            content = urllib2.urlopen(url, timeout=20).read()
            return json.loads(content)["result"]
        def _generate_putilin_usage_url(graphs, last_histogram_timestamp, history_graph=True):
            if history_graph:
                tpl = {
                    "start_ts" : start_ts,
                    "end_ts" : end_ts,
                    "graphs" : graphs,
                }
                prefix = "group_graph"
            else: # current usage graph
                tpl = {
                    "ts": last_histogram_timestamp,
                    "n_bins": 50,
                    "graphs": graphs,
                }
                prefix = "histogram"

            return "%s/%s?params=%s" % (url, prefix, urllib.quote(urllib.quote(json.dumps(tpl))))

        if len(group.card.slaves) == 0:
            result.append({
                "key" : "Instance usage distribution (mem and cpu)",
                "value" : {
                    "title" : "Click Here",
                    "href" : SETTINGS.services.putilin_graphs.rest.url + "/recent_instance_usage_distribution_vs_allocated_graph/" + group.card.name
            }})

        # generate usage graphs
        last_histogram_timestamp = _get_last_histogram_timestamp()
        for host_signal_name, instance_signal_name, graph_title in [
            ("host_cpuusage", "instance_cpuusage", "Total cpu usage"),
            ("host_memusage", "instance_memusage", "Total memory usage"),
        ]:
            usage_tpl = []

            usage_tpl = [
                {
                    "stacking" : False,
                    "signal" : host_signal_name,
                    "group" : [group.card.name] + map(lambda x: x.card.name, group.card.slaves),
                    "graph_name" : "HOST USAGE",
                },
                {
                    "stacking" : True,
                    "signal" : instance_signal_name,
                    "group" : group.card.name,
                    "graph_name" : group.card.name,
                },
            ]

            if group.card.master is None:
                for slave_group in group.card.slaves:
                    usage_tpl.append({ "stacking" : True, "signal" : instance_signal_name, "group" : slave_group.card.name })

            result.append({"key" : graph_title, "value" : { "title" : "Click Here", "href" : _generate_putilin_usage_url(usage_tpl, last_histogram_timestamp) }})

        # generate allocated graphs
        for allocated_signal_name, usage_signal_name, graph_title in [
            ("instance_cpu_allocated", "instance_cpuusage", "Total cpu allocated"),
            ("instance_mem_allocated", "instance_memusage", "Total memory allocated"),
        ]:
            usage_tpl = [
                {
                    "stacking" : False,
                    "signal" : allocated_signal_name,
                    "group" : group.card.name,
                    "graph_name" : "ALLOCATED",
                },
                {
                    "stacking" : True,
                    "signal" : usage_signal_name,
                    "group" : group.card.name,
                    "graph_name" : "USED",
                },

            ]

            result.append({"key" : graph_title, "value" : { "title" : "Click Here", "href" : _generate_putilin_usage_url(usage_tpl, last_histogram_timestamp) }})

        # generate current usage graph
        for signal_name, graph_title in [
            ("instance_cpuusage", "Instance cpu usage histogram"),
            ("host_cpuusage", "Host cpu usage histogram"),
            ("instance_memusage", "Instance memory usage histogram"),
            ("host_memusage", "Host memory usage histogram"),
        ]:

            if signal_name in ["host_cpuusage", "host_memusage"]:
                if group.card.master is None:
                    groups = map(lambda x: x.card.name, [group] + group.slaves)
                else:
                    groups = [group.card.name]
                usage_tpl = [
                    {
                        "group": groups,
                        "signal": signal_name,
                    },
                ]
            else:
                usage_tpl = [
                    {
                        "group": group.card.name,
                        "signal": signal_name,
                    },
                ]

            result.append({"key": graph_title, "value" : { "title": "Click here", "href": _generate_putilin_usage_url(usage_tpl, last_histogram_timestamp, history_graph=False) }})

        return result

    def build_intlookup(self, group):
        result = {'request': copy.deepcopy(request.json)}

        do_apply = request.json['apply']
        assert (request.json['action'] in ['remove', 'build', 'build_simple_intlookup', 'reset'])
        request.json['group'] = group.card.name

        if request.json['action'] == 'build_simple_intlookup':
            from utils.pregen import generate_trivial_intlookup

            fixed_json = copy.deepcopy(request.json)
            if fixed_json['output_file'] == '':
                fixed_json.pop('output_file')
            fixed_json['groups'] = [self.db.groups.get_group(fixed_json['group'])]
            fixed_json['bases_per_group'] = int(fixed_json['bases_per_group'])
            fixed_json['shard_count'] = re.sub(',', '+', fixed_json['shard_count'])
            fixed_json['min_replicas'] = int(fixed_json.get('min_replicas', 0))

            util_request = generate_trivial_intlookup.get_parser().parse_json(fixed_json)

            generate_trivial_intlookup.normalize(util_request)
            commit_msg = "[build_simple_intlookup] Generated intlookup for group %s" % group.card.name
            success, log, exception_info, _ = self.subscript_exec(
                functools.partial(generate_trivial_intlookup.main, util_request))
        else:
            util_request = utils.common.update_intlookup.Options.from_json(request.json)
            commit_msg = "[generate_intlookup] %s" % utils.common.update_intlookup.gen_commit_msg(util_request)
            success, log, exception_info, _ = self.subscript_exec(
                functools.partial(utils.common.update_intlookup.main, util_request))

        result['operation log'] = log
        result['operation success'] = success
        result['commit'] = self.db.get_repo().get_last_commit_id()
        result['exception info'] = exception_info
        result['group'] = group.card.name

        if do_apply and success:
            self.db.update(smart=True)
            self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        return self.finalize_result(result)

    def _create_rename_group_action(self, group, is_allowed):
        del group
        return dict(name="Rename Group", enabled=is_allowed,
                    is_changing_db=True,
                    hint="You don't have permission to rename group; Ask group owners to provide you access" if not is_allowed else None)

    def _create_delete_group_action(self, group, is_allowed):
        del group
        return dict(name="Delete Group", enabled=is_allowed,
                    is_changing_db=True,
                    hint="You don't have permission to delete group; Ask group owners to provide you access" if not is_allowed else None)

    def _create_alloc_hosts_action(self, group, is_allowed):
        del group
        return dict(name="Alloc Hosts", enabled=is_allowed,
                    is_changing_db=True,
                    hint="You don't have permission to alloc hosts; Ask group owners to provide you access" if not is_allowed else None)

    def _reset_intlookup_action(self, group, is_allowed):
        del group
        return dict(name="Reset Intlookup", enabled=is_allowed,
                    is_changing_db=True,
                    hint="You don't have permission to alloc hosts; Ask group owners to provide you access" if not is_allowed else None)

    def _update_volumes_action(self, group, is_allowed):
        del group
        return dict(name="Update Volumes", enabled=is_allowed,
                    is_changing_db=True,
                    hint="You don't have permission to alloc hosts; Ask group owners to provide you access" if not is_allowed else None)

    def _build_simple_intlookup_action(self, group, is_allowed):
        del group
        return dict(name="Build Simple Intlookup", enabled=is_allowed,
                    is_changing_db=True,
                    hint="You don't have permission to alloc hosts; Ask group owners to provide you access" if not is_allowed else None)

    def _modify_group_hosts_action(self, group, is_allowed):
        enabled = True
        hint = None
        if enabled and group.card.host_donor:
            enabled = False
            hint = "Cannot modify slaves of group with donor"
        if enabled and not is_allowed:
            enabled = False
            hint = "You don't have permission to free hosts; Ask group owners to provide you access"
        return dict(name="Modify Group Hosts", enabled=enabled, is_changing_db=True, hint=hint)

    def _create_free_hosts_action(self, group, is_allowed):
        enabled = True
        hint = None
        if enabled and group.card.host_donor is not None:
            enabled = False
            hint = "This group is hosts acceptor group. To free hosts run \"Free hosts\" on group %s" % group.card.host_donor
        if enabled and not group.getHosts():
            enabled = False
            hint = "Group is already empty"
        if enabled and not is_allowed:
            enabled = False
            hint = "You don't have permission to free hosts; Ask group owners to provide you access"
        return dict(name="Free Hosts", enabled=enabled, is_changing_db=True, hint=hint)

    def _create_slave_group_action(self, group, is_allowed):
        enabled = True
        hint = None
        if enabled and group.card.master:
            enabled = False
            hint = "This group is a slave group and slave groups cannot have own slaves"
        if enabled and not is_allowed:
            enabled = False
            hint = "You don't have permission to create slave group; Ask group owners to provide you access"
        return dict(name="Create Slave Group", enabled=enabled, is_changing_db=True, hint=hint)


# TODO: remove
class UpdateGroupIntlookupPage(BasePage):
    def get(self, group):
        parser = reqparse.RequestParser()
        parser.add_argument("action", type=str, help="Action to perform")
        args = parser.parse_args()

        group = self.db.groups.get_group(group)

        result = {
            'group': group.card.name
        }
        if group.reqs is None:
            result['intlookup'] = []
            result['actions'] = {
                'build': dict(enabled=False, is_changing_db=True, hint='Non-automatic group'),
                'remove': dict(enabled=False, is_changing_db=True, hint='Non-automatic group'),
            }
        else:
            intlookup = group.reqs.intlookup
            result['intlookup'] = [intlookup] if intlookup else []
            result['actions'] = {
                'build': dict(enabled=True, is_changing_db=True, hint=''),
                'remove': dict(enabled=bool(intlookup), is_changing_db=True,
                               hint='Intlookup does not exist' if not intlookup else '')
            }
        return self.finalize_result(result, update_actions=True)


class AuxHostsPage(BasePage):
    RELATIVE_URL = "/hosts"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    @web_shared.req_cacher.api_mongo_cache
    def _get(self):
        result = {}

        parser = reqparse.RequestParser()
        args = parser.parse_args()

        result['host_names'] = self.db.hosts.get_host_names()

        return self.finalize_result(result)

    def get(self):
        return self._get(request)


class HostPage(BasePage):
    RELATIVE_URL = '/hosts/<string:host>'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self, host):
        host = str(host)  # unicode to str for compatibility

        result = {}

        parser = reqparse.RequestParser()
        args = parser.parse_args()

        host = self.db.hosts.get_host_by_name(host)
        result['host'] = host.name

        props = [
            {
                'name': 'FQDN',
                'value': host.name,
            },
            {
                'name': 'CPU',
                'value': '%s / %s cores / %s power' % (host.model, host.ncpu, host.power),
            },
            {
                'name': 'Memory',
                'value': '%s Gb' % host.memory,
            },
            {
                'name': 'Disk',
                'value': '%s Gb total / %s Gb SSD / %s disks' % (host.disk, host.ssd, host.n_disks),
            },
            {
                'name': 'Location',
                'value': 'Location %s / DC %s / Queue %s / Switch %s / Rack %s' % (
                host.location, host.dc, host.queue, host.switch, host.rack),
            },
            {
                'name': 'VLAN',
                'value': '%s' % host.vlan,
            },
            {
                'name': 'IPMI',
                'value': '%s' % host.ipmi,
            },
            {
                'name': 'OS',
                'value': '%s %s %s' % (host.os, host.kernel, host.issue),
            },
            {
                'name': 'Last Update',
                'value': host.lastupdate,
            },
        ]
        props = [
            {
                'printable_name': prop['name'],
                'units': '',
                'group': 'all',
                'type': 'str',
                'value': prop['value'],
            }
            for prop in props
            ]

        result['properties'] = props
        result['groups'] = sorted([group.card.name for group in self.db.groups.get_host_groups(host)])

        result['links'] = [
            {
                'source': 'Golem',
                'address': 'https://golem.yandex-team.ru/hostinfo.sbml?object=%s' % host.name,
            },
            {
                'source': 'Golovan',
                'address': 'http://yasm.yandex-team.ru/search/-hosts/?term=%s' % host.name,
            },
            {
                'source': 'OOPS',
                'address': 'http://oops.yandex-team.ru/host/%s' % host.name,
            },
            {
                'source': 'Agave',
                'address': 'http://agave2.yandex-team.ru/monitoring?filter[search]=%s' % host.name,
            }
        ]
        result['links'].extend(self.generate_putilin_usage_url(host))

        instances = []
        for group in self.db.groups.get_host_groups(host):
            instances.extend(filter(lambda x: x.host == host, group.get_kinda_busy_instances()))
        result['instances'] = sorted([str(instance) for instance in instances])

        result['histdb_events'] = None

        return self.finalize_result(result)

    def generate_putilin_usage_url(self, host):
        end_ts = int(time.time())
        start_ts = end_ts - 24 * 60 * 60 * 7
        tpl = {
            "start_ts" : start_ts,
            "end_ts" : end_ts,
            "zoom_level" : "auto",
        }
        url = SETTINGS.services.putilin_graphs.rest.url

        return [
            {
                "source" : "Cpu usage",
                "address" : "%s/host_cpu_graph/%s?params=%s" % (url, host.name,  urllib.quote(urllib.quote(json.dumps(tpl))))
            },
            {
                "source" : "Memory usage",
                "address" : "%s/host_memory_graph/%s?params=%s" % (url, host.name,  urllib.quote(urllib.quote(json.dumps(tpl))))
            },
        ]

class HostsStatsPage(BasePage):
    RELATIVE_URL = "/hosts-stats"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        parser = reqparse.RequestParser()
        parser.add_argument("sources", type=str, help="Hosts sources", default="")
        args = parser.parse_args()

        args.sources = args.sources.split(',') if args.sources else []
        return self.calc_hosts_stats(args.sources)

    def gencfg_authorize_post(self, login, request):
        return self.api.wbe_authorizer.can_user_view_db(login)

    def post(self):
        if not request.json:
            raise Exception(NO_JSON_IN_POST_DATA)
        if 'sources' not in request.json:
            raise Exception('Missing "sources" list in POST request')
        return self.calc_hosts_stats(request.json['sources'])

    def calc_hosts_stats(self, sources):
        # ** start prepare hosts
        # TODO: precalc
        ids = {}
        for group in self.db.groups.get_groups():
            ids[group.card.name] = 'group'
        for host in self.db.hosts.get_hosts():
            ids[host.name] = 'host'
        for intlookup in self.db.intlookups.get_intlookup_names():
            ids[intlookup] = 'intlookup'

        undefined_sources = []
        sources_by_type = defaultdict(list)
        for source in sources:
            if source not in ids:
                undefined_sources.append(source)
            else:
                sources_by_type[ids[source]].append(source)
        if undefined_sources:
            raise Exception('Undefined sources: %s' % (' ,'.join('"%s"' % us for us in undefined_sources)))

        hosts = []
        for host in sources_by_type['host']:
            hosts.append(self.db.hosts.get_host_by_name(host))
        for group in sources_by_type['group']:
            hosts.extend(self.db.groups.get_group(group).getHosts())
        for intlookup in sources_by_type['intlookup']:
            obj = self.db.intlookups.get_intlookup(intlookup)
            instances = obj.get_instances()
            intlookup_hosts = list(set(instance.host for instance in instances))
            hosts.extend(intlookup_hosts)
        # if not hosts:
        #    hosts = self.db.hosts.get_hosts()
        # ** end prepare hosts

        # prepare description
        description = []
        if sources_by_type['host']:
            description.append('host(s) %s' % ','.join(sources_by_type['host']))
        if sources_by_type['group']:
            description.append('hosts from group(s) %s' % ','.join(sources_by_type['group']))
        if sources_by_type['intlookup']:
            description.append('hosts from intlookup(s) %s' % ','.join(sources_by_type['intlookup']))
        if description:
            description = ' + '.join(description)
        else:
            description = 'all hosts'
        description = 'Statistics for %s' % description

        scheme = copy.copy(Hosts.SCHEME)
        result = {
            'description': description,
            'scheme': scheme
        }
        result_hosts = {}
        # inverted = {}
        for host in hosts:
            host_value = [getattr(host, element['name']) for element in scheme]
            # WILL NOT ZIP
            # copy_name = inverted.get(host_value)
            # if copy_name is not None:
            #    host_value = copy_name
            # else:
            #    inverted[host_value] = host.name
            #    host_value = list(host_value)
            result_hosts[host.name] = host_value
        result['hosts'] = result_hosts
        return self.finalize_result(result)


class TierPage(BasePage):
    RELATIVE_URL = '/tiers/<string:tier>'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self, tier):
        parser = reqparse.RequestParser()
        args = parser.parse_args()

        tier = self.db.tiers.get_tier(tier)

        groups, intlookups = tier.get_groups_and_intlookups()
        owners = sorted(set(sum([group.card.owners for group in groups], [])))
        result = {
            'tier': tier.name,
            'shards': [tier.get_shard_id(shard) for shard in range(tier.get_shards_count())],
            'intlookups': sorted([x.file_name for x in intlookups]),
            'groups': sorted([x.card.name for x in groups]),
            'owners': owners,
            'is_linked': bool(owners),
        }

        can_user_modify_tiers = self.api.wbe_authorizer.can_user_modify_tiers(request.yauth.login)
        result['actions'] = [
            self._create_remove_tier_action(can_user_modify_tiers),
            self._create_rename_tier_action(can_user_modify_tiers),
        ]

        return self.finalize_result(result, update_actions=True)

    def _create_remove_tier_action(self, is_allowed):
        return dict(name='Remove Tier', enabled=is_allowed,
                    is_changing_db=True,
                    hint="You don't have permission to remove tier" if not is_allowed else None)

    def _create_rename_tier_action(self, is_allowed):
        return dict(name='Rename Tier', enabled=is_allowed,
                    is_changing_db=True,
                    hint="You don't have permission to rename tier" if not is_allowed else None)


# TODO: fix "rename tier" - update primuses names
class TiersPage(BasePage):
    RELATIVE_URL = "/tiers"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    @web_shared.req_cacher.api_mongo_cache
    def _get(self):
        parser = reqparse.RequestParser()
        parser.add_argument("tier", type=str, help="Tier to modify")
        parser.add_argument("action", type=str, help="Action to perform")
        args = parser.parse_args()

        can_user_modify_tiers = self.api.wbe_authorizer.can_user_modify_tiers(request.yauth.login)

        result = {
            'tier': args.tier,
            'action': args.action,
            'actions': [
                self._create_create_tier_action(can_user_modify_tiers)
            ],
        }

        result_tiers = []
        tiers = self.db.tiers.get_tiers_groups_and_intlookups()
        for tier, (groups, intlookups) in sorted(tiers.items(), cmp=lambda x, y: cmp(x[0].name, y[0].name)):
            owners = sorted(set(sum([group.card.owners for group in groups], [])))
            result_tier = {
                "tier": tier.name,
                "shards_count": tier.get_shards_count(),
                "owners": owners,
                "is_linked": bool(owners)
            }
            result_tiers.append(result_tier)
        result['tiers'] = result_tiers
        return self.finalize_result(result, update_actions=True)

    def get(self):
        return self._get(request)

    def _create_create_tier_action(self, is_allowed):
        return dict(name='Create Tier', enabled=is_allowed,
                    is_changing_db=True,
                    hint="You don't have permission to create tier; Ask service administrators to provide access" if not is_allowed else None)

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_modify_tiers(login)

    def post(self):
        if not request.json:
            raise Exception(NO_JSON_IN_POST_DATA)

        parser = reqparse.RequestParser()
        args = parser.parse_args()

        util_request = utils.common.update_tiers.parse_json(request.json)
        commit_msg = "[tiers] %s" % utils.common.update_tiers.gen_commit_msg(util_request)
        success, log, exception_info, _ = self.subscript_exec(
            functools.partial(utils.common.update_tiers.main, util_request))
        if success:
            # TODO: speed will be optimized later
            self.db.update(smart = True)
            self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        # we suggest a group which user would like to see after performed operation
        hint_tier = util_request.tier
        if not success:
            hint_tier = util_request.tier if self.db.tiers.has_tier(util_request.tier) else None
        else:
            if util_request.action == 'remove':
                hint_tier = None
            if util_request.action == 'rename':
                hint_tier = util_request.new_tier

        result = {
            "operation log": log,
            "operation success": success,
            "commit" : self.db.get_repo().get_last_commit_id(),
            "exception info": exception_info,
            "tier": hint_tier,
        }
        return self.finalize_result(result)


class ReservedHostsPage(BasePage):
    RELATIVE_URL = '/reserved-hosts'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        hosts = []
        for group in self.db.groups.get_reserved_groups():
            hosts.extend(group.getHosts())

        result = build_group_hosts_stats_2(hosts)

        return self.finalize_result(result)


class GroupHostsPage(BasePage):
    RELATIVE_URL = '/groups/<string:group>/hosts'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self, group):
        parser = reqparse.RequestParser()
        parser.add_argument('excluded_groups', default=None, required=False)
        args = parser.parse_args()

        group = self.db.groups.get_group(group)
        if args.excluded_groups:
            excluded_groups = [self.db.groups.get_group(exclude_group) for exclude_group in
                               args.excluded_groups.split(',')]
        else:
            excluded_groups = []
        excluded_groups = set(excluded_groups)
        exclude_hosts = set(sum([exclude_group.getHosts() for exclude_group in excluded_groups], []))
        hosts = set(group.getHosts()) - exclude_hosts
        hosts_info = build_group_hosts_stats_2(hosts)
        result = {}
        result.update(hosts_info)
        result['group'] = group.card.name
        result['exluded_groups'] = sorted([x.card.name for x in excluded_groups])
        # TODO: STUB

        return self.finalize_result(result)

    def gencfg_authorize_post(self, login, request):
        result = self.api.wbe_authorizer.can_user_modify_group(login, request.view_args['group'])
        if request.json['action'] == 'alloc':
            result = result and self.api.wbe_authorizer.can_user_allocate_hosts(login)
        return result

    def post(self, group):
        parser = reqparse.RequestParser()
        args = parser.parse_args()

        if not request.json:
            raise Exception(NO_JSON_IN_POST_DATA)

        # TODO: no check for intlookup existance

        action = request.json['action']
        group = self.db.groups.get_group(group)
        if action in ['add', 'remove']:
            if group.card.master is None:
                return self.update_master_hosts(group, action)
            else:
                return self.update_slave_hosts(group, action)
        if action == 'alloc':
            return self.utils.common.alloc_hosts(group, request.json)
        if action == 'free':
            return self.free_hosts(group, request.json)

        raise Exception('Undefined action %s' % action)

    def update_slave_hosts(self, group, action):
        assert (action in ['add', 'remove'])
        assert (group.card.master is not None)
        opts = {
            'action': action + 'slavehosts',
            'group': group.card.name,
            'hosts': request.json['hosts']
        }
        assert (request.json.get('source') in [None, group.card.master.card.name])
        assert (request.json.get('target') in [None, group.card.master.card.name])
        opts = utils.common.update_igroups.parse_json(opts)
        commit_msg = utils.common.update_igroups.gen_commit_msg(opts)
        success, log, exception_info, _ = self.subscript_exec(functools.partial(utils.common.update_igroups.main, opts))
        if success:
            self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        result = {
            'request': copy.deepcopy(request.json),
            'operation log': log,
            'operation success': success,
            'commit': self.db.get_repo().get_last_commit_id(),
            'exception info': exception_info,
            'group': group.card.name
        }
        return self.finalize_result(result)

    def update_master_hosts(self, group, action):
        opts = {
            'action': 'movehosts'
        }
        if action == 'add':
            opts['group'] = group.card.name
            target_group = group
            source_group = self.db.groups.get_group(request.json['source']) if 'source' in request.json and \
                                                                               request.json['source'] else None
            assert ('target' not in request.json or request.json['target'] is None)
            commit_msg = "Added hosts to group %s from group %s: %s" % \
                         (target_group.card.name, source_group.card.name if source_group else 'None',
                          ','.join(request.json['hosts']))
        else:
            opts['group'] = request.json['target']
            target_group = self.db.groups.get_group(request.json['target'])
            source_group = group
            assert ('source' not in request.json or request.json['source'] is None)
            commit_msg = "Removed hosts from group %s to group %s: %s" % \
                         (source_group.card.name if source_group else 'None', target_group.card.name,
                          ','.join(request.json['hosts']))

        opts['hosts'] = request.json['hosts']

        # extra check
        assert (source_group is None or source_group.card.master is None)
        assert (target_group is not None and target_group.card.master is None)
        if source_group is not None:
            bad_hosts = [host for host in opts["hosts"] if
                         not source_group.hasHost(self.db.hosts.get_host_by_name(host))]
            if bad_hosts:
                raise Exception('Following hosts does not belong to group %s: %s' % (source_group.card.name, ','.join(bad_hosts)))

        # ignore this error
        # bad_hosts = [host.name for host in opts.hosts if target_group.hasHost(self.db.hosts.get_host_by_name(host))]
        # if bad_hosts:
        #    raise Exception('Following hosts are already belong to group %s: %s' % \)
        #        (target_group.name, ','.join())

        opts = utils.common.update_igroups.parse_json(opts)
        success, log, exception_info, _ = self.subscript_exec(functools.partial(utils.common.update_igroups.main, opts))
        if success:
            self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        result = {
            'request': copy.deepcopy(request.json),
            'operation log': log,
            'operation success': success,
            'commit': self.db.get_repo().get_last_commit_id(),
            'exception info': exception_info,
            'group': group.card.name,
        }
        return self.finalize_result(result)

    def alloc_hosts(self, group, request):
        opts = utils.common.alloc_hosts.Options.from_json(request)
        success, log, exception_info, hosts = self.subscript_exec(
            functools.partial(utils.common.alloc_hosts.main, opts))
        result = {}
        result['request'] = copy.deepcopy(request)
        request['group'] = group.card.name
        result['operation success'] = success
        result['operation log'] = log
        result['exception info'] = exception_info
        result['group'] = group.card.name
        result['hosts'] = [host.name for host in hosts] if hosts else None
        return self.finalize_result(result)

    def free_hosts(self, group, request):
        success, log, exception_info, mapping = self.subscript_exec(group.free_hosts)
        result = {
            'request': copy.deepcopy(request),
            'operation success': success,
            'operation log': log,
            'exception info': exception_info
        }
        if success:
            self.db.update(smart=True)
        return self.finalize_result(result)


class GroupHostsDiffPage(BasePage):
    RELATIVE_URL = '/utils/groups_host_diff'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        parser = reqparse.RequestParser()
        parser.add_argument('first', default=None, required=True, help='First group.')
        parser.add_argument('second', default=None, required=True, help='Second group.')
        args = parser.parse_args()

        if args.first is None:
            raise Exception("first group is not set")
        if args.second is None:
            raise Exception("second group is not set")

        first_hosts = set(self.db.groups.get_group(args.first).getHosts())
        second_hosts = set(self.db.groups.get_group(args.second).getHosts())
        diff = list(first_hosts - second_hosts)

        result = {
            "first": args.first,
            "second": args.second,
            "hosts_diff": sorted([x.name for x in diff])
        }
        return self.finalize_result(result)


# TODO: remove
class RuCheckPage(BasePage):
    PATH = 'rucheck.txt'

    def get(self):
        return dict(text=open(RuCheckPage.PATH).read())

    def post(self):
        text = request.json['text']
        assert (isinstance(text, str))
        with open(RuCheckPage.PATH, 'w') as f:
            f.write(text)
            f.close()
        return dict(text="OK")


class GroupCardPage(BasePage):
    RELATIVE_URL = '/groups/<string:group>/card'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self, group):
        parser = reqparse.RequestParser()
        args = parser.parse_args()

        group = self.db.groups.get_group(group)

        can_user_modify_group = self.api.wbe_authorizer.can_user_modify_group(request.yauth.login, group.card.name)
        result = {
            'group': group.card.name
        }

        full_card_info = CardUpdater().get_group_card_info(group)
        interesting_card = copy.deepcopy(full_card_info)
        CardUpdater().filter_interesting_nodes(interesting_card)
        # 'interesting_card' is actually "card preview" with readonly access
        result['interesting_card'] = interesting_card
        # and 'card' is card you can modify
        is_readonly_card = not can_user_modify_group or self.db.is_readonly()
        if is_readonly_card:
            CardUpdater().set_group_card_info_readonly(full_card_info)
        result['card'] = full_card_info
        result['is_readonly_card'] = is_readonly_card

        return self.finalize_result(result)

    def gencfg_authorize_post(self, login, request):
        return self.api.wbe_authorizer.can_user_modify_group(login, request.view_args['group'])

    def post(self, group):
        if not request.json:
            raise Exception(NO_JSON_IN_POST_DATA)
        if self.db.is_readonly():
            raise Exception("DB is readonly")

        group = self.db.groups.get_group(group)

        # FIXME: group card update can induce changes in intlookups/other groups
        backup_master = group.card.master
        backup_slaves = group.card.slaves

        group.card.master = None
        group.card.slaves = []
        backup_card = copy.deepcopy(group.card)
        group.card.master = backup_master
        group.card.slaves = backup_slaves

        backup_card.master = backup_master

        action = request.json['action']
        # TODO: 'update and realloc action'
        if action not in ['update']:
            raise Exception('Unsupported action')

        result = {
            'request': copy.deepcopy(request.json),
            'group': group.card.name,
        }

        updates = request.json['card']
        updates = {tuple(item['path']): item['value'] for item in updates}

        has_updates, isOk, exception_info, invalid_values = self.perform_nontrivial_updates(group, updates)

        if isOk:
            has_updates_simple, isOk, invalid_values_simple = CardUpdater().update_group_card(group, updates)
            has_updates |= has_updates_simple
            invalid_values.update(invalid_values_simple)

        if has_updates:
            if isOk:
                group.mark_as_modified()

                try:
                    self.db.update(smart=True)
                except Exception, e:
                    # we got exception while updating card, now we need to restore card
                    group.card = backup_card # FIXME: not correct in some cases
                    group.refresh_after_card_update()

                    result['operation success'] = False
                    result['operation log'] = str(e)
                    result['exception info'] = str(e)
                    return self.finalize_result(result)

                commit_msg = "[groups] [card] Updated group card of %s" % group.card.name
                self.api.commit(commit_msg, request, reset_changes_on_fail=True)
            else: # something failed, we should reinit group card
                group.card = backup_card # FIXME: not correct in some cases
                group.refresh_after_card_update()

        result['operation success'] = isOk
        result['commit'] = self.db.get_repo().get_last_commit_id()
        result['operation log'] = ""
        result['invalid values'] = [{'key': list(key), 'error': error}
                                    for key, error in invalid_values.items()]
        result['exception info'] = exception_info
        result['group'] = group.card.name
        return self.finalize_result(result)

    def perform_nontrivial_updates(self, group, updates):
        """
            Updating of some card fields requires more operations. For example, when modifiing instancePort one should
            update intlookups/custom_instance_power .
        """

        NONTRIVIAL_UPDATE_MAPPING = [
            (('legacy', 'funcs', 'instancePort'), self.update_instance_port),
            (('legacy', 'funcs', 'instanceCount'), self.update_instance_count),
            (('tags', 'itype'), self.set_itype),
            (('properties', 'full_host_group'), self.update_full_host_group_property),
            (('properties', 'fake_group'), self.update_fake_group_property),
            (('reqs', 'instances', 'memory_guarantee'), self.update_memory_guarantee),
        ]

        isOk, exception_info, invalid_values = True, None, {}
        for path, func in NONTRIVIAL_UPDATE_MAPPING:
            if path in updates:
                value = updates.pop(path)
                try:
                    func(group, value)
                except Exception, e:
                    isOk = False
                    invalid_values = {path: str(e)}
                    exception_info = {"class": e.__class__.__name__}
                    if issubclass(e.__class__, IGencfgException):
                        exception_info['params'] = e.dict_params()
                    break

        return True, isOk, exception_info, invalid_values

    """
        Non-trivial updaters. Some group card properties can not be changed without extra actions.
        E. g. when we change instance port, we have to change all group instances
    """

    def update_instance_port(self, group, value):
        """
            Update legacy.funcs.instancePort
        """
        utils.common.change_port.jsmain({'group': group, 'port_func': value})

    def update_instance_count(self, group, value):
        """
            Update legacy.funcs.instanceCount
        """
        intlookups = map(lambda x: self.db.intlookups.get_intlookup(x), group.card.intlookups)

        group.card.legacy.funcs.instanceCount = value
        group.ghi.change_group_instance_func(group.card.name, group.gen_instance_func())
        group.mark_as_modified()

        for intlookup in intlookups:
            def replacement(instance):
                if instance.type != group.card.name:
                    return instance
                return group.get_instance(instance.host.name, instance.port)

            intlookup.replace_instances(replacement, run_base=True, run_int=True)

        group.mark_as_modified()

    def update_fake_group_property(self, group, value):
        """
            Update properties.fake_group
        """
        if value is True:
            group.card.properties.fake_group = value
            group.card.properties.full_host_group = False
            group.card.reqs.instances.memory_guarantee.reinit("0 Gb")

    def update_full_host_group_property(self, group, value):
        """
            Update properties.full_host_group
        """
        if value is True:
            group.card.properties.full_host_group = value
            group.card.properties.fake_group = False
            group.card.reqs.instances.memory_guarantee.reinit("0 Gb")

    def update_memory_guarantee(self, group, value):
        """
            Update reqs.instances.memory_guarantee
        """
        group.card.properties.full_host_group = False
        group.card.properties.fake_group = False
        group.card.reqs.instances.memory_guarantee.reinit(value)

    def set_itype(self, group, value):
        """
            Update tags.itype
        """
        status = core.card.types.ItypeType().validate_for_update(group, value)
        if status.status == core.card.types.EStatuses.STATUS_FAIL:
            raise Exception(status.reason)

        group.set_itype(value)


class GlobalEvalVarsStorage(object):
    def __init__(self):
        pass

    def reset(self):
        for key in self.__dict__.keys()[:]:
            del self.__dict__[key]


# You can use g as global vars storage in EvalPage
# i.e.
# curl -X POST -H "Content-Type: application/json" -d '{"cmd": "g.x = 1"}' HOST:PORT/eval
#    => no output
# curl -X POST -H "Content-Type: application/json" -d '{"cmd": "print g.x"}' HOST:PORT/eval
#    => will return "1"
# curl -X POST -H "Content-Type: application/json" -d '{"cmd": "g.reset()"}' HOST:PORT/eval
#    => no output
# curl -X POST -H "Content-Type: application/json" -d '{"cmd": "print g.x"}' HOST:PORT/eval
#    => will throw exception
g = GlobalEvalVarsStorage()


class MongodbGroupCurrentUsagePage(BasePage):
    RELATIVE_URL = '/mongodb/groups/currentusage/<string:gline>'

    DESCR = {
        'instance_cpu_usage': ("cpu usage instance stats", "Cpu usage (in percents)"),
        'instance_mem_usage': ("memory usage instance stats", "Memory usage (in gigabytes)"),
        'host_cpu_usage': ("cpu usage host stats", "Cpu usage (in percents)"),
        'host_mem_usage': ("memory usage host stats", "Memory usage (in gigabytes)"),
    }

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    @classmethod
    def dummy(self, param):
        return map(lambda x: (x, param * x * x, "Elem %s" % x), range(100))

    @classmethod
    def convert_to_aggregated(self, mongoresult, signal, N=50, make_percents=False):
        # get rid of trash
        mongoresult = dict(filter(lambda (k, v): v is not None, mongoresult.items()))

        # convert usage to percents
        def instance_usage_converter(instance, usage):
            if usage is None:
                return None
            if usage < 0.01 and instance.power < 1.:  # do not calculate for instances with zero power and zero usage
                return None
            return usage / ((instance.power + 0.0001) / instance.host.power) * 100

        def host_usage_converter(instance, usage):
            if usage is None:
                return None
            return 100. * usage

        for k in mongoresult:
            if signal == 'instance_cpu_usage':
                mongoresult[k] = instance_usage_converter(k, mongoresult[k][signal])
            elif signal == 'host_cpu_usage':
                mongoresult[k] = host_usage_converter(k, mongoresult[k][signal])
            else:
                mongoresult[k] = mongoresult[k][signal]

        # get rid of trash again (some vaules become none after converting)
        mongoresult = dict(filter(lambda (k, v): v is not None, mongoresult.items()))

        if signal in ['instance_cpu_usage', 'host_cpu_usage']:
            min_value = 0.0
            max_value = 100.0
        elif signal in ['instance_mem_usage', 'host_mem_usage']:
            min_value = min(mongoresult.values()) if len(mongoresult.values()) else 0.0
            max_value = max(mongoresult.values()) if len(mongoresult.values()) else 1.0

        step = (max_value - min_value) / (N - 1)

        counts = [0] * N
        instancesbyusage = map(lambda x: [], range(N))

        for instance in mongoresult:
            v = max(min(mongoresult[instance], max_value), min_value)
            if step == 0:
                index = 0
            else:
                index = int((v - min_value) / step)
            counts[index] += 1
            instancesbyusage[index].append(instance.name())
        for i in range(N):
            if len(instancesbyusage[i]) > 3:
                instancesbyusage[i] = instancesbyusage[i][:3] + ['...']

        instancesbyusage = map(lambda x: ', '.join(x) if len(x) else '', instancesbyusage)
        result = zip(map(lambda x: min_value + x * step, range(N)), counts, instancesbyusage)
        result[1:-1] = filter(lambda (x, y, z): y != 0, result[1:-1])

        if make_percents:
            total = sum(map(lambda (x, y, z): y, result))
            result = map(lambda (x, y, z): (x, float(y) / total * 100, z), result)

        result = map(lambda elem: {'x': elem[0], 'y': elem[1], 'suffix': "instances", 'extra': elem[2]}, result)

        return result

    """
        Gline format:
        [
            {
                'groups' : <list of group names>,
                'instances' : <comma-separated list of instances>, # can not be used with 'groups'
                'blonov' : <blinov calculator expression>, # can not be used with 'instances' or 'groups'
                'signal' : <signal name>,
                'make_percents' : <False or True>,
            },
            ...
        ]
    """

    def get(self, gline):
        graphs = []
        for gdescr in json.loads(gline):
            if 'instances' in gdescr:
                raise Exception("Instances list not supported yet")
            if 'blinov' in gdescr:
                raise Exception("Blinov expression not supported yet")

            groupnames = gdescr['groups']
            signal = gdescr['signal']
            make_percents = gdescr.get('make_percents', False)

            if make_percents:
                extra = "in percents"
            else:
                extra = "in number of instances"

            charttype = highcharts_render.ChartType('column', zoomable=True)
            title = highcharts_render.Title(
                "Groups %s %s(%s)" % (', '.join(groupnames), self.DESCR[gdescr['signal']][0], extra))
            axes = highcharts_render.Axes("linear", self.DESCR[gdescr['signal']][1], "Number of instances", minvalue=0)
            tooltip = highcharts_render.MongodbCurrentTooltip()
            plotoptions = highcharts_render.MongodbCurrentPlotOptions()

            allseries = []
            for groupname in groupnames:
                # get current cpu usage for all instances
                instances = self.db.groups.get_group(groupname).get_instances()
                params = {
                    'stats_type': 'current',
                    'instances': instances,
                    'groups': None,
                    'intlookups': None,
                    'timestamp': int(time.time()),
                }

                mongoresult = mongo_usage.main(type("DummyParams", (), params)())
                converted = self.convert_to_aggregated(mongoresult, signal, make_percents=make_percents)
                allseries.append(highcharts_render.Series(groupname, converted))

            graphs.append(highcharts_render.rendergraph(charttype, title, axes, tooltip, plotoptions, allseries, []))

        repl = highcharts_render.renderall(graphs)
        return Response(response=repl, status=200, headers=None, mimetype=None, content_type="text/html",
                        direct_passthrough=False)


class MongodbHostHistUsagePage(BasePage):
    RELATIVE_URL = '/mongodb/hosts/<string:gline>'

    DESCR = {
        'instance_cpu_usage': ("cpu usage", "Used cpu (in percents)", "percents"),
        'instance_mem_usage': ("memory usage", "Memory in gigabytes", "gigabytes"),
    }

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self, gline):
        graphs = []

        for gdescr in json.loads(gline):
            signal = gdescr['signal']
            assert (signal in self.DESCR.keys())

            hosts = map(lambda x: self.db.hosts.get_host_by_name(x), gdescr['hosts'])

            params = {
                'stats_type': "hist",
                'groups': None,
                'intlookups': None,
                'instances': None,
                'hosts': hosts,
            }
            mongoresult = mongo_usage.main(type("DummyParams", (), params)())

            mongoresult = mongoresult.items()

            # FIXME: join with previous one ???
            charttype = highcharts_render.ChartType('area', zoomable=True)
            title = highcharts_render.Title("Hosts %s %s" % (','.join(gdescr['hosts']), self.DESCR[signal][0]))
            axes = highcharts_render.Axes("datetime", "Date", self.DESCR[signal][1], minvalue=0)
            tooltip = highcharts_render.MongodbHistTooltip()
            plotoptions = highcharts_render.MongodbHistPlotOptions()

            allseries = []
            for name, data in mongoresult:
                for elem in data:
                    elem[1]['instance_cpu_usage'] *= 100

                data = map(lambda elem: {
                    'x': elem[0],
                    'y': elem[1][signal],
                    'suffix': self.DESCR[gdescr['signal']][2]
                }, data)

                if name == 'ALL_SEARCH':
                    extraparams = {'type': 'line', 'color': '#00FF00'}
                    allseries.append(highcharts_render.Series(name, data, xisdate=True, params=extraparams))
                else:
                    allseries.append(highcharts_render.Series(name, data, xisdate=True))

            modifiers = []
            modifiers.append(highcharts_render.TSmoothOnSelect(
                "<b>Smooth range</b>",
                [('No', 1, True), ('Day', 24, False), ('Week', 168, False), ('Month', 720, False)],
            ))

            graphs.append(
                highcharts_render.rendergraph(charttype, title, axes, tooltip, plotoptions, allseries, modifiers))

        repl = highcharts_render.renderall(graphs)
        return Response(response=repl, status=200, headers=None, mimetype=None, content_type="text/html",
                        direct_passthrough=False)


class MongodbStatsPage(BasePage):
    RELATIVE_URL = '/mongodb/stats'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        # get list of groups
        groups = filter(lambda x: x.master is None and x.on_update_trigger is None, CURDB.groups.get_groups())

        group_regex = request.args.get('group_regex', None)
        if group_regex is not None:
            pattern = re.compile(group_regex)
            groups = filter(lambda x: pattern.match(x.card.name), groups)

        subparams = {
            'stats_type': 'hist',
            'groups': groups,
            'timestamp': 1415648398,  # normal statistics start
            'sum_stats': False,
            'filter': None,
        }
        from utils.mongo import show_admin_stats
        subresult = show_admin_stats.main(dict_to_options(subparams))

        sum_stats = defaultdict(lambda: defaultdict(int))

        for groupname in subresult:
            for timestamp in subresult[groupname]:
                d = subresult[groupname][timestamp]

                if d['host_count'] == 0:
                    continue

                sum_stats[timestamp]['group_count'] += 1

                sum_stats[timestamp]['instance_count'] += d['instance_count']
                sum_stats[timestamp]['instance_count_mongo'] += d['instance_count_mongo']
                sum_stats[timestamp]['instance_availability'] += min(1., d['instance_count_mongo'] / float(
                    max(d['instance_count'], 1)))

                sum_stats[timestamp]['host_count'] += d['host_count']
                sum_stats[timestamp]['host_count_mongo'] += d['host_count_mongo']
                sum_stats[timestamp]['host_availability'] += min(1.,
                                                                 d['host_count_mongo'] / float(max(d['host_count'], 1)))

        sum_stats = sorted(sum_stats.items(), cmp=lambda (x1, y1), (x2, y2): cmp(x1, x2))

        charttype = highcharts_render.ChartType('line', zoomable=True)
        title = highcharts_render.Title("Mongo availability")
        axes = highcharts_render.Axes("datetime", "Date", "Availability (in instances/percents)", minvalue=0)
        tooltip = highcharts_render.Tooltip()
        plotoptions = highcharts_render.MongodbHistPlotOptions(stacking=False)

        allseries = [highcharts_render.Series("Mongo host count",
                                              map(lambda (ts, data): {
                                                  'x': ts,
                                                  'y': data['host_count_mongo'],
                                                  'availability': 100 * data['host_availability'] / max(
                                                      float(data['group_count']), 1.)
                                              }, sum_stats), xisdate=True),
                     highcharts_render.Series("Gencfg host count",
                                              map(lambda (ts, data): {
                                                  'x': ts,
                                                  'y': data['host_count'],
                                                  'availability': 100.},
                                                  sum_stats), xisdate=True),
                     highcharts_render.Series("Mongo instance count",
                                              map(lambda (ts, data): {
                                                  'x': ts,
                                                  'y': data['instance_count_mongo'],
                                                  'availability': 100 * data['instance_availability'] / max(
                                                      float(data['group_count']), 1.)
                                              }, sum_stats), xisdate=True),
                     highcharts_render.Series("Gencfg instance count",
                                              map(lambda (ts, data): {
                                                  'x': ts,
                                                  'y': data['instance_count'],
                                                  'availability': 100.
                                              }, sum_stats), xisdate=True)]

        # add modifier to show percents
        jscode = """
            for (j = 0; j < serie.data.length; j++) {
                serie.data[j].y = Math.round(serie.data[j].availability * 100) / 100;
            }
"""
        modifier = highcharts_render.TModifySeriesOnCheck('<b>Percents</b>', jscode)

        graph = highcharts_render.rendergraph(charttype, title, axes, tooltip, plotoptions, allseries, [modifier])
        repl = highcharts_render.renderall([graph])

        return Response(response=repl, status=200, headers=None, mimetype=None, content_type="text/html",
                        direct_passthrough=False)


class MongodbMetagroupsStatsPage(BasePage):
    RELATIVE_URL = '/mongodb/metagroups/histusage/<string:gline>'

    DESCRS = {
        "cpu": {
            "title": "Cpu usage",
            "yaxes_descr": "Cpu (in gencfg power units)",
            "main_signal": "host_power_rough",
            "signals": [
                ("host_power_rough", "Assigned group power"),
                ("host_cpu_usage_rough", "Real host cpu usage"),
            ],
        },
        "memory": {
            "title": "Memory usage",
            "yaxes_descr": "Memory (in gigabytes)",
            "main_signal": "host_memory_rough",
            "signals": [
                ("host_memory_rough", "Assigned group memory"),
                #                ("instance_mem_usage", "Real memory usage"),
            ],
        },
    }

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    """
        Gline format:
        {
            'startt' : <start time, in seconds since epoch>,
            'endt' : <end time, in seconds since epoch>,
            'metagroup_descr' : <name or desctiption of metagroup>,
            'graphs' : [
                {
                    'groups' : <list of groups>,
                    'signal' : <signal name, one of cpu,memory>,
                },
                ...
            ],
        }
    """

    def get(self, gline):
        gline = json.loads(gline)
        startt = gline['startt']
        endt = gline['endt']
        metagroup_descr = gline['metagroup_descr']

        graphs = []
        for elem in gline['graphs']:
            graphs.append(self.get_chart(elem['groups'], metagroup_descr, self.DESCRS[elem['signal']], startt, endt))

        repl = highcharts_render.renderall(graphs)
        return Response(response=repl, status=200, headers=None, mimetype=None, content_type="text/html",
                        direct_passthrough=False)

    def get_chart(self, groups, metagroup_descr, chart_descr, startt, endt):
        charttype = highcharts_render.ChartType("line", zoomable=True)
        title = highcharts_render.Title("%s (%s)" % (chart_descr["title"], metagroup_descr))
        axes = highcharts_render.Axes("datetime", "Date", chart_descr["yaxes_descr"], minvalue=0)
        tooltip = highcharts_render.Tooltip()
        plotoptions = highcharts_render.MongodbHistPlotOptions(stacking=False)

        signal_results = {}
        from utils.mongo import show_multigroup_graph
        for signal, signal_descr in chart_descr["signals"]:
            params = {
                "groups": groups,
                "signal": signal,
                "startt": startt,
                "endt": endt,
                "median_border": 90,
                "median_width": 24 * 7,
                "compress": True,
            }
            params = show_multigroup_graph.get_parser().parse_json(params)
            show_multigroup_graph.normalize(params)

            signal_results[signal] = map(lambda (x, y): {"x": x, "y": y, "suffix": ""},
                                         show_multigroup_graph.main(params))

        for lst in signal_results.values():
            for i, d in enumerate(lst):
                d["host_power"] = signal_results[chart_descr["main_signal"]][i]["y"]

        allseries = []
        for signal, signal_descr in chart_descr["signals"]:
            allseries.append(highcharts_render.Series(signal_descr, signal_results[signal], xisdate=True))

        if chart_descr["title"] == "Cpu usage":  # FIXME
            modifiers = [highcharts_render.TModifyToPercentCpuUsage('<b>CPU percents</b>')]
        else:
            modifiers = []

        return highcharts_render.rendergraph(charttype, title, axes, tooltip, plotoptions, allseries, modifiers)


class UtilPage(BasePage):
    RELATIVE_URL = '/utils/<string:util_name>'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def process_get_or_post(self, util_name, action, raw_json):
        if util_name not in wrapped_utils.UTILS:
            raise Exception("Unkown util %s" % util_name)

        if action is None:
            raise Exception("No parameter <action> in request CGI")
        if action not in ['get_params', 'check_params', 'get_result']:
            raise Exception("Incorrect parameter <action> in request CGI" % action)

        if raw_json is None:
            json_params = {}
        else:
            json_params = {}
            for elem in json.loads(raw_json):
                name, value = elem['name'], elem['value']

                if isinstance(value, unicode):
                    value = value.encode('utf-8')

                if name not in json_params:
                    json_params[name] = value
                elif not isinstance(json_params[name], list):
                    json_params[name] = [json_params[name], value]
                else:
                    json_params[name].append(value)

        if action == 'get_params':
            return wrapped_utils.UTILS[util_name].get_params(self, request, json_params)
        elif action == 'check_params':
            return wrapped_utils.UTILS[util_name].check_params(self, request, json_params, raw_json)
        elif action == 'get_result':
            return wrapped_utils.UTILS[util_name].get_result(self, request, json_params)

    def get(self, util_name):
        action = request.args.get('action')
        json_params = request.args.get('json', None)

        return self.process_get_or_post(util_name, action, json_params)

    def post(self, util_name):
        if not request.json:
            raise Exception(NO_JSON_IN_POST_DATA)

        action = request.args.get('action', None)
        json_params = request.json.get('json', None)

        return self.process_get_or_post(util_name, action, json_params)


class UtilsPage(BasePage):
    RELATIVE_URL = '/utils'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        result = {
            'utils': map(lambda x: {"title": x.TITLE, "section": x.SECTION, "util_name": x.NAME}, wrapped_utils.UTILS.itervalues()),
            'actions': [
                dict(
                    name=wrapped_utils.UTILS[util].TITLE,
                    url=wrapped_utils.UTILS[util].NAME,
                    is_changing_db=wrapped_utils.UTILS[util].IS_CHANGING_DB,
                    enabled=True,
                    hint=""
                ) for util in sorted(wrapped_utils.UTILS.keys())
            ]
        }

        return self.finalize_result(result, update_actions=True)


class UtilSlaveFormPage(BasePage):
    RELATIVE_URL = '/util_slave_form_page'

    PARAMS = ['srcgroup', ]

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        util_name = request.args['util_name']
        util = wrapped_utils.UTILS[util_name]
        util_slave_form_funcs = util.get_slave_form_params()

        params = {}
        fields_to_update = set()
        for arg in request.args:
            if arg == 'util_name':
                continue

            if arg in self.PARAMS:
                params[arg] = request.args[arg]
            elif int(request.args[arg]) == 1:
                fields_to_update.add(arg)

        result = {
            "update_master_form_fields": dict(
                map(lambda x: (x, util_slave_form_funcs[x](self, params)), fields_to_update)),
        }

        return self.finalize_result(result)


class ValidateHostNamesPage(BasePage):
    RELATIVE_URL = "/validate_host_names"

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def post(self):
        host_names = request.json['hostnames']

        resolved_names = {}
        resolved_hosts = []
        used_hosts = set()
        hosts_by_short_name = None
        for source_name in host_names:
            result_host = None
            result_error = None

            if '.' in source_name:
                if self.db.hosts.has_host(source_name):
                    result_host = self.db.hosts.get_host_by_name(source_name)
                    result_error = None
                else:
                    result_host = None
                    result_error = "Unknown host"
            else:
                if hosts_by_short_name is None:
                    hosts_by_short_name = {}
                    for host in self.db.hosts.get_hosts():
                        short_name = host.get_short_name()
                        if short_name in hosts_by_short_name:
                            # ambigious
                            hosts_by_short_name[short_name] = None
                        else:
                            hosts_by_short_name[short_name] = host

                if source_name in hosts_by_short_name:
                    result_host = hosts_by_short_name[source_name]
                    if result_host is None:
                        result_error = "Ambigious host short name, two or more hosts have the same name"
                    else:
                        result_error = None
                else:
                    result_host = None
                    result_error = "Unknown host"

            resolved_names[source_name] = {"hostname": result_host.name if result_host else None, "error": result_error}
            if result_host:
                if result_host not in used_hosts:
                    resolved_hosts.append(result_host)
                    used_hosts.add(result_host)

        result = {
            "resolved_names": resolved_names
        }

        hosts_info = build_group_hosts_stats_2(resolved_hosts)
        result.update(hosts_info)

        return self.finalize_result(result)


class GroupSlaveFormPage(BasePage):
    RELATIVE_URL = "/group_slave_form_page"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        groupname = request.args.get('srcgroup', None)
        if groupname is None:
            raise Exception("No <groupname> parameter in request")

        group = CURDB.groups.get_group(groupname)
        card_info = CardUpdater().get_group_card_info_old(group)
        card_info_dict = dict(map(lambda x: (tuple(x['path']), x.get('value', None)), card_info))

        result = {}
        for k in request.args:
            if k == 'srcgroup':
                continue
            if int(request.args[k]) == 0:
                continue

            path = tuple(k.split('.'))
            if path not in card_info_dict:
                raise Exception("Path <%s> not found in group card" % k)

            result['/'.join(path)] = card_info_dict[path]

        result = {
            "update_master_form_fields": result,
        }

        return self.finalize_result(result)


class CtypesPage(BasePage):
    RELATIVE_URL = "/ctypes"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        result = map(lambda x: {"ctype": x["name"], "descr": x["descr"]}, self.db.ctypes.get_ctypes())
        result.sort(cmp=lambda x, y: cmp(x["ctype"], y["ctype"]))

        return {
            "ctypes": result,
            "actions": [
                dict(name="Create Ctype", section="create", enabled=True, is_changing_db=True, hint=None),
            ],
        }

    def post(self):
        if not request.json:
            raise Exception(NO_JSON_IN_POST_DATA)

        if "action" not in request.json:
            raise Exception("No <action> field in json <%s>" % request.json)

        if request.json["action"] == "add":
            new_ctype = core.ctypes.CType(core.ctypes, {"name": request.json["ctype"], "descr": request.json["descr"]})
            success, log, exception_info, _ = self.subscript_exec(
                functools.partial(self.db.ctypes.add_ctype, new_ctype))
            commit_msg = "[ctypes] [add] Added ctype <%s>" % request.json["ctype"]
            hint_ctype = request.json["ctype"]
        elif request.json["action"] == "remove":
            success, log, exception_info, _ = self.subscript_exec(
                functools.partial(self.db.ctypes.remove_ctype, request.json["ctype"]))
            commit_msg = "[ctypes] [remove] Removed ctype <%s>" % request.json["ctype"]
            hint_ctype = None
        elif request.json["action"] == "rename":
            success, log, exception_info, _ = self.subscript_exec(
                functools.partial(self.db.ctypes.rename_ctype, request.json["ctype"], request.json["new_ctype"]))
            commit_msg = "[ctypes] [rename] Renamed ctype <%s> into <%s>" % (
            request.json["ctype"], request.json["new_ctype"])
            hint_ctype = request.json["new_ctype"]
        else:
            raise Exception("Unknown action <%s> in json <%s>" % (request.json["action"], request.json))

        self.db.update(smart=True)
        self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        result = {
            "operation log": log,
            "operation success": success,
            "commit" : self.db.get_repo().get_last_commit_id(),
            "exception info": exception_info,
            "ctype": hint_ctype,
        }
        return self.finalize_result(result)


class CtypePage(BasePage):
    RELATIVE_URL = "/ctypes/<string:ctype_name>"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self, ctype_name):
        ctype = self.db.ctypes.get_ctype(ctype_name)
        affected_groups = map(lambda x: x.card.name, self.db.ctypes.get_groups_with_ctype(ctype.name))
        summary = [
            dict(key="Ctype name", value=ctype.name),
            dict(key="Ctype descriptoin", value=ctype.descr),
            dict(key="Groups with this ctype", value=len(affected_groups)),
        ]

        return {
            "name": ctype.name,
            "descr": ctype.descr,
            "groups": affected_groups,
            "summary": summary,
            "actions": [
                dict(name="Rename", section="rename", enabled=True, is_changing_db=True, hint=None),
                dict(name="Remove", section="remove", enabled=True, is_changing_db=True, hint=None),
            ],
        }


class ItypesPage(BasePage):
    RELATIVE_URL = "/itypes"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        result = map(lambda x: {"itype": x["name"], "descr": x["descr"]}, self.db.itypes.get_itypes())
        result.sort(cmp=lambda x, y: cmp(x["itype"], y["itype"]))

        return {
            "itypes": result,
        }

    def post(self):
        if not request.json:
            raise Exception(NO_JSON_IN_POST_DATA)

        if "action" not in request.json:
            raise Exception("No <action> field in json <%s>" % request.json)

        if request.json["action"] == "add":
            new_itype = core.itypes.IType(core.itypes, {"name": request.json["itype"], "descr": request.json["descr"], "config_type": request.json.get("config_type", None)})
            success, log, exception_info, _ = self.subscript_exec(
                functools.partial(self.db.itypes.add_itype, new_itype))
            commit_msg = "[itypes] [add] Added itype <%s>" % request.json["itype"]
            hint_itype = request.json["itype"]
        elif request.json["action"] == "remove":
            success, log, exception_info, _ = self.subscript_exec(
                functools.partial(self.db.itypes.remove_itype, request.json["itype"]))
            commit_msg = "[itypes] [remove] Removed itype <%s>" % request.json["itype"]
            hint_itype = None
        elif request.json["action"] == "rename":
            success, log, exception_info, _ = self.subscript_exec(
                functools.partial(self.db.itypes.rename_itype, request.json["itype"], request.json["new_itype"]))
            commit_msg = "[itypes] [rename] Renamed itype <%s> into <%s>" % (
            request.json["itype"], request.json["new_itype"])
            hint_itype = request.json["new_itype"]
        else:
            raise Exception("Unknown action <%s> in json <%s>" % (request.json["action"], request.json))

        # TODO: speed will be optimized later
        self.db.itypes.update()
        self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        result = {
            "operation log": log,
            "operation success": success,
            "commit" : self.db.get_repo().get_last_commit_id(),
            "exception info": exception_info,
            "itype": hint_itype,
        }
        return self.finalize_result(result)


class ItypePage(BasePage):
    RELATIVE_URL = "/itypes/<string:itype_name>"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self, itype_name):
        itype = self.db.itypes.get_itype(itype_name)
        affected_groups = map(lambda x: x.card.name, self.db.itypes.get_groups_with_itype(itype.name))
        summary = [
            dict(key="Itype name", value=itype.name),
            dict(key="Itype descriptoin", value=itype.descr),
            dict(key="Groups with this itype", value=len(affected_groups)),
        ]

        return {
            "name": itype.name,
            "descr": itype.descr,
            "groups": affected_groups,
            "summary": summary,
            "actions": [
                dict(name="Rename", section="rename", enabled=True, is_changing_db=True, hint=None),
                dict(name="Remove", section="remove", enabled=True, is_changing_db=True, hint=None),
            ],
        }


class SuggestHbfMacrosesPage(BasePage):
    """Return list of hbf macroses for suggest"""
    RELATIVE_URL = "/suggest_hbf_macroses"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        login = request.yauth.login

        gencfg_macroses = {x.name for x in CURDB.hbfmacroses.get_hbf_macroses()}
        # filter hbf macroses with same owner and add to result
        hbf_macroses = {x for (x, y) in HBF_GROUPS.data.iteritems() if login in y['owners']}
        hbf_macroses = {x for x in hbf_macroses if not x.startswith('_GENCFG_')}

        result = [dict(hbf_macros=x) for x in (gencfg_macroses | hbf_macroses)]

        return {
            'hbf_macroses': result,
        }


class HbfMacrosesPage(BasePage):
    RELATIVE_URL = "/hbf_macroses"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        result = map(lambda x: {"hbf_macros": x["name"], "descr": x["description"], "owners": x["owners"]}, self.db.hbfmacroses.get_hbf_macroses())
        result.sort(cmp=lambda x, y: cmp(x["hbf_macros"], y["hbf_macros"]))

        return {
            "hbf_macroses": result,
        }

    def post(self):
        if not request.json:
            raise Exception(NO_JSON_IN_POST_DATA)

        if "action" not in request.json:
            raise Exception("No <action> field in json <%s>" % request.json)

        if request.json["action"] == "add":
            request_owners = [x.strip() for x in request.json['owners'].strip().split(',') if x.strip() != '']
            success, log, exception_info, _ = self.subscript_exec(
                functools.partial(self.db.hbfmacroses.add_hbf_macros, request.json["name"], request.json["descr"], request_owners)
            )
            commit_msg = "[hbf_macroses] [add] Added hbf macros <%s>" % request.json["name"]
            hint_hbf_macros = request.json["name"]
        elif request.json["action"] == "remove":
            success, log, exception_info, _ = self.subscript_exec(
                functools.partial(self.db.hbfmacroses.remove_hbf_macros, request.json["name"]))
            commit_msg = "[hbf_macroses] [remove] Removed hbf macros <%s>" % request.json["name"]
            hint_hbf_macros = None
        else:
            raise Exception("Unknown action <%s> in json <%s>" % (request.json["action"], request.json))

        # TODO: speed will be optimized later
        self.db.hbfmacroses.update()
        self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        result = {
            "operation log": log,
            "operation success": success,
            "commit" : self.db.get_repo().get_last_commit_id(),
            "exception info": exception_info,
            "hbf_macros": hint_hbf_macros,
        }
        return self.finalize_result(result)


class HbfMacrosPage(BasePage):
    RELATIVE_URL = "/hbf_macroses/<string:hbf_macros_name>"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self, hbf_macros_name):
        hbf_macros = self.db.hbfmacroses.get_hbf_macros(hbf_macros_name)
        affected_groups = map(lambda x: x.card.name, self.db.hbfmacroses.get_groups_with_hbf_macros(hbf_macros.name))
        summary = [
            dict(key="Hbf macros name", value=hbf_macros.name),
            dict(key="Hbf macros description", value=hbf_macros.description),
            dict(key="Hbf macros owners", value=','.join(hbf_macros.owners)),
            dict(key="Groups with this itype", value=len(affected_groups)),
        ]

        return {
            "name": hbf_macros.name,
            "descr": hbf_macros.description,
            "groups": affected_groups,
            "summary": summary,
            "actions": [
                dict(name="Remove", section="remove", enabled=True, is_changing_db=True, hint=None),
            ],
        }


class IntlookupsPage(BasePage):
    RELATIVE_URL = "/intlookups"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        result = map(lambda x: {"intlookup": x}, self.db.intlookups.get_intlookups_names())
        result.sort(cmp=lambda x, y: cmp(x["intlookup"], y["intlookup"]))

        return {
            "intlookups": result,
        }


class IntlookupPage(BasePage):
    RELATIVE_URL = "/intlookups/<string:intlookup_name>"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self, intlookup_name):
        intlookup = self.api.db.intlookups.get_intlookup(str(intlookup_name))

        summary = []

        summary.append(dict(key="Intlookup name", value=frontend_output.TLeafElement(intlookup.file_name).to_json()))

        summary.append(dict(key="Basesearchers type",
                            value=frontend_output.TLeafElement(intlookup.base_type, tp='group').to_json()))

        if intlookup.tiers is not None:
            tiers_value = map(lambda x: frontend_output.TLeafElement(x, tp='tier'), intlookup.tiers)
            tiers_value = frontend_output.TLeafElement(tiers_value, tp='list').to_json()
            summary.append(dict(key="Intlookup tiers", value=tiers_value))

        summary.append(
            dict(key="Hosts per group", value=frontend_output.TLeafElement(intlookup.hosts_per_group).to_json()))

        # fill replicas count
        result = utils.common.show_replicas_count.jsmain({"intlookups": intlookup.file_name})
        result = result["intlookups_info"][0]
        summary.append(dict(key="Replicas per shard (min/avg/max)", value=frontend_output.TLeafElement(
            "%.2f/%.2f/%.2f" % (result["min_replicas"], result["avg_replicas"], result["max_replicas"])).to_json()))

        # fill power per shard
        result = utils.common.show_replicas_count.jsmain({"intlookups": intlookup.file_name, "show_power": True})
        result = result["intlookups_info"][0]
        summary.append(dict(key="Power per shard (min/avg/max)", value=frontend_output.TLeafElement(
            "%.2f/%.2f/%.2f" % (result["min_replicas"], result["avg_replicas"], result["max_replicas"])).to_json()))

        # fill total intlookup power
        total_power = sum(map(lambda x: x.power, intlookup.get_used_base_instances()))
        group_power = sum(map(lambda x: x.power, self.api.db.groups.get_group(intlookup.base_type).get_instances()))
        if group_power == 0.0:
            group.power = 1.0
        summary.append(dict(key="Intlookup power", value=frontend_output.TLeafElement(
            "%.2f (%.2f%% of %s)" % (total_power, total_power / group_power * 100., intlookup.base_type)).to_json()))

        # fill instances count
        base_instances_count = len(intlookup.get_used_base_instances())
        int_instances_count = len(intlookup.get_used_int_instances())
        intl2_instances_count = len(intlookup.get_intl2_instances())
        value = "%d base / %d int / %d intl2" % (base_instances_count, int_instances_count, intl2_instances_count)
        summary.append(
            dict(key="Instances count (base/int/intl2)", value=frontend_output.TLeafElement(value).to_json()))

        return {
            "name": intlookup.file_name,
            "summary": summary
        }


class MetaprjsPage(BasePage):
    RELATIVE_URL = "/metaprjs"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        result = sorted([x for x in self.api.db.constants.METAPRJS.keys()])

        return {
            "metaprjs": result,
            "actions": [
                dict(name="Create Ctype", section="create", enabled=True, is_changing_db=True, hint=None),
            ],
        }


class MiscUserListPage(BasePage):
    RELATIVE_URL = "/misc/user_list"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self):
        return {
            "users": get_possible_group_owners(),  # 15 min cache
        }


class MdDocPage(BasePage):
    RELATIVE_URL = "/mddoc/<string:fname>/<string:path>"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def get(self, fname, path):
        mddoc = TMdDoc(os.path.join(self.api.db.SCHEMES_DIR, SCHEME_LEAF_DOC_FILE))

        html_doc = mddoc.get_doc(fname, path, TMdDoc.EFormat.HTML)
        success = (html_doc is not None)

        return {
            "doc": html_doc,
            "operation success": success,
        }


class VolumesPage(BasePage):
    """Volumes modification"""
    RELATIVE_URL = "/groups/<string:group>/volumes"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.wbe_authorizer.can_user_view_db(login)

    def gencfg_authorize_post(self, login, request):
        return self.api.wbe_authorizer.can_user_modify_group(login, request.view_args['group'])

    def get(self, group):
        """Get list of group volumes as raw json"""
        return {
            "volumes": gaux.aux_volumes.volumes_as_json_string(group),
            "operation success": True,
        }

    def post(self, group):
        group = self.db.groups.get_group(group)

        try:
            new_volumes = request.json['volumes']
            new_volumes = [gaux.aux_volumes.TVolumeInfo.from_json(x) for x in new_volumes]
            new_volumes.sort(key=lambda x: x.guest_mp)
        except Exception as e:
            return {
                "group": group.card.name,
                "operation log": traceback.format_exc(),
                "operation success": False,
                "exception info": {
                    "class": e.__class__.__name__,
                },
            }

        group.card.reqs.volumes = [x.to_card_node() for x in new_volumes]
        gaux.aux_volumes.update_ssd_hdd_group_reqs(group)
        group.mark_as_modified()
        self.db.groups.update(smart=True)

        commit_msg = "[groups] [volumes] Updated group %s volumes" % group.card.name
        self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        return {
            "commit": self.db.get_repo().get_last_commit_id(url=self.db.get_repo().svn_info_relative_url()),
            "group": group.card.name,
            "operation success": True,
        }


from api_base import WebApiBase, AuthorizerBase, build_base_argparser


class WbeAuthorizer(AuthorizerBase):
    """ Contains most typical operation types """

    def __init__(self, api):
        self.api = api

    def _ignore_auth_reqs(self):
        return self.api.options.testing or not self.api.options.allow_auth

    def can_user_view_db(self, login):
        return self._ignore_auth_reqs() \
               or login is not None

    def can_user_allocate_hosts(self, login):
        return self._ignore_auth_reqs() \
               or login is not None

    def can_user_modify_group(self, login, group_name):
        return self._ignore_auth_reqs() \
               or self.is_super_user(login) \
               or login in self.api.db.groups.get_group(group_name).get_extended_owners()

    def can_user_modify_tiers(self, login):
        return self._ignore_auth_reqs() \
               or login is not None

    def can_user_create_new_group(self, login):
        return self._ignore_auth_reqs() \
               or login is not None


class WbeApi(WebApiBase):
    COMMIT_PREFIX = "[web] "

    def __init__(self, *args, **kwargs):
        WebApiBase.__init__(self, *args, **kwargs)

        self.templates_dir = os.path.join(os.path.dirname(__file__), 'templates')

        # TODO: change name
        self.wbe_authorizer = WbeAuthorizer(self)

        # TODO: disabled during migration to Git
        # if self.options.allow_commit:
        #    if self.db.has_local_modification() and not self.options.revert_local_modification:
        #        raise Exception("Cannot start wbe in commit mode, DB has local modifications and neither --production nor --revert-local-modifications is specified")

def parse_cmd(jsonargs=None):
    parser = build_base_argparser("Web backend")

    if jsonargs is None:
        if len(sys.argv) == 1:
            sys.argv.append('-h')
        options = parser.parse_args()
    else:
        options = parser.parse_json(jsonargs)

    if options.production and options.testing:
        raise Exception("Uncompatible parameters: --production and --testing")

    if options.production:
        options.allow_commit = True
        options.allow_auth = True
    elif options.testing:
        options.allow_commit = False
        options.allow_auth = False

    return options


def prepare_wbe(jsonargs=None):
    options = parse_cmd(jsonargs=jsonargs)

    app = Flask(__name__)
    api = WbeApi(app, options=options)
    Compress(app)

    constant_resources = [
        (FailPage, '/fail'),
        (RuCheckPage.init(api), '/rucheck'),
        (Favicon, '/favicon.ico'),
    ]
    for resource in constant_resources:
        api.add_absolute_resource(resource[0], resource[1])

    data_resources = [
        MainPage,
        AuxHostsPage,
        HostPage,
        SearchPage,
        GroupsPage,
        GroupPage,
        HostsStatsPage,
        TierPage,
        TiersPage,
        GroupHostsPage,
        GroupCardPage,
        GroupHostsDiffPage,
        GroupSlaveFormPage,
        MongodbGroupCurrentUsagePage,
        MongodbHostHistUsagePage,
        MongodbStatsPage,
        MongodbMetagroupsStatsPage,
        UtilPage,
        UtilsPage,
        UtilSlaveFormPage,
        ValidateHostNamesPage,
        ReservedHostsPage,
        CtypesPage,
        CtypePage,
        ItypesPage,
        ItypePage,
        SuggestHbfMacrosesPage,
        HbfMacrosesPage,
        HbfMacrosPage,
        IntlookupsPage,
        IntlookupPage,
        MetaprjsPage,
        MdDocPage,
        VolumesPage,

        # various misc handlers
        MiscUserListPage,
    ]
    for _resource in data_resources[:]:
        resource = _resource.init(api)
        api.add_branch_resource(resource, resource.RELATIVE_URL)

    return app, options


def main():
    app, options = prepare_wbe()

    # in debug mode server is automatically restarted on every change and debug info
    # host - is a host ip which we listen to
    app.run(debug=(not options.production), host=options.host, port=options.port, use_reloader=False,
            threaded=options.threaded)


if __name__ == '__main__':
    WbeApi.global_run_service(main)
