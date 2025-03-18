import time
import string
import re
import math
import xmlrpclib
import random
import os
import json
import functools
from datetime import date
import httplib
import copy

from pkg_resources import require

require('PyYAML')
import yaml

from core.db import DB, CURDB
import core.argparse.types
from gaux.aux_utils import dict_to_options, OAuthTransport
from gaux.aux_staff import get_possible_group_owners
from gaux.aux_repo import get_last_tag
from gaux.aux_multithread import run_in_multi, ERunInMulti
from config import MAIN_DIR, TAG_PATTERN, SCHEME_LEAF_DOC_FILE
from core.settings import SETTINGS
from core.instances import Instance
from core.card.types import ByteSize
from core.card.node import TMdDoc
from core.svnapi import SvnRepository, extract_watchers_from_message
from core.exceptions import TValidateCardNodeError, UtilNormalizeException
import utils.common.change_port
from collections import OrderedDict

import frontend_input as frin
import frontend_output as frout

SANDBOX_PROXY = xmlrpclib.ServerProxy(SETTINGS.services.sandbox.xmlrpc.url, allow_none=True,
                                      transport=OAuthTransport('invalid_oauth_token'))


# Decorator for adding statica variable to functino
def static_var(varname, value):
    def decorate(func):
        setattr(func, varname, value)
        return func

    return decorate


def check_bytesize(s):
    try:
        ByteSize(s)
    except:
        raise Exception("Can not convert <%s> to ByteSize type" % s)


def check_commit_message(s):
    watchers = extract_watchers_from_message(s)
    if len(watchers) == 0:
        raise Exception("You must specify at least one login@ in message")


class THostsInput(frin.TFrontStringFromPreparedInput):
    def __init__(self, name, display_name, description, start_value, hostnames, allow_empty=False):
        converter = lambda x: [] if x == "" else core.argparse.types.hosts(x)
        super(THostsInput, self).__init__(name, display_name, description, start_value, hostnames,
                                          allow_empty=allow_empty, value_converter=converter)


class TGroupInput(frin.TFrontStringInput):
    def __init__(self, name, display_name, description, start_value, allow_empty=False):
        converter = lambda x: None if x == "" else core.argparse.types.group(x)
        super(TGroupInput, self).__init__(name, display_name, description, start_value, allow_empty=allow_empty,
                                          value_converter=converter)


class TGroupsInput(frin.TFrontStringInput):
    def __init__(self, name, display_name, description, start_value, allow_empty=False):
        converter = lambda x: [] if x == "" else core.argparse.types.groups(x)
        super(TGroupsInput, self).__init__(name, display_name, description, start_value, allow_empty=allow_empty,
                                           value_converter=converter)


class TXGroupsInput(frin.TFrontStringInput):
    def __init__(self, name, display_name, description, start_value, allow_empty=False):
        converter = lambda x: [] if x == "" else core.argparse.types.xgroups(x)
        super(TXGroupsInput, self).__init__(name, display_name, description, start_value, allow_empty=allow_empty,
                                            value_converter=converter)

    def from_json(self, input_d, output_d):
        super(TXGroupsInput, self).from_json(input_d, output_d)

        if not self.allow_empty and output_d[self.name] == []:
            raise Exception("Empty xgroups list (not found group satisfying request)")


class TTimeStampInput(frin.TFrontStringInput):
    DESCRIPTION = """Timestamp in on of the following formats:
    123456789 - seconds since epoch,
    3.6h - 3.6 hour before now,
    1.7d - 1.7 day before now
"""

    def __init__(self, name, display_name, start_value, allow_empty=False):
        converter = core.argparse.types.xtimestamp
        super(TTimeStampInput, self).__init__(name, display_name, core.argparse.types.xtimestamp.description,
                                              start_value, allow_empty=allow_empty, value_converter=converter)


def standard_tag_filters(db, filters=None, fakename="groups"):
    if filters is None:
        filters = ['owner', 'ctype', 'itype', 'prj', 'metaprj']
    owner_input = frin.TFrontStringInput("%s_owned_by" % fakename, "Group owners (comma-separated)",
                                         "Group owners (no owners means omit this constraint)", "", tp="logins")
    ctype_input = frin.TFrontStringFromPreparedInput("%s_ctype" % fakename, "Group ctypes (comma-separated)",
                                                     "Group ctypes (no ctypes means omit this constraint)", "",
                                                     map(lambda x: x.name, db.ctypes.get_ctypes()))
    itype_input = frin.TFrontStringFromPreparedInput("%s_itype" % fakename, "Group itypes (comma-separated)",
                                                     "Group itypes (no itypes means omit this constraint)", "",
                                                     map(lambda x: x.name, db.itypes.get_itypes()))
    prj_input = frin.TFrontStringFromPreparedInput("%s_prj" % fakename, "Group prjs (comma-separated)",
                                                   "Group prjs (no prjs means omit this constraint)", "",
                                                   sum(map(lambda x: x.card.tags.prj, db.groups.get_groups()), []))
    metaprj_input = frin.TFrontStringFromPreparedInput("%s_metaprj" % fakename, "Group metaprjs (comma-separated)",
                                                       "Group metaprjs (no metaprjs means omit this constraint)", "",
                                                       db.constants.METAPRJS.keys())

    FLTS = [
        TAddToFilterElementInput(owner_input, lambda v, x: v == '' or len(
            set(core.argparse.types.comma_list(v)) & set(x.owners)) > 0),
        TAddToFilterElementInput(ctype_input,
                                 lambda v, x: v == '' or x.card.tags.ctype in set(core.argparse.types.comma_list(v))),
        TAddToFilterElementInput(itype_input,
                                 lambda v, x: v == '' or x.card.tags.itype in set(core.argparse.types.comma_list(v))),
        TAddToFilterElementInput(prj_input, lambda v, x: v == '' or len(
            set(x.card.tags.prj) & set(core.argparse.types.comma_list(v))) > 0),
        TAddToFilterElementInput(metaprj_input,
                                 lambda v, x: v == '' or x.card.tags.metaprj in set(core.argparse.types.comma_list(v))),
    ]

    return FLTS


def wrap_lambda(flt1, flt2):
    return lambda x: flt1(x) and flt2(x)


def hash_dict(d):
    return hash(frozenset(d.items()))


def get_broken_goals_links(sandbox_task, common_params=None):
    localproxy = xmlrpclib.ServerProxy(SETTINGS.services.sandbox.xmlrpc.url, allow_none=True,
                                       transport=OAuthTransport('invalid_oauth_token'))

    if not sandbox_task['ctx'].get('build_broken_goals'):
        return []

    resources = localproxy.list_resources({'resource_type': 'CONFIG_BUILD_LOGS', 'task_id': sandbox_task['id']})
    if len(resources) != 1:
        return []

    return map(lambda x: (x.partition('/')[2], '%s/%s.log' % (resources[0]['proxy_url'], x.partition('/')[2])),
               sandbox_task['ctx']['build_broken_goals'])


def get_sandbox_test_tasks_since(start_time):
    limit = 10
    prev_result_size = 0
    result = []
    while True:
        result = SANDBOX_PROXY.list_tasks({'task_type': 'TEST_CONFIG_GENERATOR', 'limit': limit})

        if len(result) == 0:
            break
        if len(result) == prev_result_size:
            break
        if result[-1]['timestamp'] < start_time:
            break

        limit *= 2
        prev_result_size = len(result)

    return result


@static_var("cached_start_time", None)
@static_var("cached_tasks", dict())
def get_sandbox_test_tasks(start_time):
    def is_finished(task):
        if task['status'] in ['SUCCESS', 'FAILURE', 'EXCEPTION', 'DRAFT', 'DELETED', 'UNKNOWN', 'STOPPED', 'STOPPING',
                              'DRAFT', 'TEMPORARY']:
            return True
        return False

    ctasks = get_sandbox_test_tasks.cached_tasks

    if get_sandbox_test_tasks.cached_start_time is None:
        ctasks.update(dict(map(lambda x: (x['id'], x), get_sandbox_test_tasks_since(start_time))))
        get_sandbox_test_tasks.cached_start_time = start_time
    else:
        if start_time >= get_sandbox_test_tasks.cached_start_time:
            start_time = max(map(lambda x: x['timestamp'], ctasks.itervalues()) + [start_time])
        else:
            get_sandbox_test_tasks.cached_start_time = start_time

        new_test_tasks = get_sandbox_test_tasks_since(start_time)
        for task in new_test_tasks:
            if task['id'] in ctasks and is_finished(ctasks[task['id']]):
                continue
            ctasks[task['id']] = task

        for task_id in ctasks:
            if not is_finished(ctasks[task_id]):
                ctasks[task_id] = SANDBOX_PROXY.list_tasks({'task_type': 'TEST_CONFIG_GENERATOR', 'id': task_id})[0]

    missing_broken_goals_tasks = filter(lambda x: is_finished(x) and 'broken_goals_links' not in x, ctasks.itervalues())
    for task, task_broken_goals, error in run_in_multi(get_broken_goals_links, missing_broken_goals_tasks, {},
                                                       workers=30, mode=ERunInMulti.PROCESS):
        if task_broken_goals is None:
            ctasks[task['id']]['broken_goals_links'] = []
        else:
            ctasks[task['id']]['broken_goals_links'] = task_broken_goals

    for task_id in ctasks:
        if 'broken_goals_links' not in ctasks[task_id]:
            ctasks[task_id]['broken_goals_links'] = []

    tasks = filter(lambda x: 'last_commit' in x['ctx'], ctasks.itervalues())
    tasks = dict(map(lambda x: (x['ctx']['last_commit'], x), tasks))

    return tasks


def generate_commits_table(commits):
    table_data = []

    # add commit column
    def get_commit_url(commit):
        return "https://arc.yandex-team.ru/wsvn/arc?op=comp&compare[]=%2F@{}&compare[]=%2F@{}".format(
            commit - 1, commit
        )

    signal_value = map(lambda x: {"value": x["commit"]["id"], "sortable": -x["commit"]["id"],
                                  "externalurl": get_commit_url(x["commit"]["id"])}, commits)
    table_data.append(
        frout.TTableColumn("Commit", "Commit id", "externalurl", signal_value, fltmode="fltdisabled", width="5%"))

    # add commit author
    signal_value = map(lambda x: {
        "value": x["commit"]["author"],
        "sortable": x["commit"]["author"],
        "externalurl": "%s/%s" % (SETTINGS.services.staff.http.url, x["commit"]["author"])
    }, commits)
    table_data.append(
        frout.TTableColumn("Author", "Commit author", "externalurl", signal_value, fltmode="fltregex", width="5%"))

    # add sandbox test task
    signal_value = []
    for commit in commits:
        if "testinfo" in commit:
            signal_value.append({
                "value": str(commit["testinfo"]["taskid"]),
                "externalurl": "%s%s" % (SETTINGS.services.sandbox.http.tasks.url, commit["testinfo"]["taskid"]),
            })
        else:
            signal_value.append(None)
    table_data.append(
        frout.TTableColumn("Task", "Sandbox task", "externalurl", signal_value, fltmode="fltregex", width="7%"))

    # add sandbox task status
    signal_value = []
    for commit in commits:
        if not "testinfo" in commit:
            signal_value.append("NOTSTARTED")
        elif commit["testinfo"]["status"] == "SCHEDULED":
            signal_value.append("SCHEDULED")
        elif commit["testinfo"]["status"] == "BROKEN":
            signal_value.append({"value": "BROKEN", "css": "printable__bold_text printable__red_text"})
        elif commit["testinfo"]["status"] == "SUCCESS":
            signal_value.append(
                {"value": commit["testinfo"]["status"], "css": "printable__bold_text printable__green_text"})
        elif commit["testinfo"]["status"] == "FAILURE":
            signal_value.append(
                {"value": commit["testinfo"]["status"], "css": "printable__bold_text printable__red_text"})
        else:
            raise Exception("Unknown status <%s> for commit <%s>" % (commit["testinfo"]["status"], commit["commit"]["id"]))
    table_data.append(
        frout.TTableColumn("Test status", "Test status", "str", signal_value, fltmode="fltregex", width="10%"))

    # add commit message
    signal_value = map(lambda x: {"value": x["commit"]["message"], "limit": 100, "cut": 200}, commits)
    table_data.append(
        frout.TTableColumn("Commit message", "Commit message", "textmore", signal_value, fltmode="fltregex",
                           width="20%", css="printable__whitespace_pre-wrap"))

    # add list of broken goals
    signal_value = []
    for commit in commits:
        if not "testinfo" in commit:
            signal_value.append([])
        else:
            signal_value.append(map(lambda x: {"value": x["goal"], "externalurl": x["proxy_url"]},
                                    commit["testinfo"].get("broken_goals", [])))
    table_data.append(
        frout.TListTableColumn("Broken goals", "Broken goals", "externalurl", signal_value, fltmode="fltdisabled",
                               width="10%", css="printable__whitespace_pre-wrap"))

    # add diffbuilder result
    signal_value = []
    for commit in commits:
        if not "testinfo" in commit:
            signal_value.append("")
        else:
            signal_value.append({"value": commit["testinfo"].get("diff", ""), "limit": 200, "cut": 300})
    table_data.append(
        frout.TTableColumn("Diffbuilder result", "Diffbuilder result", "textmore", signal_value, fltmode="fltregex",
                           width="40%", css="printable__whitespace_pre-wrap"))

    return frin.TTableInput("Sample Table", table_data)


class TBoolFilterElementInput(frin.TFrontCheckboxInput):
    def __init__(self, name, display_name, description, start_value, my_filter):
        super(TBoolFilterElementInput, self).__init__(name, display_name, description, start_value)
        self.my_filter = my_filter

    def from_json(self, input_d, output_d):
        if self.name not in input_d:
            if self.start_value:
                out_filter = self.my_filter
            else:
                out_filter = lambda x: True
        else:
            if input_d[self.name] == '1':
                out_filter = self.my_filter
            else:
                out_filter = lambda x: True

        if 'filter' in output_d:
            old_filter = output_d['filter']
            output_d['filter'] = wrap_lambda(out_filter, old_filter)
        else:
            output_d['filter'] = out_filter


class TAddToFilterElementInput(frin.IFrontInput):
    """
        Class-wrapper for input element, which add input element value as extra filter to <filter> option
    """

    def __init__(self, slave, my_filter):
        super(TAddToFilterElementInput, self).__init__()

        self.slave = slave
        self.my_filter = my_filter
        self.name = self.slave.name  # FIXME

    def to_json(self, frontend_json):
        return self.slave.to_json(frontend_json)

    def from_json(self, input_d, output_d):
        if self.slave.name in input_d:
            out_filter = lambda x: self.my_filter(input_d[self.name], x)
        elif self.slave.start_value is not None:
            out_filter = lambda x: self.my_filter(self.slave.start_value, x)
        else:
            out_filter = lambda x: True

        if 'filter' in output_d:
            old_filter = output_d['filter']
            output_d['filter'] = wrap_lambda(out_filter, old_filter)
        else:
            output_d['filter'] = out_filter


class TAddSlavesCheckboxInput(frin.TFrontCheckboxInput):
    def __init__(self, name, display_name, description, start_value):
        super(TAddSlavesCheckboxInput, self).__init__(name, display_name, description, start_value)

    def from_json(self, input_d, output_d):
        input_v = input_d.get(self.name, None)
        if input_v == '1' or (input_v is None and self.start_value == True):
            output_d['groups'] = sum(map(lambda x: [x] + x.slaves, output_d['groups']), [])
        else:
            output_d['groups'] = filter(lambda x: x.card.master is None, output_d['groups'])


class WrappedUtil(object):
    def __init__(self):
        pass

    def get_params_impl(self, parent, request):
        raise NotImplementedError("Function generated_params not implemented")

    def get_slave_form_params(self):
        return {}

    def get_params(self, parent, request, json_params, task_params=None):
        params_impl = self.get_params_impl(parent, request)

        found_names = set()
        for param in params_impl:
            if param.name in found_names:
                raise Exception("Parameter with name %s mentioned at least twice" % param.name)
            found_names.add(param.name)

        input_fields = frin.convert_input_to_frontent(params_impl, json_params)

        groups = map(lambda x: {'title': x.card.name, 'my': request.yauth.login in x.card.owners},
                     parent.db.groups.get_groups())

        return {
            "input_fields": input_fields,
            "slave_form_fields": self.get_slave_form_params().keys(),
            "groups": groups,
            "hasDefault": self.RUN_DEFAULT,
        }

    def check_params(self, parent, request, json_params, raw_json):
        result = self.get_params(parent, request, json_params)
        errors = self.get_error_params(parent, request, json_params)

        result["json"] = raw_json

        if len(errors.keys()):
            for d in result["input_fields"]:
                if d["name"] in errors:
                    d["errormsg"] = errors[d["name"]]
            result["error"] = True
        else:
            result["error"] = False

        return result

    def get_error_params(self, parent, request, json_params):
        converted, errors = frin.convert_input_from_frontend(self.get_params_impl(parent, request), json_params,
                                                             raise_errors=False)
        return errors

    def parent_api_commit(self, parent, request, commit):
        parent.api.commit('[utils] [%s] %s' % (self.NAME, commit), request)


class MongoGenerateReport(WrappedUtil):
    NAME = 'mongo_usage_report'
    TITLE = "Cpu Usage"
    SECTION = "Mongo"

    IS_CHANGING_DB = False
    RUN_DEFAULT = True

    def __init__(self):
        super(MongoGenerateReport, self).__init__()

    def get_params_impl(self, parent, request):
        return [
                   TXGroupsInput("groups", "Groups",
                                 "Comma-separated list of regex on group (like 'SAS.*,.*_WEB_BASE')", "ALLM"),
                   TBoolFilterElementInput("ignore_nonsearch", "Ignore non-search",
                                           "No statistics for non-search groups", True,
                                           lambda x: x.card.properties.nonsearch == False),
                   TBoolFilterElementInput("ignore_background", "Ignore background groups",
                                           "No statistics for background groups (like golovan yasmagent)", True,
                                           lambda x: x.card.properties.background_group == False),
                   TAddSlavesCheckboxInput("add_slave_groups", "Slave groups statistics",
                                           "Generate statistics for slave groups also", False),
               ] + standard_tag_filters(parent.db) + [
                   TTimeStampInput("timestamp", "Timestamp", "0h"),
               ]

    def generate_aggregate_stats(self, report_result):
        if len(report_result) == 0:
            return [{
                'type': 'str',
                'value': 'No groups available',
                'css': ' printable__bold_text',
                'children': [],
            }]

        result = {}

        for signal_name in ['instance_count', 'host_count', 'instance_power', 'host_power']:
            result[signal_name] = round(sum(map(lambda x: x[signal_name], report_result)), 2)

        baseline_host_usage = sum(map(lambda x: x['host_cpu_usage'] * x['host_power'], report_result))

        result['baseline_host_usage'] = round(baseline_host_usage / result['host_power'], 2)

        result['baseline_host_usage_dispersion'] = 0
        for elem in report_result:
            if elem['host_cpu_usage'] > result['baseline_host_usage']:
                result['baseline_host_usage_dispersion'] += (elem['host_cpu_usage'] / result[
                    'baseline_host_usage'] - 1) * elem['host_power']
        result['baseline_host_usage_dispersion'] = round(
            result['baseline_host_usage_dispersion'] / result['host_power'] * 100, 2)

        aggregate_statistics = frout.TTreeElement(frout.TLeafElement('Aggregate statistics'))
        aggregate_statistics.children.extend([
            frout.TTreeElement(frout.TLeafElement('Number of instances'), frout.TLeafElement(result['instance_count'])),
            frout.TTreeElement(frout.TLeafElement('Number of hosts'), frout.TLeafElement(result['host_count'])),
            frout.TTreeElement(frout.TLeafElement('Instances power'), frout.TLeafElement(result['instance_power'])),
            frout.TTreeElement(frout.TLeafElement('Hosts power'), frout.TLeafElement(result['host_power'])),
        ])

        calculated_statistics = frout.TTreeElement(frout.TLeafElement('Calculated statistics'))
        calculated_statistics.children.extend([
            frout.TTreeElement(frout.TLeafElement('Average host cpu usage'),
                               frout.TLeafElement(result['baseline_host_usage'])),
            frout.TTreeElement(frout.TLeafElement('Diff from ideal distribution'),
                               frout.TLeafElement(result['baseline_host_usage_dispersion'])),
        ])

        return [
            aggregate_statistics.to_json(),
            calculated_statistics.to_json(),
        ]

    # noinspection PyListCreation
    def get_result(self, parent, request, form_params):
        form_params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)

        import utils.mongo.generate_report
        generated_report = utils.mongo.generate_report.main(dict_to_options(form_params))

        result = {
            "aggregate_data": self.generate_aggregate_stats(generated_report[1].values()),
            "nodata": sorted(list(generated_report[0]))
        }

        groupnames = sorted(generated_report[1].keys())

        front_table_data = []

        front_table_data.append(
            frout.TTableColumn("Group name", "Group name", "group", groupnames, fltmode="fltregex", width="18%"))

        signal_value = map(lambda x: generated_report[1][x]['instance_cpu_usage'], groupnames)
        front_table_data.append(
            frout.TTableColumn("ICpu", "Instance cpu (in percents)", "str", signal_value, fltmode="fltrange",
                               width="8%"))

        signal_value = map(lambda x: generated_report[1][x]['host_cpu_usage'], groupnames)
        front_table_data.append(
            frout.TTableColumn("HCpu", "Host cpu (in percents)", "str", signal_value, fltmode="fltrange", width="8%"))

        signal_value = map(lambda x: generated_report[1][x]['host_cpu_usage_rough'], groupnames)
        front_table_data.append(
            frout.TTableColumn("HCpuA", "Host cpu (in percents, if used all assigned hosts)", "str", signal_value,
                               fltmode="fltrange", width="8%"))

        signal_value = map(lambda x: generated_report[1][x]['instance_mem_usage'], groupnames)
        front_table_data.append(
            frout.TTableColumn("IMem", "Memory, used by instance (in gigabytes)", "str", signal_value,
                               fltmode="fltrange", width="8%"))

        signal_value = map(lambda x: generated_report[1][x]['host_mem_usage'], groupnames)
        front_table_data.append(
            frout.TTableColumn("HMem", "Memory, used by all instances and other stuff (in gigabytes)", "str",
                               signal_value, fltmode="fltrange", width="8%"))

        signal_value = map(lambda x: generated_report[1][x]['instance_power'], groupnames)
        front_table_data.append(
            frout.TTableColumn("IPower", "Instance power (in gencfg power units)", "str", signal_value,
                               fltmode="fltrange", width="10%"))

        signal_value = map(lambda x: generated_report[1][x]['host_power'], groupnames)
        front_table_data.append(
            frout.TTableColumn("HPower", "Host power (in gencfg power units)", "str", signal_value, fltmode="fltrange",
                               width="10%"))

        signal_value = map(lambda x: generated_report[1][x]['instance_count'], groupnames)
        front_table_data.append(
            frout.TTableColumn("ICount", "Instance count ", "str", signal_value, fltmode="fltrange", width="10%"))

        signal_value = map(lambda x: generated_report[1][x]['host_count'], groupnames)
        front_table_data.append(
            frout.TTableColumn("HCount", "Host count", "str", signal_value, fltmode="fltrange", width="10%"))

        signal_value = map(lambda x: generated_report[1][x]['ts'], groupnames)
        front_table_data.append(
            frout.TTableColumn("Timestamp", "Timestamp of last statistics update", "str", signal_value,
                               converter=lambda x: time.strftime("%Y-%m-%d %H:%M", time.localtime(x)), width="10%"))

        result['havedata'] = frout.generate_table(front_table_data)

        return result


class MongoShowUnused(WrappedUtil):
    NAME = "mongo_show_unused"
    SECTION = "Mongo"
    TITLE = "Unused Hosts"

    IS_CHANGING_DB = False
    RUN_DEFAULT = True

    def __init__(self):
        super(MongoShowUnused, self).__init__()

    def get_params_impl(self, parent, request):
        return [
                   TXGroupsInput("groups", "Groups",
                                 "Comma-separated list of regex on group (like 'SAS.*,.*_WEB_BASE')", "ALLM",
                                 allow_empty=False),
                   frin.TFrontNumberInput("unused_time", "Unused last N days",
                                          "Show only hosts which were unused last N days", 14),
                   TBoolFilterElementInput("ignore_nonsearch", "Ignore non-search",
                                           "No statistics for non-search groups", True,
                                           lambda x: x.card.properties.nonsearch == False),
                   TBoolFilterElementInput("ignore_reserved", "Ignore reserved", "Ignore reserved groups", False,
                                           lambda x: x.card.tags.metaprj != "reserve"),
                   TBoolFilterElementInput("ignore_temporary", "Ignore temporary", "Ignore temporary groups", False,
                                           lambda x: x.card.properties.expires is None),
                   TBoolFilterElementInput("ignore_slaves", "Ignore slave groups", "Igrone slave groups", True,
                                           lambda x: x.card.master is None),
               ] + standard_tag_filters(parent.db)

    def generate_aggregate_tree(self, result):
        notfound_element = frout.TTreeElement(frout.TLeafElement("Not found statistics"))
        notfound_element_list = frout.TTreeElement(
            frout.TLeafElement("Not found list (%s total)" % len(result['nodata'])))
        notfound_element_list.value = frout.TLeafElement(
            map(lambda x: frout.TLeafElement(x.name, tp="host"), result['nodata'][:100]), tp="list")
        notfound_element.children.append(notfound_element_list)

        unused_hosts = list(set(sum(result['havedata'].values(), [])))
        unused_hosts = map(lambda (x, y): x, unused_hosts)

        aggregate_element = frout.TTreeElement(frout.TLeafElement("Aggregate unused statistics"))
        aggregate_element.children.append(
            frout.TTreeElement(frout.TLeafElement("Number of unused"), frout.TLeafElement(len(unused_hosts))))
        aggregate_element.children.append(frout.TTreeElement(frout.TLeafElement("Power of unused"),
                                                             frout.TLeafElement(
                                                                 sum(map(lambda x: x.power, unused_hosts)))))
        aggregate_element.children.append(frout.TTreeElement(frout.TLeafElement("Memory of unused"),
                                                             frout.TLeafElement(
                                                                 sum(map(lambda x: x.memory, unused_hosts)))))

        return [
            notfound_element.to_json(),
            aggregate_element.to_json(),
        ]

    def generate_table(self, result):
        result = result['havedata']
        result = dict(filter(lambda (x, y): len(y) > 0, result.iteritems()))
        for group in result:
            result[group] = map(lambda (x, y): x, result[group])

        groups = sorted(result.keys())

        front_table_data = []

        signal_value = map(lambda x: x.card.name, groups)
        front_table_data.append(
            frout.TTableColumn("Group name", "Group name", "group", signal_value, fltmode="fltregex", width="20%"))

        signal_value = map(lambda x: ' '.join(x.card.owners), groups)
        front_table_data.append(
            frout.TTableColumn("Admins", "List of admins", "str", signal_value, fltmode="fltregex", width="15%"))

        signal_value = map(lambda x: len(result[x]), groups)
        front_table_data.append(
            frout.TTableColumn("Unused", "Number of unused machines", "str", signal_value, fltmode="fltrange",
                               width="5%"))

        signal_value = map(lambda x: len(x.getHosts()), groups)
        front_table_data.append(
            frout.TTableColumn("Total", "Number of total machines", "str", signal_value, fltmode="fltrange",
                               width="5%"))

        signal_value = map(lambda x: map(lambda y: y.name, result[x][:100]), groups)
        front_table_data.append(
            frout.TListTableColumn("Unused hosts", "Example unused hosts", "host", signal_value, width="50%"))

        return frout.generate_table(front_table_data)

    def get_result(self, parent, request, form_params):
        form_params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)

        form_params['hosts'] = None  # FIXME

        from utils.mongo import show_unused
        unused = show_unused.main(dict_to_options(form_params), parent.db)

        result = {
            'tree': self.generate_aggregate_tree(unused),
            'table': self.generate_table(unused),
        }

        return result


class AddMachines(WrappedUtil):
    NAME = "add_machines"
    SECTION = "Other"
    TITLE = "Add Machines"

    IS_CHANGING_DB = True
    RUN_DEFAULT = False

    def __init__(self):
        super(AddMachines, self).__init__()

    def get_params_impl(self, parent, request):
        return [
            frin.TFrontStringInput("hosts", "Hosts to add",
                                   "Comma-separated list of hosts (in fqdn form with correct domain name!!!)", ""),
            TGroupInput("reserved_group", "Assigned group", "Assign added hosts to specified group", "",
                        allow_empty=True),
            frin.TDescrInput("apply_options", "Apply options"),
            frin.TFrontCheckboxInput("apply", "Apply changes", "Commit added hosts to db", False),
            frin.TFrontStringInput("commit_message", "Commit message", "Detailed commit message", "", tp="text"),
        ]

    # noinspection PyListCreation
    def get_result(self, parent, request, form_params):
        params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)
        params['action'] = 'add'
        params['verbose'] = False
        params['apply'] = False

        from utils.pregen import update_hosts
        suboptions = update_hosts.get_parser().parse_json(params)
        subresult = update_hosts.main(suboptions, from_cmd=False)

        table_data = []
        table_data.append(
            frout.TTableColumn("Hostname", "Hostname", "str", map(lambda x: x['hostname'], subresult['fail_hosts'])))
        table_data.append(
            frout.TTableColumn("Status", "Status", "str", map(lambda x: x['status'], subresult['fail_hosts'])))

        subresult['fail_hosts'] = frout.generate_table(table_data)

        return subresult


class MongoHaveDataStats(WrappedUtil):
    NAME = "mongo_have_stats"
    SECTION = "Mongo"
    TITLE = "Have Stats"

    IS_CHANGING_DB = False
    RUN_DEFAULT = True

    def __init__(self):
        super(WrappedUtil, self).__init__()

    def get_params_impl(self, parent, request):
        groups_input = frin.TFrontStringInput("group_regex", "Regex on group name",
                                              "Show only groups satisfying python regex from module 're'", "")

        return [
            TXGroupsInput("groups", "Groups", "Comma-separated list of regex on group (like 'SAS.*,.*_WEB_BASE')",
                          "ALLM"),
            TAddToFilterElementInput(groups_input, lambda v, x: v == '' or re.match(v, x.name)),
            TBoolFilterElementInput("ignore_nonsearch", "Ignore non-search", "Ignore non-search groups", True,
                                    lambda x: x.card.properties.nonsearch == False),
            TBoolFilterElementInput("ignore_unraisable", "Ignore non-raisable", "Ignore non-raisable groups", True,
                                    lambda x: x.card.properties.unraisable_group == False),
        ]

    def get_result(self, parent, request, form_params):
        params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)
        params["timestamp"] = int(time.time())
        params["sum_stats"] = False
        params["stats_type"] = "current"

        from utils.mongo import show_admin_stats
        subresult = show_admin_stats.main(dict_to_options(params))

        subresult = dict(map(lambda x: (x, subresult[x].values()[0]), subresult))

        groups = sorted(subresult.keys())

        table_data = []

        signal_value = groups
        table_data.append(
            frout.TTableColumn("Group name", "Group name", "group", signal_value, fltmode="fltregex", width="40%"))

        signal_value = map(lambda x: subresult[x]["host_count_mongo"], groups)
        table_data.append(
            frout.TTableColumn("HaveH", "Number of hosts with statistics", "str", signal_value, fltmode="fltrange",
                               width="10%"))

        signal_value = map(lambda x: subresult[x]["host_count"], groups)
        table_data.append(
            frout.TTableColumn("TotalH", "Total number of group hosts", "str", signal_value, fltmode="fltrange",
                               width="10%"))

        signal_value = map(lambda x: 100 * subresult[x]["host_count_mongo"] / float(max(1, subresult[x]["host_count"])),
                           groups)
        table_data.append(frout.TTableColumn("HostP", "Percents of available hosts", "float", signal_value,
                                             converter=lambda x: "{0:.2f}".format(x),
                                             fltmode="fltrange", width="10%"))

        signal_value = map(lambda x: subresult[x]["instance_count_mongo"], groups)
        table_data.append(
            frout.TTableColumn("HaveI", "Number of instances with statistics", "str", signal_value, fltmode="fltrange",
                               width="10%"))

        signal_value = map(lambda x: subresult[x]["instance_count"], groups)
        table_data.append(
            frout.TTableColumn("TotalI", "Total number of group instances", "str", signal_value, fltmode="fltrange",
                               width="10%"))

        signal_value = map(
            lambda x: 100 * subresult[x]["instance_count_mongo"] / float(max(1, subresult[x]["instance_count"])),
            groups)
        table_data.append(frout.TTableColumn("InstancesP", "Percents of available instances", "float", signal_value,
                                             converter=lambda x: "{0:.2f}".format(x),
                                             fltmode="fltrange", width="10%"))

        table_result = frout.generate_table(table_data)

        if form_params.get('group_regex', '') != '':
            histurl = '%s/mongodb/stats?group_regex=%s' % (parent.api.gen_base_url(), form_params['group_regex'])
        else:
            histurl = '%s/mongodb/stats?' % (parent.api.gen_base_url())

        result = {
            'histurl': {
                'title': "History graph",
                'href': histurl,
            },
            'table': table_result
        }

        return result


class TConcatRequirementsInput(frin.TFrontChoiceInput):
    def __init__(self, name, display_name, description, start_value, items):
        super(TConcatRequirementsInput, self).__init__(name, display_name, description, start_value, items)

    def from_json(self, input_d, output_d):
        if self.name in input_d:
            value = input_d[self.name]
        else:
            value = self.start_value

        if 'comparator' not in output_d:
            output_d['comparator'] = value
        else:
            output_d['comparator'] += ',' + value


class ReplaceHosts(WrappedUtil):
    NAME = "replace_hosts"
    SECTION = "Utils"
    TITLE = "Replace Hosts"

    IS_CHANGING_DB = True
    RUN_DEFAULT = False

    def get_params_impl(self, parent, request):
        UNWORKING_GROUP_DESCR = """Choose unworking group accroding to how machine is broken:
<ul>
    <li>- not responding -> ALL_UNWORKING</li>
    <li>- flaps (hang over or reboots to often) -> ALL_UNWORKING_FLAPS</li>
    <li>- overheat -> ALL_UNWORKING_OVERHEAT</li>
    <li>- lowfreq (no overheat detected) -> ALL_UNWORKING_LOWFREQ</li>
    <li>- no fastbone -> ALL_UNWORKING_NOFASTBONE</li>
</ul>
"""

        HOST_SELECTION_FILTER = """Added host_selection filter, which filters out suitable hosts. Examples.
<ul>
   <li>- filter only hosts with SSD: <b>lambda x: x.ssd &gt; 0</b> </li>
   <li>- filter only new modern hsots hosts with a lot or memory: <b>lambda x: x.memory &gt; 200 and x.model in ['E5-2660', 'E5-2650v2']</b></li>
   <li>- filter only hosts in certain dc: <b>lambda x: x.dc in ['iva', 'ugrb']</b></li>
</ul>
"""

        return [
            frin.TDescrInput("group_options", "Group params"),
            THostsInput("hosts", "Hosts", "Comma-separated list of hosts to replace (preferably in fqdn format)", "",
                        parent.db.hosts.get_host_names()),
            TGroupsInput("src_groups", "Reserved group", "Comma-separated list of reserved groups",
                         "MSK_RESERVED,SAS_RESERVED,MAN_RESERVED"),
            TGroupInput("dest_group", "Unworking group", UNWORKING_GROUP_DESCR, "ALL_UNWORKING"),

            frin.TDescrInput("replaced_hosts_constraints", "Constraints on replaced machines"),
            TConcatRequirementsInput("mem_affinity", "Memory affinity", "Choose replacement hosts memory requirements",
                                     "mem=", [
                                         ("Same memory", "mem="), ("Same or more memory", "mem+"), ("Disabled", "mem-")
                                     ]),
            TConcatRequirementsInput("power_affinity", "Power affinity", "Choose replacement hosts power requirements",
                                     "power=", [
                                         ("Same power", "power="), ("Same or more power", "power+"),
                                         ("Disabled", "power-")
                                     ]),
            TConcatRequirementsInput("disk_affinity", "Disk affinity",
                                     "Choose replacement hosts disk size requirements", "disk=", [
                                         ("Same disk", "disk="), ("Same or more disk", "disk+"), ("Disabled", "disk-")
                                     ]),
            TConcatRequirementsInput("ssd_affinity", "Ssd affinity", "Choose replacement hosts ssd size requirements",
                                     "ssd=", [
                                         ("Same ssd", "ssd="), ("Same or more ssd", "ssd+"), ("Disabled", "ssd-")
                                     ]),
            TConcatRequirementsInput("loaction_affinity", "Location affinity", "Choose replacement hosts location",
                                     "queue=", [
                                         ("Same queue", "queue=,dc=,location="), ("Same dc", "queue-,dc=,location="),
                                         ("Same location", "queue-,dc-,location="), ("Disabled", "queue-,dc-,location-")
                                     ]),

            frin.TDescrInput("extended_constaints", "More complex constraints on hosts"),
            frin.TFrontStringInput("filter", "Host selection filter", HOST_SELECTION_FILTER, "lambda x: True",
                                   allow_empty=False),

            frin.TDescrInput("apply_options", "Apply options"),
            frin.TFrontCheckboxInput("apply", "Apply changes", "Apply and commit changes", False),
            frin.TFrontStringInput("commit_message", "Commit message", "Detailed commit message", "", tp="text",
                                   allow_empty=True),
        ]

    def _mk_diff_element(self, descr, old_value, new_value):
        diff_value = "%+.2f" % (new_value - old_value)
        if new_value >= old_value:
            return frout.TTreeElement(frout.TLeafElement(descr),
                                      frout.TLeafElement(diff_value, css="printable__green_text"))
        else:
            return frout.TTreeElement(frout.TLeafElement(descr),
                                      frout.TLeafElement(diff_value, css="printable__red_text"))

    def convert_host_result(self, host, replace_data):
        main_element = frout.TTreeElement(frout.TLeafElement(host.name, tp='host'))

        replace_host_found, replace_host = replace_data['replace_host']
        if not replace_host_found:
            replace_host, failed_checkers = replace_host

            main_element.children.append(frout.TExtenededLeafElement(
                "Status",
                {"value": "NOT FOUND", "css": "printable__bold_text printable__red_text"}
            ))
            main_element.children.append(frout.TExtenededLeafElement(
                "Candidate",
                {"value": replace_host.name, "type": "host"}
            ))
            main_element.children.append(frout.TExtenededLeafElement(
                "Modify checkers",
                ", ".join(map(lambda x: "%s" % x, failed_checkers))
            ))

        else:
            main_children = main_element.children

            main_children.append(frout.TTreeElement(frout.TLeafElement("Status"),
                                                    frout.TLeafElement("FOUND",
                                                                       css="printable__bold_text printable__green_text")))

            main_children.append(
                frout.TTreeElement(frout.TLeafElement("Replace Host"), frout.TLeafElement(replace_host.name, 'host')))
            main_children[-1].children.append(self._mk_diff_element("Memory", host.memory, replace_host.memory))
            main_children[-1].children.append(self._mk_diff_element("Power", host.power, replace_host.power))
            main_children[-1].children.append(self._mk_diff_element("Disk", host.disk, replace_host.disk))
            main_children[-1].children.append(self._mk_diff_element("SSD", host.ssd, replace_host.ssd))

            main_children.append(frout.TTreeElement(frout.TLeafElement('Replaced Instances')))
            for old_instance, new_instance in replace_data['replace_instances']:
                main_children[-1].children.append(frout.TExtenededLeafElement(
                    {"value": old_instance.full_shortcut_name(), "css": "printable__overflow_hidden"},
                    {"value": new_instance.full_shortcut_name(), "css": "printable__overflow_hidden"},
                ))

            main_children.append(frout.TTreeElement(frout.TLeafElement('Affected Groups')))
            for group in replace_data['affected_groups']:
                main_children[-1].children.append(frout.TTreeElement(frout.TLeafElement(group.card.name, tp='group')))

        return main_element

    def get_updated_input_fields(self, params, result):
        update_input_fields = {}

        not_replaced = []
        replaced = []
        for host, replace_data in result.iteritems():
            replace_host_found, replace_host = replace_data['replace_host']
            if not replace_host_found:
                not_replaced.append(host.name)
            else:
                replaced.append("    %s -> %s" % (host.name, replace_host.name))

        if params['apply']:
            update_input_fields['hosts'] = ','.join(not_replaced)
            update_input_fields['commit_message'] = ''
        else:
            if len(replaced) > 0:
                update_input_fields['commit_message'] = 'Replaced hosts:\n%s' % ('\n'.join(replaced))

        return update_input_fields

    def get_result(self, parent, request, form_params):
        params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)
        params['filter'] = core.argparse.types.pythonlambda(params['filter'])
        params['comparator'] = params['comparator']
        params['verbose'] = False
        params['skip_missing'] = True
        params['modify'] = False

        from utils.common import replace_host2
        result = replace_host2.jsmain(params)

        if params['apply']:
            self.parent_api_commit(parent, request, params['commit_message'])

        # create output tree
        host_elements = []
        for host, replace_data in result.iteritems():
            host_elements.append(self.convert_host_result(host, replace_data))
        if params['apply']:
            commited_element = frout.TTreeElement(frout.TLeafElement("Commit status"),
                                                  frout.TLeafElement("COMMITED",
                                                                     css="printable__bold_text printable__green_text"))
        else:
            commited_element = frout.TTreeElement(frout.TLeafElement("Commit status"),
                                                  frout.TLeafElement("NOT COMMITED",
                                                                     css="printable__bold_text printable__red_text"))
        commited_element.children.extend(host_elements)

        return {
            'tree': [commited_element.to_json()],
            'input_fields': self.get_updated_input_fields(params, result),
        }


class CheckAliveHosts(WrappedUtil):
    NAME = "check_alive_hosts"
    SECTION = "Check Alive Hosts"
    TITLE = "Check Alive Hosts"

    IS_CHANGING_DB = True
    RUN_DEFAULT = False

    def get_params_impl(self, parent, request):
        return [
            THostsInput("hosts", "Hosts to check",
                        "Comma-separated list of hosts to replace (preferably in fqdn format)", "",
                        parent.db.hosts.get_host_names(), allow_empty=True),
            TGroupsInput("groups", "Groups to check", "Comma-separated list of reserved groups", "", allow_empty=True),
            frin.TFrontChoiceInput("checker", "Check mode",
                                   "Choose check mode: sshport - check if port 22 is open, lowfreq - check if cpu frequency is low, fastbone - check if fastbone works, ...",
                                   "sshport",
                                   [("sshport", "sshport"), ("lowfreq", "lowfreq"), ("fastbone", "fastbone")],
                                   ),
        ]

    def convert_hostlist(self, descr, hostlist):
        MAXLEN = 100
        values = map(lambda x: frout.TTreeElement(frout.TLeafElement(x.name, tp='host')), hostlist[:MAXLEN])
        if len(hostlist) > MAXLEN:
            values.append(frout.TTreeElement(frout.TLeafElement('...', tp='str')))

        result = frout.TTreeElement(frout.TLeafElement(descr), frout.TLeafElement(values, tp='list'))

        return result

    def get_result(self, parent, request, form_params):
        from gaux.aux_multithread import ERunInMulti
        from utils.pregen import check_alive_machines

        params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)
        params['flt'] = None
        params['unworking_group'] = None
        params['mode'] = ERunInMulti.PROCESS
        params['workers'] = 100
        params['verbose'] = False
        params['add_working_to_reserved'] = False

        result = check_alive_machines.main(dict_to_options(params))

        main_element = frout.TTreeElement(frout.TLeafElement("Main Tree"))
        main_element.children.extend([
            self.convert_hostlist("Success hosts", result['succ_hosts']),
            self.convert_hostlist("Failed hosts", result['failed_hosts']),
            self.convert_hostlist("Hosts without result", result['error_hosts']),
        ])

        return {
            'tree': [
                main_element.to_json(),
            ],
        }


class CreateNewTag(WrappedUtil):
    NAME = "create_new_tag"
    SECTION = "Svn"
    TITLE = "Create Tag"

    IS_CHANGING_DB = True
    RUN_DEFAULT = False

    def _get_running_tasks_table(self):
        running_tasks = []
        for status in ['ENQUEUING', 'ENQUEUED', 'PREPARING', 'EXECUTING', 'WAIT_TASK']:
            running_tasks.extend(SANDBOX_PROXY.list_tasks({'task_type': 'RELEASE_CONFIG_GENERATOR', 'status': status}))

        if len(running_tasks) > 0:
            table_data = []

            signal_value = map(lambda x: {"value": str(x["id"]), "externalurl": x["url"]}, running_tasks)
            table_data.append(
                frout.TTableColumn("Task id", "Task id", "externalurl", signal_value, fltmode="fltregex", width="10%"))

            signal_value = map(lambda x: x["status"], running_tasks)
            table_data.append(
                frout.TTableColumn("Task status", "Task status", "str", signal_value, fltmode="fltregex", width="10%"))

            signal_value = map(lambda x: time.strftime("%Y-%m-%d %H:%M", time.localtime(x["timestamp"])), running_tasks)
            table_data.append(
                frout.TTableColumn("Created at", "Created at", "str", signal_value, fltmode="fltregex", width="10%"))

            signal_value = map(lambda x: x["descr"], running_tasks)
            table_data.append(
                frout.TTableColumn("Task descr", "Task descr", "str", signal_value, fltmode="fltregex", width="50%"))

            return frin.TTableInput("Running Tasks Table", table_data)
        else:
            return None

    def _get_untagged_commits_table(self, last_tag):
        repo = SvnRepository(MAIN_DIR)

        last_tag_commit = repo.get_last_commit_id(url="svn+ssh://arcadia.yandex.ru/arc/tags/gencfg/%s" % (last_tag))

        # get list of commits
        import utils.mongo.update_commits_info as update_commits_info
        subutil_params = {
            "action": update_commits_info.EActions.SHOW,
            "update_db": True,
            "start_commit": last_tag_commit + 1,
        }
        commits = update_commits_info.jsmain(subutil_params)

        return generate_commits_table(commits)

    def get_params_impl(self, parent, request):
        extra_params = []

        info_tree = []
        # add information on last generated tag
        last_tag_info = []

        last_tag = get_last_tag(TAG_PATTERN, SvnRepository(MAIN_DIR))
        last_tag_info.append(frout.TTreeElement(frout.TLeafElement("Name"), frout.TLeafElement(last_tag)))

        last_tag_task = filter(lambda x: x['ctx'].get('tag', None) == last_tag,
                               SANDBOX_PROXY.list_tasks({'task_type': 'BUILD_CONFIG_GENERATOR', 'limit': 10}))
        if len(last_tag_task) == 0:
            last_tag_info.append(
                frout.TExtenededLeafElement("Sandbox Task",
                                            {"value": "NOT FOUND", "css": " printable__bold_text printable__red_text"}),
            )
        else:
            last_tag_info.append(
                frout.TTreeElement(
                    frout.TLeafElement("Sandbox Task"),
                    frout.TLeafElement({"value": last_tag_task[0]["id"], "externalurl": last_tag_task[0]["url"]},
                                       tp="externalurl"),
                )
            )
            last_tag_info.append(
                frout.TExtenededLeafElement("Tag Release Status", last_tag_task[0]["status"]),
            )

        last_tag_entry = frout.TTreeElement(frout.TLeafElement('Last tag', css=' printable__bold_text'),
                                            children=last_tag_info)
        info_tree.append(last_tag_entry)

        # add information on running tasks
        running_tasks_table = self._get_running_tasks_table()
        if running_tasks_table is not None:
            info_tree.append(
                frout.TTreeElement(frout.TLeafElement("Currently running tasks"), children=[running_tasks_table]))

        # add information on untagged commits
        untagged_commits_table = self._get_untagged_commits_table(last_tag)
        untagged_commits_count = len(untagged_commits_table.table_columns[0].cells_data)
        info_tree.append(frout.TTreeElement(frout.TLeafElement("Untagged commits (%d total)" % untagged_commits_count),
                                            children=[untagged_commits_table]))

        extra_params.append(frin.TPrintableInput('info_tree', info_tree))

        return extra_params

    def get_result(self, parent, request, form_params):
        class TimeoutTransport(OAuthTransport):
            """
                Auxiliarily class, that sets timeout on outgoing xml request
            """

            def __init__(self, *args, **kwargs):
                OAuthTransport.__init__(self, *args, **kwargs)

            def make_connection(self, host):
                h = httplib.HTTPConnection(host, timeout=30)
                return h

        proxy = xmlrpclib.ServerProxy('https://sandbox.yandex-team.ru/sandbox/xmlrpc', allow_none=True,
                                      transport=TimeoutTransport('invalid_oauth_token'))

        params = {
            "type_name": "RELEASE_CONFIG_GENERATOR",
            "owner": "GENCFG",
            "descr": "Create new tag from gencfg frontend by %s" % request.yauth.login,
            "priority": ("SERVICE", "HIGH"),
            "arch": "linux_ubuntu_12.04_precise",  # command "getent group yandex_mnt" works only on precise
            "ctx": {
                "notify_if_finished": request.yauth.login,
                "notify_if_failed": request.yauth.login,
            },
        }

        try:
            task_id = proxy.createTask(params)
            return {
                'task_url': 'http://sandbox.yandex-team.ru/sandbox/tasks/view?task_id=%s' % task_id,
                'task_id': task_id,
            }
        except Exception, e:
            return {
                'error': str(e),
            }


def get_input_hosts_table(db, groupnames, checkbox_name):
    groups = map(lambda x: db.groups.get_group(x), groupnames)
    hosts = sum(map(lambda x: x.getHosts(), groups), [])

    columns_data = [
        frout.TTableColumn("Hname", "Host name", "host", map(lambda x: x.name, hosts),
                           flt_form_name="table_filter_hname", fltmode="fltregex", width="15%"),
        frout.TTableColumn("Model", "Host cpu model", "str", map(lambda x: x.model, hosts),
                           flt_form_name="table_filter_model", fltmode="fltregex", width="10%"),
        frout.TTableColumn("Memory", "Host memory", "float", map(lambda x: x.memory, hosts),
                           flt_form_name="table_filter_memory", fltmode="fltrange", width="10%"),
        frout.TTableColumn("Disk", "Host disk size", "float", map(lambda x: x.disk, hosts),
                           flt_form_name="table_filter_disk", fltmode="fltrange", width="10%"),
        frout.TTableColumn("NDisks", "Host disk count", "float", map(lambda x: x.n_disks, hosts),
                           flt_form_name="table_filter_ndisks", fltmode="fltrange", width="10%"),
        frout.TTableColumn("SSD", "Host ssd size", "float", map(lambda x: x.ssd, hosts),
                           flt_form_name="table_filter_ssd", fltmode="fltrange", width="10%"),
        frout.TTableColumn("Netcard", "Host netcard (one of)", "str", map(lambda x: x.netcard, hosts),
                           flt_form_name="table_filter_netcard",
                           fltmode="fltregex", width="20%"),
        frout.TTableColumn("Location", "Host location (dc, queue, switch)", "str",
                           map(lambda x: "%s %s %s" % (x.location, x.queue, x.switch), hosts),
                           flt_form_name="table_filter_location", fltmode="fltregex", width="20%"),
        frout.TCheckboxTableColumn("Choosen", "Choosen", "checkbox",
                                   map(lambda x: {"name": checkbox_name, "value": x.name, "checked": False}, hosts),
                                   flt_form_name="table_filter_checkbox", fltmode="fltchecked", width="5%"),
    ]

    return columns_data


class ShowInstanceState(WrappedUtil):
    NAME = "show_instance_state"
    SECTION = "Mongo"
    TITLE = "Have Stats2"

    IS_CHANGING_DB = False
    RUN_DEFAULT = True

    def __init__(self):
        super(ShowInstanceState, self).__init__()

    def get_params_impl(self, parent, request):
        return [
                   TXGroupsInput("groups", "Groups",
                                 "Comma-separated list of regex on group (like 'SAS.*,.*_WEB_BASE')", "MAN_.*",
                                 allow_empty=False),
               ] + standard_tag_filters(parent.db) + [
                   TTimeStampInput("timestamp", "Start time", "3h", allow_empty=False),
               ]

    # noinspection PyListCreation
    def get_result(self, parent, request, form_params):
        form_params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)

        form_params["action"] = "show_brief"

        from utils.mongo import show_hosts_from_instancestate
        suboptions = show_hosts_from_instancestate.get_parser().parse_json(form_params)
        subresult = show_hosts_from_instancestate.main(suboptions)

        main_table_data = []
        main_table_data.append(
            frout.TTableColumn("Group name", "Group name", "group", [], fltmode="fltregex", width="20%"))
        main_table_data.append(
            frout.TTableColumn("Owners", "List of group owners", "str", [], fltmode="fltregex", width="15%"))
        main_table_data.append(
            frout.TTableColumn("Total", "Total number of group instances", "float", [], fltmode="fltrange", width="5%"))
        main_table_data.append(
            frout.TTableColumn("Down", "Number of instances, unavailable in isntancestateV3", "float", [],
                               fltmode="fltrange", width="5%"))
        main_table_data.append(
            frout.TTableColumn("Perc", "Percents of unavailable instances", "float", [], fltmode="fltrange",
                               width="5%"))
        main_table_data.append(
            frout.TTableColumn("Examples", "Example instances (from instancestateV3)", "str", [], fltmode="fltregex",
                               width="50%"))

        for group, total_instances, down_instances, example_instances in subresult:
            main_table_data[0].cells_data.append(group.card.name)
            main_table_data[1].cells_data.append(" ".join(group.card.owners))
            main_table_data[2].cells_data.append(total_instances)
            main_table_data[3].cells_data.append(down_instances)
            main_table_data[4].cells_data.append(round(float(down_instances) / max(total_instances, 1) * 100, 2))
            main_table_data[5].cells_data.append(" ".join(example_instances[:20]))

        return frout.generate_table(main_table_data)


class MongoShowOverused(WrappedUtil):
    NAME = "mongo_show_overused"
    SECTION = "Mongo"
    TITLE = "Overused Hosts"
    IS_CHANGING_DB = False
    RUN_DEFAULT = True

    def __init__(self):
        super(MongoShowOverused, self).__init__()

    def get_params_impl(self, parent, request):
        return [
            TXGroupsInput("groups", "Groups", "Comma-separated list of regex on group (like 'SAS.*,.*_WEB_BASE')",
                          "ALLM", allow_empty=False),
            frin.TFrontNumberInput("minimal_usage", "Minimal cpu usage",
                                   "Show only hosts which usage more than specified (in percents)", 60),
            TBoolFilterElementInput("ignore_nonsearch", "Ignore non-search", "No statistics for non-search groups",
                                    True, lambda x: x.card.properties.nonsearch == False),
            TBoolFilterElementInput("ignore_fake", "Ignore fake", "Ignore fake groups", True,
                                    lambda x: x.card.properties.fake_group == False),
        ]

    def get_result(self, parent, request, form_params):
        form_params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)

        filtered_groups = filter(form_params['filter'], form_params['groups'])
        filtered_groups = filter(
            lambda x: x.card.properties.fake_group == False and x.card.properties.background_group == False,
            filtered_groups)

        all_hosts = list(set(sum(map(lambda x: x.getHosts(), filtered_groups), [])))
        filtered_hosts = filter(lambda x: not x.is_vm_guest(), all_hosts)
        all_instances = map(lambda host: Instance(host, 1.0, 65535, 'UNKNOWN', 0), filtered_hosts)

        sub_options = {
            'groups': None,
            'intlookups': None,
            'hosts': None,
            'instances': all_instances,
            'stats_type': 'current',
            'timestamp': int(time.time()),
        }

        from utils.mongo import show_usage
        sub_options = show_usage.get_parser().parse_json(sub_options)
        sub_result = show_usage.main(sub_options)

        overused_instances = filter(
            lambda x: sub_result[x] is not None and sub_result[x]['instance_cpu_usage'] >= form_params[
                                                                                               'minimal_usage'] / 100.,
            sub_result.iterkeys())
        overused_hosts = set(map(lambda x: x.host, overused_instances))

        overused_hosts_by_group = dict()
        for group in filtered_groups:
            group_overused_hosts = set(group.getHosts()) & overused_hosts
            if len(group_overused_hosts) > 0:
                overused_hosts_by_group[group] = list(group_overused_hosts)

        result = {
            'no_overused': map(lambda x: x.card.name, set(filtered_groups) - set(overused_hosts_by_group.keys()))
        }

        overused_groups = overused_hosts_by_group.keys()

        overused_table_data = []

        signal_value = map(lambda x: x.card.name, overused_groups)
        overused_table_data.append(
            frout.TTableColumn("Group name", "Group name", "group", signal_value, fltmode="fltregex", width="18%"))

        signal_value = map(lambda x: ' '.join(x.card.owners), overused_groups)
        overused_table_data.append(
            frout.TTableColumn("Admins", "List of admins", "str", signal_value, fltmode="fltregex", width="15%"))

        signal_value = map(lambda x: len(overused_hosts_by_group[x]), overused_groups)
        overused_table_data.append(
            frout.TTableColumn("Overused", "Number of overused machines", "str", signal_value, fltmode="fltrange",
                               width="5%"))

        signal_value = map(lambda x: len(x.getHosts()), overused_groups)
        overused_table_data.append(
            frout.TTableColumn("Total", "Number of total machines", "str", signal_value, fltmode="fltrange",
                               width="5%"))

        signal_value = map(lambda x: map(lambda y: y.name, overused_hosts_by_group[x][:100]), overused_groups)
        overused_table_data.append(
            frout.TListTableColumn("Overused hosts", "Example overused hosts", "host", signal_value, width="50%"))

        result['have_overused'] = frout.generate_table(overused_table_data)

        return result


class MongoGenerateMemoryReport(WrappedUtil):
    NAME = "mongo_memory_usage_report"
    SECTION = "Mongo"
    TITLE = "Memory Usage"

    IS_CHANGING_DB = False
    RUN_DEFAULT = True

    def __init__(self):
        super(MongoGenerateMemoryReport, self).__init__()

    def get_params_impl(self, parent, request):
        return [
                   TXGroupsInput("groups", "Groups",
                                 "Comma-separated list of regex on group (like 'SAS.*,.*_WEB_BASE')", "ALL"),
                   TGroupsInput("exclude_groups", "Excluded Groups", "Comma-separated list of excluded groups", "",
                                allow_empty=True),
                   TAddSlavesCheckboxInput("add_slave_groups", "Slave groups statistics", "Add slave groups", True),
                   TBoolFilterElementInput("ignore_nonsearch", "Ignore non-search",
                                           "No statistics for non-search groups", True,
                                           lambda x: x.card.properties.nonsearch == False),
                   TBoolFilterElementInput("ignore_background", "Ignore background groups",
                                           "No statistics for background groups (like golovan yasmagent)", True,
                                           lambda x: x.card.properties.background_group == False),
               ] + standard_tag_filters(parent.db) + [
                   TTimeStampInput("timestamp", "Timestamp", "0h"),
               ]

    def generate_tree(self, nomongo_groups, noreqs_groups, filtered_groups, generated_report):
        nomongo_groups = map(lambda x: frout.TLeafElement(x.card.name, tp='group'), nomongo_groups)
        noreqs_groups = map(lambda x: frout.TLeafElement(x.card.name, tp='group'), noreqs_groups)
        filtered_groups = map(lambda x: frout.TLeafElement(x.card.name, tp='group'), filtered_groups)

        result = frout.TTreeElement(frout.TLeafElement("Groups statistics"))

        result.children.append(
            frout.TTreeElement(
                frout.TLeafElement('Groups without memory reqs (%d total)' % len(noreqs_groups)),
                frout.TLeafElement(noreqs_groups, tp='list')
            )
        )

        result.children.append(
            frout.TTreeElement(
                frout.TLeafElement('Groups without statistics in mongo (%d total)' % len(nomongo_groups)),
                frout.TLeafElement(nomongo_groups, tp='list')
            )
        )

        result.children.append(
            frout.TTreeElement(
                frout.TLeafElement('Groups not ready to calculate memory statistics (%d total)' % len(filtered_groups)),
                frout.TLeafElement(filtered_groups, tp='list')
            )
        )

        # calculate wasted memory (difference between assigned and really used
        wasted_memory = sum(map(lambda x: self.group_wasted_memory(generated_report, x), generated_report.keys()))
        assigned_memory = sum(map(lambda x: round(
            x.card.reqs.instances.memory_guarantee.megabytes() / 1024. * len(x.get_kinda_busy_instances()), 2),
                                  generated_report.keys()))
        assigned_memory = max(assigned_memory, 1)
        result.children.append(
            frout.TTreeElement(
                frout.TLeafElement('Wasted memory (in gigabytes)'),
                frout.TLeafElement('%s Gb (%s%% of assigned)' % (
                wasted_memory, round(100 * wasted_memory / float(assigned_memory), 2))),
            )
        )

        # calculate unassigned memory (differenct between total memory on hosts and assigned)
        all_hosts = list(set(sum(map(lambda x: list(x.get_kinda_busy_hosts()), generated_report.keys()), [])))
        all_memory = sum(map(lambda x: x.memory, all_hosts))
        all_memory = max(all_memory, 1)
        unassigned_memory = all_memory - assigned_memory
        result.children.append(
            frout.TTreeElement(
                frout.TLeafElement('Unassigned memory (in gigabytes)'),
                frout.TLeafElement('%s Gb (%s%% of total)' % (
                unassigned_memory, round(100 * unassigned_memory / float(all_memory), 2))),
            )
        )

        return [result.to_json()]

    def group_wasted_memory(self, generated_report, group):
        n_instances = len(group.get_kinda_busy_instances())
        used_memory = generated_report[group]['instance_mem_usage']
        assigned_memory = round(group.card.reqs.instances.memory_guarantee.megabytes() / 1024., 2)

        if assigned_memory > 0:
            unused_ratio = used_memory / assigned_memory
        else:
            unused_ratio = 0.0

        if unused_ratio < 0.85:
            unused_ratio = 0.85 - unused_ratio
        #        elif unused_ratio > 1.0:
        #            unused_ratio = 1.0 - unused_ratio
        else:
            unused_ratio = 0.0

        return round(n_instances * assigned_memory * unused_ratio, 2)

    def generate_table(self, generated_report):
        groups = generated_report.keys()

        table_data = []

        signal_value = map(lambda x: x.card.name, groups)
        table_data.append(frout.TTableColumn("Group name", "Group name", "group", signal_value,
                                             fltmode="fltregex", width="18%"))

        signal_value = map(lambda x: ' '.join(x.card.owners), groups)
        table_data.append(frout.TTableColumn("Group owners", "Group owners", "str", signal_value,
                                             fltmode="fltregex", width="12%"))

        signal_value = map(lambda x: len(x.get_kinda_busy_instances()), groups)
        table_data.append(frout.TTableColumn("NInstancess", "Number of group instances", "int", signal_value,
                                             fltmode="fltrange", width="8%"))

        signal_value = map(lambda x: generated_report[x]['instance_mem_usage'], groups)
        table_data.append(frout.TTableColumn("IMem", "Memory, used by instance (in gigabytes)", "str",
                                             signal_value, fltmode="fltrange", width="8%"))

        signal_value = map(lambda x: round(x.card.reqs.instances.memory_guarantee.megabytes() / 1024., 2), groups)
        table_data.append(frout.TTableColumn("GMem", "Memory, used by instance (according to gencfg)", "str",
                                             signal_value, fltmode="fltrange", width="8%"))

        signal_value = map(lambda x: round(100 * generated_report[x]['instance_mem_usage'] / (
        x.card.reqs.instances.memory_guarantee.megabytes() / 1024.), 2), groups)
        table_data.append(frout.TTableColumn("PMem", "Ratio of used memory to assigned memory", "float",
                                             signal_value, fltmode="fltrange", width="8%"))

        signal_value = map(lambda x: self.group_wasted_memory(generated_report, x), groups)
        table_data.append(frout.TTableColumn("WMem", "Total wasted memory (in gigabytes)", "float",
                                             signal_value, fltmode="fltrange", width="10%"))

        # calculate rough unassigned memory (only for master groups)
        signal_value = []
        for group in groups:
            if group.master is not None:
                signal_value.append(0.)
            else:
                total_memory = sum(map(lambda x: x.memory, group.getHosts()))

                group_and_slaves = filter(lambda x: x in groups, [group] + group.slaves)
                assigned_memory = sum(map(lambda x: round(
                    x.card.reqs.instances.memory_guarantee.megabytes() / 1024. * len(x.get_kinda_busy_instances()), 2),
                                          group_and_slaves))

                signal_value.append(total_memory - assigned_memory)
        table_data.append(frout.TTableColumn("UMem", "Total unassigned memory (in gigabytes, very rough)", "float",
                                             signal_value, fltmode="fltrange", width="10%"))

        signal_value = map(lambda x: generated_report[x]['ts'], groups)
        table_data.append(frout.TTableColumn("Timestamp", "Timestamp of last statistics update", "str", signal_value,
                                             converter=lambda x: time.strftime("%Y-%m-%d %H:%M", time.localtime(x)),
                                             width="10%"))

        return frout.generate_table(table_data)

    def get_result(self, parent, request, form_params):
        params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)

        if form_params.get('ignore_nonsearch', '1') == '1':
            params['groups'] = filter(lambda x: x.card.properties.nonsearch == False, params['groups'])

        params['groups'] = filter(lambda x: x not in params['exclude_groups'], params['groups'])

        filtered_groups = filter(
            lambda x: x.card.properties.unraisable_group == True or x.card.properties.fake_group == True,
            params['groups'])
        params['groups'] = filter(
            lambda x: x.card.properties.unraisable_group == False and x.card.properties.fake_group == False,
            params['groups'])

        noreqs_groups = filter(lambda x: x.card.reqs.instances.memory_guarantee.value == 0, params['groups'])
        params['groups'] = filter(lambda x: x.card.reqs.instances.memory_guarantee.value > 0, params['groups'])

        # filter out slaves with master groups not in list
        # params['groups'] = filter(lambda x: x.master is None or x.master in params['groups'], params['groups'])

        from utils.mongo import generate_report
        generated_report = generate_report.main(dict_to_options(params))

        nomongo_groups = list(generated_report[0])
        generated_report = dict(map(lambda (x, y): (parent.db.groups.get_group(x), y), generated_report[1].iteritems()))

        result = {
            'tree': self.generate_tree(nomongo_groups, noreqs_groups, filtered_groups, generated_report),
            'table': self.generate_table(generated_report),
        }

        return result


class MailAdmins(WrappedUtil):
    NAME = 'mail_admins'
    SECTION = "Other"
    TITLE = "Mail Admins"

    IS_CHANGING_DB = False
    RUN_DEFAULT = False

    def __init__(self):
        super(MailAdmins, self).__init__()

    def get_params_impl(self, parent, request):
        return [
            frin.TFrontStringInput("subject", "Mail Subject", "Mail subject", ""),
            frin.TFrontStringInput("recipients", "Extra recipients",
                                   "Extra recipients (added to extracted from mail body)", "", allow_empty=True),
            frin.TFrontStringInput("content", "Mail Body", "Mail subject", "", tp="text"),
            frin.TFrontCheckboxInput("load_recipients_from_groups", "Extract recipients from groups",
                                     "For every group, found in mail body, add its owners to recipeints", True),
            frin.TFrontCheckboxInput("load_recipients_from_hosts", "Extract recipients from hosts",
                                     "For every host, found in mail body, add its owners to recipeints", True),
        ]

    def get_result(self, parent, request, form_params):
        params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)
        params['per_admin_objects'] = True
        params['prompt'] = False

        from utils.common import spam_admins
        params = spam_admins.get_parser().parse_json(params)
        spam_admins.normalize(params)

        result, status_tree = frout.wrap_exception(lambda: spam_admins.main(params))

        result_tree = [status_tree.to_json()]
        if result is not None:
            result_tree.append(
                frout.TTreeElement(
                    frout.TLeafElement("Recipients"),
                    frout.TLeafElement(",".join(result["recipients"]))
                ).to_json()
            )

        return {
            'tree': result_tree,
        }


TAGDIR = "/home/kimkim/tmp/94.2/dbs"


def calculate_timestamp(subdir):
    return DB(os.path.join(TAGDIR, subdir, 'db')).get_repo().get_last_commit().date


def calculate_timestamps():
    subdirs = sorted(os.listdir(TAGDIR))
    return map(lambda x: (time.strftime('%d.%m.%Y', time.localtime(calculate_timestamp(x))), x), subdirs)


class ShowMetagroups(WrappedUtil):
    LOCATION_FILTERS = {
        'all': lambda x: True,
        'msk': lambda x: x.location == 'msk',
        'sas': lambda x: x.location == 'sas',
        'man': lambda x: x.location == 'man',
    }

    NAME = "show_metagroups"
    SECTION = "Other"
    TITLE = "Show Metagroups"
    IS_CHANGING_DB = True
    RUN_DEFAULT = False

    def get_params_impl(self, parent, request):
        return [
            frin.TFrontStringInput("config", "Metagroups config", "Metagroups config in lua format",
                                   open(os.path.join(CURDB.PATH, 'configs', 'other', 'osolmetagroups.yaml')).read(),
                                   tp="text"),
            frin.TFrontChoiceInput("location", "Select location", "Select location or choose <all> to get all stats",
                                   "all",
                                   [("all", "all"), ("msk", "msk"), ("sas", "sas"), ("man", "man")]),
            frin.TFrontChoiceInput("start_db", "Start date", "Select start accounting date", "",
                                   calculate_timestamps()),
            frin.TFrontChoiceInput("finish_db", "Finish date", "Select end accounting date", "",
                                   calculate_timestamps()),
        ]

    def generate_tree(self, metagroups, dt):
        root_node = frout.TTreeElement(frout.TLeafElement("Groups at %s" % dt))
        for metagroup in metagroups:
            metagroup_node = frout.TTreeElement(frout.TLeafElement(metagroup.descr))
            for group in metagroup.groups:
                group_power = sum(map(lambda x: x.power, group.getHosts()))
                group_memory = sum(map(lambda x: x.memory, group.getHosts()))
                group_node = frout.TTreeElement(
                    frout.TLeafElement(group.card.name, tp='group', css=" utils-mongo__info"),
                    frout.TLeafElement("%s power, %s Gb memory" % (group_power, group_memory)))
                metagroup_node.children.append(group_node)

            root_node.children.append(metagroup_node)
        return root_node

    def generate_usage_url(self, metagroup, location, base_url, startt, endt):
        if location == 'msk':
            groups = filter(lambda x: x.card.name.startswith('MSK_'), metagroup.groups)
        elif location == 'sas':
            groups = filter(lambda x: x.card.name.startswith('SAS_'), metagroup.groups)
        elif location == 'man':
            groups = filter(lambda x: x.card.name.startswith('MAN_'), metagroup.groups)
        elif location == 'ams':
            groups = filter(lambda x: x.card.name.startswith('AMS_'), metagroup.groups)
        else:
            groups = metagroup.groups
        groups = map(lambda x: x.card.name, groups)

        params = {
            'startt': startt,
            'endt': endt,
            'metagroup_descr': metagroup.descr,
            'graphs': [
                {
                    'groups': groups,
                    'signal': 'cpu',
                },
                {
                    'groups': groups,
                    'signal': 'memory',
                },
            ]
        }
        params = json.dumps(params)

        url = '%s%s' % (base_url, '/mongodb/metagroups/histusage/%s' % params)

        return url

    def generate_table(self, start_statistics, finish_statistics, params, base_url):
        def c_perc(v):
            return frout.Converters.percents(v)

        def c_sign_perc(v):
            return frout.Converters.sign_percents(v)

        def c_smart_float(v):
            return frout.Converters.smart_float(v)

        table_data = []

        signal_value = map(lambda x: x.descr, finish_statistics)
        table_data.append(
            frout.TTableColumn("Metagroup name", "Metagroup name as defined in config", "str", signal_value,
                               fltmode="fltregex", width="20%"))

        start_timestamp, end_timestamp = calculate_timestamp(params['start_db']), calculate_timestamp(
            params['finish_db'])
        signal_value = map(lambda x: {
            'value': "%s (%s)" % (
                c_smart_float(x.stats['used_cpu']),
                c_perc(x.stats['used_cpu'] / (x.stats['assigned_cpu'] + 0.001) * 100)),
            'externalurl': self.generate_usage_url(x, params['location'], base_url, start_timestamp, end_timestamp)
        }, finish_statistics)
        #        signal_value = map(lambda x: { 'value' : x, 'externalurl' : 'http://lenta.ru' }, signal_value)
        table_data.append(
            frout.TTableColumn("CpuU", "Cpu usage at finish date (in gencfg units)", "externalurl", signal_value,
                               fltmode="fltdisabled", width="10%"))

        signal_value = map(lambda x: x.stats['assigned_cpu'], finish_statistics)
        table_data.append(
            frout.TTableColumn("CpuA", "Assigned cpu at finish date (in gencfg units)", "str", signal_value,
                               converter=c_smart_float, fltmode="fltrange", width="5%"))

        signal_value = map(lambda x: x.stats['assigned_cpu'] / 1293., finish_statistics)
        table_data.append(
            frout.TTableColumn("Cpu E5-2650v2", "Assigned cpu at finish date (in machines of E5-2650v2)", "str",
                               signal_value,
                               converter=c_smart_float, fltmode="fltrange", width="5%"))

        #        signal_value = map(lambda x: "%s (%s)" % (
        #                c_smart_float(x.stats['used_mem']),
        #                c_perc(x.stats['used_mem'] / (x.stats['assigned_mem'] + 0.001) * 100)),
        #            finish_statistics)
        #        table_data.append(frout.TTableColumn("MemoryU", "Memory usage at finish date (in gigabytes)", "str", signal_value,
        #                                                       fltmode = "fltdisabled", width = "10%"))

        signal_value = map(lambda x: x.stats['assigned_mem'], finish_statistics)
        table_data.append(
            frout.TTableColumn("MemoryA", "Assigned memory at finish date (in gigabytes)", "str", signal_value,
                               converter=c_smart_float, fltmode="fltrange", width="5%"))
        signal_value = map(lambda x: x.stats['assigned_mem'] / 256, finish_statistics)
        table_data.append(frout.TTableColumn("Memory E5-2650v2",
                                             "Assigned memory at finish date (in numver of E5-2650v2 with 256Gb))",
                                             "str", signal_value,
                                             converter=c_smart_float, fltmode="fltrange", width="5%"))

        signal_value = map(lambda x, y: "%s (%s)" % (
            c_smart_float(y.stats['assigned_cpu'] - x.stats['assigned_cpu']),
            c_sign_perc(y.stats['assigned_cpu'] / (x.stats['assigned_cpu'] + 0.001) * 100 - 100)),
                           start_statistics, finish_statistics)
        table_data.append(frout.TTableColumn("CpuDiff", "Cpu added since start date", "str", signal_value,
                                             fltmode="fltdisabled", width="10%"))

        #        signal_value = map(lambda x, y: "%s (%s)" % (
        #                c_smart_float(y.stats['assigned_mem'] - x.stats['assigned_mem']),
        #                c_sign_perc(y.stats['assigned_mem'] / (x.stats['assigned_mem'] + 0.001) * 100 - 100)),
        #            start_statistics, finish_statistics)
        #        table_data.append(frout.TTableColumn("MemoryDiff", "Memory added since start date", "str", signal_value,
        #                                                       fltmode = "fltrange", width = "10%"))

        return frout.generate_table(table_data, disable_filters=True)

    def get_result(self, parent, request, form_params):
        params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)

        from utils.common import show_metagroups_power2

        params['config'] = yaml.load(params['config'])
        params['host_filter'] = self.LOCATION_FILTERS[params['location']]

        # calculate start statistics
        params['calculate_used'] = False
        params['db'] = DB(os.path.join(TAGDIR, params['start_db'], 'db'))
        start_statistics = show_metagroups_power2.main(show_metagroups_power2.get_parser().parse_json(params))

        # calculate end statistics
        params['calculate_used'] = True
        params['db'] = DB(os.path.join(TAGDIR, params['finish_db'], 'db'))
        end_statistics = show_metagroups_power2.main(show_metagroups_power2.get_parser().parse_json(params))

        result_tree = [
            self.generate_tree(start_statistics, "start date").to_json(),
            self.generate_tree(end_statistics, "end date").to_json(),
        ]
        result_table = self.generate_table(start_statistics, end_statistics, params, parent.api.gen_base_url())

        result = {
            'tree': result_tree,
            'table': result_table,
        }

        return result


class AllocateGroupInDynamic(WrappedUtil):
    NAME = "allocate_group_in_dynamic"
    SECTION = "Dynamic"
    TITLE = "Allocate"
    IS_CHANGING_DB = True
    RUN_DEFAULT = False

    ALLOCATE_LOCATIONS = [
        ("MAN", "allocate_in_man", "Allocate in Man", "Allocate group in Man (prefix MAN)", False),
        ("SAS", "allocate_in_sas", "Allocate in Sas", "Allocate group in Sas (prefix SAS)", False),
        ("MSK_IVA", "allocate_in_msk_iva", "Allocate in Msk Iva", "Create group in Msk Iva (prefix MSK_IVA)", False),
        ("MSK_MYT", "allocate_in_msk_myt", "Allocate in Msk Myt", "Create group in Msk Myt (prefix MSK_MYT)", False),
        ("VLA", "allocate_in_vla", "Allocate in Vla", "Create group in Vladimir (prefix VLA)", False),
    ]

    def get_params_impl(self, parent, request):
        help_models = map(lambda x: parent.db.cpumodels.get_model(x), ['E5530', 'E5645', 'E5-2660', 'E5-2650v2', 'X5675'])
        dynamic_groups = map(lambda x: (x.card.name, x.card.name),
                             filter(lambda x: 'dynamic-all' in x.card.tags.prj, CURDB.groups.get_groups()))
        avail_tiers = map(lambda x: x.name, CURDB.tiers.get_tiers())

        extra_params = []

        stats_tree = frout.TTreeElement(frout.TLeafElement("All dynamic groups statistics"))
        for groupname, something in dynamic_groups:
            import optimizers.dynamic.main as update_fastconf
            subparams = {'master_group': groupname, 'action': 'get_stats',}
            subparams = update_fastconf.get_parser().parse_json(subparams)
            group_stats = update_fastconf.main(subparams, parent.db, from_cmd=False)

            group_tree = frout.TTreeElement(frout.TLeafElement(groupname, "group"))
            for location in group_stats:
                lstats = group_stats[location]
                group_location_tree = frout.TTreeElement(frout.TLeafElement(location))
                pct = 100 * lstats.free_room.power / max(float(lstats.total_room.power), 1)
                group_location_tree.children.append(frout.TExtenededLeafElement("Power Left", "%s units (%.2f%%)" % \
                                                                                (lstats.free_room.power, pct)))
                pct = 100 * lstats.free_room.memory / max(float(lstats.total_room.memory), 1)
                group_location_tree.children.append(frout.TExtenededLeafElement("Memory Left", "%.1f Gb (%.2f%%)" % \
                                                                                (lstats.free_room.memory, pct)))
                pct = 100 * lstats.free_room.disk / max(float(lstats.total_room.disk), 1)
                group_location_tree.children.append(frout.TExtenededLeafElement("Disk Left", "%.0f Gb(%.2f%%)" % \
                                                                                (lstats.free_room.disk, pct)))
                pct = 100 * lstats.free_room.ssd / max(float(lstats.total_room.ssd), 1)
                group_location_tree.children.append(frout.TExtenededLeafElement("SSD Left", "%.0f Gb(%.2f%%)" % \
                                                                                (lstats.free_room.ssd, pct)))

                group_tree.children.append(group_location_tree)

            stats_tree.children.append(group_tree)

        extra_params.append(frin.TPrintableInput('stats_tree', [stats_tree]))

        return extra_params + [
            frin.TDescrInput("group_options", "Common group params"),
            frin.TFrontChoiceInput("master_group", "Master group name",
                                   "Main group for dynamic allocation of resources", None, dynamic_groups),
            frin.TFrontStringInput("group", "Group Name", frin.TMdDocPath("name"), ""),
            frin.TFrontStringInput("description", "Group description", frin.TMdDocPath("description"), "", tp="text"),
            frin.TFrontStringInput("owners", "Group owners", frin.TMdDocPath("owners"),
                                   request.yauth.login if request.yauth.login else "", tp="logins"),
            frin.TFrontStringInput("watchers", "Group watchers", frin.TMdDocPath("watchers"), "", tp="logins",
                                   allow_empty=True),

            # tag options
            frin.TDescrInput("tags_options", "Group tags"),
            frin.TFrontCIMInput("ctype", "Cluster Type (a_ctype)", frin.TMdDocPath("tags.ctype"), "none", parent.db,
                                "ctype", multiple=False),
            frin.TFrontCIMInput("itype", "Instance Type (a_itype)", frin.TMdDocPath("tags.itype"), "none", parent.db,
                                "itype", multiple=False),
            frin.TFrontStringInput("prj", "Project (a_prj)", frin.TMdDocPath("tags.prj"), "none"),
            frin.TFrontCIMInput("metaprj", "Project (a_metaprj)", frin.TMdDocPath("tags.metaprj"), "web", parent.db,
                                "metaprj", multiple=False),

            # instances reqsource requirements
            frin.TDescrInput("dynamic_options", "Group resources requirements"),
            frin.TFrontStringInput("memory", "Memory per instance", frin.TMdDocPath("reqs.instances.memory_guarantee"),
                                   "1 Gb", value_checker=check_bytesize),
            frin.TFrontNumberInput("min_power", "Needed power PER LOCATION", frin.TMdDocPath("reqs.shards.min_power"),
                                   0, tp="int"),
            frin.TFrontNumberInput("cores_per_instance", "Cores PER INSTANCE",
                                   "Cores per instance (mutually exclusive with <Needed power PER LOCATION> field", 0,
                                   tp="int"),
            frin.TFrontNumberInput("min_replicas", "Minimal number of instances PER LOCATION",
                                   frin.TMdDocPath("reqs.shards.min_replicas"), 3, tp="int"),
            frin.TFrontNumberInput("max_replicas", "Maximal number of instances PER LOCATION",
                                   frin.TMdDocPath("reqs.shards.max_replicas"), 3, tp="int"),
            frin.TFrontCheckboxInput("equal_instances_power", "Allocate instances with equal power",
                                     frin.TMdDocPath("reqs.shards.equal_instances_power"), True),
            frin.TFrontStringInput("disk", "Required disk per instance", frin.TMdDocPath("reqs.instances.disk"), "0 Gb",
                                   value_checker=check_bytesize),
            frin.TFrontStringInput("ssd", "Required ssd per instance", frin.TMdDocPath("reqs.instances.ssd"), "0 Gb",
                                   value_checker=check_bytesize),

            # restriction options
            frin.TDescrInput("restriction_options", "Restrict allocated macihnes"),
            frin.TFrontCheckboxInput("l3enabled", "Allocate machines with L3 only", "Allocate machines with L3 only", False),

            # intlookup creation options
            frin.TDescrInput("intlookup_options", "Intlookup generation options (optional)"),
            frin.TFrontStringFromPreparedInput("tier_name", "Tier name", "Tier name", "", avail_tiers, allow_empty=True,
                                               multiple=False),

            # gaux options
            frin.TDescrInput("port_options", "Port options (optional)"),
            frin.TFrontCheckboxInput("all_groups_same_port", "Create same port for group in every DC",
                                     "Create same port for group in every DC", False),
            frin.TFrontNumberInput("all_groups_port", "Custom port for allocated groups (UNSAFE)",
                                   "Custom port for allocated groups (UNSAFE)", None, tp="int", allow_empty=True),

            # location options
            frin.TDescrInput("location_options", "Affected locations"),

        ] + map(lambda x: frin.TFrontCheckboxInput(*x[1:]), self.ALLOCATE_LOCATIONS) + [

                   frin.TDescrInput("apply_options", "Apply options"),
                   frin.TFrontCheckboxInput("apply", "Apply changes", "Apply and commit changes", False),
                   frin.TFrontStringInput("commit_message", "Commit message", "Detailed commit message", "", tp="text",
                                          allow_empty=True),
               ]

    def get_slave_form_params(self):
        def get_group_property(path, converter, wbe, params):
            if 'srcgroup' in params:
                group = wbe.db.groups.get_group(params['srcgroup'])
                return converter(group.card.get_card_value(path))
            else:
                raise Exception("Bad slave form params %s for allocate_group_in_dynamic" % params)

        return {
            'owners': functools.partial(get_group_property, ["owners"], lambda x: ",".join(x)),
            'description': functools.partial(get_group_property, ["description"], lambda x: x),
            'watchers': functools.partial(get_group_property, ["watchers"], lambda x: ",".join(x)),
            'ctype': functools.partial(get_group_property, ["tags", "ctype"], lambda x: x),
            'itype': functools.partial(get_group_property, ["tags", "itype"], lambda x: x),
            'prj': functools.partial(get_group_property, ["tags", "prj"], lambda x: ",".join(x)),
            'metaprj': functools.partial(get_group_property, ["tags", "metaprj"], lambda x: x),
            'memory': functools.partial(get_group_property, ["reqs", "instances", "memory_guarantee"],
                                        lambda x: str(x)),
            'min_power': functools.partial(get_group_property, ["reqs", "shards", "min_power"], lambda x: x),
            'min_replicas': functools.partial(get_group_property, ["reqs", "shards", "min_replicas"], lambda x: x),
            'equal_instances_power': functools.partial(get_group_property, ["reqs", "shards", "equal_instances_power"],
                                                       lambda x: x),
            'l3enabled': functools.partial(get_group_property, ["reqs", "hosts", "l3enabled"], lambda x: x),
        }

    def check_params(self, parent, request, json_params, raw_json):
        result = super(AllocateGroupInDynamic, self).check_params(parent, request, json_params, raw_json)

        allocate_location_keys = map(lambda x: x[1], self.ALLOCATE_LOCATIONS)

        if len(filter(lambda x: x["name"] in allocate_location_keys and x["value"] == True,
                      result["input_fields"])) == 0:
            result["error"] = True
            for elem in result["input_fields"]:
                if elem["name"] in allocate_location_keys:
                    elem["errormsg"] = "You should choose at least one location"

        min_power_field = filter(lambda x: x["name"] == "min_power", result["input_fields"])[0]
        cores_per_instance_field = filter(lambda x: x["name"] == "cores_per_instance", result["input_fields"])[0]
        if "errormsg" not in min_power_field and "errormsg" not in cores_per_instance_field:
            if int(min_power_field["value"]) > 0 and int(cores_per_instance_field["value"]) > 0:
                min_power_field["errormsg"] = "Can not specify both <Needed power> and <Cores per instance>"
                cores_per_instance_field["errormsg"] = "Can not specify both <Needed power> and <Cores per instance>"

        return result

    def get_result(self, parent, request, form_params):
        form_params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)

        import optimizers.dynamic.main as update_fastconf

        try:
            allocated_groups = []
            for location_prefix, location_key, _1, _2, _3 in self.ALLOCATE_LOCATIONS:
                if form_params[location_key]:
                    update_fastconf_params = copy.deepcopy(form_params)
                    update_fastconf_params["action"] = "add"
                    if update_fastconf_params[
                        "cores_per_instance"] > 0:  # translate cores_per_instances into total power required
                        update_fastconf_params["min_power"] = 40 * update_fastconf_params["cores_per_instance"] * \
                                                              update_fastconf_params["min_replicas"]
                        update_fastconf_params["max_replicas"] = update_fastconf_params["min_replicas"]
                        update_fastconf_params["equal_instances_power"] = True

                    subutil_params = update_fastconf.get_parser().parse_json(update_fastconf_params)

                    subutil_params.location = location_prefix
                    subutil_params.group = "%s_%s" % (location_prefix, subutil_params.group)
                    subutil_params.owners = core.argparse.types.comma_list(subutil_params.owners)
                    subutil_params.watchers = core.argparse.types.comma_list(subutil_params.watchers)
                    subutil_params.prj = core.argparse.types.comma_list(subutil_params.prj)
                    subutil_params.apply = False

                    update_fastconf.normalize(subutil_params)
                    newgroup, allocation_report = update_fastconf.main(subutil_params, parent.db, from_cmd=False)
                    allocation_report.commit(newgroup)
                    allocated_groups.append(newgroup)

                    # allocate intlookups if necessary
                    if form_params["tier_name"] not in [None, ""]:
                        from utils.pregen import generate_trivial_intlookup
                        gti_params = {
                            "groups": [newgroup.card.name],
                            "bases_per_group": 1,
                            "shard_count": form_params["tier_name"],
                            "output_file": newgroup.card.name,
                            "no_apply": True,
                        }
                        gti_params = generate_trivial_intlookup.get_parser().parse_json(gti_params)
                        generate_trivial_intlookup.main(gti_params)

            # fix ports if necessary
            all_groups_port = None
            if form_params["all_groups_same_port"]:
                all_groups_port = allocated_groups[-1].card.reqs.instances.port
            if form_params["all_groups_port"] is not None:
                all_groups_port = form_params["all_groups_port"]

            if all_groups_port is not None:
                for group in allocated_groups:
                    utils.common.change_port.jsmain({
                        'group': group,
                        'port_func': 'old%s' % all_groups_port,
                        'no_apply': True,
                    })
                    group.card.reqs.instances.port = all_groups_port

            # generate printable tree
            if form_params['apply']:
                result_node = frout.TTreeElement(frout.TLeafElement("Status"),
                                                 frout.TLeafElement("COMMITED",
                                                                    css="printable__bold_text printable__green_text"))
            else:
                result_node = frout.TTreeElement(frout.TLeafElement("Status"),
                                                 frout.TLeafElement("NOT COMMITED",
                                                                    css="printable__bold_text printable__red_text"))
            groups_node = frout.TTreeElement(frout.TLeafElement("Allocated groups"))
            for group in allocated_groups:
                group_node = frout.TTreeElement(frout.TLeafElement(group.card.name, tp="group"))

                group_instances_node = frout.TTreeElement(frout.TLeafElement('Added instances'))
                for instance in group.get_instances():
                    instance_node = frout.TExtenededLeafElement(
                        {"value": "%s:%s" % (instance.host.name.partition('.')[0], instance.port)},
                        {"value": "%s power" % int(instance.power)},
                    )
                    group_instances_node.children.append(instance_node)
                group_node.children.append(group_instances_node)
                groups_node.children.append(group_node)
            result_node.children.append(groups_node)

            # update input fields
            input_fields = {}
            if form_params.get("commit_message", "") == "":
                sample_commit_message = "Added groups: %s" % (" ".join(map(lambda x: x.card.name, allocated_groups)))
                input_fields["commit_message"] = sample_commit_message

            # apply or revert changes
            if form_params['apply']:
                parent.db.update(smart=True)
            else:
                # have to revert changed here :(
                for group in allocated_groups:
                    parent.db.groups.remove_group(group)
            self.parent_api_commit(parent, request, "[fast_checked] %s" % form_params['commit_message'])
        except (TValidateCardNodeError, update_fastconf.NoResourcesError, UtilNormalizeException) as error:
            for group in allocated_groups:
                parent.db.groups.remove_group(group)

            short_reason = {
                TValidateCardNodeError: "INVALID OPTIONS",
                update_fastconf.NoResourcesError: "NOT ENOUGH RESOURCES",
                UtilNormalizeException: "INVALID OPTIONS",
            }[error.__class__]

            result_node = frout.TTreeElement(frout.TLeafElement("Status"),
                                             frout.TLeafElement(short_reason, css="printable__bold_text printable__red_text"))
            result_node.children.append(
                frout.TExtenededLeafElement({"value": "Reason"}, {"value": str(error), "css": "printable__red_text"}))

            input_fields = {}
        return {
            'tree': map(lambda x: x.to_json(), [result_node]),
            'input_fields': input_fields,
        }


class ShowGitLog(WrappedUtil):
    NAME = "show_git_log"
    SECTION = "Svn"
    TITLE = "Show log"

    IS_CHANGING_DB = False
    RUN_DEFAULT = True

    def get_params_impl(self, parent, request):
        return [
            TTimeStampInput("startts", "Start timestamp", "2h"),
        ]

    def get_result(self, parent, request, form_params):
        form_params = frin.convert_input_from_frontend(self.get_params_impl(parent, request), form_params)

        # get list of commits
        import utils.mongo.update_commits_info as update_commits_info
        subutil_params = {
            "action": update_commits_info.EActions.SHOW,
            "update_db": (form_params.get("offset", 0) == 0),
            "limit": form_params.get("limit", 10),
            "offset": form_params.get("offset", 0),
            "author": form_params.get("author", None),
            "modifies": form_params.get("modifies", None),
            "labels": form_params.get("labels", None),
        }
        commits = update_commits_info.jsmain(subutil_params)

        result = {
            'table': generate_commits_table(commits).to_json(),
        }

        return result


class ShowModels(WrappedUtil):
    NAME = "show_cpu_models"
    SECTION = "Other"
    TITLE = "Cpu Models"

    IS_CHANGING_DB = False
    RUN_DEFAULT = True

    def get_params_impl(self, parent, request):
        return []

    def get_result(self, parent, request, form_params):
        cpu_models = parent.db.cpumodels.models.values()

        table_data = []

        signal_value = map(lambda x: {"value": x.model,
                                      "externalurl": x.url if x.url != "" else "https://www.google.ru/search?&q=%s" % x.model},
                           cpu_models)
        table_data.append(
            frout.TTableColumn("Name", "Cpu model name", "externalurl", signal_value, fltmode="fltregex", width="4%"))

        signal_value = map(lambda x: x.fullname, cpu_models)
        table_data.append(
            frout.TTableColumn("Full name", "Full cpu model name", "str", signal_value, fltmode="fltregex",
                               width="15%"))

        signal_value = map(lambda x: x.power, cpu_models)
        table_data.append(
            frout.TTableColumn("Machine power", "Machine power", "float", signal_value, fltmode="fltrange", width="5%"))

        signal_value = map(lambda x: x.ncpu, cpu_models)
        table_data.append(
            frout.TTableColumn("NCPU", "Number of cpus", "int", signal_value, fltmode="fltrange", width="3%"))

        signal_value = map(lambda x: x.tbfreq[-1], cpu_models)
        table_data.append(
            frout.TTableColumn("Freq (MHz)", "Cpu frequency (in MHz, TB On, Peak load)", "int", signal_value,
                               fltmode="fltrange", width="3%"))

        result = {
            'table': frout.generate_table(table_data),
        }

        return result


UTILS = OrderedDict(map(lambda x: (x.NAME, x), [
    MongoGenerateReport(),
    MongoGenerateMemoryReport(),
    MongoShowUnused(),
    MongoShowOverused(),
    MongoHaveDataStats(),
    ShowInstanceState(),
    #            AddMachines(),
    ReplaceHosts(),
    #            CheckAliveHosts(),
    CreateNewTag(),
    #            MailAdmins(),
    #            ShowMetagroups(),
    AllocateGroupInDynamic(),
    ShowGitLog(),
    ShowModels(),
]))
