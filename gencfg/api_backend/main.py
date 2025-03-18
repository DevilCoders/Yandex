#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../web_shared')))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../contrib')))
# DO NOT DELETE THIS!!!
# this will update sys.path which is required for import

from collections import OrderedDict

import gencfg

from flask import Flask, request, send_from_directory
from flask_compress import Compress
from flask_restful import Resource, reqparse
import copy
import time
from collections import defaultdict
import json
import functools
import re

from core.card.updater import CardUpdater
from core.card.types import ByteSize, Date
from core.igroups import IGroup
from core.settings import SETTINGS
from core.audit.cpu import suggest_for_group
import core.ctypes
import utils.common.update_igroups
import utils.common.update_card
import utils.api.searcherlookup_groups_instances
import utils.mongo.update_commits_info
import optimizers.dynamic.main as update_fastconf
from gaux.aux_performance import perf_timer
from config import MAIN_DIR
import jsonschema
import web_shared.req_cacher
from gaux.aux_ast import convert_to_ast_and_eval
from gaux.aux_utils import transform_tree
import gaux.aux_hbf
import gaux.aux_portovm
import gaux.aux_volumes
import gaux.aux_abc
import gaux.aux_staff

from api_base import WebApiBase, build_base_argparser, AuthorizerBase


class BasePage(Resource):
    @classmethod
    def init(cls, api):
        cls.api = api
        cls.db = api.db
        return cls


class MainPage(BasePage):
    RELATIVE_URL = "/"

    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def get(self):
        return {"branch": request.branch}


class SearcherlookupGroupInstancesPage(BasePage):
    RELATIVE_URL = "/searcherlookup/groups/<string:groupname>/instances"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    @web_shared.req_cacher.api_mongo_cache
    def _get(self, groupname):
        # check for group existance, will throw exception when group does not exist
        group = self.db.groups.get_group(groupname, raise_notfound=False)
        if group is None:
            raise WebApiBase.GencfgNotFoundError("Group <%s> not found in db" % groupname)

        util_params = {
            'groups': [group.card.name],
            'db': self.db,
        }
        result = utils.api.searcherlookup_groups_instances.jsmain(util_params)

        return result[groupname]

    def get(self, groupname):
        return self._get(request, groupname)


class DbGroupInstancesPage(BasePage):
    RELATIVE_URL = '/groups/<string:groupname>/instances'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    @web_shared.req_cacher.api_mongo_cache
    def _get(self, groupname):
        # check for group existance, will throw exception when group does not exist
        if not self.db.groups.has_group(groupname):
            raise Exception("Unknown group \"%s\"" % groupname)

        # ==================================== GENCFG-1654 START ===============================================
        if groupname == 'ALL_SEARCH_VM':
            is_guest_group = True
            instances = []
            for group in self.db.groups.get_groups():
                if group.card.properties.nonsearch == True:
                    continue
                if group.card.on_update_trigger is not None:
                    continue
                if group.card.tags.itype not in ('psi', 'portovm'):
                    continue
                instances.extend([gaux.aux_portovm.guest_instance(x, db=self.db) for x in group.get_kinda_busy_instances()])
        # ==================================== GENCFG-1654 FINISH ==============================================
        elif groupname.endswith('_GUEST'):
            is_guest_group = True
            group = self.db.groups.get_group(groupname.rpartition('_')[0])
            instances = [gaux.aux_portovm.guest_instance(x, self.db) for x in group.get_kinda_busy_instances()]
        else:
            is_guest_group = False
            group = self.db.groups.get_group(groupname)
            if len(group.card.intlookups) > 0:
                instances = group.get_busy_instances()
            else:
                instances = group.get_instances()

        result = []
        for instance in instances:
            # generate hbf hostname/addr
            hbf_mtn_hostname = None
            hbf_mtn_ipv6addr = None
            instance_hbf_info = gaux.aux_hbf.generate_hbf_info(group, instance)
            if (not is_guest_group) and ('interfaces' in instance_hbf_info) and ('backbone' in instance_hbf_info['interfaces']):
                hbf_mtn_hostname = instance_hbf_info['interfaces']['backbone']['hostname']
                hbf_mtn_ipv6addr = instance_hbf_info['interfaces']['backbone']['ipv6addr']

            result.append(dict(
                hostname=instance.host.name,
                port=instance.port,
                power=instance.power,
                dc=instance.host.dc,
                location=instance.host.location,
                domain=instance.host.domain,
                hbf_mtn_hostname=hbf_mtn_hostname,
                hbf_mtn_ipv6addr=hbf_mtn_ipv6addr,
            ))

        result.sort(key=lambda x: (x['hostname'], x['port']))
        result = {"instances": result}
        return result

    def get(self, groupname):
        return self._get(request, groupname)


class DbIntlookupInstancesPage(BasePage):
    RELATIVE_URL = '/intlookups/<string:intlookup_name>/instances'

    AVAILABLE_INSTANCE_TYPES = ['base', 'int']

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self, intlookup_name):
        intlookup_name = str(intlookup_name)
        parser = reqparse.RequestParser()
        parser.add_argument('instance_type', help='Instance type: base or int')
        args = parser.parse_args()

        if args.instance_type and args.instance_type not in self.AVAILABLE_INSTANCE_TYPES:
            raise Exception('Instance type should empty or one of: {}'.format(self.AVAILABLE_INSTANCE_TYPES))

        # exception will be raised if intlookup does not exist,
        # in this case response code will be 400
        self.db.intlookups.check_name(intlookup_name)
        self.db.intlookups.check_existance(intlookup_name)

        intlookup = self.db.intlookups.get_intlookup(intlookup_name)

        if args.instance_type == 'int':
            instances = intlookup.get_int_instances()
        elif args.instance_type == 'base':
            instances = intlookup.get_base_instances()
        elif args.instance_type == 'intl2':
            instances = intlookup.get_intl2_instances()
        else:
            instances = intlookup.get_instances()

        result = [{
                      'hostname': instance.host.name,
                      'port': instance.port,
                      'power': instance.power,
                      'dc': instance.host.dc,
                      'location': instance.host.location,
                      'domain': instance.host.domain,
                  } for instance in instances]
        result = {'instances': result}
        return result


class DbIntlookupInstancesByShardPage(BasePage):
    RELATIVE_URL = '/intlookups/<string:intlookup_name>/instances_by_shard'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self, intlookup_name):
        if not self.db.intlookups.has_intlookup(str(intlookup_name)):
            raise WebApiBase.GencfgNotFoundError("Intlookup <%s> not found in db" % intlookup_name)

        intlookup = self.db.intlookups.get_intlookup(str(intlookup_name))

        result = []
        for shard_id in xrange(intlookup.get_shards_count()):
            subresult = intlookup.get_base_instances_for_shard(shard_id)
            subresult = map(lambda x: {
                'hostname': x.host.name,
                'port': x.port,
                'power': x.power,
                'dc': x.host.dc,
                'location': x.host.location,
                'domain': x.host.domain
            }, subresult)
            result.append(subresult)

        return {'instances_by_shard': result}


class DbIntlookupPage(BasePage):
    """Intlookup with all its topology (RX-260)"""

    RELATIVE_URL = '/intlookups/<string:intlookup_name>'

    def __convert_for_intlookup(self, instance):
        """Transfor instance to dict, corresponding to instance, in response"""
        return dict(
            group_name=instance.type,
            host_name=instance.host.name,
            port=instance.port,
            power=instance.power,
            # networking info
            ipv6addr=instance.ipv6addr,
            # location info
            switch=instance.host.switch,
            queue=instance.host.queue,
            dc=instance.host.dc,
            location=instance.host.location,
            # tier info
            tier_name=None,
            shard_name=None,
        )

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self, intlookup_name):
        if not self.db.intlookups.has_intlookup(str(intlookup_name)):
            raise WebApiBase.GencfgNotFoundError("Intlookup <%s> not found in db" % intlookup_name)

        intlookup = self.db.intlookups.get_intlookup(str(intlookup_name))
        intlookup_shards = [(intlookup.get_tier_for_shard(x), intlookup.get_primus_for_shard(x)) for x in xrange(intlookup.get_shards_count())]

        result = []
        current_shard = 0
        for intl2_group in intlookup.intl2_groups:
            # process intl2 instances
            intl2_instances = [self.__convert_for_intlookup(x) for x in intl2_group.intl2searchers]
            # process multishards
            multishards = []
            for multishard in intl2_group.multishards:
                brigades = []
                for brigade in multishard.brigades:
                    int_instances = [self.__convert_for_intlookup(x) for x in brigade.intsearchers]
                    base_instances = [self.__convert_for_intlookup(x[0]) for x in brigade.basesearchers]
                    for offset, base_instance in enumerate(base_instances):
                        base_instance['tier_name'] = intlookup_shards[current_shard+offset][0]
                        if base_instance['tier_name'] == 'None':
                            base_instance['tier_name'] = None
                        base_instance['shard_name'] = intlookup_shards[current_shard+offset][1]

                    brigades.append(dict(intesarchers=int_instances, basesearchers=base_instances))

                current_shard += intlookup.hosts_per_group
                multishards.append(dict(brigades=brigades))

            result.append(dict(intl2searchers=intl2_instances, multishards=multishards))

        return dict(
            name=intlookup.file_name,
            tiers=copy.copy(intlookup.tiers),
            hosts_per_group=intlookup.hosts_per_group,
            intl2_groups=result
        )


class DbCommitDescriptionPage(BasePage):
    RELATIVE_URL = '/description'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self):
        return {'description': self.db.get_repo().describe_commit()}


class FastconfGroupsPage(BasePage):
    RELATIVE_URL = "/fastconf/<string:fc_cluster>/groups"
    # TODO: move to shared class
    FORZA_GROUP = "ALL_DYNAMIC"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self, fc_cluster):
        if fc_cluster.lower() != "forza":
            raise Exception("Undefined fastconf cluster %s" % fc_cluster)

        return {
            "groups": [slave.card.name for slave in
                       self.api.db.groups.get_group(FastconfGroupsPage.FORZA_GROUP).slaves],
        }

    def gencfg_authorize_post(self, login, request):
        if not self.api.api_authorizer.can_user_update_fastconf(login):
            return False

        action = request.json["action"]
        group = request.json["group"] if "group" in request.json else None

        if action == "add":
            return self.api.api_authorizer.can_user_create_fastconf_group(login)
        elif action == "remove":
            return self.api.api_authorizer.can_user_update_fastconf_group(login, action, group)
        elif action == "remove_all":
            return self.api.api_authorizer.can_user_remove_all_fastconf_groups(login)
        else:
            assert False

    @perf_timer
    def post(self, fc_cluster):
        # set default group, as we have only one fastconf cluseter
        if fc_cluster.lower() != "forza":
            raise Exception("Undefined fastconf cluster %s" % fc_cluster)
        util_request = copy.deepcopy(request.json)
        util_request["master_group"] = FastconfGroupsPage.FORZA_GROUP

        if 'equal_instances_power' not in util_request:
            util_request['equal_instances_power'] = True

        action = util_request["action"]
        if action not in ["add", "remove", "remove_all"]:
            raise Exception("Undefined action: %s" % action)

        options = update_fastconf.parse_json(util_request)

        options.apply = True
        if options.ctype is None:
            options.ctype = "none"
        if options.itype is None:
            options.itype = "none"

        update_fastconf.main(options, self.api.db)

        # ======================================= GENCFG-1517 START =============================================
        try:
            group = self.api.db.groups.get_group(options.group)
            for property_path, property_value in util_request.get('properties', dict()).iteritems():
                update_card_options = dict(
                    groups=[group],
                    update_slaves=False,
                    key=property_path,
                    value=property_value,
                    apply=True
                )
                utils.common.update_card.jsmain(update_card_options)
        except:
            self.api.db.groups.remove_group(options.group)
            raise
        # ======================================= GENCFG-1517 FINISH ============================================

        # HACK:
        self.api.db.reset_searcherlookup()
        commit_msg = update_fastconf.gen_commit_message(options)
        try:
            self.api.db.update(smart=True)
        except:
            self.api.db.groups.remove_group(options.group)
            raise
        self.api.commit(commit_msg, request, reset_changes_on_fail=True)
        return {}


class FastconfLocationsPage(BasePage):
    RELATIVE_URL = '/fastconf/<string:fc_cluster>/locations'

    # TODO: move to shared class
    FORZA_GROUP = "ALL_DYNAMIC"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self, fc_cluster):
        if fc_cluster.lower() != "forza":
            raise Exception("Undefined fastconf cluster %s" % fc_cluster)

        fake_json = {"action": "get_locations", "master_group": FastconfLocationsPage.FORZA_GROUP}
        util_request = update_fastconf.parse_json(fake_json)
        locations = update_fastconf.main(util_request, self.api.db)

        return {
            "locations": locations,
        }


class GroupsPage(BasePage):
    RELATIVE_URL = '/groups'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self):
        return {
            "group_names": sorted([group.card.name for group in self.db.groups.get_groups()]),
        }


class GroupPage(BasePage):
    RELATIVE_URL = '/groups/<string:group_name>'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self, group_name):
        if not self.db.groups.has_group(group_name):
            raise Exception("Unknown group \"%s\"" % group_name)

        if group_name == 'ALL_SEARCH_VM':
            # ================================== GENCFG-1654 START ===============================
            hosts = []
            for group in self.db.groups.get_groups():
                if group.card.properties.nonsearch == True:
                    continue
                if group.card.on_update_trigger is not None:
                    continue
                if group.card.tags.itype not in ('psi', 'portovm'):
                    continue
                hosts.extend([gaux.aux_portovm.guest_instance(x, db=self.db).host.name for x in group.get_kinda_busy_instances()])
            hosts = sorted(set(hosts))
            return dict(
                group=group_name,
                owners=[],
                master=None,
                hosts=hosts,
            )
            # ================================== GENCFG-1654 START ===============================
        elif group_name.endswith('_GUEST'):
            # ================================== RX-213 START ====================================
            dom_group_name = group_name.rpartition('_GUEST')[0]
            if not self.db.groups.has_group(dom_group_name):
                raise Exception("Unknown group <{}>".format(group_name))

            group = self.db.groups.get_group(dom_group_name)
            guest_instances = [gaux.aux_portovm.guest_instance(x, self.db) for x in group.get_kinda_busy_instances()]
            guest_hosts = sorted(x.host.name for x in guest_instances)
            return dict(
                group=group_name,
                owners=sorted(set(group.card.owners + group.card.guest.owners)),
                master=None,
                hosts=guest_hosts
            )
            # ================================== RX-213 FINISH ===================================
        else:
            group = self.db.groups.get_group(group_name)
            return {
                "group": group.card.name,
                "owners": sorted(list(set(group.card.owners))),
                "master": group.card.master.card.name if group.card.master else None,
                "hosts": sorted([host.name for host in group.getHosts()])
            }


class GroupAllocPage(BasePage):
    RELATIVE_URL = '/groups/<string:group_name>/alloc'

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def post(self, group_name):
        request_json = copy.deepcopy(request.json)
        request_json_schema = json.loads(
            open(os.path.join(self.api.db.SCHEMES_DIR, "json", "api.alloc_group.json")).read())
        jsonschema.validate(request_json, request_json_schema)

        create_group_params = request_json
        create_group_params["action"] = "addgroup"
        create_group_params["group"] = group_name
        if "instancePort" in create_group_params:
            create_group_params["instance_port_func"] = "old%s" % create_group_params["instancePort"]
        if "instanceCount" in create_group_params:
            create_group_params["instance_count_func"] = "exactly%s" % create_group_params["instanceCount"]
        if ("memoryPerInstance" in create_group_params):
            create_group_params["properties"] = "reqs.instances.memory_guarantee=%s" % (create_group_params["memoryPerInstance"])
        if "cpuPerInstance" in create_group_params:
            create_group_params['instance_power_func'] = 'exactly{}'.format(create_group_params['cpuPerInstance'])
        else:
            create_group_params['instance_power_func'] = 'exactly1'

        # check if all hosts from reserved groups
        if create_group_params.get("parent_group", None) is None:
            reserved_groups = self.api.db.groups.get_reserved_groups()
        else:
            reserved_groups = [self.api.db.groups.get_group(create_group_params["parent_group"])]

        reserved_hosts = sum(map(lambda x: x.getHosts(), reserved_groups), [])
        reserved_hosts = set(map(lambda x: x.name, reserved_hosts))
        nonreserved_hosts = set(create_group_params.get("hosts", [])) - set(reserved_hosts)
        if len(nonreserved_hosts) > 0:
            raise Exception("Can not add non-reserved hosts <%s> to group <%s>" % (",".join(nonreserved_hosts), group_name))

        try:
            utils.common.update_igroups.jsmain(create_group_params)
        except Exception:
            # we got exception, and update_igroups could add some files via 'svn add' command
            # thus we have to reset changes
            self.api.db.get_repo().reset_all_changes()
            self.api.db.purge()
            raise

        commit_msg = "[create_group] Created group %s using api" % group_name
        self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        return {
            "added": group_name,
        }


class GroupRemovePage(BasePage):
    RELATIVE_URL = '/groups/<string:group_name>/remove'

    def gencfg_authorize_post(self, login, request):
        group_name = str(request.view_args['group_name'])
        return self.api.api_authorizer.can_user_update_fastconf_group(login, "remove", group_name)

    def post(self, group_name):
        remove_group_params = {
            "action": "remove",
            "group": group_name,
            "recursive": "True"
        }
        utils.common.update_igroups.jsmain(remove_group_params)

        commit_msg = "[remove_group] Removed group %s using api" % group_name
        self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        return {
            "removed": group_name,
        }


class GroupSlavesPage(BasePage):
    RELATIVE_URL = '/groups/<string:group_name>/slaves'

    def gencfg_authorize_get(self, login, request):
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self, group_name):
        if not self.db.groups.has_group(group_name):
            raise Exception("Unknown group \"%s\"" % group_name)

        group = self.db.groups.get_group(group_name)

        return {
            "slaves": map(lambda x: x.card.name, group.slaves),
        }


class GroupCardPage(BasePage):
    RELATIVE_URL = '/groups/<string:group_name>/card'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    @web_shared.req_cacher.api_mongo_cache
    def _get(self, group_name):
        if not self.db.groups.has_group(group_name):
            raise Exception("Unknown group \"%s\"" % group_name)

        group = self.db.groups.get_group(group_name)

        # convert group card to jonable dict
        group_card_dict = group.card.as_dict()

        # ==================================== RX-302 START ===============================================
        volumes_objects = gaux.aux_volumes.volumes_as_objects(group)
        group_card_dict['reqs']['volumes'] = [x.to_card_node().as_dict() for x in volumes_objects]
        # ==================================== RX-302 FINISH ==============================================

        # ==================================== RX-447 START ===============================================
        group_card_dict['owners_abc_roles'] = gaux.aux_abc.abc_roles_to_nanny_json(group)
        # ==================================== RX-447 STOP ================================================

        # ==================================== RX-511 START ===============================================
        group_card_dict['resolved_owners'] = gaux.aux_staff.unwrap_dpts(group.card.owners)
        # ==================================== RX-511 STOP ================================================

        # ==================================== RX-493 START ===============================================
        group_card_dict['resources'] = dict(ninstances=len(group.get_kinda_busy_instances()))
        # ==================================== RX-493 STOP ================================================


        def recurse_fix_json_types(obj):
            if isinstance(obj, (dict, OrderedDict)):
                result = OrderedDict()
                for k, v in obj.iteritems():
                    result[k] = recurse_fix_json_types(v)
                return result
            elif isinstance(obj, list):
                return [recurse_fix_json_types(x) for x in obj]
            elif isinstance(obj, ByteSize):
                return obj.value
            elif isinstance(obj, IGroup):
                return obj.card.name
            elif isinstance(obj, Date):
                return str(obj)
            else:
                return obj
        group_card_dict = recurse_fix_json_types(group_card_dict)

        return group_card_dict

    def get(self, group_name):
        return self._get(request, group_name)

    def gencfg_authorize_post(self, login, request):
        return self.api.api_authorizer.can_user_modify_group(login, request.view_args['group_name'])

    def post(self, group_name):
        group = self.api.db.groups.get_group(group_name)
        property_path = request.json['property_path']
        property_value = request.json['property_value']

        try:
            update_card_options = dict(
                groups=[group],
                update_slaves=False,
                key=property_path,
                value=property_value,
                apply=True
            )
            utils.common.update_card.jsmain(update_card_options)
        except:
            self.api.db.purge()
            raise

        commit_msg = "[update_card] Updated group <{}> card: set property <{}> to <{}>".format(group_name, property_path, property_value)
        self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        return {}


class HostsPage(BasePage):
    RELATIVE_URL = '/hosts'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self):
        return {
            "host_names": sorted([host.name for host in self.db.hosts.get_hosts()]),
        }


class HostsHwdataPage(BasePage):
    RELATIVE_URL = '/hosts_data'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self):
        return {
            "hosts_data": map(lambda x: x.save_to_json_object(), self.db.hosts.get_hosts())
        }

    def post(self):
        if "hosts" not in request.json:
            raise Exception("Missing field \"hsots\" in request body")
        host_names = request.json["hosts"]

        notfound_hosts = sorted(filter(lambda x: not self.db.hosts.has_host(x), host_names))
        found_hosts = filter(lambda x: self.db.hosts.has_host(x), host_names)

        hosts = self.db.hosts.get_hosts_by_name(found_hosts)

        return {
            "hosts_data": map(lambda x: x.save_to_json_object(), hosts),
            "notfound_hosts": notfound_hosts,
        }


class HostsToGroupsPage(BasePage):
    RELATIVE_URL = '/hosts/hosts_to_groups'

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def post(self):
        if "hosts" not in request.json:
            raise Exception("Missing field \"hosts\" in request body")
        hosts_names = request.json["hosts"]
        hosts_names = set(hosts_names)

        unknown_hosts = [host for host in hosts_names if not self.db.hosts.has_host(host)]
        if unknown_hosts:
            raise Exception("Unknown host names: %s" % ",".join(["\"%s\"" % name for name in sorted(unknown_hosts)]))
        hosts = self.db.hosts.get_hosts_by_name(list(hosts_names))

        include_slaves = bool(request.json.get("include_slaves", True))
        include_background = bool(request.json.get("include_background", False))
        include_fake = bool(request.json.get("include_fake", False))

        groups_by_host = {}
        hosts_by_group = defaultdict(list)
        all_groups = set()
        for host in hosts:
            groups = self.db.groups.get_host_groups(host)
            if not include_slaves:
                groups = [group for group in groups if group.card.master is None]
            if not include_background:
                groups = filter(lambda x: x.card.properties.background_group == False, groups)
            if not include_fake:
                groups = filter(lambda x: x.card.properties.fake_group == False, groups)
            groups_by_host[host.name] = [group.card.name for group in groups]
            for group in groups:
                hosts_by_group[group.card.name].append(host.name)
                all_groups.add(group.card.name)

        return {
            "hosts": sorted(hosts_names),
            "groups": sorted(all_groups),
            "groups_by_host": groups_by_host,
            "hosts_by_group": dict(hosts_by_group)
        }


# TODO: remove, store all fqdns instead
class GuessGencfgFQDNPage(BasePage):
    RELATIVE_URL = '/hosts/guess_gencfg_fqdn'

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def post(self):
        if "hosts" not in request.json:
            raise Exception("Missing field \"hosts\" in request body")
        host_names = request.json["hosts"]
        host_names = set(host_names)

        raise_unresolved = request.json.get("raise_unresolved", False)

        short_to_fqdn = {host.name.split('.')[0]: host.name for host in self.db.hosts.get_hosts()}

        invalid_names = set()
        correct_names = {}
        for host_name in host_names:
            short_name = host_name.split('.')[0]
            if short_name not in short_to_fqdn:
                invalid_names.add(host_name)
            else:
                correct_names[host_name] = short_to_fqdn[short_name]
        if len(invalid_names) and raise_unresolved:
            raise Exception("Unknown host names: %s" % (",".join(sorted(["\"%s\"" % x for x in invalid_names]))))

        return {
            "gencfg_fqdns": correct_names,
            "unresolved_names": list(invalid_names),
        }


class HostInstancesTagsPage(BasePage):
    RELATIVE_URL = '/hosts/<string:hostnames>/instances_tags'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get_instances_with_tags(self, host):
        result = {}

        hostgroups = self.db.groups.get_host_groups(host)
        for hostgroup in hostgroups:
            generated_slookup = hostgroup.generate_searcherlookup()
            for instance in hostgroup.get_host_instances(host):
                if instance in generated_slookup.instances:
                    result[instance.name()] = sorted(generated_slookup.instances[instance])

        return result

    @web_shared.req_cacher.api_mongo_cache
    def _get(self, hostnames):
        result = {}
        notfound = []

        for hostname in hostnames.split(","):
            hostname = str(hostname)
            if self.db.hosts.has_host(hostname):
                result.update(self.get_instances_with_tags(self.db.hosts.get_host_by_name(hostname)))
            else:
                notfound.append(hostname)

        return {
            "instances_tags": result,
            "not_found": notfound
        }

    def get(self, hostnames):
        return self._get(request, hostnames)


class InstancePropsPage(BasePage):
    RELATIVE_URL = '/instances/<string:instance_str>/props'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self, instance_str):
        hostname, _, port = instance_str.partition(':')
        port = int(port)
        if not self.db.hosts.has_host(hostname):
            raise Exception("Host <%s> not found" % hostname)

        host = self.db.hosts.get_host_by_name(hostname)
        candidate_instances = filter(lambda x: x.port == port, self.db.groups.get_host_instances(host))
        if len(candidate_instances) == 0:
            raise Exception("Instance with port %s not found on host <%s>" % (port, hostname))

        instance = candidate_instances[0]
        return_props = dict(
            map(lambda (key, _1, _2): (key, getattr(instance.host, key)), instance.host.FAST_FILE_SCHEME))
        return_props['power'] = instance.power
        return_props['group'] = instance.type

        return return_props


class ModelsPage(BasePage):
    RELATIVE_URL = '/models'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self):
        result = dict()
        for model in self.db.cpumodels.models.itervalues():
            model_dict = {
                'model': model.model,
                'fullname': model.fullname,
                'url': model.url,
                'power': model.power,
            }
            result[model.model] = model_dict

        return {
            "gencfg_models": result,
        }


class DbStatePage(BasePage):
    """
        Url for getting db state, like last commit in current db, current tag/branch/...
    """

    RELATIVE_URL = '/db/state'

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self):
        repo = self.db.get_repo()

        local_commit = repo.get_last_commit()
        local_commit_description = repo.describe_commit()
        remote_commit = repo.get_last_commit(url=repo.svn_info_relative_url())

        return {
            "local_commit": {
                "commit_id": local_commit.commit,
                "author": local_commit.author,
                "message": local_commit.message,
                "short_descr": local_commit_description,
            },
            "remote_commit": {
                "commit_id": remote_commit.commit,
                "author": remote_commit.author,
                "message": remote_commit.message,
            },
        }


class CtypesPage(BasePage):
    RELATIVE_URL = "/ctypes"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self):
        result = sorted(map(lambda x: x.name, self.db.ctypes.get_ctypes()))
        return {
            "ctypes": result,
        }


class CtypePage(BasePage):
    RELATIVE_URL = "/ctypes/<string:ctype>"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    @web_shared.req_cacher.api_mongo_cache
    def _get(self, ctype):
        ctype = self.db.ctypes.get_ctype(ctype)
        return {
            "name": ctype.name,
            "descr": ctype.descr,
        }

    def get(self, ctype):
        return self._get(request, ctype)


class CtypeCreatePage(BasePage):
    RELATIVE_URL = "/ctypes/<string:ctype>/create"

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def post(self, ctype):
        request_json = copy.deepcopy(request.json)
        request_json_schema = json.loads(
            open(os.path.join(self.api.db.SCHEMES_DIR, "json", "api.create_ctype.json")).read())
        jsonschema.validate(request_json, request_json_schema)

        new_ctype = core.ctypes.CType(self.db.ctypes, {"name": str(ctype), "descr": str(request_json["descr"])})
        self.db.ctypes.add_ctype(new_ctype)
        self.db.ctypes.update(smart=True)

        commit_msg = "[create_ctype] Created ctype %s using api" % ctype
        self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        return {
            "created": ctype,
        }


class CtypeRemovePage(BasePage):
    RELATIVE_URL = "/ctypes/<string:ctype>/remove"

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def post(self, ctype):
        self.db.ctypes.remove_ctype(str(ctype))
        self.db.ctypes.update(smart=True)

        commit_msg = "[remove_ctype] Removed ctype %s using api" % ctype
        self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        return {
            "removed": ctype,
        }


class ItypesPage(BasePage):
    RELATIVE_URL = "/itypes"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self):
        result = sorted(map(lambda x: x.name, self.db.itypes.get_itypes()))
        return {
            "itypes": result,
        }


class ItypePage(BasePage):
    RELATIVE_URL = "/itypes/<string:itype>"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    @web_shared.req_cacher.api_mongo_cache
    def _get(self, itype):
        if not self.db.itypes.has_itype(itype):
            raise WebApiBase.GencfgNotFoundError("Itype <{}> not found in db".format(itype))
        itype = self.db.itypes.get_itype(itype)
        return {
            "name": itype.name,
            "descr": itype.descr,
        }

    def get(self, itype):
        return self._get(request, itype)


class ItypeCreatePage(BasePage):
    RELATIVE_URL = "/itypes/<string:itype>/create"

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def post(self, itype):
        request_json = copy.deepcopy(request.json)
        request_json_schema = json.loads(
            open(os.path.join(self.api.db.SCHEMES_DIR, "json", "api.create_itype.json")).read())
        jsonschema.validate(request_json, request_json_schema)

        new_itype = core.itypes.IType(self.db.itypes, {"name": str(itype), "descr": str(request_json["descr"]), "config_type": None})
        self.db.itypes.add_itype(new_itype)
        self.db.itypes.update(smart=True)

        commit_msg = "[create_itype] Created itype %s using api" % itype
        self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        return {
            "created": itype,
        }


class ItypeRemovePage(BasePage):
    RELATIVE_URL = "/itypes/<string:itype>/remove"

    def gencfg_authorize_post(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def post(self, itype):
        self.db.itypes.remove_itype(str(itype))
        self.db.itypes.update(smart=True)

        commit_msg = "[remove_itype] Removed itype %s using api" % itype
        self.api.commit(commit_msg, request, reset_changes_on_fail=True)

        return {
            "removed": itype,
        }


class TiersPage(BasePage):
    RELATIVE_URL = "/tiers"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self):
        tier_names = sorted(x.name for x in self.db.tiers.get_tiers())
        return dict(
            tiers=tier_names,
        )


class TierPage(BasePage):
    RELATIVE_URL = "/tiers/<string:tier>"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    @web_shared.req_cacher.api_mongo_cache
    def _get(self, tier):
        tier = self.db.tiers.get_tier(tier)

        result = dict(
            name=tier.name,
            description=tier.description,
            shards=[tier.get_shard_id_for_searcherlookup(x) for x in xrange(tier.get_shards_count())],
        )

        return result

    def get(self, tier):
        return self._get(request, tier)


class GroupsByTagsPage(BasePage):
    """
        GENCFG-653: return list of groups with specified itype/ctype/prj tags
    """

    RELATIVE_URL = "/groups_by_tag"

    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    @web_shared.req_cacher.api_mongo_cache
    def _get(self):
        """
            Return list of groups with specified tags. Tags specified by <query> - python boolean expression
        """

        print request.__dict__

        if "query" not in request.args:
            raise Exception, "You must specify <query> parameter"
        query = request.args["query"]

        # unfortunately we can not use compiler.ast due to using symbol <->
        # thus we have to replace this symbol
        underline = "zTzopQTF"

        def identifier_func(group_card, identifier):
            identifier = re.sub(underline, "-", identifier)
            if identifier.startswith('a_itype'):
                return identifier == 'a_itype_%s' % group_card['tags']['itype']
            elif identifier.startswith('a_ctype'):
                return identifier == 'a_ctype_%s' % group_card['tags']['ctype']
            elif identifier.startswith('a_prj'):
                return identifier in map(lambda x: 'a_prj_%s' % x, group_card['tags']['prj'])
            else:
                raise Exception, "Can not parse <%s> as itype, ctype or prj" % identifier

        result = {
            "query" : query,
            "master_groups": [],
            "slave_groups": [],
        }

        query = re.sub("-", underline, query)
        group_cards = [x.card.as_dict() for x in self.db.groups.get_groups()]
        group_cards.extend([x.guest_group_card_dict() for x in self.db.groups.get_groups() if x.has_portovm_guest_group()])
        for group_card in group_cards:
            if convert_to_ast_and_eval(query, functools.partial(identifier_func, group_card)):
                if group_card['master'] is None:
                    result["master_groups"].append(group_card['name'])
                else:
                    result["slave_groups"].append(group_card['name'])

        return result

    def get(self):
        return self._get(request)


class GroupsCards(BasePage):
    """All group cards as json (RX-27)"""

    RELATIVE_URL = "/groups_cards"

    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def get(self):
        def _replace_bytesize(obj):
            if obj.__class__ == ByteSize:
                return True, obj.value
            elif obj.__class__ == Date:
                return True, str(obj)
            elif obj.__class__ == IGroup:
                return True, obj.card.name
            else:
                return False, None

        group = self.db.groups.get_groups()
        group.sort(key=lambda x: x.card.name)

        result = []
        for group in sorted(self.db.groups.get_groups(), key=lambda x: x.card.name):
            result.append(transform_tree(group.card.as_dict(), _replace_bytesize))

        return { 'groups_cards': result }


class CpuAuditSuggestPage(BasePage):
    """Suggest for specified list of groups (RX-239)"""

    RELATIVE_URL = "/audit/cpu/suggest"

    def gencfg_authorize_get(self, login, request):
        del login, request
        return True

    def get(self):
        result = []
        for suggest in (suggest_for_group(x) for x in self.db.groups.get_groups()):
            suggest_result = dict(
                group=suggest.group.card.name,
                method=suggest.suggest_type,
                power=suggest.power,
                msg=suggest.msg,
                service_coeff=suggest.service_coeff,
                traffic_coeff=suggest.traffic_coeff,
            )
            result.append(suggest_result)

        return { 'suggest': result }


class HbfMacrosesPage(BasePage):
    """List of hbf macroses (GENCFG-2151)"""
    RELATIVE_URL = "/hbf_macroses"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self):
        macroses = self.db.hbfmacroses.get_hbf_macroses()
        macroses.sort(key=lambda x: x.hbf_project_id)
        result = [x.to_json() for x in macroses]
        return dict(hbf_macroses=result)


class SlbsPage(BasePage):
    """List of racktables slbs with ips (RX-401)"""
    RELATIVE_URL = "/slbs"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self):
        slbs = self.db.slbs.get_all()
        slbs.sort(key=lambda x: x.fqdn)
        result = [dict(fqdn=x.fqdn, ips=x.ips) for x in slbs]
        return dict(slbs=result)


class SlbPage(BasePage):
    RELATIVE_URL = "/slbs/<string:slb_fqdn>"

    def gencfg_authorize_get(self, login, request):
        del request
        return self.api.api_authorizer.can_user_view_db(login)

    def get(self, slb_fqdn):
        slb = self.db.slbs.get(slb_fqdn)
        result = dict(fqdn=slb.fqdn, ips=slb.ips)
        return dict(slb=result)


class BuildStatusPage(BasePage):
    """
        Report, whether gencfg is broken or not.
    """

    RELATIVE_URL = "/build_status"

    def get(self):
        """
            Return json with current status
        """

        # get data from mongo
        params = {
            "action": utils.mongo.update_commits_info.EActions.SHOW,
            "limit": 300,
        }
        commits = utils.mongo.update_commits_info.jsmain(params)

        # filter out commits with bad statuses
        commits = filter(lambda commit: ("testinfo" in commit) and
                                        ("status" in commit["testinfo"]) and
                                        (commit["testinfo"]["status"] in ["SUCCESS", "FAILURE"]), commits)
        if len(commits) == 0:
            raise Exception("Not found commits with statuses SUCCESS,FAILURE")

        # get current status
        last_commit = commits[0]

        # check last success commit
        success_commits = filter(lambda commit: commit["testinfo"]["status"] == "SUCCESS", commits)
        if len(success_commits) > 0:
            last_success_commit = success_commits[0]
        else:
            last_success_commit = None

        if last_commit["testinfo"]["status"] == "SUCCESS":
            build_status = True
        else:
            if last_success_commit is not None:
                if last_commit["commit"]["date"] - last_success_commit["commit"]["date"] > SETTINGS.backend.api.resources.build_status.delay:
                    build_status = False
                else:
                    build_status = True
            else:
                build_status = False

        return {
            "last_commit" : last_commit,
            "last_success_commit": last_success_commit,
            "build_status": build_status,
        }


class ApiAuthorizer(AuthorizerBase):
    """ Contains most typical operation types """

    def __init__(self, api):
        self.api = api

    def _ignore_auth_reqs(self):
        return self.api.options.testing or not self.api.options.allow_auth

    def can_user_view_db(self, login):
        del login
        return True

    def can_user_update_fastconf(self, login):
        del login
        return True

    def can_user_create_fastconf_group(self, login):
        del login
        return True

    def can_user_update_fastconf_group(self, login, action, group):
        del login, action, group
        return True

    def can_user_remove_all_fastconf_groups(self, login):
        del login
        return True

    def can_user_modify_group(self, login, group_name):
        return True  # auth is not turned on so far
        return self._ignore_auth_reqs() \
               or self.is_super_user(login) \
               or login in self.api.db.groups.get_group(group_name).get_extended_owners()


class ApiApi(WebApiBase):
    COMMIT_PREFIX = "[api] "

    def __init__(self, *args, **kwargs):
        WebApiBase.__init__(self, *args, **kwargs)
        self.api_authorizer = ApiAuthorizer(self)


def parse_cmd(jsonargs=None):
    parser = build_base_argparser("Api backend")

    if jsonargs is None:
        if len(sys.argv) == 1:
            sys.argv.append('-h')
        options = parser.parse_args()
    else:
        options = parser.parse_json(jsonargs)

    if options.production and options.testing:
        raise Exception("Uncompatible parameters: --production and --testing")

    if options.production:
        options.allow_auth = True
    elif options.testing:
        options.allow_auth = False

    return options


def prepare_api(jsonargs=None):
    options = parse_cmd(jsonargs=jsonargs)

    app = Flask(__name__)
    api = ApiApi(app, options=options)
    Compress(app)

    resources = [
        MainPage,
        SearcherlookupGroupInstancesPage,
        DbGroupInstancesPage,
        DbIntlookupInstancesPage,
        DbIntlookupInstancesByShardPage,
        DbIntlookupPage,
        DbCommitDescriptionPage,
        FastconfGroupsPage,
        FastconfLocationsPage,
        GroupsPage,
        HostsPage,
        GroupPage,
        GroupAllocPage,
        GroupRemovePage,
        GroupSlavesPage,
        GroupCardPage,
        HostsToGroupsPage,
        GuessGencfgFQDNPage,
        HostInstancesTagsPage,
        InstancePropsPage,
        ModelsPage,
        HostsHwdataPage,
        DbStatePage,

        # ctype pages
        CtypesPage,
        CtypePage,
        CtypeCreatePage,
        CtypeRemovePage,

        # itype pages
        ItypesPage,
        ItypePage,
        ItypeCreatePage,
        ItypeRemovePage,

        # tier pages
        TiersPage,
        TierPage,

        # audit pages
        CpuAuditSuggestPage,

        # HBF macroses page (GENCFG-2151)
        HbfMacrosesPage,

        # Slbs page (RX-401)
        SlbsPage,
        SlbPage,

        # misc pages
        GroupsByTagsPage,
        BuildStatusPage,
        GroupsCards,
    ]
    for _resource in resources:
        resource = _resource.init(api)
        api.add_branch_resource(resource, resource.RELATIVE_URL)

    return app, options


def main():
    app, options = prepare_api()

    # in debug mode server is automatically restarted on every change and debug info
    # host - is a host ip which we listen to
    app.run(debug=(not options.production), host=options.host, port=options.port, use_reloader=False,
            threaded=options.threaded)


if __name__ == '__main__':
    ApiApi.global_run_service(main)
