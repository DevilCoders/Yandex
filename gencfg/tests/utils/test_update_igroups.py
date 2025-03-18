import gencfg
from utils.common import update_igroups
from core.exceptions import UtilNormalizeException, TValidateCardNodeError

from tests.util import get_db_by_path


def test_add_group_with_wrong_user(curdb):
    util_options = {
        "action": "addgroup",
        "group": "UNEXISTING_GROUP32424",
        "owners": "unexisting_user",
    }

    try:
        update_igroups.jsmain(util_options)
    except TValidateCardNodeError, e:
        assert e.msg == "Group UNEXISTING_GROUP32424: Users <unexisting_user> are not found or do not belong to admin department"


def test_manipulate_with_slave_move_to_another_master(testdb_path):
    try:
        master1_name = "TEST_IGROUPS_CASE1_MASTER1"
        master2_name = "TEST_IGROUPS_CASE1_MASTER2"
        slave_name = "TEST_IGROUPS_CASE1_SLAVE"

        util_options = {
            "action": "manipulatewithslave",
            "group": slave_name,
            "parent_group": master2_name,
            "donor_group": master2_name,
            "db": testdb_path,
        }

        update_igroups.jsmain(util_options)
        testdb = get_db_by_path(testdb_path, cached=False)
        master1 = testdb.groups.get_group(master1_name)

        assert len(master1.slaves) == 0
    finally:
        util_options = {
            "action": "manipulatewithslave",
            "group": slave_name,
            "parent_group": master1_name,
            "donor_group": master1_name,
            "db": testdb_path,
        }
        update_igroups.jsmain(util_options)


def test_remove_hosts_from_group_with_intlookup_succ(testdb_path):
    GROUPNAME = "TEST_REMOVE_HOSTS_FROM_GROUP_WITH_INTLOOKUP_SUCC"
    HOSTNAME = "test_remove_hosts_from_group_with_intlookup_succ"

    try:
        util_options = {
            "action": "movehosts",
            "group": "RESERVED",
            "hosts": HOSTNAME,
            "db": testdb_path,
        }
        update_igroups.jsmain(util_options)

        testdb = get_db_by_path(testdb_path, cached=False)
        group = testdb.groups.get_group(GROUPNAME)
        intlookup = testdb.intlookups.get_intlookup(group.card.intlookups[0])

        assert len(group.getHosts()) == 0
        assert len(intlookup.get_base_instances()) == 0
    finally:
        util_options = {
            "action": "movehosts",
            "group": GROUPNAME,
            "hosts": HOSTNAME,
            "db": testdb_path,
        }
        try:
            update_igroups.jsmain(util_options)
        except:
            pass


def test_remove_hosts_from_group_with_intlookup_fail(testdb_path):
    # noinspection PyUnusedLocal
    GROUPNAME = "TEST_REMOVE_HOSTS_FROM_GROUP_WITH_INTLOOKUP_FAIL"
    HOSTNAME = "test_remove_hosts_from_group_with_intlookup_fail"

    util_options = {
        "action": "movehosts",
        "group": "RESERVED",
        "hosts": HOSTNAME,
        "db": testdb_path,
    }

    try:
        update_igroups.jsmain(util_options)
    except Exception, e:
        assert str(
            e) == "Cannot remove hosts from group <TEST_REMOVE_HOSTS_FROM_GROUP_WITH_INTLOOKUP_FAIL>: intlookup <TEST_REMOVE_HOSTS_FROM_GROUP_WITH_INTLOOKUP_FAIL> have <2> hosts per group"


def test_add_portovm_group1(testdb_path):
    """
        Test for automatic adding portovm guest group on adding host to group with
        itype <portovm>
    """

    try:
        GROUP_NAME = "TEST_ADD_PORTOVM_GROUP1"
        GUEST_GROUP_NAME = "TEST_ADD_PORTOVM_GROUP1_GUEST"
        HOST_NAME = "test_add_portovm_group.search.yandex.net"
        GUEST_HOST_NAME = "test_add_portovm_group-8041.vm.search.yandex.net"

        util_options = {
            "action": "movehosts",
            "group": GROUP_NAME,
            "hosts": [HOST_NAME],
            "db": testdb_path,
        }

        update_igroups.jsmain(util_options)

        testdb = get_db_by_path(testdb_path, cached=False)

        assert testdb.groups.has_group(GUEST_GROUP_NAME)

        guest_group = testdb.groups.get_group(GUEST_GROUP_NAME)

        assert testdb.hosts.has_host(GUEST_HOST_NAME)
        assert guest_group.hasHost(testdb.hosts.get_host_by_name(GUEST_HOST_NAME))
    finally:
        testdb = get_db_by_path(testdb_path, cached=False)
        testdb.get_repo().reset_all_changes(remove_unversioned=True)


def test_manipulatewithslave_remove_donor_portovm(testdb_path):
    try:
        TPL = "TEST_MANIPULATEWITHSLAVE_REMOVE_DONOR_PORTOVM"
        MASTER_NAME = "%s_MASTER" % TPL
        SLAVE_NAME = "%s_SLAVE" % TPL
        GUEST_NAME = "%s_SLAVE_GUEST" % TPL
        GUEST_HOST_NAME = "test_manipulatewithslave_remove_donor_portovm-8041.vm.search.yandex.net"

        util_options = {
            "action": "manipulatewithslave",
            "group": SLAVE_NAME,
            "parent_group": MASTER_NAME,
            "db": testdb_path,
        }

        update_igroups.jsmain(util_options)

        testdb = get_db_by_path(testdb_path, cached=False)

        assert not testdb.hosts.has_host(GUEST_HOST_NAME)
        assert len(testdb.groups.get_group(GUEST_NAME).getHosts()) == 0
        assert testdb.groups.get_group(SLAVE_NAME).card.host_donor is None
    finally:
        testdb = get_db_by_path(testdb_path, cached=False)
        testdb.get_repo().reset_all_changes(remove_unversioned=True)


def test_manipulatewithslave_remove_donor_keep_hosts_portovm(testdb_path):
    try:
        TPL = "TEST_MANIPULATEWITHSLAVE_REMOVE_DONOR_KEEP_HOSTS_PORTOVM"
        MASTER_NAME = "%s_MASTER" % TPL
        SLAVE_NAME = "%s_SLAVE" % TPL
        GUEST_NAME = "%s_SLAVE_GUEST" % TPL
        GUEST_HOST_NAME = "test_manipulatewithslave_remove_donor_keep_hosts_portovm-8041.vm.search.yandex.net"

        util_options = {
            "action": "manipulatewithslave",
            "group": SLAVE_NAME,
            "parent_group": MASTER_NAME,
            "keep_hosts": True,
            "db": testdb_path,
        }

        update_igroups.jsmain(util_options)

        testdb = get_db_by_path(testdb_path, cached=False)

        assert testdb.hosts.has_host(GUEST_HOST_NAME)
        assert testdb.groups.get_group(GUEST_NAME).getHosts()[0].name == GUEST_HOST_NAME
        assert testdb.groups.get_group(SLAVE_NAME).card.host_donor is None
    finally:
        testdb = get_db_by_path(testdb_path, cached=False)
        testdb.get_repo().reset_all_changes(remove_unversioned=True)


def test_add_slave_with_donor(testdb_path):
    try:
        TPL = "TEST_ADD_SLAVE_WITH_DONOR"
        MASTER_NAME = "%s_MASTER" % TPL
        SLAVE_NAME = "%s_SLAVE" % TPL

        util_options = {
            "action": "addgroup",
            "group": SLAVE_NAME,
            "parent_group": MASTER_NAME,
            "donor_group": MASTER_NAME,
            "db": testdb_path,
        }

        update_igroups.jsmain(util_options)

        testdb = get_db_by_path(testdb_path, cached=False)

        assert testdb.groups.has_group(SLAVE_NAME)
        assert testdb.groups.get_group(SLAVE_NAME).card.master.card.name == MASTER_NAME
        assert testdb.groups.get_group(SLAVE_NAME).card.host_donor == MASTER_NAME
    finally:
        testdb = get_db_by_path(testdb_path, cached=False)
        testdb.get_repo().reset_all_changes(remove_unversioned=True)

def test_remove_group_after_fail_create(testdb_path):
    """
        When creaing group we can encounter some exception already after group was created. Thus we should remove improperly created group from db
    """

    try:
        gname = "TEST_REMOVE_GROUP_AFTER_FAIL_CREATE"

        # try to create group and fail
        util_options = {
            "action": "addgroup",
            "group": gname,
            "properties" : "reqs.instances.memory_guarantee=sfasdf",
            "db": get_db_by_path(testdb_path, cached=False),
        }
        try:
            update_igroups.jsmain(util_options)
        except Exception:
            pass

        # create group with correct params
        util_options["properties"] = "reqs.instances.memory_guarantee=1Gb"
        update_igroups.jsmain(util_options)

        testdb = get_db_by_path(testdb_path, cached=False)

        assert testdb.groups.has_group(gname)
        assert testdb.groups.get_group(gname).card.reqs.instances.memory_guarantee.value == 1024 * 1024 * 1024
    finally:
        testdb = get_db_by_path(testdb_path, cached=False)
        testdb.get_repo().reset_all_changes(remove_unversioned=True)

def test_remove_group_after_fail_create2(testdb_path):
    """
        When creaing group we can encounter some exception already after group was created. Thus we should remove improperly created group from db
    """

    try:
        testdb = get_db_by_path(testdb_path, cached=False)

        gname = "TEST_REMOVE_GROUP_AFTER_FAIL_CREATE"
        util_options = {
                "action": "addgroup",
                "group": gname,
                "properties" : "reqs.instances.memory_guarantee=100Gb",
                "db": testdb,
                "hosts" : ["test_remove_group_after_fail_create.search.yandex.net"],
        }

        try:
            update_igroups.jsmain(util_options)
        except Exception:
            pass

        util_options["properties"] = "reqs.instances.memory_guarantee=10Gb"
        update_igroups.jsmain(util_options)

        testdb = get_db_by_path(testdb_path, cached=False)
        assert testdb.groups.has_group(gname)
        assert testdb.groups.get_group(gname).card.reqs.instances.memory_guarantee.value == 10 * 1024 * 1024 * 1024
    finally:
        testdb = get_db_by_path(testdb_path, cached=False)
        testdb.get_repo().reset_all_changes(remove_unversioned=True)

def test_move_background_group_hosts(testdb_path):
    gname = "TEST_MOVE_BACKGROUND_GROUP_HOSTS"
    sgname = "SAMPLE_GROUP"
    bgname = "TEST_MOVE_BACKGROUND_GROUP_HOSTS_BACKGROUND"
    hostname = "test_move_background_group_hosts.search.yandex.net"

    testdb = get_db_by_path(testdb_path, cached=False)

    # test that adding host to background group do not remove this host from normal one
    util_options = {
        "action": "movehosts",
        "group": bgname,
        "hosts": [hostname],
        "db": testdb,
    }
    update_igroups.jsmain(util_options)

    testdb = get_db_by_path(testdb_path, cached=False)

    assert "test_move_background_group_hosts.search.yandex.net" in map(lambda x: x.name, testdb.groups.get_group(gname).getHosts())

    # test that moving hosts from normal group do not touch background one
    util_options = {
        "action": "movehosts",
        "group": sgname,
        "hosts": [hostname],
        "db": testdb,
    }
    update_igroups.jsmain(util_options)

    testdb = get_db_by_path(testdb_path, cached=False)

    assert "test_move_background_group_hosts.search.yandex.net" in map(lambda x: x.name, testdb.groups.get_group(bgname).getHosts())

    # test that emptying background group do not remove hosts from normal one groups
    util_options = {
        "action": "emptygroup",
        "group": bgname,
        "db": testdb,
    }
    update_igroups.jsmain(util_options)

    testdb = get_db_by_path(testdb_path, cached=False)

    assert "test_move_background_group_hosts.search.yandex.net" in map(lambda x: x.name, testdb.groups.get_group(sgname).getHosts())
