# coding=utf8

"""Tests utils."""

from __future__ import unicode_literals

import os
import copy
import urllib

from tests.util import get_contents, get_yaml, get_db_by_path

import pytest

unstable_only = pytest.mark.unstable_only


def test_show_cpu_models(wbe):
    db = get_db_by_path(wbe.db_path, cached=True)

    for response in [wbe.api.get("/unstable/utils/show_cpu_models?action=get_result"),
                     wbe.api.get("/unstable/utils/show_cpu_models?action=get_result&json=[]")]:
        for elem in response["table"]["data"]:
            model_name = elem["cells"][0]["value"]
            power = elem["cells"][2]["value"]

            assert model_name in db.cpumodels.models
            assert db.cpumodels.get_model(model_name).power == power


def __test_util_params(wbe, util_name):
    get_db_by_path(wbe.db_path, cached=True)
    response = [wbe.api.get("/unstable/utils/%s?action=get_params" % util_name)]

    assert "slave_form_fields" in response[0]
    assert "input_fields" in response[0]


def __test_util_result(wbe, util_name, check_fields, extra_params=None):
    get_db_by_path(wbe.db_path, cached=True)

    path = "/unstable/utils/%s?action=get_result" % util_name
    if extra_params is not None:
        path = "%s&%s" % (path, extra_params)

    response = [wbe.api.get(path)]

    for field in check_fields:
        assert field in response[0]

    return response


# ============= mongo_usage_report tests ==============================
def test_mongo_usage_report_params(wbe):
    __test_util_params(wbe, "mongo_usage_report")


def test_mongo_usage_report_result(wbe):
    response_fields = ["aggregate_data", "havedata", "nodata"]
    __test_util_result(wbe, "mongo_usage_report", response_fields)


# ============= mongo_show_unused tests ==============================
def test_mongo_show_unused_params(wbe):
    __test_util_params(wbe, "mongo_show_unused")


def test_mongo_show_unused_result(wbe):
    response_fields = ["table", "tree"]
    __test_util_result(wbe, "mongo_show_unused", response_fields)


# ============= mongo_have_stats tests ===============================
def test_mongo_have_stats_params(wbe):
    __test_util_params(wbe, "mongo_have_stats")


def test_mongo_have_stats_result(wbe):
    response_fields = ["histurl", "table"]
    __test_util_result(wbe, "mongo_have_stats", response_fields)


# ============= replace_hosts tests ==================================
def test_replace_hosts_params(wbe):
    __test_util_params(wbe, "replace_hosts")


def test_replace_hosts_result(wbe):
    extra_params = 'json=' + urllib.quote(
        '[{"name":"hosts","value":"beatrice.search.yandex.net"},{"name":"src_groups","value":"MSK_RESERVED,SAS_RESERVED,MAN_RESERVED"},{"name":"dest_group","value":"ALL_UNWORKING"},{"name":"mem_affinity","value":"mem="},{"name":"power_affinity","value":"power="},{"name":"disk_affinity","value":"disk="},{"name":"ssd_affinity","value":"ssd="},{"name":"loaction_affinity","value":"queue=,dc=,location="},{"name":"apply","value":"0"},{"name":"commit_message","value":""},{"name":"filter","value":"lambda x: x.ssd > 0"}]')
    response_fields = ["tree"]
    __test_util_result(wbe, "replace_hosts", response_fields, extra_params=extra_params)


# ============= create_new_tag tests =================================
def test_create_new_tag_params(wbe):
    __test_util_params(wbe, "create_new_tag")


# =========== show_instance_state tests ==============================
def test_show_instance_state_params(wbe):
    __test_util_params(wbe, "show_instance_state")


def test_show_instance_state_result(wbe):
    response_fields = ["data", "filters", "header"]
    __test_util_result(wbe, "show_instance_state", response_fields)


# =========== mongo_show_overused tests ====================================
def test_mongo_show_overused_params(wbe):
    __test_util_params(wbe, "mongo_show_overused")


def test_mongo_show_overused_result(wbe):
    response_fields = ["have_overused", "no_overused"]
    __test_util_result(wbe, "mongo_show_overused", response_fields)


# =========== mongo_memory_usage_report tests ==============================
def test_mongo_memory_usage_report_params(wbe):
    __test_util_params(wbe, "mongo_memory_usage_report")


# =========== allocate_group_in_dynamic tests ==============================
def test_allocate_group_in_dynamic_params(wbe):
    __test_util_params(wbe, "allocate_group_in_dynamic")


def test_allocate_group_in_dynamic_result(wbe):
    PREFIXES = ["MSK_UGRB", "MSK_FOL", "SAS", "MAN"]
    group_template = 'TEST_GROUP'
    try:
        s = '[{"name":"master_group","value":"ALL_DYNAMIC"},{"name":"group","value":"%(group_template)s"},{"name":"description","value":"Название по русски"},{"name":"owners","value":"kimkim"},{"name":"watchers","value":""},{"name":"ctype","value":"none"},{"name":"itype","value":"none"},{"name":"prj","value":"none"},{"name":"metaprj","value":"unknown"},{"name":"memory","value":"1 Gb"},{"name":"min_power","value":"100"},{"name":"min_replicas","value":"3"},{"name":"max_replicas","value":"3"},{"name":"equal_instances_power","value":"1"},{"name":"tier_name","value":"MsuserdataTier0"},{"name":"all_groups_same_port","value":"1"},{"name":"all_groups_port","value":""},{"name":"allocate_in_man","value":"1"},{"name":"allocate_in_sas","value":"1"},{"name":"allocate_in_msk_fol","value":"1"},{"name":"allocate_in_msk_ugr","value":"1"},{"name":"commit_message","value":"Коммит по русски"},{"name":"apply","value":"1"},{"name":"l3enabled","value":"0"}]' % {
            "group_template": group_template}
        s = s.encode('utf-8')
        extra_params = 'json=' + urllib.quote(s)

        response_fields = ["tree"]
        __test_util_result(wbe, "allocate_group_in_dynamic", response_fields, extra_params)

        db = get_db_by_path(wbe.db_path, cached=False)

        all_instances = []
        # check generated groups instances
        for prefix in PREFIXES:
            group = db.groups.get_group("%s_%s" % (prefix, group_template))
            instances = group.get_instances()
            all_instances.extend(instances)

            assert len(instances) == 3
            assert max(map(lambda x: x.power, instances)) - min(map(lambda x: x.power, instances)) <= 1
            assert group.card.reqs.instances.memory_guarantee.text == "1 Gb"

        assert len(set(map(lambda x: x.port, all_instances))) == 1

    finally:
        for prefix in PREFIXES:
            groupname = "%s_%s" % (prefix, group_template)
            try:
                wbe.api.check_response_status(wbe.api.post("/unstable/groups", {
                    "action": "remove",
                    "group": groupname,
                    "recursive": True,
                }))
            except:
                pass


# ========================= show_git_log tests ============================
def test_show_git_log_params(wbe):
    __test_util_params(wbe, "show_git_log")


def test_show_git_log_result(wbe):
    response_fields = ["table"]
    __test_util_result(wbe, "show_git_log", response_fields)


# ========================= show_cpu_models tests ============================
def test_show_cpu_models_params(wbe):
    __test_util_params(wbe, "show_cpu_models")
