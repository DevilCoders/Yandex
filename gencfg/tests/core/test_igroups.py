from tests.util import get_db_by_path

def _create_test_group(curdb, group_name, extra_params=None):
    assert (not curdb.groups.has_group(group_name))

    params = dict(
        description="Some description",
        owners=["owner1", "owner2"],
        watchers=["watcher1", "watcher2"],
        instance_count_func="i12g",
        instance_power_func="zero",
        instance_port_func="new8000",
        master=None,
        donor=None,
        tags=None,
        expires=None,
    )

    if extra_params is not None:
        params.update(extra_params)

    group = curdb.groups.add_group(group_name, **params)

    return group


def _remove_test_group(curdb, group_name):
    assert (curdb.groups.has_group(group_name))

    curdb.groups.remove_group(group_name)


def test_create_remove_group(curdb):
    test_group = _create_test_group(curdb, "PYTEST_GROUP")

    assert (test_group.card.name == "PYTEST_GROUP")
    assert (curdb.groups.has_group(test_group.card.name))

    _remove_test_group(curdb, test_group.card.name)

    assert (not curdb.groups.has_group(test_group.card.name))

def test_memory_guarantee_validator1(testdb_path):
    testdb = get_db_by_path(testdb_path, cached=False)

    group = testdb.groups.get_group("TEST_MEMORY_GUARANTREE_VALIDATOR_SLAVE2")
    group.mark_as_modified()

    testdb.groups.check_group_cards_on_update(smart=True)

def test_memory_guarantee_validator2(testdb_path):
    testdb = get_db_by_path(testdb_path, cached=False)

    group = testdb.groups.get_group("TEST_MEMORY_GUARANTREE_VALIDATOR_SLAVE2")
    group.card.reqs.instances.memory_guarantee.reinit("21 Gb")
    group.mark_as_modified()

    try:
        testdb.groups.check_group_cards_on_update(smart=True)
    except Exception, e:
        assert str(e) == 'Group TEST_MEMORY_GUARANTREE_VALIDATOR_SLAVE2: Host test_memory_guarantee_validator.search.yandex.net has <42.04 Gb> memory, while needed <46.00 Gb>'
