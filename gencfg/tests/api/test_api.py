import os
import sys
import tempfile
import pytest
import shutil
import urllib

unstable_only = pytest.mark.unstable_only

from gaux.aux_utils import run_command

from tests.util import dbpath_get_group, dbpath_has_group, get_db_by_path

SAMPLE_GROUP = "MSK_WEB_BASE"
SAMPLE_INTL2_GROUP = "MSK_WEB_INTL2"
SAMPLE_DYNAMIC_GROUP = "MSK_FOL_VERSTAK_MOBILE_PRIEMKA"


def _assert_dict_value(d, name, tp=None, val=None):
    assert name in d

    if tp is not None:
        assert isinstance(d[name], (tp, type(None))), "Field %s of wrong type %s, when %s needed" % (
        name, d[name].__class__, tp)

    if val is not None:
        assert d[name] == val, "Field %s with value <%s>, when should be with <%s>" % (name, d[name], val)


def test_groups(api):
    api.api.get("/unstable/groups")


def test_group(api):
    api.api.get("/unstable/groups/MSK_RESERVED")


def test_hosts(api):
    api.api.get("/unstable/hosts")


def test_hosts_to_groups(api):
    host_names = api.api.get("/unstable/hosts")["host_names"]
    host_names = sorted(host_names)
    api.api.post("/unstable/hosts/hosts_to_groups", {"hosts": host_names})


def test_guess_gencfg_fqdn(api):
    host_names = api.api.get("/unstable/hosts")["host_names"]
    host_names = sorted(host_names)
    api.api.post("/unstable/hosts/guess_gencfg_fqdn", {"hosts": host_names})


def test_group_instances(api):
    result = api.api.get("/unstable/groups/%s/instances" % SAMPLE_GROUP)

    assert "instances" in result
    assert len(result["instances"]) > 0

    instance = result["instances"][0]
    assert "hostname" in instance
    assert "power" in instance
    assert "port" in instance


def test_searcherlookup_instances(api):
    CHECK_FIELDS = [
        ("dc", unicode),
        ("domain", unicode),
        ("ipv4addr", unicode),
        ("ipv6addr", unicode),
        ("location", unicode),
        ("port", int),
        ("porto_limits", dict),
        ("power", float),
        ("tags", list),
        ("shard_name", unicode),
    ]

    result = api.api.get("/unstable/searcherlookup/groups/%s/instances" % SAMPLE_DYNAMIC_GROUP)

    assert "instances" in result

    for instance_data in result["instances"]:
        for field_name, field_type in CHECK_FIELDS:
            _assert_dict_value(instance_data, field_name, tp=field_type)
        assert instance_data["porto_limits"]["cpu_guarantee"] > 0
        assert instance_data["porto_limits"]["cpu_cores_guarantee"] > 0
        assert instance_data["porto_limits"]["memory_guarantee"] == 12884901888
        assert instance_data["porto_limits"]["memory_limit"] == 13958643712

        # assert instance_data["porto_limits"]["net_limit"] == 0

        def ltags(instance, prefix):
            return len(filter(lambda x: x.startswith(prefix), instance["tags"]))

        assert ltags(instance_data, "a_topology_group-%s" % SAMPLE_DYNAMIC_GROUP) == 1
        assert ltags(instance_data, "a_ctype") == 1
        assert ltags(instance_data, "a_itype") == 1
        assert ltags(instance_data, "a_metaprj") == 1


def test_searcherlookup_instances2(api):
    result = api.api.get("/unstable/searcherlookup/groups/%s/instances" % SAMPLE_INTL2_GROUP)

    assert "instances" in result


def test_searcherlookup_instances_notfound(api):
    result = api.api.get("/unstable/searcherlookup/groups/NON_EXISTING_GROUP/instances", ret_raw_result=True,
                         raise_bad_statuses=False)

    assert result.status_code == 404


def test_hosts_data(api):
    result = api.api.post("/unstable/hosts_data",
                          {"hosts": ["beatrice.search.yandex.net", "unexisting.host.yandex.ru"]})

    assert "hosts_data" in result
    assert len(result["hosts_data"]) == 1

    bdata = result["hosts_data"][0]
    assert bdata["name"] == "beatrice"
    assert bdata["dc"] == "ugrb"
    assert bdata["invnum"] == "900919352"
    assert bdata["model"] == "E5-2667v2"
    assert bdata["location"] == "msk"
    assert bdata["power"] == 1415.0

    assert "notfound_hosts" in result
    assert result["notfound_hosts"] == ["unexisting.host.yandex.ru"]


def test_group_slaves(api):
    result = api.api.get("/unstable/groups/MSK_WEB_BASE/slaves")

    assert "slaves" in result
    assert "MSK_WEB_INT" in result["slaves"]


@unstable_only
def test_host_instances_tags(api):
    result = api.api.get("/unstable/hosts/beatrice.search.yandex.net/instances_tags")

    assert "instances_tags" in result
    assert "beatrice.search.yandex.net:8041" in result["instances_tags"]

    arr = result["instances_tags"]["beatrice.search.yandex.net:8041"]
    assert "ALL_OSOL_HEAD_COMPILATION" in arr
    assert "a_geo_msk" in arr
    assert len(filter(lambda x: x.startswith("a_topology_version-trunk"), arr)) == 1
    assert "a_ctype_none" in arr
    assert "a_itype_none" in arr
    assert "a_prj_none" in arr


@unstable_only
def test_host_instances_tags_multi(api):
    group1 = dbpath_get_group(api, 'MSK_RESERVED', cached=False)
    group2 = dbpath_get_group(api, 'SAS_RESERVED', cached=True)

    UNEXISTING_HOST = "dsagsagagsda"

    hosts = [group1.getHosts()[0].name, group2.getHosts()[0].name]

    result = api.api.get("/unstable/hosts/%s/instances_tags" % ",".join(hosts + [UNEXISTING_HOST]))

    result_hosts = map(lambda x: x.partition(':')[0], result["instances_tags"].iterkeys())

    assert set(result_hosts) == set(hosts)
    assert result["not_found"] == [UNEXISTING_HOST]


@unstable_only
def test_intlookup_instances_by_shard(api):
    # FIXME: make fake intlookup
    result = api.api.get("/unstable/intlookups/MSK_DIVERSITY2_BASE/instances_by_shard")

    assert "instances_by_shard" in result
    assert len(result["instances_by_shard"]) == 10

    idict = result["instances_by_shard"][0][0]
    assert idict["dc"] == "iva"
    assert idict["domain"] == ".search.yandex.net"
    assert idict["hostname"] == "ws35-180.search.yandex.net"
    assert idict["location"] == "msk"
    assert idict["port"] == 23186
    assert idict["power"] == 341.5


# =============================== Create group stuff ===================================================

# somewhat copy wbe test funcs
TEST_GROUP_MASTER = "TEST_GROUP"
TEST_GROUP_SLAVE = "TEST_GROUP_SLAVE"
IDENTICAL_GROUPS_CHECK_FIELDS = set([
    ("owners",),
    ("description",),
    ("tags", "ctype"),
    ("tags", "itype"),
    ("tags", "prj"),
    ("properties", "nonsearch"),
    ("legacy", "funcs", "instancePort"),
])


def _create_group(api, groupname, params, return_full_response=False):
    response = api.api.post("/unstable/groups/%s/alloc" % groupname, params,
                            raise_bad_statuses=not return_full_response)

    if return_full_response:
        return response
    else:
        return response["added"]


def _remove_group(api, groupname):
    response = api.api.post("/unstable/groups/%s/remove" % groupname, {})

    return response["removed"]


def _get_free_hosts(api):
    reserved_group = dbpath_get_group(api, 'MSK_RESERVED', cached=False)
    return map(lambda x: x.name, reserved_group.getHosts())


@unstable_only
def test_create_master_group(api):
    try:
        owners = ["test_owner1", "test_owner2"]
        description = "Some description"
        ctype = "prod"
        itype = "base"
        prj = "web-main"
        metaprj = "web"
        hosts = _get_free_hosts(api)[:4]

        params = {
            "owners": owners,
            "description": description,
            "tags": {
                "ctype": ctype,
                "itype": itype,
                "prj": [prj],
                "metaprj": metaprj,
            },
            "hosts": hosts,
            "instanceCount": 2,
            "instancePort": 13579,
            "properties": "reqs.instances.memory_guarantee=1Gb",
        }

        groupname = _create_group(api, TEST_GROUP_MASTER, params)
        dbpath_group = dbpath_get_group(api, groupname, cached=False)

        assert dbpath_group.card.owners == owners
        assert dbpath_group.card.description == description
        assert dbpath_group.card.tags.itype == itype
        assert dbpath_group.card.tags.ctype == ctype
        assert dbpath_group.card.tags.prj == [prj]
        assert dbpath_group.card.tags.metaprj == metaprj
        assert set(map(lambda x: x.name, dbpath_group.getHosts())) == set(hosts)
        assert len(dbpath_group.getHosts()) * 2 == len(dbpath_group.get_instances())
        assert dbpath_group.get_instances()[0].port in [13579, 13580]

    finally:
        try:
            _remove_group(api, groupname)
        except:
            pass


@unstable_only
def test_create_master_group_wrong_hosts(api):
    description = "Some description"
    hosts = map(lambda x: x.name, dbpath_get_group(api, 'MSK_WEB_BASE', cached=False).getHosts())[:4]

    params = {
        "description": description,
        "hosts": hosts,
    }

    response = _create_group(api, TEST_GROUP_MASTER, params, return_full_response=True)
    assert response["error"].startswith("Can not add non-reserved hosts")


@unstable_only
def test_create_master_group_from_template(api):
    try:
        TEMPLATE_GROUP_NAME = "MSK_WEB_BASE"
        CUSTOM_DESCRIPTION = "Custom description"
        params = {
            "template_group": TEMPLATE_GROUP_NAME,
            "description": CUSTOM_DESCRIPTION
        }

        _create_group(api, TEST_GROUP_MASTER, params)

        dbpath_new_group = dbpath_get_group(api, TEST_GROUP_MASTER, cached=False)
        dbpath_template_group = dbpath_get_group(api, TEMPLATE_GROUP_NAME, cached=False)

        for field in IDENTICAL_GROUPS_CHECK_FIELDS:
            if field == ("description",):
                assert dbpath_new_group.card.get_card_value(field) == CUSTOM_DESCRIPTION
            else:
                assert dbpath_new_group.card.get_card_value(field) == dbpath_template_group.card.get_card_value(field)

    finally:
        _remove_group(api, TEST_GROUP_MASTER)


@unstable_only
def test_create_slave_group_from_template_with_donor(api):
    try:
        TEMPLATE_GROUP_NAME = "MSK_NEWS_NMETA"
        hosts = _get_free_hosts(api)[:4]

        master_params = {
            "description": "Master group",
            "hosts": hosts,
            "properties" : "properties.fake_group=True",
        }
        _create_group(api, TEST_GROUP_MASTER, master_params)

        slave_params = {
            "description": "Slave group",
            "template_group": TEMPLATE_GROUP_NAME,
            "parent_group": TEST_GROUP_MASTER,
            "donor_group": TEST_GROUP_MASTER,
            "properties" : "reqs.instances.memory_guarantee=1Gb",
        }
        _create_group(api, TEST_GROUP_SLAVE, slave_params)

        dbpath_slave_group = dbpath_get_group(api, TEST_GROUP_SLAVE, cached=False)
        dbpath_template_group = dbpath_get_group(api, TEMPLATE_GROUP_NAME, cached=True)

        for field in IDENTICAL_GROUPS_CHECK_FIELDS:
            if field != ("description",):
                assert dbpath_slave_group.card.get_card_value(field) == dbpath_template_group.card.get_card_value(field)

        assert dbpath_slave_group.card.host_donor == TEST_GROUP_MASTER
        assert dbpath_slave_group.card.master.card.name == TEST_GROUP_MASTER

    finally:
        try:
            _remove_group(api, TEST_GROUP_SLAVE)
        except:
            pass
        try:
            _remove_group(api, TEST_GROUP_MASTER)
        except:
            pass


@unstable_only
def test_create_slave_group_from_template_with_hosts(api):
    try:
        TEMPLATE_GROUP_NAME = "MSK_RESERVED"
        hosts = _get_free_hosts(api)[:4]
        slave_hosts = hosts[:2]

        master_params = {
            "description": "Master group",
            "hosts": hosts,
            "properties" : "properties.fake_group=True",
        }
        _create_group(api, TEST_GROUP_MASTER, master_params)

        slave_params = {
            "description": "Slave group",
            "template_group": TEMPLATE_GROUP_NAME,
            "parent_group": TEST_GROUP_MASTER,
            "hosts": slave_hosts,
            "properties" : "reqs.instances.memory_guarantee=1Gb",
            "instancePort" : 22333,
        }
        _create_group(api, TEST_GROUP_SLAVE, slave_params)

        dbpath_slave_group = dbpath_get_group(api, TEST_GROUP_SLAVE, cached=False)
        dbpath_template_group = dbpath_get_group(api, TEMPLATE_GROUP_NAME, cached=True)

        for field in IDENTICAL_GROUPS_CHECK_FIELDS:
            if field not in [("description",), ('legacy', 'funcs', 'instancePort')]:
                assert dbpath_slave_group.card.get_card_value(field) == dbpath_template_group.card.get_card_value(field)

        assert dbpath_slave_group.card.host_donor is None
        assert dbpath_slave_group.card.master.card.name == TEST_GROUP_MASTER
        assert set(map(lambda x: x.name, dbpath_slave_group.getHosts())) == set(slave_hosts)

    finally:
        try:
            _remove_group(api, TEST_GROUP_SLAVE)
        except:
            pass
        try:
            _remove_group(api, TEST_GROUP_MASTER)
        except:
            pass


# =============================== End of create group stuff ===================================================

# =============================== Start of card tests ==============================================================
@unstable_only
def test_get_group_card(api):
    result = api.api.get("/unstable/groups/MSK_RESERVED/card")

    assert result["name"] == "MSK_RESERVED"
    assert result["tags"]["ctype"] == "prod"
    assert "kimkim" in result["owners"]
    assert "on_update_trigger" in result
    assert "searcherlookup_postactions" in result
    assert "recluster" in result
    assert "walle" in result


# =============================== End of card tests ===========================================================

# =============================== Start of db state tests =====================================================
@unstable_only
def test_get_db_state(api):
    result = api.api.get("/unstable/db/state")

    assert "local_commit" in result
    assert "remote_commit" in result
    assert "short_descr" in result["local_commit"]
    assert result["remote_commit"]["commit_id"] - result["local_commit"]["commit_id"] >= 0


# =============================== End of db state tests =======================================================

# =============================== Start of ctype manipulation tests ===========================================
@unstable_only
def test_get_ctypes(api):
    result = api.api.get("/unstable/ctypes")

    localdb = get_db_by_path(api.db_path, cached=False)

    assert set(result["ctypes"]) == set(map(lambda x: x.name, localdb.ctypes.get_ctypes()))


@unstable_only
def test_get_ctype(api):
    api_result = api.api.get("/unstable/ctypes/prod")

    localdb = get_db_by_path(api.db_path, cached=False)
    local_result = localdb.ctypes.get_ctype("prod")

    assert api_result["name"] == local_result.name
    assert api_result["descr"] == local_result.descr


@unstable_only
def test_create_remove_ctype(api):
    CTYPE_NAME = "unexistingname"
    CTYPE_DESCR = "Unexisting ctype descr"

    # test create
    api_result = api.api.post("/unstable/ctypes/%s/create" % CTYPE_NAME, {"descr": CTYPE_DESCR})

    localdb = get_db_by_path(api.db_path, cached=False)
    local_result = localdb.ctypes.get_ctype(CTYPE_NAME)

    assert api_result["created"] == CTYPE_NAME
    assert local_result.name == CTYPE_NAME
    assert local_result.descr == CTYPE_DESCR

    # test remove
    api_result = api.api.post("/unstable/ctypes/%s/remove" % CTYPE_NAME, {})

    localdb = get_db_by_path(api.db_path, cached=False)

    assert api_result["removed"] == CTYPE_NAME
    assert not localdb.ctypes.has_ctype(CTYPE_NAME)

# =============================== End of ctype manipulation tests =============================================

# =============================== Start of /groups_by_tag tests ===============================================
def test_groups_by_tag_succ(api):
    query = 'a_ctype_prod'
    api_result = api.api.get("/unstable/groups_by_tag?query=%s" % (urllib.quote(query)))
    assert 'MSK_RESERVED' in api_result['master_groups']

    query = '(a_prj_imgs-main or a_prj_imgs-quick) and (a_itype_int or a_itype_base)'
    api_result = api.api.get("/unstable/groups_by_tag?query=%s" % (urllib.quote(query)))
    assert 'MSK_IMGS_BASE' in api_result['master_groups']
    assert 'MSK_IMGS_INT' in api_result['slave_groups']
    assert 'ALL_IMGS_QUICK_BASE_PRIEMKA' in api_result['slave_groups']

    query = '(a_prj_imgs-main or a_prj_imgs-quick) and (a_itype_int or a_itype_base) and a_ctype_prestable'
    api_result = api.api.get("/unstable/groups_by_tag?query=%s" % (urllib.quote(query)))
    assert 'SAS_IMGS_BASE' in api_result['master_groups']
    assert 'SAS_IMGS_INT' in api_result['slave_groups']

def test_groups_by_tag_fail(api):
    query = 'a_itype_base and unexisting_keyword and a_ctype_prod'
    api_result = api.api.get("/unstable/groups_by_tag?query=%s" % (urllib.quote(query)), raise_bad_statuses=False)
    assert api_result["error"] == "Can not parse <unexisting_keyword> as itype, ctype or prj"
# =============================== Start of /groups_by_tag tests ===============================================


def test_branch_not_found(api):
    result = api.api.get("/unknown_branch/searcherlookup/groups/NON_EXISTING_GROUP/instances", ret_raw_result=True,
                         raise_bad_statuses=False)

    assert result.status_code == 404

# ========================== GENCFG-678 START ==========================
def test_build_status(api):
    """
        Test if checker for current build status works or not
    """

    result = api.api.get("/unstable/build_status")

    assert "last_success_commit" in result
    assert "last_commit" in result
    assert "build_status" in result

    assert result["last_commit"]["testinfo"]["status"] in ("SUCCESS", "FAILURE")
    assert result["build_status"] in (True, False)
# ========================== GENCFG-678 FINISH =========================
