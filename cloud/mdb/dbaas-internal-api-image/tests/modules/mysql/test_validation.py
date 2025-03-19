"""
MySQL cluster validation
"""
import pytest

from dbaas_internal_api.core.exceptions import DbaasClientError
from dbaas_internal_api.modules.mysql import utils

# pylint: disable=missing-docstring, invalid-name

GB = 1024**3

old_config = {
    'mdb_offline_mode_enable_lag': 86400,
    'lower_case_table_names': 0,
    'innodb_page_size': 0,
}


class Tests_validate_mysql_config:
    def test_innodb_less_than_80_percent_of_memory_guarantee_on_large_flavour(self):
        flavor = {'memory_guarantee': 8 * GB, 'cpu_guarantee': 1}
        config = {'innodb_buffer_pool_size': 6 * GB}
        utils.validate_options(old_config, config, None, flavor, 3)

    def test_innodb_equal_80_percent_of_memory_guarantee_on_Large_flavour(self):
        flavor = {'memory_guarantee': 8 * GB, 'cpu_guarantee': 1}
        config = {'innodb_buffer_pool_size': int(0.8 * 8 * GB)}
        utils.validate_options(old_config, config, None, flavor, 3)

    def test_innodb_greater_than_80_percent_of_memory_guarantee_large_flavour(self):
        with pytest.raises(DbaasClientError):
            flavor = {'memory_guarantee': 8 * GB, 'cpu_guarantee': 1}
            config = {'innodb_buffer_pool_size': 7 * GB}
            utils.validate_options(old_config, config, None, flavor, 3)

    def test_innodb_ok_on_2g_flavour(self):
        flavor = {'memory_guarantee': 2 * GB, 'cpu_guarantee': 1}
        config = {'innodb_buffer_pool_size': 0.5 * GB}
        utils.validate_options(old_config, config, None, flavor, 3)

    def test_innodb_not_ok_on_2g_flavour(self):
        with pytest.raises(DbaasClientError):
            flavor = {'memory_guarantee': 2 * GB, 'cpu_guarantee': 1}
            config = {'innodb_buffer_pool_size': 0.5 * GB + 1}
            utils.validate_options(old_config, config, None, flavor, 3)

    def test_innodb_ok_on_4g_flavour(self):
        flavor = {'memory_guarantee': 4 * GB, 'cpu_guarantee': 1}
        config = {'innodb_buffer_pool_size': 2.5 * GB}
        utils.validate_options(old_config, config, None, flavor, 3)

    def test_innodb_not_ok_on_4g_flavour(self):
        with pytest.raises(DbaasClientError):
            flavor = {'memory_guarantee': 2 * GB, 'cpu_guarantee': 1}
            config = {'innodb_buffer_pool_size': 2.5 * GB + 1}
            utils.validate_options(old_config, config, None, flavor, 3)

    def test_offline_mode_lag_1(self):
        with pytest.raises(
            DbaasClientError, match="mdb_offline_mode_enable_lag must be greater than mdb_offline_mode_disable_lag"
        ):
            flavor = {'memory_guarantee': 2 * GB, 'cpu_guarantee': 1}
            # disable_lag should be lower than default enable_lag (when enable_lag not specified)
            config = {'mdb_offline_mode_disable_lag': 100000}
            utils.validate_options(old_config, config, "any", flavor, 3)

    def test_offline_mode_lag_2(self):
        with pytest.raises(
            DbaasClientError, match="mdb_offline_mode_enable_lag must be greater than mdb_offline_mode_disable_lag"
        ):
            flavor = {'memory_guarantee': 2 * GB, 'cpu_guarantee': 1}
            # disable_lag should be lower than enable_lag
            config = {'mdb_offline_mode_disable_lag': 1000, 'mdb_offline_mode_enable_lag': 1000}
            utils.validate_options(old_config, config, "any", flavor, 3)

    def test_max_connections_depends_on_memory_too_much(self):
        with pytest.raises(DbaasClientError):
            flavor = {'memory_guarantee': 8 * GB, 'cpu_guarantee': 1}
            config = {'max_connections': 1024 + 1}
            utils.validate_options(old_config, config, None, flavor, 3)

    def test_max_connections_depends_on_memory_below_limit(self):
        flavor = {'memory_guarantee': 8 * GB, 'cpu_guarantee': 1}
        config = {'max_connections': 1024}
        utils.validate_options(old_config, config, None, flavor, 3)


class Tests_validate_mysql_config_changes:
    def test_lower_case_table_names_not_changed(self):
        new_config = {'lower_case_table_names': 0}
        utils.validate_config_changes(old_config=old_config, new_config=new_config)

    def test_lower_case_table_names_changed(self):
        with pytest.raises(DbaasClientError):
            new_config = {'lower_case_table_names': 1}
            utils.validate_config_changes(old_config=old_config, new_config=new_config)

    def test_innodb_page_size_changed(self):
        with pytest.raises(DbaasClientError):
            new_config = {'innodb_page_size': 1}
            utils.validate_config_changes(old_config=old_config, new_config=new_config)
