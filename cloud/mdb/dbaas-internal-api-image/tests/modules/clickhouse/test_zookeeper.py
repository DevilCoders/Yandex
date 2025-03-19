"""
Tests for ZooKeeper module.
"""
from hamcrest import assert_that, contains_inanyorder

from dbaas_internal_api.modules.clickhouse.traits import ClickhouseRoles
from dbaas_internal_api.modules.clickhouse.zookeeper import __name__ as CREATE_PACKAGE
from dbaas_internal_api.modules.clickhouse.zookeeper import _choice_zk_hosts


def _choice_first(values):
    if not values:
        raise IndexError
    return sorted(values)[0]


def zk_hosts(zone_ids):
    """
    Generate CH host specs by list of zone ids.
    """
    return [{'zone_id': zone_id, 'assign_public_ip': False, 'type': ClickhouseRoles.zookeeper} for zone_id in zone_ids]


class Test_choice_zk_hosts_geo:
    get_geo_func = CREATE_PACKAGE + '.get_available_geo'
    random = CREATE_PACKAGE + '.random.SystemRandom.choice'

    def test_use_ch_geo(self, mocker):
        mocker.patch(self.get_geo_func, return_value=list('abcdef'))

        assert_that(_choice_zk_hosts(3, set('abc')), contains_inanyorder(*zk_hosts('abc')))

    def test_choice_different_geo_if_ch_nodes_created_in_one_geo(self, mocker):
        mocker.patch(self.get_geo_func, return_value=list('abc'))
        mocker.patch(self.random, side_effect=_choice_first)

        assert_that(_choice_zk_hosts(3, set('aa')), contains_inanyorder(*zk_hosts('abc')))

    def test_choice_random_from_available(self, mocker):
        mocker.patch(self.get_geo_func, return_value=list('abcdef'))
        mocker.patch(self.random, side_effect=_choice_first)

        assert_that(_choice_zk_hosts(5, set('ab')), contains_inanyorder(*zk_hosts('abcde')))

    def test_reuse_ch_hosts_geo_when_no_more_avaiable(self, mocker):
        mocker.patch(self.get_geo_func, return_value=list('ab'))
        mocker.patch(self.random, side_effect=_choice_first)

        assert_that(_choice_zk_hosts(5, set('ab')), contains_inanyorder(*zk_hosts('aaaab')))
