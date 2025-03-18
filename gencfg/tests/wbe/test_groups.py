"""Tests groups."""

from __future__ import unicode_literals

import os
import copy
import datetime

from tests.util import get_contents, get_yaml, get_db_by_path
from core.settings import SETTINGS

import pytest

unstable_only = pytest.mark.unstable_only

IDENTICAL_GROUPS_CHECK_FIELDS = set([
    ("owners",),
    ("description",),
    ("tags", "ctype"),
    ("tags", "itype"),
    ("tags", "prj"),
    ("properties", "nonsearch"),
    ("legacy", "funcs", "instancePort"),
])


def __test_create_slave_group(wbe, is_acceptor):
    master_name = _create_group(wbe, extra_params={"properties": "properties.fake_group=True",})

    slave_name = "TEST_GROUP_SLAVE"
    assert not _dbpath_has_group(wbe, slave_name, cached=False)

    wbe.api.check_response_status(wbe.api.post("/unstable/groups", {
        "action": "addgroup",
        "group": slave_name,
        "parent_group": master_name,
        "host_donor": master_name if is_acceptor else None,
        "properties": "reqs.instances.memory_guarantee=1Gb",
        "metaprj": "web",
    }))

    dbpath_slave_group = _dbpath_get_group(wbe, slave_name, cached=False)

    assert dbpath_slave_group.card.master.card.name == master_name
    assert dbpath_slave_group.card.host_donor == (master_name if is_acceptor else None)

    _remove_group(wbe, master_name)


def __find_child_in_card(card, child_path):
    """
        Find dict with path == child_path in card structure by recurse traversing
    """
    if isinstance(card, dict):
        if card.get("path", None) == child_path:
            print "FOUND", card

            return card
        for v in card.itervalues():
            subresult = __find_child_in_card(v, child_path)
            if subresult is not None:
                return subresult
    elif isinstance(card, list):
        for v in card:
            subresult = __find_child_in_card(v, child_path)
            if subresult is not None:
                return subresult
    else:
        return None
    return None


@unstable_only
def test_create_master_group(wbe):
    try:
        owners = ["test_owner1", "test_owner2"]
        description = "Some description"

        extra_params = {
            "owners": ",".join(owners),
            "description": description,
            "template_group": "",
            "properties": "properties.fake_group=True",
        }

        groupname = _create_group(wbe, extra_params=extra_params)
        dbpath_group = _dbpath_get_group(wbe, groupname, cached=False)

        assert dbpath_group.card.owners == owners
        assert dbpath_group.card.description == description
        assert dbpath_group.card.tags.itype == "none"
        assert dbpath_group.card.tags.ctype == "none"
        assert dbpath_group.card.tags.prj == ["none"]
        assert dbpath_group.card.tags.metaprj == "web"

    finally:
        try:
            _remove_group(wbe, groupname)
        except:
            pass


@unstable_only
def test_create_temporary_group_without_name(wbe):
    try:
        owners = ["test_owner1", "test_owner2"]
        description = "Some description"

        params = {
            "owners": owners,
            "description": description,
            "action": "addtempgroup",
            "expires": 30,
            "metaprj": "web",
        }

        response = wbe.api.post("/unstable/groups", params)
        groupname = response["group"]

        assert groupname.find("_EXPERIMENT_") > 0

        dbpath_group = _dbpath_get_group(wbe, groupname, cached=False)

        assert dbpath_group.card.owners == owners
        assert dbpath_group.card.description == description
        assert dbpath_group.card.tags.itype == "none"
        assert dbpath_group.card.tags.ctype == "none"
        assert dbpath_group.card.tags.prj == ["none"]
        assert dbpath_group.card.tags.metaprj == "web"
        assert dbpath_group.card.properties.expires.date - datetime.date.today() == datetime.timedelta(30)
    finally:
        _remove_group(wbe, groupname)

        assert not _dbpath_has_group(wbe, groupname, cached=False)


@unstable_only
def test_create_temporary_group_with_name(wbe):
    try:
        groupname = _create_group(wbe, extra_params={"expires": 30, "properties": "reqs.instances.memory_guarantee=1Gb"}, action_name="addtempgroup")

        dbpath_group = _dbpath_get_group(wbe, groupname, cached=False)

        assert dbpath_group.card.properties.expires.date - datetime.date.today() == datetime.timedelta(30)
    finally:
        _remove_group(wbe, groupname)
        assert not _dbpath_has_group(wbe, groupname, cached=False)


@unstable_only
def test_create_master_group_from_template(wbe):
    template_name = "MSK_WEB_BASE"
    group_name = _create_group(wbe, extra_params={"template_group": template_name, "properties": "reqs.instances.memory_guarantee=1Gb"})

    try:
        dbpath_template_group = _dbpath_get_group(wbe, template_name, cached=False)
        dbpath_new_group = _dbpath_get_group(wbe, group_name, cached=False)

        for field in IDENTICAL_GROUPS_CHECK_FIELDS:
            assert dbpath_new_group.card.get_card_value(field) == dbpath_template_group.card.get_card_value(field)
    finally:
        _remove_group(wbe, group_name)


@unstable_only
def test_create_master_group_from_template_with_params(wbe):
    template_name = "MSK_WEB_BASE"
    group_name = _create_group(wbe, extra_params={"template_group": template_name, "instance_port_func": "new12345", "description": "Other description", "properties": "reqs.instances.memory_guarantee=1Gb"})

    try:
        dbpath_template_group = _dbpath_get_group(wbe, template_name, cached=False)
        dbpath_new_group = _dbpath_get_group(wbe, group_name, cached=False)

        for field in IDENTICAL_GROUPS_CHECK_FIELDS:
            if field == ("description",):
                assert dbpath_new_group.card.get_card_value(field) == "Other description"
            elif field == ("legacy", "funcs", "instancePort"):
                assert dbpath_new_group.card.get_card_value(field) == "new12345"
            else:
                assert dbpath_new_group.card.get_card_value(field) == dbpath_template_group.card.get_card_value(field)
    finally:
        _remove_group(wbe, group_name)


@unstable_only
def test_create_master_group_from_template_and_parent(wbe):
    template_name = "MSK_WEB_BASE"
    group_name = _create_group(wbe, extra_params={"template_group": template_name, "parent_group": "MSK_RESERVED", "properties": "reqs.instances.memory_guarantee=1Gb"})

    try:
        dbpath_template_group = _dbpath_get_group(wbe, template_name, cached=False)
        dbpath_new_group = _dbpath_get_group(wbe, group_name, cached=True)

        for field in IDENTICAL_GROUPS_CHECK_FIELDS:
            assert dbpath_new_group.card.get_card_value(field) == dbpath_template_group.card.get_card_value(field)
        assert dbpath_new_group.card.master.card.name == "MSK_RESERVED"
        assert dbpath_new_group.card.host_donor is None
    finally:
        _remove_group(wbe, group_name)


@unstable_only
def test_create_master_group_from_parent_and_donor(wbe):
    donor_name = "ALL_GENCFG_NEW"
    group_name = _create_group(wbe, extra_params={"parent_group": donor_name, "donor_group": donor_name,
                                                  "instance_port_func": "old5555", "properties": "reqs.instances.memory_guarantee=1Gb"})

    try:
        dbpath_donor_group = _dbpath_get_group(wbe, donor_name, cached=False)
        dbpath_new_group = _dbpath_get_group(wbe, group_name, cached=True)

        assert dbpath_new_group.card.master.card.name == donor_name
        assert dbpath_new_group.card.host_donor == donor_name
        assert len(dbpath_new_group.getHosts()) == len(dbpath_donor_group.getHosts())
    finally:
        _remove_group(wbe, group_name)


@unstable_only
def test_create_master_group_from_template_and_parent_and_donor(wbe):
    template_name = "MSK_RESERVED"
    donor_name = "MSK_NEWS_NMETA"
    group_name = _create_group(wbe, extra_params={"template_group": template_name, "parent_group": donor_name,
                                                  "donor_group": donor_name, "properties" : "reqs.instances.memory_guarantee=1Mb"})

    try:
        dbpath_template_group = _dbpath_get_group(wbe, template_name, cached=False)
        dbpath_donor_group = _dbpath_get_group(wbe, donor_name, cached=True)
        dbpath_new_group = _dbpath_get_group(wbe, group_name, cached=True)

        for field in IDENTICAL_GROUPS_CHECK_FIELDS:
            assert dbpath_new_group.card.get_card_value(field) == dbpath_template_group.card.get_card_value(field)
        assert dbpath_new_group.card.master.card.name == donor_name
        assert len(dbpath_new_group.getHosts()) == len(dbpath_donor_group.getHosts())
    finally:
        _remove_group(wbe, group_name)


@unstable_only
def test_create_master_group_from_template_and_parent_and_donor_fail(wbe):
    template_name = "SAS_RESERVED"
    response = _create_group(wbe, extra_params={"template_group": template_name, "parent_group": "MSK_RESERVED",
                                                "donor_group": "MSK_RESERVED", "properties": "reqs.instances.memory_guarantee=1Gb"}, return_full_response=True)

    wbe.api.check_response_status(response, ok=False)
    assert response["exception info"]["class"] == "UtilNormalizeException"
    assert response["exception info"]["params"][
               "message"] == "Created group instances intersects with one of template/parent/donor group"


@unstable_only
def test_remove_to_acceptor(wbe):
    sample_group = "TEST_REMOVE_GROUP"
    reserved_group = "MSK_RESERVED"
    acceptor_group = "MSK_RESERVED_HIDDEN"

    # prepare sample group with host
    _create_group(wbe, group_name=sample_group, extra_params={"properties": "properties.full_host_group=True"})
    target_host = _dbpath_get_group(wbe, reserved_group, cached=False).getHosts()[0].name
    add_request = {
        "action": "add",
        "group": sample_group,
        "hosts": [target_host],
    }
    wbe.api.check_response_status(wbe.api.post("/unstable/groups/%s/hosts" % sample_group, add_request))

    # remove group and move host to acceptor
    remove_request = {
        "action": "remove",
        "group": sample_group,
        "acceptor": acceptor_group,
        "recursive": True,
    }
    wbe.api.check_response_status(wbe.api.post("/unstable/groups", remove_request))

    # check constraints
    dbpath_acceptor_group = _dbpath_get_group(wbe, acceptor_group, cached=False)
    assert target_host in map(lambda x: x.name, dbpath_acceptor_group.getHosts())

    # move target host to reserved_group
    move_host_request = {
        "action": "add",
        "group": reserved_group,
        "hosts": [target_host],
    }
    wbe.api.check_response_status(wbe.api.post("/unstable/groups/%s/hosts" % reserved_group, move_host_request))


@unstable_only
def test_create_empty_slave_group(wbe):
    __test_create_slave_group(wbe, False)


@unstable_only
def test_create_acceptor_slave_group(wbe):
    __test_create_slave_group(wbe, True)


@unstable_only
def test_create_acceptor_slave_group(wbe):
    pass


def test_get(wbe):
    groupname = "MSK_WEB_BASE"

    response = wbe.api.get("/unstable/groups/{}".format(groupname))

    assert response["group"] == groupname


def test_get_card(wbe):
    groupname = "MSK_WEB_BASE"
    response = wbe.api.get("/unstable/groups/{}/card".format(groupname))

    assert response["group"] == groupname
    assert "card" in response

    dbpath_group = _dbpath_get_group(wbe, groupname)

    assert __find_child_in_card(response["card"], ["name"])["value"] == dbpath_group.card.name
    assert __find_child_in_card(response["card"], ["owners"])["value"] == dbpath_group.card.owners
    assert __find_child_in_card(response["card"], ["tags", "ctype"])["value"] == dbpath_group.card.tags.ctype
    assert __find_child_in_card(response["card"], ["tags", "itype"])["value"] == dbpath_group.card.tags.itype
    assert __find_child_in_card(response["card"], ["tags", "prj"])["value"] == dbpath_group.card.tags.prj
    assert __find_child_in_card(response["card"], ["tags", "metaprj"])["value"] == dbpath_group.card.tags.metaprj


def test_get_all(wbe):
    response = wbe.api.get("/unstable/groups")

    assert "groups" in response


@unstable_only
def test_update(wbe):
    try:
        groupname = _create_group(wbe, extra_params={"properties": "reqs.instances.memory_guarantee=1Gb"})

        response = wbe.api.post("/unstable/groups/{}/card".format(groupname), {
            "action": "update",
            "card": [{"path": ["reqs", "instances", "memory_guarantee"], "value": "12 Gb"}],
        })

        wbe.api.check_response_status(response)

        dbpath_group = _dbpath_get_group(wbe, groupname, cached=False)

        assert dbpath_group.card.reqs.instances.memory_guarantee.gigabytes() == 12

    finally:
        try:
            _remove_group(wbe, groupname)
        except:
            pass


@unstable_only
def test_update_instance_port_func(wbe):
    groupname = 'MSK_RESERVED'

    response = wbe.api.post("/unstable/groups/{}/card".format(groupname), {
        "action": "update",
        "card": [
            {"path": ["legacy", "funcs", "instancePort"], "value": "new12345"},
            {"path": ["reqs", "instances", "memory_guarantee"], "value": "12 Gb"},
        ],
    })

    wbe.api.check_response_status(response)

    dbpath_group = _dbpath_get_group(wbe, groupname, cached=False)

    assert dbpath_group.card.legacy.funcs.instancePort == "new12345"
    assert dbpath_group.card.reqs.instances.memory_guarantee.gigabytes() == 12
    assert dbpath_group.get_instances()[0].port == 12345

    wbe.api.post("/unstable/groups/{}/card".format(groupname), {
        "action": "update",
        "card": [
            {"path": ["legacy", "funcs", "instancePort"], "value": "old8041"},
            {"path": ["reqs", "instances", "memory"], "value": "0 Gb"},
        ],
    })

@unstable_only
def test_update_instance_port_func_fail(wbe):
    groupname = 'MSK_RESERVED'
    response = wbe.api.post("/unstable/groups/{}/card".format(groupname), {
        "action": "update",
        "card": [
            {"path": ["legacy", "funcs", "instancePort"], "value": "azaza"},
        ],
    })

    wbe.api.check_response_status(response, ok=False)
    assert response["exception info"]["class"] == "UtilNormalizeException"
    assert response["exception info"]["params"]["message"] == "Invalid port_func value azaza"


@unstable_only
def test_update_instance_count_func(wbe):
    """
        Change instance count function
    """

    try:
        group_name = "TEST_UPDATE_INSTANCE_COUNT_FUNC"

        # create group and add couple of hosts to it
        _create_group(wbe, group_name = group_name)
        msk_reserved_hosts = wbe.api.get("/unstable/groups/MSK_RESERVED")["hosts"]
        add_hosts_request = {
            "action": "add",
            "hosts": msk_reserved_hosts[:10],
        }
        wbe.api.post("/unstable/groups/%s/hosts" % group_name, add_hosts_request)

        before_instances = len(_dbpath_get_group(wbe, group_name, cached=False).get_instances())

        # change instance count func
        response = wbe.api.post("/unstable/groups/{}/card".format(group_name), {
            "action": "update",
            "card": [
                {"path": ["legacy", "funcs", "instanceCount"], "value": "exactly3"},
            ],
        })
        wbe.api.check_response_status(response)

        after_instances = len(_dbpath_get_group(wbe, group_name, cached=False).get_instances())

        assert before_instances > 0
        assert after_instances == before_instances * 3
    finally:
        _remove_group(wbe, group_name)


@unstable_only
def test_update_unexisting_onwer_fail(wbe):
    groupname = 'MSK_RESERVED'
    user = 'unexisting_user'
    response = wbe.api.post("/unstable/groups/{}/card".format(groupname), {
        "action": "update",
        "card": [
            {"path": ["general", "owners"], "value": user},
        ],
    })

    wbe.api.check_response_status(response, ok=False)
    assert response["invalid values"][0]["key"] == ["owners"]
    assert response["invalid values"][0][
               "error"] == "Do not know how to convert value of type <<type 'str'>> to list type"


@unstable_only
def test_update_itype(wbe):
    groupname = 'MSK_RESERVED'
    mname = 'base'
    path = ["tags", "itype"]

    dbpath_group = _simple_update_group_card(wbe, groupname, path, mname, return_dbgroup=True)

    assert dbpath_group.card.tags.itype == mname

    _simple_update_group_card(wbe, groupname, path, "none")


@unstable_only
def test_update_itype_fail(wbe):
    groupname = 'MSK_RESERVED'
    mname = 'unexisting_itype'
    path = ["tags", "itype"]

    response = _simple_update_group_card(wbe, groupname, path, mname, check_failed=True, return_response=True)

    assert response["invalid values"][0]["key"] == path
    assert response["invalid values"][0]["error"] == "Unknown itype <%s>" % mname


@unstable_only
def test_update_ctype(wbe):
    groupname = 'MSK_RESERVED'
    mname = 'prestable'
    path = ["tags", "ctype"]

    dbpath_group = _simple_update_group_card(wbe, groupname, path, mname, return_dbgroup=True)

    assert dbpath_group.card.tags.ctype == mname

    _simple_update_group_card(wbe, groupname, path, "prod")


@unstable_only
def test_update_ctype_fail(wbe):
    groupname = "MSK_RESERVED"
    mname = "unexisting_ctype"
    path = ["tags", "ctype"]

    response = _simple_update_group_card(wbe, groupname, path, mname, check_failed=True, return_response=True)

    assert response["invalid values"][0]["key"] == path
    assert response["invalid values"][0]["error"] == "Unknown ctype <%s>" % mname


@unstable_only
def test_update_prj(wbe):
    groupname = "MSK_RESERVED"
    mname = ["base", "base2"]
    path = ["tags", "prj"]

    dbpath_group = _simple_update_group_card(wbe, groupname, path, mname, return_dbgroup=True)

    assert dbpath_group.card.tags.prj == mname

    _simple_update_group_card(wbe, groupname, path, ["reserve"])


@unstable_only
def test_update_prj_fail(wbe):
    groupname = "MSK_RESERVED"
    mname = ["goodprj", "bad_prj"]
    path = ["tags", "prj"]

    response = _simple_update_group_card(wbe, groupname, path, mname, check_failed=True, return_response=True)

    assert response["invalid values"][0]["key"] == path
    assert response["invalid values"][0][
               "error"] == "Group prj name <bad_prj> does not satisfy prj regexp <^[a-z][a-z0-9-]+$>"


@unstable_only
def test_update_metaprj(wbe):
    groupname = "MSK_RESERVED"
    mname = "video"
    path = ["tags", "metaprj"]

    dbpath_group = _simple_update_group_card(wbe, groupname, path, mname, return_dbgroup=True)

    assert dbpath_group.card.tags.metaprj == mname

    _simple_update_group_card(wbe, groupname, path, "reserve")


@unstable_only
def test_update_metaprj_fail(wbe):
    groupname = 'MSK_RESERVED'
    mname = 'unexisting_metaprj'
    path = ["tags", "metaprj"]

    response = _simple_update_group_card(wbe, groupname, path, mname, check_failed=True, return_response=True)

    assert response["invalid values"][0]["key"] == path
    assert response["invalid values"][0]["error"] == "Unknown metaprj <%s>" % mname


@unstable_only
def test_rename(wbe):
    oldname = _create_group(wbe, extra_params={"description": "Non-empty description", "properties": "properties.full_host_group=True"})
    dbpath_old_group = _dbpath_get_group(wbe, oldname, cached=False)

    newname = "TEST_RENAMED_GROUP"
    wbe.api.post("/unstable/groups", {
        "action": "rename",
        "group": oldname,
        "new_group": newname,
    })

    dbpath_new_group = _dbpath_get_group(wbe, newname, cached=False)

    assert not _dbpath_has_group(wbe, oldname, cached=False)
    assert dbpath_new_group.card.description == dbpath_old_group.card.description
    assert dbpath_new_group.card.owners == dbpath_old_group.card.owners
    assert dbpath_new_group.card.tags.prj == dbpath_old_group.card.tags.prj

    _remove_group(wbe, newname)


@unstable_only
def test_rename_reserved(wbe):
    response = wbe.api.post("/unstable/groups", {
        "action": "rename",
        "group": "MSK_RESERVED",
        "new_group": "MSK_NOT_RESERVED",
    })

    wbe.api.check_response_status(response, ok=False)
    assert response["exception info"]["class"] == "Exception"


@unstable_only
def test_remove_missing(wbe):
    name = "TEST GROUP"

    wbe.api.check_response_status(wbe.api.post("/unstable/groups", {
        "action": "remove",
        "group": name,
    }), ok=False)

    assert not _dbpath_has_group(wbe, name, cached=False)


# strange function (should not work)
@unstable_only
def _test_free_hosts(wbe):
    groupname = _create_group(wbe, extra_params={"properties": "properties.full_host_group=True"})

    # TODO: add intlookup

    msk_reserved_hosts = wbe.api.get("/unstable/groups/MSK_RESERVED")["hosts"]
    assert msk_reserved_hosts
    target_host = sorted(msk_reserved_hosts)[0]
    add_request = {
        'action': 'add',
        'source': 'MSK_RESERVED',
        'hosts': [target_host],
    }
    wbe.api.post("/unstable/groups/%s/hosts" % groupname, add_request)

    assert (target_host in wbe.api.get("/unstable/groups/%s" % groupname)["hosts"])

    free_request = {
        "action": "remove",
        "group": groupname,
        "verbose": True,
        "apply": True,
        "free_hosts": True
    }
    wbe.api.post("/unstable/groups/%s" % groupname, free_request)

    assert (target_host not in wbe.api.get("/unstable/groups/%s" % groupname)["hosts"])

    _remove_group(wbe, groupname)


@unstable_only
def test_build_reset_intlookup(wbe):
    # preapare group
    groupname = _create_group(wbe, extra_params={"properties": "properties.full_host_group=True"})

    msk_reserved_hosts = wbe.api.get("/unstable/groups/MSK_RESERVED")["hosts"]
    add_hosts_request = {
        "action": "add",
        "hosts": msk_reserved_hosts,
    }
    wbe.api.post("/unstable/groups/%s/hosts" % groupname, add_hosts_request)

    # build intlookup
    create_intlookup_request = {
        "action": "build_simple_intlookup",
        "groups": groupname,
        "output_file": "",
        "bases_per_group": "1",
        "shard_count": "OneShardTier0,OneShardTier0",
        "verbose": True,
        "apply": True,
        "rewrite_intlookup": True,
    }
    wbe.api.check_response_status(wbe.api.post("/unstable/groups/%s" % groupname, create_intlookup_request))

    # reset intlokup
    reset_intlookup_request = {
        "action": "reset",
        "group": groupname,
        "reset_method": "reset_brigade_groups",
        "verbose": True,
        "apply": True,
        "rewrite_intlookup": True,
    }
    wbe.api.check_response_status(wbe.api.post("/unstable/groups/%s" % groupname, reset_intlookup_request))

    _remove_group(wbe, groupname)


@unstable_only
def test_group_slave_form_page(wbe):
    groupname = _create_group(wbe, extra_params={"description": "Some description", "owners": "test_owner1,test_owner2", "properties": "properties.full_host_group=True"})

    uri = "/unstable/group_slave_form_page?srcgroup=%s&owners=1&description=1&tags.ctype=1&tags.prj=1&reqs.instances.memory_guarantee=1" % groupname
    response = wbe.api.get(uri)

    dbpath_group = _dbpath_get_group(wbe, groupname, cached=False)

    assert (response["update_master_form_fields"]["owners"] == dbpath_group.card.owners)
    assert (response["update_master_form_fields"]["description"] == dbpath_group.card.description)
    assert (response["update_master_form_fields"]["tags/ctype"] == dbpath_group.card.tags.ctype)
    assert (response["update_master_form_fields"]["tags/prj"] == dbpath_group.card.tags.prj)
    assert (response["update_master_form_fields"]["reqs/instances/memory_guarantee"] == "0 Gb")

    _remove_group(wbe, groupname)


@unstable_only
def test_util_slave_form_page(wbe):
    groupname = _create_group(wbe, extra_params={"description": "Some description", "owners": "test_owner1,test_owner2", "properties": "reqs.instances.memory_guarantee=1Mb"})

    try:
        # test allocate_group_in_dynamic
        uri = "/unstable/util_slave_form_page?srcgroup=%s&util_name=allocate_group_in_dynamic&owners=1&description=1&ctype=1&prj=1&memory=1" % groupname
        response = wbe.api.get(uri)

        dbpath_group = _dbpath_get_group(wbe, groupname, cached=False)

        assert (response["update_master_form_fields"]["owners"] == ",".join(dbpath_group.card.owners))
        assert (response["update_master_form_fields"]["description"] == dbpath_group.card.description)
        assert (response["update_master_form_fields"]["ctype"] == dbpath_group.card.tags.ctype)
        assert (response["update_master_form_fields"]["prj"] == ",".join(dbpath_group.card.tags.prj))
        assert (response["update_master_form_fields"]["memory"] == "1Mb")

    finally:
        _remove_group(wbe, groupname)


@unstable_only
def test_set_portovm_itype(wbe):
    """
        Test if setting itype to portovm creates guest group with hosts
    """

    group_name = "MSK_DIVERSITY_JUPITER_BASE"
    guest_group_name = "MSK_DIVERSITY_JUPITER_BASE_GUEST"

    try:
        # check set itype to portovm
        response = wbe.api.post("/unstable/groups/{}/card".format(group_name), {
            "action": "update",
            "card": [{"path": ["tags", "itype"], "value": SETTINGS.constants.portovm.itype}],
        })
        wbe.api.check_response_status(response)

        dbpath_group = _dbpath_get_group(wbe, group_name, cached=False)
        assert dbpath_group.card.tags.itype == SETTINGS.constants.portovm.itype

        dbpath_guest_group = _dbpath_get_group(wbe, guest_group_name, cached=False)
        assert len(dbpath_guest_group.getHosts()) == len(dbpath_group.get_instances())

        # check set itype back to initial itype
        response = wbe.api.post("/unstable/groups/{}/card".format(group_name), {
            "action": "update",
            "card": [{"path": ["tags", "itype"], "value": "base"}],
        })
        wbe.api.check_response_status(response)

        dbpath_guest_group = _dbpath_get_group(wbe, guest_group_name, cached=False)
        assert len(dbpath_guest_group.getHosts()) == 0
    finally:
        _remove_group(wbe, guest_group_name)


@unstable_only
def test_change_memory_type(wbe):
    """
        Every group can be in one of three states memorywise:
            - fake group
            - full host group
            - group with memory limits set
        Thus, when we change group card (such as changing group type from <fake group> to <full host group> we should update all
        flags, related to group type.
    """

    try:
        group_name = "TEST_CHANGE_MEMORY_TYPE"
        extra_params = { "properties": "properties.fake_group=True" }

        # create group and check its memory type
        _create_group(wbe, group_name=group_name, extra_params=extra_params)
        dbpath_group = _dbpath_get_group(wbe, group_name, cached=False)
        assert dbpath_group.card.reqs.instances.memory_guarantee.value == 0
        assert dbpath_group.card.properties.fake_group == True
        assert dbpath_group.card.properties.full_host_group == False

        # try to change to full host group
        response = wbe.api.post("/unstable/groups/{}/card".format(group_name), {
            "action": "update",
            "card": [{"path": ["properties", "full_host_group"], "value": True}],
        })
        wbe.api.check_response_status(response)

        dbpath_group = _dbpath_get_group(wbe, group_name, cached=False)
        assert dbpath_group.card.reqs.instances.memory_guarantee.value == 0
        assert dbpath_group.card.properties.fake_group == False
        assert dbpath_group.card.properties.full_host_group == True
        # try to change to group with memory limits
        response = wbe.api.post("/unstable/groups/{}/card".format(group_name), {
            "action": "update",
            "card": [{"path": ["reqs", "instances", "memory_guarantee"], "value": "12 Gb"}],
        })
        wbe.api.check_response_status(response)
        dbpath_group = _dbpath_get_group(wbe, group_name, cached=False)
        assert dbpath_group.card.reqs.instances.memory_guarantee.gigabytes() == 12
        assert dbpath_group.card.properties.fake_group == False
        assert dbpath_group.card.properties.full_host_group == False

        # try to change back to fake group
        response = wbe.api.post("/unstable/groups/{}/card".format(group_name), {
            "action": "update",
            "card": [{"path": ["properties", "fake_group"], "value": True}],
        })
        wbe.api.check_response_status(response)
        dbpath_group = _dbpath_get_group(wbe, group_name, cached=False)
        assert dbpath_group.card.reqs.instances.memory_guarantee.value == 0
        assert dbpath_group.card.properties.fake_group == True
        assert dbpath_group.card.properties.full_host_group == False
    finally:
        _remove_group(wbe, group_name)

# ================================== GENCFG-543 START ==========================================
@unstable_only
def test_update_card_after_fail_update(wbe):
    """
        Usually card update is performed in the fololowing way:
            - chagne group card
            - perform smart update (like instanceCount/instancePort)
            - commit data.
        When we catch exception, group becomes corrupted and we have to rebuild it. In this test
        we make incorrect modification, catch error and make modify again.
    """

    try:
        group_name = "TEST_UPDATE_CARD_AFTER_FAIL_UPDATE"

        # create group and add some hosts
        _create_group(wbe, group_name=group_name)
        msk_reserved_hosts = wbe.api.get("/unstable/groups/MSK_RESERVED")["hosts"]
        add_hosts_request = {
            "action": "add",
            "hosts": msk_reserved_hosts[:10],
        }
        wbe.api.post("/unstable/groups/%s/hosts" % group_name, add_hosts_request)

        # make incorrect card update
        try:
            response = wbe.api.post("/unstable/groups/{}/card".format(group_name), {
                    "action": "update",
                    "card": [{"path": ["reqs", "instances", "memory_guarantee"], "value": "10000 Gb"}],
                })
            wbe.api.check_response_status(response, ok=False)
        except:
            pass

        # make correct changes
        response = wbe.api.post("/unstable/groups/{}/card".format(group_name), {
                "action": "update",
                "card": [{"path": ["description"], "value": "Some description"}],
            })
        wbe.api.check_response_status(response)
    finally:
        _remove_group(wbe, group_name)

# ================================== GENCFG-543 FINISH =========================================


def _create_group(wbe, group_name="TEST_GROUP", extra_params={"properties" : "reqs.instances.memory_guarantee=1Gb"}, return_full_response=False, action_name="addgroup"):
    params = {
        "group": group_name,
        "owners": "",
        "description": "",
        "action": action_name,
        "metaprj": "web",
    }
    if extra_params is not None:
        params.update(extra_params)

    response = wbe.api.post("/unstable/groups", params)

    if return_full_response:
        return response
    else:
        wbe.api.check_response_status(response)
        return response["group"]


def _remove_group(wbe, groupname):
    wbe.api.check_response_status(wbe.api.post("/unstable/groups", {
        "action": "remove",
        "group": groupname,
        "recursive": True,
    }))


#    _check_no_group(wbe, groupname)

def _get_group_card(wbe, groupname):
    response = wbe.api.get("/unstable/groups/{}/card".format(groupname))

    wbe.api.check_response_status(response)
    assert response["group"] == group_name
    assert "card" in result

    return card


def _simple_update_group_card(wbe, groupname, path, value, check_failed=False, return_dbgroup=False,
                              return_response=False):
    response = wbe.api.post("/unstable/groups/{}/card".format(groupname), {
        "action": "update",
        "card": [
            {"path": path, "value": value},
        ],
    })

    wbe.api.check_response_status(response, ok=not check_failed)

    if return_dbgroup:
        return _dbpath_get_group(wbe, groupname, cached=False)
    elif return_response:
        return response
    else:
        return None


def _check_no_group(wbe, name):
    assert not os.path.exists(_get_group_path(wbe, name))


def _check_group(wbe, group):
    path = _get_group_path(wbe, group["name"])

    assert os.path.exists(path)
    assert set(os.listdir(path)) == {group["name"] + ".hosts", "card.yaml"}

    assert not get_contents(os.path.join(path, group["name"] + ".hosts")).strip()
    assert get_yaml(os.path.join(path, "card.yaml")) == group


def _get_group_path(wbe, name):
    return os.path.join(wbe.db_path, "groups", name)


def _get_groups(wbe):
    names = []
    groups_path = os.path.join(wbe.db_path, "groups")

    for name in os.listdir(groups_path):
        if name.startswith("."):
            continue

        names.append(name)
        for group in get_yaml(os.path.join(groups_path, name, "card.yaml")).get("slaves", []):
            names.append(group["name"])

    names.sort()
    assert len(names) == len(set(names))

    return names


"""
    Functions to get group/tiers other stuff params directly from db by creating DB and using standard function.
    This method looks better for testing because wbe functions return result in strange format
"""


def _dbpath_get_group(wbe, groupname, cached=True):
    db = get_db_by_path(wbe.db_path, cached=cached)
    return db.groups.get_group(groupname)


def _dbpath_has_group(wbe, groupname, cached=True):
    db = get_db_by_path(wbe.db_path, cached=cached)
    return db.groups.has_group(groupname)
