from utils.api import searcherlookup_groups_instances

from tests.util import get_db_by_path

def test_get_storages_info(testdb_path):
    mydb = get_db_by_path(testdb_path)

    params = {
        'groups' : ['TEST_GET_STORAGES_INFO1', 'TEST_GET_STORAGES_INFO2', 'TEST_GET_STORAGES_INFO3'],
        'db' : testdb_path
    }

    result = searcherlookup_groups_instances.jsmain(params)

    group1 = mydb.groups.get_group('TEST_GET_STORAGES_INFO1')
    group2 = mydb.groups.get_group('TEST_GET_STORAGES_INFO2')
    group3 = mydb.groups.get_group('TEST_GET_STORAGES_INFO3')

    assert result[group1.card.name]['instances'][0]['storages']['rootfs']['size'] == group1.card.reqs.instances.disk.gigabytes()
    assert result[group1.card.name]['instances'][0]['storages']['rootfs']['partition'] == 'hdd'
    assert result[group2.card.name]['instances'][0]['storages']['rootfs']['size'] == group2.card.reqs.instances.ssd.gigabytes()
    assert result[group2.card.name]['instances'][0]['storages']['rootfs']['partition'] == 'ssd'
    assert result[group3.card.name]['instances'][0]['storages']['rootfs']['size'] == group3.card.reqs.instances.disk.gigabytes()
    assert result[group3.card.name]['instances'][0]['storages']['rootfs']['partition'] == 'hdd'
    assert result[group3.card.name]['instances'][0]['storages']['ssd']['size'] == group3.card.reqs.instances.ssd.gigabytes()
    assert result[group3.card.name]['instances'][0]['storages']['ssd']['partition'] == 'ssd'

