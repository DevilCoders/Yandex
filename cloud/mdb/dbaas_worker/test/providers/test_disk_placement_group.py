from cloud.mdb.dbaas_worker.internal.providers.disk_placement_group import name_for_dpg


def test_name_for_dpg():
    assert len(name_for_dpg('c9qbui90r38iad2003fv', 0, 'ru-central1-c', shard_id='c9qbb4or89ipiqqt7num')) < 64
    assert len(name_for_dpg('c9qbui90r38iad2003fv', 0, 'ru-central1-c', subcid='c9qbb4or89ipiqqt7num')) < 64
    assert (
        name_for_dpg('c9qbui90r38iad2003fv', 0, 'ru-central1-c', shard_id='c9qbb4or89ipiqqt7num')
        == 'c9qbui90r38iad2003fv-c9qbb4or89ipiqqt7num-ru-central1-c-0'
    )
    assert (
        name_for_dpg('c9qbui90r38iad2003fv', 0, 'ru-central1-c', subcid='c9qbb4or89ipiqqt7num')
        == 'c9qbui90r38iad2003fv-c9qbb4or89ipiqqt7num-ru-central1-c-0'
    )
